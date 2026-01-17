#include "process_runner.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

namespace process {

ProcessHandle run_background(const std::string& command) {
    ProcessHandle ph{};

#ifdef _WIN32
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    char cmd[1024];
    strcpy(cmd, command.c_str());

    if (!CreateProcessA(
            nullptr,
            cmd,
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        // Ошибка создания процесса
        ph.handle = nullptr;
        return ph;
    }

    CloseHandle(pi.hThread);
    ph.handle = pi.hProcess;

#else
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(1);
    }
    ph.pid = pid;
#endif

    return ph;
}   

int wait_process(const ProcessHandle& process) {
#ifdef _WIN32
    if (!process.handle) return -1;

    WaitForSingleObject(process.handle, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process.handle, &exit_code);
    CloseHandle(process.handle);

    return static_cast<int>(exit_code);
#else
    int status = 0;
    waitpid(process.pid, &status, 0);
    
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return -1; // Процесс завершился ненормально
    }
#endif
}

}