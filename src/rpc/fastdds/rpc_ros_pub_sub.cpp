/**
 * @file rpc_ros_pub_sub.cpp
 * @author whoami
 * @brief 
 * @version 0.1
 * @date 2026-03-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "rpc/fastdds/rpc_ros_pub_sub.h"

namespace rpc {

class RpcRosPubSub::Impl {
 public:
  Impl();
  ~Impl();

 private:
  bool RegisterReadTopic(const std::string &topic,
                         DataCb data_callback);
  bool RegisterWriteTopic(const std::string &topic,
                          DataCb data_callback);
  bool Write(const std::string &topic, void *data, int len);

  std::shared_ptr<RpcRosSub> request_subscriber_;
  std::shared_ptr<RpcRosPub> reply_publisher_;

  std::shared_ptr<RpcRosSub> reply_subscriber_;
  std::shared_ptr<RpcRosPub> request_publisher_;

  friend RpcRosPubSub;
};

RpcRosPubSub::Impl::Impl() = default;
RpcRosPubSub::Impl::~Impl() = default;

bool RpcRosPubSub::Impl::RegisterReadTopic(const std::string &topic,
                                           DataCb data_callback) {
  request_subscriber_ = std::make_shared<RpcRosSub>();
  auto callback = [data_callback](std::shared_ptr<std::vector<uint8_t>> data) {
    data_callback(data->data(), data->size());
  };
  if (!request_subscriber_->Init(topic + "_req", callback)) {
    std::cout << "init read rpc ros request subscriber failed: [" << topic
               << "]" << std::endl;
    request_subscriber_ = nullptr;
    return false;
  }

  reply_publisher_ = std::make_shared<RpcRosPub>();
  if (!reply_publisher_->Init(topic + "_rep")) {
    std::cout << "init read rpc ros reply publisher failed: [" << topic << "]" << std::endl;
    reply_publisher_ = nullptr;
    return false;
  }
  reply_publisher_->Run();

  return true;
}

bool RpcRosPubSub::Impl::RegisterWriteTopic(const std::string &topic,
                                            DataCb data_callback) {
  reply_subscriber_ = std::make_shared<RpcRosSub>();
  auto callback = [data_callback](std::shared_ptr<std::vector<uint8_t>> data) {
    data_callback(data->data(), data->size());
  };
  if (!reply_subscriber_->Init(topic + "_rep", callback)) {
    std::cout << "init write rpc ros reply subscriber failed: [" << topic
               << "]" << std::endl;
    request_subscriber_ = nullptr;
    return false;
  }

  request_publisher_ = std::make_shared<RpcRosPub>();
  if (!request_publisher_->Init(topic + "_req")) {
    std::cout << "init write rpc ros request publisher failed: [" << topic
               << "]" << std::endl;
    request_publisher_ = nullptr;
    return false;
  }
  request_publisher_->Run();
  return true;
}

bool RpcRosPubSub::Impl::Write(const std::string &topic, void *data, int len) {
  auto data_ptr = static_cast<uint8_t *>(data);
  auto item = std::make_shared<std::vector<uint8_t>>(data_ptr, data_ptr + len);

  if (request_publisher_) {
    request_publisher_->Publish(item);
    return true;
  }

  if (reply_publisher_) {
    reply_publisher_->Publish(item);
    return true;
  }

  std::cout << "[" << topic << "] is not ready to write, not registered!" << std::endl;
  return false;
}

RpcRosPubSub::RpcRosPubSub() : impl_(std::make_unique<Impl>()) {}

RpcRosPubSub::~RpcRosPubSub() = default;

bool RpcRosPubSub::RegisterReadTopic(const std::string &topic,
                                     DataCb data_callback) {
  return impl_->RegisterReadTopic(topic, std::move(data_callback));
}

bool RpcRosPubSub::RegisterWriteTopic(const std::string &topic,
                                      DataCb data_callback) {
  return impl_->RegisterWriteTopic(topic, std::move(data_callback));
}

bool RpcRosPubSub::Write(const std::string &topic, void *data, int len) {
  return impl_->Write(topic, data, len);
}

}