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

import android.media.tv.TvContract.Channels;
import android.media.tv.TvContract;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;

public class DtvTypeFragment extends SideFragment {
    private static final String TAG = "DtvTypeFragment";
    private ChannelSettingsManager mChannelSettingsManager;
    private static final int[] TYPE = {R.string.channel_dtv_type_dtmb,
        R.string.channel_dtv_type_dvbc, R.string.channel_dtv_type_dvbt,
        R.string.channel_dtv_type_dvbt2,R.string.channel_dtv_type_atsc_t,
        R.string.channel_dtv_type_atsc_c, R.string.channel_dtv_type_isdb_t};
    private static final String[] SET_TYPE = {TvContract.Channels.TYPE_DTMB, TvContract.Channels.TYPE_DVB_C,
        TvContract.Channels.TYPE_DVB_T, TvContract.Channels.TYPE_DVB_T2,
        TvContract.Channels.TYPE_ATSC_T, TvContract.Channels.TYPE_ATSC_C,
        TvContract.Channels.TYPE_ISDB_T};

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_dtv_type);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        String currenttype = mChannelSettingsManager.getDtvType();
        if (TYPE != null) {
            for (int i = 0; i < TYPE.length; i++) {
                singlelItem item = new singlelItem(getMainActivity().getResources().getString(TYPE[i]), i);
                if (SET_TYPE[i].equals(currenttype)) {
                    item.setChecked(true);
                }
                items.add(item);
            }
        }

        return items;
    }

    private class singlelItem extends RadioButtonItem {
        private final int mTrackId;

        private singlelItem(String title, int id) {
            super(title);
            mTrackId = id;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            if (mChannelSettingsManager == null) {
                mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
            }
            mChannelSettingsManager.setDtvType(SET_TYPE[mTrackId]);//getMainActivity().getResources().getString(TYPE[mTrackId]));
        }

        @Override
        protected void onFocused() {
            super.onFocused();

        }
    }
}