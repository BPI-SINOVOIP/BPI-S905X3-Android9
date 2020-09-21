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

#include <limits.h>

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "Config.h"

#include "log_fake.h"

class MallocDebugConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetLogs();
  }

  std::unique_ptr<Config> config;

  bool InitConfig(const char* options) {
    config.reset(new Config);
    return config->Init(options);
  }
};

std::string usage_string(
  "6 malloc_debug malloc debug options usage:\n"
  "6 malloc_debug \n"
  "6 malloc_debug   front_guard[=XX]\n"
  "6 malloc_debug     Enables a front guard on all allocations. If XX is set\n"
  "6 malloc_debug     it sets the number of bytes in the guard. The default is\n"
  "6 malloc_debug     32 bytes, the max bytes is 16384.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   rear_guard[=XX]\n"
  "6 malloc_debug     Enables a rear guard on all allocations. If XX is set\n"
  "6 malloc_debug     it sets the number of bytes in the guard. The default is\n"
  "6 malloc_debug     32 bytes, the max bytes is 16384.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   guard[=XX]\n"
  "6 malloc_debug     Enables both a front guard and a rear guard on all allocations.\n"
  "6 malloc_debug     If XX is set it sets the number of bytes in both guards.\n"
  "6 malloc_debug     The default is 32 bytes, the max bytes is 16384.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   backtrace[=XX]\n"
  "6 malloc_debug     Enable capturing the backtrace at the point of allocation.\n"
  "6 malloc_debug     If XX is set it sets the number of backtrace frames.\n"
  "6 malloc_debug     This option also enables dumping the backtrace heap data\n"
  "6 malloc_debug     when a signal is received. The data is dumped to the file\n"
  "6 malloc_debug     backtrace_dump_prefix.<PID>.txt.\n"
  "6 malloc_debug     The default is 16 frames, the max number of frames is 256.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   backtrace_enable_on_signal[=XX]\n"
  "6 malloc_debug     Enable capturing the backtrace at the point of allocation.\n"
  "6 malloc_debug     The backtrace capture is not enabled until the process\n"
  "6 malloc_debug     receives a signal. If XX is set it sets the number of backtrace\n"
  "6 malloc_debug     frames. The default is 16 frames, the max number of frames is 256.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   backtrace_dump_prefix[=FILE]\n"
  "6 malloc_debug     This option only has meaning if the backtrace option has been specified.\n"
  "6 malloc_debug     This is the prefix of the name of the file to which backtrace heap\n"
  "6 malloc_debug     data will be dumped. The file will be named backtrace_dump_prefix.<PID>.txt.\n"
  "6 malloc_debug     The default is /data/local/tmp/backtrace_heap.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   backtrace_dump_on_exit\n"
  "6 malloc_debug     This option only has meaning if the backtrace option has been specified.\n"
  "6 malloc_debug     This will cause all live allocations to be dumped to the file\n"
  "6 malloc_debug     backtrace_dump_prefix.<PID>.final.txt.\n"
  "6 malloc_debug     The default is false.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   fill_on_alloc[=XX]\n"
  "6 malloc_debug     On first allocation, fill with the value 0xeb.\n"
  "6 malloc_debug     If XX is set it will only fill up to XX bytes of the\n"
  "6 malloc_debug     allocation. The default is to fill the entire allocation.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   fill_on_free[=XX]\n"
  "6 malloc_debug     On free, fill with the value 0xef. If XX is set it will\n"
  "6 malloc_debug     only fill up to XX bytes of the allocation. The default is to\n"
  "6 malloc_debug     fill the entire allocation.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   fill[=XX]\n"
  "6 malloc_debug     On both first allocation free, fill with the value 0xeb on\n"
  "6 malloc_debug     first allocation and the value 0xef. If XX is set, only fill\n"
  "6 malloc_debug     up to XX bytes. The default is to fill the entire allocation.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   expand_alloc[=XX]\n"
  "6 malloc_debug     Allocate an extra number of bytes for every allocation call.\n"
  "6 malloc_debug     If XX is set, that is the number of bytes to expand the\n"
  "6 malloc_debug     allocation by. The default is 16 bytes, the max bytes is 16384.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   free_track[=XX]\n"
  "6 malloc_debug     When a pointer is freed, do not free the memory right away.\n"
  "6 malloc_debug     Instead, keep XX of these allocations around and then verify\n"
  "6 malloc_debug     that they have not been modified when the total number of freed\n"
  "6 malloc_debug     allocations exceeds the XX amount. When the program terminates,\n"
  "6 malloc_debug     the rest of these allocations are verified. When this option is\n"
  "6 malloc_debug     enabled, it automatically records the backtrace at the time of the free.\n"
  "6 malloc_debug     The default is to record 100 allocations, the max allocations\n"
  "6 malloc_debug     to record is 16384.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   free_track_backtrace_num_frames[=XX]\n"
  "6 malloc_debug     This option only has meaning if free_track is set. This indicates\n"
  "6 malloc_debug     how many backtrace frames to capture when an allocation is freed.\n"
  "6 malloc_debug     If XX is set, that is the number of frames to capture. If XX\n"
  "6 malloc_debug     is set to zero, then no backtrace will be captured.\n"
  "6 malloc_debug     The default is to record 16 frames, the max number of frames is 256.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   leak_track\n"
  "6 malloc_debug     Enable the leak tracking of memory allocations.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   record_allocs[=XX]\n"
  "6 malloc_debug     Record every single allocation/free call. When a specific signal\n"
  "6 malloc_debug     is sent to the process, the contents of recording are written to\n"
  "6 malloc_debug     a file (/data/local/tmp/record_allocs.txt) and the recording is cleared.\n"
  "6 malloc_debug     If XX is set, that is the total number of allocations/frees that can\n"
  "6 malloc_debug     recorded. of frames to capture. The default value is 8000000.\n"
  "6 malloc_debug     If the allocation list fills up, all further allocations are not recorded.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   record_allocs_file[=FILE]\n"
  "6 malloc_debug     This option only has meaning if the record_allocs options has been specified.\n"
  "6 malloc_debug     This is the name of the file to which recording information will be dumped.\n"
  "6 malloc_debug     The default is /data/local/tmp/record_allocs.txt.\n"
  "6 malloc_debug \n"
  "6 malloc_debug   verify_pointers\n"
  "6 malloc_debug     A lightweight way to verify that free/malloc_usable_size/realloc\n"
  "6 malloc_debug     are passed valid pointers.\n"
);

TEST_F(MallocDebugConfigTest, unknown_option) {

  ASSERT_FALSE(InitConfig("unknown_option"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg("6 malloc_debug malloc_testing: unknown option unknown_option\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, good_option_and_bad_option) {
  ASSERT_FALSE(InitConfig("backtrace unknown_option"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg("6 malloc_debug malloc_testing: unknown option unknown_option\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, unparseable_number) {
  ASSERT_FALSE(InitConfig("backtrace=XXX"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg("6 malloc_debug malloc_testing: bad value for option 'backtrace'\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, illegal_value_zero) {
  ASSERT_FALSE(InitConfig("backtrace=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace', value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, no_space) {
  ASSERT_FALSE(InitConfig("backtrace=10front_guard"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace', "
      "non space found after option: front_guard\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, illegal_value_negative) {
  ASSERT_FALSE(InitConfig("backtrace=-1"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace', "
      "value cannot be negative: -1\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, overflow) {
  ASSERT_FALSE(InitConfig("backtrace=99999999999999999999"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace': "
      "Math result not representable\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, set_value_error) {
  ASSERT_FALSE(InitConfig("leak_track=12"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: value set for option 'leak_track' "
      "which does not take a value\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, space_before_equal) {
  ASSERT_TRUE(InitConfig("backtrace  =10")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(10U, config->backtrace_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, space_after_equal) {
  ASSERT_TRUE(InitConfig("backtrace=  10")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(10U, config->backtrace_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, extra_space) {
  ASSERT_TRUE(InitConfig("   backtrace=64   ")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(64U, config->backtrace_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, multiple_options) {
  ASSERT_TRUE(InitConfig("  backtrace=64   front_guard=48")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS | FRONT_GUARD, config->options());
  ASSERT_EQ(64U, config->backtrace_frames());
  ASSERT_EQ(48U, config->front_guard_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, front_guard) {
  ASSERT_TRUE(InitConfig("front_guard=48")) << getFakeLogPrint();
  ASSERT_EQ(FRONT_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(48U, config->front_guard_bytes());

  ASSERT_TRUE(InitConfig("front_guard")) << getFakeLogPrint();
  ASSERT_EQ(FRONT_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(32U, config->front_guard_bytes());

  ASSERT_TRUE(InitConfig("front_guard=39")) << getFakeLogPrint();
  ASSERT_EQ(FRONT_GUARD | TRACK_ALLOCS, config->options());
#if defined(__LP64__)
  ASSERT_EQ(48U, config->front_guard_bytes());
#else
  ASSERT_EQ(40U, config->front_guard_bytes());
#endif

  ASSERT_TRUE(InitConfig("front_guard=41")) << getFakeLogPrint();
  ASSERT_EQ(FRONT_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(48U, config->front_guard_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, rear_guard) {
  ASSERT_TRUE(InitConfig("rear_guard=50")) << getFakeLogPrint();
  ASSERT_EQ(REAR_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(50U, config->rear_guard_bytes());

  ASSERT_TRUE(InitConfig("rear_guard")) << getFakeLogPrint();
  ASSERT_EQ(REAR_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(32U, config->rear_guard_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, guard) {
  ASSERT_TRUE(InitConfig("guard=32")) << getFakeLogPrint();
  ASSERT_EQ(FRONT_GUARD | REAR_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(32U, config->front_guard_bytes());
  ASSERT_EQ(32U, config->rear_guard_bytes());

  ASSERT_TRUE(InitConfig("guard")) << getFakeLogPrint();
  ASSERT_EQ(FRONT_GUARD | REAR_GUARD | TRACK_ALLOCS, config->options());
  ASSERT_EQ(32U, config->front_guard_bytes());
  ASSERT_EQ(32U, config->rear_guard_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace) {
  ASSERT_TRUE(InitConfig("backtrace=64")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(64U, config->backtrace_frames());
  ASSERT_TRUE(config->backtrace_enabled());
  ASSERT_FALSE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_TRUE(InitConfig("backtrace")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(16U, config->backtrace_frames());
  ASSERT_TRUE(config->backtrace_enabled());
  ASSERT_FALSE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_enable_on_signal) {
  ASSERT_TRUE(InitConfig("backtrace_enable_on_signal=64")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(64U, config->backtrace_frames());
  ASSERT_FALSE(config->backtrace_enabled());
  ASSERT_TRUE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_TRUE(InitConfig("backtrace_enable_on_signal")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(16U, config->backtrace_frames());
  ASSERT_FALSE(config->backtrace_enabled());
  ASSERT_TRUE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_enable_on_signal_init) {
  ASSERT_TRUE(InitConfig("backtrace_enable_on_signal=64")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(64U, config->backtrace_frames());
  ASSERT_FALSE(config->backtrace_enabled());
  ASSERT_TRUE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_TRUE(InitConfig("backtrace")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(16U, config->backtrace_frames());
  ASSERT_TRUE(config->backtrace_enabled());
  ASSERT_FALSE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_enable_and_backtrace) {
  ASSERT_TRUE(InitConfig("backtrace_enable_on_signal backtrace")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(16U, config->backtrace_frames());
  ASSERT_TRUE(config->backtrace_enabled());
  ASSERT_TRUE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_TRUE(InitConfig("backtrace backtrace_enable_on_signal")) << getFakeLogPrint();
  ASSERT_EQ(BACKTRACE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(16U, config->backtrace_frames());
  ASSERT_TRUE(config->backtrace_enabled());
  ASSERT_TRUE(config->backtrace_enable_on_signal());
  ASSERT_FALSE(config->backtrace_dump_on_exit());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_dump_on_exit) {
  ASSERT_TRUE(InitConfig("backtrace_dump_on_exit")) << getFakeLogPrint();
  ASSERT_EQ(0U, config->options());
  ASSERT_TRUE(config->backtrace_dump_on_exit());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_dump_on_exit_error) {
  ASSERT_FALSE(InitConfig("backtrace_dump_on_exit=something")) << getFakeLogPrint();

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: value set for option 'backtrace_dump_on_exit' "
      "which does not take a value\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_dump_prefix) {
  ASSERT_TRUE(InitConfig("backtrace_dump_prefix")) << getFakeLogPrint();
  ASSERT_EQ(0U, config->options());
  ASSERT_EQ("/data/local/tmp/backtrace_heap", config->backtrace_dump_prefix());

  ASSERT_TRUE(InitConfig("backtrace_dump_prefix=/fake/location")) << getFakeLogPrint();
  ASSERT_EQ(0U, config->options());
  ASSERT_EQ("/fake/location", config->backtrace_dump_prefix());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, fill_on_alloc) {
  ASSERT_TRUE(InitConfig("fill_on_alloc=64")) << getFakeLogPrint();
  ASSERT_EQ(FILL_ON_ALLOC, config->options());
  ASSERT_EQ(64U, config->fill_on_alloc_bytes());

  ASSERT_TRUE(InitConfig("fill_on_alloc")) << getFakeLogPrint();
  ASSERT_EQ(FILL_ON_ALLOC, config->options());
  ASSERT_EQ(SIZE_MAX, config->fill_on_alloc_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, fill_on_free) {
  ASSERT_TRUE(InitConfig("fill_on_free=64")) << getFakeLogPrint();
  ASSERT_EQ(FILL_ON_FREE, config->options());
  ASSERT_EQ(64U, config->fill_on_free_bytes());

  ASSERT_TRUE(InitConfig("fill_on_free")) << getFakeLogPrint();
  ASSERT_EQ(FILL_ON_FREE, config->options());
  ASSERT_EQ(SIZE_MAX, config->fill_on_free_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, fill) {
  ASSERT_TRUE(InitConfig("fill=64")) << getFakeLogPrint();
  ASSERT_EQ(FILL_ON_ALLOC | FILL_ON_FREE, config->options());
  ASSERT_EQ(64U, config->fill_on_alloc_bytes());
  ASSERT_EQ(64U, config->fill_on_free_bytes());

  ASSERT_TRUE(InitConfig("fill")) << getFakeLogPrint();
  ASSERT_EQ(FILL_ON_ALLOC | FILL_ON_FREE, config->options());
  ASSERT_EQ(SIZE_MAX, config->fill_on_alloc_bytes());
  ASSERT_EQ(SIZE_MAX, config->fill_on_free_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, expand_alloc) {
  ASSERT_TRUE(InitConfig("expand_alloc=1234")) << getFakeLogPrint();
  ASSERT_EQ(EXPAND_ALLOC, config->options());
  ASSERT_EQ(1234U, config->expand_alloc_bytes());

  ASSERT_TRUE(InitConfig("expand_alloc")) << getFakeLogPrint();
  ASSERT_EQ(EXPAND_ALLOC, config->options());
  ASSERT_EQ(16U, config->expand_alloc_bytes());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track) {
  ASSERT_TRUE(InitConfig("free_track=1234")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(1234U, config->free_track_allocations());
  ASSERT_EQ(SIZE_MAX, config->fill_on_free_bytes());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  ASSERT_TRUE(InitConfig("free_track")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(100U, config->free_track_allocations());
  ASSERT_EQ(SIZE_MAX, config->fill_on_free_bytes());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_and_fill_on_free) {
  ASSERT_TRUE(InitConfig("free_track=1234 fill_on_free=32")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(1234U, config->free_track_allocations());
  ASSERT_EQ(32U, config->fill_on_free_bytes());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  ASSERT_TRUE(InitConfig("free_track fill_on_free=60")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(100U, config->free_track_allocations());
  ASSERT_EQ(60U, config->fill_on_free_bytes());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  // Now reverse the arguments.
  ASSERT_TRUE(InitConfig("fill_on_free=32 free_track=1234")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(1234U, config->free_track_allocations());
  ASSERT_EQ(32U, config->fill_on_free_bytes());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_backtrace_num_frames) {
  ASSERT_TRUE(InitConfig("free_track_backtrace_num_frames=123")) << getFakeLogPrint();

  ASSERT_EQ(0U, config->options());
  ASSERT_EQ(123U, config->free_track_backtrace_num_frames());

  ASSERT_TRUE(InitConfig("free_track_backtrace_num_frames")) << getFakeLogPrint();
  ASSERT_EQ(0U, config->options());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_backtrace_num_frames_zero) {
  ASSERT_TRUE(InitConfig("free_track_backtrace_num_frames=0")) << getFakeLogPrint();

  ASSERT_EQ(0U, config->options());
  ASSERT_EQ(0U, config->free_track_backtrace_num_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_backtrace_num_frames_and_free_track) {
  ASSERT_TRUE(InitConfig("free_track free_track_backtrace_num_frames=123")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(123U, config->free_track_backtrace_num_frames());

  ASSERT_TRUE(InitConfig("free_track free_track_backtrace_num_frames")) << getFakeLogPrint();
  ASSERT_EQ(FREE_TRACK | FILL_ON_FREE | TRACK_ALLOCS, config->options());
  ASSERT_EQ(16U, config->free_track_backtrace_num_frames());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, leak_track) {
  ASSERT_TRUE(InitConfig("leak_track")) << getFakeLogPrint();
  ASSERT_EQ(LEAK_TRACK | TRACK_ALLOCS, config->options());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, leak_track_fail) {
  ASSERT_FALSE(InitConfig("leak_track=100")) << getFakeLogPrint();

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: value set for option 'leak_track' "
      "which does not take a value\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, verify_pointers) {
  ASSERT_TRUE(InitConfig("verify_pointers")) << getFakeLogPrint();
  ASSERT_EQ(TRACK_ALLOCS, config->options());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, verify_pointers_fail) {
  ASSERT_FALSE(InitConfig("verify_pointers=200")) << getFakeLogPrint();

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: value set for option 'verify_pointers' "
      "which does not take a value\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, record_allocs) {
  ASSERT_TRUE(InitConfig("record_allocs=1234")) << getFakeLogPrint();
  ASSERT_EQ(RECORD_ALLOCS, config->options());
  ASSERT_EQ(1234U, config->record_allocs_num_entries());
  ASSERT_STREQ("/data/local/tmp/record_allocs.txt", config->record_allocs_file().c_str());

  ASSERT_TRUE(InitConfig("record_allocs")) << getFakeLogPrint();
  ASSERT_EQ(RECORD_ALLOCS, config->options());
  ASSERT_EQ(8000000U, config->record_allocs_num_entries());
  ASSERT_STREQ("/data/local/tmp/record_allocs.txt", config->record_allocs_file().c_str());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, record_allocs_file) {
  ASSERT_TRUE(InitConfig("record_allocs=1234 record_allocs_file=/fake/file")) << getFakeLogPrint();
  ASSERT_STREQ("/fake/file", config->record_allocs_file().c_str());

  ASSERT_TRUE(InitConfig("record_allocs_file")) << getFakeLogPrint();
  ASSERT_STREQ("/data/local/tmp/record_allocs.txt", config->record_allocs_file().c_str());

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  ASSERT_STREQ("", getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, guard_min_error) {
  ASSERT_FALSE(InitConfig("guard=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'guard', value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, guard_max_error) {
  ASSERT_FALSE(InitConfig("guard=20000"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'guard', "
      "value must be <= 16384: 20000\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, front_guard_min_error) {
  ASSERT_FALSE(InitConfig("front_guard=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'front_guard', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, front_guard_max_error) {
  ASSERT_FALSE(InitConfig("front_guard=20000"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'front_guard', "
      "value must be <= 16384: 20000\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, rear_guard_min_error) {
  ASSERT_FALSE(InitConfig("rear_guard=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'rear_guard', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, rear_guard_max_error) {
  ASSERT_FALSE(InitConfig("rear_guard=20000"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'rear_guard', "
      "value must be <= 16384: 20000\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, fill_min_error) {
  ASSERT_FALSE(InitConfig("fill=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'fill', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, fill_on_alloc_min_error) {
  ASSERT_FALSE(InitConfig("fill_on_alloc=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'fill_on_alloc', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, fill_on_free_min_error) {
  ASSERT_FALSE(InitConfig("fill_on_free=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'fill_on_free', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_min_error) {
  ASSERT_FALSE(InitConfig("backtrace=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_max_error) {
  ASSERT_FALSE(InitConfig("backtrace=300"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace', "
      "value must be <= 256: 300\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_enable_on_signal_min_error) {
  ASSERT_FALSE(InitConfig("backtrace_enable_on_signal=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace_enable_on_signal', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, backtrace_enable_on_signal_max_error) {
  ASSERT_FALSE(InitConfig("backtrace_enable_on_signal=300"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'backtrace_enable_on_signal', "
      "value must be <= 256: 300\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, expand_alloc_min_error) {
  ASSERT_FALSE(InitConfig("expand_alloc=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'expand_alloc', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, expand_alloc_max_error) {
  ASSERT_FALSE(InitConfig("expand_alloc=21000"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'expand_alloc', "
      "value must be <= 16384: 21000\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_min_error) {
  ASSERT_FALSE(InitConfig("free_track=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'free_track', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_max_error) {
  ASSERT_FALSE(InitConfig("free_track=21000"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'free_track', "
      "value must be <= 16384: 21000\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, free_track_backtrace_num_frames_max_error) {
  ASSERT_FALSE(InitConfig("free_track_backtrace_num_frames=400"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'free_track_backtrace_num_frames', "
      "value must be <= 256: 400\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, record_alloc_min_error) {
  ASSERT_FALSE(InitConfig("record_allocs=0"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'record_allocs', "
      "value must be >= 1: 0\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}

TEST_F(MallocDebugConfigTest, record_allocs_max_error) {
  ASSERT_FALSE(InitConfig("record_allocs=100000000"));

  ASSERT_STREQ("", getFakeLogBuf().c_str());
  std::string log_msg(
      "6 malloc_debug malloc_testing: bad value for option 'record_allocs', "
      "value must be <= 50000000: 100000000\n");
  ASSERT_STREQ((log_msg + usage_string).c_str(), getFakeLogPrint().c_str());
}
