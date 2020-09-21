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

package com.android.settings.connecteddevice.usb;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.Utils;
import com.android.settings.widget.RadioButtonPickerFragment;
import com.android.settingslib.widget.CandidateInfo;
import com.android.settingslib.widget.FooterPreference;
import com.android.settingslib.widget.FooterPreferenceMixin;

import com.google.android.collect.Lists;

import java.util.List;

/**
 * Provides options for selecting the default USB mode.
 */
public class UsbDefaultFragment extends RadioButtonPickerFragment {
    @VisibleForTesting
    UsbBackend mUsbBackend;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mUsbBackend = new UsbBackend(context);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);
        FooterPreferenceMixin footer = new FooterPreferenceMixin(this, this.getLifecycle());
        FooterPreference pref = footer.createFooterPreference();
        pref.setTitle(R.string.usb_default_info);
    }

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.USB_DEFAULT;
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.usb_default_fragment;
    }

    @Override
    protected List<? extends CandidateInfo> getCandidates() {
        List<CandidateInfo> ret = Lists.newArrayList();
        for (final long option : UsbDetailsFunctionsController.FUNCTIONS_MAP.keySet()) {
            final String title = getContext().getString(
                    UsbDetailsFunctionsController.FUNCTIONS_MAP.get(option));
            final String key = UsbBackend.usbFunctionsToString(option);

            // Only show supported functions
            if (mUsbBackend.areFunctionsSupported(option)) {
                ret.add(new CandidateInfo(true /* enabled */) {
                    @Override
                    public CharSequence loadLabel() {
                        return title;
                    }

                    @Override
                    public Drawable loadIcon() {
                        return null;
                    }

                    @Override
                    public String getKey() {
                        return key;
                    }
                });
            }
        }
        return ret;
    }

    @Override
    protected String getDefaultKey() {
        return UsbBackend.usbFunctionsToString(mUsbBackend.getDefaultUsbFunctions());
    }

    @Override
    protected boolean setDefaultKey(String key) {
        long functions = UsbBackend.usbFunctionsFromString(key);
        if (!Utils.isMonkeyRunning()) {
            mUsbBackend.setDefaultUsbFunctions(functions);
        }
        return true;
    }
}