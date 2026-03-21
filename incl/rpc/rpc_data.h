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

#include <functional>
namespace rpc {

class RpcReq;
class RpcReply;

using ReqCb = std::function<void(const RpcReq& req)>;
using RespCb = std::function<void(const RpcReply& reply)>;
using DataCb = std::function<void(uint8_t* data, uint32_t len)>;

// rpc request data
enum RequestAttribute {
    OVERWRITE = 0; APPEND = 1; REPLY_ACK = 2;
}
enum RequestType {
    ACT = 0; CANCEL = 1;
}

class RpcReqBase {
    uint64 id;
    RequestType type;
    RequestAttribute attribute;
    uint32 priority;      // uint8_t??
    uint32 debug_id;      // uint16_t??
    uint32 send_node_id;  // auto fill, uint8_t??
    uint64 timestamp;
};

}  // namespace rpc