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

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <android-base/file.h>
#include <android-base/logging.h>

#include <ese/ese.h>
ESE_INCLUDE_HW(ESE_HW_NXP_PN80T_NQ_NCI);

#include "AlaLib.h"
#include "IChannel.h"

static struct EseInterface static_ese;

static INT16 static_ese_open() {
    ese_init((&static_ese), ESE_HW_NXP_PN80T_NQ_NCI);
    auto res = ese_open(&static_ese, nullptr);
    if (res != 0) {
        LOG(ERROR) << "ese_open result: " << (int) res;
        return EE_ERROR_OPEN_FAIL;
    }
    LOG(DEBUG) << "ese_open success";
    return 1; // arbitrary fake channel
}

static bool static_ese_close(INT16 /* handle */) {
    LOG(DEBUG) << "ese_close";
    ese_close(&static_ese);
    return true;
}

static void log_hexdump(const std::string& prefix, UINT8 *buf, size_t len) {
    for (size_t i = 0; i < len; i += 16) {
        std::ostringstream o;
        for (size_t j = i; j < i + 16 && j < len; j++) {
            o << std::hex << std::setw(2) << std::setfill('0') << (int)buf[j] << " ";
        }
        LOG(DEBUG) << prefix << ": " << o.str();
    }
}

static bool static_ese_transceive(UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
                     INT32 recvBufferMaxSize, INT32& recvBufferActualSize, INT32 /* timeoutMillisec */) {
    log_hexdump("tx", xmitBuffer, xmitBufferSize);
    auto res = ese_transceive(&static_ese,
        xmitBuffer, xmitBufferSize,
        recvBuffer, recvBufferMaxSize);
    if (res < 0 || ese_error(&static_ese)) {
        LOG(ERROR) << "ese_transceive result: " << (int) res
            << " code " << ese_error_code(&static_ese)
            << " message: " << ese_error_message(&static_ese);
        recvBufferActualSize = 0;
        return false;
    }
    recvBufferActualSize = res;
    log_hexdump("rx", recvBuffer, res);
    return true;
}

static void static_ese_doeSE_Reset() {
    LOG(DEBUG) << "static_ese_doeSE_Reset (doing nothing)";
}

IChannel_t static_ese_ichannel = {
    static_ese_open,
    static_ese_close,
    static_ese_transceive,
    static_ese_doeSE_Reset,
    nullptr
};

static bool readFileToString(const std::string& filename, std::string* result) {
    if (!android::base::ReadFileToString(filename, result)) {
        PLOG(ERROR) << "Failed to read from " << filename;
        return false;
    }
    return true;
}

typedef struct ls_update_t {
    std::string shadata;
    std::string source;
    std::string dest;
} ls_update_t;

static bool send_ls_update(const ls_update_t &update) {
    UINT8 version_buf[4];
    auto status = ALA_lsGetVersion(version_buf);
    if (status != STATUS_SUCCESS) {
        LOG(ERROR) << "ALA_lsGetVersion failed with status " << (int) status;
        return false;
    }
    log_hexdump("version", version_buf, sizeof(version_buf));
    UINT8 respSW_buf[4];
    status = ALA_Start(
        const_cast<char *>(update.source.c_str()),
        const_cast<char *>(update.dest.c_str()),
        const_cast<UINT8 *>(reinterpret_cast<const UINT8*>(update.shadata.data())),
        update.shadata.size(),
        respSW_buf);
    if (status != STATUS_SUCCESS) {
        LOG(ERROR) << "ALA_Start failed with status " << (int) status;
        return false;
    }
    log_hexdump("respSW", respSW_buf, sizeof(respSW_buf));
    LOG(DEBUG) << "All interactions completed";
    return true;
}

static bool parse_args(char* argv[], ls_update_t *update) {
    char **argp = argv + 1;
    if (*argp == nullptr) {
        LOG(ERROR) << "No shadata supplied";
        return false;
    }
    if (!readFileToString(*argp, &update->shadata)) {
        return false;
    }
    argp++;
    if (*argp == nullptr) {
        LOG(ERROR) << "No source supplied";
        return false;
    }
    update->source = *argp;
    argp++;
    if (*argp == nullptr) {
        LOG(ERROR) << "No dest supplied";
        return false;
    }
    update->dest = *argp;
    argp++;
    if (*argp != nullptr) {
        LOG(ERROR) << "Extra arguments present";
        return false;
    }
    return true;
}

static bool ls_update_cli(char* argv[]) {
    ls_update_t update;
    if (!parse_args(argv, &update)) {
        return false;
    }
    auto status = ALA_Init(&static_ese_ichannel);
    if (status != STATUS_SUCCESS) {
        LOG(ERROR) << "ALA_Init failed with status " << (int) status;
        return false;
    }
    auto success = send_ls_update(update);
    if (ALA_DeInit()) {
        LOG(INFO) << "ALA_DeInit succeeded";
    } else {
        LOG(ERROR) << "ALA_DeInit failed";
        return false;
    }
    return success;
}

int main(int /* argc */, char* argv[]) {
    setenv("ANDROID_LOG_TAGS", "*:v", 1); // TODO: remove this line.
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));
    LOG(DEBUG) << "Start of ESE provisioning";
    return ls_update_cli(argv) ? 0 : -1;
}
