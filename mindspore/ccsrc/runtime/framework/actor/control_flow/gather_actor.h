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

#ifndef MINDSPORE_CCSRC_RUNTIME_FRAMEWORK_ACTOR_CONTROLFLOW_GATHER_ACTOR_H_
#define MINDSPORE_CCSRC_RUNTIME_FRAMEWORK_ACTOR_CONTROLFLOW_GATHER_ACTOR_H_

#include <vector>
#include <string>
#include <memory>
#include <stack>
#include <utility>
#include <algorithm>
#include "utils/hash_map.h"
#include "runtime/framework/actor/actor_common.h"
#include "runtime/framework/actor/control_flow/control_actor.h"

namespace mindspore {
namespace runtime {

// Gather actor will be used in the control flow. When the subgraph is called, the real parameters need to be put
// together and sent to the subgraph.
class GatherActor : public ControlActor {
 public:
  GatherActor(const std::string &name, const AID &memory_manager_aid, const std::vector<KernelWithIndex> &parameters,
              const AnfNodePtr &node);
  ~GatherActor() override = default;
  const mindspore::HashMap<FuncGraph *, std::vector<AID>> &output_data_with_branch_id_arrows() const {
    return output_data_with_branch_id_arrows_;
  }

 protected:
  void FetchInput(OpContext<DeviceTensor> *const context) override;
  void SendOutput(OpContext<DeviceTensor> *const context) override;
  void IncreaseDynamicRefCounts(OpContext<DeviceTensor> *const context) override;

 private:
  friend class ControlNodeScheduler;

  void FetchOutput(OpRealParameterWithBranchID *const output, OpContext<DeviceTensor> *const context);
  void SendMemoryFreeReq(OpContext<DeviceTensor> *const context) override;

  // There will be multiple output branches for gather actor according the funcgraph in partial.
  mindspore::HashMap<FuncGraph *, std::vector<AID>> output_data_with_branch_id_arrows_;
};

using GatherActorPtr = std::shared_ptr<GatherActor>;
}  // namespace runtime
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_RUNTIME_FRAMEWORK_ACTOR_CONTROLFLOW_GATHER_ACTOR_H_
