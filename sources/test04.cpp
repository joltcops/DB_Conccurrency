#include "lockmanager.h"
#include <thread>
#include <vector>
#include <chrono>
#include <print>

// a linear chain of requests + try_lock

void t0(LockManager& lm, int tid) {
    try {
        lm.begin_transaction(tid);
        std::println(">> Transaction {} has started", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int op = lm.try_lock(tid, 0, 0);
        if(op == 1) std::println(">> Trylock: Transaction {} acquired write lock on resource 0", tid);
        else if(op == 0) std::println(">> Trylock: Transaction {} cannot acquire write lock on resource 0 immediately", tid);
        else std::println(">> Trylock: Transaction {} cannot acquire write lock on resource 0 - leads to deadlock", tid);
        std::println(">> Transaction {} is trying to acquire write lock on resource 0", tid);
        lm.write_lock(tid, 0);
        std::println(">> Transaction {} acquired write lock on resource 0", tid);
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
        int op = lm.try_lock(tid, 0, 0);
        if(op == 1) std::println(">> Trylock: Transaction {} acquired write lock on resource 0", tid);
        else if(op == 0) std::println(">> Trylock: Transaction {} cannot acquire write lock on resource 0 immediately", tid);
        else std::println(">> Trylock: Transaction {} cannot acquire write lock on resource 0 - leads to deadlock", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::println(">> Transaction {} is trying to acquire write lock on resource 1", tid);
        lm.write_lock(tid, 1);
        std::println(">> Transaction {} acquired write lock on resource 1", tid);
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
        std::println(">> Transaction {} is trying to acquire write lock on resource 2", tid);
        lm.write_lock(tid, 2);
        std::println(">> Transaction {} acquired write lock on resource 2", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        lm.finish_transaction(tid);
        std::println(">> Transaction {} has finished", tid);
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}
void t3(LockManager& lm, int tid) {
    try {
        lm.begin_transaction(tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::println(">> Transaction {} is trying to acquire write lock on resource 2", tid);
        lm.write_lock(tid, 2);
        std::println(">> Transaction {} acquired write lock on resource 2", tid);
        lm.finish_transaction(tid);
        std::println(">> Transaction {} has finished", tid);
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}

void t4(LockManager& lm, int tid) {
    try {
        std::println(">> Transaction {} has started - Print graph transaction", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::println(">> Printing Graph");
        lm.allocated_edges();
        lm.request_edges();
        std::println(">> Transaction {} exitting", tid); 
    } catch (const std::exception& e) {
        std::println(">> Transaction {} error: {}", tid, e.what());
    }
}

int main() {
    {
        LockManager lm;
        std::vector<std::jthread> threads;
        threads.emplace_back(t0, std::ref(lm), 0);
        threads.emplace_back(t1, std::ref(lm), 1);
        threads.emplace_back(t2, std::ref(lm), 2);
        threads.emplace_back(t3, std::ref(lm), 3);
        threads.emplace_back(t4, std::ref(lm), 4);
    }
    
    std::println(">> All transactions completed.");
    return 0;
}