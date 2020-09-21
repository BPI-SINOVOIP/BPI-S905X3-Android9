/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.media;

import static androidx.annotation.RestrictTo.Scope.LIBRARY_GROUP;

import android.os.Bundle;
import android.util.Log;

import androidx.annotation.IntDef;
import androidx.annotation.Nullable;
import androidx.annotation.RestrictTo;
import androidx.core.util.ObjectsCompat;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * A class to encapsulate rating information used as content metadata.
 * A rating is defined by its rating style (see {@link #RATING_HEART},
 * {@link #RATING_THUMB_UP_DOWN}, {@link #RATING_3_STARS}, {@link #RATING_4_STARS},
 * {@link #RATING_5_STARS} or {@link #RATING_PERCENTAGE}) and the actual rating value (which may
 * be defined as "unrated"), both of which are defined when the rating instance is constructed
 * through one of the factory methods.
 */
// New version of Rating with following change
//   - Don't implement Parcelable for updatable support.
public final class Rating2 {
    /**
     * @hide
     */
    @RestrictTo(LIBRARY_GROUP)
    @IntDef({RATING_NONE, RATING_HEART, RATING_THUMB_UP_DOWN, RATING_3_STARS, RATING_4_STARS,
            RATING_5_STARS, RATING_PERCENTAGE})
    @Retention(RetentionPolicy.SOURCE)
    public @interface Style {}

    /**
     * @hide
     */
    @RestrictTo(LIBRARY_GROUP)
    @IntDef({RATING_3_STARS, RATING_4_STARS, RATING_5_STARS})
    @Retention(RetentionPolicy.SOURCE)
    public @interface StarStyle {}

    /**
     * Indicates a rating style is not supported. A Rating2 will never have this
     * type, but can be used by other classes to indicate they do not support
     * Rating2.
     */
    public static final int RATING_NONE = 0;

    /**
     * A rating style with a single degree of rating, "heart" vs "no heart". Can be used to
     * indicate the content referred to is a favorite (or not).
     */
    public static final int RATING_HEART = 1;

    /**
     * A rating style for "thumb up" vs "thumb down".
     */
    public static final int RATING_THUMB_UP_DOWN = 2;

    /**
     * A rating style with 0 to 3 stars.
     */
    public static final int RATING_3_STARS = 3;

    /**
     * A rating style with 0 to 4 stars.
     */
    public static final int RATING_4_STARS = 4;

    /**
     * A rating style with 0 to 5 stars.
     */
    public static final int RATING_5_STARS = 5;

    /**
     * A rating style expressed as a percentage.
     */
    public static final int RATING_PERCENTAGE = 6;

    private static final String TAG = "Rating2";

    private static final float RATING_NOT_RATED = -1.0f;
    private static final String KEY_STYLE = "android.media.rating2.style";
    private static final String KEY_VALUE = "android.media.rating2.value";

    private final int mRatingStyle;
    private final float mRatingValue;

    private Rating2(@Style int ratingStyle, float rating) {
        mRatingStyle = ratingStyle;
        mRatingValue = rating;
    }

    @Override
    public String toString() {
        return "Rating2:style=" + mRatingStyle + " rating="
                + (mRatingValue < 0.0f ? "unrated" : String.valueOf(mRatingValue));
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof Rating2)) {
            return false;
        }
        Rating2 other = (Rating2) obj;
        return mRatingStyle == other.mRatingStyle && mRatingValue == other.mRatingValue;
    }

    @Override
    public int hashCode() {
        return ObjectsCompat.hash(mRatingStyle, mRatingValue);
    }

    /**
     * Create an instance from bundle object, previously created by {@link #toBundle()}
     *
     * @param bundle bundle
     * @return new Rating2 instance or {@code null} for error
     */
    public static Rating2 fromBundle(@Nullable Bundle bundle) {
        if (bundle == null) {
            return null;
        }
        return new Rating2(bundle.getInt(KEY_STYLE), bundle.getFloat(KEY_VALUE));
    }

    /**
     * Return bundle for this object to share across the process.
     * @return bundle of this object
     */
    public Bundle toBundle() {
        Bundle bundle = new Bundle();
        bundle.putInt(KEY_STYLE, mRatingStyle);
        bundle.putFloat(KEY_VALUE, mRatingValue);
        return bundle;
    }

    /**
     * Return a Rating2 instance with no rating.
     * Create and return a new Rating2 instance with no rating known for the given
     * rating style.
     *
     * @param ratingStyle one of {@link #RATING_HEART}, {@link #RATING_THUMB_UP_DOWN},
     *    {@link #RATING_3_STARS}, {@link #RATING_4_STARS}, {@link #RATING_5_STARS},
     *    or {@link #RATING_PERCENTAGE}.
     * @return null if an invalid rating style is passed, a new Rating2 instance otherwise.
     */
    public static @Nullable Rating2 newUnratedRating(@Style int ratingStyle) {
        switch(ratingStyle) {
            case RATING_HEART:
            case RATING_THUMB_UP_DOWN:
            case RATING_3_STARS:
            case RATING_4_STARS:
            case RATING_5_STARS:
            case RATING_PERCENTAGE:
                return new Rating2(ratingStyle, RATING_NOT_RATED);
            default:
                return null;
        }
    }

    /**
     * Return a Rating2 instance with a heart-based rating.
     * Create and return a new Rating2 instance with a rating style of {@link #RATING_HEART},
     * and a heart-based rating.
     *
     * @param hasHeart true for a "heart selected" rating, false for "heart unselected".
     * @return a new Rating2 instance.
     */
    public static @Nullable Rating2 newHeartRating(boolean hasHeart) {
        return new Rating2(RATING_HEART, hasHeart ? 1.0f : 0.0f);
    }

    /**
     * Return a Rating2 instance with a thumb-based rating.
     * Create and return a new Rating2 instance with a {@link #RATING_THUMB_UP_DOWN}
     * rating style, and a "thumb up" or "thumb down" rating.
     *
     * @param thumbIsUp true for a "thumb up" rating, false for "thumb down".
     * @return a new Rating2 instance.
     */
    public static @Nullable Rating2 newThumbRating(boolean thumbIsUp) {
        return new Rating2(RATING_THUMB_UP_DOWN, thumbIsUp ? 1.0f : 0.0f);
    }

    /**
     * Return a Rating2 instance with a star-based rating.
     * Create and return a new Rating2 instance with one of the star-base rating styles
     * and the given integer or fractional number of stars. Non integer values can for instance
     * be used to represent an average rating value, which might not be an integer number of stars.
     *
     * @param starRatingStyle one of {@link #RATING_3_STARS}, {@link #RATING_4_STARS},
     *     {@link #RATING_5_STARS}.
     * @param starRating a number ranging from 0.0f to 3.0f, 4.0f or 5.0f according to
     *     the rating style.
     * @return null if the rating style is invalid, or the rating is out of range,
     *     a new Rating2 instance otherwise.
     */
    public static @Nullable Rating2 newStarRating(@StarStyle int starRatingStyle,
            float starRating) {
        float maxRating;
        switch(starRatingStyle) {
            case RATING_3_STARS:
                maxRating = 3.0f;
                break;
            case RATING_4_STARS:
                maxRating = 4.0f;
                break;
            case RATING_5_STARS:
                maxRating = 5.0f;
                break;
            default:
                Log.e(TAG, "Invalid rating style (" + starRatingStyle + ") for a star rating");
                return null;
        }
        if ((starRating < 0.0f) || (starRating > maxRating)) {
            Log.e(TAG, "Trying to set out of range star-based rating");
            return null;
        }
        return new Rating2(starRatingStyle, starRating);
    }

    /**
     * Return a Rating2 instance with a percentage-based rating.
     * Create and return a new Rating2 instance with a {@link #RATING_PERCENTAGE}
     * rating style, and a rating of the given percentage.
     *
     * @param percent the value of the rating
     * @return null if the rating is out of range, a new Rating2 instance otherwise.
     */
    public static @Nullable Rating2 newPercentageRating(float percent) {
        if ((percent < 0.0f) || (percent > 100.0f)) {
            Log.e(TAG, "Invalid percentage-based rating value");
            return null;
        } else {
            return new Rating2(RATING_PERCENTAGE, percent);
        }
    }

    /**
     * Return whether there is a rating value available.
     * @return true if the instance was not created with {@link #newUnratedRating(int)}.
     */
    public boolean isRated() {
        return mRatingValue >= 0.0f;
    }

    /**
     * Return the rating style.
     * @return one of {@link #RATING_HEART}, {@link #RATING_THUMB_UP_DOWN},
     *    {@link #RATING_3_STARS}, {@link #RATING_4_STARS}, {@link #RATING_5_STARS},
     *    or {@link #RATING_PERCENTAGE}.
     */
    public @Style int getRatingStyle() {
        return mRatingStyle;
    }

    /**
     * Return whether the rating is "heart selected".
     * @return true if the rating is "heart selected", false if the rating is "heart unselected",
     *    if the rating style is not {@link #RATING_HEART} or if it is unrated.
     */
    public boolean hasHeart() {
        return mRatingStyle == RATING_HEART && mRatingValue == 1.0f;
    }

    /**
     * Return whether the rating is "thumb up".
     * @return true if the rating is "thumb up", false if the rating is "thumb down",
     *    if the rating style is not {@link #RATING_THUMB_UP_DOWN} or if it is unrated.
     */
    public boolean isThumbUp() {
        return mRatingStyle == RATING_THUMB_UP_DOWN && mRatingValue == 1.0f;
    }

    /**
     * Return the star-based rating value.
     * @return a rating value greater or equal to 0.0f, or a negative value if the rating style is
     *    not star-based, or if it is unrated.
     */
    public float getStarRating() {
        switch (mRatingStyle) {
            case RATING_3_STARS:
            case RATING_4_STARS:
            case RATING_5_STARS:
                if (isRated()) {
                    return mRatingValue;
                }
            // Fall through
            default:
                return -1.0f;
        }
    }

    /**
     * Return the percentage-based rating value.
     * @return a rating value greater or equal to 0.0f, or a negative value if the rating style is
     *    not percentage-based, or if it is unrated.
     */
    public float getPercentRating() {
        if ((mRatingStyle != RATING_PERCENTAGE) || !isRated()) {
            return -1.0f;
        } else {
            return mRatingValue;
        }
    }
}
