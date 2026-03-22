/**
 * @file rpc_data.cpp
 * @author whoami
 * @brief
 * @version 0.1
 * @date 2026-03-22
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "rpc/rpc_data.h"

#include <atomic>

namespace rpc {

RpcReq::RpcReq() : RpcReq(GenerateReqID(), RequestType::ACT, RequestAttribute::APPEND, 100, 0) {}

RpcReq::RpcReq(uint64_t req_id, RequestType type, RequestAttribute attribute, uint8_t priority, uint16_t debug_id) {
    SetId(req_id);
    SetType(type);
    SetAttribute(attribute);
    SetPriority(priority);
    SetDebugId(debug_id);
}

RpcReq::RpcReq(RequestType type, RequestAttribute attribute, uint8_t priority, uint16_t debug_id)
    : RpcReq(GenerateReqID(), type, attribute, priority, debug_id) {}

uint8_t RpcReq::node_id_ = 0;

uint64_t RpcReq::GenerateReqID(uint8_t flag) {
    static volatile std::atomic<uint32_t> uuid(0);
    uint64_t temp = 0;
    uint64_t ret_id = 0;
    struct timespec t{};
    clock_gettime(CLOCK_MONOTONIC, &t);

    // Note: Snowflake based unique ID generator,
    // +-----------+---------+------+-------+
    // | timestamp | node id | flag | count |
    // +-----------+---------+------+-------+
    //    32 bits     8 bits  4 bits 20 bits
    temp = static_cast<uint64_t>(t.tv_sec);
    ret_id = temp << 32;
    temp = node_id_;
    ret_id |= temp << 24;
    temp = flag;
    ret_id |= temp << 20;
    temp = uuid++;
    ret_id |= (temp & 0xFFFFF);
    return ret_id;
}

void RpcReq::SetNodeId(uint8_t node_id) {
    node_id_ = node_id;
}

std::shared_ptr<RpcReq> RpcReq::Clone() const {
    std::shared_ptr<RpcReq> new_req = std::make_shared<RpcReq>(0);
    new_req->CopyFrom(*this);
    return new_req;
}

void RpcReq::Clone(const RpcReq& obj) {
    CopyFrom(obj);
}

RpcReply::RpcReply(const RpcReq& request, ReplyAttribute attribute)
    : RpcReply(request.Id(), request.SendNodeId(), attribute) {
    topic_ = request.topic_;
}

RpcReply::RpcReply(uint64_t req_id, uint8_t reply_node_id, ReplyAttribute attribute) {
    SetReqId(req_id);
    SetAttribute(attribute);
    reply_node_id_ = reply_node_id;
}

std::shared_ptr<RpcReply> RpcReply::Clone() const {
    std::shared_ptr<RpcReply> new_req = std::make_shared<RpcReply>();
    new_req->CopyFrom(*this);
    return new_req;
}

void RpcReply::Clone(const RpcReply& obj) {
    CopyFrom(obj);
}

}