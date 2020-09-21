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
 */
package com.android.tradefed.device;

import com.android.tradefed.config.Option;

import java.util.ArrayList;
import java.util.List;

/**
 * Container for {@link ITestDevice} {@link Option}s
 */
public class TestDeviceOptions {

    /** Do not provide a setter method for that Option as it might be misused. */
    @Option(name = "enable-root", description = "enable adb root on boot.")
    private boolean mEnableAdbRoot = true;

    @Option(name = "disable-keyguard",
            description = "attempt to disable keyguard once boot is complete.")
    private boolean mDisableKeyguard = true;

    @Option(name = "enable-logcat", description =
            "Enable background logcat capture when invocation is running.")
    private boolean mEnableLogcat = true;

    @Option(name = "max-tmp-logcat-file", description =
        "The maximum size of tmp logcat data to retain, in bytes. " +
        "Only used if --enable-logcat is set")
    private long mMaxLogcatDataSize = 20 * 1024 * 1024;

    @Option(name = "logcat-options", description =
            "Options to be passed down to logcat command, if unspecified, \"-v threadtime\" will " +
            "be used. Only used if --enable-logcat is set")
    private String mLogcatOptions = null;

    @Option(name = "fastboot-timeout", description =
            "time in ms to wait for a device to boot into fastboot.")
    private int mFastbootTimeout = 1 * 60 * 1000;

    @Option(name = "adb-recovery-timeout", description =
            "time in ms to wait for a device to boot into recovery.")
    private int mAdbRecoveryTimeout = 1 * 60 * 1000;

    @Option(name = "reboot-timeout", description =
            "time in ms to wait for a device to reboot to full system.")
    private int mRebootTimeout = 2 * 60 * 1000;

    @Option(name = "use-fastboot-erase", description =
            "use fastboot erase instead of fastboot format to wipe partitions")
    private boolean mUseFastbootErase = false;

    @Option(name = "unencrypt-reboot-timeout", description = "time in ms to wait for the device to "
            + "format the filesystem and reboot after unencryption")
    private int mUnencryptRebootTimeout = 0;

    @Option(name = "online-timeout", description = "default time in ms to wait for the device to "
            + "be visible on adb.", isTimeVal = true)
    private long mOnlineTimeout = 1 * 60 * 1000;

    @Option(name = "available-timeout", description = "default time in ms to wait for the device "
            + "to be available aka fully boot.")
    private long mAvailableTimeout = 6 * 60 * 1000;

    @Option(name = "conn-check-url",
            description = "default URL to be used for connectivity checks.")
    private String mConnCheckUrl = "http://www.google.com";

    @Option(name = "wifi-attempts",
            description = "default number of attempts to connect to wifi network.")
    private int mWifiAttempts = 5;

    @Option(name = "wifi-retry-wait-time",
            description = "the base wait time in ms between wifi connect retries. "
            + "The actual wait time would be a multiple of this value.")
    private int mWifiRetryWaitTime = 60 * 1000;

    @Option(
        name = "max-wifi-connect-time",
        isTimeVal = true,
        description = "the maximum amount of time to attempt to connect to wifi."
    )
    private long mMaxWifiConnectTime = 10 * 60 * 1000;

    @Option(name = "wifi-exponential-retry",
            description = "Change the wifi connection retry strategy from a linear wait time into"
                    + " a binary exponential back-offs when retrying.")
    private boolean mWifiExpoRetryEnabled = true;

    @Option(name = "wifiutil-apk-path", description = "path to the wifiutil APK file")
    private String mWifiUtilAPKPath = null;

    @Option(name = "post-boot-command",
            description = "shell command to run after reboots during invocation")
    private List<String> mPostBootCommands = new ArrayList<String>();

    @Option(name = "disable-reboot",
            description = "disables device reboots globally, making them no-ops")
    private boolean mDisableReboot = false;

    @Option(name = "cutoff-battery", description =
            "the minimum battery level required to continue the invocation. Scale: 0-100")
    private Integer mCutoffBattery = null;

    /**
     * Check whether adb root should be enabled on boot for this device
     */
    public boolean isEnableAdbRoot() {
        return mEnableAdbRoot;
    }

    /**
     * Check whether or not we should attempt to disable the keyguard once boot has completed
     */
    public boolean isDisableKeyguard() {
        return mDisableKeyguard;
    }

    /**
     * Set whether or not we should attempt to disable the keyguard once boot has completed
     */
    public void setDisableKeyguard(boolean disableKeyguard) {
        mDisableKeyguard = disableKeyguard;
    }

    /**
     * Get the approximate maximum size of a tmp logcat data to retain, in bytes.
     */
    public long getMaxLogcatDataSize() {
        return mMaxLogcatDataSize;
    }

    /**
     * Set the approximate maximum size of a tmp logcat to retain, in bytes
     */
    public void setMaxLogcatDataSize(long maxLogcatDataSize) {
        mMaxLogcatDataSize = maxLogcatDataSize;
    }

    /**
     * @return the timeout to boot into fastboot mode in msecs.
     */
    public int getFastbootTimeout() {
        return mFastbootTimeout;
    }

    /**
     * @param fastbootTimeout the timout in msecs to boot into fastboot mode.
     */
    public void setFastbootTimeout(int fastbootTimeout) {
        mFastbootTimeout = fastbootTimeout;
    }

    /**
     * @return the timeout in msecs to boot into recovery mode.
     */
    public int getAdbRecoveryTimeout() {
        return mAdbRecoveryTimeout;
    }

    /**
     * @param adbRecoveryTimeout the timeout in msecs to boot into recovery mode.
     */
    public void setAdbRecoveryTimeout(int adbRecoveryTimeout) {
        mAdbRecoveryTimeout = adbRecoveryTimeout;
    }

    /**
     * @return the timeout in msecs for the full system boot.
     */
    public int getRebootTimeout() {
        return mRebootTimeout;
    }

    /**
     * @param rebootTimeout the timeout in msecs for the system to fully boot.
     */
    public void setRebootTimeout(int rebootTimeout) {
        mRebootTimeout = rebootTimeout;
    }

    /**
     * @return whether to use fastboot erase instead of fastboot format to wipe partitions.
     */
    public boolean getUseFastbootErase() {
        return mUseFastbootErase;
    }

    /**
     * @param useFastbootErase whether to use fastboot erase instead of fastboot format to wipe
     * partitions.
     */
    public void setUseFastbootErase(boolean useFastbootErase) {
        mUseFastbootErase = useFastbootErase;
    }

    /**
     * @return the timeout in msecs for the filesystem to be formatted and the device to reboot
     * after unencryption.
     */
    public int getUnencryptRebootTimeout() {
        return mUnencryptRebootTimeout;
    }

    /**
     * @param unencryptRebootTimeout the timeout in msecs for the filesystem to be formatted and
     * the device to reboot after unencryption.
     */
    public void setUnencryptRebootTimeout(int unencryptRebootTimeout) {
        mUnencryptRebootTimeout = unencryptRebootTimeout;
    }

    /**
     * @return the default time in ms to to wait for a device to be online.
     */
    public long getOnlineTimeout() {
        return mOnlineTimeout;
    }

    public void setOnlineTimeout(long onlineTimeout) {
        mOnlineTimeout = onlineTimeout;
    }

    /**
     * @return the default time in ms to to wait for a device to be available.
     */
    public long getAvailableTimeout() {
        return mAvailableTimeout;
    }

    /**
     * @return the default URL to be used for connectivity tests.
     */
    public String getConnCheckUrl() {
        return mConnCheckUrl;
    }

    public void setConnCheckUrl(String url) {
      mConnCheckUrl = url;
    }

    /**
     * @return true if background logcat capture is enabled
     */
    public boolean isLogcatCaptureEnabled() {
        return mEnableLogcat;
    }

    /**
     * @return the default number of attempts to connect to wifi network.
     */
    public int getWifiAttempts() {
        return mWifiAttempts;
    }

    public void setWifiAttempts(int wifiAttempts) {
        mWifiAttempts = wifiAttempts;
    }

    /**
     * @return the base wait time between wifi connect retries.
     */
    public int getWifiRetryWaitTime() {
        return mWifiRetryWaitTime;
    }

    /** @return the maximum time to attempt to connect to wifi. */
    public long getMaxWifiConnectTime() {
        return mMaxWifiConnectTime;
    }

    /**
     * @return a list of shell commands to run after reboots.
     */
    public List<String> getPostBootCommands() {
        return mPostBootCommands;
    }

    /**
     * @return the minimum battery level to continue the invocation.
     */
    public Integer getCutoffBattery() {
        return mCutoffBattery;
    }

    /**
     * set the minimum battery level to continue the invocation.
     */
    public void setCutoffBattery(int cutoffBattery) {
        if (cutoffBattery < 0 || cutoffBattery > 100) {
            // Prevent impossible value.
            throw new RuntimeException(String.format("Battery cutoff wasn't changed,"
                    + "the value %s isn't within possible range (0-100).", cutoffBattery));
        }
        mCutoffBattery = cutoffBattery;
    }

    /**
     * @return the configured logcat options
     */
    public String getLogcatOptions() {
        return mLogcatOptions;
    }

    /**
     * Set the options to be passed down to logcat
     */
    public void setLogcatOptions(String logcatOptions) {
        mLogcatOptions = logcatOptions;
    }

    /**
     * @return if device reboot should be disabled
     */
    public boolean shouldDisableReboot() {
        return mDisableReboot;
    }

    /**
     * @return if the exponential retry strategy should be used.
     */
    public boolean isWifiExpoRetryEnabled() {
        return mWifiExpoRetryEnabled;
    }

    /** @return the wifiutil apk path */
    public String getWifiUtilAPKPath() {
        return mWifiUtilAPKPath;
    }
}
