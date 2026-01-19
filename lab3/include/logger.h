#pragma once
#include <atomic>
#include <string>

void log_write(const std::string& filename, const std::string& msg);
std::string current_time();
void logger_thread(std::atomic<int>* counter, const std::string& log_file);
unsigned long get_pid();
