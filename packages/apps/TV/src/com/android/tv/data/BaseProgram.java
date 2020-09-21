/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.data;

import android.content.Context;
import android.media.tv.TvContentRating;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import com.android.tv.R;
import java.util.Comparator;

/**
 * Base class for {@link com.android.tv.data.Program} and {@link
 * com.android.tv.dvr.data.RecordedProgram}.
 */
public abstract class BaseProgram {
    /**
     * Comparator used to compare {@link BaseProgram} according to its season and episodes number.
     * If a program's season or episode number is null, it will be consider "smaller" than programs
     * with season or episode numbers.
     */
    public static final Comparator<BaseProgram> EPISODE_COMPARATOR = new EpisodeComparator(false);

    /**
     * Comparator used to compare {@link BaseProgram} according to its season and episodes number
     * with season numbers in a reversed order. If a program's season or episode number is null, it
     * will be consider "smaller" than programs with season or episode numbers.
     */
    public static final Comparator<BaseProgram> SEASON_REVERSED_EPISODE_COMPARATOR =
            new EpisodeComparator(true);

    private static class EpisodeComparator implements Comparator<BaseProgram> {
        private final boolean mReversedSeason;

        EpisodeComparator(boolean reversedSeason) {
            mReversedSeason = reversedSeason;
        }

        @Override
        public int compare(BaseProgram lhs, BaseProgram rhs) {
            if (lhs == rhs) {
                return 0;
            }
            int seasonNumberCompare = numberCompare(lhs.getSeasonNumber(), rhs.getSeasonNumber());
            if (seasonNumberCompare != 0) {
                return mReversedSeason ? -seasonNumberCompare : seasonNumberCompare;
            } else {
                return numberCompare(lhs.getEpisodeNumber(), rhs.getEpisodeNumber());
            }
        }
    }

    /** Compares two strings represent season numbers or episode numbers of programs. */
    public static int numberCompare(String s1, String s2) {
        if (s1 == s2) {
            return 0;
        } else if (s1 == null) {
            return -1;
        } else if (s2 == null) {
            return 1;
        } else if (s1.equals(s2)) {
            return 0;
        }
        try {
            return Integer.compare(Integer.parseInt(s1), Integer.parseInt(s2));
        } catch (NumberFormatException e) {
            return s1.compareTo(s2);
        }
    }

    /** Returns ID of the program. */
    public abstract long getId();

    /** Returns the title of the program. */
    public abstract String getTitle();

    /** Returns the episode title. */
    public abstract String getEpisodeTitle();

    /** Returns the displayed title of the program episode. */
    public String getEpisodeDisplayTitle(Context context) {
        String episodeNumber = getEpisodeNumber();
        String episodeTitle = getEpisodeTitle();
        if (!TextUtils.isEmpty(episodeNumber)) {
            episodeTitle = episodeTitle == null ? "" : episodeTitle;
            String seasonNumber = getSeasonNumber();
            if (TextUtils.isEmpty(seasonNumber) || TextUtils.equals(seasonNumber, "0")) {
                // Do not show "S0: ".
                return context.getResources()
                        .getString(
                                R.string.display_episode_title_format_no_season_number,
                                episodeNumber,
                                episodeTitle);
            } else {
                return context.getResources()
                        .getString(
                                R.string.display_episode_title_format,
                                seasonNumber,
                                episodeNumber,
                                episodeTitle);
            }
        }
        return episodeTitle;
    }

    /**
     * Returns the content description of the program episode, suitable for being spoken by an
     * accessibility service.
     */
    public String getEpisodeContentDescription(Context context) {
        String episodeNumber = getEpisodeNumber();
        String episodeTitle = getEpisodeTitle();
        if (!TextUtils.isEmpty(episodeNumber)) {
            episodeTitle = episodeTitle == null ? "" : episodeTitle;
            String seasonNumber = getSeasonNumber();
            if (TextUtils.isEmpty(seasonNumber) || TextUtils.equals(seasonNumber, "0")) {
                // Do not list season if it is empty or 0
                return context.getResources()
                        .getString(
                                R.string.content_description_episode_format_no_season_number,
                                episodeNumber,
                                episodeTitle);
            } else {
                return context.getResources()
                        .getString(
                                R.string.content_description_episode_format,
                                seasonNumber,
                                episodeNumber,
                                episodeTitle);
            }
        }
        return episodeTitle;
    }

    /** Returns the description of the program. */
    public abstract String getDescription();

    /** Returns the long description of the program. */
    public abstract String getLongDescription();

    /** Returns the start time of the program in Milliseconds. */
    public abstract long getStartTimeUtcMillis();

    /** Returns the end time of the program in Milliseconds. */
    public abstract long getEndTimeUtcMillis();

    /** Returns the duration of the program in Milliseconds. */
    public abstract long getDurationMillis();

    /** Returns the series ID. */
    public abstract String getSeriesId();

    /** Returns the season number. */
    public abstract String getSeasonNumber();

    /** Returns the episode number. */
    public abstract String getEpisodeNumber();

    /** Returns URI of the program's poster. */
    public abstract String getPosterArtUri();

    /** Returns URI of the program's thumbnail. */
    public abstract String getThumbnailUri();

    /** Returns the array of the ID's of the canonical genres. */
    public abstract int[] getCanonicalGenreIds();

    /** Returns the array of content ratings. */
    @Nullable
    public abstract TvContentRating[] getContentRatings();

    /** Returns channel's ID of the program. */
    public abstract long getChannelId();

    /** Returns if the program is valid. */
    public abstract boolean isValid();

    /** Checks whether the program is episodic or not. */
    public boolean isEpisodic() {
        return getSeriesId() != null;
    }

    /** Generates the series ID for the other inputs than the tuner TV input. */
    public static String generateSeriesId(String packageName, String title) {
        return packageName + "/" + title;
    }
}
