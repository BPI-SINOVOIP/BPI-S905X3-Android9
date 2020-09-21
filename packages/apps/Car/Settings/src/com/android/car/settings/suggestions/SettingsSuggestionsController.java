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
 * limitations under the License
 */
package com.android.car.settings.suggestions;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.service.settings.suggestions.Suggestion;
import android.support.annotation.StringRes;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;

import com.android.car.list.TypedPagedListAdapter;
import com.android.car.settings.R;
import com.android.car.settings.common.Logger;
import com.android.settingslib.suggestions.SuggestionController;
import com.android.settingslib.suggestions.SuggestionControllerMixin;
import com.android.settingslib.suggestions.SuggestionLoader;
import com.android.settingslib.utils.IconCache;

import java.util.ArrayList;
import java.util.List;


/**
 * Retrieves suggestions and prepares them for rendering.
 * Modeled after {@link SuggestionControllerMixin}, differs by implementing support library version
 * of LoaderManager and Loader. Does not implement use of LifeCycle.
 */
public class SettingsSuggestionsController implements
        SuggestionController.ServiceConnectionListener,
        LoaderManager.LoaderCallbacks<List<Suggestion>> {
    private static final Logger LOG = new Logger(SettingsSuggestionsController.class);
    private static final ComponentName mComponentName = new ComponentName(
            "com.android.settings.intelligence",
            "com.android.settings.intelligence.suggestions.SuggestionService");
    // These values are hard coded until we receive the OK to plumb them through
    // SettingsIntelligence. This is ok as right now only SUW uses this framework.
    @StringRes
    private static final int PRIMARY_ACTION_ID = R.string.suggestion_primary_button;
    @StringRes
    private static final int SECONDARY_ACTION_ID = R.string.suggestion_secondary_button;

    private final Context mContext;
    private final LoaderManager mLoaderManager;
    private final Listener mListener;
    private final SuggestionController mSuggestionController;
    private final IconCache mIconCache;

    public SettingsSuggestionsController(
            Context context,
            LoaderManager loaderManager,
            @NonNull Listener listener) {
        mContext = context;
        mLoaderManager = loaderManager;
        mListener = listener;
        mIconCache = new IconCache(context);
        mSuggestionController = new SuggestionController(
                mContext,
                mComponentName,
                /* listener= */ this);
    }

    @Override
    public void onServiceConnected() {
        LOG.v("onServiceConnected");
        mLoaderManager.restartLoader(
                SettingsSuggestionsLoader.LOADER_ID_SUGGESTIONS,
                /* args= */ null,
                /* callback= */ this);
    }

    @Override
    public void onServiceDisconnected() {
        LOG.v("onServiceDisconnected");
        cleanupLoader();
    }

    @NonNull
    @Override
    public Loader<List<Suggestion>> onCreateLoader(int id, @Nullable Bundle args) {
        LOG.v("onCreateLoader: " + id);
        if (id == SettingsSuggestionsLoader.LOADER_ID_SUGGESTIONS) {
            return new SettingsSuggestionsLoader(mContext, mSuggestionController);
        }
        throw new IllegalArgumentException("This loader id is not supported " + id);
    }

    @Override
    public void onLoadFinished(
            @NonNull Loader<List<Suggestion>> loader,
            List<Suggestion> suggestionList) {
        LOG.v("onLoadFinished");
        if (suggestionList == null) {
            return;
        }
        ArrayList<TypedPagedListAdapter.LineItem> items = new ArrayList<>();
        for (final Suggestion suggestion : suggestionList) {
            LOG.v("Suggestion ID: " + suggestion.getId());
            Drawable itemIcon = mIconCache.getIcon(suggestion.getIcon());
            SuggestionLineItem suggestionLineItem =
                    new SuggestionLineItem(
                            suggestion.getTitle(),
                            suggestion.getSummary(),
                            itemIcon,
                            mContext.getString(PRIMARY_ACTION_ID),
                            mContext.getString(SECONDARY_ACTION_ID),
                            v -> {
                                try {
                                    suggestion.getPendingIntent().send();
                                    launchSuggestion(suggestion);
                                } catch (PendingIntent.CanceledException e) {
                                    LOG.w("Failed to start suggestion " + suggestion.getTitle());
                                }
                            },
                            adapterPosition -> {
                                dismissSuggestion(suggestion);
                                mListener.onSuggestionDismissed(adapterPosition);

                            });
            items.add(suggestionLineItem);
        }
        mListener.onSuggestionsLoaded(items);
    }

    @Override
    public void onLoaderReset(@NonNull Loader<List<Suggestion>> loader) {
        LOG.v("onLoaderReset");
    }

    /**
     * Start the suggestions controller.
     */
    public void start() {
        LOG.v("Start");
        mSuggestionController.start();
    }

    /**
     * Stop the suggestions controller.
     */
    public void stop() {
        LOG.v("Stop");
        mSuggestionController.stop();
        cleanupLoader();

    }

    private void cleanupLoader() {
        LOG.v("cleanupLoader");
        mLoaderManager.destroyLoader(SuggestionLoader.LOADER_ID_SUGGESTIONS);
    }

    private void dismissSuggestion(Suggestion suggestion) {
        LOG.v("dismissSuggestion");
        mSuggestionController.dismissSuggestions(suggestion);
    }

    private void launchSuggestion(Suggestion suggestion) {
        LOG.v("launchSuggestion");
        mSuggestionController.launchSuggestion(suggestion);
    }

    /**
     * Listener interface to notify of data state changes and actions.
     */
    public interface Listener {
        /**
         * Invoked when deferred setup items have been loaded.
         *
         * @param suggestions List of deferred setup suggestions.
         */
        void onSuggestionsLoaded(@NonNull ArrayList<TypedPagedListAdapter.LineItem> suggestions);

        /***
         * Invoked when a suggestion is dismissed.
         *
         * @param adapterPosition the position of the suggestion item in it's adapter.
         */
        void onSuggestionDismissed(int adapterPosition);
    }
}
