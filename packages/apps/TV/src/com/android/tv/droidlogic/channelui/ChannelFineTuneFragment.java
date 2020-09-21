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

public class ChannelFineTuneFragment extends SideFragment {
    private static final String TAG = "ChannelFineTuneFragment";
    private static final boolean DEBUG = true;
    private ChannelSettingsManager mChannelSettingsManager;

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_fine_tune);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    public void onResume() {
        super.onResume();
        setItems(refresh());
    }

    @Override
    protected List<Item> getItemList() {
        return refresh();
    }

    private List<Item> refresh() {
        List<Item> items = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        items.add(new SingleSeekBarItem(TAG, mChannelSettingsManager.getFineTuneProgress()));
        return items;
    }

    private String parseFrequencyBand(double freq) {
        String band = "";
        if (freq > 44.25 && freq < 143.25) {
            band = "VHFL";
        } else if (freq >= 143.25 && freq < 426.25) {
            band = "VHFH";
        } else if (freq >= 426.25 && freq < 868.25) {
            band = "UHF";
        }
        return band;
    }

    private String getFineTuneFrequencyInfo() {
        String info = null;
        int freq = mChannelSettingsManager.getFrequency();
        if (freq == -1) {
            return null;
        }
        double f = freq + mChannelSettingsManager.getFineTune();
        f /= 1000 * 1000;
        info = Double.toString(f) + getString(R.string.channel_fine_mhz) + " " + parseFrequencyBand(f);
        return info;
    }

    private final static int MAX= 100;
    private final static int OFFSET = -50;
    private final static int SHOWMAX = 200;

    private class SingleSeekBarItem extends SingleStepSeekBarItem {
        private int mProgress;

        private SingleSeekBarItem(String title, int progress) {
            super(title, progress, getFineTuneFrequencyInfo(), MAX, SHOWMAX, OFFSET);
            mProgress = progress;
        }

        @Override
        protected void onProgressChanged(SeekBar seekBar, int progress) {
            super.onProgressChanged(seekBar, progress);
            setDescription(getFineTuneFrequencyInfo());
            mChannelSettingsManager.setFineTuneStep(progress - mChannelSettingsManager.getFineTuneProgress());
            if (DEBUG) Log.d(TAG, "onProgressChanged progress = " + progress + ", showprogress = " + (progress + OFFSET) * SHOWMAX / MAX);
        }

        @Override
        protected void onFocused() {
            //super.onFocused();
        }
    }
}