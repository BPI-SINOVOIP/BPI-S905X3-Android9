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

#include "dns_tls_frontend.h"

#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <unistd.h>

#define LOG_TAG "DnsTlsFrontend"
#include <log/log.h>
#include <netdutils/SocketOption.h>

using android::netdutils::enableSockopt;

namespace {

// Copied from DnsTlsTransport.
bool getSPKIDigest(const X509* cert, std::vector<uint8_t>* out) {
    int spki_len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert), NULL);
    unsigned char spki[spki_len];
    unsigned char* temp = spki;
    if (spki_len != i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert), &temp)) {
        ALOGE("SPKI length mismatch");
        return false;
    }
    out->resize(test::SHA256_SIZE);
    unsigned int digest_len = 0;
    int ret = EVP_Digest(spki, spki_len, out->data(), &digest_len, EVP_sha256(), NULL);
    if (ret != 1) {
        ALOGE("Server cert digest extraction failed");
        return false;
    }
    if (digest_len != out->size()) {
        ALOGE("Wrong digest length: %d", digest_len);
        return false;
    }
    return true;
}

std::string errno2str() {
    char error_msg[512] = { 0 };
    if (strerror_r(errno, error_msg, sizeof(error_msg)))
        return std::string();
    return std::string(error_msg);
}

#define APLOGI(fmt, ...) ALOGI(fmt ": [%d] %s", __VA_ARGS__, errno, errno2str().c_str())

std::string addr2str(const sockaddr* sa, socklen_t sa_len) {
    char host_str[NI_MAXHOST] = { 0 };
    int rv = getnameinfo(sa, sa_len, host_str, sizeof(host_str), nullptr, 0,
                         NI_NUMERICHOST);
    if (rv == 0) return std::string(host_str);
    return std::string();
}

bssl::UniquePtr<EVP_PKEY> make_private_key() {
    bssl::UniquePtr<BIGNUM> e(BN_new());
    if (!e) {
        ALOGE("BN_new failed");
        return nullptr;
    }
    if (!BN_set_word(e.get(), RSA_F4)) {
        ALOGE("BN_set_word failed");
        return nullptr;
    }

    bssl::UniquePtr<RSA> rsa(RSA_new());
    if (!rsa) {
        ALOGE("RSA_new failed");
        return nullptr;
    }
    if (!RSA_generate_key_ex(rsa.get(), 2048, e.get(), NULL)) {
        ALOGE("RSA_generate_key_ex failed");
        return nullptr;
    }

    bssl::UniquePtr<EVP_PKEY> privkey(EVP_PKEY_new());
    if (!privkey) {
        ALOGE("EVP_PKEY_new failed");
        return nullptr;
    }
    if(!EVP_PKEY_assign_RSA(privkey.get(), rsa.get())) {
        ALOGE("EVP_PKEY_assign_RSA failed");
        return nullptr;
    }

    // |rsa| is now owned by |privkey|, so no need to free it.
    rsa.release();
    return privkey;
}

bssl::UniquePtr<X509> make_cert(EVP_PKEY* privkey, EVP_PKEY* parent_key) {
    bssl::UniquePtr<X509> cert(X509_new());
    if (!cert) {
        ALOGE("X509_new failed");
        return nullptr;
    }

    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1);

    // Set one hour expiration.
    X509_gmtime_adj(X509_get_notBefore(cert.get()), 0);
    X509_gmtime_adj(X509_get_notAfter(cert.get()), 60 * 60);

    X509_set_pubkey(cert.get(), privkey);

    if (!X509_sign(cert.get(), parent_key, EVP_sha256())) {
        ALOGE("X509_sign failed");
        return nullptr;
    }

    return cert;
}

}

namespace test {

bool DnsTlsFrontend::startServer() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ctx_.reset(SSL_CTX_new(TLS_server_method()));
    if (!ctx_) {
        ALOGE("SSL context creation failed");
        return false;
    }

    SSL_CTX_set_ecdh_auto(ctx_.get(), 1);

    // Make certificate chain
    std::vector<bssl::UniquePtr<EVP_PKEY>> keys(chain_length_);
    for (int i = 0; i < chain_length_; ++i) {
        keys[i] = make_private_key();
    }
    std::vector<bssl::UniquePtr<X509>> certs(chain_length_);
    for (int i = 0; i < chain_length_; ++i) {
        int next = std::min(i + 1, chain_length_ - 1);
        certs[i] = make_cert(keys[i].get(), keys[next].get());
    }

    // Install certificate chain.
    if (SSL_CTX_use_certificate(ctx_.get(), certs[0].get()) <= 0) {
        ALOGE("SSL_CTX_use_certificate failed");
        return false;
    }
    if (SSL_CTX_use_PrivateKey(ctx_.get(), keys[0].get()) <= 0 ) {
        ALOGE("SSL_CTX_use_PrivateKey failed");
        return false;
    }
    for (int i = 1; i < chain_length_; ++i) {
        if (SSL_CTX_add1_chain_cert(ctx_.get(), certs[i].get()) != 1) {
            ALOGE("SSL_CTX_add1_chain_cert failed");
            return false;
        }
    }

    // Report the fingerprint of the "middle" cert.  For N = 2, this is the root.
    int fp_index = chain_length_ / 2;
    if (!getSPKIDigest(certs[fp_index].get(), &fingerprint_)) {
        ALOGE("getSPKIDigest failed");
        return false;
    }

    // Set up TCP server socket for clients.
    addrinfo frontend_ai_hints{
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };
    addrinfo* frontend_ai_res;
    int rv = getaddrinfo(listen_address_.c_str(), listen_service_.c_str(),
                         &frontend_ai_hints, &frontend_ai_res);
    if (rv) {
        ALOGE("frontend getaddrinfo(%s, %s) failed: %s", listen_address_.c_str(),
            listen_service_.c_str(), gai_strerror(rv));
        return false;
    }

    int s = -1;
    for (const addrinfo* ai = frontend_ai_res ; ai ; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s < 0) continue;
        enableSockopt(s, SOL_SOCKET, SO_REUSEPORT);
        enableSockopt(s, SOL_SOCKET, SO_REUSEADDR);
        if (bind(s, ai->ai_addr, ai->ai_addrlen)) {
            APLOGI("bind failed for socket %d", s);
            close(s);
            s = -1;
            continue;
        }
        std::string host_str = addr2str(ai->ai_addr, ai->ai_addrlen);
        ALOGI("bound to TCP %s:%s", host_str.c_str(), listen_service_.c_str());
        break;
    }
    freeaddrinfo(frontend_ai_res);
    if (s < 0) {
        ALOGE("server socket creation failed");
        return false;
    }

    if (listen(s, 1) < 0) {
        ALOGE("listen failed");
        return false;
    }

    socket_ = s;

    // Set up UDP client socket to backend.
    addrinfo backend_ai_hints{
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM
    };
    addrinfo* backend_ai_res;
    rv = getaddrinfo(backend_address_.c_str(), backend_service_.c_str(),
                         &backend_ai_hints, &backend_ai_res);
    if (rv) {
        ALOGE("backend getaddrinfo(%s, %s) failed: %s", listen_address_.c_str(),
            listen_service_.c_str(), gai_strerror(rv));
        return false;
    }
    backend_socket_ = socket(backend_ai_res->ai_family, backend_ai_res->ai_socktype,
        backend_ai_res->ai_protocol);
    if (backend_socket_ < 0) {
        ALOGE("backend socket creation failed");
        return false;
    }
    connect(backend_socket_, backend_ai_res->ai_addr, backend_ai_res->ai_addrlen);
    freeaddrinfo(backend_ai_res);

    {
        std::lock_guard<std::mutex> lock(update_mutex_);
        handler_thread_ = std::thread(&DnsTlsFrontend::requestHandler, this);
    }
    ALOGI("server started successfully");
    return true;
}

void DnsTlsFrontend::requestHandler() {
    ALOGD("Request handler started");
    struct pollfd fds[1] = {{ .fd = socket_, .events = POLLIN }};

    while (!terminate_) {
        int poll_code = poll(fds, 1, 10 /* ms */);
        if (poll_code == 0) {
            // Timeout.  Poll again.
            continue;
        } else if (poll_code < 0) {
            ALOGW("Poll failed with error %d", poll_code);
            // Error.
            break;
        }
        sockaddr_storage addr;
        socklen_t len = sizeof(addr);

        ALOGD("Trying to accept a client");
        int client = accept(socket_, reinterpret_cast<sockaddr*>(&addr), &len);
        ALOGD("Got client socket %d", client);
        if (client < 0) {
            // Stop
            break;
        }

        bssl::UniquePtr<SSL> ssl(SSL_new(ctx_.get()));
        SSL_set_fd(ssl.get(), client);

        ALOGD("Doing SSL handshake");
        bool success = false;
        if (SSL_accept(ssl.get()) <= 0) {
            ALOGI("SSL negotiation failure");
        } else {
            ALOGD("SSL handshake complete");
            success = handleOneRequest(ssl.get());
        }

        close(client);

        if (success) {
            // Increment queries_ as late as possible, because it represents
            // a query that is fully processed, and the response returned to the
            // client, including cleanup actions.
            ++queries_;
        }
    }
    ALOGD("Request handler terminating");
}

bool DnsTlsFrontend::handleOneRequest(SSL* ssl) {
    uint8_t queryHeader[2];
    if (SSL_read(ssl, &queryHeader, 2) != 2) {
        ALOGI("Not enough header bytes");
        return false;
    }
    const uint16_t qlen = (queryHeader[0] << 8) | queryHeader[1];
    uint8_t query[qlen];
    size_t qbytes = 0;
    while (qbytes < qlen) {
        int ret = SSL_read(ssl, query + qbytes, qlen - qbytes);
        if (ret <= 0) {
            ALOGI("Error while reading query");
            return false;
        }
        qbytes += ret;
    }
    int sent = send(backend_socket_, query, qlen, 0);
    if (sent != qlen) {
        ALOGI("Failed to send query");
        return false;
    }
    const int max_size = 4096;
    uint8_t recv_buffer[max_size];
    int rlen = recv(backend_socket_, recv_buffer, max_size, 0);
    if (rlen <= 0) {
        ALOGI("Failed to receive response");
        return false;
    }
    uint8_t responseHeader[2];
    responseHeader[0] = rlen >> 8;
    responseHeader[1] = rlen;
    if (SSL_write(ssl, responseHeader, 2) != 2) {
        ALOGI("Failed to write response header");
        return false;
    }
    if (SSL_write(ssl, recv_buffer, rlen) != rlen) {
        ALOGI("Failed to write response body");
        return false;
    }
    return true;
}

bool DnsTlsFrontend::stopServer() {
    std::lock_guard<std::mutex> lock(update_mutex_);
    if (!running()) {
        ALOGI("server not running");
        return false;
    }
    if (terminate_) {
        ALOGI("LOGIC ERROR");
        return false;
    }
    ALOGI("stopping frontend");
    terminate_ = true;
    handler_thread_.join();
    close(socket_);
    close(backend_socket_);
    terminate_ = false;
    socket_ = -1;
    backend_socket_ = -1;
    ctx_.reset();
    fingerprint_.clear();
    ALOGI("frontend stopped successfully");
    return true;
}

bool DnsTlsFrontend::waitForQueries(int number, int timeoutMs) const {
    constexpr int intervalMs = 20;
    int limit = timeoutMs / intervalMs;
    for (int count = 0; count <= limit; ++count) {
        bool done = queries_ >= number;
        // Always sleep at least one more interval after we are done, to wait for
        // any immediate post-query actions that the client may take (such as
        // marking this server as reachable during validation).
        usleep(intervalMs * 1000);
        if (done) {
            // For ensuring that calls have sufficient headroom for slow machines
            ALOGD("Query arrived in %d/%d of allotted time", count, limit);
            return true;
        }
    }
    return false;
}

}  // namespace test
