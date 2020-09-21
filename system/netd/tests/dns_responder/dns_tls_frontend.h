/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef DNS_TLS_FRONTEND_H
#define DNS_TLS_FRONTEND_H

#include <arpa/nameser.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <android-base/thread_annotations.h>
#include <openssl/ssl.h>

namespace test {

constexpr int SHA256_SIZE = 32;

/*
 * Simple DNS over TLS reverse proxy that forwards to a UDP backend.
 * Only handles a single request at a time.
 */
class DnsTlsFrontend {
public:
    DnsTlsFrontend(const std::string& listen_address, const std::string& listen_service,
            const std::string& backend_address, const std::string& backend_service) :
            listen_address_(listen_address), listen_service_(listen_service),
            backend_address_(backend_address), backend_service_(backend_service),
            queries_(0), terminate_(false) { }
    ~DnsTlsFrontend() {
        stopServer();
    }
    const std::string& listen_address() const {
        return listen_address_;
    }
    const std::string& listen_service() const {
        return listen_service_;
    }
    bool running() const {
        return socket_ != -1;
    }
    bool startServer();
    bool stopServer();
    int queries() const { return queries_; }
    bool waitForQueries(int number, int timeoutMs) const;
    void set_chain_length(int length) { chain_length_ = length; }
    // Represents a fingerprint from the middle of the certificate chain.
    const std::vector<uint8_t>& fingerprint() const { return fingerprint_; }

private:
    void requestHandler();
    bool handleOneRequest(SSL* ssl);

    std::string listen_address_;
    std::string listen_service_;
    std::string backend_address_;
    std::string backend_service_;
    bssl::UniquePtr<SSL_CTX> ctx_;
    int socket_ = -1;
    int backend_socket_ = -1;
    std::atomic<int> queries_;
    std::atomic<bool> terminate_;
    std::thread handler_thread_ GUARDED_BY(update_mutex_);
    std::mutex update_mutex_;
    int chain_length_ = 1;
    std::vector<uint8_t> fingerprint_;
};

}  // namespace test

#endif  // DNS_TLS_FRONTEND_H
