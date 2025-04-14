#include "lockmanager.h"

LockManager::LockManager() {
    graph.resize(N);
    transaction_phase.resize(N, Phase::GROWING);
    locks_held.resize(N);
    for (int i = 0; i < M; i++) {
        state[i] = LockState::UNLOCKED;
    }
}

void LockManager::begin_transaction(int tid) {
    transaction_phase[tid] = Phase::GROWING;
    std::println("Transaction {} has begun", tid);
}

void LockManager::finish_transaction(int tid) {
    std::println("Transaction {} has finished", tid);
    std::vector<int> resources_to_release(locks_held[tid].begin(), locks_held[tid].end());
    for (int rid : resources_to_release) {
        unlock(tid, rid);
    }
    std::println("Transaction {} terminated successfully", tid);
}

void LockManager::abort_transaction(int tid) {
    std::println("Aborting transaction {}", tid);
    std::vector<int> resources_to_release(locks_held[tid].begin(), locks_held[tid].end());
    for (int rid : resources_to_release) {
        unlock(tid, rid);
    }
    graph[tid].clear();
    transaction_phase[tid] = Phase::GROWING;
    throw std::runtime_error("abort_transaction");
}

int LockManager::try_lock(int tid, int rid, bool is_read_lock) {
    if (transaction_phase[tid] == Phase::SHRINKING) {
        std::println("Transaction {} in shrinking phase, locking violates 2PL protocol.", tid);
        return false;
    }
    
    if ((is_read_lock && (state[rid] != LockState::WRITE_GRANTED && wait_queue[rid].empty())) || 
        (!is_read_lock && state[rid] == LockState::UNLOCKED && wait_queue[rid].empty())) {
            if(is_read_lock)
            {
                std::println("Transaction {} can acquire read lock on resource {}", tid, rid);
                state[rid] = LockState::READ_GRANTED;
            }
            else
            {
                std::println("Transaction {} can acquire write lock on resource {}", tid, rid);
                state[rid] = LockState::WRITE_GRANTED;
            }
        locks_held[tid].insert(rid);
        return 1;  
    }

    std::println("Resource {} is currently locked, transaction {} cannot immediately acquire {} lock", 
                rid, tid, (is_read_lock ? "read" : "write"));
    
    graph[tid].push_back(rid);

    bool deadlock_detected = false;
    std::unique_lock<std::mutex> dl_lock(deadlock_mtx);
    
    std::vector<bool> visited(N, false), rec_stack(N, false);
    for (int i = 0; i < N; i++) {
        if (!visited[i]) {
            std::vector<int> cycle;
            if (dfs(i, visited, rec_stack, cycle)) {
                deadlock_detected = true;
                std::println("Potential deadlock detected if transaction {} waits for {} lock on resource {}", 
                            tid, (is_read_lock ? "read" : "write"), rid);
                break;
            }
        }
    }

    auto& g = graph[tid];
    auto it = find(g.begin(), g.end(), rid);
    if(it != g.end()) {
        g.erase(it);
    }
    if(!deadlock_detected) return 0;
    return -1;
}   

void LockManager::read_lock(int tid, int rid) {
    if (transaction_phase[tid] == Phase::SHRINKING) {
        std::println("Transaction {} in shrinking phase, locking violates 2PL protocol.", tid);
        abort_transaction(tid);
    }

    std::unique_lock<std::mutex> lock(mtx[rid]);

    if (state[rid] == LockState::WRITE_GRANTED || !wait_queue[rid].empty()) {
        std::println("Transaction {} waiting for read lock on resource {}", tid, rid);
        wait_queue[rid].push({ReqType::READ_REQ, tid});
        graph[tid].push_back(rid);

        auto wait_result = cv[rid].wait_for(lock, std::chrono::seconds(TIMEOUT), 
            [this, rid, tid]() { 
                return state[rid] != LockState::WRITE_GRANTED && wait_queue[rid].front().second == tid; 
            });

        if (!wait_result) {
            std::println("Timeout for transaction {} waiting for read lock on {}", tid, rid);
            if(canIRunDeadlockDetection(tid)) deadlock_detection(tid);
            cv[rid].wait(lock, [this, rid, tid]() {
                return state[rid] != LockState::WRITE_GRANTED && wait_queue[rid].front().second == tid;
            });
        }

        wait_queue[rid].pop();
    }
    auto it = find(graph[tid].begin(), graph[tid].end(), rid);
    if(it != graph[tid].end()) {
        graph[tid].erase(it);
    }
    state[rid] = LockState::READ_GRANTED;
    locks_held[tid].insert(rid);
    std::println("Transaction {} acquired read lock on resource {}", tid, rid);
}

void LockManager::write_lock(int tid, int rid) {
    if (transaction_phase[tid] == Phase::SHRINKING) {
        std::println("Transaction {} in shrinking phase, locking violates 2PL protocol.", tid);
        abort_transaction(tid);
    }

    std::unique_lock<std::mutex> lock(mtx[rid]);

    if (state[rid] != LockState::UNLOCKED || !wait_queue[rid].empty()) {
        std::println("Transaction {} waiting for write lock on resource {}", tid, rid);
        wait_queue[rid].push({ReqType::WRITE_REQ, tid});
        graph[tid].push_back(rid);

        auto wait_result = cv[rid].wait_for(lock, std::chrono::seconds(TIMEOUT), 
            [this, rid, tid]() { 
                return state[rid] == LockState::UNLOCKED && wait_queue[rid].front().second == tid; 
            });

        if (!wait_result) {
            std::println("Timeout for transaction {} waiting for write lock on {}", tid, rid);
            if(canIRunDeadlockDetection(tid)) deadlock_detection(tid);
            cv[rid].wait(lock, [this, rid, tid]() {
                return state[rid] == LockState::UNLOCKED && wait_queue[rid].front().second == tid;
            });
        }

        wait_queue[rid].pop();
    }

    state[rid] = LockState::WRITE_GRANTED;
    locks_held[tid].insert(rid);
    auto it = find(graph[tid].begin(), graph[tid].end(), rid);
    if(it != graph[tid].end()) {
        graph[tid].erase(it);
    }
    std::println("Transaction {} acquired write lock on resource {}", tid, rid);
}

void LockManager::unlock(int tid, int rid) {
    std::println("Transaction {} requesting to unlock resource {}", tid, rid);
    std::unique_lock lock(mtx[rid]);
    std::println("Transaction {} has locked resource {}", tid, rid);

    if (locks_held[tid].find(rid) == locks_held[tid].end()) {
        abort_transaction(tid);
    }

    transaction_phase[tid] = Phase::SHRINKING;
    locks_held[tid].erase(rid);

    auto& g = graph[tid];
    auto it = find(g.begin(), g.end(), rid);
    if(it != g.end()) {
        g.erase(it);
    }

    state[rid] = LockState::UNLOCKED;
    std::println("Transaction {} released lock on resource {}", tid, rid);

    if (!wait_queue[rid].empty()) {
        auto [req_type, waiting_tid] = wait_queue[rid].front();
        state[rid] = (req_type == ReqType::READ_REQ) ?
                        LockState::READ_GRANTED : LockState::UNLOCKED;
        std::println("Granting {} lock on resource {} to waiting transaction {}",
                        (req_type == ReqType::READ_REQ ? "read" : "write"), rid, waiting_tid);
        cv[rid].notify_all();
    }
}

int LockManager::canIRunDeadlockDetection(int tid){
    std::unique_lock<std::mutex> lock(deadlock_mtx);
    std::println("Transaction {} is checking if it can run deadlock detection", tid);
    std::vector<bool> visited(N, false), rec_stack(N, false);

    for (int i = 0; i < N; i++) {
        if (!visited[i]) {
            std::vector<int> cycle;
            if (dfs(i, visited, rec_stack, cycle)) {
                int to_abort = *std::max_element(cycle.begin(), cycle.end());
                if(to_abort == tid){
                    return 1;
                }
            }
        }
    }
    return 0;
}

void LockManager::deadlock_detection(int tid) {
    std::println("Transaction {} performing deadlock detection", tid);
    std::println("Printing graph edges:");
    allocated_edges();
    request_edges();
    std::vector<bool> visited(N, false), rec_stack(N, false);
    for (int i = 0; i < N; i++) {
        if (!visited[i]) {
            std::vector<int> cycle;
            if (dfs(i, visited, rec_stack, cycle)) {
                std::println("Deadlock detected involving transactions:");
                for (int t : cycle) {
                    std::println("  {}", t);
                }
                int to_abort = *std::max_element(cycle.begin(), cycle.end());
                if(to_abort == tid){
                    // abort transaction
                    std::println("Aborting transaction {}", tid);
                    std::vector<int> resources_to_release(locks_held[tid].begin(), locks_held[tid].end());
                    for (int rid : resources_to_release) {
                        unlock(tid, rid);
                    }
                    graph[tid].clear();
                    transaction_phase[tid] = Phase::GROWING;
                    throw std::runtime_error("abort_transaction");
                }
                return;
            }
        }
    }

    std::println("No deadlock detected by transaction {}", tid);
}

bool LockManager::dfs(int v, std::vector<bool>& visited, std::vector<bool>& rec_stack, std::vector<int>& cycle) {
    if (!visited[v]) {
        visited[v] = true;
        rec_stack[v] = true;

        for (const auto& x : graph[v]) {
                int rid = x;
                for (int i = 0; i < N; ++i) {
                    if (i != v && locks_held[i].find(rid) != locks_held[i].end()) {
                        if (!visited[i] && dfs(i, visited, rec_stack, cycle)) {
                            cycle.push_back(i);
                            return true;
                        } else if (rec_stack[i]) {
                            cycle.push_back(i);
                            return true;
                        }
                    }
                }
            
        }
    }
    rec_stack[v] = false;
    return false;
}

void LockManager::allocated_edges(){
    std::println("Allocated edges:");
    for (int i = 0; i < N; ++i) {
            if(locks_held[i].size()){
            std::println("  Transaction {}: ", i);
            for (const auto& rid : locks_held[i]) {
                std::println("      Resource {}", rid);
            }
        }
    }
    std::println();
}

void LockManager::request_edges(){
    std::println("Request edges:");
    for (int i = 0; i < N; ++i) {
        if(graph[i].size()){
            std::println("  Transaction {}: ", i);
            for (const auto& rid : graph[i]) {
                std::println("      Resource {}", rid);
            }
        }
    }
    std::println();
}
