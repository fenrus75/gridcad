# GridCad Development Guidelines

## Build & Test Instructions
- **Configure**: `meson setup build`
- **Compile**: `ninja -C build`
- **Run**: `./build/gridcad`
- **Test**: `meson test -C build`
- **Meson Targets**: When adding a new `.cpp` source file, update `meson.build` for **all** relevant targets (`gridcad`, `test_logic`, `wiretest`) so unit tests stay in sync.
- **Test Cache Isolation**: Tests exercising `~/.cache/gridcad/` (e.g. `layout_overlay`) MUST use isolated temporary paths or a temporary `HOME` directory so they never overwrite user cached layouts.

## Tooling & Interactive Debugging
- **Turbostar MCP**: When the `turbostar` MCP server is available, prefer its tools (`fs_read_lines`, `fs_batch_read`, `fs_grep_files`, `fs_replace_lines`, `fs_compile_project`, `run_executable`) over built-in/shell tools.
- **Live GDB Debugging**: Prefer `run_executable` with `Debugger: true` over shell-based `gdb` or temporary print statements:
  - Launch: `run_executable(Binary="build/<target>", Debugger=true)` → returns `app_run_id` and `gdb_run_id`.
  - Control: `agent_write_to_run(RunId=gdb_run_id, Data="break <file>:<line>\ncontinue\n", Output=true)`.
  - Inspect: `agent_write_to_run(RunId=gdb_run_id, Data="bt\n", Output=true)` or `Data="p <expr>\n"`.
  - Clean up: `agent_terminate_run(RunId=app_run_id)` (terminates both app and gdb).

## Code Conventions
- **Language Standard**: C++23.
- **Include Guards**: Prefer `#pragma once`.
- **Zero-Warning Baseline**: Maintain 0 compiler warnings on all GridCad code (excluding third-party `imgui` / `imgui_hex_editor`).
- **Memory Management**: Follow RAII strictly; avoid raw `new` and `delete`, using `std::make_unique` and `std::make_shared`.
- **String & Container Handling**:
  - Prefer `std::format` over string concatenation.
  - Prefer `std::string_view` over `const std::string &` and `std::span<const T>` over `const std::vector<T> &` for read-only parameters; avoid raw `char *` except when interfacing with C APIs or the OS kernel.
  - Label methods and parameters `constexpr`, `const`, and `noexcept` where appropriate.
- **Standard Library**: Prefer C++ Standard Library containers and algorithms over custom implementations.
- **Member Initialization**: Initialize all class and struct member variables (scalars, pointers, POD structs) inline at their declaration in the class definition (e.g., `= 0`, `= false`, `= nullptr`, `= {}`).
- **No `memcmp`/`memset` on C++ Structs**: Never use `memcmp`/`memset` on C++ structs (struct padding bytes are uninitialized and trigger UB / `-Wclass-memaccess`); use `bool operator==(const T &) const = default;` and value-initialization (`= {}`) instead.
- **File Organization**: Place each class in a dedicated `.cpp` file and matching header in the same directory.

## Security Considerations
- **Security-First**: Adopt a security-first mindset.
- **Untrusted Input**: Clearly label variables and parameters holding untrusted data (CLI flags, imported JSON/VCD files, user/network input) using the `untrusted_` prefix or a `/* untrusted */` parameter comment.
- **Early Validation**: Sanity-check and validate all untrusted input as early as possible.
- **Bounds Checking**: Verify all buffer and array accesses against underflows and overflows.

## Code Commenting Conventions
- **Intent over Logic**: Focus comments on rationale (the "why") and design goals rather than restating code logic.
- **Ownership & Constraints**: Document ownership, thread-safety expectations, and function constraints.
- **Mutex Declarations**: Precede every mutex declaration in header files with a comment block explaining:
  1. What specific data or resources the mutex protects.
  2. Locking rules, lifecycle, or ordering guidelines.

## Version Control & Testing Conventions
- **Test-First on Bug Fixes**: When fixing a bug (not a new feature), create a failing testcase before fixing the bug; verify it fails first, and passes once the bug is fixed.
- **Pre-commit Review**: Perform a code review before each commit to ensure no stray or accidental edits are included.
- **Test Verification**: Run the full test suite (`meson test -C build`) before committing changes.
- **Commit Cadence**: `git commit` after each logical change or item implemented.
