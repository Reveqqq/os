#include "counter.h"
#include "logger.h"
#include "spawn.h"
#include <atomic>
#include <thread>

#if defined(_WIN32)
// Windows заголовки
#include <windows.h>
#else
// POSIX заголовки
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#endif

int main() {
    const std::string log_file = "log.txt";

    // --- Shared counter ---
    std::atomic<int>* counter = nullptr;

#if defined(_WIN32)
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), "GlobalCounter");
    int* counter_ptr = (int*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    counter = new(counter_ptr) std::atomic<int>(0);
#else
    const char* shm_name = "/shared_counter_os_lab";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(std::atomic<int>));
    void* ptr = mmap(nullptr, sizeof(std::atomic<int>), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    counter = new(ptr) std::atomic<int>(0);
#endif

    // --- Leader detection ---
    bool is_leader = false;

#if defined(_WIN32)
    HANDLE leader_mutex = CreateMutex(NULL, FALSE, "LeaderMutex");
    is_leader = (WaitForSingleObject(leader_mutex, 0) == WAIT_OBJECT_0);
#else
    const char* lock_file = "/tmp/leader.lock";
    int fd = open(lock_file, O_CREAT | O_RDWR, 0666);
    is_leader = (flock(fd, LOCK_EX | LOCK_NB) == 0);
#endif

    log_write(log_file, "[" + current_time() + "] PID=" + std::to_string(get_pid()) + " Program start");

    std::thread(counter_timer, counter).detach();
    std::thread(user_input_thread, counter).detach();

    if (is_leader) {
        std::thread(logger_thread, counter, log_file).detach();
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            spawn_copies(counter, log_file);
        }
    } else {
        while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
