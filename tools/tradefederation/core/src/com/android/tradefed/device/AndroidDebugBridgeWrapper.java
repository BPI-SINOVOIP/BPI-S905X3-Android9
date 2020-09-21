/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.annotations.VisibleForTesting;
import com.android.ddmlib.AndroidDebugBridge;
import com.android.ddmlib.AndroidDebugBridge.IDeviceChangeListener;
import com.android.ddmlib.DdmPreferences;
import com.android.ddmlib.IDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A wrapper that directs {@link IAndroidDebugBridge} calls to the 'real'
 * {@link AndroidDebugBridge}.
 */
class AndroidDebugBridgeWrapper implements IAndroidDebugBridge {

    private static final Pattern VERSION_PATTERN = Pattern.compile("^.*(\\d+)\\.(\\d+)\\.(\\d+).*");
    private static final Pattern REVISION_PATTERN = Pattern.compile(".*(Revision )(.*)$");
    private static final Pattern SUB_VERSION_PATTERN = Pattern.compile(".*(Version )(.*)");
    private static final Pattern PATH_PATTERN = Pattern.compile(".*(Installed as )(.*)");
    private AndroidDebugBridge mAdbBridge = null;

    /**
     * Creates a {@link AndroidDebugBridgeWrapper}.
     */
    AndroidDebugBridgeWrapper() {
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IDevice[] getDevices() {
        if (mAdbBridge == null) {
            throw new IllegalStateException("getDevices called before init");
        }
        return mAdbBridge.getDevices();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addDeviceChangeListener(IDeviceChangeListener listener) {
        AndroidDebugBridge.addDeviceChangeListener(listener);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void removeDeviceChangeListener(IDeviceChangeListener listener) {
        AndroidDebugBridge.removeDeviceChangeListener(listener);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void init(boolean clientSupport, String adbOsLocation) {
        AndroidDebugBridge.init(clientSupport);
        mAdbBridge = AndroidDebugBridge.createBridge(adbOsLocation, false);
    }

    /** {@inheritDoc} */
    @Override
    public String getAdbVersion(String adbOsLocation) {
        CommandResult res =
                getRunUtil().runTimedCmd(DdmPreferences.getTimeOut(), adbOsLocation, "version");
        if (CommandStatus.SUCCESS.equals(res.getStatus())) {
            StringBuilder version = new StringBuilder();
            Matcher m = VERSION_PATTERN.matcher(res.getStdout());
            if (m.find()) {
                version.append(m.group(1));
                version.append(".");
                version.append(m.group(2));
                version.append(".");
                version.append(m.group(3));
            } else {
                return null;
            }
            Matcher revision = REVISION_PATTERN.matcher(res.getStdout());
            if (revision.find()) {
                version.append("-");
                version.append(revision.group(2));
            }
            Matcher subVersion = SUB_VERSION_PATTERN.matcher(res.getStdout());
            if (subVersion.find()) {
                version.append(" subVersion: ");
                version.append(subVersion.group(2));
            }
            Matcher installPath = PATH_PATTERN.matcher(res.getStdout());
            if (installPath.find()) {
                version.append(" install path: ");
                version.append(installPath.group(2));
            }
            return version.toString();
        }
        CLog.e(
                "Failed to read adb version:\nstdout:%s\nstderr:%s",
                res.getStdout(), res.getStderr());
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void terminate() {
        AndroidDebugBridge.terminate();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void disconnectBridge() {
        AndroidDebugBridge.disconnectBridge();
    }

    /** Returns a {@link IRunUtil} to run processes. */
    @VisibleForTesting
    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }
}
