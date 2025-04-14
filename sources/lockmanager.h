#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <numeric>
#include <print>
#include <random>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <set>
#include <stdexcept>

#define N 10  // Number of transactions
#define M 10  // Number of resources
#define TIMEOUT 10  // Timeout in seconds

enum class Phase { GROWING, SHRINKING };
enum class LockState { READ_GRANTED, WRITE_GRANTED, UNLOCKED };
enum class ReqType { READ_REQ, WRITE_REQ };

class LockManager {
private:
    std::mutex mtx[M];                     
    LockState state[M];                            
    std::queue<std::pair<ReqType, int> > wait_queue[M];    // req_type, tid 
    std::vector<std::vector<int>> graph;       
    std::condition_variable_any cv[M];            
    std::vector<Phase> transaction_phase;         
    std::vector<std::set<int>> locks_held;    
    std::mutex deadlock_mtx;

    bool dfs(int v, std::vector<bool>& visited, std::vector<bool>& rec_stack, std::vector<int>& cycle);

public:
    LockManager();
    
    void begin_transaction(int tid);
    void finish_transaction(int tid);
    void abort_transaction(int tid);
    
    int try_lock(int tid, int rid, bool is_read_lock);
    void read_lock(int tid, int rid);
    void write_lock(int tid, int rid);
    void unlock(int tid, int rid);
    
    int canIRunDeadlockDetection(int tid);
    void deadlock_detection(int tid);

    void allocated_edges();
    void request_edges();
};