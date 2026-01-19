#include "counter.h"
#include <iostream>
#include <thread>
#include <chrono>

void counter_timer(std::atomic<int>* counter) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        counter->fetch_add(1);
    }
}

void user_input_thread(std::atomic<int>* counter) {
    while (true) {
        int value;
        std::cout << "Enter new counter value: ";
        std::cin >> value;
        counter->store(value);
    }
}
