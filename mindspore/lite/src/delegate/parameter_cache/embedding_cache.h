/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_LITE_EMBEDDING_CACHE_H_
#define MINDSPORE_LITE_EMBEDDING_CACHE_H_
#include <cmath>
#include <algorithm>
#include <memory>
#include "include/api/status.h"
#include "include/api/data_type.h"
#include "src/common/log_adapter.h"
#include "src/delegate/parameter_cache/cache_algorithm.h"
#include "src/delegate/parameter_cache/cache_mem_base.h"

namespace mindspore {
namespace cache {
class EmbeddingCache {
 public:
  EmbeddingCache(size_t vocab_size, size_t host_cache_size, size_t device_cache_size, size_t device_start_index,
                 size_t embedding_size, size_t batch_elements, DataType data_type, void *host_addr, int rank_id,
                 int rank_group_size)
      : vocab_size_(vocab_size),
        host_cache_size_(host_cache_size),
        device_cache_size_(device_cache_size),
        device_start_index_(device_start_index),
        embedding_size_(embedding_size),
        batch_elements_(batch_elements),
        data_type_(data_type),
        host_addr_(host_addr),
        rank_id_(rank_id),
        rank_group_size_(rank_group_size) {
    auto local_shard_size = static_cast<int>(std::ceil(static_cast<float>(vocab_size_) / rank_group_size_));
    min_host_index_ = local_shard_size * rank_id_;
    max_host_index_ = std::min(min_host_index_ + local_shard_size, static_cast<int>(vocab_size_));
    host_cache_size_ = max_host_index_ - min_host_index_;
    switch (data_type_) {
      case DataType::kNumberTypeFloat16:
        sizeof_data_type_ = sizeof(int16_t);
        break;
      default:
        sizeof_data_type_ = sizeof(float);
    }
    MS_LOG(INFO) << "rank_group_size_ num:" << rank_group_size_ << ", rank id:" << rank_id_
                 << ", vocab_size_:" << vocab_size_ << ", host_cache_size_:" << host_cache_size_
                 << ", device_cache_size_:" << device_cache_size_ << ", embedding_size_:" << embedding_size_
                 << ", batch_elements_:" << batch_elements_ << ", index begin:" << min_host_index_
                 << ", index end:" << max_host_index_;
  }
  ~EmbeddingCache();
  Status Init(uint32_t device_id, const void *context);
  Status SetHostCacheAddr(void *addr, size_t size);
  Status SetDeviceCacheAddr(void *host_mem_addr, size_t size);
  Status CheckCacheHit(const int *batch_ids, const size_t batch_ids_len, int *hash_index);
  size_t GetDeviceStartIndex() { return device_start_index_; }

 private:
  std::shared_ptr<cache::CacheMemBase> device_cache_{nullptr};
  std::shared_ptr<CacheAlgorithm> cache_{nullptr};

  size_t vocab_size_{0};         // total size
  size_t host_cache_size_{0};    // local host size
  size_t device_cache_size_{0};  // local device cache size
  size_t device_start_index_{0};
  size_t embedding_size_{0};
  size_t batch_elements_{0};

  DataType data_type_{DataType::kNumberTypeFloat32};
  size_t sizeof_data_type_{0};

  void *device_addr_{nullptr};  // hash_info.device_address.addr
  void *host_addr_{nullptr};

  int *hash_swap_index_addr_;  // embedding_device_cache_->hash_swap_index_addr_
  void *hash_swap_value_addr_;
  void *hash_swap_value_device_addr_;

  int rank_id_;
  int rank_group_size_;
  int min_host_index_{0};
  int max_host_index_{0};
};
}  // namespace cache
}  // namespace mindspore
#endif  // MINDSPORE_LITE_EMBEDDING_CACHE_H_
