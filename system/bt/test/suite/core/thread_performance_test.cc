#include <base/bind.h>
#include <base/logging.h>
#include <base/run_loop.h>
#include <base/threading/thread.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <chrono>
#include <iostream>

#include "osi/include/fixed_queue.h"
#include "osi/include/thread.h"

#define NUM_MESSAGES_TO_SEND 1000000

base::MessageLoop* message_loop;
base::RunLoop* run_loop;
thread_t* thread;

volatile static int counter = 0;
volatile bool loop_ready = false;

void set_loop_ready() { loop_ready = true; }

void callback(fixed_queue_t* queue, void* data) {
  if (queue != nullptr) {
    fixed_queue_dequeue(queue);
  }

  counter++;
}

void run_message_loop(void* UNUSED) {
  message_loop = new base::MessageLoop();
  run_loop = new base::RunLoop();

  message_loop->task_runner()->PostTask(FROM_HERE, base::Bind(&set_loop_ready));

  run_loop->Run();

  delete message_loop;
  message_loop = nullptr;

  delete run_loop;
  run_loop = nullptr;
}

class PerformanceTest : public testing::Test {
 public:
  fixed_queue_t* bt_msg_queue;

  void SetUp() override {
    counter = 0;

    bt_msg_queue = fixed_queue_new(SIZE_MAX);
    thread = thread_new("performance test thread");
    thread_post(thread, run_message_loop, nullptr);
  }

  void TearDown() override {
    fixed_queue_free(bt_msg_queue, NULL);
    bt_msg_queue = nullptr;

    thread_free(thread);
    thread = nullptr;
  }
};

TEST_F(PerformanceTest, message_loop_speed_test) {
  loop_ready = false;
  int test_data = 0;

  std::chrono::steady_clock::time_point start_time =
      std::chrono::steady_clock::now();
  while (!loop_ready) {
  }
  for (int i = 0; i < NUM_MESSAGES_TO_SEND; i++) {
    fixed_queue_enqueue(bt_msg_queue, (void*)&test_data);
    message_loop->task_runner()->PostTask(
        FROM_HERE, base::Bind(&callback, bt_msg_queue, nullptr));
  }

  message_loop->task_runner()->PostTask(FROM_HERE,
                                        run_loop->QuitWhenIdleClosure());
  while (counter < NUM_MESSAGES_TO_SEND) {
  }

  std::chrono::steady_clock::time_point end_time =
      std::chrono::steady_clock::now();
  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time);

  LOG(INFO) << "Message loop took " << duration.count() << "ms for "
            << NUM_MESSAGES_TO_SEND << " messages";
}

TEST_F(PerformanceTest, reactor_thread_speed_test) {
  counter = 0;
  int test_data = 0;

  thread = thread_new("queue performance test thread");
  bt_msg_queue = fixed_queue_new(SIZE_MAX);
  fixed_queue_register_dequeue(bt_msg_queue, thread_get_reactor(thread),
                               callback, NULL);

  std::chrono::steady_clock::time_point start_time =
      std::chrono::steady_clock::now();
  for (int i = 0; i < NUM_MESSAGES_TO_SEND; i++) {
    fixed_queue_enqueue(bt_msg_queue, (void*)&test_data);
  }

  while (counter < NUM_MESSAGES_TO_SEND) {
  };

  std::chrono::steady_clock::time_point end_time =
      std::chrono::steady_clock::now();
  std::chrono::milliseconds duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time);

  LOG(INFO) << "Reactor thread took " << duration.count() << "ms for "
            << NUM_MESSAGES_TO_SEND << " messages";

  fixed_queue_free(bt_msg_queue, NULL);
  bt_msg_queue = nullptr;

  thread_free(thread);
  thread = nullptr;
}
