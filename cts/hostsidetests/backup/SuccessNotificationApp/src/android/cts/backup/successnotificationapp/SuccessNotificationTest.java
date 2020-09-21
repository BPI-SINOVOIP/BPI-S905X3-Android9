/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.cts.backup.successnotificationapp;

import static android.support.test.InstrumentationRegistry.getTargetContext;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SuccessNotificationTest {
    protected static final String PREFS_FILE = "android.cts.backup.successnotificationapp.PREFS";
    private static final String KEY_VALUE_RESTORE_APP_PACKAGE =
            "android.cts.backup.keyvaluerestoreapp";
    private static final String FULLBACKUPONLY_APP_PACKAGE = "android.cts.backup.fullbackuponlyapp";

    @Test
    public void clearBackupSuccessNotificationsReceived() {
        getTargetContext().getSharedPreferences(PREFS_FILE, Context.MODE_PRIVATE).edit().clear()
                .commit();
    }

    @Test
    public void verifyBackupSuccessNotificationReceivedForKeyValueApp() {
        assertTrue(getTargetContext().getSharedPreferences(PREFS_FILE, Context.MODE_PRIVATE)
                .getBoolean(KEY_VALUE_RESTORE_APP_PACKAGE, false));
    }

    @Test
    public void verifyBackupSuccessNotificationReceivedForFullBackupApp() {
        assertTrue(getTargetContext().getSharedPreferences(PREFS_FILE, Context.MODE_PRIVATE)
                .getBoolean(FULLBACKUPONLY_APP_PACKAGE, false));
    }
}
