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

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;

public class AudioAdSwitchFragment extends SideFragment {
    private static final String TAG = "AudioAdSwitchFragment";
    private ChannelSettingsManager mChannelSettingsManager;
    private final static int[] STATUS = {R.string.channel_audio_ad_switch_off, R.string.channel_audio_ad_switch_on};

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_audio_ad_switch);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        for (int i = 0; i < STATUS.length; i++) {
            String info = getString(STATUS[i]);
            SoundChannelItem item = new SoundChannelItem(info, i);
            if (mChannelSettingsManager.getADSwitchStatus() == i) {
                item.setChecked(true);
            }
            items.add(item);
        }

        return items;
    }

    private class SoundChannelItem extends RadioButtonItem {
        private final int mTrackId;

        private SoundChannelItem(String title, int trackId) {
            super(title);
            mTrackId = trackId;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            if (mChannelSettingsManager == null) {
                mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
            }
            mChannelSettingsManager.setAudioADSwitch(mTrackId);
        }

        @Override
        protected void onFocused() {
            super.onFocused();
        }
    }
}