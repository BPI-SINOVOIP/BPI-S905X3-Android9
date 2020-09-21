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

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.ActionItem;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;

public class ChannelEditNameFragment extends SideFragment {
    private static final String TAG = "ChannelEditNameFragment";
    private static final boolean mDebug = false;
    private String mName;
    private long mChannelId;
    private ChannelSettingsManager mChannelSettingsManager;

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_edit_edit);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        ChannelSettingsManager manager = new ChannelSettingsManager(getMainActivity());
        mName = getArguments().getString("title");
        mChannelId = getArguments().getLong("type");
        if (mDebug) Log.d(TAG, "mName = " + mName + ", mChannelId = " + mChannelId);
        items.add(new SingleEditTextItem(mName, mChannelId));

        return items;
    }

    private class SingleEditTextItem extends EditTextItem {
        private final long channelid;

        private SingleEditTextItem(String title, long id) {
            super(title, -1);//String.valueOf(progress));
            channelid = id;
        }

        @Override
        protected void onTextChanged(String text) {
            super.onTextChanged(text);
            //mChannelSettingsManager.setVolumeCompensate(progress - 20);
            if (mChannelSettingsManager == null) {
                mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
            }
            if (text != null) {
                mChannelSettingsManager.setChannelName(channelid, text);
            }
            if (mDebug)  Log.d(TAG, "onTextChanged text = " + text);
        }

        @Override
        protected void onFocused() {
            //super.onFocused();
        }
    }
}