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

package com.android.tv.tuner.setup;

import android.content.res.Resources;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import android.util.Log;
import android.view.View;
import com.android.tv.common.ui.setup.SetupGuidedStepFragment;
import com.android.tv.common.ui.setup.SetupMultiPaneFragment;
import com.android.tv.tuner.R;
import java.util.ArrayList;
import java.util.List;

/** Lineup Fragment shows available lineups and lets users select one of them. */
public class LineupFragment extends SetupMultiPaneFragment {
    public static final String TAG = "LineupFragment";
    public static final boolean DEBUG = false;

    public static final String ACTION_CATEGORY = "com.android.tv.tuner.setup.LineupFragment";
    public static final String KEY_LINEUP_NAMES = "lineup_names";
    public static final String KEY_MATCH_NUMBERS = "match_numbers";
    public static final String KEY_DEFAULT_LINEUP = "default_lineup";
    public static final String KEY_LINEUP_NOT_FOUND = "lineup_not_found";
    public static final int ACTION_ID_RETRY = SetupMultiPaneFragment.MAX_SUBCLASSES_ID - 1;

    private ContentFragment contentFragment;
    private Bundle args;
    private ArrayList<String> lineups;
    private boolean lineupNotFound;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        if (savedInstanceState == null) {
            lineups = getArguments().getStringArrayList(KEY_LINEUP_NAMES);
        }
        super.onCreate(savedInstanceState);
    }

    @Override
    protected SetupGuidedStepFragment onCreateContentFragment() {
        contentFragment = new LineupFragment.ContentFragment();
        Bundle args = new Bundle();
        Bundle arguments = this.args != null ? this.args : getArguments();
        args.putStringArrayList(KEY_LINEUP_NAMES, lineups);
        args.putIntegerArrayList(
                KEY_MATCH_NUMBERS, arguments.getIntegerArrayList(KEY_MATCH_NUMBERS));
        args.putInt(KEY_DEFAULT_LINEUP, arguments.getInt(KEY_DEFAULT_LINEUP));
        args.putBoolean(KEY_LINEUP_NOT_FOUND, lineupNotFound);
        contentFragment.setArguments(args);
        return contentFragment;
    }

    @Override
    protected String getActionCategory() {
        return ACTION_CATEGORY;
    }

    public void onLineupFound(Bundle args) {
        if (DEBUG) {
            Log.d(TAG, "onLineupFound");
        }
        this.args = args;
        lineups = args.getStringArrayList(KEY_LINEUP_NAMES);
        lineupNotFound = false;
        if (contentFragment != null) {
            updateContentFragment();
        }
    }

    public void onLineupNotFound() {
        if (DEBUG) {
            Log.d(TAG, "onLineupNotFound");
        }
        lineupNotFound = true;
        if (contentFragment != null) {
            updateContentFragment();
        }
    }

    public void onRetry() {
        if (DEBUG) {
            Log.d(TAG, "onRetry");
        }
        if (contentFragment != null) {
            lineupNotFound = false;
            lineups = null;
            updateContentFragment();
        } else {
            // onRetry() can be called only when retry button is clicked.
            // This should never happen.
            throw new RuntimeException(
                    "ContentFragment hasn't been created when onRetry() is called");
        }
    }

    @Override
    protected boolean needsDoneButton() {
        return false;
    }

    private void updateContentFragment() {
        contentFragment = (ContentFragment) onCreateContentFragment();
        getChildFragmentManager()
                .beginTransaction()
                .replace(
                        com.android.tv.common.R.id.guided_step_fragment_container,
                        contentFragment,
                        SetupMultiPaneFragment.CONTENT_FRAGMENT_TAG)
                .commit();
    }

    /** The content fragment of {@link LineupFragment}. */
    public static class ContentFragment extends SetupGuidedStepFragment {

        private ArrayList<String> lineups;
        private ArrayList<Integer> matchNumbers;
        private int defaultLineup;
        private boolean lineupNotFound;

        @Override
        public void onCreate(Bundle savedInstanceState) {
            if (savedInstanceState == null) {
                lineups = getArguments().getStringArrayList(KEY_LINEUP_NAMES);
                matchNumbers = getArguments().getIntegerArrayList(KEY_MATCH_NUMBERS);
                defaultLineup = getArguments().getInt(KEY_DEFAULT_LINEUP);
                this.lineupNotFound = getArguments().getBoolean(KEY_LINEUP_NOT_FOUND);
            }
            super.onCreate(savedInstanceState);
        }

        @Override
        public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
            super.onViewCreated(view, savedInstanceState);
            int position = findActionPositionById(defaultLineup);
            if (position >= 0 && position < getActions().size()) {
                setSelectedActionPosition(position);
            }
        }

        @NonNull
        @Override
        public Guidance onCreateGuidance(Bundle savedInstanceState) {
            if ((lineups != null && lineups.isEmpty()) || this.lineupNotFound) {
                return new Guidance(
                        getString(R.string.ut_lineup_title_lineups_not_found),
                        getString(R.string.ut_lineup_description_lineups_not_found),
                        getString(R.string.ut_setup_breadcrumb),
                        null);
            } else if (lineups == null) {
                return new Guidance(
                        getString(R.string.ut_lineup_title_fetching_lineups),
                        getString(R.string.ut_lineup_description_fetching_lineups),
                        getString(R.string.ut_setup_breadcrumb),
                        null);
            }
            return new Guidance(
                    getString(R.string.ut_lineup_title_lineups_found),
                    getString(R.string.ut_lineup_description_lineups_found),
                    getString(R.string.ut_setup_breadcrumb),
                    null);
        }

        @Override
        public void onCreateActions(
                @NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
            actions.addAll(buildActions());
        }

        @Override
        protected String getActionCategory() {
            return ACTION_CATEGORY;
        }

        private List<GuidedAction> buildActions() {
            List<GuidedAction> actions = new ArrayList<>();

            if ((lineups != null && lineups.isEmpty()) || this.lineupNotFound) {
                actions.add(
                        new GuidedAction.Builder(getActivity())
                                .id(ACTION_ID_RETRY)
                                .title(com.android.tv.common.R.string.action_text_retry)
                                .build());
                actions.add(
                        new GuidedAction.Builder(getActivity())
                                .id(ACTION_SKIP)
                                .title(com.android.tv.common.R.string.action_text_skip)
                                .build());
            } else if (lineups == null) {
                actions.add(
                        new GuidedAction.Builder(getActivity())
                                .id(ACTION_SKIP)
                                .title(com.android.tv.common.R.string.action_text_skip)
                                .build());
            } else {
                Resources res = getResources();
                for (int i = 0; i < lineups.size(); ++i) {
                    int matchNumber = matchNumbers.get(i);
                    String description =
                            matchNumber == 0
                                    ? res.getString(R.string.ut_lineup_no_channels_matched)
                                    : res.getQuantityString(
                                            R.plurals.ut_lineup_channels_matched,
                                            matchNumber,
                                            matchNumber);
                    actions.add(
                            new GuidedAction.Builder(getActivity())
                                    .id(i)
                                    .title(lineups.get(i))
                                    .description(description)
                                    .build());
                }
            }
            return actions;
        }
    }
}
