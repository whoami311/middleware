/**
 * @file rpc_client.h
 * @author whoami
 * @brief 
 * @version 0.1
 * @date 2026-03-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

namespace rpc {

/**
 * @brief RpcClient is rpc request data
 */
class RpcClient {
 public:
  /**
   * @brief Default constructor.
   */
  explicit RpcClient(std::string name = "RpcClient",
                     RpcBackEnd backend = RpcBackEnd::FASTDDS_PUB_SUB);

  /**
   * @brief Destructor.
   */
  ~RpcClient();

  void Send(const std::string &topic, const RPCRequest &request,
            rep_callback_t reply_callback);

 private:
  class Impl;

  std::unique_ptr<Impl> impl_;
};

}