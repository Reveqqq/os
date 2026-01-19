#include "logger.h"
#include <fstream>
#include <sstream>
#include <mutex>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

std::mutex log_mutex;

void log_write(const std::string& filename, const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream log(filename, std::ios::app);
    log << msg << std::endl;
}

std::string current_time() {
    auto now = std::chrono::system_clock::now();
    auto t_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t_c);
#else
    localtime_r(&t_c, &tm);
#endif
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<long long>(ms.count()));
    return std::string(buf);
}

unsigned long get_pid() {
#if defined(_WIN32)
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

void logger_thread(std::atomic<int>* counter, const std::string& log_file) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::ostringstream oss;
        oss << "[" << current_time() << "] PID=" << get_pid()
            << " Counter=" << counter->load();
        log_write(log_file, oss.str());
    }
}
