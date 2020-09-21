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
package com.google.android.car.kitchensink.volume;

import android.content.Context;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.TextView;

import com.google.android.car.kitchensink.R;
import com.google.android.car.kitchensink.volume.VolumeTestFragment.VolumeInfo;


public class VolumeAdapter extends ArrayAdapter<VolumeInfo> {

    private final Context mContext;
    private VolumeInfo[] mVolumeList;
    private final int mLayoutResourceId;
    private VolumeTestFragment mFragment;


    public VolumeAdapter(Context c, int layoutResourceId, VolumeInfo[] volumeList,
            VolumeTestFragment fragment) {
        super(c, layoutResourceId, volumeList);
        mFragment = fragment;
        mContext = c;
        this.mLayoutResourceId = layoutResourceId;
        this.mVolumeList = volumeList;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder vh = new ViewHolder();
        if (convertView == null) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            convertView = inflater.inflate(mLayoutResourceId, parent, false);
            vh.id = convertView.findViewById(R.id.stream_id);
            vh.maxVolume = convertView.findViewById(R.id.volume_limit);
            vh.currentVolume = convertView.findViewById(R.id.current_volume);
            vh.upButton = convertView.findViewById(R.id.volume_up);
            vh.downButton = convertView.findViewById(R.id.volume_down);
            vh.requestButton = convertView.findViewById(R.id.request);
            convertView.setTag(vh);
        } else {
            vh = (ViewHolder) convertView.getTag();
        }
        if (mVolumeList[position] != null) {
            vh.id.setText(mVolumeList[position].mId);
            vh.maxVolume.setText(String.valueOf(mVolumeList[position].mMax));
            vh.currentVolume.setText(String.valueOf(mVolumeList[position].mCurrent));
            int color = mVolumeList[position].mHasFocus ? Color.GREEN : Color.GRAY;
            vh.requestButton.setBackgroundColor(color);
            if (position == 0) {
                vh.upButton.setVisibility(View.INVISIBLE);
                vh.downButton.setVisibility(View.INVISIBLE);
                vh.requestButton.setVisibility(View.INVISIBLE);
            } else {
                vh.upButton.setVisibility(View.VISIBLE);
                vh.downButton.setVisibility(View.VISIBLE);
                vh.requestButton.setVisibility(View.VISIBLE);
                vh.upButton.setOnClickListener((view) -> {
                    mFragment.adjustVolumeByOne(mVolumeList[position].mGroupId, true);
                });
                vh.downButton.setOnClickListener((view) -> {
                    mFragment.adjustVolumeByOne(mVolumeList[position].mGroupId, false);
                });

                vh.requestButton.setOnClickListener((view) -> {
                    mFragment.requestFocus(mVolumeList[position].mGroupId);
                });
            }
        }
        return convertView;
    }

    @Override
    public int getCount() {
        return mVolumeList.length;
    }


    public void refreshVolumes(VolumeInfo[] volumes) {
        mVolumeList = volumes;
        notifyDataSetChanged();
    }

    static class ViewHolder {
        TextView id;
        TextView maxVolume;
        TextView currentVolume;
        Button upButton;
        Button downButton;
        Button requestButton;
    }
}
