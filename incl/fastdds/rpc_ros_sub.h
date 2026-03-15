/**
 * @file rpc_ros_sub.h
 * @author whoami
 * @brief ros subscriber by fastdds
 * @version 0.1
 * @date 2026-03-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <functional>
#include <memory>
#include <vector>

namespace rpc {

/**
 * @brief callback type of RpcRosSubscriber.
 * @param item subscribed data
 */
using HandlerFuncType = std::function<void(const std::shared_ptr<std::vector<uint8_t>>& item)>;

/**
 * @brief subscriber for rpc communication with ROS2 by fastdds.
 *
 */
class RpcRosSub {
public:
    /**
     * @brief Construct a new Rpc Ros Sub object
     *
     */
    RpcRosSub();

    /**
     * @brief Destroy the Rpc Ros Sub object
     *
     */
    ~RpcRosSub();

    /**
     * @brief Init subscriber by fastdds
     *
     * @param topic topic of subscriber
     * @param handler callback for subscriber
     * @param domain_id domain of subscriber
     * @param multi_thread_handle multi thread handle or not
     * @return true means success, false means failed.
     */
    bool Init(const std::string& topic, const HandlerFuncType& handler, int domain_id = 0,
              const bool multi_thread_handle = false);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace rpc