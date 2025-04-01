#include <bits/stdc++.h>
#include <mutex>
#include <syncstream>
#include <condition_variable>
#include <chrono>

using namespace std;

#define READ_REQ 2004
#define WRITE_REQ 2005
#define READ_GRANTED 2006
#define WRITE_GRANTED 2007
#define UNLOCKED 2008

#define RESOURCE 3001
#define TRANSACTION 3002

#define N 3
#define M 2  

#define TIMEOUT 10 

typedef struct Node {
    int type; 
    int id;   
} Node;

enum class Phase { GROWING, SHRINKING };

class LockManager {
    public:
        mutex mtx[M];                     
        int state[M];                            
        queue<pair<int, int>> wait_queue[M];     
        vector<vector<Node>> graph;             
        condition_variable_any cv[M];            
        vector<Phase> transaction_phase;         
        vector<set<int>> locks_held;    
        mutex deadlock_mtx;
        
        bool dfs(int v, vector<bool> &visited, vector<bool> &rec_stack, vector<int> &cycle){
            if(!visited[v]){
                visited[v] = true;
                rec_stack[v] = true;

                for(const auto &node: graph[v]){
                    if(node.type == RESOURCE){
                        int rid = node.id;
                        for(int i = 0; i < N; ++i){
                            if(i != v && locks_held[i].find(rid) != locks_held[i].end()){
                                if(!visited[i] && dfs(i, visited, rec_stack, cycle)){
                                    cycle.push_back(i);
                                    return true;
                                } else if(rec_stack[i]){
                                    cycle.push_back(i);
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            rec_stack[v] = false;
            return false;
        }

        LockManager() {
            graph.resize(N);
            transaction_phase.resize(N, Phase::GROWING);
            locks_held.resize(N);
            
            for (int i = 0; i < M; i++) {
                state[i] = UNLOCKED;
            }
        }

        void abort_transaction(int tid, int flag) {
            osyncstream(cout) << "Aborting transaction " << tid << endl;
            
            vector<int> resources_to_release;
            for (int rid : locks_held[tid]) {
                resources_to_release.push_back(rid);
            }
            
            for (int rid : resources_to_release) {
                unlock(tid, rid);
            }
            
            graph[tid].clear();
            transaction_phase[tid] = Phase::GROWING;
            throw runtime_error("abort_transaction");
        }

        void read_lock(int tid, int rid){
            if(transaction_phase[tid] == Phase::SHRINKING){
                abort_transaction(tid, 0);
            }
            unique_lock<mutex> lock(mtx[rid]);

            if(state[rid] == WRITE_GRANTED || !wait_queue[rid].empty()){
                osyncstream(cout) << "Transaction " << tid << " waiting for read lock on resource " << rid << endl;

                wait_queue[rid].push({READ_REQ, tid});
                graph[tid].push_back({RESOURCE, rid});

                auto wait_result = cv[rid].wait_for(lock, chrono::seconds(TIMEOUT), 
                    [this, rid, tid]() { 
                        return state[rid] != WRITE_GRANTED && wait_queue[rid].front().second == tid; 
                    });

                if(!wait_result){
                    osyncstream(cout) << "Timeout for transaction " << tid << " waiting for read lock on " << rid << endl;
                    deadlock_detection(tid);

                    cv[rid].wait(lock, [this, rid, tid](){
                        return state[rid] != WRITE_GRANTED && wait_queue[rid].front().second == tid;
                    });
                }

                wait_queue[rid].pop();
            }

            state[rid] = READ_GRANTED;
            locks_held[tid].insert(rid);
            osyncstream(cout) << "Transaction " << tid << " acquired read lock on resource " << rid << endl;
        }

        void write_lock(int tid, int rid){
            if(transaction_phase[tid] == Phase::SHRINKING){
                abort_transaction(tid, 0);
            }
            unique_lock<mutex> lock(mtx[rid]);

            if(state[rid] != UNLOCKED || !wait_queue[rid].empty()){
                osyncstream(cout) << "Transaction " << tid << " waiting for write lock on resource " << rid << endl;

                wait_queue[rid].push({WRITE_REQ, tid});
                graph[tid].push_back({RESOURCE, rid});

                auto wait_result = cv[rid].wait_for(lock, chrono::seconds(TIMEOUT), 
                    [this, rid, tid]() { 
                        return state[rid] == UNLOCKED && wait_queue[rid].front().second == tid; 
                    });

                if(!wait_result){
                    osyncstream(cout) << "Timeout for transaction " << tid << " waiting for write lock on " << rid << endl;
                    deadlock_detection(tid);

                    cv[rid].wait(lock, [this, rid, tid](){
                        return state[rid] == UNLOCKED && wait_queue[rid].front().second == tid;
                    });
                }

                wait_queue[rid].pop();
            }

            state[rid] = WRITE_GRANTED;
            locks_held[tid].insert(rid);
            osyncstream(cout) << "Transaction " << tid << " acquired write lock on resource " << rid << endl;
        }

        void unlock(int tid, int rid){
            osyncstream(cout) << "Transaction " << tid << " requesting to unlock resource " << rid << endl;
            unique_lock<mutex> lock(mtx[rid]);

            if(locks_held[tid].find(rid) == locks_held[tid].end()){
                abort_transaction(tid, 0);
            }
            
            transaction_phase[tid] = Phase::SHRINKING;
            locks_held[tid].erase(rid);

            for(auto it = graph[tid].begin(); it != graph[tid].end(); ++it){
                if(it->type == RESOURCE && it->id == rid){
                    graph[tid].erase(it);
                    break;
                }
            }

            state[rid] = UNLOCKED;
            osyncstream(cout) << "Transaction " << tid << " released lock on resource " << rid << endl;

            if(!wait_queue[rid].empty()){
                auto [req_type, waiting_tid] = wait_queue[rid].front();

                if(req_type == READ_REQ){
                    state[rid] = READ_GRANTED;
                    osyncstream(cout) << "Granting read lock on resource " << rid << " to waiting transaction " << waiting_tid << endl;
                } else {
                    state[rid] = WRITE_GRANTED;
                    osyncstream(cout) << "Granting write lock on resource " << rid << " to waiting transaction " << waiting_tid << endl;
                }

                cv[rid].notify_all();
            }
        }

        void deadlock_detection(int tid){
            unique_lock<mutex> lock(deadlock_mtx);
            osyncstream(cout) << "Transaction " << tid << " performing deadlock detection" << endl;

            vector<bool> visited(N, false);
            vector<bool> rec_stack(N, false);
            
            for (int i = 0; i < N; i++) {
                if (!visited[i]) {
                    vector<int> cycle;
                    if (dfs(i, visited, rec_stack, cycle)) {
                        osyncstream(cout) << "Deadlock detected involving transactions: ";
                        for (int t : cycle) {
                            cout << t << " ";
                        }
                        cout << endl;
                        
                        int to_abort = *max_element(cycle.begin(), cycle.end());
                        for (int i = 0; i < M; i++) {
                            cv[i].notify_all();
                        }
                        throw runtime_error("Transaction " + to_string(to_abort) + " aborted due to deadlock");
                    }
                }
            }
            osyncstream(cout) << "No deadlock detected by transaction " << tid << endl;
        }

        void begin_transaction(int tid){
            transaction_phase[tid] = Phase::GROWING;
            osyncstream(cout) << "Transaction " << tid << " has begun" << endl;
        }

        void finish_transaction(int tid){
            osyncstream(cout) << "Transaction " << tid << " has finished" << endl;

            vector<int> resources_to_release;
            for (int rid : locks_held[tid]) {
                resources_to_release.push_back(rid);
            }
            
            for (int rid : resources_to_release) {
                unlock(tid, rid);
            }
            
            osyncstream(cout) << "Transaction " << tid << " terminated successfully" << endl;
        }

        void debug() {
            osyncstream(cout) << "\n--- Lock Manager State ---" << endl;
            
            cout << "Resource States:" << endl;
            for (int i = 0; i < M; i++) {
                cout << "Resource " << i << ": ";
                switch (state[i]) {
                    case UNLOCKED: cout << "UNLOCKED"; break;
                    case READ_GRANTED: cout << "READ_GRANTED"; break;
                    case WRITE_GRANTED: cout << "WRITE_GRANTED"; break;
                    default: cout << "UNKNOWN";
                }
                cout << endl;
            }
            
            cout << "\nTransaction Locks:" << endl;
            for (int i = 0; i < N; i++) {
                cout << "Transaction " << i << " (";
                cout << (transaction_phase[i] == Phase::GROWING ? "GROWING" : "SHRINKING");
                cout << "): ";
                
                if (locks_held[i].empty()) {
                    cout << "No locks held";
                } else {
                    cout << "Holds locks on resources: ";
                    for (int rid : locks_held[i]) {
                        cout << rid << " ";
                    }
                }
                cout << endl;
            }
            
            cout << "\nWait Queues:" << endl;
            for (int i = 0; i < M; i++) {
                cout << "Resource " << i << ": ";
                if (wait_queue[i].empty()) {
                    cout << "No waiting transactions";
                } else {
                    cout << "Waiting transactions: ";
                    queue<pair<int, int>> temp = wait_queue[i];
                    while (!temp.empty()) {
                        auto [req_type, tid] = temp.front();
                        cout << tid << "(";
                        cout << (req_type == READ_REQ ? "R" : "W");
                        cout << ") ";
                        temp.pop();
                    }
                }
                cout << endl;
            }
            
            cout << "------------------------\n" << endl;
        }
};    

void t1(LockManager& lm, int tid){
    try{    
        lm.begin_transaction(tid);
        lm.write_lock(tid, 0);
        this_thread::sleep_for(chrono::milliseconds(1000));
        lm.write_lock(tid, 1);
        lm.finish_transaction(tid);
    }
    catch(const exception& e){
        osyncstream(cout) << "Transaction " << tid << " error: " << e.what() << endl;
        return;
    }
}

void t2(LockManager& lm, int tid){
    try{    
        lm.begin_transaction(tid);
        lm.write_lock(tid, 1);
        this_thread::sleep_for(chrono::milliseconds(1000));
        lm.write_lock(tid, 0);
        lm.finish_transaction(tid);
    }
    catch(const exception& e){
        osyncstream(cout) << "Transaction " << tid << " error: " << e.what() << endl;
        return;
    }
}

int main() {
    srand(time(nullptr));
    LockManager lm;
    
    vector<thread> threads;
    
    threads.emplace_back(t1, ref(lm), 0);
    threads.emplace_back(t2, ref(lm), 1);
    
    // Wait for all transactions to complete
    for (auto& t : threads) {
        t.join();
    }
    
    cout << "All transactions completed." << endl;
    return 0;
}
