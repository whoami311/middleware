/**
 * @file rpc_ros_pub_sub.h
 * @author whoami
 * @brief
 * @version 0.1
 * @date 2026-03-22
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <memory>

#include "rpc/rpc_base.h"

namespace rpc {

class RpcRosPubSub : public RpcBase {
public:
    RpcRosPubSub();

    ~RpcRosPubSub();

    /**
     * @brief register reader with topic and callback
     * @param topic topic of reader
     * @param data_callback callback of reader
     * @return true means success, false means failed.
     */
    bool RegisterReadTopic(const std::string& topic, DataCb data_callback) override;

    /**
     * @brief register writer with topic and callback
     * @param topic topic of writer
     * @param data_callback callback of writer
     * @return true means success, false means failed.
     */
    bool RegisterWriteTopic(const std::string& topic, DataCb data_callback) override;

    /**
     * @brief publish data
     * @param topic topic of publisher
     * @param data data to publish
     * @param len len of data to publish
     * @return true means success, false means failed.
     */
    bool Write(const std::string& topic, void* data, int len) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace rpc