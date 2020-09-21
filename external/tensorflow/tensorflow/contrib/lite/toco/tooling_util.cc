/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "tensorflow/contrib/lite/toco/tooling_util.h"

#include <functional>
#include <iterator>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "tensorflow/contrib/lite/toco/dump_graphviz.h"
#include "tensorflow/contrib/lite/toco/model_flags.pb.h"
#include "tensorflow/contrib/lite/toco/toco_graphviz_dump_options.h"
#include "tensorflow/contrib/lite/toco/toco_port.h"
#include "tensorflow/core/platform/logging.h"

namespace toco {

// Find the longest common prefix of two strings.
absl::string_view FindLongestCommonPrefix(absl::string_view a,
                                          absl::string_view b) {
  if (a.empty() || b.empty()) return absl::string_view();

  const char* pa = a.data();
  const char* pb = b.data();
  size_t count = 0;
  const size_t limit = std::min(a.size(), b.size());
  while (count < limit && *pa == *pb) {
    ++pa;
    ++pb;
    ++count;
  }

  return absl::string_view(a.data(), count);
}

string LogName(const Operator& op) {
  const string& opname = HelpfulOperatorTypeName(op);
  if (op.outputs.empty()) {
    return toco::port::StringF("{%s operator}", opname);
  } else {
    return toco::port::StringF("{%s operator with output %s}", opname,
                               op.outputs[0]);
  }
}

bool IsInputArray(const Model& model, const string& name) {
  for (const auto& input_array : model.flags.input_arrays()) {
    if (input_array.name() == name) {
      return true;
    }
  }
  return false;
}

bool IsArrayConsumed(const Model& model, const string& name) {
  if (GetOpWithInput(model, name)) {
    return true;
  }
  for (const string& model_output : model.flags.output_arrays()) {
    if (model_output == name) {
      return true;
    }
  }
  for (const auto& rnn_state : model.flags.rnn_states()) {
    if (rnn_state.back_edge_source_array() == name) {
      return true;
    }
  }
  return false;
}

int CountTrueOutputs(const Model& model, const Operator& op) {
  int count = 0;
  for (const string& output : op.outputs) {
    if (IsArrayConsumed(model, output)) {
      ++count;
    }
  }
  return count;
}

int CountOpsWithInput(const Model& model, const string& array_name) {
  int count = 0;
  for (const auto& op : model.operators) {
    for (auto& input : op->inputs) {
      if (input == array_name) {
        count++;
      }
    }
  }
  return count;
}

bool DeleteArrayIfUnused(const string& array_name, Model* model) {
  if (IsDiscardableArray(*model, array_name) &&
      CountOpsWithInput(*model, array_name) == 0) {
    model->EraseArray(array_name);
    return true;
  }
  return false;
}

bool DeleteArrayIfUsedOnce(const string& array_name, Model* model) {
  if (IsDiscardableArray(*model, array_name) &&
      CountOpsWithInput(*model, array_name) == 1) {
    model->EraseArray(array_name);
    return true;
  }
  return false;
}

std::vector<std::unique_ptr<Operator>>::const_iterator FindOpWithOutput(
    const Model& model, const string& array_name) {
  for (auto it = model.operators.begin(); it != model.operators.end(); ++it) {
    for (auto& output : it->get()->outputs) {
      if (output == array_name) {
        return it;
      }
    }
  }
  return model.operators.end();
}

std::vector<std::unique_ptr<Operator>>::iterator FindOpWithOutput(
    Model& model, const string& array_name) {
  for (auto it = model.operators.begin(); it != model.operators.end(); ++it) {
    for (auto& output : it->get()->outputs) {
      if (output == array_name) {
        return it;
      }
    }
  }
  return model.operators.end();
}

Operator* GetOpWithOutput(const Model& model, const string& array_name) {
  auto it = FindOpWithOutput(model, array_name);
  return it == model.operators.end() ? nullptr : it->get();
}

// GetFirstOpWithInput assumes that this finds the first op.
std::vector<std::unique_ptr<Operator>>::const_iterator FindOpWithInput(
    const Model& model, const string& array_name) {
  for (auto it = model.operators.begin(); it != model.operators.end(); ++it) {
    for (auto& input : it->get()->inputs) {
      if (input == array_name) {
        return it;
      }
    }
  }
  return model.operators.end();
}

std::vector<std::unique_ptr<Operator>>::iterator FindOpWithInput(
    Model& model, const string& array_name) {
  for (auto it = model.operators.begin(); it != model.operators.end(); ++it) {
    for (auto& input : it->get()->inputs) {
      if (input == array_name) {
        return it;
      }
    }
  }
  return model.operators.end();
}

std::vector<std::unique_ptr<Operator>>::const_iterator FindOp(
    const Model& model, const Operator* op) {
  for (auto it = model.operators.begin(); it != model.operators.end(); ++it) {
    if (it->get() == op) {
      return it;
    }
  }
  return model.operators.end();
}

std::vector<std::unique_ptr<Operator>>::iterator FindOp(Model& model,
                                                        const Operator* op) {
  for (auto it = model.operators.begin(); it != model.operators.end(); ++it) {
    if (it->get() == op) {
      return it;
    }
  }
  return model.operators.end();
}

Operator* GetOpWithInput(const Model& model, const string& array_name) {
  auto it = FindOpWithInput(model, array_name);
  return it == model.operators.end() ? nullptr : it->get();
}

Operator* GetFirstOpWithInput(const Model& model, const string& array_name) {
  auto it = FindOpWithInput(model, array_name);
  return it == model.operators.end() ? nullptr : it->get();
}

string FormatArraysList(const Model& model, const std::vector<string>& list) {
  if (list.empty()) {
    return "[]";
  }
  string result = "";
  if (list.size() > 1) {
    result += "[ ";
  }
  for (std::size_t i = 0; i < list.size(); i++) {
    if (i > 0) {
      result += ", ";
    }
    result += list[i];
  }
  if (list.size() > 1) {
    result += " ]";
  }
  return result;
}

const char* OperatorTypeName(OperatorType type) {
  switch (type) {
#define HANDLE_OPERATORTYPENAME_CASE(c) \
  case OperatorType::k##c:              \
    return #c;
    HANDLE_OPERATORTYPENAME_CASE(Add)
    HANDLE_OPERATORTYPENAME_CASE(AddN)
    HANDLE_OPERATORTYPENAME_CASE(AveragePool)
    HANDLE_OPERATORTYPENAME_CASE(BatchMatMul)
    HANDLE_OPERATORTYPENAME_CASE(BatchNormalization)
    HANDLE_OPERATORTYPENAME_CASE(Conv)
    HANDLE_OPERATORTYPENAME_CASE(Concatenation)
    HANDLE_OPERATORTYPENAME_CASE(DepthwiseConv)
    HANDLE_OPERATORTYPENAME_CASE(DepthToSpace)
    HANDLE_OPERATORTYPENAME_CASE(SpaceToDepth)
    HANDLE_OPERATORTYPENAME_CASE(FullyConnected)
    HANDLE_OPERATORTYPENAME_CASE(Dequantize)
    HANDLE_OPERATORTYPENAME_CASE(L2Normalization)
    HANDLE_OPERATORTYPENAME_CASE(LocalResponseNormalization)
    HANDLE_OPERATORTYPENAME_CASE(Logistic)
    HANDLE_OPERATORTYPENAME_CASE(LstmCell)
    HANDLE_OPERATORTYPENAME_CASE(MaxPool)
    HANDLE_OPERATORTYPENAME_CASE(L2Pool)
    HANDLE_OPERATORTYPENAME_CASE(FakeQuant)
    HANDLE_OPERATORTYPENAME_CASE(Mul)
    HANDLE_OPERATORTYPENAME_CASE(Relu)
    HANDLE_OPERATORTYPENAME_CASE(Relu1)
    HANDLE_OPERATORTYPENAME_CASE(Relu6)
    HANDLE_OPERATORTYPENAME_CASE(ReorderAxes)
    HANDLE_OPERATORTYPENAME_CASE(Softmax)
    HANDLE_OPERATORTYPENAME_CASE(LogSoftmax)
    HANDLE_OPERATORTYPENAME_CASE(Div)
    HANDLE_OPERATORTYPENAME_CASE(Tanh)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowAll)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowAssert)
    HANDLE_OPERATORTYPENAME_CASE(ExpandDims)
    HANDLE_OPERATORTYPENAME_CASE(Fill)
    HANDLE_OPERATORTYPENAME_CASE(FloorMod)
    HANDLE_OPERATORTYPENAME_CASE(FloorDiv)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowGreater)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowGreaterEqual)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowIdentity)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowLess)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowLessEqual)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowMatMul)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowMax)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowMaximum)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowMerge)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowMin)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowMinimum)
    HANDLE_OPERATORTYPENAME_CASE(Neg)
    HANDLE_OPERATORTYPENAME_CASE(Pad)
    HANDLE_OPERATORTYPENAME_CASE(StridedSlice)
    HANDLE_OPERATORTYPENAME_CASE(Stack)
    HANDLE_OPERATORTYPENAME_CASE(Range)
    HANDLE_OPERATORTYPENAME_CASE(Rank)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowReshape)
    HANDLE_OPERATORTYPENAME_CASE(Squeeze)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowRsqrt)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowShape)
    HANDLE_OPERATORTYPENAME_CASE(Slice)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowSplit)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowSqrt)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowSquare)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowSwitch)
    HANDLE_OPERATORTYPENAME_CASE(Sub)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowSum)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowTile)
    HANDLE_OPERATORTYPENAME_CASE(Transpose)
    HANDLE_OPERATORTYPENAME_CASE(TransposeConv)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowConcat)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowConcatV2)
    HANDLE_OPERATORTYPENAME_CASE(Cast)
    HANDLE_OPERATORTYPENAME_CASE(Floor)
    HANDLE_OPERATORTYPENAME_CASE(Gather)
    HANDLE_OPERATORTYPENAME_CASE(ResizeBilinear)
    HANDLE_OPERATORTYPENAME_CASE(SpaceToBatchND)
    HANDLE_OPERATORTYPENAME_CASE(BatchToSpaceND)
    HANDLE_OPERATORTYPENAME_CASE(Mean)
    HANDLE_OPERATORTYPENAME_CASE(Svdf)
    HANDLE_OPERATORTYPENAME_CASE(ArgMax)
    HANDLE_OPERATORTYPENAME_CASE(TopK_V2)
    HANDLE_OPERATORTYPENAME_CASE(TensorFlowUnsupported)
    HANDLE_OPERATORTYPENAME_CASE(Exp)
    default:
      LOG(FATAL) << "Unhandled op type";
#undef HANDLE_OPERATORTYPENAME_CASE
  }
}

string HelpfulOperatorTypeName(const Operator& op) {
  if (op.type == OperatorType::kTensorFlowUnsupported) {
    return toco::port::StringF(
        "(Unsupported TensorFlow op: %s)",
        static_cast<const TensorFlowUnsupportedOperator&>(op).tensorflow_op);
  }
  return OperatorTypeName(op.type);
}

bool OperatorSupportsFusedActivation(OperatorType type) {
  switch (type) {
    case OperatorType::kConcatenation:
    case OperatorType::kGather:
    case OperatorType::kSlice:
    case OperatorType::kSqueeze:
    case OperatorType::kTensorFlowReshape:
    case OperatorType::kTensorFlowSplit:
      return false;
    default:
      return true;
  }
}

void LogSummary(int log_level, const Model& model) {
  VLOG(log_level) << "Operators summary (" << model.operators.size()
                  << " operators):";
  std::unordered_multiset<OperatorType> ops_by_type;
  for (const auto& op : model.operators) {
    ops_by_type.insert(op->type);
  }
  auto it = ops_by_type.begin();
  while (it != ops_by_type.end()) {
    int count = ops_by_type.count(*it);
    VLOG(log_level) << "    " << OperatorTypeName(*it) << ": " << count;
    std::advance(it, count);
  }
}

void LogArray(int log_level, const Model& model, const string& name) {
  const auto& array = model.GetArray(name);
  VLOG(log_level) << "Array: " << name;
  switch (array.data_type) {
    case ArrayDataType::kNone:
      VLOG(log_level) << "  Data type:";
      break;
    case ArrayDataType::kFloat:
      VLOG(log_level) << "  Data type: kFloat";
      break;
    case ArrayDataType::kInt32:
      VLOG(log_level) << "  Data type: kInt32";
      break;
    case ArrayDataType::kUint8:
      VLOG(log_level) << "  Data type: kUint8";
      break;
    case ArrayDataType::kString:
      VLOG(log_level) << "  Data type: kString";
      break;
    default:
      VLOG(log_level) << "  Data type: other (numerical value: "
                      << static_cast<int>(array.data_type) << ")";
      break;
  }
  switch (array.final_data_type) {
    case ArrayDataType::kNone:
      VLOG(log_level) << "  Final type:";
      break;
    case ArrayDataType::kFloat:
      VLOG(log_level) << "  Final type: kFloat";
      break;
    case ArrayDataType::kInt32:
      VLOG(log_level) << "  Final type: kInt32";
      break;
    case ArrayDataType::kUint8:
      VLOG(log_level) << "  Final type: kUint8";
      break;
    case ArrayDataType::kString:
      VLOG(log_level) << "  Final type: kString";
      break;
    default:
      VLOG(log_level) << "  Final type: other (numerical value: "
                      << static_cast<int>(array.data_type) << ")";
      break;
  }
  if (array.buffer) {
    VLOG(log_level) << "  Constant Buffer";
  }
  if (array.alloc) {
    VLOG(log_level) << "  Transient Alloc";
  }
  if (array.has_shape()) {
    const Shape& array_shape = array.shape();
    if (array_shape.dimensions_count() == 0) {
      VLOG(log_level) << "  (Zero dimensions)";
    } else {
      string message = "  Dims: ";
      bool first = true;
      for (const int dim : array_shape.dims()) {
        if (!first) {
          message += ", ";
        }
        first = false;
        toco::port::AppendF(&message, "%d", dim);
      }
      VLOG(log_level) << message;
    }
  }
  if (array.minmax) {
    VLOG(log_level) << "  MinMax: " << array.minmax->min << " .. "
                    << array.minmax->max;
  }
  if (array.quantization_params) {
    VLOG(log_level) << "  QuantizationParams: zero_point="
                    << static_cast<int>(array.quantization_params->zero_point)
                    << ", scale=" << array.quantization_params->scale;
  }
}

void DumpGraphvizVideoFrame(const Model& model) {
  namespace port = toco::port;

  const auto& dump_options = *GraphVizDumpOptions::singleton();
  if (!dump_options.dump_graphviz_video) {
    return;
  }
  CHECK(!dump_options.dump_graphviz.empty());
  // TODO(benoitjacob): the static data here means that this function
  // is stateful, not reentrant, and effectively leaks memory till exit
  // (since dump_hashes can only grow in size). It also means that it
  // really only is intended to be called for a single model during the
  // process' lifetime. So it's not great design at all. The overriding
  // design aspect here is to make the video-dumping code as unintrusive
  // and self-contained as possible. Eventually, we'll want to have that
  // cleaned-up, but that will require some form of general statefulness
  // in toco (some kind of 'tooling state' data structure) that does
  // not exist at present, and would be premature to design here just for
  // this new video-dumping feature.
  static int dump_id = 0;
  static std::unordered_set<std::size_t> dump_hashes;
  string graphviz_dump;
  DumpGraphviz(model, &graphviz_dump);
  std::size_t hash = std::hash<string>{}(graphviz_dump);
  if (!dump_hashes.count(hash)) {
    LOG(INFO) << "DUMPING GRAPHVIZ VIDEO FRAME: " << dump_id;
    dump_hashes.insert(hash);
    CHECK(port::file::SetContents(
              port::file::JoinPath(
                  dump_options.dump_graphviz,
                  toco::port::StringF("toco_video_%05d.dot", dump_id)),
              graphviz_dump, port::file::Defaults())
              .ok());
    dump_id++;
  }
}

void LogDump(int log_level, const string& message, const Model& model) {
  namespace port = toco::port;
  const auto& dump_options = *GraphVizDumpOptions::singleton();

  DumpGraphvizVideoFrame(model);
  if (!dump_options.dump_graphviz.empty()) {
    string graphviz_dump;

    DumpGraphviz(model, &graphviz_dump);
    CHECK(port::file::SetContents(
              port::file::JoinPath(
                  dump_options.dump_graphviz,
                  absl::StrCat("toco_",
                               absl::StrReplaceAll(message, {{" ", "_"}}),
                               ".dot")),
              graphviz_dump, port::file::Defaults())
              .ok());
  }

  if (!VLOG_IS_ON(log_level)) {
    return;
  }
  VLOG(log_level) << "BEGIN DUMP OF TOCO MODEL (" << message << ")";
  LogSummary(log_level, model);
  std::unordered_set<string> already_printed_arrays;
  for (const auto& op : model.operators) {
    for (const auto& input : op->inputs) {
      if (!already_printed_arrays.count(input)) {
        already_printed_arrays.insert(input);
        LogArray(log_level, model, input);
      }
    }
    VLOG(log_level) << HelpfulOperatorTypeName(*op) << " :";
    VLOG(log_level) << "  " << FormatArraysList(model, op->inputs) << " -> "
                    << FormatArraysList(model, op->outputs);
    if (op->fused_activation_function != FusedActivationFunctionType::kNone) {
      VLOG(log_level) << "    (with fused activation function)";
    }
    for (const auto& output : op->outputs) {
      if (!already_printed_arrays.count(output)) {
        already_printed_arrays.insert(output);
        LogArray(log_level, model, output);
      }
    }
  }
  VLOG(log_level) << "END DUMP OF TOCO MODEL (" << message << ")";
}

// Note remaining raw-array extension in ProcessTensorFlowReshapeOperator().
void ExtendShape(Shape* shape, int new_shape_size) {
  CHECK_GE(new_shape_size, shape->dimensions_count());
  const int size_increase = new_shape_size - shape->dimensions_count();
  auto* shape_dims = shape->mutable_dims();
  shape_dims->insert(shape_dims->begin(), size_increase, 1);
}

// TODO(b/62904716) Remove along with remaining uses.
void UnextendShape(Shape* shape, int new_shape_size) {
  CHECK_LE(new_shape_size, shape->dimensions_count());
  const int size_reduction = shape->dimensions_count() - new_shape_size;
  for (int i = 0; i < size_reduction; i++) {
    CHECK_EQ(shape->dims(i), 1);
  }
  std::vector<int>& shape_dims = *shape->mutable_dims();
  shape_dims.erase(shape_dims.begin(), shape_dims.begin() + size_reduction);
}

void CheckShapeDimensions(const Shape& shape) {
  for (int i = 0; i < shape.dimensions_count(); ++i) {
    CHECK_GE(shape.dims()[i], 1) << "shape has dimension 0 at index << " << i
                                 << ". shape = " << ShapeToString(shape);
  }
}

bool ShapesAgreeUpToBroadcasting(const Shape& shape0, const Shape& shape1) {
  CheckShapeDimensions(shape0);
  CheckShapeDimensions(shape1);

  const Shape* longer = &shape0;
  const Shape* shorter = &shape1;
  if (shape1.dimensions_count() > shape0.dimensions_count()) {
    longer = &shape1;
    shorter = &shape0;
  }

  // Walk dimensions back to front until we run out of dimensions in the shorter
  // shape.
  int longer_index = longer->dimensions_count() - 1;
  int shorter_index = shorter->dimensions_count() - 1;
  while (shorter_index >= 0) {
    const int d_long = longer->dims(longer_index);
    const int d_short = shorter->dims(shorter_index);
    // Broadcasting fails if the dimensions are different *and* neither is 1.
    if ((d_long != d_short) && (d_long != 1) && (d_short != 1)) {
      return false;
    }
    longer_index--;
    shorter_index--;
  }
  return true;
}

bool ShapesAgreeUpToExtending(const Shape& shape0, const Shape& shape1) {
  CheckShapeDimensions(shape0);
  CheckShapeDimensions(shape1);

  const Shape* longer = &shape0;
  const Shape* shorter = &shape1;
  if (shape1.dimensions_count() > shape0.dimensions_count()) {
    longer = &shape1;
    shorter = &shape0;
  }

  // Walk dimensions back to front until we run out of dimensions in the shorter
  // shape.
  int longer_index = longer->dimensions_count() - 1;
  int shorter_index = shorter->dimensions_count() - 1;
  while (shorter_index >= 0) {
    const int d_long = longer->dims(longer_index);
    const int d_short = shorter->dims(shorter_index);
    // Extending fails if the dimensions are different.
    if (d_long != d_short) {
      return false;
    }
    longer_index--;
    shorter_index--;
  }

  // The remaining dimensions in the longer shape must be 1.
  while (longer_index >= 0) {
    const int d_long = longer->dims(longer_index);
    if (d_long != 1) {
      return false;
    }
    longer_index--;
  }

  return true;
}

int RequiredBufferSizeForShape(const Shape& shape) {
  int max_offset = 1;
  for (const auto& dim : shape.dims()) {
    CHECK_GE(dim, 1);
    max_offset *= dim;
  }
  return max_offset;
}

bool IsConstantParameterArray(const Model& model, const string& name) {
  if (!model.HasArray(name)) {
    return false;
  }

  return !!model.GetArray(name).buffer;
}

namespace {
// Take an array name, which may be something like "name:3_5" and make it
// acceptable as a TF node name, say "name_3_5";
string SanitizeNameForTFNode(const string& array_name) {
  auto node_name = array_name;
  std::replace(node_name.begin(), node_name.end(), ':', '_');
  return node_name;
}

void CheckInputArraysAreNotOutputArrays(const ModelFlags& model_flags) {
  for (const auto& input_array : model_flags.input_arrays()) {
    for (const string& output_array : model_flags.output_arrays()) {
      QCHECK_NE(input_array.name(), output_array)
          << "The array " << output_array
          << " is listed in both --input_arrays and --output_arrays.";
    }
  }
}

bool IsAsciiPrintable(const string& name) {
  for (char c : name) {
    if (!absl::ascii_isprint(c)) {
      return false;
    }
  }
  return true;
}

string DumpAscii(const string& name) {
  string result;
  port::AppendF(&result, "ASCII | Hex\n");
  port::AppendF(&result, "------+----\n");
  for (char c : name) {
    if (absl::ascii_isprint(c)) {
      port::AppendF(&result, "%c     | %x\n", c, c);
    } else {
      port::AppendF(&result, "      | %x   Not ASCII printable!\n", c);
    }
  }
  return result;
}

void CheckNonAsciiIOArrays(const ModelFlags& model_flags) {
  if (model_flags.allow_nonascii_arrays()) {
    return;
  }
  for (const auto& input_array : model_flags.input_arrays()) {
    QCHECK(IsAsciiPrintable(input_array.name()))
        << "Non-ASCII-printable character found in --input_arrays: "
        << input_array.name()
        << ". Pass --allow_nonascii_arrays to allow that. "
        << "Here is a dump of the string:\n\n"
        << DumpAscii(input_array.name());
  }
  for (const string& output_array : model_flags.output_arrays()) {
    QCHECK(IsAsciiPrintable(output_array))
        << "Non-ASCII-printable character found in --output_arrays: "
        << output_array << ". Pass --allow_nonascii_arrays to allow that. "
        << "Here is a dump of the string:\n\n"
        << DumpAscii(output_array);
  }
}

void CheckNonExistentIOArrays(const Model& model) {
  if (model.flags.allow_nonexistent_arrays()) {
    return;
  }
  for (const auto& input_array : model.flags.input_arrays()) {
    CHECK(model.HasArray(input_array.name()))
        << "Input array not found: " << input_array.name();
  }
  for (const string& output_array : model.flags.output_arrays()) {
    CHECK(model.HasArray(output_array))
        << "Output array not found: " << output_array;
  }
  for (const auto& rnn_state : model.flags.rnn_states()) {
    if (!rnn_state.discardable()) {
      CHECK(model.HasArray(rnn_state.state_array()));
      CHECK(model.HasArray(rnn_state.back_edge_source_array()));
    }
  }
}
}  // namespace

void CheckNoMissingArray(const Model& model) {
  for (const auto& op : model.operators) {
    for (const auto& input : op->inputs) {
      CHECK(model.HasArray(input) || model.optional_arrays.count(input))
          << "Input: " << input << " missing for op: " << op->outputs[0] << ".";
    }
    for (const auto& output : op->outputs) {
      CHECK(model.HasArray(output)) << "Output: " << output << " missing.";
    }
  }
  CheckNonExistentIOArrays(model);
}

void FixNoMissingArray(Model* model) {
  for (const auto& op : model->operators) {
    for (const auto& input : op->inputs) {
      if (!model->HasArray(input)) {
        model->GetOrCreateArray(input);
      }
    }
    for (const auto& output : op->outputs) {
      if (!model->HasArray(output)) {
        model->GetOrCreateArray(output);
      }
    }
  }
  if (model->flags.allow_nonexistent_arrays()) {
    for (const string& output_array : model->flags.output_arrays()) {
      model->GetOrCreateArray(output_array);
    }
    for (const auto& rnn_state : model->flags.rnn_states()) {
      model->GetOrCreateArray(rnn_state.state_array());
      model->GetOrCreateArray(rnn_state.back_edge_source_array());
    }
  }
}

void CheckNoOrphanedArray(const Model& model) {
  std::unordered_set<string> arrays_without_known_use;
  for (const auto& array : model.GetArrayMap()) {
    if (IsDiscardableArray(model, array.first)) {
      arrays_without_known_use.insert(array.first);
    }
  }
  for (const auto& op : model.operators) {
    for (const auto& input : op->inputs) {
      arrays_without_known_use.erase(input);
    }
    for (const auto& output : op->outputs) {
      arrays_without_known_use.erase(output);
    }
  }
  for (const auto& rnn_state : model.flags.rnn_states()) {
    arrays_without_known_use.erase(rnn_state.state_array());
    arrays_without_known_use.erase(rnn_state.back_edge_source_array());
  }
  if (!arrays_without_known_use.empty()) {
    for (const auto& array : arrays_without_known_use) {
      LOG(INFO) << "Error: Orphaned array: " << array;
    }
  }
  CHECK(arrays_without_known_use.empty());
}

void FixNoOrphanedArray(Model* model) {
  std::unordered_set<string> arrays_without_known_use;
  for (const auto& array : model->GetArrayMap()) {
    arrays_without_known_use.insert(array.first);
  }
  for (const auto& op : model->operators) {
    for (const auto& input : op->inputs) {
      arrays_without_known_use.erase(input);
    }
    for (const auto& output : op->outputs) {
      arrays_without_known_use.erase(output);
    }
  }
  for (const auto& rnn_state : model->flags.rnn_states()) {
    arrays_without_known_use.erase(rnn_state.state_array());
    arrays_without_known_use.erase(rnn_state.back_edge_source_array());
  }
  for (const auto& array : arrays_without_known_use) {
    if (IsDiscardableArray(*model, array)) {
      model->EraseArray(array);
    }
  }
}

// Apply checks to arrays individually (for-each fashion).
//
// Check consistency of array fields, check name.
void CheckEachArray(const Model& model) {
  for (const auto& array_entry : model.GetArrayMap()) {
    const auto& array = array_entry.second;
    if (array->has_shape()) {
      for (int d : array->shape().dims()) {
        CHECK_GE(d, 1);
      }
    }
    // It's OK to have a buffer or an alloc, but not both.
    // (Since allocs are for transient arrays without a buffer).
    CHECK(!array->buffer || !array->alloc);
    // If there is a buffer, its type should be consistent with data_type.
    if (array->buffer) {
      CHECK(array->buffer->type == array->data_type);
    }

    // Check name.  Either "name_with_suffix_8", "name_with_port:3", but not
    // "name_with_both:3_8".
    const string& name = array_entry.first;
    auto colon_pos = name.find_first_of(":");
    if (colon_pos != string::npos) {
      CHECK_EQ(name.substr(colon_pos + 1).find_first_not_of("0123456789"),
               string::npos)
          << "Array name must only have digits after colon";
    }
    CHECK_GT(colon_pos, 0)
        << "First character of array name must not be a colon.";
  }
}

void CheckOperatorOrdering(const Model& model) {
  std::unordered_set<string> arrays_behind_us;
  for (const auto& array_entry : model.GetArrayMap()) {
    if (!GetOpWithOutput(model, array_entry.first)) {
      arrays_behind_us.insert(array_entry.first);
    }
  }
  arrays_behind_us.insert(model.optional_arrays.begin(),
                          model.optional_arrays.end());
  for (const auto& op : model.operators) {
    for (const auto& input : op->inputs) {
      if (!IsConstantParameterArray(model, input)) {
        CHECK(arrays_behind_us.count(input));
      }
    }
    for (const auto& output : op->outputs) {
      CHECK(!arrays_behind_us.count(output));
      arrays_behind_us.insert(output);
    }
  }
  for (const string& output_array : model.flags.output_arrays()) {
    CHECK(arrays_behind_us.count(output_array));
  }
}

void FixOperatorOrdering(Model* model) {
  std::unordered_set<string> arrays_behind_us;
  for (const auto& array_entry : model->GetArrayMap()) {
    if (!GetOpWithOutput(*model, array_entry.first)) {
      arrays_behind_us.insert(array_entry.first);
    }
  }
  arrays_behind_us.insert(model->optional_arrays.begin(),
                          model->optional_arrays.end());
  std::vector<std::unique_ptr<Operator>> old_operators;
  std::swap(old_operators, model->operators);
  std::set<std::size_t> remaining;
  for (std::size_t i = 0; i < old_operators.size(); i++) {
    remaining.insert(i);
  }
  std::unordered_map<string, string> reason_why_leftover;
  while (true) {
    bool inserted_something = false;
    for (auto i : remaining) {
      bool can_insert = true;
      auto& op = old_operators[i];
      CHECK(op.get());
      for (const auto& input : op->inputs) {
        if (!IsConstantParameterArray(*model, input) &&
            !arrays_behind_us.count(input)) {
          for (const string& output : op->outputs) {
            reason_why_leftover[output] = input;
          }
          can_insert = false;
          break;
        }
      }
      if (can_insert) {
        model->operators.emplace_back(nullptr);
        for (const auto& output : op->outputs) {
          arrays_behind_us.insert(output);
        }
        std::swap(op, model->operators.back());
        remaining.erase(i);
        inserted_something = true;
        break;
      }
    }
    if (!inserted_something) {
      break;
    }
  }
  if (!remaining.empty()) {
    LOG(ERROR)
        << "No viable ordering of operators was found. "
        << "Here is a 'backtrace' of at least one part of the graph that is "
        << "problematic. It starts with the first operator that has as "
        << "problematic input array, and then walks back the graph to "
        << "the operator that produced that input array, etc., until we find "
        << "the root cause:";
    LOG(ERROR) << "BEGIN TRACE OF OPERATOR WITH BAD INPUT";
    LOG(ERROR) << "Here is the first-encountered operator with a bad input: ";
    const Operator* bad_op = old_operators[*remaining.begin()].get();
    std::unordered_set<string> bad_inputs_already_traced;
    // The following while(true) loop should always end with a LOG(FATAL).
    while (true) {
      LOG(ERROR) << HelpfulOperatorTypeName(*bad_op) << " : "
                 << FormatArraysList(*model, bad_op->inputs) << " -> "
                 << FormatArraysList(*model, bad_op->outputs);
      bool found_bad_output = false;
      string bad_output;
      for (const string& output : bad_op->outputs) {
        if (reason_why_leftover.count(output)) {
          found_bad_output = true;
          bad_output = output;
          break;
        }
      }
      CHECK(found_bad_output);
      const string& bad_input = reason_why_leftover[bad_output];
      LOG(ERROR) << "The bad input here is: " << bad_input;
      if (bad_inputs_already_traced.count(bad_input)) {
        LOG(FATAL)
            << "Cycle found! We already encountered that "
            << "input array, " << bad_input << ", earlier in the "
            << "above trace! We expect graphs to be acyclic, even "
            << "RNNs. Let us know if some graph actually needs to have "
            << "cycles, but first, please check if it really is "
            << "an *inference* graph. *Training* graphs are out-of-scope "
            << "for toco.";
      }
      bad_inputs_already_traced.insert(bad_input);
      bad_op = nullptr;
      for (auto i : remaining) {
        const Operator* op = old_operators[i].get();
        for (const string& output : op->outputs) {
          if (bad_input == output) {
            bad_op = op;
            break;
          }
        }
        if (bad_op) {
          break;
        }
      }
      if (!bad_op) {
        LOG(ERROR) << "And that's the root cause: "
                   << "that array, " << bad_input << ", isn't produced by any "
                   << "operator, or provided in any other way.";
        LOG(ERROR) << "END TRACE OF OPERATOR WITH BAD INPUT";
        LOG(FATAL) << "(The above was a multi-line fatal error)";
      }
      LOG(ERROR) << "And that array is the output of the following operator:";
    }
  }
  CHECK(remaining.empty())
      << "Should never get here! In case of bad graph, "
      << "the above code should have generated a FATAL error already!";
}

void CheckInvariants(const Model& model) {
  CheckInputArraysAreNotOutputArrays(model.flags);
  CheckNonAsciiIOArrays(model.flags);
  CheckNoMissingArray(model);
  CheckNoOrphanedArray(model);
  CheckEachArray(model);
  CheckOperatorOrdering(model);
}

void CheckCountInRange(const ::toco::ModelFlags::ModelCheck& model_check,
                       const int count, const string& count_description) {
  if (model_check.count_min() >= 0) {
    CHECK_GE(count, model_check.count_min())
        << "Mismatch in " << count_description << ": count  was " << count
        << ", but the specified "
        << (model_check.count_max() > model_check.count_min() ? "minimum"
                                                              : "value")
        << " was " << model_check.count_min() << ".";
  }
  if (model_check.count_max() > model_check.count_min()) {
    CHECK_LE(count, model_check.count_max())
        << "Mismatch in " << count_description << ": count  was " << count
        << ", but the specified maximum was " << model_check.count_max() << ".";
  }
}

void CheckModelCounts(const Model& model) {
  std::unordered_multiset<OperatorType> ops_by_type;
  std::unordered_map<string, OperatorType> op_type_by_name;
  if (model.flags.model_checks_size() == 0) {
    return;
  }

  for (const auto& op : model.operators) {
    ops_by_type.insert(op->type);
    op_type_by_name[OperatorTypeName(op->type)] = op->type;
  }
  for (const auto& model_check : model.flags.model_checks()) {
    string count_type = model_check.count_type();
    if (count_type == "None") {
      continue;
    } else if (count_type == "Arrays") {
      CheckCountInRange(model_check, model.GetArrayMap().size(),
                        "count of arrays");
    } else if (count_type == "Total") {
      CheckCountInRange(model_check, model.operators.size(),
                        "count of all operator instances");
    } else {
      // The check type is not itself checked against the set of valid
      // operators, mainly because the enum set cannot be iterated in C++.
      const int found_count =
          op_type_by_name.count(count_type) > 0
              ? ops_by_type.count(op_type_by_name[count_type])
              : 0;
      CheckCountInRange(model_check, found_count,
                        "count of instances of " + count_type + " operator");
    }
  }
}

void MakeArrayDims(int num_dims, int batch, int height, int width, int depth,
                   std::vector<int>* out_dims) {
  CHECK(out_dims->empty());
  if (num_dims == 0) {
    return;
  } else if (num_dims == 1) {
    CHECK_EQ(batch, 1);
    *out_dims = {depth};
  } else if (num_dims == 2) {
    *out_dims = {batch, depth};
  } else if (num_dims == 3) {
    CHECK_EQ(batch, 1);
    *out_dims = {height, width, depth};
  } else if (num_dims == 4) {
    *out_dims = {batch, height, width, depth};
  } else {
    LOG(FATAL) << "Should not get here: " << num_dims;
  }
}

void CreateOrCheckRnnStateArray(const string& name, int size, Model* model) {
  int batch = 1;
  int num_dims = -1;
  for (const auto& input_array : model->flags.input_arrays()) {
    // Pick 'num_dims' and 'batch' from the first input_arrays, unless we find
    // a better match by name.
    if (input_array.name() == name || num_dims == -1) {
      num_dims = input_array.shape().dims_size();
      if (num_dims > 0) {
        batch = input_array.shape().dims(0);
      }
    }
  }
  Array& array = model->GetOrCreateArray(name);
  if (array.has_shape()) {
    num_dims = array.shape().dimensions_count();
  }
  if (!array.has_shape() && num_dims >= 0) {
    Shape* shape = array.mutable_shape();
    std::vector<int> dims;
    MakeArrayDims(num_dims, batch, 1, 1, size, &dims);
    *shape->mutable_dims() = dims;
  }
}

void ResolveModelFlags(const ModelFlags& model_flags, Model* model) {
  // Merge info about input_arrays from model_flags into model->flags
  for (const auto& specified_input_array : model_flags.input_arrays()) {
    toco::InputArray* dst_input_array = nullptr;
    for (int i = 0; i < model->flags.input_arrays_size(); i++) {
      toco::InputArray* candidate_dst_input_array =
          model->flags.mutable_input_arrays(i);
      if (candidate_dst_input_array->name() == specified_input_array.name()) {
        // specified_input_array from model_flags maps to dst_input_array
        // in model->flags
        dst_input_array = candidate_dst_input_array;
        break;
      }
    }
    if (!dst_input_array) {
      // Specified_input_array from model_flags is not found in model->flags.
      // Match a name-less specified input array when there can be no ambiguity
      // as there is only 1 input array.
      if (model->flags.input_arrays_size() == 1 &&
          model_flags.input_arrays_size() == 1 &&
          !specified_input_array.has_name()) {
        dst_input_array = model->flags.mutable_input_arrays(0);
      }
    }
    if (!dst_input_array) {
      // Still no match, so create a new input array to copy
      // specified_input_array into.
      dst_input_array = model->flags.add_input_arrays();
      dst_input_array->set_name(specified_input_array.name());
    }

#define RESOLVE_MODEL_FLAG(field_name)                                       \
  if (specified_input_array.has_##field_name()) {                            \
    if (dst_input_array->has_##field_name()) {                               \
      QCHECK_EQ(dst_input_array->field_name(),                               \
                specified_input_array.field_name())                          \
          << "For input array '" << dst_input_array->name() << "', "         \
          << "specified " #field_name " flag with value: "                   \
          << specified_input_array.field_name()                              \
          << " does not agree with already defined " #field_name             \
             " of this model, with value: "                                  \
          << specified_input_array.field_name();                             \
    } else {                                                                 \
      dst_input_array->set_##field_name(specified_input_array.field_name()); \
    }                                                                        \
  }
    RESOLVE_MODEL_FLAG(std_value);
    RESOLVE_MODEL_FLAG(mean_value);
#undef RESOLVE_MODEL_FLAG

    if (specified_input_array.has_shape()) {
      if (dst_input_array->has_shape()) {
        QCHECK_EQ(specified_input_array.shape().dims_size(),
                  dst_input_array->shape().dims_size())
            << "For input array '" << specified_input_array.name() << "', "
            << "size of specified input shape flag with size: "
            << specified_input_array.shape().dims_size()
            << " does not agree with already defined input shape"
               " of this model, with size: "
            << dst_input_array->shape().dims_size();
        // We treat the first dimension as a special case, since it is often
        // a batch size and the input_shape flag is effectively overriding
        // the model.
        for (int i = 1; i < specified_input_array.shape().dims_size(); i++) {
          QCHECK_EQ(specified_input_array.shape().dims(i),
                    dst_input_array->shape().dims(i))
              << "At dimension number " << i << " of input array "
              << specified_input_array.name() << ", the specified shape's "
              << "dimension flag with dimension: "
              << specified_input_array.shape().dims(i)
              << " does not agree with already defined shape"
              << " of this model, with dimension: "
              << dst_input_array->shape().dims(i);
        }
      } else {
        *dst_input_array->mutable_shape() = specified_input_array.shape();
      }
    }

    if (specified_input_array.has_data_type()) {
      QCHECK(!dst_input_array->has_data_type());
      dst_input_array->set_data_type(specified_input_array.data_type());
    }
  }

  if (model_flags.output_arrays_size() > 0) {
    model->flags.mutable_output_arrays()->CopyFrom(model_flags.output_arrays());
  }

#define RESOLVE_MODEL_FLAG(name)                                           \
  if (model_flags.has_##name()) {                                          \
    if (model->flags.has_##name()) {                                       \
      QCHECK_EQ(model_flags.name(), model->flags.name())                   \
          << "Specified " #name " flag with value: " << model_flags.name() \
          << " does not agree with already defined " #name                 \
             " of this model, with value: "                                \
          << model->flags.name();                                          \
    } else {                                                               \
      model->flags.set_##name(model_flags.name());                         \
    }                                                                      \
  }

  RESOLVE_MODEL_FLAG(variable_batch)

#undef RESOLVE_MODEL_FLAG

  if (!model_flags.rnn_states().empty()) {
    model->flags.mutable_rnn_states()->CopyFrom(model_flags.rnn_states());
  }

  if (model->flags.model_checks_size() == 0) {
    model->flags.mutable_model_checks()->CopyFrom(model_flags.model_checks());
  }

  QCHECK_GT(model->flags.output_arrays_size(), 0)
      << "This model does not define output arrays, so a "
         "--output_arrays flag must be given on the command-line.";

  for (const auto& input_array_proto : model->flags.input_arrays()) {
    auto& input_array = model->GetOrCreateArray(input_array_proto.name());
    if (input_array_proto.has_data_type()) {
      const ArrayDataType specified_type =
          ConvertIODataTypeToArrayDataType(input_array_proto.data_type());
      QCHECK(specified_type != ArrayDataType::kNone);
      if (input_array.data_type != ArrayDataType::kNone) {
        QCHECK(specified_type == input_array.data_type)
            << "For input array " << input_array_proto.name()
            << " the specified input data type "
            << IODataType_Name(input_array_proto.data_type())
            << " conflicts with the existing type.";
      }
      input_array.data_type = specified_type;
    }

    if (input_array.data_type == ArrayDataType::kNone) {
      // We start out with a float input array;
      // that may get replaced by a uint8 array later, by
      // MakeInitialDequantizeOp.
      input_array.data_type = ArrayDataType::kFloat;
    }

    // Compare/merge the model->flags describing the input_shape with
    // the actual input array's shape.
    if (!input_array.has_shape()) {
      if (input_array_proto.has_shape()) {
        auto& input_array_dims = *input_array.mutable_shape()->mutable_dims();
        for (auto dim : input_array_proto.shape().dims()) {
          CHECK_GE(dim, 1);
          input_array_dims.push_back(dim);
        }
      }
    } else {
      if (input_array_proto.has_shape()) {
        // If an input shape was specified on the flags ensure that it matches
        // the actual shape in the model.
        const auto& input_array_dims =
            *input_array.mutable_shape()->mutable_dims();
        CHECK_EQ(input_array_dims.size(),
                 input_array_proto.shape().dims_size());
        for (int i = 0; i < input_array_dims.size(); i++) {
          CHECK_EQ(input_array_dims[i], input_array_proto.shape().dims(i));
        }
      }
    }

    const float mean_value = input_array_proto.mean_value();
    const float std_value = input_array_proto.std_value();
    MinMax input_minmax;
    input_minmax.min = (0.f - mean_value) / std_value;
    input_minmax.max = (255.f - mean_value) / std_value;
    if (input_array.minmax) {
      if (input_array_proto.has_mean_value() ||
          input_array_proto.has_std_value()) {
        CHECK(input_minmax == *input_array.minmax)
            << input_minmax.min << ", " << input_minmax.max
            << " != " << input_array.minmax->min << ", "
            << input_array.minmax->max;
      }
    } else {
      input_array.GetOrCreateMinMax() = input_minmax;
    }
  }
  // Creation of the RNN state arrays
  for (const auto& rnn_state : model->flags.rnn_states()) {
    CreateOrCheckRnnStateArray(rnn_state.state_array(), rnn_state.size(),
                               model);
  }

  for (const auto& input_array : model->flags.input_arrays()) {
    if (input_array.has_shape()) {
      CHECK(input_array.shape().dims_size());
    }
  }

  model->flags.set_allow_nonascii_arrays(model_flags.allow_nonascii_arrays());
  model->flags.set_allow_nonexistent_arrays(
      model_flags.allow_nonexistent_arrays());

  CHECK(!model->flags.has_arrays_extra_info());
  *model->flags.mutable_arrays_extra_info() = model_flags.arrays_extra_info();
}

void CheckIsReadyForQuantization(const Model& model) {
  for (const auto& op : model.operators) {
    for (const auto& input : op->inputs) {
      const auto& input_array = model.GetArray(input);
      if (input_array.data_type != ArrayDataType::kFloat) {
        // The array is not floats, no quantization needed.
        continue;
      }
      if (input_array.minmax) {
        // The array has minmax, we're good.
        continue;
      }
      if (input_array.buffer) {
        // The array has a constant buffer, so we can
        // fall back to computing the minmax from actual array entries
        // (with a WARNING about possible accuracy implications).
        continue;
      }
      LOG(FATAL)
          << "Array " << input << ", which is an input to the "
          << HelpfulOperatorTypeName(*op) << " operator producing the output "
          << "array " << op->outputs[0] << ", is lacking min/max data, "
          << "which is necessary for quantization. Either target a "
          << "non-quantized output format, or change the input graph to "
          << "contain min/max information, or pass --default_ranges_min= and "
          << "--default_ranges_max= if you do not care about the accuracy of "
          << "results.";
    }
  }
}

void UseDefaultMinMaxRangeValues(Model* model, double default_ranges_min,
                                 double default_ranges_max) {
  for (const auto& op : model->operators) {
    for (const auto& input : op->inputs) {
      auto& input_array = model->GetArray(input);
      if (!input_array.minmax && !input_array.buffer) {
        auto& minmax = input_array.GetOrCreateMinMax();
        minmax.min = default_ranges_min;
        minmax.max = default_ranges_max;
      }
    }
    for (const auto& output : op->outputs) {
      auto& output_array = model->GetArray(output);
      if (!output_array.minmax && !output_array.buffer) {
        auto& minmax = output_array.GetOrCreateMinMax();
        minmax.min = default_ranges_min;
        minmax.max = default_ranges_max;
      }
    }
  }
}

int ElementSize(ArrayDataType data_type) {
  switch (data_type) {
    case ArrayDataType::kFloat:
      return 4;
    case ArrayDataType::kInt8:
      return 1;
    case ArrayDataType::kUint8:
      return 1;
    case ArrayDataType::kInt16:
      return 2;
    case ArrayDataType::kUint16:
      return 2;
    case ArrayDataType::kInt32:
      return 4;
    case ArrayDataType::kUint32:
      return 4;
    case ArrayDataType::kInt64:
      return 8;
    case ArrayDataType::kUint64:
      return 8;

    // Usually not critical limitation because strings are only input and/or
    // output.
    case ArrayDataType::kString:
      LOG(FATAL) << "Transient arrays with strings are not supported yet";
      return 0;
    default:
      LOG(FATAL) << "Should not get here.";
      return 0;
  }
}

void DropMinMax(Model* model, const string& array_name) {
  auto& array = model->GetArray(array_name);
  if (!!array.minmax) {
    LOG(WARNING) << "Dropping MinMax information in array " << array_name
                 << ". Expect inaccuracy in quantized inference.";
    array.minmax = nullptr;
  }
}

bool IsAllocatableTransientArray(const Model& model, const string& array_name) {
  // Optional array is not transient
  if (model.IsOptionalArray(array_name)) return false;
  // The model's input and output arrays are externally allocated.
  // They are not transient arrays.
  if (IsInputArray(model, array_name)) {
    return false;
  }
  for (const string& output_array : model.flags.output_arrays()) {
    if (array_name == output_array) {
      return false;
    }
  }
  const auto& array = &model.GetArray(array_name);
  // An array with a constant buffer isn't a transient array.
  if (!!array->buffer) {
    return false;
  }
  // An array without shape isn't allocatable.
  if (!array->has_shape()) {
    return false;
  }
  return true;
}

string AvailableArrayName(const Model& model, const string& name) {
  string sanitized_name = SanitizeNameForTFNode(name);
  if (!model.HasArray(sanitized_name) &&
      !model.IsOptionalArray(sanitized_name)) {
    return sanitized_name;
  }
  const int kNumSuffixesToTry = 1000;
  for (int i = 0; i < kNumSuffixesToTry; i++) {
    const string& name_with_suffix =
        toco::port::StringF("%s_%d", sanitized_name, i);
    if (!model.HasArray(name_with_suffix) &&
        !model.IsOptionalArray(name_with_suffix)) {
      return name_with_suffix;
    }
  }
  LOG(FATAL) << "Could not find an available array name starting with "
             << sanitized_name << ". Tried " << kNumSuffixesToTry
             << " suffixes, all were taken!";
  return "";
}

string ShapeToString(const Shape& shape) {
  if (shape.dimensions_count() == 0) {
    return "[]";
  }

  return absl::StrCat("[ ", absl::StrJoin(shape.dims(), ", "), " ]");
}

void PrintArrayShape(Model* model, const string& name) {
  if (!model->GetArray(name).has_shape()) {
    LOG(INFO) << name << " has no shape";
    return;
  }
  LOG(INFO) << name
            << " has shape: " << ShapeToString(model->GetArray(name).shape());
}

bool IsArrayFullyConnectedWeights(const Model& model, const string& name) {
  bool is_fc_weights = false;
  bool is_something_else = false;
  for (const auto& op : model.operators) {
    for (int input_index = 0; input_index < op->inputs.size(); input_index++) {
      if (op->inputs[input_index] == name) {
        if (op->type == OperatorType::kFullyConnected && input_index == 1) {
          is_fc_weights = true;
        } else {
          is_something_else = true;
        }
      }
    }
  }
  CHECK(!(is_fc_weights && is_something_else));
  return is_fc_weights;
}

string CreateInt32Array(Model* model, const string& param_name,
                        const std::vector<int>& value) {
  auto param_array_name = AvailableArrayName(*model, param_name);
  auto& param_array = model->GetOrCreateArray(param_array_name);
  param_array.mutable_shape()->ReplaceDims({static_cast<int>(value.size())});
  param_array.data_type = ArrayDataType::kInt32;
  auto& param_array_data =
      param_array.GetMutableBuffer<ArrayDataType::kInt32>().data;
  param_array_data.resize(RequiredBufferSizeForShape(param_array.shape()));
  for (int i = 0; i < value.size(); ++i) {
    param_array_data[i] = value[i];
  }
  return param_array_name;
}

bool EstimateArithmeticOpsCount(const Model& model, int64* result) {
  int64 total = 0;
  for (const auto& op : model.operators) {
    switch (op->type) {
      case OperatorType::kFullyConnected:
      case OperatorType::kConv:
      case OperatorType::kDepthwiseConv: {
        const auto& output_array = model.GetArray(op->outputs[0]);
        const auto& weights_array = model.GetArray(op->inputs[1]);
        if (!output_array.has_shape() || !weights_array.has_shape()) {
          return false;
        }
        int cols = 1;
        for (int i = 0; i < output_array.shape().dimensions_count() - 1; i++) {
          cols *= output_array.shape().dims(i);
        }
        const int64 cost_per_col =
            2 * RequiredBufferSizeForShape(weights_array.shape());
        total += cost_per_col * cols;
        if (op->inputs.size() > 2) {
          // There is a bias vector. One more op per output value.
          total += RequiredBufferSizeForShape(output_array.shape());
        }
        break;
      }
      case OperatorType::kAdd:
      case OperatorType::kSub:
      case OperatorType::kMul: {
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        total += RequiredBufferSizeForShape(output_array.shape());
        break;
      }
      case OperatorType::kAddN: {
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        // AddN cost is roughly the same cost as N-1 Adds.
        const int num_adds = op->inputs.size() - 1;
        total += num_adds * RequiredBufferSizeForShape(output_array.shape());
        break;
      }
      case OperatorType::kLogistic:
      case OperatorType::kSoftmax:
      case OperatorType::kLogSoftmax:
      case OperatorType::kTanh: {
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        // As a very rough ballpark, the cost of evaluating a math function
        // such as tanh or logistic is about 32 multiplications, and about as
        // many additions/subtractions. (Just a power-of-two order-of-magnitude
        // from looking at actual implementations that we use in runtime/ code).
        total += 64 * RequiredBufferSizeForShape(output_array.shape());
        break;
      }
      case OperatorType::kMaxPool: {
        const auto& maxpool = *static_cast<const MaxPoolOperator*>(op.get());
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        total += RequiredBufferSizeForShape(output_array.shape()) *
                 maxpool.kheight * maxpool.kwidth;
        break;
      }
      case OperatorType::kAveragePool: {
        const auto& avgpool =
            *static_cast<const AveragePoolOperator*>(op.get());
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        total += RequiredBufferSizeForShape(output_array.shape()) *
                 avgpool.kheight * avgpool.kwidth;
        break;
      }
      case OperatorType::kL2Pool: {
        const auto* maxpool = static_cast<const MaxPoolOperator*>(op.get());
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        // The sum of squares requires (kheight*kwidth) multiply-adds,
        // and then there is the sqrt which we ballpark at 32 ops.
        const int64 cost_per_val = 2 * maxpool->kheight * maxpool->kwidth + 32;
        total +=
            RequiredBufferSizeForShape(output_array.shape()) * cost_per_val;
        break;
      }
      case OperatorType::kL2Normalization: {
        const auto& output_array = model.GetArray(op->outputs[0]);
        if (!output_array.has_shape()) {
          return false;
        }
        // Computing the squared L2 norm is N multiply-adds so 2N ops,
        // then the single inverse-sqrt is negligible, then we multiply each
        // value by the resulting multiplier, so an extra N ops. Total 3N ops.
        total += 3 * RequiredBufferSizeForShape(output_array.shape());
        break;
      }
      default:
        break;
    }
  }
  *result = total;
  return true;
}

void GetShuffleShape(AxesOrder input_axes_order, AxesOrder output_axes_order,
                     std::vector<int>* shuffle) {
  CHECK_EQ(AxesCount(input_axes_order), AxesCount(output_axes_order));
  shuffle->resize(4);
  for (int i = 0; i < 4; i++) {
    (*shuffle)[i] = i;
  }
  if (input_axes_order == output_axes_order) {
    // nothing to do
  } else if (AxesCount(input_axes_order) == 2) {
    shuffle->resize(2);
    (*shuffle)[0] = 1;
    (*shuffle)[1] = 0;
  } else if (input_axes_order == AxesOrder::kOHWI &&
             output_axes_order == AxesOrder::kHWIO) {
    // 3210 <- 3210
    // HWIO <- OHWI
    (*shuffle)[0] = 1;
    (*shuffle)[1] = 2;
    (*shuffle)[2] = 3;
    (*shuffle)[3] = 0;
  } else if (input_axes_order == AxesOrder::kHWIO &&
             output_axes_order == AxesOrder::kOHWI) {
    // 3210 <- 3210
    // OHWI <- HWIO
    (*shuffle)[0] = 3;
    (*shuffle)[1] = 0;
    (*shuffle)[2] = 1;
    (*shuffle)[3] = 2;
  } else {
    LOG(FATAL) << "Bad shuffle";
  }
}

void ExtendShuffle(const std::vector<int>& input_shuffle, int newdim,
                   std::vector<int>* extended_shuffle) {
  *extended_shuffle = input_shuffle;
  CHECK(newdim >= input_shuffle.size());
  const int pad_size = newdim - input_shuffle.size();
  extended_shuffle->resize(newdim);
  for (int i = 0; i < pad_size; i++) {
    (*extended_shuffle)[i] = i;
  }
  for (int i = pad_size; i < newdim; i++) {
    (*extended_shuffle)[i] = input_shuffle[i - pad_size] + pad_size;
  }
}

void ShuffleDims(const Shape& input_shape, AxesOrder input_axes_order,
                 AxesOrder output_axes_order, Shape* output_shape) {
  if (input_axes_order == AxesOrder::kHWIM &&
      output_axes_order == AxesOrder::k1HWO) {
    // This special case isn't just a permutation, the IM pair of dims get
    // merged into the 3 dim, so we have to special-case it.
    *output_shape = Shape({1, input_shape.dims(0), input_shape.dims(1),
                           input_shape.dims(3) * input_shape.dims(2)});
  } else {
    std::vector<int> shuffle;
    GetShuffleShape(input_axes_order, output_axes_order, &shuffle);
    std::vector<int>* output_dims = output_shape->mutable_dims();
    output_dims->resize(input_shape.dimensions_count());
    for (int i = 0; i < input_shape.dimensions_count(); i++) {
      (*output_dims)[i] = input_shape.dims(shuffle[i]);
    }
  }
}

template <typename T>
void ShuffleArrayTemplate(const Shape& input_shape, AxesOrder input_axes_order,
                          AxesOrder output_axes_order,
                          const Shape& output_shape, const T* input_data,
                          T* output_data) {
  if (input_axes_order == AxesOrder::kHWIM &&
      output_axes_order == AxesOrder::k1HWO) {
    // This special case isn't just a permutation, the IM pair of dims get
    // merged into the O dim, so we have to special-case it. Fortunately,
    // as far as array shuffling is concerned, it's just the identity
    // transformation.
    memcpy(output_data, input_data,
           RequiredBufferSizeForShape(input_shape) * sizeof(output_data[0]));
    return;
  }
  CHECK(input_shape.dimensions_count() == output_shape.dimensions_count());
  const int dim = input_shape.dimensions_count();
  CHECK_LE(dim, 4);
  std::vector<int> shuffle;
  GetShuffleShape(input_axes_order, output_axes_order, &shuffle);
  CHECK(shuffle.size() >= dim);
  for (int i = 0; i < dim; i++) {
    CHECK(shuffle[i] >= 0 && shuffle[i] < dim);
    CHECK(input_shape.dims(shuffle[i]) == output_shape.dims(i));
  }
  Shape extended_input_shape = input_shape;
  ExtendShape(&extended_input_shape, 4);
  Shape extended_output_shape = output_shape;
  ExtendShape(&extended_output_shape, 4);
  std::vector<int> extended_shuffle;
  ExtendShuffle(shuffle, 4, &extended_shuffle);

  const std::vector<int>& extended_input_dims = extended_input_shape.dims();
  const std::vector<int>& extended_output_dims = extended_output_shape.dims();

  // TODO(starka): Rework to handle different numbers of dimensions.
  int input_strides[4];
  input_strides[3] = 1;
  input_strides[2] = extended_input_dims[3];
  input_strides[1] = input_strides[2] * extended_input_dims[2];
  input_strides[0] = input_strides[1] * extended_input_dims[1];
  const int input_stride_0 = input_strides[extended_shuffle[3]];
  const int input_stride_1 = input_strides[extended_shuffle[2]];
  const int input_stride_2 = input_strides[extended_shuffle[1]];
  const int input_stride_3 = input_strides[extended_shuffle[0]];

  const int output_size_0 = extended_output_dims[3];
  const int output_size_1 = extended_output_dims[2];
  const int output_size_2 = extended_output_dims[1];
  const int output_size_3 = extended_output_dims[0];
  const int output_stride_0 = 1;
  const int output_stride_1 = output_size_0;
  const int output_stride_2 = output_stride_1 * output_size_1;
  const int output_stride_3 = output_stride_2 * output_size_2;

  for (int i3 = 0; i3 < output_size_3; i3++) {
    const T* const input_ptr_3 = input_data + i3 * input_stride_3;
    T* const output_ptr_3 = output_data + i3 * output_stride_3;
    for (int i2 = 0; i2 < output_size_2; i2++) {
      const T* const input_ptr_2 = input_ptr_3 + i2 * input_stride_2;
      T* const output_ptr_2 = output_ptr_3 + i2 * output_stride_2;
      for (int i1 = 0; i1 < output_size_1; i1++) {
        const T* input_ptr = input_ptr_2 + i1 * input_stride_1;
        T* output_ptr = output_ptr_2 + i1 * output_stride_1;
        T* const output_ptr_end = output_ptr + output_size_0 * output_stride_0;
        while (output_ptr != output_ptr_end) {
          *output_ptr = *input_ptr;
          input_ptr += input_stride_0;
          output_ptr += output_stride_0;
        }
      }
    }
  }
}

void ShuffleArray(const Shape& input_shape, AxesOrder input_axes_order,
                  AxesOrder output_axes_order, const Shape& output_shape,
                  const uint8* input_data, uint8* output_data) {
  ShuffleArrayTemplate<uint8>(input_shape, input_axes_order, output_axes_order,
                              output_shape, input_data, output_data);
}

void ShuffleArray(const Shape& input_shape, AxesOrder input_axes_order,
                  AxesOrder output_axes_order, const Shape& output_shape,
                  const float* input_data, float* output_data) {
  ShuffleArrayTemplate<float>(input_shape, input_axes_order, output_axes_order,
                              output_shape, input_data, output_data);
}

int AxesCount(AxesOrder axes_order) {
  switch (axes_order) {
    case AxesOrder::kOneAxis:
      return 1;
    case AxesOrder::kRC:
      return 2;
    case AxesOrder::kCR:
      return 2;
    case AxesOrder::kHWIO:
      return 4;
    case AxesOrder::kOHWI:
      return 4;
    case AxesOrder::kHWIM:
      return 4;
    case AxesOrder::k1HWO:
      return 4;
    case AxesOrder::kNHWC:
      return 4;
    default:
      LOG(FATAL) << "Bad AxesOrder";
      return 0;
  }
}

bool IsDiscardableArray(const Model& model, const string& array_name) {
  for (const auto& input_array : model.flags.input_arrays()) {
    if (array_name == input_array.name()) {
      return false;
    }
  }
  for (const string& output_array : model.flags.output_arrays()) {
    if (array_name == output_array) {
      return false;
    }
  }
  for (const auto& rnn_state : model.flags.rnn_states()) {
    if (!rnn_state.discardable()) {
      if (array_name == rnn_state.state_array()) {
        return false;
      }
      if (array_name == rnn_state.back_edge_source_array()) {
        return false;
      }
    }
  }
  return true;
}

void CheckFinalDataTypesSatisfied(const Model& model) {
  for (const auto& array_entry : model.GetArrayMap()) {
    const auto& array = *array_entry.second;
    if (array.final_data_type != ArrayDataType::kNone) {
      CHECK(array.final_data_type == array.data_type)
          << "Array \"" << array_entry.first
          << "\" has mis-matching actual and final data types ("
          << static_cast<int>(array.data_type) << ","
          << static_cast<int>(array.final_data_type) << ").";
    }
  }
}

ArrayDataType ConvertIODataTypeToArrayDataType(IODataType type) {
  switch (type) {
    case FLOAT:
      return ArrayDataType::kFloat;
    case QUANTIZED_UINT8:
      return ArrayDataType::kUint8;
    case INT32:
      return ArrayDataType::kInt32;
    case INT64:
      return ArrayDataType::kInt64;
    default:
      return ArrayDataType::kNone;
  }
}

void FinishBuildingRNNStates(Model* model) {
  for (const auto& rnn_state : model->flags.rnn_states()) {
    if (!model->HasArray(rnn_state.back_edge_source_array()) ||
        !model->HasArray(rnn_state.state_array())) {
      CHECK(model->HasArray(rnn_state.back_edge_source_array()));
      CHECK(model->HasArray(rnn_state.state_array()));
      continue;
    }
    const auto& src_array = model->GetArray(rnn_state.back_edge_source_array());
    auto& dst_array = model->GetArray(rnn_state.state_array());
    if (src_array.data_type == ArrayDataType::kNone &&
        dst_array.data_type == ArrayDataType::kNone) {
      dst_array.data_type = ArrayDataType::kFloat;
    }
  }
}

void UseArraysExtraInfo(Model* model) {
  for (const auto& entry : model->flags.arrays_extra_info().entries()) {
    QCHECK(model->HasArray(entry.name()))
        << "ArraysExtraInfo refers to non-existent array name: "
        << entry.name();
    auto& minmax = model->GetArray(entry.name()).GetOrCreateMinMax();
    minmax.min = entry.min();
    minmax.max = entry.max();
  }
}

}  // namespace toco
