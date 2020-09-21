/*
 * Copyright (C) 2012 The Android Open Source Project
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
package com.android.tradefed.result;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;

import com.google.common.annotations.VisibleForTesting;

import java.net.InetAddress;
import java.net.UnknownHostException;



/**
 * An {@link EmailResultReporter} that will send email when invocation fails due to a device not
 * available exception.
 */
@OptionClass(alias = "device-unavail-email")
public class DeviceUnavailEmailResultReporter extends EmailResultReporter {

    @Override
    protected boolean shouldSendMessage() {
        return getInvocationException() instanceof DeviceNotAvailableException;
    }

    @Override
    protected String generateEmailSubject() {
        StringBuilder subj = new StringBuilder();
        subj.append("Device unavailable ");
        for (IBuildInfo build : getInvocationContext().getBuildInfos()) {
            subj.append(build.toString());
        }
        subj.append(" ");
        subj.append(String.format("hostname %s", getHostname()));

        // Sample email subject: Device unavailable: mantaray-user JDQ39
        // 015d172c980c2208 atl-034.mtv.corp.google.com
        return subj.toString();
    }

    /**
     * Fetch the hostname and returns it, or returns "Unknown" if an error occurs.
     */
    @VisibleForTesting
    String getHostname() {
        try {
            return InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            CLog.e(e);
            return "Unknown";
        }
    }
}
