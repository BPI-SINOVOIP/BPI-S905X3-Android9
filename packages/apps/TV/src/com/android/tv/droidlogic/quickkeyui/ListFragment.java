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

package com.android.tv.droidlogic.quickkeyui;

import android.util.Log;
import android.media.tv.TvContract;
import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.data.api.Channel;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.data.ChannelNumber;
import com.android.tv.R;

import java.util.ArrayList;
import java.util.List;
import java.util.Collections;
import java.util.Comparator;

import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.DroidLogicTvUtils;

public class ListFragment extends SideFragment {
    private static final String TAG = "ListFragment";

    @Override
    protected String getTitle() {
        return getString(R.string.channel_source_setting);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    private List<Channel> videoChannelsList = new ArrayList<Channel>();
    private List<Channel> radioChannelsList = new ArrayList<Channel>();

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        int listvalue = getArguments().getInt("list");
        int typevalue = getArguments().getInt("type");
        List<Channel> alllist = getNeedChannelList(listvalue, typevalue);
        Collections.sort(alllist, new StringComparator());
        for (Channel one : alllist) {
            RadioButtonItem single = new SingleOptionItem(one.getDisplayNumber() + "." + one.getDisplayName(), one.getId());
            if (getMainActivity().mQuickKeyInfo.getCurrentChannelId() == one.getId()) {
                single.setChecked(true);
            }
            items.add(single);
        }

        return items;
    }

    private List<Channel> getNeedChannelList(int listvalue, int typevalue) {
        List<Channel> need = new ArrayList<>();
        if (listvalue == MultiOptionFragment.LIST_CHANNEL) {
            if (typevalue == MultiOptionFragment.TYPE_VIDEO) {
                List<Channel> tempVideoChannelList = getMainActivity().mQuickKeyInfo.getChannelTuner().getVideoChannelList();
                if (tempVideoChannelList != null) {
                    videoChannelsList.clear();
                    for (int i = 0; i <= tempVideoChannelList.size() - 1; i++) {
                        Log.d(TAG, "ChannelList:videochannels=============display number: " + tempVideoChannelList.get(i).getDisplayNumber()
                            + ", channel type: " + tempVideoChannelList.get(i).getType());
                        if (TvContract.Channels.TYPE_OTHER.equals(tempVideoChannelList.get(i).getType())) {
                            if (!tempVideoChannelList.get(i).IsHidden()) {
                                videoChannelsList.add(tempVideoChannelList.get(i));
                            } else {
                                Log.d(TAG, "skip hidden channels");
                            }
                        } else if (DroidLogicTvUtils.isAtscCountry(getActivity())) {
                            ChannelInfo channelInfo = getMainActivity().convertToChannelInfo(tempVideoChannelList.get(i), getActivity());
                            if (channelInfo != null
                                && channelInfo.getSignalType().equals(DroidLogicTvUtils.getCurrentSignalType(getActivity()))) {
                                videoChannelsList.add(tempVideoChannelList.get(i));
                            }
                        } else {
                            if (DroidLogicTvUtils.isATV(getActivity()) && tempVideoChannelList.get(i).isAnalogChannel()) {
                                videoChannelsList.add(tempVideoChannelList.get(i));
                            } else if (DroidLogicTvUtils.isDTV(getActivity()) && tempVideoChannelList.get(i).isDigitalChannel()) {
                                videoChannelsList.add(tempVideoChannelList.get(i));
                            }
                        }
                    }
                } else {
                    Log.d(TAG, "tempVideoChannelList == null");
                }
                return videoChannelsList;
            } else if (typevalue == MultiOptionFragment.TYPE_RADIO) {
                List<Channel> tempRadioChannelList = getMainActivity().mQuickKeyInfo.getChannelTuner().getRadioChannelList();
                if (tempRadioChannelList != null) {
                    radioChannelsList.clear();
                    for (int i = 0; i <= tempRadioChannelList.size() - 1; i++) {
                        Log.d(TAG, "ChannelList:radiochannels=============display number: " + tempRadioChannelList.get(i).getDisplayNumber()
                            + ", channel type: " + tempRadioChannelList.get(i).getType());
                        if (TvContract.Channels.TYPE_OTHER.equals(tempRadioChannelList.get(i).getType())) {
                            radioChannelsList.add(tempRadioChannelList.get(i));
                        } else if (DroidLogicTvUtils.isAtscCountry(getActivity())) {
                            ChannelInfo channelInfo = getMainActivity().convertToChannelInfo(tempRadioChannelList.get(i), getActivity());
                            if (channelInfo != null
                                && channelInfo.getSignalType().equals(DroidLogicTvUtils.getCurrentSignalType(getActivity()))) {
                                radioChannelsList.add(tempRadioChannelList.get(i));
                            }
                        } else {
                            if (DroidLogicTvUtils.isATV(getActivity()) && tempRadioChannelList.get(i).isAnalogChannel()) {
                                radioChannelsList.add(tempRadioChannelList.get(i));
                            } else if (DroidLogicTvUtils.isDTV(getActivity()) && tempRadioChannelList.get(i).isDigitalChannel()) {
                                radioChannelsList.add(tempRadioChannelList.get(i));
                            }
                        }
                    }
                } else {
                    Log.d(TAG, "tempRadioChannelList == null");
                }
                return radioChannelsList;
            }
        } else if (listvalue == MultiOptionFragment.LIST_FAV) {
            if (typevalue == MultiOptionFragment.TYPE_VIDEO) {
                List<Channel> all = getMainActivity().mQuickKeyInfo.getChannelTuner().getVideoChannelList();
                for (Channel one : all) {
                    if (one.isFavourite()) {
                        if (TvContract.Channels.TYPE_OTHER.equals(one.getType())) {
                            need.add(one);
                        } else if (DroidLogicTvUtils.isAtscCountry(getActivity())) {
                            ChannelInfo channelInfo = getMainActivity().convertToChannelInfo(one, getActivity());
                            if (channelInfo != null
                                && channelInfo.getSignalType().equals(DroidLogicTvUtils.getCurrentSignalType(getActivity()))) {
                                need.add(one);
                            }
                        } else {
                            if (DroidLogicTvUtils.isATV(getActivity()) && one.isAnalogChannel()) {
                                need.add(one);
                            } else if (DroidLogicTvUtils.isDTV(getActivity()) && one.isDigitalChannel()) {
                                need.add(one);
                            }
                        }
                    }
                }
                return need;
            } else if (typevalue == MultiOptionFragment.TYPE_RADIO) {
                List<Channel> all = getMainActivity().mQuickKeyInfo.getChannelTuner().getRadioChannelList();
                for (Channel one : all) {
                    if (one.isFavourite()) {
                        if (TvContract.Channels.TYPE_OTHER.equals(one.getType())) {
                            need.add(one);
                        } else if (DroidLogicTvUtils.isAtscCountry(getActivity())) {
                            ChannelInfo channelInfo = getMainActivity().convertToChannelInfo(one, getActivity());
                            if (channelInfo != null
                                && channelInfo.getSignalType().equals(DroidLogicTvUtils.getCurrentSignalType(getActivity()))) {
                                need.add(one);
                            }
                        } else {
                            if (DroidLogicTvUtils.isATV(getActivity()) && one.isAnalogChannel()) {
                                need.add(one);
                            } else if (DroidLogicTvUtils.isDTV(getActivity()) && one.isDigitalChannel()) {
                                need.add(one);
                            }
                        }
                    }
                }
                return need;
            }
        }
        return need;
    }

    private class SingleOptionItem extends RadioButtonItem {
        private final long mTrackId;
        private final String mTitle;

        private SingleOptionItem(String title, long trackId) {
            super(title);
            mTitle = title;
            mTrackId = trackId;
        }

        public String getTitle() {
            return mTitle;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            if (getMainActivity().isCurrentChannelDvrRecording()) {//deal dvr recording first
                Log.d(TAG, "check pvr status");
                getMainActivity().CheckNeedStopDvrFragmentWhenTuneTo(getMainActivity().mQuickKeyInfo.getChannelTuner().getChannelById(mTrackId));
                closeFragment();
            } else {
                getMainActivity().tuneToChannel(getMainActivity().mQuickKeyInfo.getChannelTuner().getChannelById(mTrackId));
            }
            //closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
        }
    }

    private class StringComparator implements Comparator<Channel> {
        public int compare(Channel object1, Channel object2) {
            String p1 = object1.getDisplayNumber();
            String p2 = object2.getDisplayNumber();
            return ChannelNumber.compare(p1, p2);
        }
    }
}
