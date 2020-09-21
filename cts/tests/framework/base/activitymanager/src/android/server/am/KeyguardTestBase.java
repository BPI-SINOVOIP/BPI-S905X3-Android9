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
 * limitations under the License
 */

package android.server.am;

import static android.server.am.Components.KeyguardDismissLoggerCallback.ENTRY_ON_DISMISS_CANCELLED;
import static android.server.am.Components.KeyguardDismissLoggerCallback.ENTRY_ON_DISMISS_ERROR;
import static android.server.am.Components.KeyguardDismissLoggerCallback.ENTRY_ON_DISMISS_SUCCEEDED;
import static android.server.am.Components.KeyguardDismissLoggerCallback.KEYGUARD_DISMISS_LOG_TAG;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logAlways;

import static org.junit.Assert.fail;

import android.app.KeyguardManager;
import android.os.SystemClock;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

class KeyguardTestBase extends ActivityManagerTestBase {

    KeyguardManager mKeyguardManager;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mKeyguardManager = mContext.getSystemService(KeyguardManager.class);
    }

    void assertOnDismissSucceededInLogcat(LogSeparator logSeparator) {
        assertInLogcat(KEYGUARD_DISMISS_LOG_TAG, ENTRY_ON_DISMISS_SUCCEEDED, logSeparator);
    }

    void assertOnDismissCancelledInLogcat(LogSeparator logSeparator) {
        assertInLogcat(KEYGUARD_DISMISS_LOG_TAG, ENTRY_ON_DISMISS_CANCELLED, logSeparator);
    }

    void assertOnDismissErrorInLogcat(LogSeparator logSeparator) {
        assertInLogcat(KEYGUARD_DISMISS_LOG_TAG, ENTRY_ON_DISMISS_ERROR, logSeparator);
    }

    private void assertInLogcat(String logTag, String entry, LogSeparator logSeparator) {
        final Pattern pattern = Pattern.compile("(.+)" + entry);
        for (int retry = 1; retry <= 5; retry++) {
            final String[] lines = getDeviceLogsForComponents(logSeparator, logTag);
            for (int i = lines.length - 1; i >= 0; i--) {
                final String line = lines[i].trim();
                log(line);
                Matcher matcher = pattern.matcher(line);
                if (matcher.matches()) {
                    return;
                }
            }
            logAlways("Waiting for " + entry + "... retry=" + retry);
            SystemClock.sleep(500);
        }
        fail("Waiting for " + entry + " failed");
    }
}
