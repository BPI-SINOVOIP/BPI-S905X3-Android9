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

import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.property.CarPropertyManager;
import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.google.android.car.kitchensink.R;

import java.util.List;

class PropertyListAdapter extends BaseAdapter implements ListAdapter {
    private static final int DEFAULT_RATE = 1;
    private static final String TAG = "PropertyListAdapter";
    private final Context mContext;
    private final PropertyListEventListener mListener;
    private final CarPropertyManager mMgr;
    private final List<PropertyInfo> mPropInfo;
    private final TextView mTvEventLog;

    PropertyListAdapter(List<PropertyInfo> propInfo, CarPropertyManager mgr, TextView eventLog,
                        ScrollView svEventLog, Context context) {
        mContext = context;
        mListener = new PropertyListEventListener(eventLog, svEventLog);
        mMgr = mgr;
        mPropInfo = propInfo;
        mTvEventLog = eventLog;
    }

    @Override
    public int getCount() {
        return mPropInfo.size();
    }

    @Override
    public Object getItem(int pos) {
        return mPropInfo.get(pos);
    }

    @Override
    public long getItemId(int pos) {
        return mPropInfo.get(pos).mPropId;
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            view = inflater.inflate(R.layout.property_list_item, null);
        }

        //Handle TextView and display string from your list
        TextView listItemText = (TextView) view.findViewById(R.id.tvPropertyName);
        listItemText.setText(mPropInfo.get(position).toString());

        //Handle buttons and add onClickListeners
        ToggleButton btn = (ToggleButton) view.findViewById(R.id.tbRegisterPropertyBtn);

        btn.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                CarPropertyConfig c = mPropInfo.get(position).mConfig;
                int propId = c.getPropertyId();
                try {
                    if (btn.isChecked()) {
                        mMgr.registerListener(mListener, propId, DEFAULT_RATE);
                    } else {
                        mMgr.unregisterListener(mListener, propId);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Unhandled exception: ", e);
                }
            }
        });
        return view;
    }


    private static class PropertyListEventListener implements
            CarPropertyManager.CarPropertyEventListener {
        private int mNumEvents;
        private ScrollView mScrollView;
        private TextView mTvLogEvent;

        PropertyListEventListener(TextView logEvent, ScrollView scrollView) {
            mScrollView = scrollView;
            mTvLogEvent = logEvent;
        }

        @Override
        public void onChangeEvent(CarPropertyValue value) {
            mNumEvents++;
            mTvLogEvent.append("Event " + mNumEvents + ":"
                               + " time=" + value.getTimestamp()
                               + " propId=0x" + toHexString(value.getPropertyId())
                               + " areaId=0x" + toHexString(value.getAreaId())
                               + " status=" + value.getStatus()
                               + " value=" + value.getValue()
                               + "\n");
            scrollToBottom();
        }

        @Override
        public void onErrorEvent(int propId, int areaId) {
            mTvLogEvent.append("Received error event propId=0x" + toHexString(propId)
                               + ", areaId=0x" + toHexString(areaId));
            scrollToBottom();
        }

        private void scrollToBottom() {
            mScrollView.post(new Runnable() {
                public void run() {
                    mScrollView.fullScroll(View.FOCUS_DOWN);
                    //mScrollView.smoothScrollTo(0, mTextStatus.getBottom());
                }
            });
        }

    }
}
