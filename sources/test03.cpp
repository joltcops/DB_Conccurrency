#include "lockmanager.h"
#include <thread>
#include <vector>
#include <chrono>
#include <print>

// a more complicated example of deadlock detection.

void t0(LockManager& lm, int tid) {
    try {
        lm.begin_transaction(tid);
        std::println(">> Transaction {} has started", tid);
        std::println(">> Transaction {} is trying to acquire write lock on resource 2", tid);
        lm.write_lock(tid, 2);
        std::println(">> Transaction {} acquired write lock on resource 2", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::println(">> Transaction {} is trying to acquire read lock on resource 0", tid);
        lm.read_lock(tid, 0);
        std::println(">> Transaction {} acquired read lock on resource 0", tid);
        std::println(">> Transaction {} is trying to acquire read lock on resource 1", tid);
        lm.read_lock(tid, 1);
        std::println(">> Transaction {} acquired read lock on resource 1", tid);
        lm.finish_transaction(tid);
        std::println(">> Transaction {} has finished", tid);
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}

void t1(LockManager& lm, int tid) {
    try {
        lm.begin_transaction(tid);
        std::println(">> Transaction {} has started", tid);
        std::println(">> Transaction {} is trying to acquire write lock on resource 0", tid);
        lm.write_lock(tid, 0);
        std::println(">> Transaction {} acquired write lock on resource 0", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::println(">> Transaction {} is trying to acquire read lock on resource 1", tid);
        lm.read_lock(tid, 1);
        std::println(">> Transaction {} acquired read lock on resource 1", tid);
        lm.finish_transaction(tid);
        std::println(">> Transaction {} has finished", tid);
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}

void t2(LockManager& lm, int tid) {
    try {
        lm.begin_transaction(tid);
        std::println(">> Transaction {} has started", tid);
        std::println(">> Transaction {} is trying to acquire write lock on resource 1", tid);
        lm.write_lock(tid, 1);
        std::println(">> Transaction {} acquired write lock on resource 1", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::println(">> Transaction {} is trying to acquire read lock on resource 2", tid);
        lm.read_lock(tid, 2);
        std::println(">> Transaction {} acquired read lock on resource 2", tid);
        lm.finish_transaction(tid);
        std::println(">> Transaction {} has finished", tid);
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}

int main() {
    {LockManager lm;
    std::vector<std::jthread> threads;
    threads.emplace_back(t0, std::ref(lm), 0);
    threads.emplace_back(t1, std::ref(lm), 1);
    threads.emplace_back(t2, std::ref(lm), 2);
    }
    std::println(">> All transactions completed.");
    return 0;
}