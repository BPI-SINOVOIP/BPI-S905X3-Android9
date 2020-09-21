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

#include "event_attr.h"
#include "event_type.h"
#include "record.h"
#include "record_equal_test.h"

class RecordTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    const EventType* type = FindEventTypeByName("cpu-cycles");
    ASSERT_TRUE(type != nullptr);
    event_attr = CreateDefaultPerfEventAttr(*type);
    event_attr.sample_id_all = 1;
  }

  void CheckRecordMatchBinary(Record& record) {
    std::vector<std::unique_ptr<Record>> records =
        ReadRecordsFromBuffer(event_attr, record.BinaryForTestingOnly(), record.size());
    ASSERT_EQ(1u, records.size());
    CheckRecordEqual(record, *records[0]);
  }

  perf_event_attr event_attr;
};

TEST_F(RecordTest, MmapRecordMatchBinary) {
  MmapRecord record(event_attr, true, 1, 2, 0x1000, 0x2000, 0x3000,
                    "MmapRecord", 0);
  CheckRecordMatchBinary(record);
}

TEST_F(RecordTest, CommRecordMatchBinary) {
  CommRecord record(event_attr, 1, 2, "CommRecord", 0, 7);
  CheckRecordMatchBinary(record);
}

TEST_F(RecordTest, SampleRecordMatchBinary) {
  event_attr.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME
                           | PERF_SAMPLE_ID | PERF_SAMPLE_CPU
                           | PERF_SAMPLE_PERIOD | PERF_SAMPLE_CALLCHAIN;
  SampleRecord record(event_attr, 1, 2, 3, 4, 5, 6, 7, {8, 9, 10});
  CheckRecordMatchBinary(record);
}

TEST_F(RecordTest, RecordCache_smoke) {
  event_attr.sample_id_all = 1;
  event_attr.sample_type |= PERF_SAMPLE_TIME;
  RecordCache cache(true, 2, 2);

  // Push r1.
  MmapRecord* r1 = new MmapRecord(event_attr, true, 1, 1, 0x100, 0x200, 0x300,
                                  "mmap_record1", 0, 3);
  cache.Push(std::unique_ptr<Record>(r1));
  ASSERT_EQ(nullptr, cache.Pop());

  // Push r2.
  MmapRecord* r2 = new MmapRecord(event_attr, true, 1, 1, 0x100, 0x200, 0x300,
                                  "mmap_record1", 0, 1);
  cache.Push(std::unique_ptr<Record>(r2));
  // Pop r2.
  std::unique_ptr<Record> popped_r = cache.Pop();
  ASSERT_TRUE(popped_r != nullptr);
  ASSERT_EQ(r2, popped_r.get());
  ASSERT_EQ(nullptr, cache.Pop());

  // Push r3.
  MmapRecord* r3 = new MmapRecord(event_attr, true, 1, 1, 0x100, 0x200, 0x300,
                                  "mmap_record1", 0, 4);
  cache.Push(std::unique_ptr<Record>(r3));
  ASSERT_EQ(nullptr, cache.Pop());

  // Push r4.
  MmapRecord* r4 = new MmapRecord(event_attr, true, 1, 1, 0x100, 0x200, 0x300,
                                  "mmap_record1", 0, 6);
  cache.Push(std::unique_ptr<Record>(r4));
  // Pop r1.
  popped_r = cache.Pop();
  ASSERT_TRUE(popped_r != nullptr);
  ASSERT_EQ(r1, popped_r.get());
  // Pop r3.
  popped_r = cache.Pop();
  ASSERT_TRUE(popped_r != nullptr);
  ASSERT_EQ(r3, popped_r.get());
  ASSERT_EQ(nullptr, cache.Pop());
  // Pop r4.
  std::vector<std::unique_ptr<Record>> last_records = cache.PopAll();
  ASSERT_EQ(1u, last_records.size());
  ASSERT_EQ(r4, last_records[0].get());
}

TEST_F(RecordTest, RecordCache_FIFO) {
  event_attr.sample_id_all = 1;
  event_attr.sample_type |= PERF_SAMPLE_TIME;
  RecordCache cache(true, 2, 2);
  std::vector<MmapRecord*> records;
  for (size_t i = 0; i < 10; ++i) {
    records.push_back(new MmapRecord(event_attr, true, 1, i, 0x100, 0x200,
                                     0x300, "mmap_record1", 0));
    cache.Push(std::unique_ptr<Record>(records.back()));
  }
  std::vector<std::unique_ptr<Record>> out_records = cache.PopAll();
  ASSERT_EQ(records.size(), out_records.size());
  for (size_t i = 0; i < records.size(); ++i) {
    ASSERT_EQ(records[i], out_records[i].get());
  }
}

TEST_F(RecordTest, RecordCache_PushRecordVector) {
  event_attr.sample_id_all = 1;
  event_attr.sample_type |= PERF_SAMPLE_TIME;
  RecordCache cache(true, 2, 2);
  MmapRecord* r1 = new MmapRecord(event_attr, true, 1, 1, 0x100, 0x200, 0x300,
                                  "mmap_record1", 0, 1);
  MmapRecord* r2 = new MmapRecord(event_attr, true, 1, 1, 0x100, 0x200, 0x300,
                                  "mmap_record1", 0, 3);
  std::vector<std::unique_ptr<Record>> records;
  records.push_back(std::unique_ptr<Record>(r1));
  records.push_back(std::unique_ptr<Record>(r2));
  cache.Push(std::move(records));
  std::unique_ptr<Record> popped_r = cache.Pop();
  ASSERT_TRUE(popped_r != nullptr);
  ASSERT_EQ(r1, popped_r.get());
  std::vector<std::unique_ptr<Record>> last_records = cache.PopAll();
  ASSERT_EQ(1u, last_records.size());
  ASSERT_EQ(r2, last_records[0].get());
}

TEST_F(RecordTest, SampleRecord_exclude_kernel_callchain) {
  SampleRecord r(event_attr, 0, 1, 0, 0, 0, 0, 0, {});
  ASSERT_EQ(0u, r.ExcludeKernelCallChain());

  event_attr.sample_type |= PERF_SAMPLE_CALLCHAIN;
  SampleRecord r1(event_attr, 0, 1, 0, 0, 0, 0, 0, {PERF_CONTEXT_USER, 2});
  ASSERT_EQ(1u, r1.ExcludeKernelCallChain());
  ASSERT_EQ(2u, r1.ip_data.ip);
  SampleRecord r2(event_attr, r1.BinaryForTestingOnly());
  ASSERT_EQ(1u, r.ip_data.ip);
  ASSERT_EQ(2u, r2.callchain_data.ip_nr);
  ASSERT_EQ(PERF_CONTEXT_USER, r2.callchain_data.ips[0]);
  ASSERT_EQ(2u, r2.callchain_data.ips[1]);

  SampleRecord r3(event_attr, 0, 1, 0, 0, 0, 0, 0, {1, PERF_CONTEXT_USER, 2});
  ASSERT_EQ(1u, r3.ExcludeKernelCallChain());
  ASSERT_EQ(2u, r3.ip_data.ip);
  SampleRecord r4(event_attr, r3.BinaryForTestingOnly());
  ASSERT_EQ(2u, r4.ip_data.ip);
  ASSERT_EQ(3u, r4.callchain_data.ip_nr);
  ASSERT_EQ(PERF_CONTEXT_USER, r4.callchain_data.ips[0]);
  ASSERT_EQ(PERF_CONTEXT_USER, r4.callchain_data.ips[1]);
  ASSERT_EQ(2u, r4.callchain_data.ips[2]);

  SampleRecord r5(event_attr, 0, 1, 0, 0, 0, 0, 0, {1, 2});
  ASSERT_EQ(0u, r5.ExcludeKernelCallChain());
  SampleRecord r6(event_attr, 0, 1, 0, 0, 0, 0, 0, {1, 2, PERF_CONTEXT_USER});
  ASSERT_EQ(0u, r6.ExcludeKernelCallChain());
}
