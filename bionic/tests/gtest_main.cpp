/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>

#ifndef TEMP_FAILURE_RETRY

/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

#endif

static std::string g_executable_path;
static int g_argc;
static char** g_argv;
static char** g_envp;

const std::string& get_executable_path() {
  return g_executable_path;
}

int get_argc() {
  return g_argc;
}

char** get_argv() {
  return g_argv;
}

char** get_envp() {
  return g_envp;
}

namespace testing {
namespace internal {

// Reuse of testing::internal::ColoredPrintf in gtest.
enum GTestColor {
  COLOR_DEFAULT,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW
};

void ColoredPrintf(GTestColor color, const char* fmt, ...);

}  // namespace internal
}  // namespace testing

using testing::internal::GTestColor;
using testing::internal::COLOR_RED;
using testing::internal::COLOR_GREEN;
using testing::internal::COLOR_YELLOW;
using testing::internal::ColoredPrintf;

constexpr int DEFAULT_GLOBAL_TEST_RUN_DEADLINE_MS = 90000;
constexpr int DEFAULT_GLOBAL_TEST_RUN_SLOW_THRESHOLD_MS = 2000;

// The time each test can run before killed for the reason of timeout.
// It takes effect only with --isolate option.
static int global_test_run_deadline_ms = DEFAULT_GLOBAL_TEST_RUN_DEADLINE_MS;

// The time each test can run before be warned for too much running time.
// It takes effect only with --isolate option.
static int global_test_run_slow_threshold_ms = DEFAULT_GLOBAL_TEST_RUN_SLOW_THRESHOLD_MS;

// Return timeout duration for a test, in ms.
static int GetTimeoutMs(const std::string& /*test_name*/) {
  return global_test_run_deadline_ms;
}

// Return threshold for calling a test slow, in ms.
static int GetSlowThresholdMs(const std::string& /*test_name*/) {
  return global_test_run_slow_threshold_ms;
}

static void PrintHelpInfo() {
  printf("Bionic Unit Test Options:\n"
         "  -j [JOB_COUNT] or -j[JOB_COUNT]\n"
         "      Run up to JOB_COUNT tests in parallel.\n"
         "      Use isolation mode, Run each test in a separate process.\n"
         "      If JOB_COUNT is not given, it is set to the count of available processors.\n"
         "  --no-isolate\n"
         "      Don't use isolation mode, run all tests in a single process.\n"
         "  --deadline=[TIME_IN_MS]\n"
         "      Run each test in no longer than [TIME_IN_MS] time.\n"
         "      Only valid in isolation mode. Default deadline is 90000 ms.\n"
         "  --slow-threshold=[TIME_IN_MS]\n"
         "      Test running longer than [TIME_IN_MS] will be called slow.\n"
         "      Only valid in isolation mode. Default slow threshold is 2000 ms.\n"
         "  --gtest-filter=POSITIVE_PATTERNS[-NEGATIVE_PATTERNS]\n"
         "      Used as a synonym for --gtest_filter option in gtest.\n"
         "Default bionic unit test option is -j.\n"
         "In isolation mode, you can send SIGQUIT to the parent process to show current\n"
         "running tests, or send SIGINT to the parent process to stop testing and\n"
         "clean up current running tests.\n"
         "\n");
}

enum TestResult {
  TEST_SUCCESS = 0,
  TEST_FAILED,
  TEST_TIMEOUT
};

class Test {
 public:
  Test() {} // For std::vector<Test>.
  explicit Test(const char* name) : name_(name) {}

  const std::string& GetName() const { return name_; }

  void SetResult(TestResult result) {
    // Native xfails are inherently likely to actually be relying on undefined
    // behavior/uninitialized memory, and thus likely to pass from time to time
    // on CTS. Avoid that unpleasantness by just rewriting all xfail failures
    // as successes. You'll still see the actual failure details.
    if (GetName().find("xfail") == 0) result = TEST_SUCCESS;
    result_ = result;
  }

  TestResult GetResult() const { return result_; }

  void SetTestTime(int64_t elapsed_time_ns) { elapsed_time_ns_ = elapsed_time_ns; }

  int64_t GetTestTime() const { return elapsed_time_ns_; }

  void AppendTestOutput(const std::string& s) { output_ += s; }

  const std::string& GetTestOutput() const { return output_; }

 private:
  const std::string name_;
  TestResult result_;
  int64_t elapsed_time_ns_;
  std::string output_;
};

class TestCase {
 public:
  TestCase() {} // For std::vector<TestCase>.
  explicit TestCase(const char* name) : name_(name) {}

  const std::string& GetName() const { return name_; }

  void AppendTest(const char* test_name) {
    test_list_.push_back(Test(test_name));
  }

  size_t TestCount() const { return test_list_.size(); }

  std::string GetTestName(size_t test_id) const {
    VerifyTestId(test_id);
    return name_ + "." + test_list_[test_id].GetName();
  }

  Test& GetTest(size_t test_id) {
    VerifyTestId(test_id);
    return test_list_[test_id];
  }

  const Test& GetTest(size_t test_id) const {
    VerifyTestId(test_id);
    return test_list_[test_id];
  }

  void SetTestResult(size_t test_id, TestResult result) {
    VerifyTestId(test_id);
    test_list_[test_id].SetResult(result);
  }

  TestResult GetTestResult(size_t test_id) const {
    VerifyTestId(test_id);
    return test_list_[test_id].GetResult();
  }

  bool GetTestSuccess(size_t test_id) const {
    return GetTestResult(test_id) == TEST_SUCCESS;
  }

  void SetTestTime(size_t test_id, int64_t elapsed_time_ns) {
    VerifyTestId(test_id);
    test_list_[test_id].SetTestTime(elapsed_time_ns);
  }

  int64_t GetTestTime(size_t test_id) const {
    VerifyTestId(test_id);
    return test_list_[test_id].GetTestTime();
  }

 private:
  void VerifyTestId(size_t test_id) const {
    if(test_id >= test_list_.size()) {
      fprintf(stderr, "test_id %zu out of range [0, %zu)\n", test_id, test_list_.size());
      exit(1);
    }
  }

 private:
  const std::string name_;
  std::vector<Test> test_list_;
};

class TestResultPrinter : public testing::EmptyTestEventListener {
 public:
  TestResultPrinter() : pinfo_(NULL) {}
  virtual void OnTestStart(const testing::TestInfo& test_info) {
    pinfo_ = &test_info; // Record test_info for use in OnTestPartResult.
  }
  virtual void OnTestPartResult(const testing::TestPartResult& result);

 private:
  const testing::TestInfo* pinfo_;
};

// Called after an assertion failure.
void TestResultPrinter::OnTestPartResult(const testing::TestPartResult& result) {
  // If the test part succeeded, we don't need to do anything.
  if (result.type() == testing::TestPartResult::kSuccess)
    return;

  // Print failure message from the assertion (e.g. expected this and got that).
  printf("%s:(%d) Failure in test %s.%s\n%s\n", result.file_name(), result.line_number(),
         pinfo_->test_case_name(), pinfo_->name(), result.message());
  fflush(stdout);
}

static int64_t NanoTime() {
  std::chrono::nanoseconds duration(std::chrono::steady_clock::now().time_since_epoch());
  return static_cast<int64_t>(duration.count());
}

static bool EnumerateTests(int argc, char** argv, std::vector<TestCase>& testcase_list) {
  std::vector<const char*> args(argv, argv + argc);
  args.push_back("--gtest_list_tests");
  args.push_back(nullptr);

  // We use posix_spawn(3) rather than the simpler popen(3) because we don't want an intervening
  // surprise shell invocation making quoting interesting for --gtest_filter (http://b/68949647).

  android::base::unique_fd read_fd;
  android::base::unique_fd write_fd;
  if (!android::base::Pipe(&read_fd, &write_fd)) {
    perror("pipe");
    return false;
  }

  posix_spawn_file_actions_t fa;
  posix_spawn_file_actions_init(&fa);
  posix_spawn_file_actions_addclose(&fa, read_fd);
  posix_spawn_file_actions_adddup2(&fa, write_fd, 1);
  posix_spawn_file_actions_adddup2(&fa, write_fd, 2);
  posix_spawn_file_actions_addclose(&fa, write_fd);

  pid_t pid;
  int result = posix_spawnp(&pid, argv[0], &fa, nullptr, const_cast<char**>(args.data()), nullptr);
  posix_spawn_file_actions_destroy(&fa);
  if (result == -1) {
    perror("posix_spawn");
    return false;
  }
  write_fd.reset();

  std::string content;
  if (!android::base::ReadFdToString(read_fd, &content)) {
    perror("ReadFdToString");
    return false;
  }

  for (auto& line : android::base::Split(content, "\n")) {
    line = android::base::Split(line, "#")[0];
    line = android::base::Trim(line);
    if (line.empty()) continue;
    if (android::base::EndsWith(line, ".")) {
      line.pop_back();
      testcase_list.push_back(TestCase(line.c_str()));
    } else {
      testcase_list.back().AppendTest(line.c_str());
    }
  }

  int status;
  if (TEMP_FAILURE_RETRY(waitpid(pid, &status, 0)) != pid) {
    perror("waitpid");
    return false;
  }
  return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

// Part of the following *Print functions are copied from external/gtest/src/gtest.cc:
// PrettyUnitTestResultPrinter. The reason for copy is that PrettyUnitTestResultPrinter
// is defined and used in gtest.cc, which is hard to reuse.
static void OnTestIterationStartPrint(const std::vector<TestCase>& testcase_list, size_t iteration,
                                      int iteration_count, size_t job_count) {
  if (iteration_count != 1) {
    printf("\nRepeating all tests (iteration %zu) . . .\n\n", iteration);
  }
  ColoredPrintf(COLOR_GREEN,  "[==========] ");

  size_t testcase_count = testcase_list.size();
  size_t test_count = 0;
  for (const auto& testcase : testcase_list) {
    test_count += testcase.TestCount();
  }

  printf("Running %zu %s from %zu %s (%zu %s).\n",
         test_count, (test_count == 1) ? "test" : "tests",
         testcase_count, (testcase_count == 1) ? "test case" : "test cases",
         job_count, (job_count == 1) ? "job" : "jobs");
  fflush(stdout);
}

// bionic cts test needs gtest output format.
#if defined(USING_GTEST_OUTPUT_FORMAT)

static void OnTestEndPrint(const TestCase& testcase, size_t test_id) {
  ColoredPrintf(COLOR_GREEN, "[ RUN      ] ");
  printf("%s\n", testcase.GetTestName(test_id).c_str());

  const std::string& test_output = testcase.GetTest(test_id).GetTestOutput();
  printf("%s", test_output.c_str());

  TestResult result = testcase.GetTestResult(test_id);
  if (result == TEST_SUCCESS) {
    ColoredPrintf(COLOR_GREEN, "[       OK ] ");
  } else {
    ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
  }
  printf("%s", testcase.GetTestName(test_id).c_str());
  if (testing::GTEST_FLAG(print_time)) {
    printf(" (%" PRId64 " ms)", testcase.GetTestTime(test_id) / 1000000);
  }
  printf("\n");
  fflush(stdout);
}

#else  // !defined(USING_GTEST_OUTPUT_FORMAT)

static void OnTestEndPrint(const TestCase& testcase, size_t test_id) {
  TestResult result = testcase.GetTestResult(test_id);
  if (result == TEST_SUCCESS) {
    ColoredPrintf(COLOR_GREEN, "[    OK    ] ");
  } else if (result == TEST_FAILED) {
    ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
  } else if (result == TEST_TIMEOUT) {
    ColoredPrintf(COLOR_RED, "[ TIMEOUT  ] ");
  }

  printf("%s", testcase.GetTestName(test_id).c_str());
  if (testing::GTEST_FLAG(print_time)) {
    printf(" (%" PRId64 " ms)", testcase.GetTestTime(test_id) / 1000000);
  }
  printf("\n");

  const std::string& test_output = testcase.GetTest(test_id).GetTestOutput();
  printf("%s", test_output.c_str());
  fflush(stdout);
}

#endif  // !defined(USING_GTEST_OUTPUT_FORMAT)

static void OnTestIterationEndPrint(const std::vector<TestCase>& testcase_list, size_t /*iteration*/,
                                    int64_t elapsed_time_ns) {

  std::vector<std::string> fail_test_name_list;
  std::vector<std::pair<std::string, int64_t>> timeout_test_list;

  // For tests that were slow but didn't time out.
  std::vector<std::tuple<std::string, int64_t, int>> slow_test_list;
  size_t testcase_count = testcase_list.size();
  size_t test_count = 0;
  size_t success_test_count = 0;
  size_t expected_failure_count = 0;

  for (const auto& testcase : testcase_list) {
    test_count += testcase.TestCount();
    for (size_t i = 0; i < testcase.TestCount(); ++i) {
      TestResult result = testcase.GetTestResult(i);
      if (result == TEST_TIMEOUT) {
        timeout_test_list.push_back(
            std::make_pair(testcase.GetTestName(i), testcase.GetTestTime(i)));
      } else if (result == TEST_SUCCESS) {
        ++success_test_count;
        if (testcase.GetTestName(i).find(".xfail_") != std::string::npos) ++expected_failure_count;
      } else if (result == TEST_FAILED) {
          fail_test_name_list.push_back(testcase.GetTestName(i));
      }
      if (result != TEST_TIMEOUT &&
          testcase.GetTestTime(i) / 1000000 >= GetSlowThresholdMs(testcase.GetTestName(i))) {
        slow_test_list.push_back(std::make_tuple(testcase.GetTestName(i),
                                                 testcase.GetTestTime(i),
                                                 GetSlowThresholdMs(testcase.GetTestName(i))));
      }
    }
  }

  ColoredPrintf(COLOR_GREEN,  "[==========] ");
  printf("%zu %s from %zu %s ran.", test_count, (test_count == 1) ? "test" : "tests",
                                    testcase_count, (testcase_count == 1) ? "test case" : "test cases");
  if (testing::GTEST_FLAG(print_time)) {
    printf(" (%" PRId64 " ms total)", elapsed_time_ns / 1000000);
  }
  printf("\n");
  ColoredPrintf(COLOR_GREEN,  "[   PASS   ] ");
  printf("%zu %s.", success_test_count, (success_test_count == 1) ? "test" : "tests");
  if (expected_failure_count > 0) {
    printf(" (%zu expected failure%s.)", expected_failure_count,
           (expected_failure_count == 1) ? "" : "s");
  }
  printf("\n");

  // Print tests that timed out.
  size_t timeout_test_count = timeout_test_list.size();
  if (timeout_test_count > 0) {
    ColoredPrintf(COLOR_RED, "[ TIMEOUT  ] ");
    printf("%zu %s, listed below:\n", timeout_test_count, (timeout_test_count == 1) ? "test" : "tests");
    for (const auto& timeout_pair : timeout_test_list) {
      ColoredPrintf(COLOR_RED, "[ TIMEOUT  ] ");
      printf("%s (stopped at %" PRId64 " ms)\n", timeout_pair.first.c_str(),
                                                 timeout_pair.second / 1000000);
    }
  }

  // Print tests that were slow.
  size_t slow_test_count = slow_test_list.size();
  if (slow_test_count > 0) {
    ColoredPrintf(COLOR_YELLOW, "[   SLOW   ] ");
    printf("%zu %s, listed below:\n", slow_test_count, (slow_test_count == 1) ? "test" : "tests");
    for (const auto& slow_tuple : slow_test_list) {
      ColoredPrintf(COLOR_YELLOW, "[   SLOW   ] ");
      printf("%s (%" PRId64 " ms, exceeded %d ms)\n", std::get<0>(slow_tuple).c_str(),
             std::get<1>(slow_tuple) / 1000000, std::get<2>(slow_tuple));
    }
  }

  // Print tests that failed.
  size_t fail_test_count = fail_test_name_list.size();
  if (fail_test_count > 0) {
    ColoredPrintf(COLOR_RED,  "[   FAIL   ] ");
    printf("%zu %s, listed below:\n", fail_test_count, (fail_test_count == 1) ? "test" : "tests");
    for (const auto& name : fail_test_name_list) {
      ColoredPrintf(COLOR_RED, "[   FAIL   ] ");
      printf("%s\n", name.c_str());
    }
  }

  if (timeout_test_count > 0 || slow_test_count > 0 || fail_test_count > 0) {
    printf("\n");
  }

  if (timeout_test_count > 0) {
    printf("%2zu TIMEOUT %s\n", timeout_test_count, (timeout_test_count == 1) ? "TEST" : "TESTS");
  }
  if (slow_test_count > 0) {
    printf("%2zu SLOW %s\n", slow_test_count, (slow_test_count == 1) ? "TEST" : "TESTS");
  }
  if (fail_test_count > 0) {
    printf("%2zu FAILED %s\n", fail_test_count, (fail_test_count == 1) ? "TEST" : "TESTS");
  }

  fflush(stdout);
}

std::string XmlEscape(const std::string& xml) {
  std::string escaped;
  escaped.reserve(xml.size());

  for (auto c : xml) {
    switch (c) {
    case '<':
      escaped.append("&lt;");
      break;
    case '>':
      escaped.append("&gt;");
      break;
    case '&':
      escaped.append("&amp;");
      break;
    case '\'':
      escaped.append("&apos;");
      break;
    case '"':
      escaped.append("&quot;");
      break;
    default:
      escaped.append(1, c);
      break;
    }
  }

  return escaped;
}

// Output xml file when --gtest_output is used, write this function as we can't reuse
// gtest.cc:XmlUnitTestResultPrinter. The reason is XmlUnitTestResultPrinter is totally
// defined in gtest.cc and not expose to outside. What's more, as we don't run gtest in
// the parent process, we don't have gtest classes which are needed by XmlUnitTestResultPrinter.
void OnTestIterationEndXmlPrint(const std::string& xml_output_filename,
                                const std::vector<TestCase>& testcase_list,
                                time_t epoch_iteration_start_time,
                                int64_t elapsed_time_ns) {
  FILE* fp = fopen(xml_output_filename.c_str(), "we");
  if (fp == NULL) {
    fprintf(stderr, "failed to open '%s': %s\n", xml_output_filename.c_str(), strerror(errno));
    exit(1);
  }

  size_t total_test_count = 0;
  size_t total_failed_count = 0;
  std::vector<size_t> failed_count_list(testcase_list.size(), 0);
  std::vector<int64_t> elapsed_time_list(testcase_list.size(), 0);
  for (size_t i = 0; i < testcase_list.size(); ++i) {
    auto& testcase = testcase_list[i];
    total_test_count += testcase.TestCount();
    for (size_t j = 0; j < testcase.TestCount(); ++j) {
      if (!testcase.GetTestSuccess(j)) {
        ++failed_count_list[i];
      }
      elapsed_time_list[i] += testcase.GetTestTime(j);
    }
    total_failed_count += failed_count_list[i];
  }

  const tm* time_struct = localtime(&epoch_iteration_start_time);
  char timestamp[40];
  snprintf(timestamp, sizeof(timestamp), "%4d-%02d-%02dT%02d:%02d:%02d",
           time_struct->tm_year + 1900, time_struct->tm_mon + 1, time_struct->tm_mday,
           time_struct->tm_hour, time_struct->tm_min, time_struct->tm_sec);

  fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", fp);
  fprintf(fp, "<testsuites tests=\"%zu\" failures=\"%zu\" disabled=\"0\" errors=\"0\"",
          total_test_count, total_failed_count);
  fprintf(fp, " timestamp=\"%s\" time=\"%.3lf\" name=\"AllTests\">\n", timestamp, elapsed_time_ns / 1e9);
  for (size_t i = 0; i < testcase_list.size(); ++i) {
    auto& testcase = testcase_list[i];
    fprintf(fp, "  <testsuite name=\"%s\" tests=\"%zu\" failures=\"%zu\" disabled=\"0\" errors=\"0\"",
            testcase.GetName().c_str(), testcase.TestCount(), failed_count_list[i]);
    fprintf(fp, " time=\"%.3lf\">\n", elapsed_time_list[i] / 1e9);

    for (size_t j = 0; j < testcase.TestCount(); ++j) {
      fprintf(fp, "    <testcase name=\"%s\" status=\"run\" time=\"%.3lf\" classname=\"%s\"",
              testcase.GetTest(j).GetName().c_str(), testcase.GetTestTime(j) / 1e9,
              testcase.GetName().c_str());
      if (!testcase.GetTestSuccess(j)) {
        fputs(" />\n", fp);
      } else {
        fputs(">\n", fp);
        const std::string& test_output = testcase.GetTest(j).GetTestOutput();
        const std::string escaped_test_output = XmlEscape(test_output);
        fprintf(fp, "      <failure message=\"%s\" type=\"\">\n", escaped_test_output.c_str());
        fputs("      </failure>\n", fp);
        fputs("    </testcase>\n", fp);
      }
    }

    fputs("  </testsuite>\n", fp);
  }
  fputs("</testsuites>\n", fp);
  fclose(fp);
}

static bool sigint_flag;
static bool sigquit_flag;

static void signal_handler(int sig) {
  if (sig == SIGINT) {
    sigint_flag = true;
  } else if (sig == SIGQUIT) {
    sigquit_flag = true;
  }
}

static bool RegisterSignalHandler() {
  sigint_flag = false;
  sigquit_flag = false;
  sig_t ret = signal(SIGINT, signal_handler);
  if (ret != SIG_ERR) {
    ret = signal(SIGQUIT, signal_handler);
  }
  if (ret == SIG_ERR) {
    perror("RegisterSignalHandler");
    return false;
  }
  return true;
}

static bool UnregisterSignalHandler() {
  sig_t ret = signal(SIGINT, SIG_DFL);
  if (ret != SIG_ERR) {
    ret = signal(SIGQUIT, SIG_DFL);
  }
  if (ret == SIG_ERR) {
    perror("UnregisterSignalHandler");
    return false;
  }
  return true;
}

struct ChildProcInfo {
  pid_t pid;
  int64_t start_time_ns;
  int64_t end_time_ns;
  int64_t deadline_end_time_ns; // The time when the test is thought of as timeout.
  size_t testcase_id, test_id;
  bool finished;
  bool timed_out;
  int exit_status;
  int child_read_fd; // File descriptor to read child test failure info.
};

// Forked Child process, run the single test.
static void ChildProcessFn(int argc, char** argv, const std::string& test_name) {
  char** new_argv = new char*[argc + 2];
  memcpy(new_argv, argv, sizeof(char*) * argc);

  char* filter_arg = new char [test_name.size() + 20];
  strcpy(filter_arg, "--gtest_filter=");
  strcat(filter_arg, test_name.c_str());
  new_argv[argc] = filter_arg;
  new_argv[argc + 1] = NULL;

  int new_argc = argc + 1;
  testing::InitGoogleTest(&new_argc, new_argv);
  int result = RUN_ALL_TESTS();
  exit(result);
}

static ChildProcInfo RunChildProcess(const std::string& test_name, int testcase_id, int test_id,
                                     int argc, char** argv) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    perror("pipe in RunTestInSeparateProc");
    exit(1);
  }
  if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl in RunTestInSeparateProc");
    exit(1);
  }
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork in RunTestInSeparateProc");
    exit(1);
  } else if (pid == 0) {
    // In child process, run a single test.
    close(pipefd[0]);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);

    if (!UnregisterSignalHandler()) {
      exit(1);
    }
    ChildProcessFn(argc, argv, test_name);
    // Unreachable.
  }
  // In parent process, initialize child process info.
  close(pipefd[1]);
  ChildProcInfo child_proc;
  child_proc.child_read_fd = pipefd[0];
  child_proc.pid = pid;
  child_proc.start_time_ns = NanoTime();
  child_proc.deadline_end_time_ns = child_proc.start_time_ns + GetTimeoutMs(test_name) * 1000000LL;
  child_proc.testcase_id = testcase_id;
  child_proc.test_id = test_id;
  child_proc.finished = false;
  return child_proc;
}

static void HandleSignals(std::vector<TestCase>& testcase_list,
                            std::vector<ChildProcInfo>& child_proc_list) {
  if (sigquit_flag) {
    sigquit_flag = false;
    // Print current running tests.
    printf("List of current running tests:\n");
    for (const auto& child_proc : child_proc_list) {
      if (child_proc.pid != 0) {
        std::string test_name = testcase_list[child_proc.testcase_id].GetTestName(child_proc.test_id);
        int64_t current_time_ns = NanoTime();
        int64_t run_time_ms = (current_time_ns - child_proc.start_time_ns) / 1000000;
        printf("  %s (%" PRId64 " ms)\n", test_name.c_str(), run_time_ms);
      }
    }
  } else if (sigint_flag) {
    sigint_flag = false;
    // Kill current running tests.
    for (const auto& child_proc : child_proc_list) {
      if (child_proc.pid != 0) {
        // Send SIGKILL to ensure the child process can be killed unconditionally.
        kill(child_proc.pid, SIGKILL);
      }
    }
    // SIGINT kills the parent process as well.
    exit(1);
  }
}

static bool CheckChildProcExit(pid_t exit_pid, int exit_status,
                               std::vector<ChildProcInfo>& child_proc_list) {
  for (size_t i = 0; i < child_proc_list.size(); ++i) {
    if (child_proc_list[i].pid == exit_pid) {
      child_proc_list[i].finished = true;
      child_proc_list[i].timed_out = false;
      child_proc_list[i].exit_status = exit_status;
      child_proc_list[i].end_time_ns = NanoTime();
      return true;
    }
  }
  return false;
}

static size_t CheckChildProcTimeout(std::vector<ChildProcInfo>& child_proc_list) {
  int64_t current_time_ns = NanoTime();
  size_t timeout_child_count = 0;
  for (size_t i = 0; i < child_proc_list.size(); ++i) {
    if (child_proc_list[i].deadline_end_time_ns <= current_time_ns) {
      child_proc_list[i].finished = true;
      child_proc_list[i].timed_out = true;
      child_proc_list[i].end_time_ns = current_time_ns;
      ++timeout_child_count;
    }
  }
  return timeout_child_count;
}

static void ReadChildProcOutput(std::vector<TestCase>& testcase_list,
                                std::vector<ChildProcInfo>& child_proc_list) {
  for (const auto& child_proc : child_proc_list) {
    TestCase& testcase = testcase_list[child_proc.testcase_id];
    int test_id = child_proc.test_id;
    while (true) {
      char buf[1024];
      ssize_t bytes_read = TEMP_FAILURE_RETRY(read(child_proc.child_read_fd, buf, sizeof(buf) - 1));
      if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        testcase.GetTest(test_id).AppendTestOutput(buf);
      } else if (bytes_read == 0) {
        break; // Read end.
      } else {
        if (errno == EAGAIN) {
          break;
        }
        perror("failed to read child_read_fd");
        exit(1);
      }
    }
  }
}

static void WaitChildProcs(std::vector<TestCase>& testcase_list,
                           std::vector<ChildProcInfo>& child_proc_list) {
  size_t finished_child_count = 0;
  while (true) {
    int status;
    pid_t result;
    while ((result = TEMP_FAILURE_RETRY(waitpid(-1, &status, WNOHANG))) > 0) {
      if (CheckChildProcExit(result, status, child_proc_list)) {
        ++finished_child_count;
      }
    }

    if (result == -1) {
      if (errno == ECHILD) {
        // This happens when we have no running child processes.
        return;
      } else {
        perror("waitpid");
        exit(1);
      }
    } else if (result == 0) {
      finished_child_count += CheckChildProcTimeout(child_proc_list);
    }

    ReadChildProcOutput(testcase_list, child_proc_list);
    if (finished_child_count > 0) {
      return;
    }

    HandleSignals(testcase_list, child_proc_list);

    // sleep 1 ms to avoid busy looping.
    timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000000;
    nanosleep(&sleep_time, NULL);
  }
}

static TestResult WaitForOneChild(pid_t pid) {
  int exit_status;
  pid_t result = TEMP_FAILURE_RETRY(waitpid(pid, &exit_status, 0));

  TestResult test_result = TEST_SUCCESS;
  if (result != pid || WEXITSTATUS(exit_status) != 0) {
    test_result = TEST_FAILED;
  }
  return test_result;
}

static void CollectChildTestResult(const ChildProcInfo& child_proc, TestCase& testcase) {
  int test_id = child_proc.test_id;
  testcase.SetTestTime(test_id, child_proc.end_time_ns - child_proc.start_time_ns);
  if (child_proc.timed_out) {
    // The child process marked as timed_out has not exited, and we should kill it manually.
    kill(child_proc.pid, SIGKILL);
    WaitForOneChild(child_proc.pid);
  }
  close(child_proc.child_read_fd);

  if (child_proc.timed_out) {
    testcase.SetTestResult(test_id, TEST_TIMEOUT);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s killed because of timeout at %" PRId64 " ms.\n",
             testcase.GetTestName(test_id).c_str(), testcase.GetTestTime(test_id) / 1000000);
    testcase.GetTest(test_id).AppendTestOutput(buf);

  } else if (WIFSIGNALED(child_proc.exit_status)) {
    // Record signal terminated test as failed.
    testcase.SetTestResult(test_id, TEST_FAILED);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s terminated by signal: %s.\n",
             testcase.GetTestName(test_id).c_str(), strsignal(WTERMSIG(child_proc.exit_status)));
    testcase.GetTest(test_id).AppendTestOutput(buf);

  } else {
    int exitcode = WEXITSTATUS(child_proc.exit_status);
    testcase.SetTestResult(test_id, exitcode == 0 ? TEST_SUCCESS : TEST_FAILED);
    if (exitcode != 0) {
      char buf[1024];
      snprintf(buf, sizeof(buf), "%s exited with exitcode %d.\n",
               testcase.GetTestName(test_id).c_str(), exitcode);
      testcase.GetTest(test_id).AppendTestOutput(buf);
    }
  }
}

// We choose to use multi-fork and multi-wait here instead of multi-thread, because it always
// makes deadlock to use fork in multi-thread.
// Returns true if all tests run successfully, otherwise return false.
static bool RunTestInSeparateProc(int argc, char** argv, std::vector<TestCase>& testcase_list,
                                  int iteration_count, size_t job_count,
                                  const std::string& xml_output_filename) {
  // Stop default result printer to avoid environment setup/teardown information for each test.
  testing::UnitTest::GetInstance()->listeners().Release(
                        testing::UnitTest::GetInstance()->listeners().default_result_printer());
  testing::UnitTest::GetInstance()->listeners().Append(new TestResultPrinter);

  if (!RegisterSignalHandler()) {
    exit(1);
  }

  bool all_tests_passed = true;

  for (size_t iteration = 1;
       iteration_count < 0 || iteration <= static_cast<size_t>(iteration_count);
       ++iteration) {
    OnTestIterationStartPrint(testcase_list, iteration, iteration_count, job_count);
    int64_t iteration_start_time_ns = NanoTime();
    time_t epoch_iteration_start_time = time(NULL);

    // Run up to job_count tests in parallel, each test in a child process.
    std::vector<ChildProcInfo> child_proc_list;

    // Next test to run is [next_testcase_id:next_test_id].
    size_t next_testcase_id = 0;
    size_t next_test_id = 0;

    // Record how many tests are finished.
    std::vector<size_t> finished_test_count_list(testcase_list.size(), 0);
    size_t finished_testcase_count = 0;

    while (finished_testcase_count < testcase_list.size()) {
      // run up to job_count child processes.
      while (child_proc_list.size() < job_count && next_testcase_id < testcase_list.size()) {
        std::string test_name = testcase_list[next_testcase_id].GetTestName(next_test_id);
        ChildProcInfo child_proc = RunChildProcess(test_name, next_testcase_id, next_test_id,
                                                   argc, argv);
        child_proc_list.push_back(child_proc);
        if (++next_test_id == testcase_list[next_testcase_id].TestCount()) {
          next_test_id = 0;
          ++next_testcase_id;
        }
      }

      // Wait for any child proc finish or timeout.
      WaitChildProcs(testcase_list, child_proc_list);

      // Collect result.
      auto it = child_proc_list.begin();
      while (it != child_proc_list.end()) {
        auto& child_proc = *it;
        if (child_proc.finished == true) {
          size_t testcase_id = child_proc.testcase_id;
          size_t test_id = child_proc.test_id;
          TestCase& testcase = testcase_list[testcase_id];

          CollectChildTestResult(child_proc, testcase);
          OnTestEndPrint(testcase, test_id);

          if (++finished_test_count_list[testcase_id] == testcase.TestCount()) {
            ++finished_testcase_count;
          }
          if (!testcase.GetTestSuccess(test_id)) {
            all_tests_passed = false;
          }

          it = child_proc_list.erase(it);
        } else {
          ++it;
        }
      }
    }

    int64_t elapsed_time_ns = NanoTime() - iteration_start_time_ns;
    OnTestIterationEndPrint(testcase_list, iteration, elapsed_time_ns);
    if (!xml_output_filename.empty()) {
      OnTestIterationEndXmlPrint(xml_output_filename, testcase_list, epoch_iteration_start_time,
                                 elapsed_time_ns);
    }
  }

  if (!UnregisterSignalHandler()) {
    exit(1);
  }

  return all_tests_passed;
}

static size_t GetDefaultJobCount() {
  return static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN));
}

static void AddPathSeparatorInTestProgramPath(std::vector<char*>& args) {
  // To run DeathTest in threadsafe mode, gtest requires that the user must invoke the
  // test program via a valid path that contains at least one path separator.
  // The reason is that gtest uses clone() + execve() to run DeathTest in threadsafe mode,
  // and execve() doesn't read environment variable PATH, so execve() will not success
  // until we specify the absolute path or relative path of the test program directly.
  if (strchr(args[0], '/') == nullptr) {
    args[0] = strdup(g_executable_path.c_str());
  }
}

static void AddGtestFilterSynonym(std::vector<char*>& args) {
  // Support --gtest-filter as a synonym for --gtest_filter.
  for (size_t i = 1; i < args.size(); ++i) {
    if (strncmp(args[i], "--gtest-filter", strlen("--gtest-filter")) == 0) {
      args[i][7] = '_';
    }
  }
}

struct IsolationTestOptions {
  bool isolate;
  size_t job_count;
  int test_deadline_ms;
  int test_slow_threshold_ms;
  std::string gtest_color;
  bool gtest_print_time;
  int gtest_repeat;
  std::string gtest_output;
};

// Pick options not for gtest: There are two parts in args, one part is used in isolation test mode
// as described in PrintHelpInfo(), the other part is handled by testing::InitGoogleTest() in
// gtest. PickOptions() picks the first part into IsolationTestOptions structure, leaving the second
// part in args.
// Arguments:
//   args is used to pass in all command arguments, and pass out only the part of options for gtest.
//   options is used to pass out test options in isolation mode.
// Return false if there is error in arguments.
static bool PickOptions(std::vector<char*>& args, IsolationTestOptions& options) {
  for (size_t i = 1; i < args.size(); ++i) {
    if (strcmp(args[i], "--help") == 0 || strcmp(args[i], "-h") == 0) {
      PrintHelpInfo();
      options.isolate = false;
      return true;
    }
  }

  AddPathSeparatorInTestProgramPath(args);
  AddGtestFilterSynonym(args);

  // if --bionic-selftest argument is used, only enable self tests, otherwise remove self tests.
  bool enable_selftest = false;
  for (size_t i = 1; i < args.size(); ++i) {
    if (strcmp(args[i], "--bionic-selftest") == 0) {
      // This argument is to enable "bionic_selftest*" for self test, and is not shown in help info.
      // Don't remove this option from arguments.
      enable_selftest = true;
    }
  }
  std::string gtest_filter_str;
  for (size_t i = args.size() - 1; i >= 1; --i) {
    if (strncmp(args[i], "--gtest_filter=", strlen("--gtest_filter=")) == 0) {
      gtest_filter_str = args[i] + strlen("--gtest_filter=");
      args.erase(args.begin() + i);
      break;
    }
  }
  if (enable_selftest == true) {
    gtest_filter_str = "bionic_selftest*";
  } else {
    if (gtest_filter_str.empty()) {
      gtest_filter_str = "-bionic_selftest*";
    } else {
      // Find if '-' for NEGATIVE_PATTERNS exists.
      if (gtest_filter_str.find('-') != std::string::npos) {
        gtest_filter_str += ":bionic_selftest*";
      } else {
        gtest_filter_str += ":-bionic_selftest*";
      }
    }
  }
  gtest_filter_str = "--gtest_filter=" + gtest_filter_str;
  args.push_back(strdup(gtest_filter_str.c_str()));

  options.isolate = true;
  // Parse arguments that make us can't run in isolation mode.
  for (size_t i = 1; i < args.size(); ++i) {
    if (strcmp(args[i], "--no-isolate") == 0) {
      options.isolate = false;
    } else if (strcmp(args[i], "--gtest_list_tests") == 0) {
      options.isolate = false;
    }
  }

  // Stop parsing if we will not run in isolation mode.
  if (options.isolate == false) {
    return true;
  }

  // Init default isolation test options.
  options.job_count = GetDefaultJobCount();
  options.test_deadline_ms = DEFAULT_GLOBAL_TEST_RUN_DEADLINE_MS;
  options.test_slow_threshold_ms = DEFAULT_GLOBAL_TEST_RUN_SLOW_THRESHOLD_MS;
  options.gtest_color = testing::GTEST_FLAG(color);
  options.gtest_print_time = testing::GTEST_FLAG(print_time);
  options.gtest_repeat = testing::GTEST_FLAG(repeat);
  options.gtest_output = testing::GTEST_FLAG(output);

  // Parse arguments speficied for isolation mode.
  for (size_t i = 1; i < args.size(); ++i) {
    if (strncmp(args[i], "-j", strlen("-j")) == 0) {
      char* p = args[i] + strlen("-j");
      int count = 0;
      if (*p != '\0') {
        // Argument like -j5.
        count = atoi(p);
      } else if (args.size() > i + 1) {
        // Arguments like -j 5.
        count = atoi(args[i + 1]);
        ++i;
      }
      if (count <= 0) {
        fprintf(stderr, "invalid job count: %d\n", count);
        return false;
      }
      options.job_count = static_cast<size_t>(count);
    } else if (strncmp(args[i], "--deadline=", strlen("--deadline=")) == 0) {
      int time_ms = atoi(args[i] + strlen("--deadline="));
      if (time_ms <= 0) {
        fprintf(stderr, "invalid deadline: %d\n", time_ms);
        return false;
      }
      options.test_deadline_ms = time_ms;
    } else if (strncmp(args[i], "--slow-threshold=", strlen("--slow-threshold=")) == 0) {
      int time_ms = atoi(args[i] + strlen("--slow-threshold="));
      if (time_ms <= 0) {
        fprintf(stderr, "invalid slow test threshold: %d\n", time_ms);
        return false;
      }
      options.test_slow_threshold_ms = time_ms;
    } else if (strncmp(args[i], "--gtest_color=", strlen("--gtest_color=")) == 0) {
      options.gtest_color = args[i] + strlen("--gtest_color=");
    } else if (strcmp(args[i], "--gtest_print_time=0") == 0) {
      options.gtest_print_time = false;
    } else if (strncmp(args[i], "--gtest_repeat=", strlen("--gtest_repeat=")) == 0) {
      // If the value of gtest_repeat is < 0, then it indicates the tests
      // should be repeated forever.
      options.gtest_repeat = atoi(args[i] + strlen("--gtest_repeat="));
      // Remove --gtest_repeat=xx from arguments, so child process only run one iteration for a single test.
      args.erase(args.begin() + i);
      --i;
    } else if (strncmp(args[i], "--gtest_output=", strlen("--gtest_output=")) == 0) {
      std::string output = args[i] + strlen("--gtest_output=");
      // generate output xml file path according to the strategy in gtest.
      bool success = true;
      if (strncmp(output.c_str(), "xml:", strlen("xml:")) == 0) {
        output = output.substr(strlen("xml:"));
        if (output.size() == 0) {
          success = false;
        }
        // Make absolute path.
        if (success && output[0] != '/') {
          char* cwd = getcwd(NULL, 0);
          if (cwd != NULL) {
            output = std::string(cwd) + "/" + output;
            free(cwd);
          } else {
            success = false;
          }
        }
        // Add file name if output is a directory.
        if (success && output.back() == '/') {
          output += "test_details.xml";
        }
      }
      if (success) {
        options.gtest_output = output;
      } else {
        fprintf(stderr, "invalid gtest_output file: %s\n", args[i]);
        return false;
      }

      // Remove --gtest_output=xxx from arguments, so child process will not write xml file.
      args.erase(args.begin() + i);
      --i;
    }
  }

  // Add --no-isolate in args to prevent child process from running in isolation mode again.
  // As DeathTest will try to call execve(), this argument should always be added.
  args.insert(args.begin() + 1, strdup("--no-isolate"));
  return true;
}

static std::string get_proc_self_exe() {
  char path[PATH_MAX];
  ssize_t path_len = readlink("/proc/self/exe", path, sizeof(path));
  if (path_len <= 0 || path_len >= static_cast<ssize_t>(sizeof(path))) {
    perror("readlink");
    exit(1);
  }

  return std::string(path, path_len);
}

int main(int argc, char** argv, char** envp) {
  g_executable_path = get_proc_self_exe();
  g_argc = argc;
  g_argv = argv;
  g_envp = envp;
  std::vector<char*> arg_list(argv, argv + argc);

  IsolationTestOptions options;
  if (PickOptions(arg_list, options) == false) {
    return 1;
  }

  if (options.isolate == true) {
    // Set global variables.
    global_test_run_deadline_ms = options.test_deadline_ms;
    global_test_run_slow_threshold_ms = options.test_slow_threshold_ms;
    testing::GTEST_FLAG(color) = options.gtest_color.c_str();
    testing::GTEST_FLAG(print_time) = options.gtest_print_time;
    std::vector<TestCase> testcase_list;

    argc = static_cast<int>(arg_list.size());
    arg_list.push_back(NULL);
    if (EnumerateTests(argc, arg_list.data(), testcase_list) == false) {
      return 1;
    }
    bool all_test_passed =  RunTestInSeparateProc(argc, arg_list.data(), testcase_list,
                              options.gtest_repeat, options.job_count, options.gtest_output);
    return all_test_passed ? 0 : 1;
  } else {
    argc = static_cast<int>(arg_list.size());
    arg_list.push_back(NULL);
    testing::InitGoogleTest(&argc, arg_list.data());
    return RUN_ALL_TESTS();
  }
}

//################################################################################
// Bionic Gtest self test, run this by --bionic-selftest option.

TEST(bionic_selftest, test_success) {
  ASSERT_EQ(1, 1);
}

TEST(bionic_selftest, test_fail) {
  ASSERT_EQ(0, 1);
}

TEST(bionic_selftest, test_time_warn) {
  sleep(4);
}

TEST(bionic_selftest, test_timeout) {
  while (1) {}
}

TEST(bionic_selftest, test_signal_SEGV_terminated) {
  char* p = reinterpret_cast<char*>(static_cast<intptr_t>(atoi("0")));
  *p = 3;
}

class bionic_selftest_DeathTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  }
};

static void deathtest_helper_success() {
  ASSERT_EQ(1, 1);
  exit(0);
}

TEST_F(bionic_selftest_DeathTest, success) {
  ASSERT_EXIT(deathtest_helper_success(), ::testing::ExitedWithCode(0), "");
}

static void deathtest_helper_fail() {
  ASSERT_EQ(1, 0);
}

TEST_F(bionic_selftest_DeathTest, fail) {
  ASSERT_EXIT(deathtest_helper_fail(), ::testing::ExitedWithCode(0), "");
}

class BionicSelfTest : public ::testing::TestWithParam<bool> {
};

TEST_P(BionicSelfTest, test_success) {
  ASSERT_EQ(GetParam(), GetParam());
}

INSTANTIATE_TEST_CASE_P(bionic_selftest, BionicSelfTest, ::testing::Values(true, false));

template <typename T>
class bionic_selftest_TestT : public ::testing::Test {
};

typedef ::testing::Types<char, int> MyTypes;

TYPED_TEST_CASE(bionic_selftest_TestT, MyTypes);

TYPED_TEST(bionic_selftest_TestT, test_success) {
  ASSERT_EQ(true, true);
}
