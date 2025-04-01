#include <bits/stdc++.h>
#include <shared_mutex>
#include <syncstream>
#include <condition_variable>
#include <chrono>

using namespace std;

void func(int tid) {
    osyncstream(cout) << "func called by " << tid << endl;
    if(tid == 1) {
        return; // Thread 1 continues normally
    }
    else {
        throw runtime_error("tid is 2"); // Thread 2 throws exception
    }
}

void tf(int tid) {
    try {
        osyncstream(cout) << "Transaction " << tid << " has begun" << endl;
        this_thread::sleep_for(chrono::milliseconds(1000));
        
        func(tid); // This will throw for tid=2
        
        osyncstream(cout) << tid << " intermediate print" << endl;
    }
    catch(const exception& e) {
        osyncstream(cout) << "Transaction " << tid << " error: " << e.what() << endl;
    }
    osyncstream(cout) << "Transaction " << tid << " has finished" << endl;
}

int main() {
    thread t1(tf, 1);
    thread t2(tf, 2);
        
    t1.join();
    t2.join();
        
    return 0;
}