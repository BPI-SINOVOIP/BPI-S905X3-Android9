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

package com.googlecode.android_scripting.facade.telephony;

import android.app.Service;
import android.content.Context;
import android.telephony.SubscriptionManager;

import com.android.ims.ImsConfig;
import com.android.ims.ImsException;
import com.android.ims.ImsManager;

import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcParameter;

/**
 * Exposes ImsManager functionality.
 */
public class ImsManagerFacade extends RpcReceiver {

    private final Service mService;
    private final Context mContext;
    private ImsManager mImsManager;

    public ImsManagerFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mContext = mService.getBaseContext();
        mImsManager = ImsManager.getInstance(mContext,
                SubscriptionManager.getDefaultVoicePhoneId());
    }

    /**
    * Reset IMS settings to factory default.
    */
    @Rpc(description = "Resets ImsManager settings to factory default.")
    public void imsFactoryReset() {
        mImsManager.factoryReset();
    }

    @Rpc(description = "Return True if Enhanced 4g Lte mode is enabled by platform.")
    public boolean imsIsEnhanced4gLteModeSettingEnabledByPlatform() {
        return mImsManager.isVolteEnabledByPlatform();
    }

    @Rpc(description = "Return True if Enhanced 4g Lte mode is enabled by user.")
    public boolean imsIsEnhanced4gLteModeSettingEnabledByUser() {
        return mImsManager.isEnhanced4gLteModeSettingEnabledByUser();
    }

    @Rpc(description = "Set Enhanced 4G mode.")
    public void imsSetEnhanced4gMode(
            @RpcParameter(name = "enable") Boolean enable) {
        mImsManager.setEnhanced4gLteModeSetting(enable);
    }

    @Rpc(description = "Check for VoLTE Provisioning.")
    public boolean imsIsVolteProvisionedOnDevice() {
        return mImsManager.isVolteProvisionedOnDevice();
    }

    @Rpc(description = "Set Modem Provisioning for VoLTE")
    public void imsSetVolteProvisioning(
            @RpcParameter(name = "enable") Boolean enable)
            throws ImsException{
        mImsManager.getConfigInterface().setProvisionedValue(
                ImsConfig.ConfigConstants.VLT_SETTING_ENABLED,
                enable? 1 : 0);
    }

    /**************************
     * Begin WFC Calling APIs
     **************************/

    @Rpc(description = "Return True if WiFi Calling is enabled for platform.")
    public boolean imsIsWfcEnabledByPlatform() {
        return ImsManager.isWfcEnabledByPlatform(mContext);
    }

    @Rpc(description = "Set whether or not WFC is enabled during roaming")
    public void imsSetWfcRoamingSetting(
                        @RpcParameter(name = "enable")
            Boolean enable) {
        mImsManager.setWfcRoamingSetting(mContext, enable);

    }

    @Rpc(description = "Return True if WiFi Calling is enabled during roaming.")
    public boolean imsIsWfcRoamingEnabledByUser() {
        return mImsManager.isWfcEnabledByPlatform();
    }

    @Rpc(description = "Set the Wifi Calling Mode of operation")
    public void imsSetWfcMode(
                        @RpcParameter(name = "mode")
            String mode)
            throws IllegalArgumentException {

        int mode_val;

        switch (mode.toUpperCase()) {
            case TelephonyConstants.WFC_MODE_WIFI_ONLY:
                mode_val =
                        ImsConfig.WfcModeFeatureValueConstants.WIFI_ONLY;
                break;
            case TelephonyConstants.WFC_MODE_CELLULAR_PREFERRED:
                mode_val =
                        ImsConfig.WfcModeFeatureValueConstants.CELLULAR_PREFERRED;
                break;
            case TelephonyConstants.WFC_MODE_WIFI_PREFERRED:
                mode_val =
                        ImsConfig.WfcModeFeatureValueConstants.WIFI_PREFERRED;
                break;
            case TelephonyConstants.WFC_MODE_DISABLED:
                if (mImsManager.isWfcEnabledByPlatform()
                        && mImsManager.isWfcEnabledByUser()) {
                    mImsManager.setWfcSetting(false);
                }
                return;
            default:
                throw new IllegalArgumentException("Invalid WfcMode");
        }

        if (mImsManager.isWfcEnabledByPlatform()
                && !mImsManager.isWfcEnabledByUser()) {
            mImsManager.setWfcSetting(true);
        }
        mImsManager.setWfcMode(mode_val);

        return;
    }

    @Rpc(description = "Return current WFC Mode if Enabled.")
    public String imsGetWfcMode() {
        if (!mImsManager.isWfcEnabledByUser()) {
            return TelephonyConstants.WFC_MODE_DISABLED;
        }
        return TelephonyUtils.getWfcModeString(
            mImsManager.getWfcMode());
    }

    @Rpc(description = "Return True if WiFi Calling is enabled by user.")
    public boolean imsIsWfcEnabledByUser() {
        return mImsManager.isWfcEnabledByUser();
    }

    @Rpc(description = "Set whether or not WFC is enabled")
    public void imsSetWfcSetting(
                        @RpcParameter(name = "enable")  Boolean enable) {
        mImsManager.setWfcSetting(enable);
    }

    /**************************
     * Begin VT APIs
     **************************/

    @Rpc(description = "Return True if Video Calling is enabled by the platform.")
    public boolean imsIsVtEnabledByPlatform() {
        return mImsManager.isVtEnabledByPlatform();
    }

    @Rpc(description = "Return True if Video Calling is provisioned for this device.")
    public boolean imsIsVtProvisionedOnDevice() {
        return mImsManager.isVtProvisionedOnDevice();
    }

    @Rpc(description = "Set Video Telephony Enabled")
    public void imsSetVtSetting(Boolean enabled) {
        mImsManager.setVtSetting(enabled);
    }

    @Rpc(description = "Return user enabled status for Video Telephony")
    public boolean imsIsVtEnabledByUser() {
        return mImsManager.isVtEnabledByUser();
    }

    @Override
    public void shutdown() {

    }
}
