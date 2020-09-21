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

#include <memory>
#include <utility>
#include <vector>

#include "tensorflow/compiler/xla/client/client_library.h"
#include "tensorflow/compiler/xla/client/computation.h"
#include "tensorflow/compiler/xla/client/computation_builder.h"
#include "tensorflow/compiler/xla/client/global_data.h"
#include "tensorflow/compiler/xla/layout_util.h"
#include "tensorflow/compiler/xla/literal_util.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/compiler/xla/statusor.h"
#include "tensorflow/compiler/xla/test.h"
#include "tensorflow/compiler/xla/tests/literal_test_util.h"
#include "tensorflow/compiler/xla/tests/test_macros.h"
#include "tensorflow/compiler/xla/tests/test_utils.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"
#include "tensorflow/core/platform/types.h"

namespace xla {
namespace {

// An enumerator for the client types that we want to iterate over in
// the various tests.
enum class ClientType { kLocal, kCompileOnly };
ClientType client_types[] = {ClientType::kLocal, ClientType::kCompileOnly};

class ComputeConstantTest : public ::testing::Test {
 public:
  explicit ComputeConstantTest(
      perftools::gputools::Platform* platform = nullptr)
      : platform_(platform) {}

  string TestName() const {
    return ::testing::UnitTest::GetInstance()->current_test_info()->name();
  }

  Client* ClientOrDie(::perftools::gputools::Platform* platform,
                      ClientType client_type) {
    if (client_type == ClientType::kLocal) {
      StatusOr<Client*> result =
          ClientLibrary::GetOrCreateLocalClient(platform);
      TF_CHECK_OK(result.status())
          << "could not create LocalClient for testing";
      return result.ValueOrDie();
    } else if (client_type == ClientType::kCompileOnly) {
      StatusOr<Client*> result =
          ClientLibrary::GetOrCreateCompileOnlyClient(platform);
      TF_CHECK_OK(result.status())
          << "could not create CompileOnlyClient for testing";
      return result.ValueOrDie();
    }
    LOG(FATAL) << "invalid client_type value";
  }

  StatusOr<std::unique_ptr<Literal>> ComputeConstantLiteral(
      Client* client, const ComputationDataHandle& operand,
      ComputationBuilder* builder, Layout* output_layout = nullptr,
      tensorflow::gtl::ArraySlice<Literal> parameters = {}) {
    TF_ASSIGN_OR_RETURN(auto computed, builder->ComputeConstant(
                                           operand, output_layout, parameters));
    return std::move(computed);
  }

  template <class Scalar>
  StatusOr<Scalar> ComputeConstantScalar(
      Client* client, const ComputationDataHandle& operand,
      ComputationBuilder* builder,
      tensorflow::gtl::ArraySlice<Literal> parameters = {}) {
    TF_ASSIGN_OR_RETURN(
        auto literal,
        ComputeConstantLiteral(client, operand, builder, nullptr, parameters));
    return literal->Get<Scalar>({});
  }

  bool IsConstant(const ComputationDataHandle& operand,
                  ComputationBuilder* builder, int64 num_parameters = 0) {
    StatusOr<bool> result = builder->IsConstant(operand, num_parameters);
    EXPECT_TRUE(result.ok()) << result.status();
    return result.ok() ? result.ValueOrDie() : false;
  }

  perftools::gputools::Platform* platform_;
};

TEST_F(ComputeConstantTest, ScalarInt32Literal) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto computation = b.ConstantR0<int32>(42);
    EXPECT_TRUE(IsConstant(computation, &b));

    auto value = ComputeConstantScalar<int32>(client, computation, &b);
    ASSERT_TRUE(value.ok()) << value.status();
    EXPECT_EQ(value.ValueOrDie(), 42);
  }
}

TEST_F(ComputeConstantTest, ScalarFloatAdd) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto computation =
        b.Add(b.ConstantR0<float>(42.5f), b.ConstantR0<float>(1.5f));
    EXPECT_TRUE(IsConstant(computation, &b));

    auto value = ComputeConstantScalar<float>(client, computation, &b);
    ASSERT_TRUE(value.ok()) << value.status();
    EXPECT_EQ(value.ValueOrDie(), 44.0f);
  }
}

TEST_F(ComputeConstantTest, ScalarRng) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto computation =
        b.RngUniform(b.ConstantR0<float>(1.1f), b.ConstantR0<float>(2.1f),
                     ShapeUtil::MakeShape(F32, {}));
    EXPECT_FALSE(IsConstant(computation, &b));

    auto value = ComputeConstantScalar<float>(client, computation, &b);
    ASSERT_FALSE(value.ok())
        << "computing a RNG value should not be considered a constant";
  }
}

TEST_F(ComputeConstantTest, Param) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto param = b.Parameter(0, ShapeUtil::MakeShape(F32, {}), "lhs");
    auto computation = b.Add(param, b.ConstantR0<float>(1.5f));

    std::vector<Literal> arguments;
    arguments.push_back(std::move(*Literal::CreateR0(42.5f)));
    EXPECT_TRUE(IsConstant(computation, &b, arguments.size()));

    auto value =
        ComputeConstantScalar<float>(client, computation, &b, arguments);
    ASSERT_TRUE(value.ok()) << value.status();
    EXPECT_EQ(value.ValueOrDie(), 44.0f);
  }
}

TEST_F(ComputeConstantTest, DirectParamMissing) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto computation = b.Parameter(0, ShapeUtil::MakeShape(F32, {}), "param");
    EXPECT_FALSE(IsConstant(computation, &b));

    auto value = ComputeConstantScalar<float>(client, computation, &b);
    EXPECT_TRUE(tensorflow::StringPiece(value.status().ToString())
                    .contains("depends on a parameter"))
        << value.status();
  }
}

TEST_F(ComputeConstantTest, IndirectParamMissing) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto computation =
        b.Add(b.ConstantR0<float>(1.0f),
              b.Parameter(0, ShapeUtil::MakeShape(F32, {}), "param"));
    EXPECT_FALSE(IsConstant(computation, &b));

    auto value = ComputeConstantScalar<float>(client, computation, &b);
    EXPECT_TRUE(tensorflow::StringPiece(value.status().ToString())
                    .contains("depends on a parameter"))
        << value.status();
  }
}

// Test computation of an expression interspersed with param nodes but
// the expression does not depend on the param nodes.
TEST_F(ComputeConstantTest, UnrelatedParam) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());

    auto param_a = b.Parameter(10, ShapeUtil::MakeShape(F32, {}), "param0");
    auto constant_4 =
        b.Add(b.ConstantR0<float>(2.5f), b.ConstantR0<float>(1.5f));
    auto not_constant_a = b.Add(constant_4, param_a);

    auto param_b = b.Parameter(1, ShapeUtil::MakeShape(F32, {}), "param1");
    auto constant_9 =
        b.Mul(b.ConstantR0<float>(2.0f), b.ConstantR0<float>(4.5f));
    auto not_constant_b = b.Add(param_b, constant_9);

    auto constant_13 = b.Add(constant_4, constant_9);
    b.Add(not_constant_b, b.Add(constant_13, not_constant_a));

    EXPECT_TRUE(IsConstant(constant_13, &b));

    auto value = ComputeConstantScalar<float>(client, constant_13, &b);
    ASSERT_TRUE(value.ok()) << value.status();
    EXPECT_EQ(value.ValueOrDie(), 13.0f);
  }
}

TEST_F(ComputeConstantTest, NonScalarAdd) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());

    auto computation =
        b.Add(b.ConstantR1<int32>({1, 2}), b.ConstantR1<int32>({3, 4}));
    EXPECT_TRUE(IsConstant(computation, &b));

    auto computed = ComputeConstantLiteral(client, computation, &b);
    ASSERT_TRUE(computed.ok()) << computed.status();
    std::unique_ptr<Literal> expected_literal =
        Literal::CreateR1<int32>({4, 6});
    LiteralTestUtil::ExpectEqual(*expected_literal, *computed.ValueOrDie());
  }
}

TEST_F(ComputeConstantTest, IntegerDivide) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());
    auto computation = b.Div(b.ConstantR0<int32>(15), b.ConstantR0<int32>(3));
    EXPECT_TRUE(IsConstant(computation, &b));

    auto computed = ComputeConstantLiteral(client, computation, &b);
    ASSERT_TRUE(computed.ok()) << computed.status();
    std::unique_ptr<Literal> expected_literal = Literal::CreateR0<int32>(5);
    LiteralTestUtil::ExpectEqual(*expected_literal, *computed.ValueOrDie());
  }
}

XLA_TEST_F(ComputeConstantTest, Layout) {
  for (ClientType client_type : client_types) {
    Client* client = ClientOrDie(platform_, client_type);
    ComputationBuilder b(client, TestName());

    std::vector<std::vector<int64>> layouts = {{0, 1}, {1, 0}};
    for (const std::vector<int64>& layout : layouts) {
      auto layout_proto = LayoutUtil::MakeLayout(layout);
      auto computed = ComputeConstantLiteral(
          client,
          b.Add(b.ConstantR2<int32>({{1, 2}, {3, 4}}),
                b.ConstantR2<int32>({{10, 20}, {30, 40}})),
          &b, &layout_proto);
      ASSERT_TRUE(computed.ok()) << computed.status();

      std::unique_ptr<Literal> expected_literal =
          Literal::CreateR2WithLayout<int32>({{11, 22}, {33, 44}},
                                             LayoutUtil::MakeLayout(layout));
      LiteralTestUtil::AssertEqualShapesAndLayouts(
          expected_literal->shape(), computed.ValueOrDie()->shape());
      LiteralTestUtil::ExpectEqual(*expected_literal, *computed.ValueOrDie());
    }
  }
}

}  // namespace
}  // namespace xla
