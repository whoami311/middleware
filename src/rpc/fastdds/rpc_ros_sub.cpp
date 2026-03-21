/**
 * @file rpc_ros_sub.cpp
 * @author whoami
 * @brief ros subscriber by fastdds
 * @version 0.1
 * @date 2026-03-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "rpc/fastdds/rpc_ros_sub.h"

#include <condition_variable>
#include <iostream>
#include <queue>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "ByteArrayPubSubTypes.hpp"

namespace rpc {

constexpr size_t kMsgQueueSizeLimit = 100;
constexpr size_t kMsgWaitTimeout = 500;

class RpcRosSub::Impl {
public:
    Impl();
    ~Impl();

    bool Init(const std::string& topic, const HandlerFuncType& handler, int domain_id = 0,
              bool multi_thread_handle = false);

private:
    class SubListener : public eprosima::fastdds::dds::DataReaderListener {
    public:
        SubListener(const HandlerFuncType& handler, const std::string& topic, bool multi_thread_handle = false)
            : samples_(0),
              n_matched_(0),
              n_msg_(0),
              handler_(handler),
              topic_name_(topic),
              multi_thread_handle_(multi_thread_handle) {
            if (multi_thread_handle_) {
                std::cout << "Use multi_thread_handle" << std::endl;
                is_running_ = true;
                msg_handle_thread_ = std::thread(&SubListener::MessageHandleThread, this);
            }
        }
        ~SubListener() override {
            is_running_ = false;
            while (true) {
                // Wait for all of the messages to be sent out
                std::unique_lock<std::mutex> queue_lock(queue_mutex_);
                if (msg_queue_.empty())
                    break;
                queue_lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(kMsgWaitTimeout));
            }

            if (msg_handle_thread_.joinable())
                msg_handle_thread_.join();
        }

        void on_subscription_matched(eprosima::fastdds::dds::DataReader*,
                                     const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override {
            if (info.current_count_change == 1) {
                std::cout << "Subscriber matched." << std::endl;
                n_matched_++;
                std::cout << "Topic " << topic_name_ << " Subscriber matched" << std::endl;
            } else if (info.current_count_change == -1) {
                std::cout << "Subscriber unmatched." << std::endl;
                n_matched_--;
                std::cout << "Topic " << topic_name_ << " Subscriber unmatched" << std::endl;
            } else {
                std::cout << info.current_count_change
                          << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }

        void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
            eprosima::fastdds::dds::SampleInfo info;
            auto msg = std::make_shared<msg_intf::msg::ByteArray>();
            if (reader->take_next_sample(msg.get(), &info) == eprosima::fastdds::dds::RETCODE_OK) {
                if (info.valid_data) {
                    samples_++;
                    std::cout << "Message: " << " RECEIVED." << std::endl;
                    uint8_t* start = reinterpret_cast<uint8_t*>(msg->data().data());
                    std::vector<uint8_t> vec(start, start + msg->data().size());
                    auto pointer = std::make_shared<std::vector<uint8_t>>(vec);
                    if (!multi_thread_handle_) {
                        handler_(pointer);
                    } else {
                        std::lock_guard<std::mutex> lk(queue_mutex_);
                        msg_queue_.push(pointer);
                        if (msg_queue_.size() > kMsgQueueSizeLimit)
                            std::cout << "msg_queue_.size() = " << msg_queue_.size();

                        queue_cv_.notify_one();
                    }

                    n_msg_++;
                }
            }
        }

        std::atomic<int> samples_;

    private:
        // Message handle function
        void MessageHandleThread();

    private:
        int n_matched_;
        int n_msg_;
        HandlerFuncType handler_;
        std::string topic_name_;

        // whether run handle in a separated thread
        bool multi_thread_handle_;

        // Message handle thread
        std::thread msg_handle_thread_;

        // Message queue
        std::queue<std::shared_ptr<std::vector<uint8_t>>> msg_queue_;
        // Mutex for exclusive access of the message queue
        std::mutex queue_mutex_;
        // Conditional variable for notifying message update
        std::condition_variable queue_cv_;
        // running
        std::atomic<bool> is_running_;
    };

private:
    eprosima::fastdds::dds::DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    eprosima::fastdds::dds::DataReader* reader_;
    eprosima::fastdds::dds::Topic* topic_;
    eprosima::fastdds::dds::TypeSupport type_;

    std::shared_ptr<SubListener> listener_;
    msg_intf::msg::ByteArray my_type_;
    size_t msg_cnt = 0;
    std::string topic_name_;
    int domain_id_;

    friend RpcRosSub;
};

RpcRosSub::Impl::Impl()
    : participant_(nullptr),
      subscriber_(nullptr),
      reader_(nullptr),
      topic_(nullptr),
      type_(new msg_intf::msg::ByteArrayPubSubType()),
      listener_(nullptr),
      domain_id_(0) {}

RpcRosSub::Impl::~Impl() {
    if (reader_ != nullptr) {
        subscriber_->delete_datareader(reader_);
    }
    if (topic_ != nullptr) {
        participant_->delete_topic(topic_);
    }
    if (subscriber_ != nullptr) {
        participant_->delete_subscriber(subscriber_);
    }
    eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool RpcRosSub::Impl::Init(const std::string& topic, const HandlerFuncType& handler, int domain_id,
                           bool multi_thread_handle) {
    topic_name_ = topic;
    domain_id_ = domain_id;
    listener_ = std::make_shared<SubListener>(handler, topic, multi_thread_handle);
    eprosima::fastdds::dds::DomainParticipantQos participantQos;
    participantQos.name("Participant_subscriber");
    participant_ =
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

    if (participant_ == nullptr) {
        return false;
    }

    // Register the Type
    type_.register_type(participant_);

    // Create the subscriptions Topic
    topic_ =
        participant_->create_topic(topic_name_, "msg_intf::msg::ByteArray", eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr) {
        return false;
    }

    // Create the Subscriber
    subscriber_ = participant_->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (subscriber_ == nullptr) {
        return false;
    }

    // Create the DataReader
    reader_ = subscriber_->create_datareader(topic_, eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT, listener_.get());

    if (reader_ == nullptr) {
        return false;
    }

    std::cout << "Subscriber for topic " << topic_name_ << " created, waiting for Publishers.";

    return true;
}

void RpcRosSub::Impl::SubListener::MessageHandleThread() {
    while (true) {
        if (!is_running_ && msg_queue_.empty())
            break;

        std::unique_lock<std::mutex> queue_lock(queue_mutex_);
        if (!queue_cv_.wait_for(queue_lock, std::chrono::milliseconds(kMsgWaitTimeout),
                                [&] { return !msg_queue_.empty(); })) {
            continue;
        }
        auto msg = msg_queue_.front();
        msg_queue_.pop();
        queue_lock.unlock();
        // get the response object and parse it
        handler_(msg);
    }
}

RpcRosSub::RpcRosSub() : impl_(std::make_unique<Impl>()) {}

RpcRosSub::~RpcRosSub() = default;

bool RpcRosSub::Init(const std::string& topic, const HandlerFuncType& handler, int domain_id,
                            const bool multi_thread_handle) {
    return impl_->Init(topic, handler, domain_id, multi_thread_handle);
}

}  // namespace rpc