/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <jni.h>
#include <gtest/gtest.h>
#include <android/log.h>

#include <errno.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <functional>
#include <vector>

#define LOG_TAG "Cts-DebugTest"

// Used by child processes only
#define assert_or_exit(x)                                                                         \
    do {                                                                                          \
        if(x) break;                                                                              \
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Assertion " #x " failed. errno(%d): %s", \
                errno, strerror(errno));                                                          \
        _exit(1);                                                                                 \
    } while (0)

static void parent(pid_t child) {
    int status;
    int wpid = waitpid(child, &status, 0);
    ASSERT_EQ(child, wpid);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(0, WEXITSTATUS(status));
}

static void run_test(const std::function<void(pid_t)> &test) {
    pid_t pid = fork();
    ASSERT_NE(-1, pid) << "fork() failed with " << strerror(errno);
    if (pid != 0) {
        parent(pid);
    } else {
        // child
        test(getppid());
        _exit(0);
    }
}

static void ptraceAttach(pid_t parent) {
    assert_or_exit(ptrace(PTRACE_ATTACH, parent, nullptr, nullptr) == 0);
    int status;
    assert_or_exit(waitpid(parent, &status, __WALL) == parent);
    assert_or_exit(WIFSTOPPED(status));
    assert_or_exit(WSTOPSIG(status) == SIGSTOP);

    assert_or_exit(ptrace(PTRACE_DETACH, parent, nullptr, nullptr) == 0);
}

TEST(DebugTest, ptraceAttach) {
    run_test(ptraceAttach);
}

static void processVmReadv(pid_t parent, const std::vector<long *> &addresses) {
    long destination;
    iovec local = { &destination, sizeof destination };

    for (long *address : addresses) {
        // Since we are forked, the address will be valid in the remote process as well.
        iovec remote = { address, sizeof *address };
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s About to read %p\n", __func__,
                            address);
        assert_or_exit(process_vm_readv(parent, &local, 1, &remote, 1, 0) == sizeof destination);

        // Compare the data with the contents of our memory.
        assert_or_exit(destination == *address);
    }
}

static long global_variable = 0x47474747;
// public static native boolean processVmReadv();
TEST(DebugTest, processVmReadv) {
    long stack_variable = 0x42424242;
    // This runs the test with a selection of different kinds of addresses and
    // makes sure the child process (simulating a debugger) can read them.
    run_test([&](pid_t parent) {
        processVmReadv(parent, std::vector<long *>{
                                   &global_variable, &stack_variable,
                                   reinterpret_cast<long *>(&processVmReadv)});
    });
}

// public static native boolean processVmReadvNullptr();
TEST(DebugTest, processVmReadvNullptr) {
    // Make sure reading unallocated memory behaves reasonably.
    run_test([](pid_t parent) {
        long destination;
        iovec local = {&destination, sizeof destination};
        iovec remote = {nullptr, sizeof(long)};

        assert_or_exit(process_vm_readv(parent, &local, 1, &remote, 1, 0) == -1);
        assert_or_exit(errno == EFAULT);
    });
}
