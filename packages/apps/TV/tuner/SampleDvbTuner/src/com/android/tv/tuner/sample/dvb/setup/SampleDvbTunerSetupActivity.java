/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tv.tuner.sample.dvb.setup;

import android.app.FragmentManager;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.media.tv.TvInputInfo;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;
import android.util.Pair;
import android.view.KeyEvent;
import com.android.tv.common.BaseApplication;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.ui.setup.SetupFragment;
import com.android.tv.common.ui.setup.SetupMultiPaneFragment;
import com.android.tv.common.util.PostalCodeUtils;
import com.android.tv.tuner.TunerHal;
import com.android.tv.tuner.sample.dvb.R;
import com.android.tv.tuner.setup.BaseTunerSetupActivity;
import com.android.tv.tuner.setup.ConnectionTypeFragment;
import com.android.tv.tuner.setup.LineupFragment;
import com.android.tv.tuner.setup.PostalCodeFragment;
import com.android.tv.tuner.setup.ScanFragment;
import com.android.tv.tuner.setup.ScanResultFragment;
import com.android.tv.tuner.setup.WelcomeFragment;
import com.google.android.tv.partner.support.EpgContract;
import com.google.android.tv.partner.support.EpgInput;
import com.google.android.tv.partner.support.EpgInputs;
import com.google.android.tv.partner.support.Lineup;
import com.google.android.tv.partner.support.Lineups;
import com.google.android.tv.partner.support.TunerSetupUtils;
import java.util.ArrayList;
import java.util.List;

/** An activity that serves Live TV tuner setup process. */
public class SampleDvbTunerSetupActivity extends BaseTunerSetupActivity {
    private static final String TAG = "SampleDvbTunerSetupActivity";
    private static final boolean DEBUG = false;

    private static final int FETCH_LINEUP_TIMEOUT_MS = 10000; // 10 seconds
    private static final int FETCH_LINEUP_RETRY_TIMEOUT_MS = 20000; // 20 seconds
    private static final String OTAD_PREFIX = "OTAD";
    private static final String STRING_BROADCAST_DIGITAL = "Broadcast Digital";

    private LineupFragment currentLineupFragment;

    private List<String> channelNumbers;
    private List<Lineup> lineups;
    private Lineup selectedLineup;
    private List<Pair<Lineup, Integer>> lineupMatchCountPair;
    private FetchLineupTask fetchLineupTask;
    private EpgInput epgInput;
    private String postalCode;
    private final Handler handler = new Handler();
    private final Runnable cancelFetchLineupTaskRunnable =
            new Runnable() {
                @Override
                public void run() {
                    cancelFetchLineup();
                }
            };
    private String embeddedInputId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        if (DEBUG) {
            Log.d(TAG, "onCreate");
        }
        embeddedInputId = BaseApplication.getSingletons(this).getEmbeddedTunerInputId();
        new QueryEpgInputTask(embeddedInputId).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void executeGetTunerTypeAndCountAsyncTask() {
        new AsyncTask<Void, Void, Integer>() {
            @Override
            protected Integer doInBackground(Void... arg0) {
                return TunerHal.getTunerTypeAndCount(SampleDvbTunerSetupActivity.this).first;
            }

            @Override
            protected void onPostExecute(Integer result) {
                if (!SampleDvbTunerSetupActivity.this.isDestroyed()) {
                    mTunerType = result;
                    if (result == null) {
                        finish();
                    } else if (!mActivityStopped) {
                        showInitialFragment();
                    } else {
                        mPendingShowInitialFragment = true;
                    }
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    @Override
    protected boolean executeAction(String category, int actionId, Bundle params) {
        switch (category) {
            case WelcomeFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case SetupMultiPaneFragment.ACTION_DONE:
                        super.executeAction(category, actionId, params);
                        break;
                    default:
                        String postalCode = PostalCodeUtils.getLastPostalCode(this);
                        if (mNeedToShowPostalCodeFragment
                                || (CommonFeatures.ENABLE_CLOUD_EPG_REGION.isEnabled(
                                                getApplicationContext())
                                        && TextUtils.isEmpty(postalCode))) {
                            // We cannot get postal code automatically. Postal code input fragment
                            // should always be shown even if users have input some valid postal
                            // code in this activity before.
                            mNeedToShowPostalCodeFragment = true;
                            showPostalCodeFragment();
                        } else {
                            lineups = null;
                            selectedLineup = null;
                            this.postalCode = postalCode;
                            restartFetchLineupTask();
                            showConnectionTypeFragment();
                        }
                        break;
                }
                return true;
            case PostalCodeFragment.ACTION_CATEGORY:
                lineups = null;
                selectedLineup = null;
                switch (actionId) {
                    case SetupMultiPaneFragment.ACTION_DONE:
                        String postalCode = params.getString(PostalCodeFragment.KEY_POSTAL_CODE);
                        if (postalCode != null) {
                            this.postalCode = postalCode;
                            restartFetchLineupTask();
                        }
                        // fall through
                    case SetupMultiPaneFragment.ACTION_SKIP:
                        showConnectionTypeFragment();
                        break;
                    default: // fall out
                }
                return true;
            case ConnectionTypeFragment.ACTION_CATEGORY:
                channelNumbers = null;
                lineupMatchCountPair = null;
                return super.executeAction(category, actionId, params);
            case ScanFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case ScanFragment.ACTION_CANCEL:
                        getFragmentManager().popBackStack();
                        return true;
                    case ScanFragment.ACTION_FINISH:
                        clearTunerHal();
                        channelNumbers =
                                params.getStringArrayList(ScanFragment.KEY_CHANNEL_NUMBERS);
                        selectedLineup = null;
                        if (CommonFeatures.ENABLE_CLOUD_EPG_REGION.isEnabled(
                                        getApplicationContext())
                                && channelNumbers != null
                                && !channelNumbers.isEmpty()
                                && !TextUtils.isEmpty(this.postalCode)) {
                            showLineupFragment();
                        } else {
                            showScanResultFragment();
                        }
                        return true;
                    default: // fall out
                }
                break;
            case LineupFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case LineupFragment.ACTION_SKIP:
                        selectedLineup = null;
                        currentLineupFragment = null;
                        showScanResultFragment();
                        break;
                    case LineupFragment.ACTION_ID_RETRY:
                        currentLineupFragment.onRetry();
                        restartFetchLineupTask();
                        handler.postDelayed(
                                cancelFetchLineupTaskRunnable, FETCH_LINEUP_RETRY_TIMEOUT_MS);
                        break;
                    default:
                        if (actionId >= 0 && actionId < lineupMatchCountPair.size()) {
                            if (DEBUG) {
                                if (selectedLineup != null) {
                                    Log.d(
                                            TAG,
                                            "Lineup " + selectedLineup.getName() + " is selected.");
                                }
                            }
                            selectedLineup = lineupMatchCountPair.get(actionId).first;
                        }
                        currentLineupFragment = null;
                        showScanResultFragment();
                        break;
                }
                return true;
            case ScanResultFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case SetupMultiPaneFragment.ACTION_DONE:
                        new SampleDvbTunerSetupActivity.InsertOrModifyEpgInputTask(
                                        selectedLineup, embeddedInputId)
                                .executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                        break;
                    default:
                        // scan again
                        if (lineups == null || lineups.isEmpty()) {
                            lineups = null;
                            restartFetchLineupTask();
                        }
                        super.executeAction(category, actionId, params);
                        break;
                }
                return true;
            default: // fall out
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            FragmentManager manager = getFragmentManager();
            int count = manager.getBackStackEntryCount();
            if (count > 0) {
                String lastTag = manager.getBackStackEntryAt(count - 1).getName();
                if (LineupFragment.class.getCanonicalName().equals(lastTag) && count >= 2) {
                    // Pops fragment including ScanFragment.
                    manager.popBackStack(
                            manager.getBackStackEntryAt(count - 2).getName(),
                            FragmentManager.POP_BACK_STACK_INCLUSIVE);
                    return true;
                }
                if (ScanResultFragment.class.getCanonicalName().equals(lastTag) && count >= 2) {
                    String secondLastTag = manager.getBackStackEntryAt(count - 2).getName();
                    if (ScanFragment.class.getCanonicalName().equals(secondLastTag)) {
                        // Pops fragment including ScanFragment.
                        manager.popBackStack(
                                secondLastTag, FragmentManager.POP_BACK_STACK_INCLUSIVE);
                        return true;
                    }
                    if (LineupFragment.class.getCanonicalName().equals(secondLastTag)) {
                        currentLineupFragment =
                                (LineupFragment) manager.findFragmentByTag(secondLastTag);
                        if (lineups == null || lineups.isEmpty()) {
                            lineups = null;
                            restartFetchLineupTask();
                        }
                    }
                } else if (ScanFragment.class.getCanonicalName().equals(lastTag)) {
                    mLastScanFragment.finishScan(true);
                    return true;
                }
            }
        }
        return super.onKeyUp(keyCode, event);
    }

    private void showLineupFragment() {
        if (lineupMatchCountPair == null && lineups != null) {
            lineupMatchCountPair = TunerSetupUtils.lineupChannelMatchCount(lineups, channelNumbers);
        }
        currentLineupFragment = new LineupFragment();
        currentLineupFragment.setArguments(getArgsForLineupFragment());
        currentLineupFragment.setShortDistance(
                SetupFragment.FRAGMENT_ENTER_TRANSITION | SetupFragment.FRAGMENT_RETURN_TRANSITION);
        handler.removeCallbacksAndMessages(null);
        showFragment(currentLineupFragment, true);
        handler.postDelayed(cancelFetchLineupTaskRunnable, FETCH_LINEUP_TIMEOUT_MS);
    }

    private Bundle getArgsForLineupFragment() {
        Bundle args = new Bundle();
        if (lineupMatchCountPair == null) {
            return args;
        }
        ArrayList<String> lineupNames = new ArrayList<>(lineupMatchCountPair.size());
        ArrayList<Integer> matchNumbers = new ArrayList<>(lineupMatchCountPair.size());
        int defaultLineupIndex = 0;
        for (Pair<Lineup, Integer> pair : lineupMatchCountPair) {
            Lineup lineup = pair.first;
            String name;
            if (!TextUtils.isEmpty(lineup.getName())) {
                name = lineup.getName();
            } else {
                name = lineup.getId();
            }
            if (name.equals(OTAD_PREFIX + postalCode) || name.equals(STRING_BROADCAST_DIGITAL)) {
                // rename OTA / antenna lineups
                name = getString(R.string.ut_lineup_name_antenna);
            }
            lineupNames.add(name);
            matchNumbers.add(pair.second);
            if (epgInput != null && TextUtils.equals(lineup.getId(), epgInput.getLineupId())) {
                // The last index is the current one.
                defaultLineupIndex = lineupNames.size() - 1;
            }
        }
        args.putStringArrayList(LineupFragment.KEY_LINEUP_NAMES, lineupNames);
        args.putIntegerArrayList(LineupFragment.KEY_MATCH_NUMBERS, matchNumbers);
        args.putInt(LineupFragment.KEY_DEFAULT_LINEUP, defaultLineupIndex);
        return args;
    }

    private void cancelFetchLineup() {
        if (fetchLineupTask == null) {
            return;
        }
        AsyncTask.Status status = fetchLineupTask.getStatus();
        if (status == AsyncTask.Status.RUNNING || status == AsyncTask.Status.PENDING) {
            fetchLineupTask.cancel(true);
            fetchLineupTask = null;
            if (currentLineupFragment != null) {
                currentLineupFragment.onLineupNotFound();
            }
        }
    }

    private void restartFetchLineupTask() {
        if (!CommonFeatures.ENABLE_CLOUD_EPG_REGION.isEnabled(getApplicationContext())
                || TextUtils.isEmpty(postalCode)) {
            return;
        }
        if (fetchLineupTask != null) {
            fetchLineupTask.cancel(true);
        }
        handler.removeCallbacksAndMessages(null);
        fetchLineupTask = new FetchLineupTask(getContentResolver(), postalCode);
        fetchLineupTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private class FetchLineupTask extends AsyncTask<Void, Void, List<Lineup>> {
        private final ContentResolver contentResolver;
        private final String postalCode;

        FetchLineupTask(ContentResolver contentResolver, String postalCode) {
            this.contentResolver = contentResolver;
            this.postalCode = postalCode;
        }

        @Override
        protected List<Lineup> doInBackground(Void... args) {
            if (contentResolver == null || TextUtils.isEmpty(postalCode)) {
                return new ArrayList<>();
            }
            return new ArrayList<>(Lineups.query(contentResolver, postalCode));
        }

        @Override
        protected void onPostExecute(List<Lineup> lineups) {
            if (DEBUG) {
                if (lineups != null) {
                    Log.d(TAG, "FetchLineupTask fetched " + lineups.size() + " lineups");
                } else {
                    Log.d(TAG, "FetchLineupTask returned null");
                }
            }
            SampleDvbTunerSetupActivity.this.lineups = lineups;
            if (currentLineupFragment != null) {
                if (lineups == null || lineups.isEmpty()) {
                    currentLineupFragment.onLineupNotFound();
                } else {
                    lineupMatchCountPair =
                            TunerSetupUtils.lineupChannelMatchCount(
                                    SampleDvbTunerSetupActivity.this.lineups, channelNumbers);
                    currentLineupFragment.onLineupFound(getArgsForLineupFragment());
                }
            }
        }
    }

    private class InsertOrModifyEpgInputTask extends AsyncTask<Void, Void, Void> {
        private final Lineup lineup;
        private final String inputId;

        InsertOrModifyEpgInputTask(@Nullable Lineup lineup, String inputId) {
            this.lineup = lineup;
            this.inputId = inputId;
        }

        @Override
        protected Void doInBackground(Void... args) {
            if (lineup == null
                    || (SampleDvbTunerSetupActivity.this.epgInput != null
                            && TextUtils.equals(
                                    lineup.getId(),
                                    SampleDvbTunerSetupActivity.this.epgInput.getLineupId()))) {
                return null;
            }
            ContentValues values = new ContentValues();
            values.put(EpgContract.EpgInputs.COLUMN_INPUT_ID, inputId);
            values.put(EpgContract.EpgInputs.COLUMN_LINEUP_ID, lineup.getId());

            ContentResolver contentResolver = getContentResolver();
            if (SampleDvbTunerSetupActivity.this.epgInput != null) {
                values.put(
                        EpgContract.EpgInputs.COLUMN_ID,
                        SampleDvbTunerSetupActivity.this.epgInput.getId());
                EpgInputs.update(contentResolver, EpgInput.createEpgChannel(values));
                return null;
            }
            EpgInput epgInput = EpgInputs.queryEpgInput(contentResolver, inputId);
            if (epgInput == null) {
                contentResolver.insert(EpgContract.EpgInputs.CONTENT_URI, values);
            } else {
                values.put(EpgContract.EpgInputs.COLUMN_ID, epgInput.getId());
                EpgInputs.update(contentResolver, EpgInput.createEpgChannel(values));
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            Intent data = new Intent();
            data.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
            data.putExtra(EpgContract.EXTRA_USE_CLOUD_EPG, true);
            setResult(RESULT_OK, data);
            finish();
        }
    }

    private class QueryEpgInputTask extends AsyncTask<Void, Void, EpgInput> {
        private final String inputId;

        QueryEpgInputTask(String inputId) {
            this.inputId = inputId;
        }

        @Override
        protected EpgInput doInBackground(Void... args) {
            ContentResolver contentResolver = getContentResolver();
            return EpgInputs.queryEpgInput(contentResolver, inputId);
        }

        @Override
        protected void onPostExecute(EpgInput result) {
            epgInput = result;
        }
    }
}
