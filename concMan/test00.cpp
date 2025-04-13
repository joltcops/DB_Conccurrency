#include "lockmanager.h"
#include <thread>
#include <vector>
#include <chrono>
#include <print>

// void t1(LockManager& lm, int tid) {
//     try {
//         lm.begin_transaction(tid);
//         int tl = lm.try_lock(tid, 0, false);
//         if(tl == -1) {
//             std::println(">> Transaction {} cannot acquire write lock on resource 0", tid);
//             lm.write_lock(tid, 0);
//         }
//         else if (tl == 0) {
//             std::println(">> Transaction {} can acquire write lock on resource 0", tid);
//             lm.write_lock(tid, 0);
//         }
//         else{
//             std::println(">> Transaction {} acquired write lock on resource 0", tid);
//         }

//         std::println(">> t1 acquired write lock for resource 0");
//         std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//         lm.read_lock(tid, 1);
//         std::println(">> t1 acquired read lock for resource 1");
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//         lm.finish_transaction(tid);
//     } catch (const std::exception& e) {
//         std::println("Transaction {} error: {}", tid, e.what());
//     }
// }

// void t2(LockManager& lm, int tid) {
//     try {
//         lm.begin_transaction(tid);
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//         int tl = lm.try_lock(tid, 0, true);
//         if (tl == -1) {
//             std::println(">> Transaction {} cannot acquire read lock on resource 0", tid);
//             lm.read_lock(tid, 0);
//         }
//         else if (tl == 0) {
//             std::println(">> Transaction {} can acquire read lock on resource 0", tid);
//             lm.read_lock(tid, 0);
//         }
//         else{
//             std::println(">> Transaction {} acquired read lock on resource 0", tid);
//         }
//         std::println(">> t2 acquired read lock for resource 0");
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//         lm.read_lock(tid, 1);
//         std::println(">> t2 acquired read lock for resource 1");
//         std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//         lm.finish_transaction(tid);
//     } catch (const std::exception& e) {
//         std::println(">> Transaction {} error: {}", tid, e.what());
//     }
// }

void t0(LockManager& lm, int tid) {
    try {
        lm.begin_transaction(tid);
        std::println(">> Transaction {} has started", tid);
        std::println(">> Transaction {} is trying to acquire write lock on resource 0", tid);
        lm.write_lock(tid, 0);
        std::println(">> Transaction {} acquired write lock on resource 0", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::println(">> Transaction {} is trying to acquire write lock on resource 1", tid);
        lm.write_lock(tid, 1);
        std::println(">> Transaction {} acquired write lock on resource 1", tid);
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
        std::println(">> Transaction {} is trying to acquire write lock on resource 1", tid);
        lm.write_lock(tid, 1);
        std::println(">> Transaction {} acquired write lock on resource 1", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::println(">> Transaction {} is trying to acquire write lock on resource 0", tid);
        lm.write_lock(tid, 0);
        std::println(">> Transaction {} acquired write lock on resource 0", tid);
        lm.finish_transaction(tid);
        std::println(">> Transaction {} has finished", tid);
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}

int main() {
    LockManager lm;
    std::vector<std::jthread> threads;
    threads.emplace_back(t0, std::ref(lm), 0);
    threads.emplace_back(t1, std::ref(lm), 1);
    std::println(">> All transactions completed.");
    return 0;
}