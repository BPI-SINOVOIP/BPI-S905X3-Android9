/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#define LOG_TAG "TvTestInterface"
#include <utils/Log.h>
#include <binder/IPCThreadState.h>
#include "TvServerHidlClient.h"

using namespace android;

int main(int argc, char *argv[]) {

    for (int i=0; i<argc; i++) {
        ALOGI("input param argv[%d] = %s\n", i, argv[i]);
    }
    char param[256] = {0};
    sprintf(param, "{\"fe\":{\"mode\":17104899,\"mod\":7,\"freq\":%s},\"v\":{\"pid\":%s,\"fmt\":0},\"a\":{\"pid\":%s,\"fmt\":3,\"AudComp\":0},\"p\":{\"pid\":%s},\"para\":{\"max\":{\"time\":600},\"path\":\"/data/user_de/0/com.droidlogic.tvinput/cache\"}}",
        argv[3], argv[4], argv[5], argv[4]);

    ALOGI("param = %s", param);
    int cmd= atoi(argv[1]);
    sp<TvServerHidlClient> TvSession = TvServerHidlClient::connect(CONNECT_TYPE_EXTEND);
    if (TvSession != NULL) {
        TvSession->stopTv();
        TvSession->switchInputSrc(11); //dtv source
        TvSession->sendPlayCmd(cmd, argv[2], param);
        TvSession.clear();
    }
    exit(0);
    return 0;
}
