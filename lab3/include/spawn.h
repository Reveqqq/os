#pragma once
#include <atomic>
#include <string>

void spawn_copies(std::atomic<int>* counter, const std::string& log_file);
