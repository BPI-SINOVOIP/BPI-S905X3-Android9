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
import android.widget.TextView;
import android.view.View;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.ui.TunableTvView;
import com.android.tv.data.api.Channel;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Timer;
import java.util.TimerTask;

public class ChannelInfoFragment extends SideFragment {
    private static final String TRACKER_LABEL = "ChannelInfoFragment";
    private List<ActionItem> mActionItems;
    private InfoItem mStrength;
    private InfoItem mQuality;
    private boolean mScheduled = false;

    private TimerTask task = new TimerTask() {
        public void run() {
            (getMainActivity()).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    updateStrength();
                }
            });
        }
    };

    private Timer timer = new Timer();

    private void initStrengthQualityUpdate() {
        if (mScheduled) {
            abortStrengthQualityUpdate();
        }
        timer.schedule(task, 1000, 1000);
        mScheduled = true;
        //Log.d(TRACKER_LABEL, "initStrengthQualityUpdate");
    }

    private void abortStrengthQualityUpdate() {
        if (mScheduled) {
            task.cancel();
            timer.cancel();
            mScheduled = false;
            //Log.d(TRACKER_LABEL, "abortStrengthQualityUpdate");
        }
    }

    private boolean isDtvKitChannel() {
        boolean result = false;
        MainActivity mainAcitvity = getMainActivity();
        TunableTvView tvview = null;
        Channel channel = null;
        if (mainAcitvity != null) {
            tvview = mainAcitvity.getTvView();
        }
        if (tvview != null) {
            channel = tvview.getCurrentChannel();
        }
        if (channel != null && (channel.getInputId()).startsWith(ChannelSettingsManager.DTVKIT_PACKAGE)) {
            result = true;
        }
        return result;
    }

    private void updateStrength() {
        boolean isDtvKit = isDtvKitChannel();
        if (!isDtvKit) {
            return;
        }
        MainActivity mainAcitvity = getMainActivity();
        if (mainAcitvity != null) {
            TunableTvView tvview = mainAcitvity.getTvView();
            if (tvview != null) {
                int strength = tvview.getSignalStrength();
                int quality = tvview.getSignalQuality();
                if (mStrength != null && !(String.valueOf(strength) + "%").equals(mStrength.mValue)) {
                    mStrength.setDisplay(null, String.valueOf(strength) + "%");
                    Log.d(TRACKER_LABEL, "update Strength");
                }
                if (mQuality != null && !(String.valueOf(quality) + "%").equals(mQuality.mValue)) {
                    mQuality.setDisplay(null, String.valueOf(quality) + "%");
                    Log.d(TRACKER_LABEL, "update Quality");
                }
            }
        } else {
            Log.d(TRACKER_LABEL, "task mStrength null");
        }
    }

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_info);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        //mActionItems = new ArrayList<>();
        ChannelSettingsManager manager = new ChannelSettingsManager(getMainActivity());
        ArrayList<HashMap<String, String>> optionListData = new ArrayList<HashMap<String, String>>();
        optionListData = manager.getChannelInfoStatus();
        if (optionListData != null) {
            InfoItem temp;
            for (int i =0; i < optionListData.size(); i++) {
                String name = optionListData.get(i).get(ChannelSettingsManager.STRING_NAME);
                String value = optionListData.get(i).get(ChannelSettingsManager.STRING_STATUS);
                String itemkeyvalue = optionListData.get(i).get(ChannelSettingsManager.KEY_INFO_ITEM);
                if (ChannelSettingsManager.VALUE_STRENGTH.equals(itemkeyvalue)) {
                    if (isDtvKitChannel()) {
                        TunableTvView tvview = getMainActivity().getTvView();
                        if (tvview != null) {
                            value = String.valueOf(tvview.getSignalStrength());
                        }
                    }
                    temp = new InfoItem(name, value + "%");
                    mStrength = temp;
                } else if (ChannelSettingsManager.VALUE_QUALITY.equals(itemkeyvalue)) {
                    if (isDtvKitChannel()) {
                        TunableTvView tvview = getMainActivity().getTvView();
                        if (tvview != null) {
                            value = String.valueOf(tvview.getSignalQuality());
                        }
                        temp = new InfoItem(name, value + "%");
                        mQuality = temp;
                    } else {
                        continue;//other case can't be available for the moment
                    }
                } else {
                    temp = new InfoItem(name, value);
                }
                items.add(temp);
            }
        }

        return items;
    }

    @Override
    public void onResume() {
        super.onResume();
        initStrengthQualityUpdate();
    }

    @Override
    public void onPause() {
        super.onPause();
        abortStrengthQualityUpdate();
    }

    public class InfoItem extends ActionItem {
        private String mItem;
        private String mValue;
        private TextView mTextView;

        public InfoItem(String item, String value) {
            super(item + ": " + value);
            this.mItem = item;
            this.mValue = value;
        }

        public void setDisplay(String item, String value) {
            if (mTextView == null) {
                return;
            }
            if (item != null && value != null) {
                mTextView.setText(mItem + ": " + mValue);
            } else if (item != null) {
                mTextView.setText(item + ": " + mValue);
            } else {
                mTextView.setText(mItem + ": " + value);
            }
        }

        @Override
        protected int getResourceId() {
            return R.layout.option_item_action;
        }

        @Override
        protected void onBind(View view) {
            super.onBind(view);
            mTextView = (TextView) view.findViewById(R.id.title);
            mTextView.setText(mItem + ": " + mValue);
            TextView descriptionView = (TextView) view.findViewById(R.id.description);
            descriptionView.setVisibility(View.GONE);
        }

        @Override
        protected void onSelected() {

        }
    }
}