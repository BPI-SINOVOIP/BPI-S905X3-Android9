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
import android.os.Bundle;
import android.provider.SearchIndexableResource;
import android.support.annotation.VisibleForTesting;

import com.android.internal.logging.nano.MetricsProto;
import com.android.settings.R;
import com.android.settings.dashboard.DashboardFragment;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;
import com.android.settingslib.core.AbstractPreferenceController;

import com.google.android.collect.Lists;

import java.util.ArrayList;
import java.util.List;

/**
 * Controls the USB device details and provides updates to individual controllers.
 */
public class UsbDetailsFragment extends DashboardFragment {
    private static final String TAG = UsbDetailsFragment.class.getSimpleName();

    private List<UsbDetailsController> mControllers;
    private UsbBackend mUsbBackend;

    @VisibleForTesting
    UsbConnectionBroadcastReceiver mUsbReceiver;

    private UsbConnectionBroadcastReceiver.UsbConnectionListener mUsbConnectionListener =
            (connected, functions, powerRole, dataRole) -> {
                for (UsbDetailsController controller : mControllers) {
                    controller.refresh(connected, functions, powerRole, dataRole);
                }
            };

    @Override
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.USB_DEVICE_DETAILS;
    }

    @Override
    protected String getLogTag() {
        return TAG;
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.usb_details_fragment;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);
    }

    public boolean isConnected() {
        return mUsbReceiver.isConnected();
    }

    @Override
    protected List<AbstractPreferenceController> createPreferenceControllers(Context context) {
        mUsbBackend = new UsbBackend(context);
        mControllers = createControllerList(context, mUsbBackend, this);
        mUsbReceiver = new UsbConnectionBroadcastReceiver(context, mUsbConnectionListener,
                mUsbBackend);
        this.getLifecycle().addObserver(mUsbReceiver);

        return new ArrayList<>(mControllers);
    }

    private static List<UsbDetailsController> createControllerList(Context context,
            UsbBackend usbBackend, UsbDetailsFragment fragment) {
        List<UsbDetailsController> ret = new ArrayList<>();
        ret.add(new UsbDetailsHeaderController(context, fragment, usbBackend));
        ret.add(new UsbDetailsDataRoleController(context, fragment, usbBackend));
        ret.add(new UsbDetailsFunctionsController(context, fragment, usbBackend));
        ret.add(new UsbDetailsPowerRoleController(context, fragment, usbBackend));
        return ret;
    }

    /**
     * For Search.
     */
    public static final Indexable.SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(
                        Context context, boolean enabled) {
                    SearchIndexableResource res = new SearchIndexableResource(context);
                    res.xmlResId = R.xml.usb_details_fragment;
                    return Lists.newArrayList(res);
                }

                @Override
                public List<String> getNonIndexableKeys(Context context) {
                    return super.getNonIndexableKeys(context);
                }

                @Override
                public List<AbstractPreferenceController> createPreferenceControllers(
                        Context context) {
                    return new ArrayList<>(
                            createControllerList(context, new UsbBackend(context), null));
                }
            };
}
