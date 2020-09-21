/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "IOEventLoop.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

TEST(IOEventLoop, read) {
  int fd[2];
  ASSERT_EQ(0, pipe(fd));
  IOEventLoop loop;
  int count = 0;
  int retry_count = 0;
  ASSERT_NE(nullptr, loop.AddReadEvent(fd[0], [&]() {
    while (true) {
      char c;
      int ret = read(fd[0], &c, 1);
      if (ret == 1) {
        if (++count == 100) {
          return loop.ExitLoop();
        }
      } else if (ret == -1 && errno == EAGAIN) {
        retry_count++;
        break;
      } else {
        return false;
      }
    }
    return true;
  }));
  std::thread thread([&]() {
    for (int i = 0; i < 100; ++i) {
      usleep(1000);
      char c;
      write(fd[1], &c, 1);
    }
  });
  ASSERT_TRUE(loop.RunLoop());
  thread.join();
  ASSERT_EQ(100, count);
  // Test retry_count to make sure we are not doing blocking read.
  ASSERT_GT(retry_count, 0);
  close(fd[0]);
  close(fd[1]);
}

TEST(IOEventLoop, write) {
  int fd[2];
  ASSERT_EQ(0, pipe(fd));
  IOEventLoop loop;
  int count = 0;
  ASSERT_NE(nullptr, loop.AddWriteEvent(fd[1], [&]() {
    int ret = 0;
    char buf[4096];
    while ((ret = write(fd[1], buf, sizeof(buf))) > 0) {
    }
    if (ret == -1 && errno == EAGAIN) {
      if (++count == 100) {
        loop.ExitLoop();
      }
      return true;
    }
    return false;
  }));
  std::thread thread([&]() {
    usleep(500000);
    while (true) {
      usleep(1000);
      char buf[4096];
      if (read(fd[0], buf, sizeof(buf)) <= 0) {
        break;
      }
    }
  });
  ASSERT_TRUE(loop.RunLoop());
  // close fd[1] to make read thread stop.
  close(fd[1]);
  thread.join();
  close(fd[0]);
  ASSERT_EQ(100, count);
}

TEST(IOEventLoop, signal) {
  IOEventLoop loop;
  int count = 0;
  ASSERT_TRUE(loop.AddSignalEvent(SIGINT, [&]() {
    if (++count == 100) {
      loop.ExitLoop();
    }
    return true;
  }));
  std::atomic<bool> stop_thread(false);
  std::thread thread([&]() {
    while (!stop_thread) {
      usleep(1000);
      kill(getpid(), SIGINT);
    }
  });
  ASSERT_TRUE(loop.RunLoop());
  stop_thread = true;
  thread.join();
  ASSERT_EQ(100, count);
}

void TestPeriodicEvents(int period_in_us, int iterations, bool precise) {
  timeval tv;
  tv.tv_sec = period_in_us / 1000000;
  tv.tv_usec = period_in_us % 1000000;
  int count = 0;
  IOEventLoop loop;
  if (precise) {
    ASSERT_TRUE(loop.UsePreciseTimer());
  }
  ASSERT_TRUE(loop.AddPeriodicEvent(tv, [&]() {
    if (++count == iterations) {
      loop.ExitLoop();
    }
    return true;
  }));
  auto start_time = std::chrono::steady_clock::now();
  ASSERT_TRUE(loop.RunLoop());
  auto end_time = std::chrono::steady_clock::now();
  ASSERT_EQ(iterations, count);
  double time_used = std::chrono::duration_cast<std::chrono::duration<double>>(
                         end_time - start_time).count();
  double min_time_in_sec = period_in_us / 1e6 * iterations;
  double max_time_in_sec = min_time_in_sec + (precise ? 0.1 : 1);
  ASSERT_GE(time_used, min_time_in_sec);
  ASSERT_LT(time_used, max_time_in_sec);
}

TEST(IOEventLoop, periodic) {
  TestPeriodicEvents(1000000, 1, false);
}

TEST(IOEventLoop, periodic_precise) {
  TestPeriodicEvents(1000, 100, true);
}

TEST(IOEventLoop, read_and_del_event) {
  int fd[2];
  ASSERT_EQ(0, pipe(fd));
  IOEventLoop loop;
  int count = 0;
  IOEventRef ref = loop.AddReadEvent(fd[0], [&]() {
    count++;
    return IOEventLoop::DelEvent(ref);
  });
  ASSERT_NE(nullptr, ref);

  std::thread thread([&]() {
    for (int i = 0; i < 100; ++i) {
      usleep(1000);
      char c;
      write(fd[1], &c, 1);
    }
  });
  ASSERT_TRUE(loop.RunLoop());
  thread.join();
  ASSERT_EQ(1, count);
  close(fd[0]);
  close(fd[1]);
}

TEST(IOEventLoop, disable_enable_event) {
  int fd[2];
  ASSERT_EQ(0, pipe(fd));
  IOEventLoop loop;
  int count = 0;
  IOEventRef ref = loop.AddWriteEvent(fd[1], [&]() {
    count++;
    return IOEventLoop::DisableEvent(ref);
  });
  ASSERT_NE(nullptr, ref);

  timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  int periodic_count = 0;
  ASSERT_TRUE(loop.AddPeriodicEvent(tv, [&]() {
    periodic_count++;
    if (periodic_count == 1) {
      if (count != 1) {
        return false;
      }
      return IOEventLoop::EnableEvent(ref);
    } else {
      if (count != 2) {
        return false;
      }
      return loop.ExitLoop();
    }
  }));

  ASSERT_TRUE(loop.RunLoop());
  ASSERT_EQ(2, count);
  ASSERT_EQ(2, periodic_count);
  close(fd[0]);
  close(fd[1]);
}
