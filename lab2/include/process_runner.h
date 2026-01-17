#pragma once
#include <string>

namespace process {

struct ProcessHandle {
#ifdef _WIN32
    void* handle;   // HANDLE процесса
#else
    int pid;        // PID процесса
#endif
};

/// Запуск программы в фоновом режиме
ProcessHandle run_background(const std::string& command);

/// Ожидание завершения и получение кода возврата
int wait_process(const ProcessHandle& process);

}
