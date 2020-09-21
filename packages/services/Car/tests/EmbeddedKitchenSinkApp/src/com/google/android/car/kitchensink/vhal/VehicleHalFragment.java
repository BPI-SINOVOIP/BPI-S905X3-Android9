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
package com.google.android.car.kitchensink.vhal;

import android.annotation.Nullable;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.os.Bundle;
import android.os.RemoteException;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

public class VehicleHalFragment extends Fragment {
    private static final String TAG = "CAR.VEHICLEHAL.KS";

    private KitchenSinkActivity mActivity;
    private ListView mListView;

    private final OnClickListener mNopOnClickListener = new OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) { }
    };

    private final OnItemClickListener mOnClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            HalPropertyInfo entry = (HalPropertyInfo)parent.getItemAtPosition(position);
            AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
            builder.setTitle("Info for " + entry.name)
                   .setPositiveButton(android.R.string.yes, mNopOnClickListener)
                   .setMessage(entry.config.toString())
                   .show();
        }
    };

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.vhal, container, false);
        mActivity = (KitchenSinkActivity) getHost();
        mListView = view.findViewById(R.id.hal_prop_list);
        mListView.setOnItemClickListener(mOnClickListener);

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();

        final IVehicle vehicle;
        try {
            vehicle = Objects.requireNonNull(IVehicle.getService());
        } catch (NullPointerException | RemoteException e) {
            Log.e(TAG, "unable to retrieve Vehicle HAL service", e);
            return;
        }

        final List<VehiclePropConfig> propConfigList;
        try {
            propConfigList = Objects.requireNonNull(vehicle.getAllPropConfigs());
        } catch (NullPointerException | RemoteException e) {
            Log.e(TAG, "unable to retrieve prop configs", e);
            return;
        }

        final List<HalPropertyInfo> supportedProperties = propConfigList.stream()
            .map(HalPropertyInfo::new)
            .sorted()
            .collect(Collectors.toList());

        mListView.setAdapter(new ArrayAdapter<HalPropertyInfo>(mActivity,
                android.R.layout.simple_list_item_1,
                supportedProperties));
    }

    private static class HalPropertyInfo implements Comparable<HalPropertyInfo> {
        public final int id;
        public final String name;
        public final VehiclePropConfig config;

        HalPropertyInfo(VehiclePropConfig config) {
            this.config = config;
            id = config.prop;
            name = VehicleProperty.toString(id);
        }

        @Override
        public String toString() {
            return name;
        }

        @Override
        public boolean equals(Object other) {
            if (other instanceof HalPropertyInfo) {
                return ((HalPropertyInfo)other).id == id;
            }
            return false;
        }

        @Override
        public int hashCode() {
            return id;
        }

        @Override
        public int compareTo(HalPropertyInfo halPropertyInfo) {
            return name.compareTo(halPropertyInfo.name);
        }
    }
}
