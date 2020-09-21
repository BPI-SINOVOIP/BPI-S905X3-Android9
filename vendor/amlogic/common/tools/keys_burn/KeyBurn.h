/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef ANDROID_KEYBURN_H
#define ANDROID_KEYBURN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
int writeKey(int type, char * path);
#ifdef __cplusplus
}
#endif
enum key_type {
    HDCP_RX_1_4 = 0,
    HDCP_RX_2_2,
    HDCP_TX_1_4,
    HDCP_TX_2_2,
    PLAYREADY,
    WIDEVINE,
};

#endif // ANDROID_KEYBURN_H
