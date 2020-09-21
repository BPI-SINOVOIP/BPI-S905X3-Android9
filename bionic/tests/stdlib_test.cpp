/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include "BionicDeathTest.h"
#include "math_data_test.h"
#include "TemporaryFile.h"
#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <limits>
#include <string>

#if defined(__BIONIC__)
  #define ALIGNED_ALLOC_AVAILABLE 1
#elif defined(__GLIBC_PREREQ)
  #if __GLIBC_PREREQ(2, 16)
    #define ALIGNED_ALLOC_AVAILABLE 1
  #endif
#endif

// The random number generator tests all set the seed, get four values, reset the seed and check
// that they get the first two values repeated, and then reset the seed and check two more values
// to rule out the possibility that we're just going round a cycle of four values.
// TODO: factor this out.

TEST(stdlib, drand48) {
  srand48(0x01020304);
  EXPECT_DOUBLE_EQ(0.65619299195623526, drand48());
  EXPECT_DOUBLE_EQ(0.18522597229772941, drand48());
  EXPECT_DOUBLE_EQ(0.42015087072844537, drand48());
  EXPECT_DOUBLE_EQ(0.061637783047395089, drand48());
  srand48(0x01020304);
  EXPECT_DOUBLE_EQ(0.65619299195623526, drand48());
  EXPECT_DOUBLE_EQ(0.18522597229772941, drand48());
  srand48(0x01020304);
  EXPECT_DOUBLE_EQ(0.65619299195623526, drand48());
  EXPECT_DOUBLE_EQ(0.18522597229772941, drand48());
}

TEST(stdlib, erand48) {
  const unsigned short seed[3] = { 0x330e, 0xabcd, 0x1234 };
  unsigned short xsubi[3];
  memcpy(xsubi, seed, sizeof(seed));
  EXPECT_DOUBLE_EQ(0.39646477376027534, erand48(xsubi));
  EXPECT_DOUBLE_EQ(0.84048536941142515, erand48(xsubi));
  EXPECT_DOUBLE_EQ(0.35333609724524351, erand48(xsubi));
  EXPECT_DOUBLE_EQ(0.44658343479654405, erand48(xsubi));
  memcpy(xsubi, seed, sizeof(seed));
  EXPECT_DOUBLE_EQ(0.39646477376027534, erand48(xsubi));
  EXPECT_DOUBLE_EQ(0.84048536941142515, erand48(xsubi));
  memcpy(xsubi, seed, sizeof(seed));
  EXPECT_DOUBLE_EQ(0.39646477376027534, erand48(xsubi));
  EXPECT_DOUBLE_EQ(0.84048536941142515, erand48(xsubi));
}

TEST(stdlib, lcong48) {
  unsigned short p[7] = { 0x0102, 0x0304, 0x0506, 0x0708, 0x090a, 0x0b0c, 0x0d0e };
  lcong48(p);
  EXPECT_EQ(1531389981, lrand48());
  EXPECT_EQ(1598801533, lrand48());
  EXPECT_EQ(2080534853, lrand48());
  EXPECT_EQ(1102488897, lrand48());
  lcong48(p);
  EXPECT_EQ(1531389981, lrand48());
  EXPECT_EQ(1598801533, lrand48());
  lcong48(p);
  EXPECT_EQ(1531389981, lrand48());
  EXPECT_EQ(1598801533, lrand48());
}

TEST(stdlib, lrand48) {
  srand48(0x01020304);
  EXPECT_EQ(1409163720, lrand48());
  EXPECT_EQ(397769746, lrand48());
  EXPECT_EQ(902267124, lrand48());
  EXPECT_EQ(132366131, lrand48());
  srand48(0x01020304);
  EXPECT_EQ(1409163720, lrand48());
  EXPECT_EQ(397769746, lrand48());
  srand48(0x01020304);
  EXPECT_EQ(1409163720, lrand48());
  EXPECT_EQ(397769746, lrand48());
}

TEST(stdlib, random) {
  srandom(0x01020304);
  EXPECT_EQ(55436735, random());
  EXPECT_EQ(1399865117, random());
  EXPECT_EQ(2032643283, random());
  EXPECT_EQ(571329216, random());
  srandom(0x01020304);
  EXPECT_EQ(55436735, random());
  EXPECT_EQ(1399865117, random());
  srandom(0x01020304);
  EXPECT_EQ(55436735, random());
  EXPECT_EQ(1399865117, random());
}

TEST(stdlib, rand) {
  srand(0x01020304);
  EXPECT_EQ(55436735, rand());
  EXPECT_EQ(1399865117, rand());
  EXPECT_EQ(2032643283, rand());
  EXPECT_EQ(571329216, rand());
  srand(0x01020304);
  EXPECT_EQ(55436735, rand());
  EXPECT_EQ(1399865117, rand());
  srand(0x01020304);
  EXPECT_EQ(55436735, rand());
  EXPECT_EQ(1399865117, rand());
}

TEST(stdlib, mrand48) {
  srand48(0x01020304);
  EXPECT_EQ(-1476639856, mrand48());
  EXPECT_EQ(795539493, mrand48());
  EXPECT_EQ(1804534249, mrand48());
  EXPECT_EQ(264732262, mrand48());
  srand48(0x01020304);
  EXPECT_EQ(-1476639856, mrand48());
  EXPECT_EQ(795539493, mrand48());
  srand48(0x01020304);
  EXPECT_EQ(-1476639856, mrand48());
  EXPECT_EQ(795539493, mrand48());
}

TEST(stdlib, jrand48_distribution) {
  const int iterations = 4096;
  const int pivot_low  = 1536;
  const int pivot_high = 2560;

  unsigned short xsubi[3];
  int bits[32] = {};

  for (int iter = 0; iter < iterations; ++iter) {
    long rand_val = jrand48(xsubi);
    for (int bit = 0; bit < 32; ++bit) {
      bits[bit] += (static_cast<unsigned long>(rand_val) >> bit) & 0x01;
    }
  }

  // Check that bit probability is uniform
  for (int bit = 0; bit < 32; ++bit) {
    EXPECT_TRUE((pivot_low <= bits[bit]) && (bits[bit] <= pivot_high));
  }
}

TEST(stdlib, mrand48_distribution) {
  const int iterations = 4096;
  const int pivot_low  = 1536;
  const int pivot_high = 2560;

  int bits[32] = {};

  for (int iter = 0; iter < iterations; ++iter) {
    long rand_val = mrand48();
    for (int bit = 0; bit < 32; ++bit) {
      bits[bit] += (static_cast<unsigned long>(rand_val) >> bit) & 0x01;
    }
  }

  // Check that bit probability is uniform
  for (int bit = 0; bit < 32; ++bit) {
    EXPECT_TRUE((pivot_low <= bits[bit]) && (bits[bit] <= pivot_high));
  }
}

TEST(stdlib, posix_memalign_sweep) {
  void* ptr;

  // These should all fail.
  for (size_t align = 0; align < sizeof(long); align++) {
    ASSERT_EQ(EINVAL, posix_memalign(&ptr, align, 256))
        << "Unexpected value at align " << align;
  }

  // Verify powers of 2 up to 2048 allocate, and verify that all other
  // alignment values between the powers of 2 fail.
  size_t last_align = sizeof(long);
  for (size_t align = sizeof(long); align <= 2048; align <<= 1) {
    // Try all of the non power of 2 values from the last until this value.
    for (size_t fail_align = last_align + 1; fail_align < align; fail_align++) {
      ASSERT_EQ(EINVAL, posix_memalign(&ptr, fail_align, 256))
          << "Unexpected success at align " << fail_align;
    }
    ASSERT_EQ(0, posix_memalign(&ptr, align, 256))
        << "Unexpected failure at align " << align;
    ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr) & (align - 1))
        << "Did not return a valid aligned ptr " << ptr << " expected alignment " << align;
    free(ptr);
    last_align = align;
  }
}

TEST(stdlib, posix_memalign_various_sizes) {
  std::vector<size_t> sizes{1, 4, 8, 256, 1024, 65000, 128000, 256000, 1000000};
  for (auto size : sizes) {
    void* ptr;
    ASSERT_EQ(0, posix_memalign(&ptr, 16, 1))
        << "posix_memalign failed at size " << size;
    ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr) & 0xf)
        << "Pointer not aligned at size " << size << " ptr " << ptr;
    free(ptr);
  }
}

TEST(stdlib, posix_memalign_overflow) {
  void* ptr;
  ASSERT_NE(0, posix_memalign(&ptr, 16, SIZE_MAX));
}

TEST(stdlib, aligned_alloc_sweep) {
#if defined(ALIGNED_ALLOC_AVAILABLE)
  // Verify powers of 2 up to 2048 allocate, and verify that all other
  // alignment values between the powers of 2 fail.
  size_t last_align = 1;
  for (size_t align = 1; align <= 2048; align <<= 1) {
    // Try all of the non power of 2 values from the last until this value.
    for (size_t fail_align = last_align + 1; fail_align < align; fail_align++) {
      ASSERT_TRUE(aligned_alloc(fail_align, 256) == nullptr)
          << "Unexpected success at align " << fail_align;
      ASSERT_EQ(EINVAL, errno) << "Unexpected errno at align " << fail_align;
    }
    void* ptr = aligned_alloc(align, 256);
    ASSERT_TRUE(ptr != nullptr) << "Unexpected failure at align " << align;
    ASSERT_EQ(0U, reinterpret_cast<uintptr_t>(ptr) & (align - 1))
        << "Did not return a valid aligned ptr " << ptr << " expected alignment " << align;
    free(ptr);
    last_align = align;
  }
#else
  GTEST_LOG_(INFO) << "This test requires a C library that has aligned_alloc.\n";
#endif
}

TEST(stdlib, aligned_alloc_overflow) {
#if defined(ALIGNED_ALLOC_AVAILABLE)
  ASSERT_TRUE(aligned_alloc(16, SIZE_MAX) == nullptr);
#else
  GTEST_LOG_(INFO) << "This test requires a C library that has aligned_alloc.\n";
#endif
}

TEST(stdlib, aligned_alloc_size_not_multiple_of_alignment) {
#if defined(ALIGNED_ALLOC_AVAILABLE)
  for (size_t size = 1; size <= 2048; size++) {
    void* ptr = aligned_alloc(2048, size);
    ASSERT_TRUE(ptr != nullptr) << "Failed at size " << std::to_string(size);
    free(ptr);
  }
#else
  GTEST_LOG_(INFO) << "This test requires a C library that has aligned_alloc.\n";
#endif
}

TEST(stdlib, realpath__NULL_filename) {
  errno = 0;
  // Work around the compile-time error generated by FORTIFY here.
  const char* path = NULL;
  char* p = realpath(path, NULL);
  ASSERT_TRUE(p == NULL);
  ASSERT_EQ(EINVAL, errno);
}

TEST(stdlib, realpath__empty_filename) {
  errno = 0;
  char* p = realpath("", NULL);
  ASSERT_TRUE(p == NULL);
  ASSERT_EQ(ENOENT, errno);
}

TEST(stdlib, realpath__ENOENT) {
  errno = 0;
  char* p = realpath("/this/directory/path/almost/certainly/does/not/exist", NULL);
  ASSERT_TRUE(p == NULL);
  ASSERT_EQ(ENOENT, errno);
}

TEST(stdlib, realpath__component_after_non_directory) {
  errno = 0;
  char* p = realpath("/dev/null/.", NULL);
  ASSERT_TRUE(p == NULL);
  ASSERT_EQ(ENOTDIR, errno);

  errno = 0;
  p = realpath("/dev/null/..", NULL);
  ASSERT_TRUE(p == NULL);
  ASSERT_EQ(ENOTDIR, errno);
}

TEST(stdlib, realpath) {
  // Get the name of this executable.
  char executable_path[PATH_MAX];
  int rc = readlink("/proc/self/exe", executable_path, sizeof(executable_path));
  ASSERT_NE(rc, -1);
  executable_path[rc] = '\0';

  char buf[PATH_MAX + 1];
  char* p = realpath("/proc/self/exe", buf);
  ASSERT_STREQ(executable_path, p);

  p = realpath("/proc/self/exe", NULL);
  ASSERT_STREQ(executable_path, p);
  free(p);
}

TEST(stdlib, qsort) {
  struct s {
    char name[16];
    static int comparator(const void* lhs, const void* rhs) {
      return strcmp(reinterpret_cast<const s*>(lhs)->name, reinterpret_cast<const s*>(rhs)->name);
    }
  };
  s entries[3];
  strcpy(entries[0].name, "charlie");
  strcpy(entries[1].name, "bravo");
  strcpy(entries[2].name, "alpha");

  qsort(entries, 3, sizeof(s), s::comparator);
  ASSERT_STREQ("alpha", entries[0].name);
  ASSERT_STREQ("bravo", entries[1].name);
  ASSERT_STREQ("charlie", entries[2].name);

  qsort(entries, 3, sizeof(s), s::comparator);
  ASSERT_STREQ("alpha", entries[0].name);
  ASSERT_STREQ("bravo", entries[1].name);
  ASSERT_STREQ("charlie", entries[2].name);
}

static void* TestBug57421_child(void* arg) {
  pthread_t main_thread = reinterpret_cast<pthread_t>(arg);
  pthread_join(main_thread, NULL);
  char* value = getenv("ENVIRONMENT_VARIABLE");
  if (value == NULL) {
    setenv("ENVIRONMENT_VARIABLE", "value", 1);
  }
  return NULL;
}

static void TestBug57421_main() {
  pthread_t t;
  ASSERT_EQ(0, pthread_create(&t, NULL, TestBug57421_child, reinterpret_cast<void*>(pthread_self())));
  pthread_exit(NULL);
}

// Even though this isn't really a death test, we have to say "DeathTest" here so gtest knows to
// run this test (which exits normally) in its own process.

class stdlib_DeathTest : public BionicDeathTest {};

TEST_F(stdlib_DeathTest, getenv_after_main_thread_exits) {
  // https://code.google.com/p/android/issues/detail?id=57421
  ASSERT_EXIT(TestBug57421_main(), ::testing::ExitedWithCode(0), "");
}

TEST(stdlib, mkostemp64) {
  TemporaryFile tf([](char* path) { return mkostemp64(path, O_CLOEXEC); });
  AssertCloseOnExec(tf.fd, true);
}

TEST(stdlib, mkostemp) {
  TemporaryFile tf([](char* path) { return mkostemp(path, O_CLOEXEC); });
  AssertCloseOnExec(tf.fd, true);
}

TEST(stdlib, mkstemp64) {
  TemporaryFile tf(mkstemp64);
  struct stat64 sb;
  ASSERT_EQ(0, fstat64(tf.fd, &sb));
  ASSERT_EQ(O_LARGEFILE, fcntl(tf.fd, F_GETFL) & O_LARGEFILE);
}

TEST(stdlib, mkstemp) {
  TemporaryFile tf;
  struct stat sb;
  ASSERT_EQ(0, fstat(tf.fd, &sb));
}

TEST(stdlib, system) {
  int status;

  status = system("exit 0");
  ASSERT_TRUE(WIFEXITED(status));
  ASSERT_EQ(0, WEXITSTATUS(status));

  status = system("exit 1");
  ASSERT_TRUE(WIFEXITED(status));
  ASSERT_EQ(1, WEXITSTATUS(status));
}

TEST(stdlib, atof) {
  ASSERT_DOUBLE_EQ(1.23, atof("1.23"));
}

template <typename T>
static void CheckStrToFloat(T fn(const char* s, char** end)) {
  FpUlpEq<0, T> pred;

  EXPECT_PRED_FORMAT2(pred, 9.0, fn("9.0", nullptr));
  EXPECT_PRED_FORMAT2(pred, 9.0, fn("0.9e1", nullptr));
  EXPECT_PRED_FORMAT2(pred, 9.0, fn("0x1.2p3", nullptr));

  const char* s = " \t\v\f\r\n9.0";
  char* p;
  EXPECT_PRED_FORMAT2(pred, 9.0, fn(s, &p));
  EXPECT_EQ(s + strlen(s), p);

  EXPECT_TRUE(isnan(fn("+nan", nullptr)));
  EXPECT_TRUE(isnan(fn("nan", nullptr)));
  EXPECT_TRUE(isnan(fn("-nan", nullptr)));

  EXPECT_TRUE(isnan(fn("+nan(0xff)", nullptr)));
  EXPECT_TRUE(isnan(fn("nan(0xff)", nullptr)));
  EXPECT_TRUE(isnan(fn("-nan(0xff)", nullptr)));

  EXPECT_TRUE(isnan(fn("+nanny", &p)));
  EXPECT_STREQ("ny", p);
  EXPECT_TRUE(isnan(fn("nanny", &p)));
  EXPECT_STREQ("ny", p);
  EXPECT_TRUE(isnan(fn("-nanny", &p)));
  EXPECT_STREQ("ny", p);

  EXPECT_EQ(0, fn("muppet", &p));
  EXPECT_STREQ("muppet", p);
  EXPECT_EQ(0, fn("  muppet", &p));
  EXPECT_STREQ("  muppet", p);

  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("+inf", nullptr));
  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("inf", nullptr));
  EXPECT_EQ(-std::numeric_limits<T>::infinity(), fn("-inf", nullptr));

  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("+infinity", nullptr));
  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("infinity", nullptr));
  EXPECT_EQ(-std::numeric_limits<T>::infinity(), fn("-infinity", nullptr));

  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("+infinitude", &p));
  EXPECT_STREQ("initude", p);
  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("infinitude", &p));
  EXPECT_STREQ("initude", p);
  EXPECT_EQ(-std::numeric_limits<T>::infinity(), fn("-infinitude", &p));
  EXPECT_STREQ("initude", p);

  // Check case-insensitivity.
  EXPECT_EQ(std::numeric_limits<T>::infinity(), fn("InFiNiTy", nullptr));
  EXPECT_TRUE(isnan(fn("NaN", nullptr)));
}

TEST(stdlib, strtod) {
  CheckStrToFloat(strtod);
}

TEST(stdlib, strtof) {
  CheckStrToFloat(strtof);
}

TEST(stdlib, strtold) {
  CheckStrToFloat(strtold);
}

TEST(stdlib, strtof_2206701) {
  ASSERT_EQ(0.0f, strtof("7.0064923216240853546186479164495e-46", NULL));
  ASSERT_EQ(1.4e-45f, strtof("7.0064923216240853546186479164496e-46", NULL));
}

TEST(stdlib, strtod_largest_subnormal) {
  // This value has been known to cause javac and java to infinite loop.
  // http://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/
  ASSERT_EQ(2.2250738585072014e-308, strtod("2.2250738585072012e-308", NULL));
  ASSERT_EQ(2.2250738585072014e-308, strtod("0.00022250738585072012e-304", NULL));
  ASSERT_EQ(2.2250738585072014e-308, strtod("00000002.2250738585072012e-308", NULL));
  ASSERT_EQ(2.2250738585072014e-308, strtod("2.225073858507201200000e-308", NULL));
  ASSERT_EQ(2.2250738585072014e-308, strtod("2.2250738585072012e-00308", NULL));
  ASSERT_EQ(2.2250738585072014e-308, strtod("2.22507385850720129978001e-308", NULL));
  ASSERT_EQ(-2.2250738585072014e-308, strtod("-2.2250738585072012e-308", NULL));
}

TEST(stdlib, quick_exit) {
  pid_t pid = fork();
  ASSERT_NE(-1, pid) << strerror(errno);

  if (pid == 0) {
    quick_exit(99);
  }

  AssertChildExited(pid, 99);
}

static int quick_exit_status = 0;

static void quick_exit_1(void) {
  ASSERT_EQ(quick_exit_status, 0);
  quick_exit_status = 1;
}

static void quick_exit_2(void) {
  ASSERT_EQ(quick_exit_status, 1);
}

static void not_run(void) {
  FAIL();
}

TEST(stdlib, at_quick_exit) {
  pid_t pid = fork();
  ASSERT_NE(-1, pid) << strerror(errno);

  if (pid == 0) {
    ASSERT_EQ(at_quick_exit(quick_exit_2), 0);
    ASSERT_EQ(at_quick_exit(quick_exit_1), 0);
    atexit(not_run);
    quick_exit(99);
  }

  AssertChildExited(pid, 99);
}

TEST(unistd, _Exit) {
  pid_t pid = fork();
  ASSERT_NE(-1, pid) << strerror(errno);

  if (pid == 0) {
    _Exit(99);
  }

  AssertChildExited(pid, 99);
}

TEST(stdlib, pty_smoke) {
  // getpt returns a pty with O_RDWR|O_NOCTTY.
  int fd = getpt();
  ASSERT_NE(-1, fd);

  // grantpt is a no-op.
  ASSERT_EQ(0, grantpt(fd));

  // ptsname_r should start "/dev/pts/".
  char name_r[128];
  ASSERT_EQ(0, ptsname_r(fd, name_r, sizeof(name_r)));
  name_r[9] = 0;
  ASSERT_STREQ("/dev/pts/", name_r);

  close(fd);
}

TEST(stdlib, posix_openpt) {
  int fd = posix_openpt(O_RDWR|O_NOCTTY|O_CLOEXEC);
  ASSERT_NE(-1, fd);
  close(fd);
}

TEST(stdlib, ptsname_r_ENOTTY) {
  errno = 0;
  char buf[128];
  ASSERT_EQ(ENOTTY, ptsname_r(STDOUT_FILENO, buf, sizeof(buf)));
  ASSERT_EQ(ENOTTY, errno);
}

TEST(stdlib, ptsname_r_EINVAL) {
  int fd = getpt();
  ASSERT_NE(-1, fd);
  errno = 0;
  char* buf = NULL;
  ASSERT_EQ(EINVAL, ptsname_r(fd, buf, 128));
  ASSERT_EQ(EINVAL, errno);
  close(fd);
}

TEST(stdlib, ptsname_r_ERANGE) {
  int fd = getpt();
  ASSERT_NE(-1, fd);
  errno = 0;
  char buf[1];
  ASSERT_EQ(ERANGE, ptsname_r(fd, buf, sizeof(buf)));
  ASSERT_EQ(ERANGE, errno);
  close(fd);
}

TEST(stdlib, ttyname) {
  int fd = getpt();
  ASSERT_NE(-1, fd);

  // ttyname returns "/dev/ptmx" for a pty.
  ASSERT_STREQ("/dev/ptmx", ttyname(fd));

  close(fd);
}

TEST(stdlib, ttyname_r) {
  int fd = getpt();
  ASSERT_NE(-1, fd);

  // ttyname_r returns "/dev/ptmx" for a pty.
  char name_r[128];
  ASSERT_EQ(0, ttyname_r(fd, name_r, sizeof(name_r)));
  ASSERT_STREQ("/dev/ptmx", name_r);

  close(fd);
}

TEST(stdlib, ttyname_r_ENOTTY) {
  int fd = open("/dev/null", O_WRONLY);
  errno = 0;
  char buf[128];
  ASSERT_EQ(ENOTTY, ttyname_r(fd, buf, sizeof(buf)));
  ASSERT_EQ(ENOTTY, errno);
  close(fd);
}

TEST(stdlib, ttyname_r_EINVAL) {
  int fd = getpt();
  ASSERT_NE(-1, fd);
  errno = 0;
  char* buf = NULL;
  ASSERT_EQ(EINVAL, ttyname_r(fd, buf, 128));
  ASSERT_EQ(EINVAL, errno);
  close(fd);
}

TEST(stdlib, ttyname_r_ERANGE) {
  int fd = getpt();
  ASSERT_NE(-1, fd);
  errno = 0;
  char buf[1];
  ASSERT_EQ(ERANGE, ttyname_r(fd, buf, sizeof(buf)));
  ASSERT_EQ(ERANGE, errno);
  close(fd);
}

TEST(stdlib, unlockpt_ENOTTY) {
  int fd = open("/dev/null", O_WRONLY);
  errno = 0;
  ASSERT_EQ(-1, unlockpt(fd));
  ASSERT_EQ(ENOTTY, errno);
  close(fd);
}

TEST(stdlib, getsubopt) {
  char* const tokens[] = {
    const_cast<char*>("a"),
    const_cast<char*>("b"),
    const_cast<char*>("foo"),
    nullptr
  };
  std::string input = "a,b,foo=bar,a,unknown";
  char* subopts = &input[0];
  char* value = nullptr;

  ASSERT_EQ(0, getsubopt(&subopts, tokens, &value));
  ASSERT_EQ(nullptr, value);
  ASSERT_EQ(1, getsubopt(&subopts, tokens, &value));
  ASSERT_EQ(nullptr, value);
  ASSERT_EQ(2, getsubopt(&subopts, tokens, &value));
  ASSERT_STREQ("bar", value);
  ASSERT_EQ(0, getsubopt(&subopts, tokens, &value));
  ASSERT_EQ(nullptr, value);

  ASSERT_EQ(-1, getsubopt(&subopts, tokens, &value));
}

TEST(stdlib, mblen) {
  // "If s is a null pointer, mblen() shall return a non-zero or 0 value, if character encodings,
  // respectively, do or do not have state-dependent encodings." We're always UTF-8.
  EXPECT_EQ(0, mblen(nullptr, 1));

  ASSERT_STREQ("C.UTF-8", setlocale(LC_ALL, "C.UTF-8"));

  // 1-byte UTF-8.
  EXPECT_EQ(1, mblen("abcdef", 6));
  // 2-byte UTF-8.
  EXPECT_EQ(2, mblen("\xc2\xa2" "cdef", 6));
  // 3-byte UTF-8.
  EXPECT_EQ(3, mblen("\xe2\x82\xac" "def", 6));
  // 4-byte UTF-8.
  EXPECT_EQ(4, mblen("\xf0\xa4\xad\xa2" "ef", 6));

  // Illegal over-long sequence.
  ASSERT_EQ(-1, mblen("\xf0\x82\x82\xac" "ef", 6));

  // "mblen() shall ... return 0 (if s points to the null byte)".
  EXPECT_EQ(0, mblen("", 1));
}

template <typename T>
static void CheckStrToInt(T fn(const char* s, char** end, int base)) {
  char* end_p;

  // Negative base => invalid.
  errno = 0;
  ASSERT_EQ(T(0), fn("123", &end_p, -1));
  ASSERT_EQ(EINVAL, errno);

  // Base 1 => invalid (base 0 means "please guess").
  errno = 0;
  ASSERT_EQ(T(0), fn("123", &end_p, 1));
  ASSERT_EQ(EINVAL, errno);

  // Base > 36 => invalid.
  errno = 0;
  ASSERT_EQ(T(0), fn("123", &end_p, 37));
  ASSERT_EQ(EINVAL, errno);

  // If we see "0x" *not* followed by a hex digit, we shouldn't swallow the 'x'.
  ASSERT_EQ(T(0), fn("0xy", &end_p, 16));
  ASSERT_EQ('x', *end_p);

  if (std::numeric_limits<T>::is_signed) {
    // Minimum (such as -128).
    std::string min{std::to_string(std::numeric_limits<T>::min())};
    end_p = nullptr;
    errno = 0;
    ASSERT_EQ(std::numeric_limits<T>::min(), fn(min.c_str(), &end_p, 0));
    ASSERT_EQ(0, errno);
    ASSERT_EQ('\0', *end_p);
    // Too negative (such as -129).
    min.back() = (min.back() + 1);
    end_p = nullptr;
    errno = 0;
    ASSERT_EQ(std::numeric_limits<T>::min(), fn(min.c_str(), &end_p, 0));
    ASSERT_EQ(ERANGE, errno);
    ASSERT_EQ('\0', *end_p);
  }

  // Maximum (such as 127).
  std::string max{std::to_string(std::numeric_limits<T>::max())};
  end_p = nullptr;
  errno = 0;
  ASSERT_EQ(std::numeric_limits<T>::max(), fn(max.c_str(), &end_p, 0));
  ASSERT_EQ(0, errno);
  ASSERT_EQ('\0', *end_p);
  // Too positive (such as 128).
  max.back() = (max.back() + 1);
  end_p = nullptr;
  errno = 0;
  ASSERT_EQ(std::numeric_limits<T>::max(), fn(max.c_str(), &end_p, 0));
  ASSERT_EQ(ERANGE, errno);
  ASSERT_EQ('\0', *end_p);

  // In case of overflow, strto* leaves us pointing past the end of the number,
  // not at the digit that overflowed.
  end_p = nullptr;
  errno = 0;
  ASSERT_EQ(std::numeric_limits<T>::max(),
            fn("99999999999999999999999999999999999999999999999999999abc", &end_p, 0));
  ASSERT_EQ(ERANGE, errno);
  ASSERT_STREQ("abc", end_p);
  if (std::numeric_limits<T>::is_signed) {
      end_p = nullptr;
      errno = 0;
      ASSERT_EQ(std::numeric_limits<T>::min(),
                fn("-99999999999999999999999999999999999999999999999999999abc", &end_p, 0));
      ASSERT_EQ(ERANGE, errno);
      ASSERT_STREQ("abc", end_p);
  }
}

TEST(stdlib, strtol_smoke) {
  CheckStrToInt(strtol);
}

TEST(stdlib, strtoll_smoke) {
  CheckStrToInt(strtoll);
}

TEST(stdlib, strtoul_smoke) {
  CheckStrToInt(strtoul);
}

TEST(stdlib, strtoull_smoke) {
  CheckStrToInt(strtoull);
}

TEST(stdlib, strtoimax_smoke) {
  CheckStrToInt(strtoimax);
}

TEST(stdlib, strtoumax_smoke) {
  CheckStrToInt(strtoumax);
}

TEST(stdlib, abs) {
  ASSERT_EQ(INT_MAX, abs(-INT_MAX));
  ASSERT_EQ(INT_MAX, abs(INT_MAX));
}

TEST(stdlib, labs) {
  ASSERT_EQ(LONG_MAX, labs(-LONG_MAX));
  ASSERT_EQ(LONG_MAX, labs(LONG_MAX));
}

TEST(stdlib, llabs) {
  ASSERT_EQ(LLONG_MAX, llabs(-LLONG_MAX));
  ASSERT_EQ(LLONG_MAX, llabs(LLONG_MAX));
}
