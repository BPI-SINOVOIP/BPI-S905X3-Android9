/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <HwcPowerMode.h>
#include <MesonLog.h>

HwcPowerMode::HwcPowerMode() {
    mPowerMode = MESON_POWER_BOOT;
    mScreenBlank = true;
    mConnectorPresent = false;
}

HwcPowerMode::~HwcPowerMode() {
}

int32_t HwcPowerMode::setScreenStatus(bool bBlank) {
    mScreenBlank = bBlank;
    if (mPowerMode == MESON_POWER_BOOT && !mScreenBlank) {
        mPowerMode = mConnectorPresent ?
            MESON_POWER_CONNECTOR_IN : MESON_POWER_CONNECTOR_OUT;
    }

    return 0;
}

bool HwcPowerMode::getScreenStatus() {
    return mScreenBlank;
}

int32_t HwcPowerMode::setConnectorStatus(bool bConnected) {
    if (mPowerMode != MESON_POWER_BOOT && mConnectorPresent != bConnected) {
        mPowerMode = bConnected ?
            MESON_POWER_CONNECTOR_IN : MESON_POWER_CONNECTOR_OUT;
        MESON_LOGD("[%s]: power mode %d", __func__, mPowerMode);
    }
    mConnectorPresent = bConnected;
    return 0;
}

bool HwcPowerMode::needBlankScreen(bool bLayerPresent) {
    switch (mPowerMode) {
        case MESON_POWER_BOOT:
            return !(mConnectorPresent && bLayerPresent);
        case MESON_POWER_CONNECTOR_IN:
            return false;
        case MESON_POWER_CONNECTOR_OUT:
            return true;
        default:
            MESON_LOGE("Unkown power mode (%d).", mPowerMode);
    };

    return false;
}

meson_power_mode_t HwcPowerMode::getMode() {
    return mPowerMode;
}

