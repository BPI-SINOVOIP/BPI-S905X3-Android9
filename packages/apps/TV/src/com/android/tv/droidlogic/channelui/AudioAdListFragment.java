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
import com.android.tv.ui.sidepanel.ActionItem;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;

public class AudioAdListFragment extends SideFragment {
    private static final String TAG = "AudioAdListFragment";

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_audio_ad_list);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        ChannelSettingsManager manager = new ChannelSettingsManager(getMainActivity());
        ArrayList<HashMap<String, String>> optionListData = new ArrayList<HashMap<String, String>>();
        optionListData = manager.getAudioADList();
        if (optionListData != null) {
            for (int i =0; i < optionListData.size(); i++) {
                String info = optionListData.get(i).get(ChannelSettingsManager.STRING_NAME) + ": "
                    + optionListData.get(i).get(ChannelSettingsManager.STRING_PRIVATE);
                items.add(new ActionItem(info) {
                    @Override
                    protected void onSelected() {

                    }
                });
            }
        }

        return items;
    }
}