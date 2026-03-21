#include <chrono>
#include <iostream>
#include <thread>

#include "rpc/fastdds/rpc_ros_sub.h"

int main() {
    rpc::RpcRosSub sub;
    sub.Init("tool", [](const std::shared_ptr<std::vector<uint8_t>>& item) { 
        std::vector<uint8_t> v = *item;
        std::cout << "receive msg : ";
        for (auto i : v)
            std::cout << static_cast<int>(i) << ' ';
        std::cout << std::endl;
     });

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}