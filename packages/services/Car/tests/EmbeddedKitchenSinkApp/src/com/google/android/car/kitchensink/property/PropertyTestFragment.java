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

package com.google.android.car.kitchensink.property;

import static java.lang.Integer.toHexString;

import android.annotation.Nullable;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyManager;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyType;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

import java.util.LinkedList;
import java.util.List;
import java.util.stream.Collectors;

public class PropertyTestFragment extends Fragment implements OnItemSelectedListener{
    private static final String TAG = "PropertyTestFragment";

    private KitchenSinkActivity mActivity;
    private CarPropertyManager mMgr;
    private List<PropertyInfo> mPropInfo = null;
    private Spinner mAreaId;
    private TextView mEventLog;
    private TextView mGetValue;
    private ListView mListView;
    private Spinner mPropertyId;
    private ScrollView mScrollView;
    private EditText mSetValue;

    private final OnClickListener mNopOnClickListener = new OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) { }
    };

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater,
            @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.property, container, false);
        mActivity = (KitchenSinkActivity) getHost();
        mMgr = mActivity.getPropertyManager();

        // Get resource IDs
        mAreaId = view.findViewById(R.id.sAreaId);
        mEventLog = view.findViewById(R.id.tvEventLog);
        mGetValue = view.findViewById(R.id.tvGetPropertyValue);
        mListView = view.findViewById(R.id.lvPropertyList);
        mPropertyId = view.findViewById(R.id.sPropertyId);
        mScrollView = view.findViewById(R.id.svEventLog);
        mSetValue = view.findViewById(R.id.etSetPropertyValue);

        populateConfigList();
        mListView.setAdapter(new PropertyListAdapter(mPropInfo, mMgr, mEventLog, mScrollView,
                                                     mActivity));

        // Configure dropdown menu for propertyId spinner
        ArrayAdapter<PropertyInfo> adapter =
                new ArrayAdapter<PropertyInfo>(mActivity, android.R.layout.simple_spinner_item,
                                               mPropInfo);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPropertyId.setAdapter(adapter);
        mPropertyId.setOnItemSelectedListener(this);



        // Configure listeners for buttons
        Button b = view.findViewById(R.id.bGetProperty);
        b.setOnClickListener(v -> {
            try {
                PropertyInfo info = (PropertyInfo) mPropertyId.getSelectedItem();
                int propId = info.mConfig.getPropertyId();
                int areaId = Integer.decode(mAreaId.getSelectedItem().toString());
                CarPropertyValue value = mMgr.getProperty(propId, areaId);
                if (propId == VehicleProperty.WHEEL_TICK) {
                    Object[] ticks = (Object[]) value.getValue();
                    mGetValue.setText("Timestamp=" + value.getTimestamp()
                                      + "\nstatus=" + value.getStatus()
                                      + "\n[0]=" + (Long) ticks[0]
                                      + "\n[1]=" + (Long) ticks[1] + " [2]=" + (Long) ticks[2]
                                      + "\n[3]=" + (Long) ticks[3] + " [4]=" + (Long) ticks[4]);
                } else {
                    mGetValue.setText("Timestamp=" + value.getTimestamp()
                                      + "\nstatus=" + value.getStatus()
                                      + "\nvalue=" + value.getValue());
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to get property", e);
            }
        });

        b = view.findViewById(R.id.bSetProperty);
        b.setOnClickListener(v -> {
            try {
                PropertyInfo info = (PropertyInfo) mPropertyId.getSelectedItem();
                int propId = info.mConfig.getPropertyId();
                int areaId = Integer.decode(mAreaId.getSelectedItem().toString());
                String valueString = mSetValue.getText().toString();

                switch (propId & VehiclePropertyType.MASK) {
                    case VehiclePropertyType.BOOLEAN:
                        Boolean boolVal = Boolean.parseBoolean(valueString);
                        mMgr.setBooleanProperty(propId, areaId, boolVal);
                        break;
                    case VehiclePropertyType.FLOAT:
                        Float floatVal = Float.parseFloat(valueString);
                        mMgr.setFloatProperty(propId, areaId, floatVal);
                        break;
                    case VehiclePropertyType.INT32:
                        Integer intVal = Integer.parseInt(valueString);
                        mMgr.setIntProperty(propId, areaId, intVal);
                        break;
                    default:
                        Toast.makeText(mActivity, "PropertyType=0x" + toHexString(propId
                                & VehiclePropertyType.MASK) + " is not handled!",
                                Toast.LENGTH_LONG).show();
                        break;
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to set HVAC boolean property", e);
            }
        });

        b = view.findViewById(R.id.bClearLog);
        b.setOnClickListener(v -> {
            mEventLog.setText("");
        });

        return view;
    }

    private void populateConfigList() {
        try {
            mPropInfo = mMgr.getPropertyList()
                    .stream()
                    .map(PropertyInfo::new)
                    .sorted()
                    .collect(Collectors.toList());
        } catch (Exception e) {
            Log.e(TAG, "Unhandled exception in populateConfigList: ", e);
        }
    }

    // Spinner callbacks
    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        // An item was selected. You can retrieve the selected item using
        PropertyInfo info = (PropertyInfo) parent.getItemAtPosition(pos);
        int[] areaIds = info.mConfig.getAreaIds();
        List<String> areaString = new LinkedList<String>();
        if (areaIds.length == 0) {
            areaString.add("0x0");
        } else {
            for (int areaId : areaIds) {
                areaString.add("0x" + toHexString(areaId));
            }
        }

        // Configure dropdown menu for propertyId spinner
        ArrayAdapter<String> adapter =
                new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_item,
                                         areaString);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mAreaId.setAdapter(adapter);
    }

    public void onNothingSelected(AdapterView<?> parent) {
        // Another interface callback
    }
}
