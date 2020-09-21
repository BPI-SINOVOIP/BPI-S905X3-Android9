/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/test_utils.h>

#include <thread>

#include "command.h"
#include "environment.h"
#include "event_selection_set.h"
#include "get_test_data.h"
#include "test_util.h"

static std::unique_ptr<Command> StatCmd() {
  return CreateCommandInstance("stat");
}

TEST(stat_cmd, no_options) { ASSERT_TRUE(StatCmd()->Run({"sleep", "1"})); }

TEST(stat_cmd, event_option) {
  ASSERT_TRUE(StatCmd()->Run({"-e", "cpu-clock,task-clock", "sleep", "1"}));
}

TEST(stat_cmd, system_wide_option) {
  TEST_IN_ROOT(ASSERT_TRUE(StatCmd()->Run({"-a", "sleep", "1"})));
}

TEST(stat_cmd, verbose_option) {
  ASSERT_TRUE(StatCmd()->Run({"--verbose", "sleep", "1"}));
}

TEST(stat_cmd, tracepoint_event) {
  TEST_IN_ROOT(ASSERT_TRUE(
      StatCmd()->Run({"-a", "-e", "sched:sched_switch", "sleep", "1"})));
}

TEST(stat_cmd, event_modifier) {
  ASSERT_TRUE(
      StatCmd()->Run({"-e", "cpu-cycles:u,cpu-cycles:k", "sleep", "1"}));
}

void RunWorkloadFunction() {
  while (true) {
    for (volatile int i = 0; i < 10000; ++i);
    usleep(1);
  }
}

void CreateProcesses(size_t count,
                     std::vector<std::unique_ptr<Workload>>* workloads) {
  workloads->clear();
  // Create workloads run longer than profiling time.
  for (size_t i = 0; i < count; ++i) {
    std::unique_ptr<Workload> workload;
    if (GetDefaultAppPackageName().empty()) {
      workload = Workload::CreateWorkload(RunWorkloadFunction);
    } else {
      workload = Workload::CreateWorkload({"run-as", GetDefaultAppPackageName(), "./workload"});
      workload->SetKillFunction([](pid_t pid) {
        Workload::RunCmd({"run-as", GetDefaultAppPackageName(), "kill", std::to_string(pid)});
      });
    }
    ASSERT_TRUE(workload != nullptr);
    ASSERT_TRUE(workload->Start());
    workloads->push_back(std::move(workload));
  }
}

TEST(stat_cmd, existing_processes) {
  std::vector<std::unique_ptr<Workload>> workloads;
  CreateProcesses(2, &workloads);
  std::string pid_list = android::base::StringPrintf(
      "%d,%d", workloads[0]->GetPid(), workloads[1]->GetPid());
  ASSERT_TRUE(StatCmd()->Run({"-p", pid_list, "sleep", "1"}));
}

TEST(stat_cmd, existing_threads) {
  std::vector<std::unique_ptr<Workload>> workloads;
  CreateProcesses(2, &workloads);
  // Process id can be used as thread id in linux.
  std::string tid_list = android::base::StringPrintf(
      "%d,%d", workloads[0]->GetPid(), workloads[1]->GetPid());
  ASSERT_TRUE(StatCmd()->Run({"-t", tid_list, "sleep", "1"}));
}

TEST(stat_cmd, no_monitored_threads) { ASSERT_FALSE(StatCmd()->Run({""})); }

TEST(stat_cmd, group_option) {
  ASSERT_TRUE(
      StatCmd()->Run({"--group", "cpu-clock,page-faults", "sleep", "1"}));
  ASSERT_TRUE(StatCmd()->Run({"--group", "cpu-cycles,instructions", "--group",
                              "cpu-cycles:u,instructions:u", "--group",
                              "cpu-cycles:k,instructions:k", "sleep", "1"}));
}

TEST(stat_cmd, auto_generated_summary) {
  TemporaryFile tmp_file;
  ASSERT_TRUE(StatCmd()->Run({"--group", "instructions:u,instructions:k", "-o",
                              tmp_file.path, "sleep", "1"}));
  std::string s;
  ASSERT_TRUE(android::base::ReadFileToString(tmp_file.path, &s));
  size_t pos = s.find("instructions:u");
  ASSERT_NE(s.npos, pos);
  pos = s.find("instructions:k", pos);
  ASSERT_NE(s.npos, pos);
  pos += strlen("instructions:k");
  // Check if the summary of instructions is generated.
  ASSERT_NE(s.npos, s.find("instructions", pos));
}

TEST(stat_cmd, duration_option) {
  ASSERT_TRUE(
      StatCmd()->Run({"--duration", "1.2", "-p", std::to_string(getpid()), "--in-app"}));
  ASSERT_TRUE(StatCmd()->Run({"--duration", "1", "sleep", "2"}));
}

TEST(stat_cmd, interval_option) {
  TemporaryFile tmp_file;
  ASSERT_TRUE(
    StatCmd()->Run({"--interval", "500.0", "--duration", "1.2", "-o",
          tmp_file.path, "sleep", "2"}));
  std::string s;
  ASSERT_TRUE(android::base::ReadFileToString(tmp_file.path, &s));
  size_t count = 0;
  size_t pos = 0;
  std::string subs = "statistics:";
  while((pos = s.find(subs, pos)) != s.npos) {
    pos += subs.size();
    ++count ;
  }
  ASSERT_EQ(count, 3UL);
}

TEST(stat_cmd, interval_option_in_system_wide) {
  TEST_IN_ROOT(ASSERT_TRUE(StatCmd()->Run({"-a", "--interval", "100", "--duration", "0.3"})));
}

TEST(stat_cmd, no_modifier_for_clock_events) {
  for (const std::string& e : {"cpu-clock", "task-clock"}) {
    for (const std::string& m : {"u", "k"}) {
      ASSERT_FALSE(StatCmd()->Run({"-e", e + ":" + m, "sleep", "0.1"}))
          << "event " << e << ":" << m;
    }
  }
}

TEST(stat_cmd, handle_SIGHUP) {
  std::thread thread([]() {
    sleep(1);
    kill(getpid(), SIGHUP);
  });
  thread.detach();
  ASSERT_TRUE(StatCmd()->Run({"sleep", "1000000"}));
}

TEST(stat_cmd, stop_when_no_more_targets) {
  std::atomic<int> tid(0);
  std::thread thread([&]() {
    tid = gettid();
    sleep(1);
  });
  thread.detach();
  while (tid == 0);
  ASSERT_TRUE(StatCmd()->Run({"-t", std::to_string(tid), "--in-app"}));
}

TEST(stat_cmd, sample_speed_should_be_zero) {
  EventSelectionSet set(true);
  ASSERT_TRUE(set.AddEventType("cpu-cycles"));
  set.AddMonitoredProcesses({getpid()});
  ASSERT_TRUE(set.OpenEventFiles({-1}));
  std::vector<EventAttrWithId> attrs = set.GetEventAttrWithId();
  ASSERT_GT(attrs.size(), 0u);
  for (auto& attr : attrs) {
    ASSERT_EQ(attr.attr->sample_period, 0u);
    ASSERT_EQ(attr.attr->sample_freq, 0u);
    ASSERT_EQ(attr.attr->freq, 0u);
  }
}
