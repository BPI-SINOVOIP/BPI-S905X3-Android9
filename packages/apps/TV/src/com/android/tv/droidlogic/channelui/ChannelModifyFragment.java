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

import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.os.Bundle;
import android.util.Log;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.data.ChannelNumber;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Collections;
import java.util.Comparator;

import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;

public class ChannelModifyFragment extends SideFragment {
    private static final String TAG = "ChannelModifyFragment";
    private static final boolean mDebug = false;
    private ChannelSettingsManager mChannelSettingsManager;
    private static final int VIDEO_TYPE = 0;
    private static final int RADIO_TYPE = 1;
    private int mType;

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        String title = getString(R.string.channel_edit_tv);
        if (mType == VIDEO_TYPE) {
            title = getString(R.string.channel_edit_tv);
        } else if (mType == RADIO_TYPE) {
            title = getString(R.string.channel_edit_radio);
        }
        return title;
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    private ArrayList<ChannelInfo> optionListData;

    @Override
    protected List<Item> getItemList() {
        Log.d(TAG, "getItemList");
        List<Item> items = new ArrayList<>();
        List<DynamicActionItem> mActionItems = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        int type = (int)(getArguments().getLong("type"));

        ArrayList<ChannelInfo> videochannels = null;
        ArrayList<ChannelInfo> radiochannels = null;
        ArrayList<ChannelInfo> videochannelslist = new ArrayList<ChannelInfo>();
        ArrayList<ChannelInfo> radiochannelslist = new ArrayList<ChannelInfo>();
        ChannelInfo currentchannelinfo = null;
        String inputid = null;
        if (getMainActivity().mQuickKeyInfo.getCurrentTvInputInfo() != null) {
            inputid = getMainActivity().mQuickKeyInfo.getCurrentTvInputInfo().getId();
        }

        if (inputid != null) {
            videochannels = mChannelSettingsManager.getVideoChannelList();
            //[DroidLogic]
            //when in Channel Edit,just show the channels that belong to the current Type.
            videochannelslist.clear();
            for (int i = 0; i <= videochannels.size() - 1; i++) {
                Log.d(TAG, "ChannelEdit:videochannels=============display number: " + videochannels.get(i).getDisplayNumber()
                    + ", channel type: " + videochannels.get(i).getType());
                if (TvContract.Channels.TYPE_OTHER.equals(videochannels.get(i).getType())) {
                    videochannelslist.add(videochannels.get(i));
                } else if (DroidLogicTvUtils.isAtscCountry(getActivity())) {
                    if (videochannels.get(i).getSignalType().equals(DroidLogicTvUtils.getCurrentSignalType(getActivity()))) {
                        videochannelslist.add(videochannels.get(i));
                    }
                } else {
                    if (DroidLogicTvUtils.isATV(getActivity()) && videochannels.get(i).isAnalogChannel()) {
                        videochannelslist.add(videochannels.get(i));
                    } else if (DroidLogicTvUtils.isDTV(getActivity()) && videochannels.get(i).isDigitalChannel()) {
                        videochannelslist.add(videochannels.get(i));
                    }
                }
            }
            radiochannels = mChannelSettingsManager.getRadioChannelList();
            radiochannelslist.clear();
            for (int i = 0; i <= radiochannels.size() - 1; i++) {
                Log.d(TAG, "ChannelEdit:radiochannels=============display number: " + radiochannels.get(i).getDisplayNumber()
                    + ", channel type: " + radiochannels.get(i).getType());
                if (TvContract.Channels.TYPE_OTHER.equals(radiochannels.get(i).getType())) {
                    radiochannelslist.add(radiochannels.get(i));
                } else if (DroidLogicTvUtils.isAtscCountry(getActivity())) {
                    if (radiochannels.get(i).getSignalType().equals(DroidLogicTvUtils.getCurrentSignalType(getActivity()))) {
                        radiochannelslist.add(radiochannels.get(i));
                    }
                } else {
                    if (DroidLogicTvUtils.isATV(getActivity()) && radiochannels.get(i).isAnalogChannel()) {
                        radiochannelslist.add(radiochannels.get(i));
                    } else if (DroidLogicTvUtils.isDTV(getActivity()) && radiochannels.get(i).isDigitalChannel()) {
                        radiochannelslist.add(radiochannels.get(i));
                    }
                }
            }
            Collections.sort(videochannelslist, new StringComparator());
            Collections.sort(radiochannelslist, new StringComparator());
        }

        if (type == VIDEO_TYPE) {
            optionListData = videochannelslist;
        } else if (type == RADIO_TYPE) {
            optionListData = radiochannelslist;
        }

        if (optionListData != null) {
            for (int i = 0; i < optionListData.size(); i++) {
                String info = null;
                if (optionListData.get(i).isOtherChannel() && optionListData.get(i).getHidden() == 1) {
                    info = getString(R.string.channel_edit_skip);//for other channel
                } else if (!optionListData.get(i).isOtherChannel() && !optionListData.get(i).isBrowsable()) {
                    info = getString(R.string.channel_edit_skip);
                } else if (optionListData.get(i).isFavourite()) {
                    info = getString(R.string.channel_edit_favourite);
                }
                String displaynumber = optionListData.get(i).getDisplayNumber();
                String displayname = optionListData.get(i).getDisplayName();
                String localdisplay = optionListData.get(i).getDisplayNameLocal();
                String multiname = optionListData.get(i).getDisplayNameMulti();
                final String channelType = optionListData.get(i).getType();
                if (mDebug) Log.d(TAG, "[getItemList] optionListData.get(i).isBrowsable() = " + optionListData.get(i).isBrowsable() +
                    ", optionListData.get(i).isFavourite() = " + optionListData.get(i).isFavourite() + ", info = " + info +
                    ", displaynumber = " + displaynumber + ", displayname = " + displayname + ", localdisplay = " + localdisplay + ", multiname = " + multiname);
                DynamicSubMenuItem item = new DynamicSubMenuItem(displaynumber + "." +
                        (/*localdisplay != null ? localdisplay : */displayname),
                info, optionListData.get(i).getId(), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                    @Override
                    protected SideFragment getFragment() {
                        SideFragment fragment = new ChannelModifyDetailsFragment(channelType);
                        fragment.setListener(mSideFragmentListener);
                        return fragment;
                    }
                };
                long lastid = -1;
                long nextid = -1;
                if (i == 0) {
                    lastid = -1;
                } else {
                    lastid = optionListData.get(i - 1).getId();
                }
                if (i == optionListData.size() -1) {
                    nextid = -1;
                } else {
                    nextid = optionListData.get(i + 1).getId();
                }
                Bundle bundle = new Bundle();
                bundle.putLong("lastid", lastid);
                bundle.putLong("nextid", nextid);
                item.setBundle(bundle);
                mActionItems.add(item);
            }
        }

        items.addAll(mActionItems);

        return items;
    }

    private class StringComparator implements Comparator<ChannelInfo> {
        public int compare(ChannelInfo object1, ChannelInfo object2) {
            String p1 = object1.getDisplayNumber();
            String p2 = object2.getDisplayNumber();
            return ChannelNumber.compare(p1, p2);
        }
    }

}