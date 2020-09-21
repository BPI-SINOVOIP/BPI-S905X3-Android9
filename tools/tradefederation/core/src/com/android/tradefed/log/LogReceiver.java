/*
 * Copyright (C) 2016 The Android Open Source Project
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
package com.android.tradefed.log;

import com.android.tradefed.device.BackgroundDeviceAction;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.LargeOutputReceiver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.util.StreamUtil;

public class LogReceiver {
    private BackgroundDeviceAction mDeviceAction;
    private LargeOutputReceiver mReceiver;
    private String mDesc;

    private static final int DELAY = 5;
    private static final int LOG_SIZE = 20 * 1024 * 1024;

    public LogReceiver(ITestDevice device, String cmd, String desc) {
        this(device, cmd, LOG_SIZE, DELAY, desc);
    }

    public LogReceiver(ITestDevice device, String cmd, String desc,
            long logcat_size,int delay_secs) {
        this(device, cmd, logcat_size, delay_secs, desc);
    }

    private LogReceiver(ITestDevice device, String cmd,
            long maxFileSize, int logStartDelay, String desc) {

        mReceiver = new LargeOutputReceiver(desc, device.getSerialNumber(),
                maxFileSize);
        mDeviceAction = new BackgroundDeviceAction(cmd, desc, device,
                mReceiver, logStartDelay);
        mDesc = desc;
    }

    public String getDescriptor() {
        return mDesc;
    }

    public void start() {
        mDeviceAction.start();
    }

    public void stop() {
        mDeviceAction.cancel();
        mReceiver.cancel();
        mReceiver.delete();
    }

    public InputStreamSource getData() {
        return mReceiver.getData();
    }

    public InputStreamSource getLogcatData(int maxBytes) {
        return mReceiver.getData(maxBytes);
    }

    public void clear() {
        mReceiver.clear();
    }

    public void postLog(ITestInvocationListener listener) {
        InputStreamSource stream = getData();
        try {
            listener.testLog(getDescriptor(), LogDataType.TEXT, getData());
        } finally {
            StreamUtil.cancel(stream);
        }
    }
}