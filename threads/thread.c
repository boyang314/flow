#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/syscall.h>

int main() {
    unsigned ncpus = std::thread::hardware_concurrency();
    std::cout << "ncpus:" << ncpus << " pid:" << getpid() << '\n';

    std::mutex mtx;
    std::vector<std::thread> threads(ncpus);
    for (unsigned i=0; i<ncpus; ++i) {
        threads[i] = std::thread([&mtx, i] {
                {
                std::lock_guard<std::mutex> iolock(mtx);
                std::cout << "thread #" << i << " is running on core:" << sched_getcpu() << " get_id:" << std::this_thread::get_id() << " pthread_id:" << pthread_self() << " tid:" << syscall(SYS_gettid) << '\n';
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                });

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) perror("failed to set affinity");
    }

    for (auto& t:threads) t.join();
    return 0;
}

