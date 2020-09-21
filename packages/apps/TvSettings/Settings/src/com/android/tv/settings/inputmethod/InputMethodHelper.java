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
 * limitations under the License.
 */
package com.android.tv.settings.inputmethod;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.os.UserHandle;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import java.util.ArrayList;
import java.util.List;

/**
 * Helper class for InputMethod
 */
public class InputMethodHelper {

    public static final String TAG = "InputMethodHelper";

    /**
     * Get list of enabled InputMethod
     */
    public static List<InputMethodInfo> getEnabledSystemInputMethodList(Context context) {
        InputMethodManager inputMethodManager =
                (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        List<InputMethodInfo> enabledInputMethodInfos =
                new ArrayList<>(inputMethodManager.getEnabledInputMethodList());
        // Filter auxiliary keyboards out
        enabledInputMethodInfos.removeIf(InputMethodInfo::isAuxiliaryIme);
        return enabledInputMethodInfos;
    }

    /**
     * Get id of default InputMethod
     */
    public static String getDefaultInputMethodId(Context context) {
        return Settings.Secure.getString(context.getContentResolver(),
                Settings.Secure.DEFAULT_INPUT_METHOD);
    }

    /**
     * Set default InputMethod by id
     */
    public static void setDefaultInputMethodId(Context context, String imid) {
        if (imid == null) {
            throw new IllegalArgumentException("Null ID");
        }

        try {
            int userId = ActivityManager.getService().getCurrentUser().id;
            Settings.Secure.putStringForUser(context.getContentResolver(),
                    Settings.Secure.DEFAULT_INPUT_METHOD, imid, userId);

            Intent intent = new Intent(Intent.ACTION_INPUT_METHOD_CHANGED);
            intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
            intent.putExtra("input_method_id", imid);
            context.sendBroadcastAsUser(intent, UserHandle.CURRENT);
        } catch (RemoteException e) {
            Log.d(TAG, "set default input method remote exception", e);
        }
    }

    /**
     * Find InputMethod from a List by id.
     */
    public static InputMethodInfo findInputMethod(String imid,
            List<InputMethodInfo> enabledInputMethodInfos) {
        for (int i = 0, size = enabledInputMethodInfos.size(); i < size; i++) {
            final InputMethodInfo info = enabledInputMethodInfos.get(i);
            final String id = info.getId();
            if (TextUtils.equals(id, imid)) {
                return info;
            }
        }
        return null;
    }

    /**
     * Get settings Intent of an InputMethod.
     */
    public static Intent getInputMethodSettingsIntent(InputMethodInfo imi) {
        final Intent intent;
        final String settingsActivity = imi.getSettingsActivity();
        if (!TextUtils.isEmpty(settingsActivity)) {
            intent = new Intent(Intent.ACTION_MAIN);
            intent.setClassName(imi.getPackageName(), settingsActivity);
        } else {
            intent = null;
        }
        return intent;
    }
}
