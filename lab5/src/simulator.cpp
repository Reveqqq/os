#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include <ctime>

std::string isoTime() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

int main() {
    std::default_random_engine gen(std::random_device{}());
    std::normal_distribution<double> temp(22.0, 2.0);

    while (true) {
        std::cout << isoTime() << " "
                  << std::fixed << std::setprecision(2)
                  << temp(gen) << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
