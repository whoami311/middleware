/**
 * @file rpc_data.h
 * @author whoami
 * @brief
 * @version 0.1
 * @date 2026-03-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace rpc {

class RpcReq;
class RpcReply;

using ReqCb = std::function<void(const RpcReq& req)>;
using RepCb = std::function<void(const RpcReply& reply)>;
using DataCb = std::function<void(uint8_t* data, uint32_t len)>;

enum class RpcBackEnd {
    FASTDDS_ROS = 0
};

// rpc request data
enum class RequestAttribute {
    OVERWRITE = 0,
    APPEND = 1,
    REPLY_ACK = 2
};

enum class RequestType {
    ACT = 0,
    CANCEL = 1
};

class RpcReqBase {
public:
    void SetId(uint64_t id) {
        id_ = id;
    }

    void SetType(RequestType type) {
        type_ = type;
    }

    void SetAttribute(RequestAttribute attr) {
        attribute_ = attr;
    }

    void SetPriority(uint32_t priority) {
        priority_ = priority;
    }

    void SetDebugId(uint32_t debug_id) {
        debug_id_ = debug_id;
    }

    void SetTimestamp(uint64_t ts) {
        timestamp_ = ts;
    }

    uint64_t Id() const {
        return id_;
    }

    RequestAttribute Attribute() const {
        return attribute_;
    }

    uint32_t Priority() const {
        return priority_;
    }

    uint32_t SendNodeId() const {
        return send_node_id_;
    }

    void CopyFrom(const RpcReqBase& from) {
        id_ = from.id_;
        type_ = from.type_;
        attribute_ = from.attribute_;
        priority_ = from.priority_;
        debug_id_ = from.debug_id_;
        send_node_id_ = from.send_node_id_;
        timestamp_ = from.timestamp_;
    }

private:
    uint64_t id_;
    RequestType type_;
    RequestAttribute attribute_;
    uint32_t priority_;      // uint8_t??
    uint32_t debug_id_;      // uint16_t??
    uint32_t send_node_id_;  // auto fill, uint8_t??
    uint64_t timestamp_;
};

class RpcReq : public RpcReqBase {
public:
    enum class State {
        WAIT_SEND,
        WAIT_ACK,
        WAIT_RESULT,
        FINISHED
    };

    RpcReq();
    RpcReq(uint64_t req_id, RequestType type = RequestType::ACT, RequestAttribute attribute = RequestAttribute::APPEND,
           uint8_t priority = 100, uint16_t debug_id = 0);
    RpcReq(RequestType type, RequestAttribute attribute = RequestAttribute::APPEND, uint8_t priority = 100,
           uint16_t debug_id = 0);
    ~RpcReq() {}

    // clone
    std::shared_ptr<RpcReq> Clone() const;
    void Clone(const RpcReq& obj);

    // generate request id
    // @flag(0x01~0x0F): if you want generate special request package, such as
    // server reply data in
    // periodic, this time, the package always exist in remote server
    static uint64_t GenerateReqID(uint8_t flag = 0x00);
    // rpc private
    static void SetNodeId(uint8_t node_id);

    // for priority_queue auto sort
    bool operator<(const RpcReq& a) const {
        if (Priority() > a.Priority()) {
            return true;
        }
        return timestamp_ < a.timestamp_;
    }

    RepCb callback_rep_;
    ReqCb callback_req_;
    std::string topic_;
    State state_ = State::WAIT_SEND;
    uint64_t timestamp_;

private:
    static uint8_t node_id_;
};

// rpc reply data
enum class ReplyAttribute {
    ACK = 0,
    STATUS = 1,
    RESULT = 2,
    OW_CANCEL = 3  // being overwritten, cancelling
};

class RpcReplyBase {
public:
    void SetReqId(uint64_t req_id) {
        req_id = req_id;
    }

    void SetAttribute(ReplyAttribute attribute) {
        attribute_ = attribute;
    }

    uint64_t ReqId() const {
        return req_id_;
    }

    ReplyAttribute Attribute() const {
        return attribute_;
    }

    uint64_t Timestamp() const {
        return timestamp_;
    }

    void CopyFrom(const RpcReplyBase& from) {
        req_id_ = from.req_id_;
        attribute_ = from.attribute_;
        timestamp_ = from.timestamp_;
    }

private:
    uint64_t req_id_;  // from request package id
    ReplyAttribute attribute_;
    uint64_t timestamp_;
};

class RpcReply : public RpcReplyBase {
public:
    RpcReply() {}
    RpcReply(const RpcReq& request, ReplyAttribute attribute = ReplyAttribute::RESULT);
    RpcReply(uint64_t req_id, uint8_t reply_node_id, ReplyAttribute attribute = ReplyAttribute::RESULT);
    ~RpcReply() {}

    // clone
    std::shared_ptr<RpcReply> Clone() const;
    void Clone(const RpcReply& obj);

    std::string topic_;
    uint64_t timestamp_;
    uint8_t reply_node_id_;
};

}  // namespace rpc