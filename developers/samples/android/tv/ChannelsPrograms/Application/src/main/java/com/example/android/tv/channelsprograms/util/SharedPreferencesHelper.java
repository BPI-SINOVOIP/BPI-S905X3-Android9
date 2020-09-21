/*
 * Copyright (c) 2017 Google Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package com.example.android.tv.channelsprograms.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import com.example.android.tv.channelsprograms.model.Movie;
import com.example.android.tv.channelsprograms.model.Subscription;
import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * Helper class to store {@link Subscription}s and {@link Movie}s in {@link SharedPreferences}.
 *
 * <p>SharedPreferencesHelper provides static methods to set and get these objects.
 *
 * <p>The methods of this class should not be called on the UI thread. Marshalling an object into
 * JSON can be expensive for large objects.
 */
public final class SharedPreferencesHelper {

    private static final String TAG = "SharedPreferencesHelper";

    private static final String PREFS_NAME = "com.example.android.tv.recommendations";
    private static final String PREFS_SUBSCRIPTIONS_KEY =
            "com.example.android.tv.recommendations.prefs.SUBSCRIPTIONS";
    private static final String PREFS_SUBSCRIBED_MOVIES_PREFIX =
            "com.example.android.tv.recommendations.prefs.SUBSCRIBED_MOVIES_";

    private static final Gson mGson = new Gson();

    /**
     * Reads the {@link List <Subscription>} from {@link SharedPreferences}.
     *
     * @param context used for getting an instance of shared preferences.
     * @return a list of subscriptions or an empty list if none exist.
     */
    public static List<Subscription> readSubscriptions(Context context) {
        return getList(context, Subscription.class, PREFS_SUBSCRIPTIONS_KEY);
    }

    /**
     * Overrides the subscriptions stored in {@link SharedPreferences}.
     *
     * @param context used for getting an instance of shared preferences.
     * @param subscriptions to be stored in shared preferences.
     */
    public static void storeSubscriptions(Context context, List<Subscription> subscriptions) {
        setList(context, subscriptions, PREFS_SUBSCRIPTIONS_KEY);
    }

    /**
     * Reads the {@link List <Movie>} from {@link SharedPreferences} for a given channel.
     *
     * @param context used for getting an instance of shared preferences.
     * @param channelId of the channel that the movies are associated with.
     * @return a list of movies or an empty list if none exist.
     */
    public static List<Movie> readMovies(Context context, long channelId) {
        return getList(context, Movie.class, PREFS_SUBSCRIBED_MOVIES_PREFIX + channelId);
    }

    /**
     * Overrides the movies stored in {@link SharedPreferences} for the associated channel id.
     *
     * @param context used for getting an instance of shared preferences.
     * @param channelId of the channel that the movies are associated with.
     * @param movies to be stored.
     */
    public static void storeMovies(Context context, long channelId, List<Movie> movies) {
        setList(context, movies, PREFS_SUBSCRIBED_MOVIES_PREFIX + channelId);
    }

    /**
     * Retrieves a set of Strings from {@link SharedPreferences} and returns as a List.
     *
     * @param context used for getting an instance of shared preferences.
     * @param clazz the class that the strings will be unmarshalled into.
     * @param key the key in shared preferences to access the string set.
     * @param <T> the type of object that will be in the returned list, should be the same as the
     *     clazz that was supplied.
     * @return a list of <T> objects that were stored in shared preferences or an empty list if no
     *     objects exists.
     */
    private static <T> List<T> getList(Context context, Class<T> clazz, String key) {
        SharedPreferences sharedPreferences =
                context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        Set<String> stringSet = sharedPreferences.getStringSet(key, new HashSet<String>());
        if (stringSet.isEmpty()) {
            // Favoring mutability of the list over Collections.emptyList().
            return new ArrayList<>();
        }
        List<T> list = new ArrayList<>(stringSet.size());
        try {
            for (String contactString : stringSet) {
                list.add(mGson.fromJson(contactString, clazz));
            }
        } catch (JsonSyntaxException e) {
            Log.e(TAG, "Could not parse json.", e);
            return Collections.emptyList();
        }
        return list;
    }

    /**
     * Saves a list of Strings into {@link SharedPreferences}.
     *
     * @param context used for getting an instance of shared preferences.
     * @param list of <T> object that need to be persisted.
     * @param key the key in shared preferences which the string set will be stored.
     * @param <T> type the of object we will be marshalling and persisting.
     */
    private static <T> void setList(Context context, List<T> list, String key) {
        SharedPreferences sharedPreferences =
                context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPreferences.edit();

        Set<String> strings = new LinkedHashSet<>(list.size());
        for (T item : list) {
            strings.add(mGson.toJson(item));
        }
        editor.putStringSet(key, strings);
        editor.apply();
    }
}
