#include "spawn.h"
#include "logger.h"
#include <thread>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

void spawn_copies(std::atomic<int>* counter, const std::string& log_file) {
#if defined(_WIN32)
    // Для Windows пока оставим заглушку
#else
    static pid_t child1 = 0, child2 = 0;

    if (child1 != 0) {
        int status;
        if (waitpid(child1, &status, WNOHANG) == 0) {
            log_write(log_file, "[" + current_time() + "] Copy1 still running, skipping spawn");
            child1 = -1;
        } else child1 = 0;
    }

    if (child2 != 0) {
        int status;
        if (waitpid(child2, &status, WNOHANG) == 0) {
            log_write(log_file, "[" + current_time() + "] Copy2 still running, skipping spawn");
            child2 = -1;
        } else child2 = 0;
    }

    if (child1 == 0) {
        child1 = fork();
        if (child1 == 0) {
            log_write(log_file, "[" + current_time() + "] PID=" + std::to_string(getpid()) + " Copy1 start");
            counter->fetch_add(10);
            log_write(log_file, "[" + current_time() + "] PID=" + std::to_string(getpid()) + " Copy1 end");
            _exit(0);
        }
    }

    if (child2 == 0) {
        child2 = fork();
        if (child2 == 0) {
            log_write(log_file, "[" + current_time() + "] PID=" + std::to_string(getpid()) + " Copy2 start");
            int old_val = counter->load();
            while (!counter->compare_exchange_weak(old_val, old_val * 2)) {}
            std::this_thread::sleep_for(std::chrono::seconds(2));
            old_val = counter->load();
            while (!counter->compare_exchange_weak(old_val, old_val / 2)) {}
            log_write(log_file, "[" + current_time() + "] PID=" + std::to_string(getpid()) + " Copy2 end");
            _exit(0);
        }
    }
#endif
}
