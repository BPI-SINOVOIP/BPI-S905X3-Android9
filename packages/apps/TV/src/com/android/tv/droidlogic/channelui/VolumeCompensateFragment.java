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

package com.android.tv.droidlogic.channelui;

import android.util.Log;
import android.widget.SeekBar;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;

public class VolumeCompensateFragment extends SideFragment {
    private static final String TAG = "VolumeCompensateFragment";
    private static final boolean DEBUG = false;
    private ChannelSettingsManager mChannelSettingsManager;

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_volume_compensate);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        items.add(new SingleSeekBarItem(TAG, mChannelSettingsManager.getVolumeCompensateStatus()));
        return items;
    }

    private class SingleSeekBarItem extends SeekBarItem {
        private final int mProgress;

        private SingleSeekBarItem(String title, int progress) {
            super(title, progress);//String.valueOf(progress));
            mProgress = progress;
        }

        @Override
        protected void onProgressChanged(SeekBar seekBar, int progress) {
            super.onProgressChanged(seekBar, progress - 20);
            mChannelSettingsManager.setVolumeCompensate(progress - 20);
            if (DEBUG) Log.d(TAG, "onProgressChanged progress = " + (progress - 20) * 5);
        }

        @Override
        protected void onFocused() {
            //super.onFocused();
        }
    }
}