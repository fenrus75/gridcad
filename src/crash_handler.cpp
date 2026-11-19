/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#define UNW_LOCAL_ONLY
#include "crash_handler.h"
#include "logger.h"

#include <libunwind.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

namespace crash_handler
{

int crash_fd = -1;
static int reserved_fd = -1;
static char crash_filepath[PATH_MAX + 1] = "";
static char crashprocess_path[PATH_MAX + 1] = "";

// Dedicated alternate signal stack so stack-overflow SIGSEGV can still be handled.
static unsigned char alt_signal_stack[2 * 1024 * 1024];

// Thread-local breadcrumb buffer. Reads from fallback_signal_handler are async-signal-safe.
static thread_local char current_breadcrumb[128] = "";

void set_breadcrumb(std::string_view breadcrumb)
{
	size_t len = std::min(breadcrumb.size(), sizeof(current_breadcrumb) - 1);
	std::memcpy(current_breadcrumb, breadcrumb.data(), len);
	current_breadcrumb[len] = '\0';
}

void clear_breadcrumb()
{
	current_breadcrumb[0] = '\0';
}

scoped_breadcrumb::scoped_breadcrumb(std::string_view breadcrumb)
{
	std::memcpy(prev_, current_breadcrumb, sizeof(prev_));
	set_breadcrumb(breadcrumb);
}

scoped_breadcrumb::~scoped_breadcrumb()
{
	std::memcpy(current_breadcrumb, prev_, sizeof(current_breadcrumb));
}

bool is_debugger_attached()
{
	std::ifstream status_file("/proc/self/status");
	if (!status_file.is_open()) {
		return false;
	}

	std::string line;
	while (std::getline(status_file, line)) {
		if (line.starts_with("TracerPid:")) {
			std::string_view value_view(line);
			value_view.remove_prefix(10);
			while (!value_view.empty() && (value_view.front() == ' ' || value_view.front() == '\t')) {
				value_view.remove_prefix(1);
			}
			int pid = 0;
			auto [ptr, ec] = std::from_chars(value_view.data(), value_view.data() + value_view.size(), pid);
			if (ec == std::errc{} && pid != 0) {
				return true;
			}
			break;
		}
	}
	return false;
}

static size_t safe_strlen(const char *s)
{
	size_t len = 0;
	while (s && s[len]) {
		len++;
	}
	return len;
}

static std::filesystem::path get_crash_dir()
{
	namespace fs = std::filesystem;
	if (const char *xdg = std::getenv("XDG_CACHE_HOME"); xdg && xdg[0] != '\0') {
		return fs::path(xdg) / "gridcad" / "crashes";
	}
	if (const char *home = std::getenv("HOME"); home && home[0] != '\0') {
		return fs::path(home) / ".cache" / "gridcad" / "crashes";
	}
	return fs::path("/tmp") / "gridcad" / "crashes";
}

static void resolve_crashprocess_path()
{
	namespace fs = std::filesystem;
	std::error_code ec;

	// 1. Try sibling of current running executable
	char exe_buf[PATH_MAX + 1];
	ssize_t len = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);
	if (len > 0) {
		exe_buf[len] = '\0';
		fs::path local_helper = fs::path(exe_buf).parent_path() / "turbostar-crashprocess";
		if (fs::exists(local_helper, ec) && access(local_helper.c_str(), X_OK) == 0) {
			std::strncpy(crashprocess_path, local_helper.c_str(), sizeof(crashprocess_path) - 1);
			crashprocess_path[sizeof(crashprocess_path) - 1] = '\0';
			return;
		}
	}

	// 2. Fallback to binary name for PATH lookup via execlp
	std::strncpy(crashprocess_path, "turbostar-crashprocess", sizeof(crashprocess_path) - 1);
	crashprocess_path[sizeof(crashprocess_path) - 1] = '\0';
}

static void safe_itoa(long val, char *buf, int buf_size)
{
	if (buf_size < 2)
		return;
	if (val == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}

	char temp[32];
	int i = 0;
	bool neg = false;
	unsigned long uval = 0;
	if (val < 0) {
		neg = true;
		uval = static_cast<unsigned long>(-(val + 1)) + 1;
	} else {
		uval = static_cast<unsigned long>(val);
	}

	while (uval > 0 && i < 31) {
		temp[i++] = static_cast<char>((uval % 10) + '0');
		uval /= 10;
	}

	if (neg && i < 31) {
		temp[i++] = '-';
	}

	int j = 0;
	while (i > 0 && j < buf_size - 1) {
		buf[j++] = temp[--i];
	}
	buf[j] = '\0';
}

static void safe_hex_toa(unsigned long val, char *buf, int buf_size)
{
	if (buf_size < 2)
		return;
	if (val == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}

	char temp[32];
	int i = 0;
	const char *hex_chars = "0123456789abcdef";

	while (val > 0 && i < 31) {
		temp[i++] = hex_chars[val % 16];
		val /= 16;
	}

	int j = 0;
	while (i > 0 && j < buf_size - 1) {
		buf[j++] = temp[--i];
	}
	buf[j] = '\0';
}

static void safe_hex_16(unsigned long val, char *buf, int buf_size)
{
	if (buf_size < 17)
		return;
	const char *hex_chars = "0123456789abcdef";
	for (int i = 15; i >= 0; --i) {
		buf[i] = hex_chars[val & 0xf];
		val >>= 4;
	}
	buf[16] = '\0';
}

static void ensure_crash_file_open_signal_safe()
{
	if (crash_fd != -1 || safe_strlen(crash_filepath) == 0) {
		return;
	}
	int fd = open(crash_filepath, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	if (fd == -1 && errno == EMFILE && reserved_fd != -1) {
		close(reserved_fd);
		reserved_fd = -1;
		fd = open(crash_filepath, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	}
	if (fd != -1) {
		crash_fd = fd;
	}
}

static void safe_write(const char *msg)
{
	if (!msg)
		return;
	size_t len = safe_strlen(msg);
	if (len == 0)
		return;
	[[maybe_unused]] ssize_t r1 = write(STDERR_FILENO, msg, len);
	ensure_crash_file_open_signal_safe();
	if (crash_fd != -1) {
		[[maybe_unused]] ssize_t r2 = write(crash_fd, msg, len);
	}
}

static void setup_crash_file()
{
	namespace fs = std::filesystem;
	try {
		fs::path crash_dir = get_crash_dir();

		auto now = fs::file_time_type::clock::now();
		if (fs::exists(crash_dir)) {
			for (auto &p : fs::directory_iterator(crash_dir)) {
				std::error_code ec;
				if (p.is_regular_file(ec) && fs::file_size(p.path(), ec) == 0) {
					auto last_write = fs::last_write_time(p.path(), ec);
					if (!ec && (now - last_write) > std::chrono::hours(1)) {
						fs::remove(p.path(), ec);
					}
				}
			}
		} else {
			fs::create_directories(crash_dir);
		}

		// Pre-allocate reserved_fd so we can still open a crash file on EMFILE.
		if (reserved_fd == -1) {
			reserved_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
		}

		resolve_crashprocess_path();

		pid_t pid = getpid();
		long now_sec = static_cast<long>(std::time(nullptr));
		std::string path_str = (crash_dir / ("crash_" + std::to_string(pid) + "_" + std::to_string(now_sec) + ".txt")).string();
		if (path_str.size() < sizeof(crash_filepath)) {
			std::strncpy(crash_filepath, path_str.c_str(), sizeof(crash_filepath) - 1);
			crash_filepath[sizeof(crash_filepath) - 1] = '\0';
			crash_fd = -1;
		}
	} catch (...) {
		// Ignore filesystem errors during crash handler setup.
	}
}

static void fallback_terminate_handler()
{
	std::string exc_info = "Unknown uncaught exception";
	std::exception_ptr exc_ptr = std::current_exception();
	if (exc_ptr) {
		try {
			std::rethrow_exception(exc_ptr);
		} catch (const std::exception &e) {
			exc_info = std::string("Uncaught std::exception: ") + e.what();
		} catch (...) {
			exc_info = "Uncaught unknown exception type";
		}
	}

	ensure_crash_file_open_signal_safe();
	pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
	char comm_buf[32] = {0};
	int comm_fd = open("/proc/thread-self/comm", O_RDONLY | O_CLOEXEC);
	if (comm_fd != -1) {
		ssize_t n = read(comm_fd, comm_buf, sizeof(comm_buf) - 1);
		close(comm_fd);
		if (n > 0) {
			while (n > 0 && (comm_buf[n - 1] == '\n' || comm_buf[n - 1] == '\r')) {
				comm_buf[--n] = '\0';
			}
		}
	}
	std::string thread_info = std::format("Thread: {}", tid);
	if (comm_buf[0] != '\0') {
		thread_info += std::format(" ({})", comm_buf);
	}
	thread_info += "\n";
	if (current_breadcrumb[0] != '\0') {
		thread_info += std::format("Breadcrumb: {}\n", current_breadcrumb);
	}

	std::string msg = "\n*** GridCad Uncaught Exception ***\n" + exc_info + "\n" + thread_info;
	if (crash_fd != -1) {
		[[maybe_unused]] ssize_t r1 = write(crash_fd, msg.c_str(), msg.length());
	}
	[[maybe_unused]] ssize_t r2 = write(STDERR_FILENO, msg.c_str(), msg.length());

	std::abort();
}

static void fallback_signal_handler(int sig, siginfo_t *info, void *ucontext)
{
	int is_write = 0;
	int si_code = 0;
	int is_addr_valid = 0;
	if (info) {
		si_code = info->si_code;
		if (sig == SIGSEGV || sig == SIGBUS || sig == SIGFPE || sig == SIGILL) {
			is_addr_valid = 1;
		}
#if defined(__x86_64__)
		if (sig == SIGSEGV || sig == SIGBUS) {
			ucontext_t *uc = reinterpret_cast<ucontext_t *>(ucontext);
			if (uc) {
				unsigned long err_code = static_cast<unsigned long>(uc->uc_mcontext.gregs[REG_ERR]);
				if (err_code & 0x02) {
					is_write = 1;
				}
			}
		}
#endif
	}

	safe_write("\n*** GridCad Crash Catcher ***\nCaught signal: ");

	char sig_buf[16];
	safe_itoa(sig, sig_buf, sizeof(sig_buf));
	safe_write(sig_buf);

	const char *sig_name = " (Unknown)";
	if (sig == SIGSEGV) {
		sig_name = " (SIGSEGV - Segmentation Fault)";
	} else if (sig == SIGABRT) {
		sig_name = " (SIGABRT - Aborted)";
	} else if (sig == SIGFPE) {
		sig_name = " (SIGFPE - Floating Point Exception)";
	} else if (sig == SIGILL) {
		sig_name = " (SIGILL - Illegal Instruction)";
	} else if (sig == SIGBUS) {
		sig_name = " (SIGBUS - Bus Error)";
	}
	safe_write(sig_name);
	safe_write("\n");

	if (is_addr_valid && info) {
		safe_write("CrashAddress: 0x");
		char addr_str[32];
		safe_hex_toa(reinterpret_cast<unsigned long>(info->si_addr), addr_str, sizeof(addr_str));
		safe_write(addr_str);
		if (sig == SIGSEGV || sig == SIGBUS) {
			safe_write(is_write ? " (write)" : " (read)");
		}
		safe_write("\n");
	}

	if (sig == SIGSEGV) {
		if (si_code == SEGV_MAPERR) {
			safe_write("Type: SEGV_MAPERR\n");
		} else if (si_code == SEGV_ACCERR) {
			safe_write("Type: SEGV_ACCERR\n");
		}
	}

	safe_write("Thread: ");
	pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
	char tid_buf[16];
	safe_itoa(static_cast<long>(tid), tid_buf, sizeof(tid_buf));
	safe_write(tid_buf);

	char comm_buf[32] = {0};
	int comm_fd = open("/proc/thread-self/comm", O_RDONLY | O_CLOEXEC);
	if (comm_fd != -1) {
		ssize_t n = read(comm_fd, comm_buf, sizeof(comm_buf) - 1);
		close(comm_fd);
		if (n > 0) {
			while (n > 0 && (comm_buf[n - 1] == '\n' || comm_buf[n - 1] == '\r')) {
				comm_buf[--n] = '\0';
			}
		}
	}
	if (comm_buf[0] != '\0') {
		safe_write(" (");
		safe_write(comm_buf);
		safe_write(")");
	}
	safe_write("\n");

	if (current_breadcrumb[0] != '\0') {
		safe_write("Breadcrumb: ");
		safe_write(current_breadcrumb);
		safe_write("\n");
	}

	char exe_path[PATH_MAX + 1];
	ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (len > 0) {
		exe_path[len] = '\0';
		safe_write("Executable: ");
		safe_write(exe_path);
		safe_write("\n");
	}

#if defined(__x86_64__)
	ucontext_t *uc = reinterpret_cast<ucontext_t *>(ucontext);
	if (uc) {
		safe_write("\nCPU Registers:\n");
		struct reg_desc {
			const char *name;
			int reg_idx;
		};
		static const reg_desc regs[] = {
			{"  RAX: 0x", REG_RAX}, {"  RBX: 0x", REG_RBX},
			{"  RCX: 0x", REG_RCX}, {"  RDX: 0x", REG_RDX},
			{"  RSI: 0x", REG_RSI}, {"  RDI: 0x", REG_RDI},
			{"  RBP: 0x", REG_RBP}, {"  RSP: 0x", REG_RSP},
			{"  R8:  0x", REG_R8},  {"  R9:  0x", REG_R9},
			{"  R10: 0x", REG_R10}, {"  R11: 0x", REG_R11},
			{"  R12: 0x", REG_R12}, {"  R13: 0x", REG_R13},
			{"  R14: 0x", REG_R14}, {"  R15: 0x", REG_R15},
			{"  RIP: 0x", REG_RIP}, {"  EFL: 0x", REG_EFL},
		};
		char r_buf[32];
		for (size_t i = 0; i < sizeof(regs) / sizeof(regs[0]); i += 2) {
			safe_write(regs[i].name);
			safe_hex_16(static_cast<unsigned long>(uc->uc_mcontext.gregs[regs[i].reg_idx]), r_buf, sizeof(r_buf));
			safe_write(r_buf);

			if (i + 1 < sizeof(regs) / sizeof(regs[0])) {
				safe_write(regs[i + 1].name);
				safe_hex_16(static_cast<unsigned long>(uc->uc_mcontext.gregs[regs[i + 1].reg_idx]), r_buf, sizeof(r_buf));
				safe_write(r_buf);
			}
			safe_write("\n");
		}
	}
#endif

	safe_write("\nStack trace:\n");

	unw_cursor_t cursor;
	if (unw_init_local(&cursor, reinterpret_cast<unw_context_t *>(ucontext)) == 0) {
		int frame = 0;
		do {
			unw_word_t ip;
			if (unw_get_reg(&cursor, UNW_REG_IP, &ip) == 0) {
				safe_write("  #");
				char frame_num_str[16];
				safe_itoa(frame, frame_num_str, sizeof(frame_num_str));
				safe_write(frame_num_str);

				safe_write(" 0x");
				char ip_str[32];
				safe_hex_toa(ip, ip_str, sizeof(ip_str));
				safe_write(ip_str);

				char symbol[256];
				unw_word_t offset;
				if (unw_get_proc_name(&cursor, symbol, sizeof(symbol), &offset) == 0) {
					safe_write(" in ");
					safe_write(symbol);
					safe_write(" + 0x");
					char offset_str[32];
					safe_hex_toa(offset, offset_str, sizeof(offset_str));
					safe_write(offset_str);
				}
				safe_write("\n");
			}
			frame++;
		} while (unw_step(&cursor) > 0 && frame < 128);
	} else {
		safe_write("  Failed to initialize stack unwinding via libunwind.\n");
	}

	if (crash_fd != -1) {
		safe_write("\nCrash report: ");
		safe_write(crash_filepath);
		safe_write("\n");
		close(crash_fd);
		crash_fd = -1;
	}

	if (safe_strlen(crash_filepath) > 0 && safe_strlen(crashprocess_path) > 0) {
		pid_t parent_pid = getpid();
		pid_t helper_pid = fork();
		if (helper_pid == 0) {
			char pid_str[32];
			safe_itoa(parent_pid, pid_str, sizeof(pid_str));
			execl(crashprocess_path, "turbostar-crashprocess", crash_filepath, pid_str, nullptr);
			execlp("turbostar-crashprocess", "turbostar-crashprocess", crash_filepath, pid_str, nullptr);
			_exit(1);
		} else if (helper_pid > 0) {
			waitpid(helper_pid, nullptr, 0);
		}
	}

	signal(sig, SIG_DFL);
	kill(getpid(), sig);
	_exit(128 + sig);
}

void install_fallback_handler()
{
	if (is_debugger_attached()) {
		logger::get().info("crash_handler: Debugger attached (TracerPid != 0); skipping crash handler installation.");
		return;
	}

	setup_crash_file();

	std::set_terminate(fallback_terminate_handler);

	struct sigaction old_sa;
	if (sigaction(SIGSEGV, nullptr, &old_sa) == 0) {
		if (old_sa.sa_handler != SIG_DFL && old_sa.sa_handler != SIG_IGN) {
			return;
		}
	}

	stack_t ss = {};
	ss.ss_sp = alt_signal_stack;
	ss.ss_size = sizeof(alt_signal_stack);
	ss.ss_flags = 0;
	sigaltstack(&ss, nullptr);

	struct sigaction new_sa = {};
	new_sa.sa_sigaction = fallback_signal_handler;
	new_sa.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_ONSTACK;
	sigemptyset(&new_sa.sa_mask);

	sigaction(SIGSEGV, &new_sa, nullptr);
	sigaction(SIGABRT, &new_sa, nullptr);
	sigaction(SIGILL, &new_sa, nullptr);
	sigaction(SIGFPE, &new_sa, nullptr);
	sigaction(SIGBUS, &new_sa, nullptr);
}

} // namespace crash_handler
