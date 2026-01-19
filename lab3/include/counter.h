#pragma once
#include <atomic>

void counter_timer(std::atomic<int>* counter);
void user_input_thread(std::atomic<int>* counter);