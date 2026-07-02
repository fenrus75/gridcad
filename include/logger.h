#pragma once

#include <string>
#include <string_view>
#include <format>
#include <print>
#include <fstream>
#include <mutex>
#include <chrono>

enum class log_level {
    debug,
    info,
    warning,
    error,
    fatal
};

class logger {
public:
    // Singleton access
    static logger& get() {
        static logger instance;
        return instance;
    }

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) = delete;
    logger& operator=(logger&&) = delete;

    // Configuration
    void set_level(log_level level) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_level_ = level;
    }

    bool set_log_file(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        file_stream_.open(filepath, std::ios::out | std::ios::app);
        return file_stream_.is_open();
    }

    // Generic formatting log function
    template <typename... Args>
    void log(log_level level, std::format_string<Args...> fmt, Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (level < current_level_) {
            return;
        }

        std::string message = std::format(fmt, std::forward<Args>(args)...);
        write_log(level, message);
    }

    // Helper functions for levels
    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(log_level::debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        log(log_level::info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(log_level::warning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        log(log_level::error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void fatal(std::format_string<Args...> fmt, Args&&... args) {
        log(log_level::fatal, fmt, std::forward<Args>(args)...);
    }

private:
    logger() = default;
    ~logger() {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
    }

    std::string_view level_to_string(log_level level) const {
        switch (level) {
            case log_level::debug:   return "DEBUG";
            case log_level::info:    return "INFO";
            case log_level::warning: return "WARN";
            case log_level::error:   return "ERROR";
            case log_level::fatal:   return "FATAL";
        }
        return "UNKNOWN";
    }

    void write_log(log_level level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        std::string timestamp = std::format("{:%Y-%m-%d %H:%M:%S}", now);

        std::string log_line = std::format("[{}] [{}] {}\n", timestamp, level_to_string(level), message);

        // Always print to console using C++23 std::print
        std::print("{}", log_line);

        // Stream to file if it is open
        if (file_stream_.is_open()) {
            file_stream_ << log_line;
            file_stream_.flush();
        }
    }

    std::mutex mutex_;
    log_level current_level_ = log_level::info;
    std::ofstream file_stream_;
};
