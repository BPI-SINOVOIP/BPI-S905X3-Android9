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
package com.android.settings.bluetooth;

import android.content.Context;
import android.view.View;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.logging.nano.MetricsProto;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.core.SubSettingLauncher;
import com.android.settings.location.ScanningSettings;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.utils.AnnotationSpan;
import com.android.settings.widget.SwitchWidgetController;
import com.android.settingslib.bluetooth.LocalBluetoothAdapter;
import com.android.settingslib.bluetooth.LocalBluetoothManager;
import com.android.settingslib.core.lifecycle.LifecycleObserver;
import com.android.settingslib.core.lifecycle.events.OnStart;
import com.android.settingslib.core.lifecycle.events.OnStop;
import com.android.settingslib.widget.FooterPreference;

/**
 * PreferenceController to update of bluetooth state. All behavior except managing the footer text
 * is delegated to the SwitchWidgetController it uses.
 */
public class BluetoothSwitchPreferenceController
        implements LifecycleObserver, OnStart, OnStop,
        SwitchWidgetController.OnSwitchChangeListener, View.OnClickListener {

    @VisibleForTesting
    LocalBluetoothAdapter mBluetoothAdapter;

    private LocalBluetoothManager mBluetoothManager;
    private BluetoothEnabler mBluetoothEnabler;
    private RestrictionUtils mRestrictionUtils;
    private SwitchWidgetController mSwitch;
    private Context mContext;
    private FooterPreference mFooterPreference;

    public BluetoothSwitchPreferenceController(Context context,
            SwitchWidgetController switchController,
            FooterPreference footerPreference) {
        this(context, Utils.getLocalBtManager(context), new RestrictionUtils(), switchController,
                footerPreference);
    }

    @VisibleForTesting
    public BluetoothSwitchPreferenceController(Context context,
            LocalBluetoothManager bluetoothManager, RestrictionUtils restrictionUtils,
            SwitchWidgetController switchController, FooterPreference footerPreference) {
        mBluetoothManager = bluetoothManager;
        mRestrictionUtils = restrictionUtils;
        mSwitch = switchController;
        mContext = context;
        mFooterPreference = footerPreference;

        mSwitch.setupView();
        updateText(mSwitch.isChecked());

        if (mBluetoothManager != null) {
            mBluetoothAdapter = mBluetoothManager.getBluetoothAdapter();
        }
        mBluetoothEnabler = new BluetoothEnabler(context,
                switchController,
                FeatureFactory.getFactory(context).getMetricsFeatureProvider(), mBluetoothManager,
                MetricsEvent.ACTION_SETTINGS_MASTER_SWITCH_BLUETOOTH_TOGGLE,
                mRestrictionUtils);
        mBluetoothEnabler.setToggleCallback(this);
    }

    @Override
    public void onStart() {
        mBluetoothEnabler.resume(mContext);
        if (mSwitch != null) {
            updateText(mSwitch.isChecked());
        }
    }

    @Override
    public void onStop() {
        mBluetoothEnabler.pause();
    }

    @Override
    public boolean onSwitchToggled(boolean isChecked) {
        updateText(isChecked);
        return true;
    }

    @Override
    public void onClick(View v) {
        // send users to scanning settings if they click on the link in the summary text
        new SubSettingLauncher(mContext)
                .setDestination(ScanningSettings.class.getName())
                .setSourceMetricsCategory(MetricsProto.MetricsEvent.BLUETOOTH_FRAGMENT)
                .launch();
    }

    @VisibleForTesting void updateText(boolean isChecked) {
        if (!isChecked
                && Utils.isBluetoothScanningEnabled(mContext)) {
            AnnotationSpan.LinkInfo info = new AnnotationSpan.LinkInfo(
                    AnnotationSpan.LinkInfo.DEFAULT_ANNOTATION, this);
            CharSequence text = AnnotationSpan.linkify(
                    mContext.getText(R.string.bluetooth_scanning_on_info_message), info);
            mFooterPreference.setTitle(text);
        } else {
            mFooterPreference.setTitle(R.string.bluetooth_empty_list_bluetooth_off);
        }
    }
}