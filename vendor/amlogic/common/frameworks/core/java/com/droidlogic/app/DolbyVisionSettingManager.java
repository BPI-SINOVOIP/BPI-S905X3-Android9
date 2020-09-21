/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Luan.Yuan
 *  @version  1.0
 *  @date     2017/02/21
 *  @par function description:
 *  - This is Dolby Vision Manager, control DV feature, sysFs, proc env.
 */
package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class DolbyVisionSettingManager {
    private static final String TAG                 = "DolbyVisionSettingManager";

    private static final String PROP_DOLBY_VISION_ENABLE  = "persist.vendor.sys.dolbyvision.enable";
    private static final String PROP_DOLBY_VISION_TV_ENABLE  = "persist.vendor.sys.tv.dolbyvision.enable";

    public static final int DOVISION_DISABLE        = 0;
    public static final int DOVISION_ENABLE         = 1;

    public static final int OUPUT_MODE_STATE_INIT   = 0;
    public static final int OUPUT_MODE_STATE_SWITCH = 1;
    private Context mContext;
    private SystemControlManager mSystenControl;

    public DolbyVisionSettingManager(Context context){
        mContext = context;
        mSystenControl = SystemControlManager.getInstance();
    }

    public void initSetDolbyVision() {
        mSystenControl.initDolbyVision(OUPUT_MODE_STATE_INIT);
    }

    /* *
     * @Description: Enable/Disable Dolby Vision
     * @params: state: 1:Enable  DV
     *                 0:Disable DV
     */
    public void setDolbyVisionEnable(int state) {
        mSystenControl.setDolbyVisionEnable(state);
    }

    /* *
     * @Description: Determine Whether TV support Dolby Vision
     * @return: if TV support Dolby Vision
     *              return the Highest resolution Tv supported.
     *          else
     *              return ""
     */
    public String isTvSupportDolbyVision() {
        return mSystenControl.isTvSupportDolbyVision();
    }

    /* *
     * @Description: get current state of Dolby Vision
     * @return: if DV is Enable  return true
     *                   Disable return false
     */
    public boolean isDolbyVisionEnable() {
        if (!isTvSupportDolbyVision().equals("")) {
            return mSystenControl.getPropertyBoolean(PROP_DOLBY_VISION_TV_ENABLE, false);
        } else {
            return mSystenControl.getPropertyBoolean(PROP_DOLBY_VISION_ENABLE, false);
        }
    }

    public int getDolbyVisionType() {
        return mSystenControl.getDolbyVisionType();
    }
    public long resolveResolutionValue(String mode) {
        return mSystenControl.resolveResolutionValue(mode);
    }

    /* *
     * @Description: set Dolby Vision Graphics Priority when DV is Enabled.
     * @params: "0":Video Priority    "1":Graphics Priority
     */
    public void setGraphicsPriority(String mode) {
        mSystenControl.setGraphicsPriority(mode);
    }

    /* *
     * @Description: set Dolby Vision Graphics Priority when DV is Enabled.
     * @return: current Priority
     */
    public String getGraphicsPriority() {
        return mSystenControl.getGraphicsPriority();
    }
}
