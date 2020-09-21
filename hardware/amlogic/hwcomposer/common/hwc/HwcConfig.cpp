/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <HwcConfig.h>
#include <MesonLog.h>
#include <cutils/properties.h>
#include <systemcontrol.h>
#include <misc.h>

int32_t HwcConfig::getFramebufferSize(int disp, uint32_t & width, uint32_t & height) {
    char uiMode[PROPERTY_VALUE_MAX] = {0};
    if (disp == 0) {
        /*primary display*/
        if (sys_get_string_prop("vendor.ui_mode", uiMode) > 0) {
            if (!strncmp(uiMode, "720", 3)) {
                width  = 1280;
                height = 720;
            } else if (!strncmp(uiMode, "1080", 4)) {
                width  = 1920;
                height = 1080;
            } else {
                MESON_ASSERT(0, "%s: get not support mode [%s] from vendor.ui_mode",
                    __func__, uiMode);
            }
        } else {
        #ifdef HWC_PRIMARY_FRAMEBUFFER_WIDTH
            width  = HWC_PRIMARY_FRAMEBUFFER_WIDTH;
            height = HWC_PRIMARY_FRAMEBUFFER_HEIGHT;
        #else
            MESON_ASSERT(0, "HWC_PRIMARY_FRAMEBUFFER_WIDTH not set.");
        #endif
        }
    } else {
    /*extend display*/
    #ifdef HWC_EXTEND_FRAMEBUFFER_WIDTH
        width = HWC_EXTEND_FRAMEBUFFER_WIDTH;
        height = HWC_EXTEND_FRAMEBUFFER_HEIGHT;
    #else
        MESON_ASSERT(0, "HWC_EXTEND_FRAMEBUFFER_WIDTH not set.");
    #endif
    }

    MESON_LOGI("HwcConfig::default frame buffer size (%d x %d)", width, height);
    return 0;
}

uint32_t HwcConfig::getDisplayNum() {
    return HWC_DISPLAY_NUM;
}

hwc_connector_t HwcConfig::getConnectorType(int disp) {
    hwc_connector_t connector_type = HWC_CONNECTOR_NULL;
    const char * connectorstr = NULL;
    if (disp == 0) {
        #ifdef HWC_PRIMARY_CONNECTOR_TYPE
            connectorstr = HWC_PRIMARY_CONNECTOR_TYPE;
        #else
            MESON_ASSERT(0, "HWC_PRIMARY_CONNECTOR_TYPE not set.");
        #endif
    } else {
        #ifdef HWC_EXTEND_CONNECTOR_TYPE
            connectorstr = HWC_EXTEND_CONNECTOR_TYPE;
        #else
            MESON_ASSERT(0, "HWC_EXTEND_CONNECTOR_TYPE not set.");
        #endif
    }

    if (connectorstr != NULL) {
        if (strcasecmp(connectorstr, "hdmi") == 0) {
            connector_type = HWC_HDMI_CVBS;
        } else if (strcasecmp(connectorstr, "panel") == 0) {
            connector_type = HWC_PANEL_ONLY;
        } else if (strcasecmp(connectorstr, "cvbs") == 0) {
            connector_type = HWC_CVBS_ONLY;
        } else if (strcasecmp(connectorstr, "hdmi-only") == 0) {
            connector_type = HWC_HDMI_ONLY;
        } else {
            MESON_LOGE("%s-%d get connector type failed.", __func__, disp);
        }
    }

    MESON_LOGD("%s-%d get connector type %s-%d",
        __func__, disp, connectorstr, connector_type);
    return connector_type;
}

hwc_pipe_policy_t HwcConfig::getPipeline() {
    const char * pipeStr = "default";
#ifdef HWC_PIPELINE
    pipeStr = HWC_PIPELINE;
#endif

    if (strcasecmp(pipeStr, "default") == 0) {
        return HWC_PIPE_DEFAULT;
    } else if (strcasecmp(pipeStr, "dual") == 0) {
        return HWC_PIPE_DUAL;
    } else if (strcasecmp(pipeStr, "VIU1VDINVIU2") == 0) {
        return HWC_PIPE_LOOPBACK;
    } else {
        MESON_ASSERT(0, "getPipeline %s failed.", pipeStr);
    }

    return HWC_PIPE_DEFAULT;
}

hwc_modes_policy_t HwcConfig::getModePolicy(int disp) {
    UNUSED(disp);
#ifdef HWC_ENABLE_FULL_ACTIVE_MODE
    return FULL_ACTIVE_POLICY;
#elif defined HWC_ENABLE_ACTIVE_MODE
    return ACTIVE_MODE_POLICY;
#elif defined HWC_ENABLE_REAL_MODE
    return REAL_MODE_POLICY;
#else
    return FIXED_SIZE_POLICY;
#endif
}

bool HwcConfig::isHeadlessMode() {
#ifdef HWC_ENABLE_HEADLESS_MODE
        return true;
#else
        return false;
#endif
}

int32_t HwcConfig::headlessRefreshRate() {
#ifdef HWC_HEADLESS_REFRESHRATE
        return HWC_HEADLESS_REFRESHRATE;
#else
        MESON_ASSERT(0, "HWC_HEADLESS_REFRESHRATE not set.");
        return 1;
#endif
}

bool HwcConfig::softwareVsyncEnabled() {
#ifdef HWC_ENABLE_SOFTWARE_VSYNC
    return true;
#else
    return false;
#endif
}

bool HwcConfig::preDisplayCalibrateEnabled() {
#ifdef HWC_ENABLE_PRE_DISPLAY_CALIBRATE
    return true;
#else
    return false;
#endif
}

bool HwcConfig::primaryHotplugEnabled() {
#ifdef HWC_ENABLE_PRIMARY_HOTPLUG
    return true;
#else
    return false;
#endif
}

bool HwcConfig::secureLayerProcessEnabled() {
#ifdef HWC_ENABLE_SECURE_LAYER_PROCESS
        return true;
#else
        return false;
#endif
}

bool HwcConfig::cursorPlaneDisabled() {
#ifdef HWC_DISABLE_CURSOR_PLANE
        return true;
#else
        return false;
#endif
}

bool HwcConfig::defaultHdrCapEnabled() {
#ifdef HWC_ENABLE_DEFAULT_HDR_CAPABILITIES
    return true;
#else
    return false;
#endif
}

bool HwcConfig::forceClientEnabled() {
#ifdef HWC_FORCE_CLIENT_COMPOSITION
    return true;
#else
    return false;
#endif
}

bool HwcConfig::alwaysVdinLoopback() {
#ifdef HWC_PIPE_VIU1VDINVIU2_ALWAYS_LOOPBACK
    return true;
#else
    return false;
#endif
}

bool HwcConfig::dynamicSwitchConnectorEnabled() {
#ifdef HWC_DYNAMIC_SWITCH_CONNECTOR
    return true;
#else
    return false;
#endif
}

bool HwcConfig::dynamicSwitchViuEnabled() {
#ifdef HWC_DYNAMIC_SWITCH_VIU
    return true;
#else
    return false;
#endif
}

void HwcConfig::dump(String8 & dumpstr) {
    if (isHeadlessMode()) {
        dumpstr.appendFormat("\t HeadlessMode refreshrate: %d", headlessRefreshRate());
        dumpstr.append("\n");
    } else {
        int displaynum = getDisplayNum();
        for (int i = 0; i < displaynum; i++) {
            uint32_t w,h;
            getFramebufferSize(i, w, h);

            dumpstr.appendFormat("Display (%d) %d x %d :\n", i, w, h);
            dumpstr.appendFormat("\t ConntecorConfig: %d", getConnectorType(i));
            dumpstr.appendFormat("\t ModePolicy: %d", getModePolicy(i));
            dumpstr.append("\n");
            dumpstr.appendFormat("\t PipelineConfig: %d", getPipeline());
            dumpstr.append("\n");
            dumpstr.appendFormat("\t SoftwareVsync: %s", softwareVsyncEnabled() ? "Y" : "N");
            dumpstr.appendFormat("\t CursorPlane: %s", cursorPlaneDisabled() ? "N" : "Y");
            dumpstr.append("\n");
            dumpstr.appendFormat("\t PrimaryHotplug: %s", primaryHotplugEnabled() ? "Y" : "N");
            dumpstr.appendFormat("\t SecureLayer: %s", secureLayerProcessEnabled() ? "Y" : "N");
            dumpstr.append("\n");
            dumpstr.appendFormat("\t PreDisplayCalibrate: %s", preDisplayCalibrateEnabled() ? "Y" : "N");
            dumpstr.appendFormat("\t DefaultHdr: %s", defaultHdrCapEnabled() ? "Y" : "N");
            dumpstr.append("\n");
            dumpstr.appendFormat("\t ForceClient: %s", forceClientEnabled() ? "Y" : "N");
            dumpstr.append("\n");
            dumpstr.appendFormat("\t DynamicSwitchConnector: %s", dynamicSwitchConnectorEnabled() ? "Y" : "N");
            dumpstr.append("\n");
            dumpstr.appendFormat("\t DynamicSwitchViu: %s", dynamicSwitchViuEnabled() ? "Y" : "N");
            dumpstr.append("\n");
        }
    }

    dumpstr.append("\n");
}

