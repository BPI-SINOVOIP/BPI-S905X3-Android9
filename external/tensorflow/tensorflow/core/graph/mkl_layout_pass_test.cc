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

#ifdef INTEL_MKL

#include "tensorflow/core/graph/mkl_layout_pass.h"
#include "tensorflow/core/graph/mkl_graph_util.h"

#include <algorithm>
#include <string>
#include <vector>

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/graph_constructor.h"
#include "tensorflow/core/graph/testlib.h"
#include "tensorflow/core/kernels/ops_util.h"
#include "tensorflow/core/lib/random/simple_philox.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/test_benchmark.h"

namespace tensorflow {

#ifdef INTEL_MKL_ML

namespace {

const char kCPUDevice[] = "/job:a/replica:0/task:0/device:CPU:0";
const char kGPUDevice[] = "/job:a/replica:0/task:0/device:GPU:0";

static void InitGraph(const string& s, Graph* graph,
                      const string& device = kCPUDevice) {
  GraphDef graph_def;

  auto parser = protobuf::TextFormat::Parser();
  //  parser.AllowRelaxedWhitespace(true);
  CHECK(parser.MergeFromString(s, &graph_def)) << s;
  GraphConstructorOptions opts;
  TF_CHECK_OK(ConvertGraphDefToGraph(opts, graph_def, graph));

  for (Node* node : graph->nodes()) {
    node->set_assigned_device_name(device);
  }
}

class MklLayoutPassTest : public ::testing::Test {
 public:
  MklLayoutPassTest() : graph_(OpRegistry::Global()) {}

  void InitGraph(const string& s, const string& device = kCPUDevice) {
    ::tensorflow::InitGraph(s, &graph_, device);
    original_ = CanonicalGraphString(&graph_);
  }

  static bool IncludeNode(const Node* n) { return n->IsOp(); }

  static string EdgeId(const Node* n, int index) {
    if (index == 0) {
      return n->name();
    } else if (index == Graph::kControlSlot) {
      return strings::StrCat(n->name(), ":control");
    } else {
      return strings::StrCat(n->name(), ":", index);
    }
  }

  string CanonicalGraphString(Graph* g) {
    std::vector<string> nodes;
    std::vector<string> edges;
    for (const Node* n : g->nodes()) {
      if (IncludeNode(n)) {
        nodes.push_back(strings::StrCat(n->name(), "(", n->type_string(), ")"));
      }
    }
    for (const Edge* e : g->edges()) {
      if (IncludeNode(e->src()) && IncludeNode(e->dst())) {
        edges.push_back(strings::StrCat(EdgeId(e->src(), e->src_output()), "->",
                                        EdgeId(e->dst(), e->dst_input())));
      }
    }
    // Canonicalize
    std::sort(nodes.begin(), nodes.end());
    std::sort(edges.begin(), edges.end());
    return strings::StrCat(str_util::Join(nodes, ";"), "|",
                           str_util::Join(edges, ";"));
  }

  string DoMklLayoutOptimizationPass() {
    string before = CanonicalGraphString(&graph_);
    LOG(ERROR) << "Before MKL layout rewrite pass: " << before;

    std::unique_ptr<Graph>* ug = new std::unique_ptr<Graph>(&graph_);
    RunMklLayoutRewritePass(ug);

    string result = CanonicalGraphString(&graph_);
    LOG(ERROR) << "After MKL layout rewrite pass:  " << result;
    return result;
  }

  const string& OriginalGraph() const { return original_; }

  Graph graph_;
  string original_;
};

REGISTER_OP("Input").Output("o: float").SetIsStateful();
REGISTER_OP("InputList").Output("o: N * float").Attr("N: int").SetIsStateful();
REGISTER_OP("HalfInput").Output("o: half").SetIsStateful();
REGISTER_OP("Int32Input").Output("o: int32").SetIsStateful();
REGISTER_OP("_MklInput").Output("o: uint8").SetIsStateful();
REGISTER_OP("_MklInput2")
    .Output("o: uint8")
    .Output("o1: uint8")
    .SetIsStateful();

/////////////////////////////////////////////////////////////////////
//  Unit tests related to node merge optiimization
/////////////////////////////////////////////////////////////////////

TEST_F(MklLayoutPassTest, Basic) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Zeta);D(Zeta)|"
            "A->C;A->D;B->C:1;B->D:1");
}

// Test set 1: Conv2D + AddBias

// C=_MklConv2D(A,M,B,N); E=BiasAdd(C,D); Z=Zeta(E,Y) (for interleaved ordering)
// C=_MklConv2D(A,B,M,N); E=BiasAdd(C,D); Z=Zeta(E,Y) (for contiguous ordering)
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Positive) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);D(Input);DMT/_0(Const);E(_MklConv2DWithBias);"
            "M(_MklInput);N(_MklInput);Y(Input);Z(Zeta)|A->E;"
            "A:control->DMT/_0:control;B->E:1;D->E:2;DMT/_0->E:5;E->Z;M->E:3;"
            "N->E:4;Y->Z:1");
}

// C=_MklConv2D(A,M:1,B,N:1); E=BiasAdd(C,D); Z=Zeta(E,Y) (for interleaved)
// C=_MklConv2D(A,B,M:1,N:1); E=BiasAdd(C,D); Z=Zeta(E,Y) (for contiguous)
// Test for correct output slots selected
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Positive1) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput2'}"
      "node { name: 'N' op: '_MklInput2'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M:1', 'N:1']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);D(Input);DMT/_0(Const);E(_MklConv2DWithBias);"
            "M(_MklInput2);N(_MklInput2);Y(Input);Z(Zeta)|A->E;"
            "A:control->DMT/_0:control;B->E:1;D->E:2;DMT/_0->E:5;E->Z;"
            "M:1->E:3;N:1->E:4;Y->Z:1");
}

// C=Conv2D(A,B); E=BiasAdd(C,D); Z=Zeta(E,Y);
// This is a case of node rewrite followed by node merge.
// We will first rewrite Conv2D to _MklConv2D, and then merge _MklConv2D
// with BiasAdd to produce _MklConv2DWithBias.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Positive2) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);E(_MklConv2DWithBias);Y(Input);Z(Zeta)|"
            "A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->E:1;D->E:2;DMT/_0->E:3;DMT/_1->E:4;"
            "DMT/_2->E:5;E->Z;Y->Z:1");
}

// Graph contains only _MklConv2D, no AddBias.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_NoAddBias) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);M(_MklInput);N(_MklInput)|"
            "A->C;B->C:1;M->C:2;N->C:3");
}

// _MklConv2D output does not go to BiasAdd.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_Dataflow1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D', 'E'] }");  // Output of _MklConv2D does not go to BiasAdd.
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);E(Input);F(BiasAdd);"
            "M(_MklInput);N(_MklInput)|A->C;B->C:1;D->F;E->F:1;M->C:2;N->C:3");
}

// _MklConv2D has two outgoing edges: BiasAdd and some other dummy node (Zeta).
// Merge should not be done in such case.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_Dataflow2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D', 'E'] }"  // Conv2D has two outputs.
                              // No merge should happen.
      "node { name: 'G' op: 'Zeta'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);E(Input);F(BiasAdd);"
            "G(Zeta);M(_MklInput);N(_MklInput)|A->C;B->C:1;C->G;D->F;"
            "E->F:1;E->G:1;M->C:2;N->C:3");
}

// data_format attribute value mismatch. Merge should not be done
// in such case.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_AttrMismatch) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NHCW' } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);E(BiasAdd);M(_MklInput);"
            "N(_MklInput)|A->C;B->C:1;C->E;D->E:1;M->C:2;N->C:3");
}

// Test set 2: _MklConv2D..BiasAddGrad -> _MklConv2DWithBiasBackpropBias
// rewrite tests

// BiasAddGrad rewrite to BackpropBias in the presence of BackpropFilter
// and BackpropInput
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'F', 'E', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'Int32Input'}"
      "node { name: 'I' op: '_MklConv2DBackpropInput'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['H', 'B', 'E', 'M', 'N', 'O']}"
      "node { name: 'J' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);DMT/_0(Const);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);H(Int32Input);"
            "I(_MklConv2DBackpropInput);J(_MklConv2DWithBiasBackpropBias);"
            "M(_MklInput);N(_MklInput);O(_MklInput)|A->D;A->E:1;A->G;B->D:1;"
            "B->I:1;C->D:2;D->E;DMT/_0->J:1;E->G:2;E->I:2;E->J;"
            "E:control->DMT/_0:control;F->G:1;H->I;M->D:3;M->G:3;M->I:3;"
            "N->D:4;N->G:4;N->I:4;O->D:5;O->G:5;O->I:5");
}

// BiasAddGrad rewrite to BackpropBias in the presence of BackpropFilter
// and BackpropInput. But nodes do not match criteria for rewrite. So
// rewrite should not happen.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['E', 'F', 'A', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'Int32Input'}"
      "node { name: 'I' op: '_MklConv2DBackpropInput'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['H', 'B', 'E', 'M', 'N', 'O']}"
      "node { name: 'J' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);H(Int32Input);"
            "I(_MklConv2DBackpropInput);J(BiasAddGrad);"
            "M(_MklInput);N(_MklInput);O(_MklInput)|A->D;A->E:1;A->G:2;B->D:1;"
            "B->I:1;C->D:2;D->E;E->G;E->I:2;E->J;F->G:1;H->I;M->D:3;M->G:3;"
            "M->I:3;N->D:4;N->G:4;N->I:4;O->D:5;O->G:5;O->I:5");
}

// BiasAddGrad rewrite to BackpropBias in the presence of BackpropFilter
// and BackpropInput. But nodes do not match criteria for rewrite. So
// rewrite should not happen.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['B', 'A', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'F', 'E', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'Int32Input'}"
      "node { name: 'I' op: '_MklConv2DBackpropInput'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['H', 'B', 'E', 'M', 'N', 'O']}"
      "node { name: 'J' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);H(Int32Input);"
            "I(_MklConv2DBackpropInput);J(BiasAddGrad);"
            "M(_MklInput);N(_MklInput);O(_MklInput)|A->D:1;A->E:1;A->G;B->D;"
            "B->I:1;C->D:2;D->E;E->G:2;E->I:2;E->J;F->G:1;H->I;M->D:3;M->G:3;"
            "M->I:3;N->D:4;N->G:4;N->I:4;O->D:5;O->G:5;O->I:5");
}

// BiasAddGrad rewrite to BackpropBias in the presence of BackpropFilter only
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_BpropFilter_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'F', 'E', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);DMT/_0(Const);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);"
            "H(_MklConv2DWithBiasBackpropBias);M(_MklInput);N(_MklInput);"
            "O(_MklInput)|A->D;A->E:1;A->G;B->D:1;C->D:2;D->E;DMT/_0->H:1;"
            "E->G:2;E->H;E:control->DMT/_0:control;F->G:1;M->D:3;M->G:3;"
            "N->D:4;N->G:4;O->D:5;O->G:5");
}

// BiasAddGrad rewrite to BackpropBias in the presence of BackpropFilter only
// But BackpropFilter node inputs do not satisfy criteria for rewrite.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_BpropFilter_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['E', 'F', 'A', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);H(BiasAddGrad);"
            "M(_MklInput);N(_MklInput);O(_MklInput)|A->D;A->E:1;A->G:2;B->D:1;"
            "C->D:2;D->E;E->G;E->H;F->G:1;M->D:3;M->G:3;N->D:4;N->G:4;O->D:5;"
            "O->G:5");
}

// BiasAddGrad rewrite to BackpropBias in the presence of BackpropFilter only
// But BackpropFilter node inputs do not satisfy criteria for rewrite.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_BpropFilter_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['B', 'A', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'F', 'E', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);H(BiasAddGrad);"
            "M(_MklInput);N(_MklInput);O(_MklInput)|A->D:1;A->E:1;A->G;B->D;"
            "C->D:2;D->E;E->G:2;E->H;F->G:1;M->D:3;M->G:3;N->D:4;N->G:4;O->D:5;"
            "O->G:5");
}

// No _MklConv2DWithBias in context, but _MklConv2D in context.
// No rewrite for BiasAddGrad should happen.
// C=_MklConv2D(A,M,B,N); D=Zeta(C,A); E=BiasAddGrad(D) (for interleaved)
// C=_MklConv2D(A,B,M,N); D=Zeta(C,A); E=BiasAddGrad(D) (for contiguous)
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_Neg_NoMklConv2DWithBias) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Zeta);E(BiasAddGrad);"
            "M(_MklInput);N(_MklInput)|A->C;A->D:1;B->C:1;C->D;D->E;"
            "M->C:2;N->C:3");
}

// No Conv2D in the context for BiasAddGrad. No rewrite should happen.
// C=Polygamma(A,B); D=Zeta(C,A); E=BiasAddGrad(D)
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_Negative_NoConv2D) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Polygamma'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Polygamma);D(Zeta);E(BiasAddGrad)|"
            "A->C;A->D:1;B->C:1;C->D;D->E");
}

// No Conv2D in the context for BiasAddGrad, but MatMul in context.
// Rewrite should happen, but name of BiasAddGrad does not change.
// C=MatMul(A,B); D=Zeta(C,A); E=BiasAddGrad(D)
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_Negative_NoConv2D_MatMul) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(MatMul);D(Zeta);E(BiasAddGrad)|"
            "A->C;A->D:1;B->C:1;C->D;D->E");
}

// Test set 3: MatMul..BiasAddGrad -> BiasAddGrad rewrite tests
// C=MatMul(A,B); D=Zeta(C,A); E=BiasAddGrad(D)
TEST_F(MklLayoutPassTest, NodeMerge_MatMulBiasAddGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(MatMul);D(Zeta);E(BiasAddGrad)|"
            "A->C;A->D:1;B->C:1;C->D;D->E");
}

// No MatMul in the context for BiasAddGrad. No rewrite should happen.
// C=Polygamma(A,B); D=Zeta(C,A); E=BiasAddGrad(D)
TEST_F(MklLayoutPassTest, NodeMerge_MatMulBiasAddGrad_Negative_NoMatMul) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Polygamma'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Polygamma);D(Zeta);E(BiasAddGrad)|"
            "A->C;A->D:1;B->C:1;C->D;D->E");
}

/////////////////////////////////////////////////////////////////////
//  Unit tests related to rewriting node to Mkl node
/////////////////////////////////////////////////////////////////////

// Single Conv2D Op; No Mkl layer on the input and on the output.
// We will generate dummy Mkl tensor as 2nd input of Conv2D.
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_Basic) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['B', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->C;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;B->D;C->D:1;DMT/_0->C:2;"
            "DMT/_1->C:3");
}

// 2 Conv2D Ops in sequence. Both should get transformed and 1st Conv2D will
// have 2 outputs, both of which will be inputs to next Conv2D.
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_Positive1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(_MklConv2D);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);E(Zeta)|A->C;A->D;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->C:1;C->D:1;C->E;"
            "C:2->D:3;D->E:1;DMT/_0->C:2;DMT/_1->C:3;DMT/_2->D:2");
}

// Conv2D with INT32 which is not supported by Mkl
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_Negative_UnsupportedType) {
  InitGraph(
      "node { name: 'A' op: 'HalfInput'}"
      "node { name: 'B' op: 'HalfInput'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_HALF } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_HALF } }"
      " input: ['B', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(HalfInput);B(HalfInput);C(Conv2D);D(Zeta)|"
            "A->C;B->C:1;B->D;C->D:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Conv2DGradFilter_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);D(_MklConv2DBackpropFilter);"
            "DMT/_0(Const);DMT/_1(Const);DMT/_2(Const);E(Zeta)|"
            "A->D;A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->D:1;C->D:2;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:4;DMT/_2->D:5");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Conv2DGradInput_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropInput'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['B', 'A', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);D(_MklConv2DBackpropInput);"
            "DMT/_0(Const);DMT/_1(Const);DMT/_2(Const);E(Zeta)|"
            "A->D:1;A->E;B->D;B:control->DMT/_0:control;"
            "B:control->DMT/_1:control;B:control->DMT/_2:control;C->D:2;"
            "D->E:1;DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// Concat Op test: Concat with no Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_Basic) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['A', 'B:0', 'B:1']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(
      DoMklLayoutOptimizationPass(),
      "A(Const);B(InputList);C(Input);D(_MklConcat);DMT/_0(Const);"
      "DMT/_1(Const);DMT/_2(Const);E(Zeta)|A->D;A:control->DMT/_0:control;"
      "A:control->DMT/_1:control;A:control->DMT/_2:control;B->D:1;"
      "B:1->D:2;C->E;D->E:1;DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// Concat with 2 Mkl layers feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_Input_Mkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['G', 'E', 'F']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(_MklConv2D);"
            "F(_MklConv2D);G(Const);H(_MklConcat);I(Zeta)|A->E;A->I;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "B->E:1;C->F;C:control->DMT/_0:control;C:control->DMT/_1:control;"
            "D->F:1;DMT/_0->F:2;DMT/_1->F:3;DMT/_2->E:2;DMT/_3->E:3;"
            "DMT/_4->H:3;E->H:1;E:2->H:4;F->H:2;F:2->H:5;G->H;"
            "G:control->DMT/_4:control;H->I:1");
}

// Concat with 1 Mkl and 1 non-Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_Input_MixedMkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['G', 'E', 'F']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);E(_MklConv2D);F(Zeta);G(Const);"
            "H(_MklConcat);I(Zeta)|A->E;A->I;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->E:1;C->F;D->F:1;DMT/_0->E:2;"
            "DMT/_1->E:3;DMT/_2->H:3;DMT/_3->H:5;E->H:1;E:2->H:4;F->H:2;"
            "G->H;G:control->DMT/_2:control;G:control->DMT/_3:control;H->I:1");
}

// ConcatV2 Op test: ConcatV2 with no Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_Basic) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['B:0', 'B:1', 'A']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Const);B(InputList);C(Input);D(_MklConcatV2);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);E(Zeta)|A->D:2;B->D;B:1->D:1;"
            "B:control->DMT/_0:control;B:control->DMT/_1:control;"
            "B:control->DMT/_2:control;C->E;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:4;DMT/_2->D:5");
}

// ConcatV2 with 2 Mkl layers feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_Input_Mkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['E', 'F', 'G']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(_MklConv2D);"
            "F(_MklConv2D);G(Const);H(_MklConcatV2);I(Zeta)|A->E;A->I;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;B->E:1;C->F;"
            "C:control->DMT/_0:control;C:control->DMT/_1:control;"
            "D->F:1;DMT/_0->F:2;DMT/_1->F:3;DMT/_2->E:2;DMT/_3->E:3;"
            "DMT/_4->H:5;E->H;E:2->H:3;E:control->DMT/_4:control;F->H:1;"
            "F:2->H:4;G->H:2;H->I:1");
}

// ConcatV2 with 1 Mkl and 1 non-Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_Input_MixedMkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['E', 'F', 'G']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);E(_MklConv2D);F(Zeta);G(Const);"
            "H(_MklConcatV2);I(Zeta)|A->E;A->I;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->E:1;C->F;D->F:1;DMT/_0->E:2;"
            "DMT/_1->E:3;DMT/_2->H:4;DMT/_3->H:5;E->H;E:2->H:3;"
            "E:control->DMT/_2:control;E:control->DMT/_3:control;F->H:1;"
            "G->H:2;H->I:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Relu_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Relu'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklRelu);C(Zeta);DMT/_0(Const)|A->B;A->C;"
            "A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ReluGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'ReluGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklReluGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->C;A->D;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;C->D:1;DMT/_0->C:2;DMT/_1->C:3");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ReluReluGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Relu'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'ReluGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklRelu);C(_MklReluGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->B;A->C;A->D;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;B:1->C:3;C->D:1;DMT/_0->B:1;"
            "DMT/_1->C:2");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPool_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'AvgPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklAvgPool);C(Zeta);DMT/_0(Const)|A->B;A->C;"
            "A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPoolGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Int32Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'AvgPoolGrad' "
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['B', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Int32Input);B(Input);C(_MklAvgPoolGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->C;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;B->D;C->D:1;DMT/_0->C:2;"
            "DMT/_1->C:3");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPoolAvgPoolGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'I' op: 'Int32Input'}"
      "node { name: 'B' op: 'AvgPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'AvgPoolGrad' "
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['I', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklAvgPool);C(_MklAvgPoolGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const);I(Int32Input)|A->B;A->D;A:control->DMT/_0:control;"
            "B->C:1;B:1->C:3;C->D:1;DMT/_0->B:1;DMT/_1->C:2;I->C;"
            "I:control->DMT/_1:control");
}

TEST_F(MklLayoutPassTest, NodeRewrite_FusedBatchNormGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'FusedBatchNormGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'epsilon'      value { f: 0.0001 } }"
      " attr { key: 'is_training'  value { b: true } }"
      " input: ['A', 'B', 'C', 'D', 'E'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'F'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Input);"
            "F(_MklFusedBatchNormGrad);G(Zeta)|A->F;A->G;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->F:1;C->F:2;D->F:3;"
            "DMT/_0->F:5;DMT/_1->F:6;DMT/_2->F:7;DMT/_3->F:8;DMT/_4->F:9;"
            "E->F:4;F->G:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_FusedBatchNorm_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'FusedBatchNorm'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'epsilon'      value { f: 0.0001 } }"
      " attr { key: 'is_training'  value { b: true } }"
      " input: ['A', 'B', 'C', 'D', 'E'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'F'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Input);"
            "F(_MklFusedBatchNorm);G(Zeta)|A->F;A->G;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->F:1;C->F:2;D->F:3;"
            "DMT/_0->F:5;DMT/_1->F:6;DMT/_2->F:7;DMT/_3->F:8;DMT/_4->F:9;"
            "E->F:4;F->G:1");
}

/////////////////////////////////////////////////////////////////////
//  Unit tests related to rewriting node for workspace edges
/////////////////////////////////////////////////////////////////////

/* Test LRN->MaxPool->MaxPoolGrad->LRNGrad replacement by workspace nodes. */
TEST_F(MklLayoutPassTest, MaxPoolLRN_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['B'] }"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'MaxPoolGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['B', 'C', 'D'] }"
      "node { name: 'F' op: 'Input'}"
      "node { name: 'G' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['E', 'F', 'B'] }"
      "node { name: 'H' op: 'Input'}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['H', 'G'] }");
  EXPECT_EQ(
      DoMklLayoutOptimizationPass(),
      "A(Input);B(_MklLRN);C(_MklMaxPool);D(Input);DMT/_0(Const);DMT/_1(Const);"
      "DMT/_2(Const);E(_MklMaxPoolGrad);F(Input);G(_MklLRNGrad);H(Input);"
      "I(Zeta)|A->B;A:control->DMT/_0:control;B->C;B->E;B->G:2;B:1->G:3;"
      "B:2->C:1;B:2->E:4;B:2->G:6;B:3->G:7;B:control->DMT/_1:control;C->E:1;"
      "C:1->E:3;C:2->E:5;C:3->E:7;D->E:2;DMT/_0->B:1;DMT/_1->E:6;DMT/_2->G:5;"
      "E->G;E:1->G:4;E:control->DMT/_2:control;F->G:1;G->I:1;H->I");
}

/* Test LRN->LRNGrad replacement by workspace nodes. */
TEST_F(MklLayoutPassTest, LRN_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['C', 'D', 'B'] }"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklLRN);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);E(_MklLRNGrad);F(Zeta)|"
            "A->B;A:control->DMT/_0:control;B->E:2;B:1->E:3;B:2->E:6;B:3->E:7;"
            "C->E;C->F;C:control->DMT/_1:control;C:control->DMT/_2:control;"
            "D->E:1;DMT/_0->B:1;DMT/_1->E:4;DMT/_2->E:5;E->F:1");
}

/* Test LRN->LRNGrad replacement when only one of them is present. */
TEST_F(MklLayoutPassTest, LRN_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklLRN);C(Zeta);DMT/_0(Const)|"
            "A->B;A->C;A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

/* Test LRN->LRNGrad replacement when only one of them is present. */
TEST_F(MklLayoutPassTest, LRN_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklLRNGrad);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Zeta)|"
            "A->D;A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->D:1;C->D:2;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:7;DMT/_2->D:4;DMT/_3->D:5;DMT/_4->D:6");
}

/* Test LRN->LRNGrad negative case, where single LRN feeds
   2 LRNGrad nodes at different slots. */
TEST_F(MklLayoutPassTest, LRN_Negative3) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['C', 'D', 'B'] }"
      "node { name: 'F' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['C', 'B', 'D'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['E', 'F'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklLRN);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);DMT/_5(Const);"
            "DMT/_6(Const);E(_MklLRNGrad);F(_MklLRNGrad);G(Zeta)|A->B;"
            "A:control->DMT/_0:control;B->E:2;"
            "B->F:1;B:1->E:3;B:2->E:6;B:2->F:5;B:3->E:7;C->E;C->F;"
            "C:control->DMT/_1:control;C:control->DMT/_2:control;"
            "C:control->DMT/_3:control;C:control->DMT/_4:control;"
            "C:control->DMT/_5:control;C:control->DMT/_6:control;"
            "D->E:1;D->F:2;DMT/_0->B:1;DMT/_1->F:3;DMT/_2->F:7;DMT/_3->F:4;"
            "DMT/_4->F:6;DMT/_5->E:4;DMT/_6->E:5;E->G;F->G:1");
}

/* Test MaxPool->MaxPoolGrad replacement by workspace+rewrite nodes. */
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'MaxPoolGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['C', 'B', 'D'] }"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklMaxPool);C(Input);D(Input);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);E(_MklMaxPoolGrad);F(Zeta)|"
            "A->B;A:control->DMT/_0:control;B->E:1;B:1->E:3;B:2->E:5;B:3->E:7;"
            "C->E;C->F;C:control->DMT/_1:control;C:control->DMT/_2:control;"
            "D->E:2;DMT/_0->B:1;DMT/_1->E:4;DMT/_2->E:6;E->F:1");
}

// Test MaxPool>MaxPoolGrad replacement when only one of them is present.
// In this case, we will rewrite MaxPool node but workspace edges will not
// be present.
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklMaxPool);C(Zeta);DMT/_0(Const)|"
            "A->B;A->C;A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

// Test MaxPoolGrad replacement when only one of them is present.
// In this case, we will rewrite MaxPoolGrad and for workspace tensor and
// its Mkl part, we will generate dummy tensor.
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'MaxPoolGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklMaxPoolGrad);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Zeta)|"
            "A->D;A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->D:1;C->D:2;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:7;DMT/_2->D:4;DMT/_3->D:5;DMT/_4->D:6");
}

// Test MaxPool handling for batch-wise pooling (NCHW)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative3) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 2, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for batch-wise pooling (NCHW)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative4) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 2, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative5) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:2, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NCHW)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative6) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:2, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for batch-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative7) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 2, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for batch-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative8) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 2, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative9) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:2} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative10) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

/////////////////////////////////////////////////////////////////////

// Single Conv2D Op on GPU device
// No rewrite should happen
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['B', 'C'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Conv2D);D(Zeta)|A->C;B->C:1;B->D;C->D:1");
}

TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(BiasAddGrad);M(_MklInput);N(_MklInput);"
            "O(_MklInput)|A->D;A->E:1;B->D:1;C->D:2;D->E;E->F;"
            "M->D:3;N->D:4;O->D:5");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Conv2DGradFilter_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);D(Conv2DBackpropFilter);E(Zeta)|"
            "A->D;A->E;B->D:1;C->D:2;D->E:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Relu_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Relu'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Relu);C(Zeta)|A->B;A->C;B->C:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ReluGrad_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'ReluGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(ReluGrad);D(Zeta)|A->C;A->D;B->C:1;C->D:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_MaxPool_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPool_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'AvgPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(AvgPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Concat Op test: Concat with no Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['A', 'B:0', 'B:1']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Const);B(InputList);C(Input);D(Concat);E(Zeta)|A->D;"
            "B->D:1;B:1->D:2;C->E;D->E:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['B:0', 'B:1', 'A']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Const);B(InputList);C(Input);D(ConcatV2);E(Zeta)|"
            "A->D:2;B->D;B:1->D:1;C->E;D->E:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_FusedBatchNorm_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'FusedBatchNorm'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'epsilon'      value { f: 0.0001 } }"
      " attr { key: 'is_training'  value { b: true } }"
      " input: ['A', 'B', 'C', 'D', 'E'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'F'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);E(Input);"
            "F(FusedBatchNorm);G(Zeta)|A->F;A->G;B->F:1;C->F:2;D->F:3;"
            "E->F:4;F->G:1");
}

TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_DeviceTest) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);E(BiasAdd);"
            "M(_MklInput);N(_MklInput);Y(Input);Z(Zeta)|A->C;"
            "B->C:1;C->E;D->E:1;E->Z;M->C:2;N->C:3;Y->Z:1");
}

/////////////////////////////////////////////////////////////////////

static void BM_MklLayoutRewritePass(int iters, int op_nodes) {
  testing::StopTiming();
  string s;
  for (int in = 0; in < 10; in++) {
    s += strings::Printf("node { name: 'in%04d' op: 'Input'}", in);
  }
  random::PhiloxRandom philox(301, 17);
  random::SimplePhilox rnd(&philox);
  for (int op = 0; op < op_nodes; op++) {
    s += strings::Printf(
        "node { name: 'op%04d' op: 'Zeta' attr { key: 'T' value { "
        "type: DT_FLOAT } } input: ['in%04d', 'in%04d' ] }",
        op, rnd.Uniform(10), rnd.Uniform(10));
  }

  bool first = true;
  while (iters > 0) {
    Graph* graph = new Graph(OpRegistry::Global());
    InitGraph(s, graph);
    int N = graph->num_node_ids();
    if (first) {
      testing::SetLabel(strings::StrCat("Per graph node.  Nodes: ", N));
      first = false;
    }
    {
      testing::StartTiming();
      std::unique_ptr<Graph> ug(graph);
      RunMklLayoutRewritePass(&ug);
      testing::StopTiming();
    }
    iters -= N;  // Our benchmark units are individual graph nodes,
                 // not whole graphs
    // delete graph;
  }
}
BENCHMARK(BM_MklLayoutRewritePass)->Arg(1000)->Arg(10000);

}  // namespace

#else  // INTEL_MKL_ML

namespace {

const char kCPUDevice[] = "/job:a/replica:0/task:0/device:CPU:0";
const char kGPUDevice[] = "/job:a/replica:0/task:0/device:GPU:0";

static void InitGraph(const string& s, Graph* graph,
                      const string& device = kCPUDevice) {
  GraphDef graph_def;

  auto parser = protobuf::TextFormat::Parser();
  //  parser.AllowRelaxedWhitespace(true);
  CHECK(parser.MergeFromString(s, &graph_def)) << s;
  GraphConstructorOptions opts;
  TF_CHECK_OK(ConvertGraphDefToGraph(opts, graph_def, graph));

  for (Node* node : graph->nodes()) {
    node->set_assigned_device_name(device);
  }
}

class MklLayoutPassTest : public ::testing::Test {
 public:
  MklLayoutPassTest() : graph_(OpRegistry::Global()) {}

  void InitGraph(const string& s, const string& device = kCPUDevice) {
    ::tensorflow::InitGraph(s, &graph_, device);
    original_ = CanonicalGraphString(&graph_);
  }

  static bool IncludeNode(const Node* n) { return n->IsOp(); }

  static string EdgeId(const Node* n, int index) {
    if (index == 0) {
      return n->name();
    } else if (index == Graph::kControlSlot) {
      return strings::StrCat(n->name(), ":control");
    } else {
      return strings::StrCat(n->name(), ":", index);
    }
  }

  string CanonicalGraphString(Graph* g) {
    std::vector<string> nodes;
    std::vector<string> edges;
    for (const Node* n : g->nodes()) {
      if (IncludeNode(n)) {
        nodes.push_back(strings::StrCat(n->name(), "(", n->type_string(), ")"));
      }
    }
    for (const Edge* e : g->edges()) {
      if (IncludeNode(e->src()) && IncludeNode(e->dst())) {
        edges.push_back(strings::StrCat(EdgeId(e->src(), e->src_output()), "->",
                                        EdgeId(e->dst(), e->dst_input())));
      }
    }
    // Canonicalize
    std::sort(nodes.begin(), nodes.end());
    std::sort(edges.begin(), edges.end());
    return strings::StrCat(str_util::Join(nodes, ";"), "|",
                           str_util::Join(edges, ";"));
  }

  string DoMklLayoutOptimizationPass() {
    string before = CanonicalGraphString(&graph_);
    LOG(ERROR) << "Before MKL layout rewrite pass: " << before;

    std::unique_ptr<Graph>* ug = new std::unique_ptr<Graph>(&graph_);
    RunMklLayoutRewritePass(ug);

    string result = CanonicalGraphString(&graph_);
    LOG(ERROR) << "After MKL layout rewrite pass:  " << result;
    return result;
  }

  const string& OriginalGraph() const { return original_; }

  Graph graph_;
  string original_;
};

REGISTER_OP("Input").Output("o: float").SetIsStateful();
REGISTER_OP("InputList").Output("o: N * float").Attr("N: int").SetIsStateful();
REGISTER_OP("HalfInput").Output("o: half").SetIsStateful();
REGISTER_OP("Int32Input").Output("o: int32").SetIsStateful();
REGISTER_OP("_MklInput").Output("o: uint8").SetIsStateful();
REGISTER_OP("_MklInput2")
    .Output("o: uint8")
    .Output("o1: uint8")
    .SetIsStateful();

/////////////////////////////////////////////////////////////////////
//  Unit tests related to node merge optiimization
/////////////////////////////////////////////////////////////////////

TEST_F(MklLayoutPassTest, Basic) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Zeta);D(Zeta)|"
            "A->C;A->D;B->C:1;B->D:1");
}

// Test set 1: Conv2D + AddBias

// C=Conv2D(A,B); E=BiasAdd(C,D); Z=Zeta(E,Y)
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Positive) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);E(_MklConv2DWithBias);Y(Input);Z(Zeta)|A->E;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->E:1;D->E:2;DMT/_0->E:3;DMT/_1->E:4;"
            "DMT/_2->E:5;E->Z;Y->Z:1");
}

// Graph contains only Conv2D, no AddBias.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_NoAddBias) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);DMT/_0(Const);DMT/_1(Const)|"
            "A->C;A:control->DMT/_0:control;A:control->DMT/_1:control;B->C:1;"
            "DMT/_0->C:2;DMT/_1->C:3");
}

// Conv2D output does not go to BiasAdd.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_Dataflow1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D', 'E'] }");  // Output of _MklConv2D does not go to BiasAdd.
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);DMT/_0(Const);"
            "DMT/_1(Const);E(Input);F(BiasAdd)|A->C;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;D->F;DMT/_0->C:2;DMT/_1->C:3;"
            "E->F:1");
}

// Conv2D has two outgoing edges: BiasAdd and some other dummy node (Zeta).
// Merge should not be done in such case.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_Dataflow2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D', 'E'] }"  // Conv2D has two outputs.
                              // No merge should happen.
      "node { name: 'G' op: 'Zeta'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);DMT/_0(Const);"
            "DMT/_1(Const);E(Input);F(BiasAdd);G(Zeta)|A->C;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;B->C:1;C->G;"
            "D->F;DMT/_0->C:2;DMT/_1->C:3;E->F:1;E->G:1");
}

// data_format attribute value mismatch. Merge should not be done
// in such case.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_Negative_AttrMismatch) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NHCW' } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);DMT/_0(Const);"
            "DMT/_1(Const);E(BiasAdd)|A->C;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;C->E;D->E:1;DMT/_0->C:2;"
            "DMT/_1->C:3");
}

// Test set 2: BiasAddGrad + Conv2DBackpropFilter fusion tests

TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackpropFilterFusion_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);"
            "D(_MklConv2DBackpropFilterWithBias);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const)|A->D;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;A:control->DMT/_2:control;B->D:1;C->D:2;"
            "DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// BiasAddGrad fusion in the presence of BackpropFilter. But nodes do not match
// criteria for rewrite. So rewrite should not happen. 3rd input of
// Conv2DBackpropFilter is different than input to BiasAddGrad.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackpropFilterFusion_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['A'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);"
            "D(_MklConv2DBackpropFilter);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);E(BiasAddGrad)|A->D;A->E;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;A:control->DMT/_2:control;B->D:1;C->D:2;"
            "DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// BiasAddGrad fusion, but nodes do not match criteria for fusion.
// Different input formats.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackpropFilterFusion_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NHWC' } }"
      " input: ['A'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);"
            "D(_MklConv2DBackpropFilter);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);E(BiasAddGrad)|A->D;A->E;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;A:control->DMT/_2:control;B->D:1;C->D:2;"
            "DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// BiasAddGrad fusion in the presence of BackpropFilter only. Fusion is done
// before node rewrite. Check this ordering.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackpropFilterFusion_Negative3) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: '_MklConv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['E', 'F', 'A', 'M', 'N', 'O'] }"
      "node { name: 'H' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(Int32Input);G(_MklConv2DBackpropFilter);H(BiasAddGrad);"
            "M(_MklInput);N(_MklInput);O(_MklInput)|A->D;A->E:1;A->G:2;B->D:1;"
            "C->D:2;D->E;E->G;E->H;F->G:1;M->D:3;M->G:3;N->D:4;N->G:4;O->D:5;"
            "O->G:5");
}

// C=Conv2D(A,B); E=BiasAdd(C,D); Y=Zeta(E,X);
// G=Conv2DBackpropInput(F,B,E)
// This is a case of node rewrite followed by node merge followed by connecting
// filter output of Conv2DWithBias to filter input of Conv2DBackpropInput.
TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_ConvBpropInput_FilterFwd) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'X' op: 'Input'}"
      "node { name: 'Y' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'X']}"
      "node { name: 'F' op: 'Int32Input'}"
      "node { name: 'G' op: 'Conv2DBackpropInput'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['F', 'B', 'E']}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['G', 'X']}");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);E(_MklConv2DWithBias);F(Int32Input);"
            "G(_MklConv2DBackpropInput);X(Input);Y(Zeta);Z(Zeta)|"
            "A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->E:1;D->E:2;DMT/_0->E:3;"
            "DMT/_1->E:4;DMT/_2->E:5;DMT/_3->G:3;E->G:2;E->Y;E:1->G:1;E:2->G:5;"
            "E:3->G:4;F->G;F:control->DMT/_3:control;G->Z;X->Y:1;X->Z:1");
}

/////////////////////////////////////////////////////////////////////
//  Unit tests related to rewriting node to Mkl node
/////////////////////////////////////////////////////////////////////

// Single Conv2D Op; No Mkl layer on the input and on the output.
// We will generate dummy Mkl tensor as 2nd input of Conv2D.
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_Basic) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['B', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->C;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;B->D;C->D:1;DMT/_0->C:2;"
            "DMT/_1->C:3");
}

// 2 Conv2D Ops in sequence. Both should get transformed and 1st Conv2D will
// have 2 outputs, both of which will be inputs to next Conv2D.
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_Positive1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(_MklConv2D);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);E(Zeta)|A->C;A->D;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->C:1;C->D:1;C->E;"
            "C:2->D:3;D->E:1;DMT/_0->C:2;DMT/_1->C:3;DMT/_2->D:2");
}

// Conv2D with INT32 which is not supported by Mkl
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_Negative_UnsupportedType) {
  InitGraph(
      "node { name: 'A' op: 'HalfInput'}"
      "node { name: 'B' op: 'HalfInput'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_HALF } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_HALF } }"
      " input: ['B', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(HalfInput);B(HalfInput);C(Conv2D);D(Zeta)|"
            "A->C;B->C:1;B->D;C->D:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Conv2DGradFilter_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);D(_MklConv2DBackpropFilter);"
            "DMT/_0(Const);DMT/_1(Const);DMT/_2(Const);E(Zeta)|"
            "A->D;A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;B->D:1;C->D:2;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:4;DMT/_2->D:5");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Conv2DGradInput_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropInput'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['B', 'A', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);D(_MklConv2DBackpropInput);"
            "DMT/_0(Const);DMT/_1(Const);DMT/_2(Const);E(Zeta)|"
            "A->D:1;A->E;B->D;B:control->DMT/_0:control;"
            "B:control->DMT/_1:control;B:control->DMT/_2:control;C->D:2;"
            "D->E:1;DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// Check that we never rewrite BiasAddGrad.
TEST_F(MklLayoutPassTest, NodeRewrite_BiasAddGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Polygamma'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Polygamma);D(Zeta);E(BiasAddGrad)|"
            "A->C;A->D:1;B->C:1;C->D;D->E");
}

// Check that we never rewrite BiasAddGrad.
TEST_F(MklLayoutPassTest, NodeRewrite_BiasAddGrad_Positive1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'MatMul'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'transpose_a'      value { b: false } }"
      " attr { key: 'transpose_b'      value { b: false } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(MatMul);D(Zeta);E(BiasAddGrad)|"
            "A->C;A->D:1;B->C:1;C->D;D->E");
}

// Check that we never rewrite BiasAddGrad.
TEST_F(MklLayoutPassTest, NodeRewrite_BiasAddGrad_Positive2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['C', 'A']}"
      "node { name: 'E' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Zeta);E(BiasAddGrad);"
            "M(_MklInput);N(_MklInput)|A->C;A->D:1;B->C:1;C->D;D->E;"
            "M->C:2;N->C:3");
}

// Concat Op test: Concat with no Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_Basic) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['A', 'B:0', 'B:1']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(
      DoMklLayoutOptimizationPass(),
      "A(Const);B(InputList);C(Input);D(_MklConcat);DMT/_0(Const);"
      "DMT/_1(Const);DMT/_2(Const);E(Zeta)|A->D;A:control->DMT/_0:control;"
      "A:control->DMT/_1:control;A:control->DMT/_2:control;B->D:1;"
      "B:1->D:2;C->E;D->E:1;DMT/_0->D:3;DMT/_1->D:4;DMT/_2->D:5");
}

// Concat with 2 Mkl layers feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_Input_Mkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['G', 'E', 'F']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(_MklConv2D);"
            "F(_MklConv2D);G(Const);H(_MklConcat);I(Zeta)|A->E;A->I;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "B->E:1;C->F;C:control->DMT/_0:control;C:control->DMT/_1:control;"
            "D->F:1;DMT/_0->F:2;DMT/_1->F:3;DMT/_2->E:2;DMT/_3->E:3;"
            "DMT/_4->H:3;E->H:1;E:2->H:4;F->H:2;F:2->H:5;G->H;"
            "G:control->DMT/_4:control;H->I:1");
}

// Concat with 1 Mkl and 1 non-Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_Input_MixedMkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['G', 'E', 'F']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);E(_MklConv2D);F(Zeta);G(Const);"
            "H(_MklConcat);I(Zeta)|A->E;A->I;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->E:1;C->F;D->F:1;DMT/_0->E:2;"
            "DMT/_1->E:3;DMT/_2->H:3;DMT/_3->H:5;E->H:1;E:2->H:4;F->H:2;"
            "G->H;G:control->DMT/_2:control;G:control->DMT/_3:control;H->I:1");
}

// ConcatV2 Op test: ConcatV2 with no Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_Basic) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['B:0', 'B:1', 'A']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Const);B(InputList);C(Input);D(_MklConcatV2);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);E(Zeta)|A->D:2;B->D;B:1->D:1;"
            "B:control->DMT/_0:control;B:control->DMT/_1:control;"
            "B:control->DMT/_2:control;C->E;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:4;DMT/_2->D:5");
}

// ConcatV2 with 2 Mkl layers feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_Input_Mkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['E', 'F', 'G']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(_MklConv2D);"
            "F(_MklConv2D);G(Const);H(_MklConcatV2);I(Zeta)|A->E;A->I;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;B->E:1;C->F;"
            "C:control->DMT/_0:control;C:control->DMT/_1:control;"
            "D->F:1;DMT/_0->F:2;DMT/_1->F:3;DMT/_2->E:2;DMT/_3->E:3;"
            "DMT/_4->H:5;E->H;E:2->H:3;E:control->DMT/_4:control;F->H:1;"
            "F:2->H:4;G->H:2;H->I:1");
}

// ConcatV2 with 1 Mkl and 1 non-Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_Input_MixedMkl) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D']}"
      "node { name: 'G' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'H' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['E', 'F', 'G']}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'H'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);E(_MklConv2D);F(Zeta);G(Const);"
            "H(_MklConcatV2);I(Zeta)|A->E;A->I;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->E:1;C->F;D->F:1;DMT/_0->E:2;"
            "DMT/_1->E:3;DMT/_2->H:4;DMT/_3->H:5;E->H;E:2->H:3;"
            "E:control->DMT/_2:control;E:control->DMT/_3:control;F->H:1;"
            "G->H:2;H->I:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Relu_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Relu'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklRelu);C(Zeta);DMT/_0(Const)|A->B;A->C;"
            "A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ReluGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'ReluGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklReluGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->C;A->D;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;C->D:1;DMT/_0->C:2;DMT/_1->C:3");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ReluReluGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Relu'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'ReluGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklRelu);C(_MklReluGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->B;A->C;A->D;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;B:1->C:3;C->D:1;DMT/_0->B:1;"
            "DMT/_1->C:2");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPool_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'AvgPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklAvgPool);C(Zeta);DMT/_0(Const)|A->B;A->C;"
            "A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPoolGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Int32Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'AvgPoolGrad' "
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['B', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Int32Input);B(Input);C(_MklAvgPoolGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const)|A->C;A:control->DMT/_0:control;"
            "A:control->DMT/_1:control;B->C:1;B->D;C->D:1;DMT/_0->C:2;"
            "DMT/_1->C:3");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPoolAvgPoolGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'I' op: 'Int32Input'}"
      "node { name: 'B' op: 'AvgPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'AvgPoolGrad' "
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['I', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklAvgPool);C(_MklAvgPoolGrad);D(Zeta);DMT/_0(Const);"
            "DMT/_1(Const);I(Int32Input)|A->B;A->D;A:control->DMT/_0:control;"
            "B->C:1;B:1->C:3;C->D:1;DMT/_0->B:1;DMT/_1->C:2;I->C;"
            "I:control->DMT/_1:control");
}

TEST_F(MklLayoutPassTest, NodeRewrite_FusedBatchNormGrad_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'FusedBatchNormGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'epsilon'      value { f: 0.0001 } }"
      " attr { key: 'is_training'  value { b: true } }"
      " input: ['A', 'B', 'C', 'D', 'E'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'F'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Input);"
            "F(_MklFusedBatchNormGrad);G(Zeta)|A->F;A->G;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->F:1;C->F:2;D->F:3;"
            "DMT/_0->F:5;DMT/_1->F:6;DMT/_2->F:7;DMT/_3->F:8;DMT/_4->F:9;"
            "E->F:4;F->G:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_FusedBatchNorm_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'FusedBatchNorm'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'epsilon'      value { f: 0.0001 } }"
      " attr { key: 'is_training'  value { b: true } }"
      " input: ['A', 'B', 'C', 'D', 'E'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'F'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Input);"
            "F(_MklFusedBatchNorm);G(Zeta)|A->F;A->G;"
            "A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->F:1;C->F:2;D->F:3;"
            "DMT/_0->F:5;DMT/_1->F:6;DMT/_2->F:7;DMT/_3->F:8;DMT/_4->F:9;"
            "E->F:4;F->G:1");
}

/////////////////////////////////////////////////////////////////////
//  Unit tests related to rewriting node for workspace edges
/////////////////////////////////////////////////////////////////////

/* Test LRN->MaxPool->MaxPoolGrad->LRNGrad replacement by workspace nodes. */
TEST_F(MklLayoutPassTest, MaxPoolLRN_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['B'] }"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'MaxPoolGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['B', 'C', 'D'] }"
      "node { name: 'F' op: 'Input'}"
      "node { name: 'G' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['E', 'F', 'B'] }"
      "node { name: 'H' op: 'Input'}"
      "node { name: 'I' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['H', 'G'] }");
  EXPECT_EQ(
      DoMklLayoutOptimizationPass(),
      "A(Input);B(_MklLRN);C(_MklMaxPool);D(Input);DMT/_0(Const);DMT/_1(Const);"
      "DMT/_2(Const);E(_MklMaxPoolGrad);F(Input);G(_MklLRNGrad);H(Input);"
      "I(Zeta)|A->B;A:control->DMT/_0:control;B->C;B->E;B->G:2;B:1->G:3;"
      "B:2->C:1;B:2->E:4;B:2->G:6;B:3->G:7;B:control->DMT/_1:control;C->E:1;"
      "C:1->E:3;C:2->E:5;C:3->E:7;D->E:2;DMT/_0->B:1;DMT/_1->E:6;DMT/_2->G:5;"
      "E->G;E:1->G:4;E:control->DMT/_2:control;F->G:1;G->I:1;H->I");
}

/* Test LRN->LRNGrad replacement by workspace nodes. */
TEST_F(MklLayoutPassTest, LRN_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['C', 'D', 'B'] }"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklLRN);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);E(_MklLRNGrad);F(Zeta)|"
            "A->B;A:control->DMT/_0:control;B->E:2;B:1->E:3;B:2->E:6;B:3->E:7;"
            "C->E;C->F;C:control->DMT/_1:control;C:control->DMT/_2:control;"
            "D->E:1;DMT/_0->B:1;DMT/_1->E:4;DMT/_2->E:5;E->F:1");
}

/* Test LRN->LRNGrad replacement when only one of them is present. */
TEST_F(MklLayoutPassTest, LRN_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklLRN);C(Zeta);DMT/_0(Const)|"
            "A->B;A->C;A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

/* Test LRN->LRNGrad replacement when only one of them is present. */
TEST_F(MklLayoutPassTest, LRN_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklLRNGrad);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Zeta)|"
            "A->D;A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->D:1;C->D:2;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:7;DMT/_2->D:4;DMT/_3->D:5;DMT/_4->D:6");
}

/* Test LRN->LRNGrad negative case, where single LRN feeds
   2 LRNGrad nodes at different slots. */
TEST_F(MklLayoutPassTest, LRN_Negative3) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'LRN'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['C', 'D', 'B'] }"
      "node { name: 'F' op: 'LRNGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'alpha'        value { f: 0.001 } }"
      " attr { key: 'beta'         value { f: 0.75 } }"
      " attr { key: 'bias'         value { f: 1.0 } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'depth_radius' value { i: 2 } }"
      " input: ['C', 'B', 'D'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['E', 'F'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklLRN);C(Input);D(Input);DMT/_0(Const);DMT/_1(Const);"
            "DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);DMT/_5(Const);"
            "DMT/_6(Const);E(_MklLRNGrad);F(_MklLRNGrad);G(Zeta)|A->B;"
            "A:control->DMT/_0:control;B->E:2;"
            "B->F:1;B:1->E:3;B:2->E:6;B:2->F:5;B:3->E:7;C->E;C->F;"
            "C:control->DMT/_1:control;C:control->DMT/_2:control;"
            "C:control->DMT/_3:control;C:control->DMT/_4:control;"
            "C:control->DMT/_5:control;C:control->DMT/_6:control;"
            "D->E:1;D->F:2;DMT/_0->B:1;DMT/_1->F:3;DMT/_2->F:7;DMT/_3->F:4;"
            "DMT/_4->F:6;DMT/_5->E:4;DMT/_6->E:5;E->G;F->G:1");
}

/* Test MaxPool->MaxPoolGrad replacement by workspace+rewrite nodes. */
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Positive) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'MaxPoolGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['C', 'B', 'D'] }"
      "node { name: 'F' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'E'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklMaxPool);C(Input);D(Input);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);E(_MklMaxPoolGrad);F(Zeta)|"
            "A->B;A:control->DMT/_0:control;B->E:1;B:1->E:3;B:2->E:5;B:3->E:7;"
            "C->E;C->F;C:control->DMT/_1:control;C:control->DMT/_2:control;"
            "D->E:2;DMT/_0->B:1;DMT/_1->E:4;DMT/_2->E:6;E->F:1");
}

// Test MaxPool>MaxPoolGrad replacement when only one of them is present.
// In this case, we will rewrite MaxPool node but workspace edges will not
// be present.
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative1) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(_MklMaxPool);C(Zeta);DMT/_0(Const)|"
            "A->B;A->C;A:control->DMT/_0:control;B->C:1;DMT/_0->B:1");
}

// Test MaxPoolGrad replacement when only one of them is present.
// In this case, we will rewrite MaxPoolGrad and for workspace tensor and
// its Mkl part, we will generate dummy tensor.
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative2) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'MaxPoolGrad'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:3, i:3} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:2, i:2} } }"
      " input: ['A', 'B', 'C'] }"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklMaxPoolGrad);DMT/_0(Const);"
            "DMT/_1(Const);DMT/_2(Const);DMT/_3(Const);DMT/_4(Const);E(Zeta)|"
            "A->D;A->E;A:control->DMT/_0:control;A:control->DMT/_1:control;"
            "A:control->DMT/_2:control;A:control->DMT/_3:control;"
            "A:control->DMT/_4:control;B->D:1;C->D:2;D->E:1;DMT/_0->D:3;"
            "DMT/_1->D:7;DMT/_2->D:4;DMT/_3->D:5;DMT/_4->D:6");
}

// Test MaxPool handling for batch-wise pooling (NCHW)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative3) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 2, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for batch-wise pooling (NCHW)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative4) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 2, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative5) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:2, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NCHW)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative6) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:2, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for batch-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative7) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 2, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for batch-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative8) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 2, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative9) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:2} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Test MaxPool handling for depth-wise pooling (NHWC)
// No rewrite should take place in such case
TEST_F(MklLayoutPassTest, NodeWorkspace_MaxPool_Negative10) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:2} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }");
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

/////////////////////////////////////////////////////////////////////

// Single Conv2D Op on GPU device
// No rewrite should happen
TEST_F(MklLayoutPassTest, NodeRewrite_Conv2D_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Conv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B']}"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['B', 'C'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Conv2D);D(Zeta)|A->C;B->C:1;B->D;C->D:1");
}

TEST_F(MklLayoutPassTest, NodeMerge_Conv2DBackprop_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'O' op: '_MklInput'}"
      "node { name: 'D' op: '_MklConv2DWithBias'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C', 'M', 'N', 'O']}"
      "node { name: 'E' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['D', 'A']}"
      "node { name: 'F' op: 'BiasAddGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['E'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(_MklConv2DWithBias);"
            "E(Zeta);F(BiasAddGrad);M(_MklInput);N(_MklInput);"
            "O(_MklInput)|A->D;A->E:1;B->D:1;C->D:2;D->E;E->F;"
            "M->D:3;N->D:4;O->D:5");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Conv2DGradFilter_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Int32Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Conv2DBackpropFilter'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'C']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'D'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Int32Input);C(Input);D(Conv2DBackpropFilter);E(Zeta)|"
            "A->D;A->E;B->D:1;C->D:2;D->E:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_Relu_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Relu'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Relu);C(Zeta)|A->B;A->C;B->C:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ReluGrad_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'ReluGrad'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }"
      "node { name: 'D' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'C'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(ReluGrad);D(Zeta)|A->C;A->D;B->C:1;C->D:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_MaxPool_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'MaxPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(MaxPool);C(Zeta)|A->B;A->C;B->C:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_AvgPool_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'AvgPool'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NHWC' } }"
      " attr { key: 'ksize'        value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'      value { s: 'VALID' } }"
      " attr { key: 'strides'      value { list: {i: 1, i:1, i:1, i:1} } }"
      " input: ['A'] }"
      "node { name: 'C' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'B'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(AvgPool);C(Zeta)|A->B;A->C;B->C:1");
}

// Concat Op test: Concat with no Mkl layer feeding it
TEST_F(MklLayoutPassTest, NodeRewrite_Concat_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Concat'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['A', 'B:0', 'B:1']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Const);B(InputList);C(Input);D(Concat);E(Zeta)|A->D;"
            "B->D:1;B:1->D:2;C->E;D->E:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_ConcatV2_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Const' "
      " attr { key: 'dtype' value { type: DT_INT32 } }"
      " attr { key: 'value' value { "
      "    tensor { dtype: DT_INT32 tensor_shape { dim { size: 1 } } "
      "    int_val: 0 } } } }"
      "node { name: 'B' op: 'InputList'"
      " attr { key: 'N'                value { i: 2 } }}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'ConcatV2'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'Tidx'             value { type: DT_INT32 } }"
      " attr { key: 'N'                value { i: 2 } }"
      " input: ['B:0', 'B:1', 'A']}"
      "node { name: 'E' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['C', 'D'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Const);B(InputList);C(Input);D(ConcatV2);E(Zeta)|"
            "A->D:2;B->D;B:1->D:1;C->E;D->E:1");
}

TEST_F(MklLayoutPassTest, NodeRewrite_FusedBatchNorm_DeviceTest) {
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'C' op: 'Input'}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'Input'}"
      "node { name: 'F' op: 'FusedBatchNorm'"
      " attr { key: 'T'            value { type: DT_FLOAT } }"
      " attr { key: 'data_format'  value { s: 'NCHW' } }"
      " attr { key: 'epsilon'      value { f: 0.0001 } }"
      " attr { key: 'is_training'  value { b: true } }"
      " input: ['A', 'B', 'C', 'D', 'E'] }"
      "node { name: 'G' op: 'Zeta' attr { key: 'T' value { type: DT_FLOAT } }"
      " input: ['A', 'F'] }",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(Input);D(Input);E(Input);"
            "F(FusedBatchNorm);G(Zeta)|A->F;A->G;B->F:1;C->F:2;D->F:3;"
            "E->F:4;F->G:1");
}

TEST_F(MklLayoutPassTest, NodeMerge_Conv2DWithBias_DeviceTest) {
  CHECK_EQ(kTensorOrdering, MklTfTensorOrdering::TENSORS_CONTIGUOUS);
  InitGraph(
      "node { name: 'A' op: 'Input'}"
      "node { name: 'B' op: 'Input'}"
      "node { name: 'M' op: '_MklInput'}"
      "node { name: 'N' op: '_MklInput'}"
      "node { name: 'C' op: '_MklConv2D'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " attr { key: 'use_cudnn_on_gpu' value { b: false } }"
      " attr { key: 'strides'          value { list: {i: 1, i:1, i:1, i:1} } }"
      " attr { key: 'padding'          value { s: 'SAME' } }"
      " input: ['A', 'B', 'M', 'N']}"
      "node { name: 'D' op: 'Input'}"
      "node { name: 'E' op: 'BiasAdd'"
      " attr { key: 'T'                value { type: DT_FLOAT } }"
      " attr { key: 'data_format'      value { s: 'NCHW' } }"
      " input: ['C', 'D'] }"
      "node { name: 'Y' op: 'Input'}"
      "node { name: 'Z' op: 'Zeta'"
      " attr {key: 'T'                 value { type: DT_FLOAT } }"
      " input: ['E', 'Y']}",
      kGPUDevice);
  EXPECT_EQ(DoMklLayoutOptimizationPass(),
            "A(Input);B(Input);C(_MklConv2D);D(Input);E(BiasAdd);"
            "M(_MklInput);N(_MklInput);Y(Input);Z(Zeta)|A->C;"
            "B->C:1;C->E;D->E:1;E->Z;M->C:2;N->C:3;Y->Z:1");
}

/////////////////////////////////////////////////////////////////////

static void BM_MklLayoutRewritePass(int iters, int op_nodes) {
  testing::StopTiming();
  string s;
  for (int in = 0; in < 10; in++) {
    s += strings::Printf("node { name: 'in%04d' op: 'Input'}", in);
  }
  random::PhiloxRandom philox(301, 17);
  random::SimplePhilox rnd(&philox);
  for (int op = 0; op < op_nodes; op++) {
    s += strings::Printf(
        "node { name: 'op%04d' op: 'Zeta' attr { key: 'T' value { "
        "type: DT_FLOAT } } input: ['in%04d', 'in%04d' ] }",
        op, rnd.Uniform(10), rnd.Uniform(10));
  }

  bool first = true;
  while (iters > 0) {
    Graph* graph = new Graph(OpRegistry::Global());
    InitGraph(s, graph);
    int N = graph->num_node_ids();
    if (first) {
      testing::SetLabel(strings::StrCat("Per graph node.  Nodes: ", N));
      first = false;
    }
    {
      testing::StartTiming();
      std::unique_ptr<Graph> ug(graph);
      RunMklLayoutRewritePass(&ug);
      testing::StopTiming();
    }
    iters -= N;  // Our benchmark units are individual graph nodes,
                 // not whole graphs
    // delete graph;
  }
}
BENCHMARK(BM_MklLayoutRewritePass)->Arg(1000)->Arg(10000);

}  // namespace

#endif  // INTEL_MKL_ML

}  // namespace tensorflow

#endif /* INTEL_MKL */
