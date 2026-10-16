#include "ghostnet/scheduler.hpp"
#include <iostream>

int main() {
    ghostnet::Scheduler scheduler;
    std::cout << "[GHOSTNET] Engine Initialized. Virtual Clock at: " 
              << scheduler.current_time() << " us" << std::endl;
    return 0;
}