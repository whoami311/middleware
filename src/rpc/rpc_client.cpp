/**
 * @file rpc_client.cpp
 * @author whoami
 * @brief
 * @version 0.1
 * @date 2026-03-22
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "rpc/rpc_client.h"
#include "rpc/rpc_common.h"

namespace rpc {

class RpcClient::Impl {
public:
    explicit Impl(RpcBackEnd backend);
    ~Impl();

private:
    void Send(const std::string& topic, const RpcReq& request, RepCb reply_callback);
    void Send(const std::shared_ptr<RpcReq>& request);
    void ResetDuplicateReply();
    void SendThread();
    const RpcBackEnd backend_;
    std::unordered_map<std::string, std::shared_ptr<RpcBase>> rpc_port_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, std::shared_ptr<RpcReq>>> request_map_;
    std::deque<std::shared_ptr<RpcReq>> resend_queue_;
    std::unordered_map<uint64_t, std::set<Timestamp>> reply_id_map_;

    std::atomic<bool> running_;
    std::thread send_thread_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    friend RpcClient;
};

RpcClient::Impl::Impl(RpcBackEnd backend) : backend_(backend), running_(true) {
    send_thread_ = std::thread(&Impl::SendThread, this);
}

RpcClient::Impl::~Impl() {
    running_ = false;
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        resend_queue_.clear();
    }
    queue_cv_.notify_all();
    if (send_thread_.joinable()) {
        send_thread_.join();
    }
}

void RpcClient::Impl::Send(const std::string& topic, const RpcReq& request, RepCb reply_callback) {
    if (!rpc_port_.count(topic)) {
        switch (backend_) {
            case RpcBackEnd::FASTDDS_ROS:
                rpc_port_[topic] = std::make_shared<RpcRosPubSub>();
                break;
            default:
                std::cout << "wrong backend setting plz check!" << std::endl;
                break;
        }
        if (!rpc_port_[topic]->RegisterWriteTopic(topic, [&, topic](uint8_t* data, uint32_t len) {
                RpcReply reply;
                if (!reply.ParseFromArray(data, len)) {
                    std::cout << "RPCReply deserialize failed" << std::endl;
                    return;
                }

                const auto id = reply.ReqId();
                const auto attribute = reply.Attribute();
                const auto timestamp = reply.Timestamp();

                std::cout << "**** recv reply [" << id << "]->[" << attribute << ":" << timestamp << "]" << std::endl;

                bool need_cb = false;
                // call reply callback
                std::unique_lock<std::mutex> lock(queue_mutex_);
                if (reply_id_map_.count(id) && reply_id_map_[id].count(timestamp)) {
                    return;
                }

                auto& request_map = request_map_[topic];
                auto it = request_map.find(id);
                if (it != request_map.end()) {
                    auto request = it->second;
                    // handle attribute
                    switch (attribute) {
                        case ReplyAttribute::ACK:
                            request->state_ = RpcReq::WAIT_RESULT;
                            break;
                        case ReplyAttribute::STATUS:
                            request->state_ = RpcReq::WAIT_RESULT;
                            need_cb = true;
                            break;
                        case ReplyAttribute::RESULT:
                            request_map.erase(it);
                            need_cb = true;
                            break;
                        case ReplyAttribute::OW_CANCEL:
                            request_map.erase(it);
                            break;
                        default:
                            break;
                    }
                    // send reply ack
                    auto reply_ack = std::make_shared<RpcReq>(id, proto::RequestType, RequestAttribute::REPLY_ACK);
                    reply_ack->topic_ = topic;
                    reply_ack->set_timestamp(timestamp);
                    reply_id_map_[id].insert(timestamp);
                    lock.unlock();
                    Send(reply_ack);

                    if (need_cb) {
                        request->callback_rep_(reply);
                    }
                }
            })) {
            std::cout << "Cannot send!" << std::endl;
            rpc_port_.erase(topic);
            return;
        }
    }

    auto new_request = request.Clone();
    new_request->callback_rep_ = std::move(reply_callback);
    new_request->topic_ = topic;
    new_request->timestamp_ = util::GetCurrentMonotonicMicroSec();
    new_request->SetTimestamp(new_request->timestamp_);

    const auto id = new_request->id();
    std::unique_lock<std::mutex> lock(queue_mutex_);
    auto& request_map = request_map_[topic];
    if (request_map.count(id)) {
        std::cout << "==**==Exist request id package:" << id << ", topic: [" << topic << "]" << std::endl;
    }
    if (new_request->Attribute() == RequestAttribute::OVERWRITE) {
        // Remove all existed requests
        request_map.clear();
    }
    // Add request id into map, and push into resend_queue
    request_map[id] = new_request;
    resend_queue_.push_back(new_request);
    lock.unlock();
    queue_cv_.notify_one();

    Send(new_request);
}

void RpcClient::Impl::Send(const std::shared_ptr<RpcReq>& request) {
    const auto& topic = request->topic_;
    std::vector<uint8_t> buffer(request->ByteSizeLong());
    if (!request->SerializeToArray(buffer.data(), buffer.size())) {
        std::cout << "RpcReq serialize failed" << std::endl;
        return;
    }

    if (!rpc_port_[topic]->Write(topic, buffer.data(), buffer.size())) {
        std::cout << "RpcReq send error!" << std::endl;
        return;
    }

    std::cout << "**** sent req [" << request->id() << "]->[" << request->attribute() << ":" << request->timestamp()
               << "] with topic [" << topic << "]" << std::endl;
    request->state_ = RpcReq::WAIT_ACK;
}

void RpcClient::Impl::ResetDuplicateReply() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    for (auto it = reply_id_map_.begin(), end = reply_id_map_.end(); it != end;) {
        auto& set = it->second;
        // Clear old reply timestamps
        set.erase(set.begin(), set.lower_bound(util::GetCurrentMonotonicMicroSec() - kDuplicateDataFilter));
        if (set.empty()) {
            // empty id
            it = reply_id_map_.erase(it);
        } else {
            ++it;
        }
    }
}

void RpcClient::Impl::SendThread() {
    constexpr int kDefaultWaitMilliSecond = 500;
    int wait_ms = kDefaultWaitMilliSecond;

    std::unique_lock<std::mutex> lock(queue_mutex_, std::defer_lock);
    while (running_) {
        std::vector<std::shared_ptr<RpcReq>> send_queue;
        lock.lock();
        if (!queue_cv_.wait_for(lock, std::chrono::milliseconds(wait_ms), [&] {
                wait_ms = kDefaultWaitMilliSecond;
                const auto current_time = util::GetCurrentMonotonicMicroSec();
                while (!resend_queue_.empty()) {
                    auto request = resend_queue_.front();
                    // Request not in request map, might have been overwritten
                    if (!request_map_[request->topic_].count(request->id()) ||
                        // No longer need resending
                        request->state_ > RpcReq::WAIT_ACK) {
                        resend_queue_.pop_front();
                        continue;
                    }
                    if (current_time - request->timestamp_ > kRetrySendTimeout) {
                        // Need resending, move to back, and add to send_queue
                        resend_queue_.pop_front();
                        request->timestamp_ = current_time;
                        resend_queue_.push_back(request);
                        send_queue.push_back(request);
                        continue;
                    } else {
                        // Dynamic sleep time for next resend request
                        wait_ms = (current_time + kRetrySendTimeout - request->timestamp_) / 1000;
                        break;
                    }
                }
                return !send_queue.empty();
            })) {
            lock.unlock();
            ResetDuplicateReply();
            continue;
        }
        lock.unlock();

        // resend requests
        for (const auto& request : send_queue) {
            Send(request);
        }
    }
}

RpcClient::RpcClient(std::string name, RpcBackEnd backend)
    : Client(std::move(name)), impl_(std::make_shared<Impl>(backend)) {}

RpcClient::~RpcClient() = default;

void RpcClient::Send(const std::string& topic, const RpcReq& request, RepCb reply_callback) {
    impl_->Send(topic, request, std::move(reply_callback));
}

}  // namespace rpc