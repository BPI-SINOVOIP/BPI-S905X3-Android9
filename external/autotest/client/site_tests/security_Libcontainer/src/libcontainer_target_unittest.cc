// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

#include <base/at_exit.h>
#include <base/files/file_util.h>
#include <base/files/scoped_file.h>
#include <base/files/scoped_temp_dir.h>
#include <base/macros.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>
#include <gtest/gtest.h>
#include <libcontainer.h>
#include <libminijail.h>

namespace libcontainer {

namespace {

// A small RAII class that redirects stdout while it's alive. It also gets the
// first 4k of the output.
class ScopedCaptureStdout {
 public:
  ScopedCaptureStdout() {
    original_stdout_fd_.reset(dup(STDOUT_FILENO));
    CHECK(original_stdout_fd_.is_valid());
    int pipe_fds[2];
    CHECK(pipe2(pipe_fds, O_NONBLOCK) != -1);
    read_fd_.reset(pipe_fds[0]);
    CHECK(dup2(pipe_fds[1], STDOUT_FILENO) != -1);
    CHECK(close(pipe_fds[1]) != -1);
  }

  ~ScopedCaptureStdout() {
    CHECK(dup2(original_stdout_fd_.get(), STDOUT_FILENO) != -1);
  }

  std::string GetContents() {
    char buffer[4096];
    ssize_t read_bytes = read(read_fd_.get(), buffer, sizeof(buffer) - 1);
    CHECK(read_bytes >= 0);
    buffer[read_bytes] = '\0';
    return std::string(buffer, read_bytes);
  }

 private:
  base::ScopedFD read_fd_;
  base::ScopedFD original_stdout_fd_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCaptureStdout);
};

}  // namespace

class LibcontainerTargetTest : public ::testing::Test {
 public:
  LibcontainerTargetTest() = default;
  ~LibcontainerTargetTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    base::FilePath rootfs;
    ASSERT_TRUE(base::CreateTemporaryDirInDir(
        temp_dir_.path(), FILE_PATH_LITERAL("rootfs"), &rootfs));

    config_ = container_config_create();
    ASSERT_NE(nullptr, config_);

    ASSERT_EQ(0, container_config_uid_map(config_, "0 0 429496729"));
    ASSERT_EQ(0, container_config_gid_map(config_, "0 0 429496729"));
    ASSERT_EQ(0, container_config_rootfs(config_, "/"));
    ASSERT_EQ(0, container_config_set_cgroup_parent(
                     config_, "chronos_containers", 1000, 1000));

    container_ = container_new("containerUT", rootfs.value().c_str());
    ASSERT_NE(nullptr, container_);
  }

  void TearDown() override {
    container_destroy(container_);
    container_ = nullptr;
    container_config_destroy(config_);
    config_ = nullptr;
    ASSERT_TRUE(temp_dir_.Delete());
  }

  struct container* container() {
    return container_;
  }
  struct container_config* config() {
    return config_;
  }

 private:
  base::ScopedTempDir temp_dir_;
  struct container* container_ = nullptr;
  struct container_config* config_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(LibcontainerTargetTest);
};

TEST_F(LibcontainerTargetTest, AddHookRedirectTest) {
  // Preserve stdout/stderr to get the output from the container.
  int stdio_fds[] = {STDOUT_FILENO, STDERR_FILENO};
  ASSERT_EQ(0, container_config_inherit_fds(config(), stdio_fds,
                                            arraysize(stdio_fds)));

  static const char* kPreChrootArgv[] = {
      "/bin/cat",
  };
  int stdin_fd;
  ASSERT_EQ(0, container_config_add_hook(
                   config(), MINIJAIL_HOOK_EVENT_PRE_CHROOT, kPreChrootArgv[0],
                   kPreChrootArgv, arraysize(kPreChrootArgv), &stdin_fd,
                   nullptr, nullptr));
  EXPECT_EQ(1, write(stdin_fd, "1", 1));
  close(stdin_fd);

  static const char* kProgramArgv[] = {
      "/bin/echo",
      "-n",
      "2",
  };
  ASSERT_EQ(0, container_config_program_argv(config(), kProgramArgv,
                                             arraysize(kProgramArgv)));

  std::string output;
  {
    ScopedCaptureStdout capture_stdout;
    EXPECT_EQ(0, container_start(container(), config()));
    EXPECT_EQ(0, container_wait(container()));
    output = capture_stdout.GetContents();
  }
  EXPECT_EQ("12", output);
}

TEST_F(LibcontainerTargetTest, AddHookOrderTest) {
  // Preserve stdout/stderr to get the output from the container.
  int stdio_fds[] = {STDOUT_FILENO, STDERR_FILENO};
  ASSERT_EQ(0, container_config_inherit_fds(config(), stdio_fds,
                                            arraysize(stdio_fds)));

  static const char* kProgramArgv[] = {
      "/bin/echo",
      "-n",
      "3",
  };
  ASSERT_EQ(0, container_config_program_argv(config(), kProgramArgv,
                                             arraysize(kProgramArgv)));

  // Hooks are run in the following order: pre-chroot, pre-dropcaps, pre-execve
  static const char* kPreExecveArgv[] = {
      "/bin/echo",
      "-n",
      "2",
  };
  ASSERT_EQ(0, container_config_add_hook(
                   config(), MINIJAIL_HOOK_EVENT_PRE_EXECVE, kPreExecveArgv[0],
                   kPreExecveArgv, arraysize(kPreExecveArgv), nullptr, nullptr,
                   nullptr));

  static const char* kPreChrootArgv[] = {
      "/bin/echo",
      "-n",
      "1",
  };
  ASSERT_EQ(0, container_config_add_hook(
                   config(), MINIJAIL_HOOK_EVENT_PRE_CHROOT, kPreChrootArgv[0],
                   kPreChrootArgv, arraysize(kPreChrootArgv), nullptr, nullptr,
                   nullptr));

  std::string output;
  {
    ScopedCaptureStdout capture_stdout;
    EXPECT_EQ(0, container_start(container(), config()));
    EXPECT_EQ(0, container_wait(container()));
    output = capture_stdout.GetContents();
  }
  EXPECT_EQ("123", output);
}

TEST_F(LibcontainerTargetTest, AddHookPidArgument) {
  // Preserve stdout/stderr to get the output from the container.
  int stdio_fds[] = {STDOUT_FILENO, STDERR_FILENO};
  ASSERT_EQ(0, container_config_inherit_fds(config(), stdio_fds,
                                            arraysize(stdio_fds)));

  static const char* kProgramArgv[] = {
      "/bin/true",
  };
  ASSERT_EQ(0, container_config_program_argv(config(), kProgramArgv,
                                             arraysize(kProgramArgv)));

  static const char* kPreExecveArgv[] = {
      "/bin/echo",
      "-n",
      "$PID",
  };
  ASSERT_EQ(0, container_config_add_hook(
                   config(), MINIJAIL_HOOK_EVENT_PRE_EXECVE, kPreExecveArgv[0],
                   kPreExecveArgv, arraysize(kPreExecveArgv), nullptr, nullptr,
                   nullptr));

  std::string output;
  int pid;
  {
    ScopedCaptureStdout capture_stdout;
    EXPECT_EQ(0, container_start(container(), config()));
    pid = container_pid(container());
    EXPECT_EQ(0, container_wait(container()));
    output = capture_stdout.GetContents();
  }
  EXPECT_EQ(base::IntToString(pid), output);
}

}  // namespace libcontainer

// Avoid including syslog.h, since it collides with some of the logging
// constants in libchrome.
#define SYSLOG_LOG_INFO 6

int main(int argc, char** argv) {
  base::AtExitManager exit_manager;
  testing::InitGoogleTest(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  minijail_log_to_fd(STDERR_FILENO, SYSLOG_LOG_INFO);
  return RUN_ALL_TESTS();
}
