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

package com.android.settings.intelligence.suggestions.eligibility;

import static android.content.Intent.EXTRA_COMPONENT_NAME;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;

import com.android.settings.intelligence.R;
import com.android.settings.intelligence.suggestions.model.CandidateSuggestion;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Filters candidate list to only valid ones.
 */
public class CandidateSuggestionFilter {

    private static final String TAG = "CandidateSuggestionFilter";

    private static CandidateSuggestionFilter sChecker;
    private static ExecutorService sExecutorService;

    public static CandidateSuggestionFilter getInstance() {
        if (sChecker == null) {
            sChecker = new CandidateSuggestionFilter();
            sExecutorService = Executors.newCachedThreadPool();
        }
        return sChecker;
    }

    @NonNull
    public synchronized List<CandidateSuggestion> filterCandidates(Context context,
            List<CandidateSuggestion> candidates) {
        final long startTime = System.currentTimeMillis();
        final List<CandidateFilterTask> checkTasks = new ArrayList<>();
        final List<CandidateSuggestion> incompleteCandidates = new ArrayList<>();
        if (candidates == null) {
            return incompleteCandidates;
        }
        // Put a check task into ExecutorService for each candidate.
        for (CandidateSuggestion candidate : candidates) {
            final CandidateFilterTask task = new CandidateFilterTask(context, candidate);
            sExecutorService.execute(task);
            checkTasks.add(task);
        }
        for (CandidateFilterTask task : checkTasks) {
            try {
                long checkTaskTimeOutValue =
                        context.getResources().getInteger(R.integer.check_task_timeout_ms);
                final CandidateSuggestion candidate = task.get(checkTaskTimeOutValue,
                        TimeUnit.MILLISECONDS);
                if (candidate != null) {
                    incompleteCandidates.add(candidate);
                }
            } catch (TimeoutException | InterruptedException | ExecutionException e) {
                Log.w(TAG, "Error checking completion state for " + task.getId());
            }
        }
        final long endTime = System.currentTimeMillis();
        Log.d(TAG, "filterCandidates duration: " + (endTime - startTime));
        return incompleteCandidates;
    }

    /**
     * {@link FutureTask} that filters status for a suggestion candidate.
     * <p/>
     * If the candidate status is valid, {@link #get()} will return the candidate itself.
     * Otherwise it returns null.
     */
    static class CandidateFilterTask extends FutureTask<CandidateSuggestion> {

        private static final String EXTRA_CANDIDATE_ID = "candidate_id";
        private static final String RESULT_IS_COMPLETE = "candidate_is_complete";

        private final String mId;

        public CandidateFilterTask(Context context, CandidateSuggestion candidate) {
            super(new GetSuggestionStatusCallable(context, candidate));
            mId = candidate.getId();
        }

        public String getId() {
            return mId;
        }

        @VisibleForTesting
        static class GetSuggestionStatusCallable implements Callable<CandidateSuggestion> {
            @VisibleForTesting
            static final String CONTENT_PROVIDER_INTENT_ACTION =
                    "com.android.settings.action.SUGGESTION_STATE_PROVIDER";
            private static final String METHOD_GET_SUGGESTION_STATE = "getSuggestionState";

            private final Context mContext;
            private final CandidateSuggestion mCandidate;

            public GetSuggestionStatusCallable(Context context, CandidateSuggestion candidate) {
                mContext = context.getApplicationContext();
                mCandidate = candidate;
            }

            @Override
            public CandidateSuggestion call() throws Exception {
                // First find if candidate has any state provider.
                final String packageName = mCandidate.getComponent().getPackageName();
                final Intent probe = new Intent(CONTENT_PROVIDER_INTENT_ACTION)
                        .setPackage(packageName);
                final List<ResolveInfo> providers = mContext.getPackageManager()
                        .queryIntentContentProviders(probe, 0 /* flags */);
                if (providers == null || providers.isEmpty()) {
                    // No provider, let it go through
                    return mCandidate;
                }
                final ProviderInfo providerInfo = providers.get(0).providerInfo;
                if (providerInfo == null || TextUtils.isEmpty(providerInfo.authority)) {
                    // Bad provider - don't let candidate pass through.
                    return null;
                }
                // Query candidate state (isComplete)
                final Uri uri = new Uri.Builder()
                        .scheme(ContentResolver.SCHEME_CONTENT)
                        .authority(providerInfo.authority)
                        .build();
                final Bundle result = mContext.getContentResolver().call(
                        uri, METHOD_GET_SUGGESTION_STATE, null /* args */,
                        buildGetSuggestionStateExtras(mCandidate));
                final boolean isComplete = result.getBoolean(RESULT_IS_COMPLETE, false);
                Log.d(TAG, "Suggestion state result " + result);
                return isComplete ? null : mCandidate;
            }

            @VisibleForTesting
            static Bundle buildGetSuggestionStateExtras(CandidateSuggestion candidate) {
                final Bundle args = new Bundle();
                final String id = candidate.getId();
                args.putString(EXTRA_CANDIDATE_ID, id);
                args.putParcelable(EXTRA_COMPONENT_NAME, candidate.getComponent());
                return args;
            }
        }
    }
}
