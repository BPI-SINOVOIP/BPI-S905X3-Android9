/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#define LOG_TAG "HDCP_TX_TEST"
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

#include "pes1.h"
#include "pes2.h"

char host[] = "127.0.0.1";
uint32_t port = 9999;

using ::android::HDCPModule;
using namespace vendor::amlogic::hardware::miracast_hdcp2::V1_0;
using ::android::hardware::hidl_vec;
using ::android::hardware::Void;
using ::android::hardware::Return;
using ::android::sp;
using ::std::vector;
using ::android::hidl::base::V1_0::IBase;

volatile bool initialize = false;

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
                initialize = true;
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
    uint32_t i, j = 0;
    uint32_t rounds = 0;
    uint32_t residues = 0;
    FILE *fp;
    uint8_t HDCP_private_data[16];
    hidl_vec<uint8_t> pes;
    Status encrypt_res = Status::OK;

    sp<HDCPObserver> observer = new HDCPObserver;

    sp<IHDCPService> hdcp_service = IHDCPService::getService();
    if (!hdcp_service)
        ALOGE("failed to get hdcp2 service\n");

    sp<IHDCP> module = IHDCP::castFrom(hdcp_service->makeHDCP(true));
    if (!module)
        ALOGE("failed to create hdcp2 encryption module\n");

    module->setObserver(observer);
    module->initAsync(host, port);
#if 1

    while (!initialize)
        sleep(1);

    clock_t start = clock();
    ALOGI("After HDCP initialized\n");
    pes.setToExternal(pes1 + 14, sizeof(pes1) - 14);
    module->encrypt(
            pes, trackIndex  /* streamCTR */,
            [&] (Status status, uint64_t outInputCTR, const hidl_vec<uint8_t>& outData) {
            encrypt_res = status;
            inputCTR = outInputCTR;
            pes1_out = std::move(outData);
            });

    rounds = (sizeof(pes1) - 14) / 16;
    residues = (sizeof(pes1) - 14) % 16;

    ALOGI("rounds: %d, residues: %d\n", rounds, residues);
#if 1
    fp = fopen("/data/pes1", "w");
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
    HDCP_private_data[0] = 0x00;

    HDCP_private_data[1] =
        (((trackIndex >> 30) & 3) << 1) | 1;

    HDCP_private_data[2] = (trackIndex >> 22) & 0xff;

    HDCP_private_data[3] =
        (((trackIndex >> 15) & 0x7f) << 1) | 1;

    HDCP_private_data[4] = (trackIndex >> 7) & 0xff;

    HDCP_private_data[5] =
        ((trackIndex & 0x7f) << 1) | 1;

    HDCP_private_data[6] = 0x00;

    HDCP_private_data[7] =
        (((inputCTR >> 60) & 0x0f) << 1) | 1;

    HDCP_private_data[8] = (inputCTR >> 52) & 0xff;

    HDCP_private_data[9] =
        (((inputCTR >> 45) & 0x7f) << 1) | 1;

    HDCP_private_data[10] = (inputCTR >> 37) & 0xff;

    HDCP_private_data[11] =
        (((inputCTR >> 30) & 0x7f) << 1) | 1;

    HDCP_private_data[12] = (inputCTR >> 22) & 0xff;

    HDCP_private_data[13] =
        (((inputCTR >> 15) & 0x7f) << 1) | 1;

    HDCP_private_data[14] = (inputCTR >> 7) & 0xff;

    HDCP_private_data[15] =
        ((inputCTR & 0x7f) << 1) | 1;

    //NORMAL_LOGGING("private:", HDCP_private_data, 16);
    pes.setToExternal(pes2 + 19, sizeof(pes2) - 19);
    module->encrypt(
            pes, trackIndex  /* streamCTR */,
            [&] (Status status, uint64_t outInputCTR, const hidl_vec<uint8_t>& outData) {
            encrypt_res = status;
            inputCTR = outInputCTR;
            pes1_out = std::move(outData);
            });


    rounds = (sizeof(pes2) - 19) / 16;
    residues = (sizeof(pes2) - 19) % 16;

    ALOGI("rounds: %d, residues: %d\n", rounds, residues);
#if 1
    fp = fopen("/data/pes2", "w");
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

    HDCP_private_data[0] = 0x00;

    HDCP_private_data[1] =
        (((trackIndex >> 30) & 3) << 1) | 1;

    HDCP_private_data[2] = (trackIndex >> 22) & 0xff;

    HDCP_private_data[3] =
        (((trackIndex >> 15) & 0x7f) << 1) | 1;

    HDCP_private_data[4] = (trackIndex >> 7) & 0xff;

    HDCP_private_data[5] =
        ((trackIndex & 0x7f) << 1) | 1;

    HDCP_private_data[6] = 0x00;

    HDCP_private_data[7] =
        (((inputCTR >> 60) & 0x0f) << 1) | 1;

    HDCP_private_data[8] = (inputCTR >> 52) & 0xff;

    HDCP_private_data[9] =
        (((inputCTR >> 45) & 0x7f) << 1) | 1;

    HDCP_private_data[10] = (inputCTR >> 37) & 0xff;

    HDCP_private_data[11] =
        (((inputCTR >> 30) & 0x7f) << 1) | 1;

    HDCP_private_data[12] = (inputCTR >> 22) & 0xff;

    HDCP_private_data[13] =
        (((inputCTR >> 15) & 0x7f) << 1) | 1;

    HDCP_private_data[14] = (inputCTR >> 7) & 0xff;

    HDCP_private_data[15] =
        ((inputCTR & 0x7f) << 1) | 1;

#endif
    ALOGI("Tx Time elapsed: %f\n", ((double)clock() - start));

#endif

    sleep(100);

    module->setObserver(NULL);
    observer.clear();
    module.clear();

    return 0;
}

