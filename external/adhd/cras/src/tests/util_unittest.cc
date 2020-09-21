// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "cras_util.h"

namespace {

static std::vector<struct timespec> time_now;

TEST(Util, SendRecvTwoFileDescriptors) {
  int fd[2];
  int fd2[2];
  int send_fds[2];
  int sock[2];
  char buf[256] = {0};
  int new_fds[2];
  char msg[] = "multi-fd";
  unsigned int num_fds = 2;

  /* Create a pipe and a pair of sockets. Then send the write end of
   * the pipe (fd[1]) through the socket, and receive it as
   * new_fd */
  ASSERT_EQ(0, pipe(fd));
  ASSERT_EQ(0, pipe(fd2));
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  send_fds[0] = fd[1];
  send_fds[1] = fd2[1];
  ASSERT_GE(cras_send_with_fds(sock[0], msg, strlen(msg), send_fds, num_fds),
            0);
  ASSERT_GE(cras_recv_with_fds(sock[1], buf, strlen(msg), new_fds, &num_fds),
            0);
  ASSERT_STREQ(msg, buf);
  ASSERT_EQ(2, num_fds);
  ASSERT_NE(-1, new_fds[0]);
  ASSERT_NE(-1, new_fds[1]);

  close(sock[0]);
  close(sock[1]);
  close(fd[1]);
  close(fd2[1]);

  /* Send a character to the new_fd, and receive it from the read end
   * of the pipe (fd[0]) */
  ASSERT_EQ(1, write(new_fds[0], "a", 1));
  ASSERT_EQ(1, read(fd[0], buf, 1));
  ASSERT_EQ('a', buf[0]);
  ASSERT_EQ(1, write(new_fds[1], "b", 1));
  ASSERT_EQ(1, read(fd2[0], buf, 1));
  ASSERT_EQ('b', buf[0]);

  close(fd[0]);
  close(fd2[0]);
  close(new_fds[0]);
  close(new_fds[1]);
}

TEST(Util, SendOneRecvTwoFileDescriptors) {
  int fd[2];
  int sock[2];
  char buf[256] = {0};
  int new_fds[2];
  char msg[] = "multi-fd";
  unsigned int num_fds = 2;

  /* Create a pipe and a pair of sockets. Then send the write end of
   * the pipe (fd[1]) through the socket, and receive it as
   * new_fd */
  ASSERT_EQ(0, pipe(fd));
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  ASSERT_GE(cras_send_with_fds(sock[0], msg, strlen(msg), &fd[1], 1), 0);
  ASSERT_GE(cras_recv_with_fds(sock[1], buf, strlen(msg), new_fds, &num_fds),
            0);
  ASSERT_STREQ(msg, buf);
  ASSERT_EQ(1, num_fds);
  ASSERT_NE(-1, new_fds[0]);
  ASSERT_EQ(-1, new_fds[1]);

  close(sock[0]);
  close(sock[1]);
  close(fd[1]);

  /* Send a character to the new_fd, and receive it from the read end
   * of the pipe (fd[0]) */
  ASSERT_EQ(1, write(new_fds[0], "a", 1));
  ASSERT_EQ(1, read(fd[0], buf, 1));
  ASSERT_EQ('a', buf[0]);

  close(fd[0]);
  close(new_fds[0]);
  close(new_fds[1]);
}

TEST(Util, SendRecvFileDescriptor) {
  int fd[2];
  int sock[2];
  char buf[256] = {0};
  int new_fd;
  char msg[] = "hello";
  unsigned int num_fds = 1;

  /* Create a pipe and a pair of sockets. Then send the write end of
   * the pipe (fd[1]) through the socket, and receive it as
   * new_fd */
  ASSERT_EQ(0, pipe(fd));
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  ASSERT_EQ(5, cras_send_with_fds(sock[0], msg, strlen(msg), &fd[1], num_fds));
  ASSERT_EQ(5,
            cras_recv_with_fds(sock[1], buf, strlen(msg), &new_fd, &num_fds));
  ASSERT_STREQ(msg, buf);
  ASSERT_EQ(1, num_fds);

  close(sock[0]);
  close(sock[1]);
  close(fd[1]);

  /* Send a character to the new_fd, and receive it from the read end
   * of the pipe (fd[0]) */
  ASSERT_EQ(1, write(new_fd, "a", 1));
  ASSERT_EQ(1, read(fd[0], buf, 1));
  ASSERT_EQ('a', buf[0]);

  close(fd[0]);
  close(new_fd);
}

TEST(Util, SendRecvNoDescriptors) {
  char buf[256] = {0};
  char msg[] = "no descriptors";
  unsigned int num_fds = 0;
  int sock[2];

  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sock));

  ASSERT_GE(cras_send_with_fds(sock[0], msg, strlen(msg), NULL, num_fds), 0);
  ASSERT_GE(cras_recv_with_fds(sock[1], buf, strlen(msg), NULL, &num_fds), 0);
  ASSERT_STREQ(msg, buf);
  ASSERT_EQ(0, num_fds);

  close(sock[0]);
  close(sock[1]);
}

TEST(Util, TimevalAfter) {
  struct timeval t0, t1;
  t0.tv_sec = 0;
  t0.tv_usec = 0;
  t1.tv_sec = 0;
  t1.tv_usec = 0;
  ASSERT_FALSE(timeval_after(&t0, &t1));
  ASSERT_FALSE(timeval_after(&t1, &t0));
  t0.tv_usec = 1;
  ASSERT_TRUE(timeval_after(&t0, &t1));
  ASSERT_FALSE(timeval_after(&t1, &t0));
  t1.tv_sec = 1;
  ASSERT_FALSE(timeval_after(&t0, &t1));
  ASSERT_TRUE(timeval_after(&t1, &t0));
}

TEST(Util, FramesToTime) {
  struct timespec t;

  cras_frames_to_time(24000, 48000, &t);
  EXPECT_EQ(0, t.tv_sec);
  EXPECT_EQ(500000000, t.tv_nsec);

  cras_frames_to_time(48000, 48000, &t);
  EXPECT_EQ(1, t.tv_sec);
  EXPECT_EQ(0, t.tv_nsec);

  cras_frames_to_time(60000, 48000, &t);
  EXPECT_EQ(1, t.tv_sec);
  EXPECT_EQ(250000000, t.tv_nsec);

  cras_frames_to_time(191999, 192000, &t);
  EXPECT_EQ(0, t.tv_sec);
  EXPECT_EQ(999994791, t.tv_nsec);
}

TEST(Util, TimeToFrames) {
  struct timespec t;
  unsigned int frames;

  t.tv_sec = 0;
  t.tv_nsec = 500000000;
  frames = cras_time_to_frames(&t, 48000);
  EXPECT_EQ(24000, frames);

  t.tv_sec = 1;
  t.tv_nsec = 500000000;
  frames = cras_time_to_frames(&t, 48000);
  EXPECT_EQ(72000, frames);

  t.tv_sec = 0;
  t.tv_nsec = 0;
  frames = cras_time_to_frames(&t, 48000);
  EXPECT_EQ(0, frames);
}

TEST(Util, FramesToMs) {

  EXPECT_EQ(500, cras_frames_to_ms(24000, 48000));
  EXPECT_EQ(0, cras_frames_to_ms(1, 48000));
  EXPECT_EQ(10, cras_frames_to_ms(480, 48000));
  EXPECT_EQ(10, cras_frames_to_ms(488, 48000));
  EXPECT_EQ(50, cras_frames_to_ms(800, 16000));
}

TEST(Util, TimespecToMs) {
  struct timespec ts;

  ts.tv_sec = 0;
  ts.tv_nsec = 500000000;
  EXPECT_EQ(500, timespec_to_ms(&ts));

  ts.tv_sec = 0;
  ts.tv_nsec = 0;
  EXPECT_EQ(0, timespec_to_ms(&ts));

  ts.tv_sec = 0;
  ts.tv_nsec = 2;
  EXPECT_EQ(1, timespec_to_ms(&ts));

  ts.tv_sec = 0;
  ts.tv_nsec = 10000000;
  EXPECT_EQ(10, timespec_to_ms(&ts));

  ts.tv_sec = 1;
  ts.tv_nsec = 0;
  EXPECT_EQ(1000, timespec_to_ms(&ts));

  ts.tv_sec = 1;
  ts.tv_nsec = 1;
  EXPECT_EQ(1001, timespec_to_ms(&ts));
}

TEST(Util, FramesSinceTime) {
  struct timespec t, tn;
  unsigned int frames;

  t.tv_sec = 0;
  t.tv_nsec = 500000000;

  tn.tv_sec = 2;
  tn.tv_nsec = 0;
  time_now.push_back(tn);
  frames = cras_frames_since_time(&t, 48000);
  EXPECT_EQ(72000, frames);

  tn.tv_sec = 0;
  time_now.push_back(tn);
  frames = cras_frames_since_time(&t, 48000);
  EXPECT_EQ(0, frames);
}

// Test cras_poll().
TEST(Util, CrasPoll) {
  int pipe_fds[2];
  struct pollfd poll_fd;
  std::string output;
  struct timespec timeout;
  char buf[256];

  ASSERT_EQ(0, pipe(pipe_fds));
  poll_fd.fd = pipe_fds[0];
  poll_fd.events = POLLIN;
  ASSERT_NE(0, poll_fd.fd >= 0);

  // Simple poll.
  output = "Hello";
  EXPECT_EQ(output.size() + 1,
            write(pipe_fds[1], output.c_str(), output.size() + 1));
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, NULL, NULL));
  ASSERT_EQ(static_cast<ssize_t>(output.size() + 1),
            read(pipe_fds[0], buf, sizeof(buf)));
  EXPECT_EQ(0, strcmp(output.c_str(), buf));

  // Negative time.
  timeout.tv_sec = 0;
  timeout.tv_nsec = -10000000;
  EXPECT_EQ(-ETIMEDOUT, cras_poll(&poll_fd, 1, &timeout, NULL));
  timeout.tv_sec = -1;
  timeout.tv_nsec = 10000000;
  EXPECT_EQ(-ETIMEDOUT, cras_poll(&poll_fd, 1, &timeout, NULL));

  // Timeout.
  timeout.tv_sec = 0;
  timeout.tv_nsec = 0;
  time_now.push_back(timeout);
  timeout.tv_nsec = 1100000;
  time_now.push_back(timeout);
  timeout.tv_nsec = 1000000;
  EXPECT_EQ(-ETIMEDOUT, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(timeout.tv_nsec, -100000);

  EXPECT_EQ(0, close(pipe_fds[0]));
  EXPECT_EQ(0, close(pipe_fds[1]));
}

/* Stubs */
extern "C" {

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
  std::vector<struct timespec>::iterator i = time_now.begin();
  if (i != time_now.end()) {
    *tp = *i;
    time_now.erase(i);
  }
  else
    memset(tp, 0, sizeof(*tp));
  return 0;
}

}  // extern "C"

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
