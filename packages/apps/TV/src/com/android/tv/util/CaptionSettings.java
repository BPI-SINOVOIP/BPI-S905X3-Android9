/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.util;

import android.content.Context;
import android.view.accessibility.CaptioningManager;
import android.provider.Settings;

import java.util.Locale;

import com.droidlogic.app.tv.DroidLogicTvUtils;

public class CaptionSettings {
    public static final int OPTION_SYSTEM = 0;
    public static final int OPTION_OFF = 1;
    public static final int OPTION_ON = 2;

    private final CaptioningManager mCaptioningManager;
    private int mOption = OPTION_SYSTEM;
    private String mLanguage;
    private String mTrackId;
    private Context mContext;

    public CaptionSettings(Context context) {
        mContext = context;
        mCaptioningManager =
                (CaptioningManager) context.getSystemService(Context.CAPTIONING_SERVICE);
    }

    public final String getSystemLanguage() {
        Locale l = mCaptioningManager.getLocale();
        if (l != null) {
            return l.getLanguage();
        }
        return null;
    }

    public final String getLanguage() {
        switch (mOption) {
            case OPTION_SYSTEM:
                return getSystemLanguage();
            case OPTION_OFF:
                return null;
            case OPTION_ON:
                return mLanguage;
        }
        return null;
    }

    public final boolean isSystemSettingEnabled() {
        return mCaptioningManager.isEnabled();
    }

    public final boolean isEnabled() {
        switch (mOption) {
            case OPTION_SYSTEM:
                return isSystemSettingEnabled();
            case OPTION_OFF:
                return false;
            case OPTION_ON:
                return true;
        }
        return false;
    }

    public int getEnableOption() {
        return mOption;
    }

    public void setEnableOption(int option) {
        mOption = option;
    }

    public void setLanguage(String language) {
        mLanguage = language;
    }

    /** Returns the track ID to be used as an alternative key. */
    public String getTrackId() {
        return mTrackId;
    }

    /** Sets the track ID to be used as an alternative key. */
    public void setTrackId(String trackId) {
        mTrackId = trackId;
    }

    public void setCaptionsEnabled(boolean enabled) {
        Settings.Secure.putInt(mContext.getContentResolver(),
                "accessibility_captioning_enabled"/*Settings.Secure.ACCESSIBILITY_CAPTIONING_ENABLED*/, enabled ? 1 : 0);
        //can set by write data base
    }

    public boolean isCaptionsStyleEnabled() {
        return Settings.Secure.getInt(mContext.getContentResolver(),
            DroidLogicTvUtils.SYSTEM_CAPTION_STYLE_ENABLE/*"accessibility_captioning_style_enabled"*/, 0) != 0;
    }

    public void setCaptionsStyleEnabled(boolean enabled) {
        Settings.Secure.putInt(mContext.getContentResolver(),
                 DroidLogicTvUtils.SYSTEM_CAPTION_STYLE_ENABLE/*"accessibility_captioning_style_enabled"*/, enabled ? 1 : 0);
    }
}
