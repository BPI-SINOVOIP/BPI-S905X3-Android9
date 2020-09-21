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
#ifndef ANDROID_HARDWARE_BROADCASTRADIO_VTS_MOCK_TIMEOUT
#define ANDROID_HARDWARE_BROADCASTRADIO_VTS_MOCK_TIMEOUT

#include <gmock/gmock.h>
#include <thread>

#ifndef EGMOCK_VERBOSE
#define EGMOCK_VERBOSE 0
#endif

/**
 * Print log message.
 *
 * INTERNAL IMPLEMENTATION - don't use in user code.
 */
#if EGMOCK_VERBOSE
#define EGMOCK_LOG_(...) ALOGV("egmock: " __VA_ARGS__)
#else
#define EGMOCK_LOG_(...)
#endif

/**
 * Common helper objects for gmock timeout extension.
 *
 * INTERNAL IMPLEMENTATION - don't use in user code.
 */
#define EGMOCK_TIMEOUT_METHOD_DEF_(Method, ...) \
    std::atomic<bool> egmock_called_##Method;   \
    std::mutex egmock_mut_##Method;             \
    std::condition_variable egmock_cond_##Method;

/**
 * Function similar to comma operator, to make it possible to return any value returned by mocked
 * function (which may be void) and discard the result of the other operation (notification about
 * a call).
 *
 * We need to invoke the mocked function (which result is returned) before the notification (which
 * result is dropped) - that's exactly the opposite of comma operator.
 *
 * INTERNAL IMPLEMENTATION - don't use in user code.
 */
template <typename T>
static T EGMockFlippedComma_(std::function<T()> returned, std::function<void()> discarded) {
    auto ret = returned();
    discarded();
    return ret;
}

template <>
inline void EGMockFlippedComma_(std::function<void()> returned, std::function<void()> discarded) {
    returned();
    discarded();
}

/**
 * Common method body for gmock timeout extension.
 *
 * INTERNAL IMPLEMENTATION - don't use in user code.
 */
#define EGMOCK_TIMEOUT_METHOD_BODY_(Method, ...)                      \
    auto invokeMock = [&]() { return egmock_##Method(__VA_ARGS__); }; \
    auto notify = [&]() {                                             \
        std::lock_guard<std::mutex> lk(egmock_mut_##Method);          \
        EGMOCK_LOG_(#Method " called");                               \
        egmock_called_##Method = true;                                \
        egmock_cond_##Method.notify_all();                            \
    };                                                                \
    return EGMockFlippedComma_<decltype(invokeMock())>(invokeMock, notify);

/**
 * Gmock MOCK_METHOD0 timeout-capable extension.
 */
#define MOCK_TIMEOUT_METHOD0(Method, ...)       \
    MOCK_METHOD0(egmock_##Method, __VA_ARGS__); \
    EGMOCK_TIMEOUT_METHOD_DEF_(Method);         \
    virtual GMOCK_RESULT_(, __VA_ARGS__) Method() { EGMOCK_TIMEOUT_METHOD_BODY_(Method); }

/**
 * Gmock MOCK_METHOD1 timeout-capable extension.
 */
#define MOCK_TIMEOUT_METHOD1(Method, ...)                                                 \
    MOCK_METHOD1(egmock_##Method, __VA_ARGS__);                                           \
    EGMOCK_TIMEOUT_METHOD_DEF_(Method);                                                   \
    virtual GMOCK_RESULT_(, __VA_ARGS__) Method(GMOCK_ARG_(, 1, __VA_ARGS__) egmock_a1) { \
        EGMOCK_TIMEOUT_METHOD_BODY_(Method, egmock_a1);                                   \
    }

/**
 * Gmock MOCK_METHOD2 timeout-capable extension.
 */
#define MOCK_TIMEOUT_METHOD2(Method, ...)                                                        \
    MOCK_METHOD2(egmock_##Method, __VA_ARGS__);                                                  \
    EGMOCK_TIMEOUT_METHOD_DEF_(Method);                                                          \
    virtual GMOCK_RESULT_(, __VA_ARGS__)                                                         \
        Method(GMOCK_ARG_(, 1, __VA_ARGS__) egmock_a1, GMOCK_ARG_(, 2, __VA_ARGS__) egmock_a2) { \
        EGMOCK_TIMEOUT_METHOD_BODY_(Method, egmock_a1, egmock_a2);                               \
    }

/**
 * Gmock EXPECT_CALL timeout-capable extension.
 *
 * It has slightly different syntax from the original macro, to make method name accessible.
 * So, instead of typing
 *     EXPECT_CALL(account, charge(100, Currency::USD));
 * you need to inline arguments
 *     EXPECT_TIMEOUT_CALL(account, charge, 100, Currency::USD);
 */
#define EXPECT_TIMEOUT_CALL(obj, Method, ...) \
    EGMOCK_LOG_(#Method " expected to call"); \
    (obj).egmock_called_##Method = false;     \
    EXPECT_CALL(obj, egmock_##Method(__VA_ARGS__))

/**
 * Waits for an earlier EXPECT_TIMEOUT_CALL to execute.
 *
 * It does not fully support special constraints of the EXPECT_CALL clause, just proceeds when the
 * first call to a given method comes. For example, in the following code:
 *     EXPECT_TIMEOUT_CALL(account, charge, 100, _);
 *     account.charge(50, Currency::USD);
 *     EXPECT_TIMEOUT_CALL_WAIT(account, charge, 500ms);
 * the wait clause will just continue, as the charge method was called.
 *
 * @param obj object for a call
 * @param Method the method to wait for
 * @param timeout the maximum time for waiting
 */
#define EXPECT_TIMEOUT_CALL_WAIT(obj, Method, timeout)                      \
    {                                                                       \
        EGMOCK_LOG_("waiting for " #Method " call");                        \
        std::unique_lock<std::mutex> lk((obj).egmock_mut_##Method);         \
        if (!(obj).egmock_called_##Method) {                                \
            auto status = (obj).egmock_cond_##Method.wait_for(lk, timeout); \
            EXPECT_EQ(std::cv_status::no_timeout, status);                  \
        }                                                                   \
    }

#endif  // ANDROID_HARDWARE_BROADCASTRADIO_VTS_MOCK_TIMEOUT
