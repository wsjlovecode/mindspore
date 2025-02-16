/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include <map>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include "utils/hash_map.h"
#include "ir/tensor.h"
#include "ir/param_info.h"
#include "ir/func_graph.h"
#include "base/core_ops.h"
#include "proto/mind_ir.pb.h"
#include "utils/check_convert_utils.h"
#include "debug/dump_proto.h"
#include "utils/ms_utils.h"
#include "utils/utils.h"
#include "frontend/parallel/tensor_layout/tensor_layout.h"

namespace mindspore {
using FloatPtr = std::shared_ptr<Float>;
using IntPtr = std::shared_ptr<Int>;
using UIntPtr = std::shared_ptr<UInt>;
// anf type to mindir type map
static mindspore::HashMap<int, mind_ir::TensorProto_DataType> g_data_type_map = {
  {kNumberTypeBool, mind_ir::TensorProto_DataType_BOOL},
  {kNumberTypeInt8, mind_ir::TensorProto_DataType_INT8},
  {kNumberTypeInt16, mind_ir::TensorProto_DataType_INT16},
  {kNumberTypeInt32, mind_ir::TensorProto_DataType_INT32},
  {kNumberTypeInt64, mind_ir::TensorProto_DataType_INT64},
  {kNumberTypeUInt8, mind_ir::TensorProto_DataType_UINT8},
  {kNumberTypeUInt16, mind_ir::TensorProto_DataType_UINT16},
  {kNumberTypeUInt32, mind_ir::TensorProto_DataType_UINT32},
  {kNumberTypeUInt64, mind_ir::TensorProto_DataType_UINT64},
  {kNumberTypeFloat16, mind_ir::TensorProto_DataType_FLOAT16},
  {kNumberTypeFloat32, mind_ir::TensorProto_DataType_FLOAT},
  {kNumberTypeFloat64, mind_ir::TensorProto_DataType_DOUBLE},
  {kObjectTypeString, mind_ir::TensorProto_DataType_STRING},
};

static mindspore::HashMap<int, mind_ir::TensorProto_DataType> g_data_bits_int_map = {
  {8, mind_ir::TensorProto_DataType_INT8},
  {16, mind_ir::TensorProto_DataType_INT16},
  {32, mind_ir::TensorProto_DataType_INT32},
  {64, mind_ir::TensorProto_DataType_INT64},
};

static mindspore::HashMap<int, mind_ir::TensorProto_DataType> g_data_bits_uint_map = {
  {8, mind_ir::TensorProto_DataType_UINT8},
  {16, mind_ir::TensorProto_DataType_UINT16},
  {32, mind_ir::TensorProto_DataType_UINT32},
  {64, mind_ir::TensorProto_DataType_UINT64},
};

static mindspore::HashMap<int, mind_ir::TensorProto_DataType> g_data_bits_float_map = {
  {16, mind_ir::TensorProto_DataType_FLOAT16},
  {32, mind_ir::TensorProto_DataType_FLOAT},
  {64, mind_ir::TensorProto_DataType_FLOAT64},
};

static std::set<std::string> g_export_attr_blacklist = {kAttrDump};

// Can build different builder according to format
class IrExportBuilder;
using IrExportBuilderPtr = std::shared_ptr<IrExportBuilder>;

class IrExporter {
 public:
  explicit IrExporter(IrExportBuilderPtr builder) : builder_(std::move(builder)) {}
  virtual ~IrExporter() = default;
  std::string GetDumpString(const FuncGraphPtr &func_graph);
  ModelProtoPtr GetDumpProto(const FuncGraphPtr &func_graph, const FuncGraphPtr &param_layout_fg = nullptr);

 private:
  IrExportBuilderPtr builder_;
};

class IrExportBuilder {
 public:
  IrExportBuilder() : model_(std::make_shared<mind_ir::ModelProto>()) {}
  ~IrExportBuilder() { google::protobuf::ShutdownProtobufLibrary(); }
  std::string GetProtoString() const;
  void BuildModelInfo();
  bool BuildModel(const FuncGraphPtr &func_graph);
  ModelProtoPtr Model() { return model_; }

  void BuildLayout(const FuncGraphPtr &func_graph);

  bool BuildFuncGraph(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto);
  bool BuildFuncGraphAttrs(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto);
  bool BuildParameters(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto);
  bool BuildNodes(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto);
  bool BuildOutput(const CNodePtr &node, mind_ir::GraphProto *const graph_proto);
  bool BuildCNode(const CNodePtr &node, mind_ir::GraphProto *const graph_proto);
  std::string BuildInputNode(const AnfNodePtr &node, mind_ir::GraphProto *const graph_proto);

  bool SetValueInfoProto(const AnfNodePtr &node, mind_ir::ValueInfoProto *const value_proto);
  bool SetParamToTensorProto(const ParameterPtr &param, mind_ir::TensorProto *const tensor_proto);
  bool SetTensorProto(const TypePtr &type, const BaseShapePtr &shape, mind_ir::TensorProto *const tensor_proto);
  bool SetTensorProtoForRef(const TypePtr &type, const AbstractBasePtr &abs, mind_ir::TensorProto *const tensor_proto);
  bool SetAttributeProto(const AnfNodePtr &node, mind_ir::NodeProto *const node_proto);
  bool SetShapeToNodeProto(const CNodePtr &node, mind_ir::NodeProto *const node_proto);
  bool SetShapeToNodeProto(const TypePtr &type, const BaseShapePtr &shape, const abstract::AbstractBasePtr &abstract,
                           mind_ir::AttributeProto *const attr_proto, std::string *const seq_string);
  bool SetValueToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetTypeToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetScalarToAttributeProto_ir(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetScalarToAttributeProtoForInt_ir(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetScalarToAttributeProto_irs(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetScalarToAttributeProtoForInt_irs(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetTypeToAttributeProto_irs(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetTensorToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto);
  bool SetSequenceToAttributeProto(const ValueSequencePtr &value, mind_ir::AttributeProto *const attr_proto,
                                   std::string *const seq_string);
  bool SetSeqElemToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto,
                                  std::string *const seq_string);

  mind_ir::TensorProto_DataType GetMindirDataType(TypeId type_id);
  mind_ir::TensorProto_DataType GetMindirDataBitsIntType(int bits);
  mind_ir::TensorProto_DataType GetMindirDataBitsFloatType(int bits);
  mind_ir::TensorProto_DataType GetMindirDataBitsUIntType(int bits);
  std::string GetNodeName(const AnfNodePtr &node);
  std::string GetUniqueNodeName(const AnfNodePtr &node);
  std::string GetOpTypeName(const AnfNodePtr &node);
  size_t GetNodeIndex() { return ++node_index_; }
  void ResetNodeIndex() { node_index_ = 0; }
  size_t GetTupleIndex() { return ++shape_index_; }
  void ResetTupleIndex() { shape_index_ = 0; }

 private:
  ModelProtoPtr model_;
  mind_ir::NodeProto *last_node_{nullptr};
  std::list<FuncGraphPtr> todo_;
  std::map<AnfNodePtr, std::string> node_index_map_;
  std::set<std::string> nodeName_;
  size_t node_index_{0};
  size_t shape_index_{0};

  bool top_graph{true};
};

using IrExporterPtr = std::shared_ptr<IrExporter>;

std::string IrExporter::GetDumpString(const FuncGraphPtr &func_graph) {
  auto dump_proto = GetDumpProto(func_graph);
  if (dump_proto == nullptr) {
    MS_LOG(EXCEPTION) << "Get dump proto for graph " << func_graph->ToString() << " failed.";
  }
  return builder_->GetProtoString();
}

ModelProtoPtr IrExporter::GetDumpProto(const FuncGraphPtr &func_graph, const FuncGraphPtr &param_layout_fg) {
  if ((builder_ == nullptr) || (func_graph == nullptr)) {
    MS_LOG(EXCEPTION) << "Input params is null.";
  }

  // Export model info
  builder_->BuildModelInfo();

  // Export model and return string
  if (!builder_->BuildModel(func_graph)) {
    return nullptr;
  }

  // Export layout information
  if (param_layout_fg) {
    builder_->BuildLayout(param_layout_fg);
  }
  return builder_->Model();
}

std::string IrExportBuilder::GetProtoString() const {
  MS_LOG(DEBUG) << "BuildModel complete!";
  return model_->SerializeAsString();
}

void IrExportBuilder::BuildModelInfo() {
  constexpr auto ir_version = "0.1.0";
  constexpr auto mindspore_name = "MindSpore";
  model_->set_ir_version(ir_version);
  model_->set_producer_name(mindspore_name);
  model_->set_model_version(VERSION);
  model_->set_little_endian(common::IsLittleByteOrder());
}

void IrExportBuilder::BuildLayout(const FuncGraphPtr &func_graph) {
  MS_EXCEPTION_IF_NULL(func_graph);
  std::vector<AnfNodePtr> graph_params = func_graph->parameters();
  mind_ir::ParallelProto *parallel_proto = model_->mutable_parallel();
  for (auto para : graph_params) {
    std::string name = std::static_pointer_cast<Parameter>(para)->name();
    auto tensor_layout = para->user_data<parallel::TensorLayout>();
    if (tensor_layout == nullptr) {
      MS_LOG(INFO) << "GetParameterLayout nullptr name = " << name;
    } else {
      mind_ir::LayoutProto *layoutProto = parallel_proto->add_layout();

      // Get all the information for layput
      auto device_arrangement = tensor_layout->device_arrangement().array();
      auto tensor_map = tensor_layout->tensor_map().array();
      auto slice_shape = tensor_layout->slice_shape().array();
      int64_t field_size = tensor_layout->get_field_size();
      bool uniform_split = tensor_layout->uniform_split();
      std::string opt_shard_group = tensor_layout->opt_shard_group();

      // Save all information to Layout Proto
      layoutProto->set_name(name);
      for (auto device_arrangement_element : device_arrangement) {
        layoutProto->add_device_arrangement_int(device_arrangement_element);
      }
      for (auto tensor_map_element : tensor_map) {
        layoutProto->add_tensor_map_int(tensor_map_element);
      }
      for (auto slice_shape_element : slice_shape) {
        layoutProto->add_slice_shape_int(slice_shape_element);
      }
      layoutProto->set_field_size(field_size);
      layoutProto->set_uniform_split(uniform_split);
      layoutProto->set_opt_shard_group(opt_shard_group);
    }
  }
}

bool IrExportBuilder::BuildModel(const FuncGraphPtr &func_graph) {
  MS_EXCEPTION_IF_NULL(func_graph);
  mind_ir::GraphProto *graph_proto = model_->mutable_graph();
  graph_proto->set_name(func_graph->ToString());
  graph_proto->set_bprop_hash(func_graph->bprop_hash());
  ResetNodeIndex();
  todo_.clear();
  nodeName_.clear();
  // Build the main funcGraph
  (void)nodeName_.insert(func_graph->ToString());
  top_graph = true;
  if (!BuildFuncGraph(func_graph, graph_proto)) {
    MS_LOG(ERROR) << "Build func_graph " << func_graph->ToString() << " failed.";
    return false;
  }

  std::set<FuncGraphPtr> graphVisited;
  (void)graphVisited.insert(func_graph);
  top_graph = false;
  while (!todo_.empty()) {
    FuncGraphPtr fg = todo_.back();
    todo_.pop_back();
    if (graphVisited.count(fg) > 0) {
      continue;
    }
    if (nodeName_.count(fg->ToString()) > 0) {
      MS_LOG(ERROR) << "There is a duplicate name: " << fg->ToString();
      return false;
    }
    (void)nodeName_.insert(fg->ToString());
    (void)graphVisited.insert(fg);
    auto graph = model_->add_functions();
    if (!BuildFuncGraph(fg, graph)) {
      MS_LOG(ERROR) << "Build func_graph " << fg->ToString() << " failed.";
      return false;
    }
  }
  // Release resource
  nodeName_.clear();
  node_index_map_.clear();
  return true;
}

bool IrExportBuilder::BuildFuncGraph(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto) {
  // Export funcGraph name.
  graph_proto->set_name(func_graph->ToString());
  // Export parameters
  // 1. parameters should be mapped to ValueInfoProto
  // 2. parameters with default value should be mapped to Initializer
  if (!BuildParameters(func_graph, graph_proto)) {
    MS_LOG(ERROR) << "Build parameters failed.";
    return false;
  }

  // Export graph attributes
  if (!BuildFuncGraphAttrs(func_graph, graph_proto)) {
    MS_LOG(ERROR) << "Build attributes for graph failed.";
    return false;
  }

  // Export operator nodes(include output)
  return BuildNodes(func_graph, graph_proto);
}

bool IrExportBuilder::BuildFuncGraphAttrs(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto) {
  MS_EXCEPTION_IF_NULL(func_graph);
  MS_EXCEPTION_IF_NULL(graph_proto);
  for (auto attr : func_graph->attrs()) {
    MS_LOG(DEBUG) << "attr: " << attr.first << " " << attr.second->DumpText() << " " << attr.second->type_name();
    mind_ir::AttributeProto *attr_proto = graph_proto->add_attribute();
    attr_proto->set_name(attr.first);
    if (!SetValueToAttributeProto(attr.second, attr_proto)) {
      MS_LOG(ERROR) << "Set value to AttributeProto for GraphProto failed.";
      return false;
    }
  }
  return true;
}

bool IrExportBuilder::BuildParameters(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto) {
  MS_EXCEPTION_IF_NULL(func_graph);
  MS_EXCEPTION_IF_NULL(graph_proto);
  for (auto &item : func_graph->parameters()) {
    MS_EXCEPTION_IF_NULL(item);
    auto param = item->cast<ParameterPtr>();
    if (param == nullptr) {
      MS_LOG(ERROR) << "Parameter: '" << item->ToString() << "' could not cast to parameter.";
      return false;
    }
    std::string param_name = GetUniqueNodeName(param);
    if (top_graph && param->has_default()) {
      MS_LOG(DEBUG) << "Parameter: '" << item->DebugString();
      mind_ir::TensorProto *parameter_proto = graph_proto->add_parameter();
      parameter_proto->set_name(param_name);
      if (!SetParamToTensorProto(param, parameter_proto)) {
        MS_LOG(ERROR) << "Set parameter " << param->DebugString() << " to TensorProto failed.";
        return false;
      }
    } else {
      mind_ir::ValueInfoProto *input_proto = graph_proto->add_input();
      input_proto->set_name(param_name);
      if (!SetValueInfoProto(param, input_proto)) {
        MS_LOG(ERROR) << "Set parameter " << param->DebugString() << " to TensorProto failed.";
        return false;
      }
    }
    if (nodeName_.count(param_name) > 0) {
      MS_LOG(ERROR) << "parameter name is duplicate:" << param_name;
      return false;
    }
    (void)nodeName_.insert(param_name);
  }
  return true;
}

mind_ir::TensorProto_DataType IrExportBuilder::GetMindirDataType(TypeId type_id) {
  auto iter = g_data_type_map.find(type_id);
  if (iter == g_data_type_map.end()) {
    MS_LOG(ERROR) << "Convert type error, unsupported type! " << type_id;
    return mind_ir::TensorProto_DataType_UNDEFINED;
  }
  return iter->second;
}

mind_ir::TensorProto_DataType IrExportBuilder::GetMindirDataBitsIntType(int bits) {
  auto iter = g_data_bits_int_map.find(bits);
  if (iter == g_data_bits_int_map.end()) {
    MS_LOG(ERROR) << "Convert bits int error, unsupported bits! " << bits;
    return mind_ir::TensorProto_DataType_UNDEFINED;
  }
  return iter->second;
}

mind_ir::TensorProto_DataType IrExportBuilder::GetMindirDataBitsUIntType(int bits) {
  auto iter = g_data_bits_uint_map.find(bits);
  if (iter == g_data_bits_uint_map.end()) {
    MS_LOG(ERROR) << "Convert bits uint error, unsupported bits! " << bits;
    return mind_ir::TensorProto_DataType_UNDEFINED;
  }
  return iter->second;
}

mind_ir::TensorProto_DataType IrExportBuilder::GetMindirDataBitsFloatType(int bits) {
  auto iter = g_data_bits_float_map.find(bits);
  if (iter == g_data_bits_float_map.end()) {
    MS_LOG(ERROR) << "Convert bits float error, unsupported bits! " << bits;
    return mind_ir::TensorProto_DataType_UNDEFINED;
  }
  return iter->second;
}

bool IrExportBuilder::SetValueInfoProto(const AnfNodePtr &node, mind_ir::ValueInfoProto *const value_proto) {
  if (node == nullptr || value_proto == nullptr) {
    MS_LOG(EXCEPTION) << "AnfNode or ValueInfo is null!";
  }
  MS_LOG(DEBUG) << "SetValueInfoProto: " << node->DebugString();
  const TypePtr &type = node->Type();
  const BaseShapePtr &shape = node->Shape();
  // For the bprop fg which has not been renormalized.
  if (type == nullptr || shape == nullptr) {
    return true;
  }
  if (type->isa<TensorType>() && shape->isa<abstract::Shape>()) {
    auto tensor = type->cast<TensorTypePtr>();
    MS_EXCEPTION_IF_NULL(tensor);
    auto elem_type = tensor->element();
    const auto &dims = shape->cast<abstract::ShapePtr>()->shape();
    mind_ir::TensorProto *tensor_proto = value_proto->add_tensor();
    auto data_type = GetMindirDataType(elem_type->type_id());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
    if (dims.size() == 0) {
      MS_LOG(DEBUG) << "The dim of ValueInfoProto is 0.";
    } else {
      for (const auto &dim : dims) {
        MS_LOG(DEBUG) << "SetValueInfoProto dim: " << dim;
        tensor_proto->add_dims(dim);
      }
    }
    if (!SetTensorProtoForRef(type, node->abstract(), tensor_proto)) {
      return false;
    }
  } else if (type->isa<Tuple>()) {
    auto tup_shape = shape->cast<abstract::TupleShapePtr>();
    value_proto->set_denotation(type->type_name() + ":" + std::to_string(tup_shape->shape().size()));
  } else {
    value_proto->set_denotation(type->type_name());
  }
  MS_LOG(DEBUG) << "Value type: " << type->type_name();
  return true;
}

bool IrExportBuilder::SetTensorToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto) {
  if (value == nullptr || attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "ValuePtr or AttributeProto is null!";
  }
  attr_proto->set_ref_attr_name("tensor:value0");
  attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
  mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
  tensor_proto->set_name("value0");
  auto data = value->cast<tensor::TensorPtr>();
  MS_EXCEPTION_IF_NULL(data);
  tensor_proto->set_raw_data(data->data_c(), static_cast<size_t>(data->data().nbytes()));
  auto dtype = data->data_type();
  auto shape = data->shape_c();
  auto data_type = GetMindirDataType(dtype);
  if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
    return false;
  }
  tensor_proto->set_data_type(data_type);
  for (const auto &dim : shape) {
    tensor_proto->add_dims(dim);
  }
  return true;
}

bool IrExportBuilder::SetTensorProto(const TypePtr &type, const BaseShapePtr &shape,
                                     mind_ir::TensorProto *const tensor_proto) {
  if (!type->isa<TensorType>() || !shape->isa<abstract::Shape>()) {
    MS_LOG(ERROR) << "Type or shape is not supported! " << type->ToString();
    return false;
  }
  auto tensor = type->cast<TensorTypePtr>();
  const auto &dims = shape->cast<abstract::ShapePtr>()->shape();
  auto data_type = GetMindirDataType(tensor->element()->type_id());
  if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
    return false;
  }
  tensor_proto->set_data_type(data_type);
  for (const auto &dim : dims) {
    tensor_proto->add_dims(dim);
  }
  return true;
}

bool IrExportBuilder::SetTensorProtoForRef(const TypePtr &type, const AbstractBasePtr &abs,
                                           mind_ir::TensorProto *const tensor_proto) {
  if (!type->isa<RefType>()) {
    return true;
  }
  auto abs_ref = abs->cast<abstract::AbstractRefPtr>();
  if (abs_ref == nullptr) {
    MS_LOG(ERROR) << "The abstract " << abs->ToString() << " should be AbstractRef.";
    return false;
  }
  auto ref_key_value = abs_ref->ref_key_value();
  if (ref_key_value == nullptr) {
    MS_LOG(INFO) << "The ref_key_value of abstract ref " << abs->ToString() << " is nullptr";
    return true;
  }
  tensor_proto->set_ref_key(ref_key_value->name());
  return true;
}

bool IrExportBuilder::SetParamToTensorProto(const ParameterPtr &param, mind_ir::TensorProto *const tensor_proto) {
  if (param == nullptr || tensor_proto == nullptr) {
    MS_LOG(EXCEPTION) << "Parameter or TensorProto is null!";
  }
  MS_LOG(DEBUG) << "SetParamToTensorProto: " << param->DebugString();
  return SetTensorProto(param->Type(), param->Shape(), tensor_proto);
}

bool IrExportBuilder::BuildNodes(const FuncGraphPtr &func_graph, mind_ir::GraphProto *const graph_proto) {
  std::vector<AnfNodePtr> nodes = TopoSort(func_graph->get_return(), SuccIncoming, AlwaysInclude);
  for (const AnfNodePtr &node : nodes) {
    MS_EXCEPTION_IF_NULL(node);
    if (!node->isa<CNode>()) {
      MS_LOG(DEBUG) << "Node: '" << node->ToString() << "' is not cnode";
      continue;
    }
    auto cnode = node->cast<CNodePtr>();
    if (cnode == func_graph->get_return()) {
      if (!BuildOutput(cnode, graph_proto)) {
        MS_LOG(ERROR) << "Build output for graph " << func_graph->ToString() << " failed.";
        return false;
      }
    } else {
      if (!BuildCNode(cnode, graph_proto)) {
        MS_LOG(ERROR) << "Build proto for cnode " << cnode->DebugString() << " failed.";
        return false;
      }
    }
  }
  return true;
}

bool IrExportBuilder::BuildOutput(const CNodePtr &node, mind_ir::GraphProto *const graph_proto) {
  MS_EXCEPTION_IF_NULL(node);
  const int OutputSize = 2;
  if (node->size() != OutputSize) {
    MS_LOG(ERROR) << "Number of inputs of return node is not equal to 2.";
    return false;
  }
  AnfNodePtr arg = node->input(1);
  std::string node_name = BuildInputNode(arg, graph_proto);
  if (node_name.empty()) {
    MS_LOG(ERROR) << "Build input node failed for arg " << arg->DebugString();
    return false;
  }
  mind_ir::ValueInfoProto *output_proto = graph_proto->add_output();
  output_proto->set_name(node_name);
  return SetValueInfoProto(arg, output_proto);
}

std::string IrExportBuilder::GetOpTypeName(const AnfNodePtr &node) {
  // May be ValueNode/CNode/Parameter
  std::string type_name = "";
  if (IsValueNode<Primitive>(node)) {
    PrimitivePtr prim = GetValueNode<PrimitivePtr>(node);
    MS_EXCEPTION_IF_NULL(prim);
    type_name = prim->ToString();
  } else if (IsValueNode<FuncGraph>(node)) {
    FuncGraphPtr fg = GetValueNode<FuncGraphPtr>(node);
    MS_EXCEPTION_IF_NULL(fg);
    todo_.push_back(fg);
    type_name = "REF::" + fg->ToString();
  } else if (node->isa<CNode>() || node->isa<Parameter>()) {
    auto nodeName = GetUniqueNodeName(node);
    type_name = "REF::" + nodeName;
    if (nodeName_.count(nodeName) == 0) {
      MS_LOG(ERROR) << "There is not the name: " << nodeName;
      return "";
    }
  } else {
    MS_LOG(ERROR) << "Need to support op type: " << node->type_name();
    return "";
  }
  MS_LOG(DEBUG) << "ExportType: " << type_name;
  return type_name;
}

bool IrExportBuilder::SetShapeToNodeProto(const TypePtr &type, const BaseShapePtr &shape, const AbstractBasePtr &abs,
                                          mind_ir::AttributeProto *const attr_proto, std::string *const seq_string) {
  MS_EXCEPTION_IF_NULL(type);
  MS_EXCEPTION_IF_NULL(shape);
  MS_EXCEPTION_IF_NULL(seq_string);
  if (type->isa<Tuple>()) {
    *seq_string += "Tuple[";
    auto elements = type->cast<TuplePtr>()->elements();
    auto tuple_shape = shape->cast<abstract::TupleShapePtr>()->shape();
    auto tuple_abs = abs->cast<abstract::AbstractTuplePtr>()->elements();
    for (size_t i = 0; i < elements.size(); i++) {
      if (!SetShapeToNodeProto(elements[i], tuple_shape[i], tuple_abs[i], attr_proto, seq_string)) {
        return false;
      }
    }
    *seq_string += "],";
  } else if (type->isa<TensorType>() && shape->isa<abstract::Shape>()) {
    string shape_name = "shape" + std::to_string(GetTupleIndex());
    *seq_string += shape_name + ",";
    mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
    tensor_proto->set_name(shape_name);
    return SetTensorProto(type, shape, tensor_proto) && SetTensorProtoForRef(type, abs, tensor_proto);
  } else if (type->isa<Number>()) {
    if (type->isa<Bool>()) {
      attr_proto->set_type(mind_ir::AttributeProto_AttributeType_BOOL);
    } else {
      string shape_name = "shape" + std::to_string(GetTupleIndex());
      *seq_string += shape_name + ",";
      mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
      tensor_proto->set_name(shape_name);
      tensor_proto->set_data_type(mind_ir::TensorProto_DataType_UINT64);
      tensor_proto->add_dims(1);
    }
  } else if (type->isa<Function>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_GRAPH);
    *seq_string += type->type_name() + ",";
  } else if (type->isa<String>() || type->isa<UMonadType>() || type->isa<IOMonadType>()) {
    *seq_string += type->type_name() + ",";
  } else {
    MS_LOG(ERROR) << "Type of cnode need to be supported: " << type->type_name();
    return false;
  }
  return true;
}

bool IrExportBuilder::SetShapeToNodeProto(const CNodePtr &node, mind_ir::NodeProto *const node_proto) {
  // Get shape of cnode
  // 1. need to get shape from tuple element
  // 2. save shape in TensorProto
  // 3. save tuple string in ref_attr_name
  MS_EXCEPTION_IF_NULL(node);
  auto type = node->Type();
  auto shape = node->Shape();
  auto abs = node->abstract();
  // For the bprop fg which has not been renormalized.
  if (type == nullptr || shape == nullptr) {
    return true;
  }
  ResetTupleIndex();
  std::string seq_string = "shape:";
  mind_ir::AttributeProto *attr_proto = node_proto->add_attribute();
  if (!SetShapeToNodeProto(type, shape, abs, attr_proto, &seq_string)) {
    MS_LOG(ERROR) << "Set shape to NodeProto for " << node->DebugString() << " failed.";
    return false;
  }
  attr_proto->set_ref_attr_name(seq_string);
  MS_LOG(DEBUG) << "CNode shape: " << seq_string;
  return true;
}

bool IrExportBuilder::BuildCNode(const CNodePtr &node, mind_ir::GraphProto *const graph_proto) {
  auto inputs_size = node->size();
  if (inputs_size < 1) {
    MS_LOG(ERROR) << "Inputs of node " << node->DebugString() << " is empty";
    return false;
  }

  // Need to build input node before dealing with cnode
  std::vector<string> input_names;
  for (size_t i = 1; i < inputs_size; i++) {
    auto input = node->input(i);
    std::string node_name = BuildInputNode(input, graph_proto);
    if (node_name.empty()) {
      MS_LOG(ERROR) << "Build input node for " << input->DebugString() << " failed.";
      return false;
    }
    input_names.push_back(node_name);
  }

  // Build cnode
  mind_ir::NodeProto *node_proto = graph_proto->add_node();
  std::string output_name = GetUniqueNodeName(node);
  if (nodeName_.count(output_name) > 0) {
    MS_LOG(EXCEPTION) << "There is a duplicate name: " << output_name;
  }
  (void)nodeName_.insert(output_name);
  node_proto->add_output(output_name);
  node_proto->set_name(output_name);
  node_proto->set_domain(node->fullname_with_scope());
  AnfNodePtr op = node->input(0);
  std::string type_name = GetOpTypeName(op);
  if (type_name.empty()) {
    MS_LOG(ERROR) << "Get op type name for " << op->DebugString() << " failed.";
    return false;
  }
  node_proto->set_op_type(type_name);
  last_node_ = node_proto;
  // Maybe Tensor or Function or nullptr
  if (!SetShapeToNodeProto(node, node_proto)) {
    return false;
  }

  (void)std::for_each(input_names.begin(), input_names.end(),
                      [&node_proto](const string &name) { node_proto->add_input(name); });

  // Add primitive attrs
  if (IsValueNode<Primitive>(op)) {
    auto prim = GetValueNode<PrimitivePtr>(op);
    if (prim->isa<prim::DoSignaturePrimitive>()) {
      auto func = prim->cast<prim::DoSignaturePrimitivePtr>()->function();
      if (func != nullptr && func->isa<Primitive>()) {
        prim = func->cast<PrimitivePtr>();
      }
    }
    for (const auto &attr : prim->attrs()) {
      MS_LOG(DEBUG) << "attr: " << attr.first << " " << attr.second->DumpText() << " " << attr.second->type_name();
      auto iter = g_export_attr_blacklist.find(attr.first);
      if (iter != g_export_attr_blacklist.end()) {
        continue;
      }
      mind_ir::AttributeProto *attr_proto = node_proto->add_attribute();
      attr_proto->set_name(attr.first);
      auto attr_value = attr.second;
      CheckAndConvertUtils::ConvertAttrValueInExport(type_name, attr.first, &attr_value);
      if (!SetValueToAttributeProto(attr_value, attr_proto)) {
        MS_LOG(ERROR) << "Set value to AttributeProto failed.";
        return false;
      }
    }
  }
  return true;
}

std::string IrExportBuilder::BuildInputNode(const AnfNodePtr &node, mind_ir::GraphProto *const graph_proto) {
  // Return the NodeName that the node has been processed.
  auto iter = node_index_map_.find(node);
  if (iter != node_index_map_.end()) {
    return iter->second;
  }

  std::string node_name = GetUniqueNodeName(node);
  // FuncGraph will be added to functions and the input name is the function name.
  if (IsValueNode<FuncGraph>(node)) {
    FuncGraphPtr fg = GetValueNode<FuncGraphPtr>(node);
    todo_.push_back(fg);
    return fg->ToString();
  }
  if (node->isa<ValueNode>()) {
    nodeName_.insert(node_name);
    // When node input is a ValueNode, need to create a Constant Node
    mind_ir::NodeProto *node_proto = graph_proto->add_node();
    node_proto->set_name(node_name);
    node_proto->add_output(node_name);
    if (!SetAttributeProto(node, node_proto)) {
      return "";
    }
  }
  return node_name;
}

std::string IrExportBuilder::GetUniqueNodeName(const AnfNodePtr &node) {
  // Naming anfnode
  // 1. parameter is unique in one func_graph
  // 2. cnode and valuenode may be reduplicative, so add index to identify.
  auto iter = node_index_map_.find(node);
  if (iter != node_index_map_.end()) {
    return iter->second;
  } else {
    std::string node_name = GetNodeName(node);
    // Compatible before. CNode = FuncGraphName:CNodeName:index ,Parameter = FuncGraphName:ParameterName
    if (node->isa<CNode>()) {
      node_name = node_name + ":" + std::to_string(GetNodeIndex());
    }
    // Avoid duplicate name.
    while (nodeName_.count(node_name) > 0) {
      node_name = node_name + "_" + std::to_string(GetNodeIndex());
    }
    node_index_map_[node] = node_name;
    return node_name;
  }
}

std::string IrExportBuilder::GetNodeName(const AnfNodePtr &node) {
  MS_EXCEPTION_IF_NULL(node);
  std::string node_name = "";
  if (node->func_graph() != nullptr) {
    node_name = node->func_graph()->ToString() + ":";
  }
  if (node->isa<ValueNode>()) {
    // Needn't value
    node_name += node->AnfNode::ToString();
  } else {
    node_name += node->ToString();
  }
  MS_LOG(DEBUG) << "GetNodeName: " << node_name;
  return node_name;
}

bool IrExportBuilder::SetAttributeProto(const AnfNodePtr &node, mind_ir::NodeProto *const node_proto) {
  if (node == nullptr || node_proto == nullptr) {
    MS_LOG(EXCEPTION) << "AnfNode or NodeProto is null!";
  }
  auto value_node = node->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_node);
  auto value = value_node->value();
  node_proto->set_op_type("Constant");
  mind_ir::AttributeProto *attr_proto = node_proto->add_attribute();
  attr_proto->set_name("value");
  MS_LOG(DEBUG) << "Set Constant attribute: " << value->ToString();
  return SetValueToAttributeProto(value, attr_proto);
}

bool IrExportBuilder::SetTypeToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto) {
  if (value == nullptr || attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "ValuePtr or AttributeProto is null!";
  }
  attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
  mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
  if (value->isa<Int>()) {
    attr_proto->set_ref_attr_name("type:value0");
    tensor_proto->set_name("value0");
    auto int_value = value->cast<IntPtr>();
    auto data_type = GetMindirDataBitsIntType(int_value->nbits());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
  } else if (value->isa<UInt>()) {
    attr_proto->set_ref_attr_name("type:value0");
    tensor_proto->set_name("value0");
    auto float_value = value->cast<UIntPtr>();
    auto data_type = GetMindirDataBitsUIntType(float_value->nbits());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
  } else if (value->isa<Float>()) {
    attr_proto->set_ref_attr_name("type:value0");
    tensor_proto->set_name("value0");
    auto float_value = value->cast<FloatPtr>();
    auto data_type = GetMindirDataBitsFloatType(float_value->nbits());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
  } else if (value->isa<Bool>()) {
    attr_proto->set_ref_attr_name("type:value0");
    tensor_proto->set_name("value0");
    tensor_proto->set_data_type(mind_ir::TensorProto_DataType_BOOL);
  } else if (value->isa<TensorType>()) {
    attr_proto->set_ref_attr_name("type:tensor0");
    tensor_proto->set_name("tensor0");
    auto elem_type = value->cast<TensorTypePtr>()->element();
    if (elem_type->isa<Int>()) {
      auto int_value = elem_type->cast<IntPtr>();
      auto data_type = GetMindirDataBitsIntType(int_value->nbits());
      if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
        return false;
      }
      tensor_proto->set_data_type(data_type);
    } else if (elem_type->isa<Float>()) {
      auto float_value = elem_type->cast<FloatPtr>();
      auto data_type = GetMindirDataBitsFloatType(float_value->nbits());
      if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
        return false;
      }
      tensor_proto->set_data_type(data_type);
    } else {
      MS_LOG(ERROR) << "Unsupported type " << elem_type->type_name();
      return false;
    }
  } else {
    MS_LOG(EXCEPTION) << "Unsupported type: " << value->type_name();
    return false;
  }
  return true;
}

bool IrExportBuilder::SetValueToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto) {
  if (value == nullptr || attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "ValuePtr or AttributeProto is null!";
  }
  if (value->isa<StringImm>() || value->isa<Scalar>()) {
    return SetScalarToAttributeProto_ir(value, attr_proto);
  } else if (value->isa<Number>() || value->isa<TensorType>()) {
    return SetTypeToAttributeProto(value, attr_proto);
  } else if (value->isa<ValueSequence>()) {
    ResetTupleIndex();
    std::string seq_string = "scalar:";
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
    if (!SetSequenceToAttributeProto(value->cast<ValueSequencePtr>(), attr_proto, &seq_string)) {
      MS_LOG(ERROR) << "Set sequence to AttributeProto failed.";
      return false;
    }
    attr_proto->set_ref_attr_name(seq_string);
    MS_LOG(DEBUG) << "Attr string: " << seq_string;
  } else if (value->isa<tensor::Tensor>()) {
    return SetTensorToAttributeProto(value, attr_proto);
  } else if (value->isa<None>()) {
    attr_proto->set_ref_attr_name("none");
    MS_LOG(DEBUG) << "Attr string: " << value->type_name();
  } else if (value->isa<Monad>()) {
    if (value->isa<UMonad>()) {
      attr_proto->set_ref_attr_name("Monad:UMonad");
    } else if (value->isa<IOMonad>()) {
      attr_proto->set_ref_attr_name("Monad:IOMonad");
    } else {
      MS_LOG(ERROR) << "Unsupported Monad type: " << value->type_name();
      return false;
    }
  } else {
    MS_LOG(ERROR) << "Unsupported type: " << value->type_name();
    return false;
  }
  return true;
}

bool IrExportBuilder::SetScalarToAttributeProto_ir(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto) {
  if (value == nullptr || attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "ValuePtr or AttributeProto is null!";
  }
  attr_proto->set_ref_attr_name("scalar:value0");
  if (value->isa<StringImm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_STRING);
    attr_proto->set_s(GetValue<std::string>(value));
  } else if (value->isa<BoolImm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_BOOL);
    int64_t attr_value = GetValue<bool>(value) ? 1 : 0;
    attr_proto->set_i(attr_value);
  } else if (SetScalarToAttributeProtoForInt_ir(value, attr_proto)) {
    return true;
  } else if (value->isa<FP32Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_FLOAT);
    attr_proto->set_f(GetValue<float>(value));
  } else if (value->isa<FP64Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_DOUBLE);
    attr_proto->set_d(GetValue<double>(value));
  } else {
    MS_LOG(ERROR) << "Unsupported scalar type: " << value->type_name();
    return false;
  }
  return true;
}

bool IrExportBuilder::SetScalarToAttributeProtoForInt_ir(const ValuePtr &value,
                                                         mind_ir::AttributeProto *const attr_proto) {
  if (value->isa<Int8Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT8);
    attr_proto->set_i(value->cast<Int8ImmPtr>()->value());
  } else if (value->isa<Int16Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT16);
    attr_proto->set_i(value->cast<Int16ImmPtr>()->value());
  } else if (value->isa<Int32Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT32);
    attr_proto->set_i(value->cast<Int32ImmPtr>()->value());
  } else if (value->isa<Int64Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT64);
    attr_proto->set_i(value->cast<Int64ImmPtr>()->value());
  } else if (value->isa<UInt8Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT8);
    attr_proto->set_i(value->cast<UInt8ImmPtr>()->value());
  } else if (value->isa<UInt16Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT16);
    attr_proto->set_i(value->cast<UInt16ImmPtr>()->value());
  } else if (value->isa<UInt32Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT32);
    attr_proto->set_i(value->cast<UInt32ImmPtr>()->value());
  } else if (value->isa<UInt64Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT64);
    attr_proto->set_i(UlongToLong(value->cast<UInt64ImmPtr>()->value()));
  } else {
    return false;
  }
  return true;
}

bool IrExportBuilder::SetTypeToAttributeProto_irs(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto) {
  if (attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "AttributeProto is null!";
  }
  if (value->isa<Int>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
    mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
    auto int_value = value->cast<IntPtr>();
    auto data_type = GetMindirDataBitsIntType(int_value->nbits());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
  } else if (value->isa<Float>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
    mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
    auto float_value = value->cast<FloatPtr>();
    auto data_type = GetMindirDataBitsFloatType(float_value->nbits());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
  } else if (value->isa<UInt>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
    mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
    auto uint_value = value->cast<FloatPtr>();
    auto data_type = GetMindirDataBitsFloatType(uint_value->nbits());
    if (data_type == mind_ir::TensorProto_DataType_UNDEFINED) {
      return false;
    }
    tensor_proto->set_data_type(data_type);
  } else if (value->isa<Bool>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSORS);
    mind_ir::TensorProto *tensor_proto = attr_proto->add_tensors();
    tensor_proto->set_data_type(mind_ir::TensorProto_DataType_BOOL);
  } else if (value->isa<tensor::Tensor>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSOR);
    return SetTensorToAttributeProto(value, attr_proto);
  } else {
    MS_LOG(EXCEPTION) << "Unsupported type: " << value->type_name();
    return false;
  }
  return true;
}

bool IrExportBuilder::SetScalarToAttributeProto_irs(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto) {
  if (attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "AttributeProto is null!";
  }
  if (value->isa<StringImm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_STRING);
    attr_proto->add_strings(GetValue<std::string>(value));
  } else if (value->isa<BoolImm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_BOOL);
    attr_proto->add_ints(GetValue<bool>(value));
  } else if (SetScalarToAttributeProtoForInt_irs(value, attr_proto)) {
    return true;
  } else if (value->isa<FP32Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_FLOAT);
    attr_proto->add_floats(GetValue<float>(value));
  } else if (value->isa<FP64Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_DOUBLE);
    attr_proto->add_doubles(GetValue<double>(value));
  } else if (value->isa<tensor::Tensor>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_TENSOR);
    return SetTensorToAttributeProto(value, attr_proto);
  } else {
    MS_LOG(ERROR) << "Unsupported scalar type: " << value->type_name();
    return false;
  }
  return true;
}

bool IrExportBuilder::SetScalarToAttributeProtoForInt_irs(const ValuePtr &value,
                                                          mind_ir::AttributeProto *const attr_proto) {
  if (value->isa<Int8Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT8);
    attr_proto->add_ints(value->cast<Int8ImmPtr>()->value());
  } else if (value->isa<Int16Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT16);
    attr_proto->add_ints(value->cast<Int16ImmPtr>()->value());
  } else if (value->isa<Int32Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT32);
    attr_proto->add_ints(value->cast<Int32ImmPtr>()->value());
  } else if (value->isa<Int64Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_INT64);
    attr_proto->add_ints(value->cast<Int64ImmPtr>()->value());
  } else if (value->isa<UInt8Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT8);
    attr_proto->add_ints(value->cast<UInt8ImmPtr>()->value());
  } else if (value->isa<UInt16Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT16);
    attr_proto->add_ints(value->cast<UInt16ImmPtr>()->value());
  } else if (value->isa<UInt32Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT32);
    attr_proto->add_ints(value->cast<UInt32ImmPtr>()->value());
  } else if (value->isa<UInt64Imm>()) {
    attr_proto->set_type(mind_ir::AttributeProto_AttributeType_UINT64);
    attr_proto->add_ints(SizeToInt(value->cast<UInt64ImmPtr>()->value()));
  } else {
    return false;
  }
  return true;
}

bool IrExportBuilder::SetSeqElemToAttributeProto(const ValuePtr &value, mind_ir::AttributeProto *const attr_proto,
                                                 std::string *const seq_string) {
  if (value == nullptr) {
    MS_LOG(ERROR) << "Value is nullptr";
    return false;
  }
  string value_name = "value" + std::to_string(GetTupleIndex());
  if (seq_string != nullptr) {
    *seq_string += value_name + ",";
  }
  if (value->isa<StringImm>() || value->isa<Scalar>()) {
    return SetScalarToAttributeProto_irs(value, attr_proto);
  }
  return SetTypeToAttributeProto_irs(value, attr_proto);
}

bool IrExportBuilder::SetSequenceToAttributeProto(const ValueSequencePtr &value,
                                                  mind_ir::AttributeProto *const attr_proto,
                                                  std::string *const seq_string) {
  if (value == nullptr || attr_proto == nullptr) {
    MS_LOG(EXCEPTION) << "ValueSequencePtr or AttributeProto is null!";
  }
  if (value->isa<ValueTuple>() && seq_string != nullptr) {
    *seq_string += "Tuple[";
    const ValueTuplePtr &tuple_value = value->cast<ValueTuplePtr>();
    if (tuple_value->value().size() == 0) {
      *seq_string += "],";
      MS_LOG(DEBUG) << "SetSequenceToAttributeProto tuple size is 0";
      return true;
    }
    for (const auto &item : tuple_value->value()) {
      if (item->isa<ValueTuple>()) {
        if (!SetSequenceToAttributeProto(item->cast<ValueTuplePtr>(), attr_proto, seq_string)) {
          MS_LOG(ERROR) << "Set sequence to AttributeProto failed.";
          return false;
        }
      } else {
        if (!SetSeqElemToAttributeProto(item, attr_proto, seq_string)) {
          MS_LOG(ERROR) << "Set seq elem to AttributeProto failed.";
          return false;
        }
      }
    }
    *seq_string += "],";
  } else if (value->isa<ValueList>() && seq_string != nullptr) {
    *seq_string += "List[";
    const ValueListPtr &list_value = value->cast<ValueListPtr>();
    if (list_value->value().size() == 0) {
      *seq_string += "],";
      MS_LOG(DEBUG) << "SetSequenceToAttributeProto list size is 0.";
      return true;
    }
    for (const auto &item : list_value->value()) {
      MS_EXCEPTION_IF_NULL(item);
      if (item->isa<ValueList>()) {
        if (!SetSequenceToAttributeProto(item->cast<ValueListPtr>(), attr_proto, seq_string)) {
          MS_LOG(ERROR) << "Set sequence to AttributeProto failed.";
          return false;
        }
      } else {
        if (!SetSeqElemToAttributeProto(item, attr_proto, seq_string)) {
          MS_LOG(ERROR) << "Set seq elem to AttributeProto failed.";
          return false;
        }
      }
    }
    *seq_string += "],";
  }
  return true;
}

std::string GetBinaryProtoString(const FuncGraphPtr &func_graph) {
  auto builder = std::make_shared<IrExportBuilder>();
  if (builder == nullptr) {
    MS_LOG(ERROR) << "Create ir exporter failed!";
    return "";
  }
  auto exporter = std::make_shared<IrExporter>(builder);
  if (exporter == nullptr) {
    return "";
  }
  auto ret = exporter->GetDumpString(func_graph);
  return ret;
}

ModelProtoPtr GetBinaryProto(const FuncGraphPtr &func_graph, const FuncGraphPtr &param_layout_fg) {
  auto exporter = std::make_shared<IrExporter>(std::make_shared<IrExportBuilder>());
  auto result = exporter->GetDumpProto(func_graph, param_layout_fg);
  return result;
}
}  // namespace mindspore
