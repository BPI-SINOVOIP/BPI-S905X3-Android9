/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.example.partnersupportsampletvinput;

import android.app.FragmentManager;
import android.content.ContentResolver;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.support.media.tv.Channel;
import android.support.media.tv.TvContractCompat;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

/** Channel Scan Fragment shows detected channels */
public class ChannelScanFragment extends GuidedStepFragment {
    public static final long ACTION_ID_SCAN_RESULTS = 1;
    public static final String KEY_POSTCODE = "postcode";
    public static final String KEY_CHANNEL_NUMBERS = "channel numbers";
    private GuidedAction mNextAction;
    private List<Channel> mChannels = new ArrayList<>();
    private Bundle mSavedInstanceState;

    private AsyncTask<Void, Void, List<Channel>> mChannelScanTask;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSavedInstanceState = savedInstanceState;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = super.onCreateView(inflater, container, savedInstanceState);
        mChannelScanTask =
                new AsyncTask<Void, Void, List<Channel>>() {
                    @Override
                    protected List<Channel> doInBackground(Void... voids) {
                        return scanChannel();
                    }

                    @Override
                    protected void onPostExecute(List<Channel> result) {
                        mChannels = result;
                        mNextAction.setFocusable(true);
                        mNextAction.setEnabled(true);
                        int position = findButtonActionPositionById(GuidedAction.ACTION_ID_NEXT);
                        if (position >= 0) {
                            notifyButtonActionChanged(position);
                            View buttonView = getButtonActionItemView(position);
                            buttonView.setFocusable(true);
                            buttonView.requestFocus();
                        }
                    }
                };
        mChannelScanTask.execute();
        return v;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mNextAction = null;
        if (mChannelScanTask != null) {
            mChannelScanTask.cancel(true);
            mChannelScanTask = null;
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        // reset actions
        List<GuidedAction> actions = new ArrayList<>();
        onCreateActions(actions, mSavedInstanceState);
        setActions(actions);
        actions = new ArrayList<>();
        onCreateButtonActions(actions, mSavedInstanceState);
        setButtonActions(actions);
    }

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new Guidance("Channel Scan", "Fake scanning for channels", null, null);
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(ACTION_ID_SCAN_RESULTS)
                        .title("Channels Found:")
                        .description("")
                        .multilineDescription(true)
                        .focusable(false)
                        .infoOnly(true)
                        .build());
    }

    @Override
    public void onCreateButtonActions(
            @NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        mNextAction =
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_NEXT)
                        .title("NEXT")
                        .focusable(false)
                        .enabled(false)
                        .build();
        actions.add(mNextAction);
        actions.add(
                new GuidedAction.Builder(getActivity())
                        .id(GuidedAction.ACTION_ID_CANCEL)
                        .title("CANCEL")
                        .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (action == null) {
            return;
        }
        FragmentManager fm = getFragmentManager();
        switch ((int) action.getId()) {
            case (int) GuidedAction.ACTION_ID_NEXT:
                LineupSelectionFragment fragment = new LineupSelectionFragment();
                Bundle args = new Bundle();
                args.putString(KEY_POSTCODE, getArguments().getString(KEY_POSTCODE));
                ArrayList<String> channelNumbers = new ArrayList<>();
                for (Channel channel : mChannels) {
                    if (channel != null) {
                        channelNumbers.add(channel.getDisplayNumber());
                    }
                }
                args.putStringArrayList(KEY_CHANNEL_NUMBERS, channelNumbers);
                fragment.setArguments(args);
                GuidedStepFragment.add(getFragmentManager(), fragment);
                break;
            case (int) GuidedAction.ACTION_ID_CANCEL:
                fm.popBackStack();
                break;
            default:
        }
    }

    private List<Channel> scanChannel() {
        List<Channel> channels = new ArrayList<>();
        channels.add(createFakeChannel("KQED", "9.1", 1));
        channels.add(createFakeChannel("BBC", "2-2", 2));
        channels.add(createFakeChannel("What\'s on", "1.1", 3));
        channels.add(createFakeChannel("FOX", "7.1", 4));
        channels.add(createFakeChannel("Play Movie", "1.1", 5));
        channels.add(createFakeChannel("PBS", "9.2", 6));
        channels.add(createFakeChannel("ApiTestChannel", "A.1", 7));
        updateChannels(channels);

        return channels;
    }

    private void updateChannels(List<Channel> channels) {
        HashMap<Integer, Channel> networkId2newChannel = new HashMap<>();
        for (Channel channel : channels) {
            networkId2newChannel.put(channel.getOriginalNetworkId(), channel);
        }

        ContentResolver cr = getActivity().getContentResolver();
        Cursor cursor =
                cr.query(
                        TvContractCompat.buildChannelsUriForInput(SampleTvInputService.INPUT_ID),
                        Channel.PROJECTION,
                        null,
                        null,
                        null,
                        null);
        ArrayList<Channel> existingChannels = new ArrayList<>();
        while (cursor.moveToNext()) {
            existingChannels.add(Channel.fromCursor(cursor));
        }
        Iterator<Channel> i = existingChannels.iterator();
        while (i.hasNext()) {
            Channel existingChannel = i.next();
            if (networkId2newChannel.containsKey(existingChannel.getOriginalNetworkId())) {
                i.remove();
                Channel newChannel =
                        networkId2newChannel.remove(existingChannel.getOriginalNetworkId());
                cr.update(
                        TvContractCompat.buildChannelUri(existingChannel.getId()),
                        newChannel.toContentValues(),
                        null,
                        null);
            }
        }
        for (Channel newChannel : networkId2newChannel.values()) {
            cr.insert(TvContractCompat.Channels.CONTENT_URI, newChannel.toContentValues());
        }
        for (Channel existingChannel : existingChannels) {
            cr.delete(TvContractCompat.buildChannelUri(existingChannel.getId()), null, null);
        }
    }

    private Channel createFakeChannel(String name, String number, int networkId) {
        Channel channel =
                new Channel.Builder()
                        .setDisplayName(name)
                        .setDisplayNumber(number)
                        .setInputId(SampleTvInputService.INPUT_ID)
                        .setOriginalNetworkId(networkId)
                        .build();
        onChannelDetected(channel);
        SystemClock.sleep(1000);

        return channel;
    }

    private void onChannelDetected(Channel channel) {
        getActivity()
                .runOnUiThread(
                        new Runnable() {
                            @Override
                            public void run() {
                                List<GuidedAction> actions = getActions();
                                actions.add(
                                        new GuidedAction.Builder(getActivity())
                                                .id(ACTION_ID_SCAN_RESULTS)
                                                .title(
                                                        channel.getDisplayNumber()
                                                                + " "
                                                                + channel.getDisplayName())
                                                .focusable(false)
                                                .build());
                                if (actions.size() >= 7) {
                                    actions.remove(1);
                                }
                                setActions(actions);
                            }
                        });
    }
}
