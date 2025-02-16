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

#include "tools/converter/import/primitive_adjust.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include "ops/batch_norm.h"
#include "ops/elu.h"
#include "ops/fused_batch_norm.h"
#include "ops/fusion/conv2d_transpose_fusion.h"
#include "ops/fusion/div_fusion.h"
#include "ops/fusion/exp_fusion.h"
#include "ops/fusion/l2_normalize_fusion.h"
#include "ops/fusion/layer_norm_fusion.h"
#include "ops/fusion/max_pool_fusion.h"
#include "ops/fusion/mul_fusion.h"
#include "ops/fusion/pad_fusion.h"
#include "ops/fusion/activation.h"
#include "ops/fusion/add_fusion.h"
#include "ops/fusion/adder_fusion.h"
#include "ops/fusion/arg_max_fusion.h"
#include "ops/fusion/arg_min_fusion.h"
#include "ops/fusion/avg_pool_fusion.h"
#include "ops/fusion/conv2d_backprop_filter_fusion.h"
#include "ops/fusion/conv2d_backprop_input_fusion.h"
#include "ops/fusion/conv2d_fusion.h"
#include "ops/partial.h"
#include "ops/fusion/partial_fusion.h"
#include "ops/fusion/pow_fusion.h"
#include "ops/fusion/prelu_fusion.h"
#include "ops/fusion/reduce_fusion.h"
#include "ops/fusion/scale_fusion.h"
#include "ops/fusion/slice_fusion.h"
#include "ops/fusion/sub_fusion.h"
#include "ops/fusion/tile_fusion.h"
#include "ops/fusion/topk_fusion.h"
#include "ops/grad/activation_grad.h"
#include "ops/grad/avg_pool_grad.h"
#include "ops/grad/batch_norm_grad.h"
#include "ops/grad/max_pool_grad.h"
#include "ops/gelu.h"
#include "ops/leaky_relu.h"
#include "ops/mat_mul.h"
#include "ops/reduce_all.h"
#include "ops/reduce_asum.h"
#include "ops/reduce_max.h"
#include "ops/reduce_mean.h"
#include "ops/reduce_min.h"
#include "ops/reduce_prod.h"
#include "ops/reduce_sum.h"
#include "ops/reduce_sum_square.h"
#include "ops/relu.h"
#include "ops/relu6.h"
#include "ops/resize.h"
#include "ops/resize_bilinear.h"
#include "ops/resize_nearest_neighbor.h"
#include "ops/sigmoid.h"
#include "ops/stack.h"
#include "ops/tanh.h"
#include "ops/sparse_softmax_cross_entropy_with_logits.h"
#include "ops/grad/resize_grad.h"
#include "tools/converter/parser/parser_utils.h"
#include "nnacl/op_base.h"
using mindspore::ops::kNameAdd;
using mindspore::ops::kNameAdder;
using mindspore::ops::kNameArgMax;
using mindspore::ops::kNameArgMin;
using mindspore::ops::kNameAvgPool;
using mindspore::ops::kNameAvgPoolGrad;
using mindspore::ops::kNameBatchNorm;
using mindspore::ops::kNameConv2D;
using mindspore::ops::kNameConv2DBackpropFilter;
using mindspore::ops::kNameConv2DBackpropInput;
using mindspore::ops::kNameConv2DTranspose;
using mindspore::ops::kNameDiv;
using mindspore::ops::kNameElu;
using mindspore::ops::kNameExp;
using mindspore::ops::kNameGeLU;
using mindspore::ops::kNameL2Normalize;
using mindspore::ops::kNameLayerNorm;
using mindspore::ops::kNameLeakyRelu;
using mindspore::ops::kNameMaxPool;
using mindspore::ops::kNameMaxPoolGrad;
using mindspore::ops::kNameMul;
using mindspore::ops::kNamePad;
using mindspore::ops::kNamePartial;
using mindspore::ops::kNamePartialFusion;
using mindspore::ops::kNamePow;
using mindspore::ops::kNamePReLU;
using mindspore::ops::kNameReduceAll;
using mindspore::ops::kNameReduceASum;
using mindspore::ops::kNameReduceMax;
using mindspore::ops::kNameReduceMean;
using mindspore::ops::kNameReduceMin;
using mindspore::ops::kNameReduceProd;
using mindspore::ops::kNameReduceSum;
using mindspore::ops::kNameReduceSumSquare;
using mindspore::ops::kNameReLU;
using mindspore::ops::kNameReLU6;
using mindspore::ops::kNameResizeBilinear;
using mindspore::ops::kNameResizeNearestNeighbor;
using mindspore::ops::kNameScale;
using mindspore::ops::kNameSigmoid;
using mindspore::ops::kNameSparseSoftmaxCrossEntropyWithLogits;
using mindspore::ops::kNameSub;
using mindspore::ops::kNameTanh;
using mindspore::ops::kNameTile;
using mindspore::ops::kNameTopK;

namespace mindspore {
namespace lite {
namespace {
constexpr auto kNameArgMaxWithValue = "ArgMaxWithValue";
constexpr auto kNameArgMinWithValue = "ArgMinWithValue";
constexpr auto kNameBatchMatMul = "BatchMatMul";
constexpr auto kNameFusedBatchNormEx = "FusedBatchNormEx";
constexpr auto kNameFusedBatchNormGradEx = "FusedBatchNormGradEx";
constexpr auto kNameFusedBatchNormGradCPU = "FusedBatchNormGradCPU";
constexpr auto kNameHSigmoid = "HSigmoid";
constexpr auto kNameHSigmoidGrad = "HSigmoidGrad";
constexpr auto kNameHSwish = "HSwish";
constexpr auto kNameHSwishGrad = "HSwishGrad";
constexpr auto kNameReluGrad = "ReluGrad";
constexpr auto kNameReLU6Grad = "ReLU6Grad";
constexpr auto kNameSigmoidGrad = "SigmoidGrad";
constexpr auto kNameEluGrad = "EluGrad";
constexpr auto kNameGeLUGrad = "GeLUGrad";
constexpr auto kNameSlice = "Slice";
constexpr auto kNameAvgPoolGradGpu = "AvgPoolGradGpu";
constexpr auto kNameAvgPoolGradCpu = "AvgPoolGradCpu";
constexpr auto kNameTanhGrad = "TanhGrad";
constexpr auto kNameResizeBilinearGrad = "ResizeBilinearGrad";
constexpr auto kNameResizeNearestNeighborGrad = "ResizeNearestNeighborGrad";
constexpr int kNCHW_H = 2;
constexpr int kNCHW_W = 3;

std::map<std::string, mindspore::ActivationType> activation_map = {{ops::kNameElu, mindspore::ELU},
                                                                   {ops::kNameGeLU, mindspore::GELU},
                                                                   {ops::kNameLeakyRelu, mindspore::LEAKY_RELU},
                                                                   {ops::kNameReLU, mindspore::RELU},
                                                                   {ops::kNameReLU6, mindspore::RELU6},
                                                                   {ops::kNameSigmoid, mindspore::SIGMOID},
                                                                   {ops::kNameTanh, mindspore::TANH},
                                                                   {kNameHSigmoid, mindspore::HSIGMOID},
                                                                   {kNameHSigmoidGrad, mindspore::HSIGMOID},
                                                                   {kNameHSwish, mindspore::HSWISH},
                                                                   {kNameHSwishGrad, mindspore::HSWISH},
                                                                   {kNameReluGrad, mindspore::RELU},
                                                                   {kNameReLU6Grad, mindspore::RELU6},
                                                                   {kNameSigmoidGrad, mindspore::SIGMOID},
                                                                   {kNameEluGrad, mindspore::ELU},
                                                                   {kNameGeLUGrad, mindspore::GELU},
                                                                   {kNameTanhGrad, mindspore::TANH}};

std::map<std::string, mindspore::ReduceMode> reduce_map = {
  {ops::kNameReduceAll, mindspore::Reduce_All}, {ops::kNameReduceASum, mindspore::Reduce_ASum},
  {ops::kNameReduceMax, mindspore::Reduce_Max}, {ops::kNameReduceMean, mindspore::Reduce_Mean},
  {ops::kNameReduceMin, mindspore::Reduce_Min}, {ops::kNameReduceProd, mindspore::Reduce_Prod},
  {ops::kNameReduceSum, mindspore::Reduce_Sum}, {ops::kNameReduceSumSquare, mindspore::Reduce_Sum_Square}};

int AttrAdjust(const PrimitivePtr &prim, const std::string &name, const std::vector<int> &position) {
  MS_ASSERT(prim != nullptr);
  if (prim->GetAttr(name) == nullptr) {
    return lite::RET_OK;
  }
  auto value_ptr = prim->GetAttr(name);
  if (utils::isa<ValueSequencePtr>(value_ptr)) {
    MS_CHECK_TRUE_MSG(value_ptr->cast<ValueSequencePtr>()->value().size() > 0, RET_ERROR, "value is empty.");
    if (value_ptr->cast<ValueSequencePtr>()->value().front()->type()->number_type() != kNumberTypeInt64) {
      MS_LOG(ERROR) << "the func is to adjust attr which is array, please check the attr.";
      return lite::RET_ERROR;
    }
  } else if (value_ptr->type()->number_type() != kNumberTypeInt64) {
    MS_LOG(ERROR) << "the func is to adjust attr which is array, please check the attr.";
    return lite::RET_ERROR;
  }
  auto origin_value = opt::CastToInt(prim->GetAttr(name));
  std::vector<int64_t> new_value;
  if (name == ops::kKernelSize && origin_value.size() == 1) {
    new_value.push_back(origin_value[0]);
    new_value.push_back(origin_value[0]);
  } else {
    for (auto index : position) {
      if (index >= static_cast<int>(origin_value.size())) {
        MS_LOG(ERROR) << "index is out of range.";
        return lite::RET_ERROR;
      }
      new_value.push_back(static_cast<int64_t>(origin_value[index]));
    }
  }
  prim->AddAttr(name, MakeValue(new_value));
  return lite::RET_OK;
}

template <typename T>
int MoveAttrMapCommon(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<T>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  dst_prim->SetAttrs(src_prim->attrs());
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrMapActivation(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto act_prim = std::make_shared<ops::Activation>();
  MS_CHECK_TRUE_MSG(act_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  act_prim->SetAttrs(src_prim->attrs());
  auto iter = activation_map.find(src_prim->name());
  if (iter == activation_map.end()) {
    MS_LOG(ERROR) << "activation mode is unsupported.";
    return lite::RET_ERROR;
  }
  act_prim->set_activation_type(iter->second);
  value_node->set_value(act_prim);
  return lite::RET_OK;
}

int MoveAttrMapActivationGrad(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto act_grad_prim = std::make_shared<ops::ActivationGrad>();
  MS_CHECK_TRUE_MSG(act_grad_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  act_grad_prim->SetAttrs(src_prim->attrs());
  auto iter = activation_map.find(src_prim->name());
  if (iter == activation_map.end()) {
    MS_LOG(ERROR) << "activation mode is unsupported.";
    return lite::RET_ERROR;
  }
  act_grad_prim->set_activation_type(iter->second);
  value_node->set_value(act_grad_prim);
  return lite::RET_OK;
}

int MoveAttrMapReduce(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<ops::ReduceFusion>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  dst_prim->SetAttrs(src_prim->attrs());
  auto iter = reduce_map.find(src_prim->name());
  if (iter == reduce_map.end()) {
    MS_LOG(ERROR) << "reduce mode is unsupported.";
    return lite::RET_ERROR;
  }
  dst_prim->set_mode(iter->second);
  dst_prim->set_coeff(1.0f);
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

namespace {
int AdjustConvAttr(const PrimitivePtr &prim) {
  auto status = AttrAdjust(prim, ops::kStride, {kNCHW_H, kNCHW_W});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust stride failed.";
    return status;
  }
  status = AttrAdjust(prim, ops::kDilation, {kNCHW_H, kNCHW_W});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust dilation failed.";
    return status;
  }
  status = AttrAdjust(prim, ops::kKernelSize, {0, 1});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust kernel size failed.";
    return status;
  }
  return RET_OK;
}
}  // namespace

int MoveAttrMapConv2D(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  PrimitivePtr dst_prim{nullptr};
  if (opt::CheckPrimitiveType(cnode, prim::kPrimConv2D)) {
    dst_prim = std::make_shared<ops::Conv2DFusion>();
  } else if (opt::CheckPrimitiveType(cnode, prim::kPrimConv2DTranspose)) {
    dst_prim = std::make_shared<ops::Conv2dTransposeFusion>();
  }
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  dst_prim->SetAttrs(src_prim->attrs());
  auto status = AdjustConvAttr(dst_prim);
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust conv attrs failed.";
    return status;
  }
  int64_t group = 1;
  if (dst_prim->GetAttr(ops::kGroup) != nullptr) {
    group = GetValue<int64_t>(dst_prim->GetAttr(ops::kGroup));
  }
  if (group > 1) {
    auto make_bool_ptr = MakeValue<bool>(true);
    MS_CHECK_TRUE_MSG(make_bool_ptr != nullptr, RET_NULL_PTR, "make_bool_ptr is nullptr.");
    dst_prim->AddAttr(ops::kIsDepthWise, make_bool_ptr);
  }
  auto make_value_ptr = MakeValue(group);
  MS_CHECK_TRUE_MSG(make_value_ptr != nullptr, RET_NULL_PTR, "make_value_ptr is nullptr.");
  dst_prim->AddAttr(ops::kGroup, make_value_ptr);
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrPool(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  PrimitivePtr dst_prim = nullptr;
  if (src_prim->name() == kNameAvgPool) {
    dst_prim = std::make_shared<ops::AvgPoolFusion>();
    MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  } else if (src_prim->name() == kNameMaxPool) {
    dst_prim = std::make_shared<ops::MaxPoolFusion>();
    MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  } else {
    MS_LOG(ERROR) << "unsupported pooling type.";
    return lite::RET_ERROR;
  }

  dst_prim->SetAttrs(src_prim->attrs());
  auto status = AttrAdjust(dst_prim, ops::kKernelSize, {2, 3});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust ksize failed.";
    return status;
  }
  status = AttrAdjust(dst_prim, ops::kStrides, {2, 3});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust strides failed.";
    return status;
  }
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrPoolGrad(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  PrimitivePtr dst_prim = nullptr;
  if (src_prim->name() == kNameAvgPoolGrad || src_prim->name() == kNameAvgPoolGradGpu ||
      src_prim->name() == kNameAvgPoolGradCpu) {
    dst_prim = std::make_shared<ops::AvgPoolGrad>();
    MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  } else if (src_prim->name() == kNameMaxPoolGrad) {
    dst_prim = std::make_shared<ops::MaxPoolGrad>();
    MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  } else {
    MS_LOG(ERROR) << "unsupported pooling type.";
    return lite::RET_ERROR;
  }
  dst_prim->SetAttrs(src_prim->attrs());
  auto status = AttrAdjust(dst_prim, ops::kKernelSize, {2, 3});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust ksize failed.";
    return status;
  }
  status = AttrAdjust(dst_prim, ops::kStrides, {2, 3});
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust strides failed.";
    return status;
  }
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrMapAdder(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<ops::AdderFusion>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  dst_prim->SetAttrs(src_prim->attrs());
  auto status = AdjustConvAttr(dst_prim);
  if (status != lite::RET_OK) {
    MS_LOG(ERROR) << "adjust conv attrs failed.";
    return status;
  }
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrMapLayerNorm(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<ops::LayerNormFusion>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  dst_prim->SetAttrs(src_prim->attrs());
  dst_prim->set_elementwise_affine(true);
  if (dst_prim->GetAttr(ops::kEpsilon) == nullptr) {
    dst_prim->set_epsilon(1e-7);
  }
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrMapResize(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<ops::Resize>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  auto size = GetValue<std::vector<int64_t>>(src_prim->GetAttr(ops::kSize));
  MS_CHECK_TRUE_MSG(size.size() > 1, RET_ERROR, "out of range.");
  dst_prim->set_new_height(size[0]);
  dst_prim->set_new_width(size[1]);
  if (src_prim->GetAttr(ops::kAlignCorners) != nullptr && GetValue<bool>(src_prim->GetAttr(ops::kAlignCorners))) {
    dst_prim->set_coordinate_transform_mode(mindspore::ALIGN_CORNERS);
  }
  if (src_prim->name() == kNameResizeBilinear) {
    dst_prim->set_method(ResizeMethod::LINEAR);
  } else if (src_prim->name() == kNameResizeNearestNeighbor) {
    dst_prim->set_method(ResizeMethod::NEAREST);
  }
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrSlice(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<ops::SliceFusion>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");
  auto begin = GetValueNode<ValuePtr>(cnode->input(opt::kInputIndexTwo));
  auto begin_value = GetValue<std::vector<int64_t>>(begin);

  std::vector<int64_t> axes(begin_value.size());
  for (size_t i = 0; i < begin_value.size(); i++) {
    axes[i] = static_cast<int64_t>(i);
  }
  dst_prim->set_axes(axes);
  dst_prim->SetAttrs(src_prim->attrs());
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}

int MoveAttrMapResizeGrad(const CNodePtr &cnode) {
  MS_ASSERT(cnode != nullptr);
  auto value_node = cnode->input(0)->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto src_prim = GetValueNode<PrimitivePtr>(value_node);
  if (src_prim == nullptr) {
    MS_LOG(ERROR) << "value node is invalid.";
    return lite::RET_ERROR;
  }
  auto dst_prim = std::make_shared<ops::ResizeGrad>();
  MS_CHECK_TRUE_MSG(dst_prim != nullptr, RET_NULL_PTR, "dst_prim is nullptr.");

  if (src_prim->name() == kNameResizeBilinearGrad) {
    dst_prim->set_method(ResizeMethod::LINEAR);
  } else if (src_prim->name() == kNameResizeNearestNeighborGrad) {
    dst_prim->set_method(ResizeMethod::NEAREST);
  } else {
    MS_LOG(ERROR) << "Resize grad method " << src_prim->name() << "is not supported";
    return lite::RET_ERROR;
  }
  MS_CHECK_TRUE_MSG(src_prim->GetAttr(ops::kAlignCorners) != nullptr, RET_NULL_PTR,
                    "src_prim->GetAttr(ops::kAlignCorners) is nullptr.");
  auto align_corners = GetValue<bool>(src_prim->GetAttr(ops::kAlignCorners));
  dst_prim->set_align_corners(align_corners);
  value_node->set_value(dst_prim);
  return lite::RET_OK;
}
}  // namespace

bool PrimitiveAdjust::Run(const FuncGraphPtr &func_graphs) {
  MS_ASSERT(func_graphs != nullptr);
  if (this->fmk_type_ != converter::kFmkTypeMs) {
    MS_LOG(INFO) << "The framework type of model should be mindir.";
    return true;
  }
  MS_ASSERT(graph != nullptr);
  static auto root_func_manager = Manage(func_graphs);
  std::set<FuncGraphPtr> all_func_graphs = {};
  lite::GetAllFuncGraph(func_graphs, &all_func_graphs);
  int i = 0;
  for (auto func_graph : all_func_graphs) {
    func_graph->set_manager(root_func_manager);
    auto make_int_ptr = MakeValue(static_cast<int>(FmkType::kFmkTypeMs));
    MS_CHECK_TRUE_MSG(make_int_ptr != nullptr, false, "make_int_ptr is nullptr.");
    func_graph->set_attr("fmk", make_int_ptr);
    if (i == 0) {
      auto make_value_ptr = MakeValue("main_graph");
      MS_CHECK_TRUE_MSG(make_value_ptr != nullptr, false, "make_value_ptr is nullptr.");
      func_graph->set_attr("graph_name", make_value_ptr);
    } else {
      auto make_value_ptr = MakeValue("subgraph" + std::to_string(i));
      MS_CHECK_TRUE_MSG(make_value_ptr != nullptr, false, "make_value_ptr is nullptr.");
      func_graph->set_attr("graph_name", make_value_ptr);
    }
    i++;
    auto node_list = TopoSort(func_graph->get_return());
    int status = lite::RET_OK;
    for (auto &node : node_list) {
      if (!utils::isa<CNodePtr>(node)) {
        continue;
      }
      auto cnode = node->cast<CNodePtr>();
      MS_ASSERT(cnode->size() > 0);
      if (cnode->input(0)->cast<CNodePtr>() != nullptr) {
        continue;
      }
      auto prim = GetValueNode<PrimitivePtr>(cnode->input(0));
      if (prim == nullptr) {
        MS_LOG(DEBUG) << "this is a call cnode, which input[0] is fg.";
        continue;
      }
      auto name = prim->name();
      auto adjust_func = PrimitiveAdjustRegistry::GetInstance()->GetPrimitiveCreator(name);
      if (adjust_func == nullptr) {
        MS_LOG(DEBUG) << "don't need to adjust.";
        continue;
      }
      status = adjust_func(cnode);
      if (status != lite::RET_OK) {
        MS_LOG(ERROR) << "convert primitive failed.";
        return false;
      }
    }
  }
  return true;
}

REGIST_PRIMITIVE_ADJUST(kNameAdd, MoveAttrMapCommon<ops::AddFusion>)
REGIST_PRIMITIVE_ADJUST(kNameAdder, MoveAttrMapAdder)
REGIST_PRIMITIVE_ADJUST(kNameArgMax, MoveAttrMapCommon<ops::ArgMaxFusion>)
REGIST_PRIMITIVE_ADJUST(kNameArgMaxWithValue, MoveAttrMapCommon<ops::ArgMaxFusion>)
REGIST_PRIMITIVE_ADJUST(kNameArgMin, MoveAttrMapCommon<ops::ArgMinFusion>)
REGIST_PRIMITIVE_ADJUST(kNameArgMinWithValue, MoveAttrMapCommon<ops::ArgMinFusion>)
REGIST_PRIMITIVE_ADJUST(kNameAvgPool, MoveAttrPool)
REGIST_PRIMITIVE_ADJUST(kNameAvgPoolGrad, MoveAttrPoolGrad)
REGIST_PRIMITIVE_ADJUST(kNameAvgPoolGradGpu, MoveAttrPoolGrad)
REGIST_PRIMITIVE_ADJUST(kNameAvgPoolGradCpu, MoveAttrPoolGrad)
REGIST_PRIMITIVE_ADJUST(kNameBatchMatMul, MoveAttrMapCommon<ops::MatMul>)
REGIST_PRIMITIVE_ADJUST(kNameBatchNorm, MoveAttrMapCommon<ops::FusedBatchNorm>)
REGIST_PRIMITIVE_ADJUST(kNameConv2DBackpropFilter, MoveAttrMapCommon<ops::Conv2DBackpropFilterFusion>)
REGIST_PRIMITIVE_ADJUST(kNameConv2DBackpropInput, MoveAttrMapCommon<ops::Conv2DBackpropInputFusion>)
REGIST_PRIMITIVE_ADJUST(kNameConv2D, MoveAttrMapConv2D)
REGIST_PRIMITIVE_ADJUST(kNameConv2DTranspose, MoveAttrMapConv2D)
REGIST_PRIMITIVE_ADJUST(kNameDiv, MoveAttrMapCommon<ops::DivFusion>)
REGIST_PRIMITIVE_ADJUST(kNameElu, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameEluGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameExp, MoveAttrMapCommon<ops::ExpFusion>)
REGIST_PRIMITIVE_ADJUST(kNameFusedBatchNormEx, MoveAttrMapCommon<ops::FusedBatchNorm>)
REGIST_PRIMITIVE_ADJUST(kNameFusedBatchNormGradEx, MoveAttrMapCommon<ops::BatchNormGrad>)
REGIST_PRIMITIVE_ADJUST(kNameFusedBatchNormGradCPU, MoveAttrMapCommon<ops::BatchNormGrad>)
REGIST_PRIMITIVE_ADJUST(kNameGeLU, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameGeLUGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameHSigmoid, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameHSigmoidGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameHSwish, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameHSwishGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameL2Normalize, MoveAttrMapCommon<ops::L2NormalizeFusion>)
REGIST_PRIMITIVE_ADJUST(kNameLayerNorm, MoveAttrMapLayerNorm)
REGIST_PRIMITIVE_ADJUST(kNameLeakyRelu, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameMaxPool, MoveAttrPool)
REGIST_PRIMITIVE_ADJUST(kNameMaxPoolGrad, MoveAttrPoolGrad)
REGIST_PRIMITIVE_ADJUST(kNameMul, MoveAttrMapCommon<ops::MulFusion>)
REGIST_PRIMITIVE_ADJUST(kNamePad, MoveAttrMapCommon<ops::PadFusion>)
REGIST_PRIMITIVE_ADJUST(kNamePartial, MoveAttrMapCommon<ops::PartialFusion>)
REGIST_PRIMITIVE_ADJUST(kNamePow, MoveAttrMapCommon<ops::PowFusion>)
REGIST_PRIMITIVE_ADJUST(kNamePReLU, MoveAttrMapCommon<ops::PReLUFusion>)
REGIST_PRIMITIVE_ADJUST(kNameReduceAll, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceASum, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceMax, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceMean, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceMin, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceProd, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceSum, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReduceSumSquare, MoveAttrMapReduce)
REGIST_PRIMITIVE_ADJUST(kNameReLU, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameReluGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameReLU6, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameReLU6Grad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameTanhGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameResizeBilinear, MoveAttrMapResize)
REGIST_PRIMITIVE_ADJUST(kNameResizeNearestNeighbor, MoveAttrMapResize)
REGIST_PRIMITIVE_ADJUST(kNameScale, MoveAttrMapCommon<ops::ScaleFusion>)
REGIST_PRIMITIVE_ADJUST(kNameSigmoid, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameSigmoidGrad, MoveAttrMapActivationGrad)
REGIST_PRIMITIVE_ADJUST(kNameSlice, MoveAttrSlice)
REGIST_PRIMITIVE_ADJUST(kNameSub, MoveAttrMapCommon<ops::SubFusion>)
REGIST_PRIMITIVE_ADJUST(kNameTanh, MoveAttrMapActivation)
REGIST_PRIMITIVE_ADJUST(kNameTile, MoveAttrMapCommon<ops::TileFusion>)
REGIST_PRIMITIVE_ADJUST(kNameTopK, MoveAttrMapCommon<ops::TopKFusion>)
REGIST_PRIMITIVE_ADJUST(kNameSparseSoftmaxCrossEntropyWithLogits,
                        MoveAttrMapCommon<ops::SparseSoftmaxCrossEntropyWithLogits>)
REGIST_PRIMITIVE_ADJUST(kNameResizeBilinearGrad, MoveAttrMapResizeGrad)
REGIST_PRIMITIVE_ADJUST(kNameResizeNearestNeighborGrad, MoveAttrMapResizeGrad)
}  // namespace lite
}  // namespace mindspore
