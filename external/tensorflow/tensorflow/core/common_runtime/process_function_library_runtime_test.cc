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
#include "tensorflow/core/common_runtime/process_function_library_runtime.h"

#include <vector>

#include "tensorflow/core/common_runtime/device_factory.h"
#include "tensorflow/core/common_runtime/function_testlib.h"
#include "tensorflow/core/common_runtime/rendezvous_mgr.h"
#include "tensorflow/core/framework/function_testlib.h"
#include "tensorflow/core/framework/tensor_testutil.h"
#include "tensorflow/core/lib/core/status_test_util.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/public/session_options.h"
#include "tensorflow/core/public/version.h"

namespace tensorflow {
namespace {

class ProcessFunctionLibraryRuntimeTest : public ::testing::Test {
 protected:
  void Init(const std::vector<FunctionDef>& flib) {
    SessionOptions options;
    auto* device_count = options.config.mutable_device_count();
    device_count->insert({"CPU", 2});
    TF_CHECK_OK(DeviceFactory::AddDevices(options, "/job:a/replica:0/task:0",
                                          &devices_));
    device_mgr_.reset(new DeviceMgr(devices_));
    FunctionDefLibrary proto;
    for (const auto& fdef : flib) *(proto.add_function()) = fdef;
    lib_def_.reset(new FunctionLibraryDefinition(OpRegistry::Global(), proto));
    OptimizerOptions opts;
    proc_flr_.reset(new ProcessFunctionLibraryRuntime(
        device_mgr_.get(), Env::Default(), TF_GRAPH_DEF_VERSION, lib_def_.get(),
        opts, nullptr /* cluster_flr */));
    rendezvous_ = new IntraProcessRendezvous(device_mgr_.get());
  }

  Status Run(const string& name, FunctionLibraryRuntime::Options opts,
             test::function::Attrs attrs,
             const FunctionLibraryRuntime::InstantiateOptions& instantiate_opts,
             const std::vector<Tensor>& args, std::vector<Tensor*> rets) {
    FunctionLibraryRuntime::Handle handle;
    Status status =
        proc_flr_->Instantiate(name, attrs, instantiate_opts, &handle);
    if (!status.ok()) {
      return status;
    }

    std::atomic<int32> call_count(0);
    std::function<void(std::function<void()>)> runner =
        [&call_count](std::function<void()> fn) {
          ++call_count;
          test::function::FunctionTestSchedClosure(fn);
        };

    Notification done;
    opts.runner = &runner;
    std::vector<Tensor> out;
    proc_flr_->Run(opts, handle, args, &out, [&status, &done](const Status& s) {
      status = s;
      done.Notify();
    });
    done.WaitForNotification();
    if (!status.ok()) {
      return status;
    }
    CHECK_EQ(rets.size(), out.size());
    for (size_t i = 0; i < rets.size(); ++i) {
      *rets[i] = out[i];
    }

    EXPECT_GE(call_count, 1);  // Test runner is used.

    // Release the handle and then try running the function. It shouldn't
    // succeed.
    status = proc_flr_->ReleaseHandle(handle);
    if (!status.ok()) {
      return status;
    }
    Notification done2;
    proc_flr_->Run(opts, handle, args, &out,
                   [&status, &done2](const Status& s) {
                     status = s;
                     done2.Notify();
                   });
    done2.WaitForNotification();
    EXPECT_TRUE(errors::IsNotFound(status));
    EXPECT_TRUE(StringPiece(status.error_message()).contains("not found."));

    return Status::OK();
  }

  std::vector<Device*> devices_;
  std::unique_ptr<DeviceMgr> device_mgr_;
  std::unique_ptr<FunctionLibraryDefinition> lib_def_;
  std::unique_ptr<ProcessFunctionLibraryRuntime> proc_flr_;
  IntraProcessRendezvous* rendezvous_;
};

TEST_F(ProcessFunctionLibraryRuntimeTest, GetFLRNull) {
  FunctionDefLibrary proto;
  std::unique_ptr<FunctionLibraryDefinition> lib_def(
      new FunctionLibraryDefinition(OpRegistry::Global(), proto));
  OptimizerOptions opts;
  std::unique_ptr<ProcessFunctionLibraryRuntime> proc_flr(
      new ProcessFunctionLibraryRuntime(
          nullptr /* device_mgr */, Env::Default(), TF_GRAPH_DEF_VERSION,
          lib_def.get(), opts, nullptr /* cluster_flr */));
  FunctionLibraryRuntime* flr =
      proc_flr->GetFLR(ProcessFunctionLibraryRuntime::kDefaultFLRDevice);
  EXPECT_NE(flr, nullptr);
}

TEST_F(ProcessFunctionLibraryRuntimeTest, Basic) {
  Init({});
  FunctionLibraryRuntime* flr =
      proc_flr_->GetFLR("/job:a/replica:0/task:0/cpu:0");
  EXPECT_NE(flr, nullptr);
  EXPECT_EQ(flr->device(), devices_[0]);
  flr = proc_flr_->GetFLR("/job:a/replica:0/task:0/device:CPU:0");
  EXPECT_NE(flr, nullptr);
  EXPECT_EQ(flr->device(), devices_[0]);
  flr = proc_flr_->GetFLR("/device:CPU:0");
  EXPECT_NE(flr, nullptr);
  EXPECT_EQ(flr->device(), devices_[0]);
  flr = proc_flr_->GetFLR("/job:a/replica:0/task:0/cpu:1");
  EXPECT_NE(flr, nullptr);
  EXPECT_EQ(flr->device(), devices_[1]);
  flr = proc_flr_->GetFLR("abc");
  EXPECT_EQ(flr, nullptr);
  rendezvous_->Unref();
}

TEST_F(ProcessFunctionLibraryRuntimeTest, GetDeviceIncarnation) {
  Init({});
  int64 incarnation;
  TF_EXPECT_OK(proc_flr_->GetDeviceIncarnation("/job:a/replica:0/task:0/cpu:1",
                                               &incarnation));
  // Incarnation is a random number other than 0.
  EXPECT_NE(incarnation, 0);
  Status s = proc_flr_->GetDeviceIncarnation("/job:a/replica:0/task:0/cpu:2",
                                             &incarnation);
  EXPECT_EQ(s.code(), error::INVALID_ARGUMENT);
  rendezvous_->Unref();
}

TEST_F(ProcessFunctionLibraryRuntimeTest, SingleCall) {
  Init({test::function::XTimesTwo()});
  FunctionLibraryRuntime::Options opts;
  opts.source_device = "/job:a/replica:0/task:0/cpu:0";
  opts.rendezvous = rendezvous_;
  opts.remote_execution = true;
  FunctionLibraryRuntime::InstantiateOptions instantiate_opts;
  instantiate_opts.target = "/job:a/replica:0/task:0/cpu:0";
  auto x = test::AsTensor<float>({1, 2, 3, 4});
  Tensor y;
  TF_CHECK_OK(
      Run("XTimesTwo", opts, {{"T", DT_FLOAT}}, instantiate_opts, {x}, {&y}));
  test::ExpectTensorEqual<float>(y, test::AsTensor<float>({2, 4, 6, 8}));
  rendezvous_->Unref();
}

TEST_F(ProcessFunctionLibraryRuntimeTest, SingleCallFindDevice) {
  Init({test::function::FindDevice()});
  FunctionLibraryRuntime::Options opts;
  opts.source_device = "/job:a/replica:0/task:0/cpu:0";
  opts.rendezvous = rendezvous_;
  opts.remote_execution = true;
  FunctionLibraryRuntime::InstantiateOptions instantiate_opts;
  instantiate_opts.target = "/job:a/replica:0/task:0/cpu:0";
  Tensor y;
  TF_CHECK_OK(Run("FindDevice", opts, {}, instantiate_opts, {}, {&y}));
  test::ExpectTensorEqual<string>(
      y, test::AsTensor<string>({"/job:a/replica:0/task:0/device:CPU:0"},
                                TensorShape({})));
  rendezvous_->Unref();
}

TEST_F(ProcessFunctionLibraryRuntimeTest, MultipleCallsSameDeviceXTimes) {
  Init({test::function::XTimesTwo(), test::function::XTimesFour()});
  auto x = test::AsTensor<float>({1, 2, 3, 4});
  FunctionLibraryRuntime::Options opts;
  opts.source_device = "/job:a/replica:0/task:0/cpu:0";
  opts.rendezvous = rendezvous_;
  opts.remote_execution = true;
  FunctionLibraryRuntime::InstantiateOptions instantiate_opts;
  instantiate_opts.target = "/job:a/replica:0/task:0/cpu:0";
  Tensor y;
  TF_CHECK_OK(
      Run("XTimesTwo", opts, {{"T", DT_FLOAT}}, instantiate_opts, {x}, {&y}));
  test::ExpectTensorEqual<float>(y, test::AsTensor<float>({2, 4, 6, 8}));
  TF_CHECK_OK(
      Run("XTimesFour", opts, {{"T", DT_FLOAT}}, instantiate_opts, {x}, {&y}));
  test::ExpectTensorEqual<float>(y, test::AsTensor<float>({4, 8, 12, 16}));
  rendezvous_->Unref();
}

TEST_F(ProcessFunctionLibraryRuntimeTest, MultipleCallsSameDeviceFindDevice) {
  Init({test::function::FindDevice()});
  FunctionLibraryRuntime::Options opts;
  opts.source_device = "/job:a/replica:0/task:0/cpu:0";
  opts.rendezvous = rendezvous_;
  opts.remote_execution = true;
  FunctionLibraryRuntime::InstantiateOptions instantiate_opts;
  instantiate_opts.target = "/job:a/replica:0/task:0/cpu:1";
  Tensor y;
  TF_CHECK_OK(Run("FindDevice", opts, {}, instantiate_opts, {}, {&y}));
  test::ExpectTensorEqual<string>(
      y, test::AsTensor<string>({"/job:a/replica:0/task:0/device:CPU:1"},
                                TensorShape({})));
  TF_CHECK_OK(Run("FindDevice", opts, {}, instantiate_opts, {}, {&y}));
  test::ExpectTensorEqual<string>(
      y, test::AsTensor<string>({"/job:a/replica:0/task:0/device:CPU:1"},
                                TensorShape({})));
  rendezvous_->Unref();
}

TEST_F(ProcessFunctionLibraryRuntimeTest, MultipleCallsDiffDeviceFindDevice) {
  Init({test::function::FindDevice()});
  FunctionLibraryRuntime::Options opts;
  opts.source_device = "/job:a/replica:0/task:0/cpu:0";
  opts.rendezvous = rendezvous_;
  opts.remote_execution = true;
  Tensor y;
  FunctionLibraryRuntime::InstantiateOptions instantiate_opts_0;
  instantiate_opts_0.target = "/job:a/replica:0/task:0/device:CPU:0";
  TF_CHECK_OK(Run("FindDevice", opts, {}, instantiate_opts_0, {}, {&y}));
  test::ExpectTensorEqual<string>(
      y, test::AsTensor<string>({"/job:a/replica:0/task:0/device:CPU:0"},
                                TensorShape({})));
  FunctionLibraryRuntime::InstantiateOptions instantiate_opts_1;
  instantiate_opts_1.target = "/job:a/replica:0/task:0/device:CPU:1";
  TF_CHECK_OK(Run("FindDevice", opts, {}, instantiate_opts_1, {}, {&y}));
  test::ExpectTensorEqual<string>(
      y, test::AsTensor<string>({"/job:a/replica:0/task:0/device:CPU:1"},
                                TensorShape({})));
  rendezvous_->Unref();
}

}  // anonymous namespace
}  // namespace tensorflow
