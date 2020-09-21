/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.example.android.tv.channelsprograms.model;

import android.content.Context;

import com.example.android.tv.channelsprograms.R;
import com.example.android.tv.channelsprograms.util.AppLinkHelper;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/** Mocks gathering movies from an external source. */
public final class MockMovieService {

    private static List<Movie> list;
    private static long count = 0;

    /**
     * Creates a list of subscriptions that every users should have.
     *
     * @param context used for accessing shared preferences.
     * @return a list of default subscriptions.
     */
    public static List<Subscription> createUniversalSubscriptions(Context context) {

        String newForYou = context.getString(R.string.new_for_you);
        Subscription flagshipSubscription =
                Subscription.createSubscription(
                        newForYou,
                        context.getString(R.string.new_for_you_description),
                        AppLinkHelper.buildBrowseUri(newForYou).toString(),
                        R.drawable.ic_movie_blue_80dp);

        String trendingVideos = context.getString(R.string.trending_videos);
        Subscription videoSubscription =
                Subscription.createSubscription(
                        trendingVideos,
                        context.getString(R.string.trending_videos_description),
                        AppLinkHelper.buildBrowseUri(trendingVideos).toString(),
                        R.drawable.ic_movie_blue_80dp);

        String featuredFilms = context.getString(R.string.featured_films);
        Subscription filmsSubscription =
                Subscription.createSubscription(
                        featuredFilms,
                        context.getString(R.string.featured_films_description),
                        AppLinkHelper.buildBrowseUri(featuredFilms).toString(),
                        R.drawable.ic_video_library_blue_80dp);

        return Arrays.asList(flagshipSubscription, videoSubscription, filmsSubscription);
    }

    /**
     * Creates and caches a list of movies.
     *
     * @return a list of movies.
     */
    public static List<Movie> getList() {
        if (list == null || list.isEmpty()) {
            list = createMovieList();
        }
        return list;
    }

    /**
     * Shuffles the list of movies to make the returned list appear to be a different list from
     * {@link #getList()}.
     *
     * @return a list of movies in random order.
     */
    public static List<Movie> getFreshList() {
        List<Movie> shuffledMovies = new ArrayList<>(getList());
        Collections.shuffle(shuffledMovies);
        return shuffledMovies;
    }

    private static List<Movie> createMovieList() {
        List<Movie> list = new ArrayList<>();
        String title[] = {
            "Zeitgeist 2010_ Year in Review",
            "Google Demo Slam_ 20ft Search",
            "Introducing Gmail Blue",
            "Introducing Google Fiber to the Pole",
            "Introducing Google Nose"
        };

        String description =
                "Fusce id nisi turpis. Praesent viverra bibendum semper. "
                        + "Donec tristique, orci sed semper lacinia, quam erat rhoncus massa, non congue tellus est "
                        + "quis tellus. Sed mollis orci venenatis quam scelerisque accumsan. Curabitur a massa sit "
                        + "amet mi accumsan mollis sed et magna. Vivamus sed aliquam risus. Nulla eget dolor in elit "
                        + "facilisis mattis. Ut aliquet luctus lacus. Phasellus nec commodo erat. Praesent tempus id "
                        + "lectus ac scelerisque. Maecenas pretium cursus lectus id volutpat.";

        String studio[] = {
            "Studio Zero", "Studio One", "Studio Two", "Studio Three", "Studio Four"
        };
        String videoUrl[] = {
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/Zeitgeist/Zeitgeist%202010_%20Year%20in%20Review.mp4",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/Demo%20Slam/Google%20Demo%20Slam_%2020ft%20Search.mp4",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Gmail%20Blue.mp4",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Google%20Fiber%20to%20the%20Pole.mp4",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Google%20Nose.mp4"
        };
        String bgImageUrl[] = {
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/Zeitgeist/Zeitgeist%202010_%20Year%20in%20Review/bg.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/Demo%20Slam/Google%20Demo%20Slam_%2020ft%20Search/bg.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Gmail%20Blue/bg.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Google%20Fiber%20to%20the%20Pole/bg.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Google%20Nose/bg.jpg",
        };
        String cardImageUrl[] = {
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/Zeitgeist/Zeitgeist%202010_%20Year%20in%20Review/card.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/Demo%20Slam/Google%20Demo%20Slam_%2020ft%20Search/card.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Gmail%20Blue/card.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Google%20Fiber%20to%20the%20Pole/card.jpg",
            "http://commondatastorage.googleapis.com/android-tv/Sample%20videos/April%20Fool's%202013/Introducing%20Google%20Nose/card.jpg"
        };

        for (int index = 0; index < title.length; ++index) {
            list.add(
                    buildMovieInfo(
                            "category",
                            title[index],
                            description,
                            studio[index],
                            videoUrl[index],
                            cardImageUrl[index],
                            bgImageUrl[index]));
        }

        return list;
    }

    private static Movie buildMovieInfo(
            String category,
            String title,
            String description,
            String studio,
            String videoUrl,
            String cardImageUrl,
            String backgroundImageUrl) {
        Movie movie = new Movie();
        movie.setId(count);
        incCount();
        movie.setTitle(title);
        movie.setDescription(description);
        movie.setStudio(studio);
        movie.setCategory(category);
        movie.setCardImageUrl(cardImageUrl);
        movie.setBackgroundImageUrl(backgroundImageUrl);
        movie.setVideoUrl(videoUrl);
        return movie;
    }

    private static void incCount() {
        count++;
    }
}
