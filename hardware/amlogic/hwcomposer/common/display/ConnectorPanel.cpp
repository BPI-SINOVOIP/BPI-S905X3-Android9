/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "ConnectorPanel.h"
#include <HwDisplayCrtc.h>
#include <misc.h>
#include <MesonLog.h>
#include "AmFramebuffer.h"
#include "AmVinfo.h"
#include <string>
#include <systemcontrol.h>

ConnectorPanel::ConnectorPanel(int32_t drvFd, uint32_t id)
    :   HwDisplayConnector(drvFd, id) {
    parseLcdInfo();
    if (mTabletMode) {
        snprintf(mName, 64, "Tablet-%d", id);
    } else {
        snprintf(mName, 64, "TV-%d", id);
    }
}

ConnectorPanel::~ConnectorPanel() {
}

int32_t ConnectorPanel::loadProperities() {
    loadPhysicalSize();
    loadDisplayModes();
    return 0;
}

int32_t ConnectorPanel::update() {
    return 0;
}

const char * ConnectorPanel::getName() {
    return mName;
}

drm_connector_type_t ConnectorPanel::getType() {
    return DRM_MODE_CONNECTOR_PANEL;
}

bool ConnectorPanel::isRemovable() {
    return false;
}

bool ConnectorPanel::isConnected(){
    return true;
}

bool ConnectorPanel::isSecure(){
    return true;
}

int32_t ConnectorPanel::parseLcdInfo() {
    /*
    typical info:
    "lcd vinfo:\n"
    "    lcd_mode:              %s\n"
    "    name:                  %s\n"
    "    mode:                  %d\n"
    "    width:                 %d\n"
    "    height:                %d\n"
    "    field_height:          %d\n"
    "    aspect_ratio_num:      %d\n"
    "    aspect_ratio_den:      %d\n"
    "    sync_duration_num:     %d\n"
    "    sync_duration_den:     %d\n"
    "    screen_real_width:     %d\n"
    "    screen_real_height:    %d\n"
    "    htotal:                %d\n"
    "    vtotal:                %d\n"
    "    fr_adj_type:           %d\n"
    "    video_clk:             %d\n"
    "    viu_color_fmt:         %d\n"
    "    viu_mux:               %d\n\n",
    */

    const char * lcdInfoPath = "/sys/class/lcd/vinfo";
    const int valLenMax = 64;
    std::string lcdInfo;

    if (sc_read_sysfs(lcdInfoPath, lcdInfo) == 0 &&
        lcdInfo.size() > 0) {
       // MESON_LOGD("Lcdinfo:(%s)", lcdInfo.c_str());

        std::size_t lineStart = 0;

        /*parse lcd mode*/
        const char * modeStr = " lcd_mode: ";
        lineStart = lcdInfo.find(modeStr);
        lineStart += strlen(modeStr);
        std::string valStr = lcdInfo.substr(lineStart, valLenMax);
        MESON_LOGD("lcd_mode: value [%s]", valStr.c_str());
        if (valStr.find("tablet", 0) != std::string::npos) {
            mTabletMode = true;
        } else {
            mTabletMode = false;
        }

        if (mTabletMode) {
            /*parse display info mode*/
            const char * infoPrefix[] = {
                " width:",
                " height:",
                " sync_duration_num:",
                " sync_duration_den:",
            };
            const int infoValueIdx[] = {
                LCD_WIDTH,
                LCD_HEIGHT,
                LCD_SYNC_DURATION_NUM,
                LCD_SYNC_DURATION_DEN,
            };
            static int infoNum = sizeof(infoValueIdx) / sizeof(int);

            MESON_LOGD("------------Lcdinfo parse start------------\n");
            for (int i = 0; i < infoNum; i ++) {
                lineStart = lcdInfo.find(infoPrefix[i], lineStart);
                lineStart += strlen(infoPrefix[i]);
                std::string valStr = lcdInfo.substr(lineStart, valLenMax);
                mLcdValues[infoValueIdx[i]] = (uint32_t)std::stoul(valStr);
                MESON_LOGD("[%s] : [%d]\n", infoPrefix[i], mLcdValues[infoValueIdx[i]]);
            }
            MESON_LOGD("------------Lcdinfo parse end------------\n");
        }
    } else {
        MESON_LOGE("parseLcdInfo ERROR.");
    }

    return 0;
}

int32_t ConnectorPanel::loadDisplayModes() {
    mDisplayModes.clear();

    if (mTabletMode) {
        drm_mode_info_t modeInfo = {
            "panel",
            DEFAULT_DISPLAY_DPI,
            DEFAULT_DISPLAY_DPI,
            mLcdValues[LCD_WIDTH],
            mLcdValues[LCD_HEIGHT],
            (float)mLcdValues[LCD_SYNC_DURATION_NUM]/mLcdValues[LCD_SYNC_DURATION_DEN]};
        mDisplayModes.emplace(mDisplayModes.size(), modeInfo);
        MESON_LOGE("use default value,get display mode: %s", modeInfo.name);
    } else {
        std::string dispmode;
        vmode_e vmode = VMODE_MAX;
        if (NULL != mCrtc) {
            mCrtc->readCurDisplayMode(dispmode);
            vmode = vmode_name_to_mode(dispmode.c_str());
        }

        addDisplayMode(dispmode);

        //for tv display mode.
        const unsigned int pos = dispmode.find("60hz", 0);
        if (pos != std::string::npos) {
            dispmode.replace(pos, 4, "50hz");
            addDisplayMode(dispmode);
        } else {
            MESON_LOGE("loadDisplayModes can not find 60hz in %s", dispmode.c_str());
        }
    }
    return 0;
}

void ConnectorPanel::getHdrCapabilities(drm_hdr_capabilities * caps) {
    if (caps) {
        memset(caps, 0, sizeof(drm_hdr_capabilities));
    }
}

void ConnectorPanel:: dump(String8& dumpstr) {
    dumpstr.appendFormat("Connector (Panel,  %d)\n",1);
    dumpstr.append("   CONFIG   |   VSYNC_PERIOD   |   WIDTH   |   HEIGHT   |"
        "   DPI_X   |   DPI_Y   \n");
    dumpstr.append("------------+------------------+-----------+------------+"
        "-----------+-----------\n");

    std::map<uint32_t, drm_mode_info_t>::iterator it = mDisplayModes.begin();
    for ( ; it != mDisplayModes.end(); ++it) {
        dumpstr.appendFormat(" %2d     |      %.3f      |   %5d   |   %5d    |"
            "    %3d    |    %3d    \n",
                 it->first,
                 it->second.refreshRate,
                 it->second.pixelW,
                 it->second.pixelH,
                 it->second.dpiX,
                 it->second.dpiY);
    }
}

