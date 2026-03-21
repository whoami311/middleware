/**
 * @file rpc_ros_pub.h
 * @author whoami
 * @brief ros publisher by fastdds
 * @version 0.1
 * @date 2026-03-14
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <memory>
#include <vector>

namespace rpc {

/**
 * @brief publisher for rpc communication with ROS2 by fastdds
 *
 */
class RpcRosPub {
public:
    /**
     * @brief Construct a new Rpc Ros Pub object
     *
     */
    RpcRosPub();
    /**
     * @brief Destroy the Rpc Ros Pub object
     *
     */
    ~RpcRosPub();

    /**
     * @brief Init publisher by fastdds
     *
     * @param topic topic of publisher
     * @param domain_id domain of publisher
     * @return true means success, false means failed.
     */
    bool Init(const std::string& topic, int domain_id = 0);

    /**
     * @brief start the processing thread.
     *
     */
    void Run();

    /**
     * @brief stop the processing thread.
     *
     */
    void Stop();

    /**
     * @brief publish data.
     *
     * @param item data for publishing
     */
    void Publish(const std::shared_ptr<std::vector<uint8_t>>& item);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace rpc
