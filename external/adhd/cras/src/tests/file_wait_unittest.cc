// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string>

#include "cras_util.h"
#include "cras_file_wait.h"

extern "C" {
// This function is not exported in cras_util.h.
void cras_file_wait_mock_race_condition(struct cras_file_wait *file_wait);
}

namespace {

// Executes "rm -rf <path>".
static int RmRF(const std::string& path) {
  std::string cmd("rm -rf \"");
  cmd += path + "\"";

  if (path == "/")
    return -EINVAL;

  int rc = system(cmd.c_str());
  if (rc < 0)
    return -errno;
  return WEXITSTATUS(rc);
}

// Filled-in by the FileWaitCallback.
struct FileWaitResult {
  size_t called;
  cras_file_wait_event_t event;
};

// Called by the file wait code for an event.
static void FileWaitCallback(void *context,
			     cras_file_wait_event_t event,
			     const char *filename)
{
  FileWaitResult *result = reinterpret_cast<FileWaitResult*>(context);
  result->called++;
  result->event = event;
}

// Do all of the EXPECTed steps for a simple wait for one file.
static void SimpleFileWait(const char *file_path) {
  struct cras_file_wait *file_wait;
  FileWaitResult file_wait_result;
  struct pollfd poll_fd;
  struct timespec timeout = {0, 100000000};
  struct stat stat_buf;
  int stat_rc;

  stat_rc = stat(file_path, &stat_buf);
  if (stat_rc < 0)
    stat_rc = -errno;

  file_wait_result.called = 0;
  EXPECT_EQ(0, cras_file_wait_create(file_path, CRAS_FILE_WAIT_FLAG_NONE,
                                     FileWaitCallback, &file_wait_result,
                                     &file_wait));
  EXPECT_NE(reinterpret_cast<struct cras_file_wait *>(NULL), file_wait);
  if (stat_rc == 0) {
    EXPECT_EQ(1, file_wait_result.called);
    EXPECT_EQ(CRAS_FILE_WAIT_EVENT_CREATED, file_wait_result.event);
  } else {
    EXPECT_EQ(0, file_wait_result.called);
  }
  poll_fd.events = POLLIN;
  poll_fd.fd = cras_file_wait_get_fd(file_wait);

  file_wait_result.called = 0;
  if (stat_rc == 0)
    EXPECT_EQ(0, RmRF(file_path));
  else
    EXPECT_EQ(0, mknod(file_path, S_IFREG | 0600, 0));
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  if (stat_rc == 0)
    EXPECT_EQ(CRAS_FILE_WAIT_EVENT_DELETED, file_wait_result.event);
  else
    EXPECT_EQ(CRAS_FILE_WAIT_EVENT_CREATED, file_wait_result.event);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));
  cras_file_wait_destroy(file_wait);
}

// Test the cras_file_wait functions including multiple path components
// missing and path components deleted and recreated.
TEST(Util, FileWait) {
  struct cras_file_wait *file_wait;
  FileWaitResult file_wait_result;
  pid_t pid = getpid();
  struct pollfd poll_fd;
  int current_dir;
  struct timespec timeout = {0, 100000000};
  char pid_buf[32];
  std::string tmp_dir(CRAS_UT_TMPDIR);
  std::string dir_path;
  std::string subdir_path;
  std::string file_path;

  snprintf(pid_buf, sizeof(pid_buf), "%d", pid);
  dir_path = tmp_dir + "/" + pid_buf;
  subdir_path = dir_path + "/subdir";
  file_path = subdir_path + "/does_not_exist";

  // Test arguments.
  // Null file path.
  EXPECT_EQ(-EINVAL, cras_file_wait_create(
                         NULL, CRAS_FILE_WAIT_FLAG_NONE,
                         FileWaitCallback, &file_wait_result, &file_wait));
  // Empty file path.
  EXPECT_EQ(-EINVAL, cras_file_wait_create(
                         "", CRAS_FILE_WAIT_FLAG_NONE,
                         FileWaitCallback, &file_wait_result, &file_wait));
  // No callback structure.
  EXPECT_EQ(-EINVAL, cras_file_wait_create(
                         ".", CRAS_FILE_WAIT_FLAG_NONE,
                         NULL, NULL, &file_wait));
  // No file wait structure.
  EXPECT_EQ(-EINVAL, cras_file_wait_create(
                         ".", CRAS_FILE_WAIT_FLAG_NONE,
                         FileWaitCallback, &file_wait_result, NULL));
  EXPECT_EQ(-EINVAL, cras_file_wait_dispatch(NULL));
  EXPECT_EQ(-EINVAL, cras_file_wait_get_fd(NULL));

  // Make sure that /tmp exists.
  file_wait_result.called = 0;
  EXPECT_EQ(0, cras_file_wait_create(CRAS_UT_TMPDIR, CRAS_FILE_WAIT_FLAG_NONE,
                                     FileWaitCallback, &file_wait_result,
                                     &file_wait));
  EXPECT_NE(reinterpret_cast<struct cras_file_wait *>(NULL), file_wait);
  EXPECT_EQ(file_wait_result.called, 1);
  ASSERT_EQ(file_wait_result.event, CRAS_FILE_WAIT_EVENT_CREATED);
  cras_file_wait_destroy(file_wait);

  // Create our temporary dir.
  ASSERT_EQ(0, RmRF(dir_path));
  ASSERT_EQ(0, mkdir(dir_path.c_str(), 0700));

  // Start looking for our file '.../does_not_exist'.
  EXPECT_EQ(0, cras_file_wait_create(file_path.c_str(),
                                     CRAS_FILE_WAIT_FLAG_NONE,
                                     FileWaitCallback, &file_wait_result,
                                     &file_wait));
  EXPECT_NE(reinterpret_cast<struct cras_file_wait *>(NULL), file_wait);
  poll_fd.events = POLLIN;
  poll_fd.fd = cras_file_wait_get_fd(file_wait);
  EXPECT_NE(0, poll_fd.fd >= 0);

  // Create a sub-directory in the path.
  file_wait_result.called = 0;
  EXPECT_EQ(0, mkdir(subdir_path.c_str(), 0700));
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(0, file_wait_result.called);
  // Removing a watch causes generation of an IN_IGNORED event for the previous
  // watch_id. cras_file_wait_dispatch will ignore this and return 0.
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Remove the directory that we're watching.
  EXPECT_EQ(0, RmRF(subdir_path));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(0, file_wait_result.called);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Create a sub-directory in the path (again).
  EXPECT_EQ(0, mkdir(subdir_path.c_str(), 0700));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(0, file_wait_result.called);
  // See IN_IGNORED above.
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Create the file we're looking for.
  EXPECT_EQ(0, mknod(file_path.c_str(), S_IFREG | 0600, 0));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  EXPECT_EQ(CRAS_FILE_WAIT_EVENT_CREATED, file_wait_result.event);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Remove the file.
  file_wait_result.called = 0;
  EXPECT_EQ(0, unlink(file_path.c_str()));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  EXPECT_EQ(CRAS_FILE_WAIT_EVENT_DELETED, file_wait_result.event);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Re-create the file.
  file_wait_result.called = 0;
  EXPECT_EQ(0, mknod(file_path.c_str(), S_IFREG | 0600, 0));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  EXPECT_EQ(CRAS_FILE_WAIT_EVENT_CREATED, file_wait_result.event);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Remove the subdir.
  file_wait_result.called = 0;
  EXPECT_EQ(0, RmRF(subdir_path));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  EXPECT_EQ(CRAS_FILE_WAIT_EVENT_DELETED, file_wait_result.event);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Create a sub-directory in the path (again), and this time mock a race
  // condition for creation of the file.
  file_wait_result.called = 0;
  EXPECT_EQ(0, mkdir(subdir_path.c_str(), 0700));
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;
  EXPECT_EQ(1, cras_poll(&poll_fd, 1, &timeout, NULL));
  cras_file_wait_mock_race_condition(file_wait);
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  EXPECT_EQ(CRAS_FILE_WAIT_EVENT_CREATED, file_wait_result.event);
  EXPECT_EQ(0, cras_file_wait_dispatch(file_wait));
  EXPECT_EQ(1, file_wait_result.called);
  EXPECT_EQ(-EAGAIN, cras_file_wait_dispatch(file_wait));

  // Cleanup.
  cras_file_wait_destroy(file_wait);

  // Treat consecutive '/' as one.
  file_path = dir_path + "//does_not_exist_too";
  SimpleFileWait(file_path.c_str());

  // Stash the current directory.
  current_dir = open(".", O_RDONLY|O_PATH|O_DIRECTORY);
  ASSERT_NE(0, current_dir >= 0);

  // Search for a file in the current directory.
  ASSERT_EQ(0, chdir(dir_path.c_str()));
  SimpleFileWait("does_not_exist_either");

  // Test notification of deletion in the current directory.
  SimpleFileWait("does_not_exist_either");

  // Search for a file in the current directory (variation).
  SimpleFileWait("./does_not_exist_either_too");

  // Return to the start directory.
  EXPECT_EQ(0, fchdir(current_dir));

  // Clean up.
  EXPECT_EQ(0, RmRF(dir_path));
}

}  //  namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
