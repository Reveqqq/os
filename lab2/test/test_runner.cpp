#include <iostream>
#include "process_runner.h"

int main() {
#ifdef _WIN32
    std::string cmd = "cmd /C exit 42";
#else
    std::string cmd = "sleep 1; exit 42";
#endif

    std::cout << "Запуск фоновой программы...\n";

    process::ProcessHandle ph = process::run_background(cmd);

    std::cout << "Ожидание завершения...\n";
    int code = process::wait_process(ph);

    std::cout << "Код возврата: " << code << std::endl;

    return 0;
}
