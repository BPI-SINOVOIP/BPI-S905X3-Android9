/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HWC_POWER_MODE_H
#define HWC_POWER_MODE_H
#include <BasicTypes.h>

typedef enum {
    MESON_POWER_BOOT = 0,
    MESON_POWER_CONNECTOR_IN,
    MESON_POWER_CONNECTOR_OUT,
} meson_power_mode_t;

class HwcPowerMode {
public:
    HwcPowerMode();
    ~HwcPowerMode();

    int32_t setScreenStatus(bool bBlank);
    bool getScreenStatus();

    int32_t setConnectorStatus(bool bConnected);

    bool needBlankScreen(bool bLayerPresent);

    meson_power_mode_t getMode();


protected:
    bool mScreenBlank;
    bool mConnectorPresent;
    meson_power_mode_t mPowerMode;
};
#endif
