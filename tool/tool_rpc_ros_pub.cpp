#include <chrono>
#include <iostream>
#include <thread>

#include "rpc/fastdds/rpc_ros_pub.h"

int main() {
  rpc::RpcRosPub pub;
  pub.Init("tool");
  pub.Run();

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    static uint8_t idx = 0;
    auto tp = std::make_shared<std::vector<uint8_t>>(4, idx++);
    pub.Publish(tp);
  }
  return 0;
}