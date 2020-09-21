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
import com.droidlogic.app.tv.TvControlManager;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;

public class ChannelAudioFragment extends SideFragment {
    private static final String TAG = "ChannelAudioFragment";
    private ChannelSettingsManager mChannelSettingsManager;

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_audio);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        ArrayList<HashMap<String, String>> optionListData = new ArrayList<HashMap<String, String>>();
        optionListData = mChannelSettingsManager.getChannelAudioList();
        if (optionListData != null) {
            for (int i = 0; i < optionListData.size(); i++) {
                String info = optionListData.get(i).get(ChannelSettingsManager.STRING_NAME);
                ChannelAudioItem item = new ChannelAudioItem(info, i);
                if (mChannelSettingsManager.getChannelAudioStatus() == i) {
                    item.setChecked(true);
                }
                items.add(item);
            }
        }

        return items;
    }

    private class ChannelAudioItem extends RadioButtonItem {
        private final int mTrackId;

        private ChannelAudioItem(String title, int trackId) {
            super(title);
            mTrackId = trackId;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            int selectedItem = 0;
            if (mChannelSettingsManager == null) {
                mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
            }

            if (mTrackId == 0) {
                selectedItem = TvControlManager.ATV_AUDIO_STD_DK;
            } else if (mTrackId == 1) {
                selectedItem = TvControlManager.ATV_AUDIO_STD_I;
            } else if (mTrackId == 2) {
                selectedItem = TvControlManager.ATV_AUDIO_STD_BG;
            } else if (mTrackId == 3) {
                selectedItem = TvControlManager.ATV_AUDIO_STD_M;
            } else if (mTrackId == 4) {
                selectedItem = TvControlManager.ATV_AUDIO_STD_L;
            } else {
                selectedItem = TvControlManager.ATV_AUDIO_STD_DK;
            }
            mChannelSettingsManager.setChannelAudio(selectedItem);
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            //getMainActivity().selectSubtitleTrack(mOption, mTrackId);
        }
    }
}