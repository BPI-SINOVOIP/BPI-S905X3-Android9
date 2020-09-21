//
// Copyright (C) 2009 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "update_engine/common/action_processor.h"

#include <string>

#include <gtest/gtest.h>

#include "update_engine/common/action.h"
#include "update_engine/common/mock_action.h"

using std::string;

namespace chromeos_update_engine {

using chromeos_update_engine::ActionPipe;

class ActionProcessorTestAction;

template<>
class ActionTraits<ActionProcessorTestAction> {
 public:
  typedef string OutputObjectType;
  typedef string InputObjectType;
};

// This is a simple Action class for testing.
class ActionProcessorTestAction : public Action<ActionProcessorTestAction> {
 public:
  typedef string InputObjectType;
  typedef string OutputObjectType;
  ActionPipe<string>* in_pipe() { return in_pipe_.get(); }
  ActionPipe<string>* out_pipe() { return out_pipe_.get(); }
  ActionProcessor* processor() { return processor_; }
  void PerformAction() {}
  void CompleteAction() {
    ASSERT_TRUE(processor());
    processor()->ActionComplete(this, ErrorCode::kSuccess);
  }
  string Type() const { return "ActionProcessorTestAction"; }
};

namespace {
class MyActionProcessorDelegate : public ActionProcessorDelegate {
 public:
  explicit MyActionProcessorDelegate(const ActionProcessor* processor)
      : processor_(processor),
        processing_done_called_(false),
        processing_stopped_called_(false),
        action_completed_called_(false),
        action_exit_code_(ErrorCode::kError) {}

  virtual void ProcessingDone(const ActionProcessor* processor,
                              ErrorCode code) {
    EXPECT_EQ(processor_, processor);
    EXPECT_FALSE(processing_done_called_);
    processing_done_called_ = true;
  }
  virtual void ProcessingStopped(const ActionProcessor* processor) {
    EXPECT_EQ(processor_, processor);
    EXPECT_FALSE(processing_stopped_called_);
    processing_stopped_called_ = true;
  }
  virtual void ActionCompleted(ActionProcessor* processor,
                               AbstractAction* action,
                               ErrorCode code) {
    EXPECT_EQ(processor_, processor);
    EXPECT_FALSE(action_completed_called_);
    action_completed_called_ = true;
    action_exit_code_ = code;
  }

  const ActionProcessor* processor_;
  bool processing_done_called_;
  bool processing_stopped_called_;
  bool action_completed_called_;
  ErrorCode action_exit_code_;
};
}  // namespace

class ActionProcessorTest : public ::testing::Test {
  void SetUp() override {
    action_processor_.set_delegate(&delegate_);
    // Silence Type() calls used for logging.
    EXPECT_CALL(mock_action_, Type()).Times(testing::AnyNumber());
  }

  void TearDown() override {
    action_processor_.set_delegate(nullptr);
  }

 protected:
  // The ActionProcessor under test.
  ActionProcessor action_processor_;

  MyActionProcessorDelegate delegate_{&action_processor_};

  // Common actions used during most tests.
  testing::StrictMock<MockAction> mock_action_;
  ActionProcessorTestAction action_;
};

TEST_F(ActionProcessorTest, SimpleTest) {
  EXPECT_FALSE(action_processor_.IsRunning());
  action_processor_.EnqueueAction(&action_);
  EXPECT_FALSE(action_processor_.IsRunning());
  EXPECT_FALSE(action_.IsRunning());
  action_processor_.StartProcessing();
  EXPECT_TRUE(action_processor_.IsRunning());
  EXPECT_TRUE(action_.IsRunning());
  EXPECT_EQ(action_processor_.current_action(), &action_);
  action_.CompleteAction();
  EXPECT_FALSE(action_processor_.IsRunning());
  EXPECT_FALSE(action_.IsRunning());
}

TEST_F(ActionProcessorTest, DelegateTest) {
  action_processor_.EnqueueAction(&action_);
  action_processor_.StartProcessing();
  action_.CompleteAction();
  EXPECT_TRUE(delegate_.processing_done_called_);
  EXPECT_TRUE(delegate_.action_completed_called_);
}

TEST_F(ActionProcessorTest, StopProcessingTest) {
  action_processor_.EnqueueAction(&action_);
  action_processor_.StartProcessing();
  action_processor_.StopProcessing();
  EXPECT_TRUE(delegate_.processing_stopped_called_);
  EXPECT_FALSE(delegate_.action_completed_called_);
  EXPECT_FALSE(action_processor_.IsRunning());
  EXPECT_EQ(nullptr, action_processor_.current_action());
}

TEST_F(ActionProcessorTest, ChainActionsTest) {
  // This test doesn't use a delegate since it terminates several actions.
  action_processor_.set_delegate(nullptr);

  ActionProcessorTestAction action1, action2;
  action_processor_.EnqueueAction(&action1);
  action_processor_.EnqueueAction(&action2);
  action_processor_.StartProcessing();
  EXPECT_EQ(&action1, action_processor_.current_action());
  EXPECT_TRUE(action_processor_.IsRunning());
  action1.CompleteAction();
  EXPECT_EQ(&action2, action_processor_.current_action());
  EXPECT_TRUE(action_processor_.IsRunning());
  action2.CompleteAction();
  EXPECT_EQ(nullptr, action_processor_.current_action());
  EXPECT_FALSE(action_processor_.IsRunning());
}

TEST_F(ActionProcessorTest, DtorTest) {
  ActionProcessorTestAction action1, action2;
  {
    ActionProcessor action_processor;
    action_processor.EnqueueAction(&action1);
    action_processor.EnqueueAction(&action2);
    action_processor.StartProcessing();
  }
  EXPECT_EQ(nullptr, action1.processor());
  EXPECT_FALSE(action1.IsRunning());
  EXPECT_EQ(nullptr, action2.processor());
  EXPECT_FALSE(action2.IsRunning());
}

TEST_F(ActionProcessorTest, DefaultDelegateTest) {
  // Just make sure it doesn't crash
  action_processor_.EnqueueAction(&action_);
  action_processor_.StartProcessing();
  action_.CompleteAction();

  action_processor_.EnqueueAction(&action_);
  action_processor_.StartProcessing();
  action_processor_.StopProcessing();
}

// This test suspends and resume the action processor while running one action_.
TEST_F(ActionProcessorTest, SuspendResumeTest) {
  action_processor_.EnqueueAction(&mock_action_);

  testing::InSequence s;
  EXPECT_CALL(mock_action_, PerformAction());
  action_processor_.StartProcessing();

  EXPECT_CALL(mock_action_, SuspendAction());
  action_processor_.SuspendProcessing();
  // Suspending the processor twice should not suspend the action twice.
  action_processor_.SuspendProcessing();

  // IsRunning should return whether there's is an action doing some work, even
  // if it is suspended.
  EXPECT_TRUE(action_processor_.IsRunning());
  EXPECT_EQ(&mock_action_, action_processor_.current_action());

  EXPECT_CALL(mock_action_, ResumeAction());
  action_processor_.ResumeProcessing();

  // Calling ResumeProcessing twice should not affect the action_.
  action_processor_.ResumeProcessing();

  action_processor_.ActionComplete(&mock_action_, ErrorCode::kSuccess);
}

// This test suspends an action that presumably doesn't support suspend/resume
// and it finished before being resumed.
TEST_F(ActionProcessorTest, ActionCompletedWhileSuspendedTest) {
  action_processor_.EnqueueAction(&mock_action_);

  testing::InSequence s;
  EXPECT_CALL(mock_action_, PerformAction());
  action_processor_.StartProcessing();

  EXPECT_CALL(mock_action_, SuspendAction());
  action_processor_.SuspendProcessing();

  // Simulate the action completion while suspended. No other call to
  // |mock_action_| is expected at this point.
  action_processor_.ActionComplete(&mock_action_, ErrorCode::kSuccess);

  // The processing should not be done since the ActionProcessor is suspended
  // and the processing is considered to be still running until resumed.
  EXPECT_FALSE(delegate_.processing_done_called_);
  EXPECT_TRUE(action_processor_.IsRunning());

  action_processor_.ResumeProcessing();
  EXPECT_TRUE(delegate_.processing_done_called_);
  EXPECT_FALSE(delegate_.processing_stopped_called_);
}

TEST_F(ActionProcessorTest, StoppedWhileSuspendedTest) {
  action_processor_.EnqueueAction(&mock_action_);

  testing::InSequence s;
  EXPECT_CALL(mock_action_, PerformAction());
  action_processor_.StartProcessing();
  EXPECT_CALL(mock_action_, SuspendAction());
  action_processor_.SuspendProcessing();

  EXPECT_CALL(mock_action_, TerminateProcessing());
  action_processor_.StopProcessing();
  // Stopping the processing should abort the current execution no matter what.
  EXPECT_TRUE(delegate_.processing_stopped_called_);
  EXPECT_FALSE(delegate_.processing_done_called_);
  EXPECT_FALSE(delegate_.action_completed_called_);
  EXPECT_FALSE(action_processor_.IsRunning());
  EXPECT_EQ(nullptr, action_processor_.current_action());
}

}  // namespace chromeos_update_engine
