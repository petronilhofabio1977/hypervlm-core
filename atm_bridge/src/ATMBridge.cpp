#include "../include/ATMBridge.hpp"
#include <iostream>

ATMBridge::ATMBridge(size_t n){
    for(size_t i=0;i<n;i++){
        workers.emplace_back([this](){
            while(run){
                Task t;
                {
                    std::unique_lock<std::mutex> lk(m);
                    cv.wait(lk,[&](){return !q.empty() || !run;});
                    if(!run && q.empty()) return;
                    t = q.front(); q.pop();
                }
                t();
            }
        });
    }
}
ATMBridge::~ATMBridge(){ shutdown(); }
void ATMBridge::submit(Task t){
    { std::lock_guard<std::mutex> lk(m); q.push(t); }
    cv.notify_one();
}
void ATMBridge::shutdown(){
    run=false;
    cv.notify_all();
    for(auto &w:workers) if(w.joinable()) w.join();
}
