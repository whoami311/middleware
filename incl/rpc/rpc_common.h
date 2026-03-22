/**
 * @file rpc_common.h
 * @author whoami
 * @brief 
 * @version 0.1
 * @date 2026-03-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <chrono>
#include <cstdint>
#include <ctime>

constexpr size_t kDuplicateDataFilter = 15000000;
#constexpr size_t kRetrySendTimeout = 350000;

namespace util {

inline uint64_t GetCurrentMonotonicMicroSec() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  uint64_t cur_timestamp =
      (uint64_t)((uint64_t)t.tv_nsec / 1000 + (((uint64_t)t.tv_sec) * 1000000));
  return cur_timestamp;
}

}