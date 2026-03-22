/**
 * @file rpc_base.h
 * @author whoami
 * @brief
 * @version 0.1
 * @date 2026-03-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <memory>

#include "rpc_data.h"

namespace rpc {

/**
 * @brief RpcBase is rpc send data
 *
 */
class RpcBase {
public:
    /**
     * @brief Construct a new Rpc Base object
     *
     */
    RpcBase();

    /**
     * @brief Destroy the Rpc Base object
     *
     */
    virtual ~RpcBase();

    /**
     * @brief register read topic
     *
     * @param topic topic name
     * @param data_callback data callback
     * @return true means successfully, false means failed
     */
    virtual bool RegisterReadTopic(const std::string& topic, DataCb data_callback) = 0;

    virtual bool RegisterWriteTopic(const std::string& topic, DataCb data_callback) = 0;

    virtual bool Write(const std::string& topic, void* data, int len) = 0;
};

}  // namespace rpc