/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_CCSRC_CXX_API_FACTORY_H
#define MINDSPORE_CCSRC_CXX_API_FACTORY_H
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "utils/utils.h"

namespace mindspore {
inline enum DeviceType g_device_target = kInvalidDeviceType;

static inline LogStream &operator<<(LogStream &stream, DeviceType device_type) {
  switch (device_type) {
    case kAscend:
      stream << "Ascend";
      break;
    case kAscend910:
      stream << "Ascend910";
      break;
    case kAscend310:
      stream << "Ascend310";
      break;
    case kGPU:
      stream << "GPU";
      break;
    case kCPU:
      stream << "CPU";
      break;
    default:
      stream << "[InvalidDeviceType: " << static_cast<int>(device_type) << "]";
      break;
  }
  return stream;
}

template <class T>
class Factory {
  using U = std::function<std::shared_ptr<T>()>;

 public:
  Factory(const Factory &) = delete;
  void operator=(const Factory &) = delete;

  static Factory &Instance() {
    static Factory instance;
    return instance;
  }

  void Register(U &&creator) { creators_.push_back(creator); }

  std::shared_ptr<T> Create(enum DeviceType device_type) {
    for (auto &item : creators_) {
      MS_EXCEPTION_IF_NULL(item);
      auto val = item();
      if (val->CheckDeviceSupport(device_type)) {
        return val;
      }
    }
    MS_LOG(WARNING) << "Unsupported device target " << device_type;
    return nullptr;
  }

 private:
  Factory() = default;
  ~Factory() = default;
  std::vector<U> creators_;
};

template <class T>
class Registrar {
  using U = std::function<std::shared_ptr<T>()>;

 public:
  explicit Registrar(U creator) { Factory<T>::Instance().Register(std::move(creator)); }
  ~Registrar() = default;
};

#define API_FACTORY_REG(BASE_CLASS, DERIVE_CLASS)                          \
  static const Registrar<BASE_CLASS> g_api_##DERIVE_CLASS##_registrar_reg( \
    []() { return std::make_shared<DERIVE_CLASS>(); });
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_CXX_API_FACTORY_H
