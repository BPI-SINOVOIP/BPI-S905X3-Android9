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

public class ChannelSwitchFragment extends SideFragment {
    private static final String TAG = "ChannelSwitchFragment";
    private ChannelSettingsManager mChannelSettingsManager;
    private static final int[] ITEM_TYPE = {R.string.channel_switch_channel_static, R.string.channel_switch_channel_black};

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_switch_channel);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        int status = mChannelSettingsManager.getSwitchChannelStatus();
        if (ITEM_TYPE != null) {
            for (int i = 0; i < ITEM_TYPE.length; i++) {
                String info = getString(ITEM_TYPE[i]);
                ChannelSwitchItem item = new ChannelSwitchItem(info, i);
                if (mChannelSettingsManager.getSwitchChannelStatus() == i) {
                    item.setChecked(true);
                }
                items.add(item);
            }
        }

        return items;
    }

    private class ChannelSwitchItem extends RadioButtonItem {
        private final int mTrackId;

        private ChannelSwitchItem(String title, int trackId) {
            super(title);
            mTrackId = trackId;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            if (mChannelSettingsManager == null) {
                mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
            }
            mChannelSettingsManager.setBlackoutEnable(mTrackId, 1);
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            //getMainActivity().selectSubtitleTrack(mOption, mTrackId);
        }
    }
}