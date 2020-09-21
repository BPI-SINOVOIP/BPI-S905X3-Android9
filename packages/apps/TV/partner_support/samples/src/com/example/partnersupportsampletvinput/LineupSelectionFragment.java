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

import android.app.Activity;
import android.app.FragmentManager;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.google.android.tv.partner.support.EpgContract;
import com.google.android.tv.partner.support.EpgInput;
import com.google.android.tv.partner.support.EpgInputs;
import com.google.android.tv.partner.support.Lineup;
import com.google.android.tv.partner.support.Lineups;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

/** Lineup Selection Fragment for users to select a lineup */
public class LineupSelectionFragment extends GuidedStepFragment {
    public static final boolean DEBUG = false;
    public static final String TAG = "LineupSelectionFragment";

    public static final String KEY_TITLE = "title";
    public static final String KEY_DESCRIPTION = "description";
    private static final long ACTION_ID_STATUS = 1;
    private static final long ACTION_ID_LINEUPS = 2;
    private static final Pattern CHANNEL_NUMBER_DELIMITER = Pattern.compile("([ .-])");
    private Activity mActivity;
    private GuidedAction mStatusAction;
    private String mPostcode;
    private List<String> mChannelNumbers;
    private Map<GuidedAction, Lineup> mActionLineupMap = new HashMap<>();

    private AsyncTask<Void, Void, List<Pair<Lineup, Integer>>> mFetchLineupTask;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mActivity = getActivity();
        mStatusAction =
                new GuidedAction.Builder(mActivity)
                        .id(ACTION_ID_STATUS)
                        .title("Loading lineups")
                        .description("please wait")
                        .focusable(false)
                        .build();

        mFetchLineupTask =
                new AsyncTask<Void, Void, List<Pair<Lineup, Integer>>>() {
                    @Override
                    protected List<Pair<Lineup, Integer>> doInBackground(Void... voids) {
                        return lineupChannelMatchCount(getLineups(), getChannelNumbers());
                    }

                    @Override
                    protected void onPostExecute(List<Pair<Lineup, Integer>> result) {
                        List<GuidedAction> actions = new ArrayList<>();
                        if (result.isEmpty()) {
                            mStatusAction.setTitle("No lineup found for postcode: " + mPostcode);
                            mStatusAction.setDescription("");
                            mStatusAction.setFocusable(true);
                            notifyActionChanged(findActionPositionById(ACTION_ID_STATUS));
                            return;
                        }
                        mActionLineupMap = new HashMap<>();
                        for (Pair<Lineup, Integer> pair : result) {
                            Lineup lineup = pair.first;
                            String title =
                                    TextUtils.isEmpty(lineup.getName())
                                            ? lineup.getId()
                                            : lineup.getName();
                            GuidedAction action =
                                    new GuidedAction.Builder(getActivity())
                                            .id(ACTION_ID_LINEUPS)
                                            .title(title)
                                            .description(pair.second + " channels match")
                                            .build();
                            mActionLineupMap.put(action, lineup);
                            actions.add(action);
                        }
                        setActions(actions);
                    }
                };

        super.onCreate(savedInstanceState);
    }

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new Guidance(
                "LineupSelectionFragment", "LineupSelectionFragment Description", null, null);
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = super.onCreateView(inflater, container, savedInstanceState);
        if (!mFetchLineupTask.isCancelled()) {
            mPostcode = getArguments().getString(ChannelScanFragment.KEY_POSTCODE);
            mChannelNumbers =
                    getArguments().getStringArrayList(ChannelScanFragment.KEY_CHANNEL_NUMBERS);
            mFetchLineupTask.execute();
        }
        return v;
    }

    @Override
    public void onDestroyView() {
        mFetchLineupTask.cancel(true);
        super.onDestroyView();
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(mStatusAction);
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        if (action == null) {
            return;
        }
        switch ((int) action.getId()) {
            case (int) ACTION_ID_STATUS:
                finishGuidedStepFragments();
                break;
            case (int) ACTION_ID_LINEUPS:
                FragmentManager fm = getFragmentManager();
                super.onGuidedActionClicked(action);
                onLineupSelected(mActionLineupMap.get(action));
                ResultFragment fragment = new ResultFragment();
                Bundle args = new Bundle();
                args.putString(KEY_TITLE, action.getTitle().toString());
                args.putString(KEY_DESCRIPTION, action.getDescription().toString());
                fragment.setArguments(args);
                GuidedStepFragment.add(fm, fragment);
                break;
            default:
        }
    }

    private List<Pair<Lineup, Integer>> lineupChannelMatchCount(
            List<Lineup> lineups, List<String> localChannels) {
        List<Pair<Lineup, Integer>> result = new ArrayList<>();
        for (Lineup lineup : lineups) {
            result.add(new Pair<>(lineup, getMatchCount(lineup.getChannels(), localChannels)));
        }
        // sort in decreasing order
        Collections.sort(
                result,
                new Comparator<Pair<Lineup, Integer>>() {
                    @Override
                    public int compare(Pair<Lineup, Integer> pair, Pair<Lineup, Integer> other) {
                        return Integer.compare(other.second, pair.second);
                    }
                });
        return result;
    }

    private static int getMatchCount(List<String> lineupChannels, List<String> localChannels) {
        int count = 0;
        for (String lineupChannel : lineupChannels) {
            if (TextUtils.isEmpty(lineupChannel)) {
                continue;
            }
            List<String> parsedNumbers = parseChannelNumber(lineupChannel);
            for (String channel : localChannels) {
                if (TextUtils.isEmpty(channel)) {
                    continue;
                }
                if (matchChannelNumber(parsedNumbers, parseChannelNumber(channel))) {
                    if (DEBUG) {
                        Log.d(TAG, lineupChannel + " matches " + channel);
                    }
                    count++;
                    break;
                }
            }
        }
        return count;
    }

    private List<Lineup> getLineups() {
        return new ArrayList<>(Lineups.query(mActivity.getContentResolver(), mPostcode));
    }

    private List<String> getChannelNumbers() {
        return mChannelNumbers;
    }

    private void onLineupSelected(@Nullable Lineup lineup) {
        if (lineup == null) {
            return;
        }
        ContentValues values = new ContentValues();
        values.put(EpgContract.EpgInputs.COLUMN_INPUT_ID, SampleTvInputService.INPUT_ID);
        values.put(EpgContract.EpgInputs.COLUMN_LINEUP_ID, lineup.getId());

        ContentResolver contentResolver = getActivity().getContentResolver();
        EpgInput epgInput = EpgInputs.queryEpgInput(contentResolver, SampleTvInputService.INPUT_ID);
        if (epgInput == null) {
            contentResolver.insert(EpgContract.EpgInputs.CONTENT_URI, values);
        } else {
            values.put(EpgContract.EpgInputs.COLUMN_ID, epgInput.getId());
            EpgInputs.update(contentResolver, EpgInput.createEpgChannel(values));
        }
    }

    /**
     * Parses the channel number string to a list of numbers (major number, minor number, etc.).
     *
     * @param channelNumber the display number of the channel
     * @return a list of numbers
     */
    private static List<String> parseChannelNumber(String channelNumber) {
        // TODO: add tests for this method
        List<String> numbers =
                new ArrayList<>(
                        Arrays.asList(TextUtils.split(channelNumber, CHANNEL_NUMBER_DELIMITER)));
        numbers.removeAll(Collections.singleton(""));
        if (numbers.size() < 1 || numbers.size() > 2) {
            Log.w(TAG, "unsupported channel number format: " + channelNumber);
            return new ArrayList<>();
        }
        return numbers;
    }

    /**
     * Checks whether two lists of channel numbers match or not. If the sizes are different,
     * additional elements are ignore.
     */
    private static boolean matchChannelNumber(List<String> numbers, List<String> other) {
        if (numbers.isEmpty() || other.isEmpty()) {
            return false;
        }
        int i = 0;
        int j = 0;
        while (i < numbers.size() && j < other.size()) {
            if (!numbers.get(i++).equals(other.get(j++))) {
                return false;
            }
        }
        return true;
    }
}
