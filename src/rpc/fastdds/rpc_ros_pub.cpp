/**
 * @file rpc_ros_pub.cpp
 * @author whoami
 * @brief
 * @version 0.1
 * @date 2026-03-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "rpc/fastdds/rpc_ros_pub.h"

#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>

// #include "spdlog/spdlog.h"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "ByteArrayPubSubTypes.hpp"

namespace rpc {

constexpr size_t kMessageWaitTimeout = 500;

class RpcRosPub::Impl {
public:
    Impl();
    ~Impl();

    bool Init(const std::string& topic, int domain_id = 0);
    void Run();
    void Stop();
    void Publish(const std::shared_ptr<std::vector<uint8_t>>& item);

private:
    class PubListener : public eprosima::fastdds::dds::DataWriterListener {
    public:
        PubListener() : matched_(0) {}
        ~PubListener() override {}
        void on_publication_matched(eprosima::fastdds::dds::DataWriter*,
                                    const eprosima::fastdds::dds::PublicationMatchedStatus& info) override {
            if (info.current_count_change == 1) {
                matched_ = info.total_count;
                std::cout << "Publisher matched." << std::endl;
            } else if (info.current_count_change == -1) {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
            } else {
                std::cout << info.current_count_change
                          << " is not a valid value for PublicationMatchedStatus "
                             "current count change."
                          << std::endl;
            }
        }
        std::atomic<int> matched_;
    };

    void MsgSendThread();

private:
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher* publisher_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::DataWriter* writer_;
    eprosima::fastdds::dds::TypeSupport type_;

    PubListener listener_;
    msg_intf::msg::ByteArrayPubSubType my_type_;

    std::atomic<bool> running_;
    std::queue<std::shared_ptr<std::vector<uint8_t>>> msg_queue_;
    std::mutex queue_mtx_;
    std::condition_variable queue_cv_;
    std::thread msg_send_thread_;

    std::string topic_name_;
    int domain_id_;

    friend RpcRosPub;
};

RpcRosPub::Impl::Impl()
    : participant_(nullptr),
      publisher_(nullptr),
      topic_(nullptr),
      writer_(nullptr),
      type_(new msg_intf::msg::ByteArrayPubSubType()),
      running_(false) {}

RpcRosPub::Impl::~Impl() {
    running_ = false;
    while (true) {
        // Wait for all of the messages to be sent out
        std::unique_lock<std::mutex> queue_lock(queue_mtx_);
        if (msg_queue_.empty())
            break;
        queue_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(kMessageWaitTimeout));
    }
    if (msg_send_thread_.joinable())
        msg_send_thread_.join();

    if (writer_ != nullptr) {
        publisher_->delete_datawriter(writer_);
    }
    if (publisher_ != nullptr) {
        participant_->delete_publisher(publisher_);
    }
    if (topic_ != nullptr) {
        participant_->delete_topic(topic_);
    }
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool RpcRosPub::Impl::Init(const std::string& topic, int domain_id) {
    topic_name_ = topic;
    domain_id_ = domain_id;

    eprosima::fastdds::dds::DomainParticipantQos participantQos;
    participantQos.name("Participant_publisher");
    participant_ =
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

    if (participant_ == nullptr) {
        return false;
    }

    // Register the Type
    type_.register_type(participant_);

    // Create the publications Topic
    topic_ =
        participant_->create_topic(topic_name_, "msg_intf::msg::ByteArray", eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr) {
        return false;
    }

    // Create the Publisher
    publisher_ = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr);

    if (publisher_ == nullptr) {
        return false;
    }

    // Create the DataWriter
    writer_ = publisher_->create_datawriter(topic_, eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT, &listener_);

    if (writer_ == nullptr) {
        return false;
    }

    std::cout << "Publisher for topic " << topic_ << " created, waiting for Subscribers." << std::endl;
    return true;
}

void RpcRosPub::Impl::Run() {
    running_ = true;
    msg_send_thread_ = std::thread(&RpcRosPub::Impl::MsgSendThread, this);
}
void RpcRosPub::Impl::Stop() {
    running_ = false;
}

void RpcRosPub::Impl::Publish(const std::shared_ptr<std::vector<uint8_t>>& item) {
    if (listener_.matched_ == 0) {
        // std::cout << "no listener" << std::endl;
        return;
    }
    {
        std::lock_guard<std::mutex> lk(queue_mtx_);
        msg_queue_.push(item);
    }
    queue_cv_.notify_one();
}

void RpcRosPub::Impl::MsgSendThread() {
    while (running_ || !msg_queue_.empty()) {
        while (listener_.matched_ == 0 && running_) std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::unique_lock<std::mutex> queue_lk(queue_mtx_);
        if (!queue_cv_.wait_for(queue_lk, std::chrono::milliseconds(kMessageWaitTimeout),
                               [&]() { return !msg_queue_.empty(); }))
            continue;

        if (running_) {
            auto item = msg_queue_.front();
            msg_queue_.pop();
            queue_lk.unlock();

            auto msg = std::make_shared<msg_intf::msg::ByteArray>();
            msg->data(reinterpret_cast<const std::vector<uint8_t>&>(*item));
            if (!writer_->write(msg.get())) {
                std::cout << "can not send!" << std::endl;
            }
        } else {
            auto empty = std::queue<std::shared_ptr<std::vector<uint8_t>>>();
            swap(msg_queue_, empty);
        }
    }
}

RpcRosPub::RpcRosPub() : impl_(std::make_unique<Impl>()) {}

RpcRosPub::~RpcRosPub() = default;

bool RpcRosPub::Init(const std::string& topic, int domain_id) {
    return impl_->Init(topic, domain_id);
}

void RpcRosPub::Run() {
    return impl_->Run();
}

void RpcRosPub::Stop() {
    impl_->Stop();
}

void RpcRosPub::Publish(const std::shared_ptr<std::vector<uint8_t>>& item) {
    impl_->Publish(item);
}

}  // namespace rpc
