/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#define LOG_TAG "HDCP_RX_BINDER_TEST"

#include <android/hidl/base/1.0/IBase.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPService.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCP.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/IHDCPObserver.h>
#include <vendor/amlogic/hardware/miracast_hdcp2/1.0/types.h>

#include <android/hidl/manager/1.0/IServiceNotification.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

#include <media/hardware/HDCPAPI.h>
#include <cutils/log.h>
#include <vector>

#include "pes1_encrypt.h"
#include "pes2_encrypt.h"

uint32_t port = 9999;
using ::android::HDCPModule;
using namespace vendor::amlogic::hardware::miracast_hdcp2::V1_0;
using ::android::hardware::hidl_vec;
using ::android::hardware::Void;
using ::android::hardware::Return;
using ::android::sp;
using ::std::vector;
using ::android::hidl::base::V1_0::IBase;

namespace vendor {
namespace amlogic {
namespace hardware {
namespace miracast_hdcp2 {
namespace V1_0 {
struct HDCPObserver : public IHDCPObserver {
    // Methods from ::vendor::amlogic::hardware::miracast_hdcp2::V1_0::IHDCPObserver follow.
    Return<void> notify(int32_t msg, int32_t ext1, int32_t ext2) {
        switch (msg) {
            case HDCPModule::HDCP_INITIALIZATION_COMPLETE:
                break;
            case HDCPModule::HDCP_INITIALIZATION_FAILED:
                break;
            case HDCPModule::HDCP_SHUTDOWN_COMPLETE:
                break;
            case HDCPModule::HDCP_SHUTDOWN_FAILED:
                break;
            case HDCPModule::HDCP_UNAUTHENTICATED_CONNECTION:
                break;
            case HDCPModule::HDCP_UNAUTHORIZED_CONNECTION:
                break;
            case HDCPModule::HDCP_REVOKED_CONNECTION:
                break;
            case HDCPModule::HDCP_TOPOLOGY_EXECEEDED:
                break;
            case HDCPModule::HDCP_UNKNOWN_ERROR:
                break;
        }
        ALOGE("msg: %d, ext1: %d, ext2: %d\n", msg, ext1, ext2);
        return Void();
    };
};
}
}
}
}
}

int main()
{
    uint32_t trackIndex = 0;
    uint64_t inputCTR = 0;
    vector<uint8_t> pes1_out;
    vector<uint8_t> pes2_out;
    uint32_t i, j = 0;
    uint32_t rounds = 0;
    uint32_t residues = 0;
    FILE *fp;
    hidl_vec<uint8_t> pes;
    Status decrypt_res = Status::OK;

    sp<HDCPObserver> observer = new HDCPObserver;

    sp<IHDCPService> hdcp_service = IHDCPService::getService();
    if (!hdcp_service)
        ALOGE("failed to get hdcp2 service\n");

    sp<IHDCP> module = IHDCP::castFrom(hdcp_service->makeHDCP(false));
    if (!module)
        ALOGE("failed to create hdcp2 decryption module\n");

    module->setObserver(observer);

    module->initAsync(NULL, port);

    sleep(10);
    clock_t start = clock();

    pes.setToExternal(pes1_encrypt + 31, sizeof(pes1_encrypt) - 31);
    module->decrypt(pes, trackIndex  /* streamCTR */, inputCTR, 0,
            [&](Status status, const hidl_vec<uint8_t>& outData) {
            decrypt_res = status;
            pes1_out = std::move(outData);
            });
    rounds = (sizeof(pes1_encrypt) - 31) / 16;
    residues = (sizeof(pes1_encrypt) - 31) % 16;

    ALOGI("rounds: %d, residues: %d\n", rounds, residues);
#if 1
    fp = fopen("/data/pes1.decrypt", "w");
    for (i = 0; i < rounds; i++) {
        for (j = 0; j < 16; j++)
            fprintf(fp, "%02x ", pes1_out[i * 16 + j]);
        fprintf(fp, "\n");
    }
    for (i = 0; i < residues; i++)
        fprintf(fp, "%02x ", pes1_out[rounds * 16 + i]);
    fprintf(fp, "\n");
    fclose(fp);
#endif
    pes1_out.clear();
#if 1
    inputCTR = 775;
    pes.setToExternal(pes2_encrypt + 36, sizeof(pes2_encrypt) - 36);
    module->decrypt(pes, trackIndex  /* streamCTR */, inputCTR, 0,
            [&](Status status, const hidl_vec<uint8_t>& outData) {
            decrypt_res = status;
            pes2_out = std::move(outData);
            });

    rounds = (sizeof(pes2_encrypt) - 36) / 16;
    residues = (sizeof(pes2_encrypt) - 36) % 16;

    ALOGI("rounds: %d, residues: %d\n", rounds, residues);
#if 1
    fp = fopen("/data/pes2.decrypt", "w");
    for (i = 0; i < rounds; i++) {
        for (j = 0; j < 16; j++)
            fprintf(fp, "%02x ", pes2_out[i * 16 + j]);
        fprintf(fp, "\n");
    }
    for (i = 0; i < residues; i++)
        fprintf(fp, "%02x ", pes2_out[rounds * 16 + i]);
    fprintf(fp, "\n");
    fclose(fp);
#endif
#endif
    pes2_out.clear();

    ALOGI("Rx Time elapsed: %f\n", ((double)clock() - start));
    sleep(150);

    module->setObserver(NULL);
    observer.clear();
    module.clear();

    return 0;
}

