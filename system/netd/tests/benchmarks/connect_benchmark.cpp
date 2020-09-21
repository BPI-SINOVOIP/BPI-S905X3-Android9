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

#define LOG_TAG "connect_benchmark"

/*
 * See README.md for general notes.
 *
 * This set of benchmarks measures the throughput of connect() calls on a single thread for IPv4 and
 * IPv6 under the following scenarios:
 *
 *  - FWmark disabled (::ANDROID_NO_USE_FWMARK_CLIENT).
 *
 *      The control case for other high load benchmarks. Essentially just testing performance of
 *      the kernel connect call. In real world use fwmark should stay on in order for traffic to
 *      be routed properly.
 *
 *  - FWmark enabled only for metrics (::ANDROID_FWMARK_METRICS_ONLY).
 *
 *      The default mode up to and including 7.1. Every time connect() is called on an AF_INET or
 *      AF_INET6 socket, netdclient sends a synchronous message to fwmarkserver to get the socket
 *      marked. Only the fields that are useful for marking or for metrics are sent in this mode;
 *      other fields are set to null for the RPC and ignored.
 *
 *  - FWmark enabled for all events.
 *
 *      The default mode starting from 7.1.2. As well as the normal connect() reporting, extra
 *      fields are filled in to log the IP and port of the connection.
 *
 *      A second synchronous message is sent to fwmarkserver after the connection completes, to
 *      record latency. This message is forwarded to the system server over a oneway binder call.
 *
 * Realtime timed tests
 * ====================
 *
 * The tests named *_high_load record the following useful information:
 *
 *   - real_time: the mean roundtrip time for one connect() call under load
 *
 *   - iterations: the number of times the test was run within the timelimit --- approximately
 *                 MinTime / real_time
 *
 * Manually timed tests
 * ====================
 *
 * All other sets of tests apart from *_high_load run with manual timing. The purpose of these is to
 * measure 90th-percentile latency for connect() calls compared to mean latency.
 *
 * (TODO: ideally this should be against median latency, but google-benchmark only supports one
 *        custom 'label' output for graphing. Stddev isn't appropriate because the latency
 *        distribution is usually spiky, not in a nice neat normal-like distribution.)
 *
 * The manually timed tests record the following useful information:
 *
 *  - real_time: the average time taken to complete a test run. Unlike the real_time used in high
 *               load tests, this is calculated from before-and-after values of the realtime clock
 *               over many iterations so may be less accurate than the under-load times.
 *
 *  - iterations: the number of times the test was run within the timelimit --- approximately
 *                MinTime / real_time, although as explained, may not be as meaningful because of
 *                overhead from timing.
 *
 *  - label: a manually-recorded time giving the 90th-percentile value of real_time over all
 *           individual runs. Should be compared to real_time.
 *
 */

#include <arpa/inet.h>
#include <cutils/sockets.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>

#include <map>
#include <functional>
#include <thread>

#include <android-base/stringprintf.h>
#include <benchmark/benchmark.h>
#include <log/log.h>
#include <utils/StrongPointer.h>

#include "FwmarkClient.h"
#include "SockDiag.h"
#include "Stopwatch.h"
#include "android/net/metrics/INetdEventListener.h"

using android::base::StringPrintf;
using android::net::metrics::INetdEventListener;

static int bindAndListen(int s) {
    sockaddr_in6 sin6 = { .sin6_family = AF_INET6 };
    if (bind(s, (sockaddr*) &sin6, sizeof(sin6)) == 0) {
        if (listen(s, 1)) {
            return -1;
        }
        sockaddr_in sin = {};
        socklen_t len = sizeof(sin);
        if (getsockname(s, (sockaddr*) &sin, &len)) {
            return -1;
        }
        return ntohs(sin.sin_port);
    } else {
        return -1;
    }
}

static void ipv4_loopback(benchmark::State& state, const bool waitBetweenRuns) {
    const int listensocket = socket(AF_INET6, SOCK_STREAM, 0);
    const int port = bindAndListen(listensocket);
    if (port == -1) {
        state.SkipWithError("Unable to bind server socket");
        return;
    }

    // ALOGW("Listening on port = %d", port);
    std::vector<uint64_t> latencies(state.max_iterations);
    uint64_t iterations = 0;

    while (state.KeepRunning()) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            state.SkipWithError(StringPrintf("socket() failed with errno=%d", errno).c_str());
            break;
        }

        const Stopwatch stopwatch;

        sockaddr_in server = { .sin_family = AF_INET, .sin_port = htons(port) };
        if (connect(sock, (sockaddr*) &server, sizeof(server))) {
            state.SkipWithError(StringPrintf("connect() failed with errno=%d", errno).c_str());
            close(sock);
            break;
        }

        if (waitBetweenRuns) {
            latencies[iterations] = stopwatch.timeTaken() * 1e6L;
            state.SetIterationTime(latencies[iterations] / 1e9L);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++iterations;
        }

        sockaddr_in6 client;
        socklen_t clientlen = sizeof(client);
        int accepted = accept(listensocket, (sockaddr *) &client, &clientlen);
        if (accepted < 0) {
            state.SkipWithError(StringPrintf("accept() failed with errno=%d", errno).c_str());
            close(sock);
            break;
        }

        close(accepted);
        close(sock);
    }
    close(listensocket);
    // ALOGI("Finished test on port = %d", port);

    if (iterations > 0) {
        latencies.resize(iterations);
        sort(latencies.begin(), latencies.end());
        state.SetLabel(StringPrintf("%lld", (long long) latencies[iterations * 9 / 10]));
    }
}

static void ipv6_loopback(benchmark::State& state, const bool waitBetweenRuns) {
    const int listensocket = socket(AF_INET6, SOCK_STREAM, 0);
    const int port = bindAndListen(listensocket);
    if (port == -1) {
        state.SkipWithError("Unable to bind server socket");
        return;
    }

    // ALOGW("Listening on port = %d", port);
    std::vector<uint64_t> latencies(state.max_iterations);
    uint64_t iterations = 0;

    while (state.KeepRunning()) {
        int sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock < 0) {
            state.SkipWithError(StringPrintf("socket() failed with errno=%d", errno).c_str());
            break;
        }

        const Stopwatch stopwatch;

        sockaddr_in6 server = { .sin6_family = AF_INET6, .sin6_port = htons(port) };
        if (connect(sock, (sockaddr*) &server, sizeof(server))) {
            state.SkipWithError(StringPrintf("connect() failed with errno=%d", errno).c_str());
            close(sock);
            break;
        }

        if (waitBetweenRuns) {
            latencies[iterations] = stopwatch.timeTaken() * 1e6L;
            state.SetIterationTime(latencies[iterations] / 1e9L);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++iterations;
        }

        sockaddr_in6 client;
        socklen_t clientlen = sizeof(client);
        int accepted = accept(listensocket, (sockaddr *) &client, &clientlen);
        if (accepted < 0) {
            state.SkipWithError(StringPrintf("accept() failed with errno=%d", errno).c_str());
            close(sock);
            break;
        }

        close(accepted);
        close(sock);
    }
    close(listensocket);
    // ALOGI("Finished test on port = %d", port);

    if (iterations > 0) {
        latencies.resize(iterations);
        sort(latencies.begin(), latencies.end());
        state.SetLabel(StringPrintf("%lld", (long long) latencies[iterations * 9 / 10]));
    }
}

static void run_at_reporting_level(decltype(ipv4_loopback) benchmarkFunction,
                                   ::benchmark::State& state, const int reportingLevel,
                                   const bool waitBetweenRuns) {
    // Our master thread (thread_index == 0) will control setup and teardown for other threads.
    const bool isMaster = (state.thread_index == 0);

    // Previous values of env variables used by fwmarkclient (only read/written by master thread)
    const std::string savedSettings[] = {
        FwmarkClient::ANDROID_NO_USE_FWMARK_CLIENT,
        FwmarkClient::ANDROID_FWMARK_METRICS_ONLY
    };
    std::map<std::string, std::string> prevSettings;

    // SETUP
    if (isMaster) {
        for (const auto setting : savedSettings) {
            const char* prevEnvStr = getenv(setting.c_str());
            if (prevEnvStr != nullptr) {
                prevSettings[setting.c_str()] = prevEnvStr;
            }
        }
        switch (reportingLevel) {
            case INetdEventListener::REPORTING_LEVEL_NONE:
                setenv(FwmarkClient::ANDROID_NO_USE_FWMARK_CLIENT, "", 1);
                break;
            case INetdEventListener::REPORTING_LEVEL_METRICS:
                unsetenv(FwmarkClient::ANDROID_NO_USE_FWMARK_CLIENT);
                setenv(FwmarkClient::ANDROID_FWMARK_METRICS_ONLY, "", 1);
                break;
            case INetdEventListener::REPORTING_LEVEL_FULL:
                unsetenv(FwmarkClient::ANDROID_NO_USE_FWMARK_CLIENT);
                unsetenv(FwmarkClient::ANDROID_FWMARK_METRICS_ONLY);
                break;
        }
    }

    // TEST
    benchmarkFunction(state, waitBetweenRuns);

    // TEARDOWN
    if (isMaster) {
        for (const auto setting : savedSettings) {
            if (prevSettings.count(setting)) {
                setenv(setting.c_str(), prevSettings[setting].c_str(), 1);
            } else {
                unsetenv(setting.c_str());
            }
        }
    }
}

constexpr int MIN_THREADS = 1;
constexpr int MAX_THREADS = 1;
constexpr double MIN_TIME = 0.5 /* seconds */;

static void ipv4_metrics_reporting_no_fwmark(::benchmark::State& state) {
    run_at_reporting_level(ipv4_loopback, state, INetdEventListener::REPORTING_LEVEL_NONE, true);
}
BENCHMARK(ipv4_metrics_reporting_no_fwmark)->MinTime(MIN_TIME)->UseManualTime();

// IPv4 metrics under low load
static void ipv4_metrics_reporting_no_load(::benchmark::State& state) {
    run_at_reporting_level(ipv4_loopback, state, INetdEventListener::REPORTING_LEVEL_METRICS, true);
}
BENCHMARK(ipv4_metrics_reporting_no_load)->MinTime(MIN_TIME)->UseManualTime();

static void ipv4_full_reporting_no_load(::benchmark::State& state) {
    run_at_reporting_level(ipv4_loopback, state, INetdEventListener::REPORTING_LEVEL_FULL, true);
}
BENCHMARK(ipv4_full_reporting_no_load)->MinTime(MIN_TIME)->UseManualTime();

// IPv4 benchmarks under high load
static void ipv4_metrics_reporting_high_load(::benchmark::State& state) {
    run_at_reporting_level(ipv4_loopback, state, INetdEventListener::REPORTING_LEVEL_METRICS,
            false);
}
BENCHMARK(ipv4_metrics_reporting_high_load)
    ->ThreadRange(MIN_THREADS, MAX_THREADS)->MinTime(MIN_TIME)->UseRealTime();

static void ipv4_full_reporting_high_load(::benchmark::State& state) {
    run_at_reporting_level(ipv4_loopback, state, INetdEventListener::REPORTING_LEVEL_FULL, false);
}
BENCHMARK(ipv4_full_reporting_high_load)
    ->ThreadRange(MIN_THREADS, MAX_THREADS)->MinTime(MIN_TIME)->UseRealTime();

// IPv6 raw connect() without using fwmark
static void ipv6_metrics_reporting_no_fwmark(::benchmark::State& state) {
    run_at_reporting_level(ipv6_loopback, state, INetdEventListener::REPORTING_LEVEL_NONE, true);
}
BENCHMARK(ipv6_metrics_reporting_no_fwmark)->MinTime(MIN_TIME)->UseManualTime();

// IPv6 metrics under low load
static void ipv6_metrics_reporting_no_load(::benchmark::State& state) {
    run_at_reporting_level(ipv6_loopback, state, INetdEventListener::REPORTING_LEVEL_METRICS, true);
}
BENCHMARK(ipv6_metrics_reporting_no_load)->MinTime(MIN_TIME)->UseManualTime();

static void ipv6_full_reporting_no_load(::benchmark::State& state) {
    run_at_reporting_level(ipv6_loopback, state, INetdEventListener::REPORTING_LEVEL_FULL, true);
}
BENCHMARK(ipv6_full_reporting_no_load)->MinTime(MIN_TIME)->UseManualTime();

// IPv6 benchmarks under high load
static void ipv6_metrics_reporting_high_load(::benchmark::State& state) {
    run_at_reporting_level(ipv6_loopback, state, INetdEventListener::REPORTING_LEVEL_METRICS,
            false);
}
BENCHMARK(ipv6_metrics_reporting_high_load)
    ->ThreadRange(MIN_THREADS, MAX_THREADS)->MinTime(MIN_TIME)->UseRealTime();

static void ipv6_full_reporting_high_load(::benchmark::State& state) {
    run_at_reporting_level(ipv6_loopback, state, INetdEventListener::REPORTING_LEVEL_FULL, false);
}
BENCHMARK(ipv6_full_reporting_high_load)
    ->ThreadRange(MIN_THREADS, MAX_THREADS)->MinTime(MIN_TIME)->UseRealTime();
