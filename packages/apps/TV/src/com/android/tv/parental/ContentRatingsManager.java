/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.parental;

import android.content.Context;
import android.media.tv.TvContentRating;
import android.media.tv.TvContentRatingSystemInfo;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.os.Message;
import android.os.Handler;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.parental.ContentRatingSystem.Rating;
import com.android.tv.parental.ContentRatingSystem.SubRating;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.parental.ContentRatingSystem.Order;

import com.droidlogic.app.tv.DroidContentRatingsParser;
import com.droidlogic.app.tv.DroidContentRatingsParser.ContentRatingSystemT;
import com.droidlogic.app.tv.DroidContentRatingsParser.RatingDefinition;

import java.util.ArrayList;
import java.util.List;

public class ContentRatingsManager {
    private final List<ContentRatingSystem> mContentRatingSystems = new ArrayList<>();
    private static String TAG = "ContentRatingsManager";
    private final Context mContext;
    private final TvInputManagerHelper.TvInputManagerInterface mTvInputManager;

     //add new definition
    public static String RATING_NO = "NoRating";
    //add for supporting rrt5
    private final List<ContentRatingSystem> mRRT5ContentRatingSystems = new ArrayList<>();
    private static final boolean mIsSupportRRT5 = true;
    public static  int EVENT_RRT_SCAN_START          = 1;
    public static  int EVENT_RRT_SCAN_END            = 3;
    private int mRRT5UpdateResult = EVENT_RRT_SCAN_END;
    private static final int MSG_GET_RRT5_RATING_LIST = 100;

    public ContentRatingsManager(
            Context context, TvInputManagerHelper.TvInputManagerInterface tvInputManager) {
        mContext = context;
        this.mTvInputManager = tvInputManager;
    }

    public void update() {
        mContentRatingSystems.clear();
        ContentRatingsParser parser = new ContentRatingsParser(mContext);

        List<TvContentRatingSystemInfo> infos = mTvInputManager.getTvContentRatingSystemList();
        for (TvContentRatingSystemInfo info : infos) {
            List<ContentRatingSystem> list = parser.parse(info);
            if (list != null) {
                mContentRatingSystems.addAll(list);
            }
        }
        if (isSupportRRT5()) {
               Message msg = mMsgHandler.obtainMessage(MSG_GET_RRT5_RATING_LIST, (isRRT5updateSuccess() ? EVENT_RRT_SCAN_END : EVENT_RRT_SCAN_START), 0);
               mMsgHandler.sendMessage(msg);
        }
    }

    /** Returns the content rating system with the give ID. */
    @Nullable
    public ContentRatingSystem getContentRatingSystem(String contentRatingSystemId) {
        for (ContentRatingSystem ratingSystem : mContentRatingSystems) {
            if (TextUtils.equals(ratingSystem.getId(), contentRatingSystemId)) {
                return ratingSystem;
            }
        }
        return null;
    }

    /** Returns a new list of all content rating systems defined. */
    public List<ContentRatingSystem> getContentRatingSystems() {
        return new ArrayList<>(mContentRatingSystems);
    }

    /**
     * Returns the long name of a given content rating including descriptors (sub-ratings) that is
     * displayed to the user. For example, "TV-PG (L, S)".
     */

    public String getDisplayNameForRating(TvContentRating canonicalRating) {
        if (canonicalRating == null) {
            return null;
        }
        if (!DroidContentRatingsParser.DOMAIN_RRT_RATINGS.equals(canonicalRating.getDomain())) {
            return getDisplayNameForVchipRating(canonicalRating);
        } else {
            return getDisplayNameForRrt5Rating(canonicalRating);
        }
    }

    public String getDisplayNameForVchipRating(TvContentRating canonicalRating) {
        Rating rating = getRating(canonicalRating);
        if (rating == null) {
            return null;
        }
        List<SubRating> subRatings = getSubRatings(rating, canonicalRating);
        if (!subRatings.isEmpty()) {
            StringBuilder builder = new StringBuilder();
            for (SubRating subRating : subRatings) {
                builder.append(subRating.getTitle());
                builder.append(", ");
            }
            return rating.getTitle() + " (" + builder.substring(0, builder.length() - 2) + ")";
        }
        return rating.getTitle();
    }

    public String getDisplayNameForRrt5Rating(TvContentRating canonicalRating) {
        if (!(canonicalRating != null && DroidContentRatingsParser.DOMAIN_RRT_RATINGS.equals(canonicalRating.getDomain()))) {
            return null;
        }
        List<String> subRatings = canonicalRating.getSubRatings();
        StringBuilder builder = new StringBuilder();
        builder.append(canonicalRating.getMainRating());
        if (subRatings != null && !subRatings.isEmpty()) {
            for (String subRating : subRatings) {
                builder.append("/");//to show  main ratings like "N/A"
                builder.append(subRating);
            }
        }
        return builder.toString();
    }

    private Rating getRating(TvContentRating canonicalRating) {
        if (canonicalRating == null || mContentRatingSystems == null) {
            return null;
        }
        for (ContentRatingSystem system : mContentRatingSystems) {
            if (system.getDomain().equals(canonicalRating.getDomain())
                    && system.getName().equals(canonicalRating.getRatingSystem())) {
                for (Rating rating : system.getRatings()) {
                    if (rating.getName().equals(canonicalRating.getMainRating())) {
                        return rating;
                    }
                }
            }
        }
        return null;
    }

    private List<SubRating> getSubRatings(Rating rating, TvContentRating canonicalRating) {
        List<SubRating> subRatings = new ArrayList<>();
        if (rating == null
                || rating.getSubRatings() == null
                || canonicalRating == null
                || canonicalRating.getSubRatings() == null) {
            return subRatings;
        }
        for (String subRatingString : canonicalRating.getSubRatings()) {
            for (SubRating subRating : rating.getSubRatings()) {
                if (subRating.getName().equals(subRatingString)) {
                    subRatings.add(subRating);
                    break;
                }
            }
        }
        return subRatings;
    }

    /**************************start: add for RRT5 ************************************/
    public List<ContentRatingSystem> getRRT5ContentRatingSystems() {
        return new ArrayList<>(mRRT5ContentRatingSystems);
    }

    public boolean isSupportRRT5() {
        return mIsSupportRRT5;
    }
    public boolean isRRT5UpdateFinish() {
        //Log.w(TAG, "isRRT5UpdateFinish:"+mRRT5UpdateResult);
        if (mRRT5UpdateResult != EVENT_RRT_SCAN_END )
            return false;
        else
            return true;
    }
    public void setRRT5updateResult(int result) {
        //Log.w(TAG, "setRRT5updateResult:"+result);
        mRRT5UpdateResult = result;
        if (isRRT5updateSuccess()) {
            updateRRT5();
        }
    }

    public boolean isRRT5updateSuccess() {
        if (mRRT5UpdateResult ==EVENT_RRT_SCAN_END)
            return true;
        else
            return false;
    }
    private final Handler mMsgHandler = new Handler () {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_GET_RRT5_RATING_LIST:
                    if (msg.arg1 == EVENT_RRT_SCAN_START) {
                        Log.d(TAG, "handleMessage return as new update running!");
                        return;
                    }
                    List<ContentRatingSystem> list_custom = new ArrayList<>();
                    DroidContentRatingsParser parser_tv = new DroidContentRatingsParser();
                    List<ContentRatingSystemT> ratingSystems = parser_tv.load_t();
                    if (ratingSystems != null && ratingSystems.size() > 0) {
                    for (ContentRatingSystemT rating_system : ratingSystems) {
                        ContentRatingSystem.Builder builder = new ContentRatingSystem.Builder(mContext);
                        if (rating_system.getRegion() <= DroidContentRatingsParser.FIXED_REGION_lEVEL_2) {
                            continue;//not show fixed region
                        }
                        builder.setName(rating_system.getName());
                        builder.addCountry(rating_system.getCountry());
                        List <RatingDefinition> ratingDefinitions = rating_system.getRatingDefinitions();
                        Order.Builder order_builder = new Order.Builder();
                        //Log.w(TAG, "ratingDefinitions size:"+ ratingDefinitions.size());
                        Log.w(TAG, "rating system name :"+ rating_system.getName());
                        for (RatingDefinition rating_definition : ratingDefinitions) {
                            Rating.Builder rating_builder = new Rating.Builder();
                            rating_builder.setName(rating_definition.getTitle());
                            rating_builder.setTitle(rating_definition.getTitle());
                            rating_builder.setDescription(rating_definition.getDescription());
                            rating_builder.setContentAgeHint(18);
                            builder.addRatingBuilder(rating_builder);
                            order_builder.addRatingName(rating_definition.getTitle());
                            Log.w(TAG, "rating definition title :"+ rating_definition.getTitle());
                        }
                        builder.addOrderBuilder(order_builder);
                        //builder.setDomain(ContentRatingsParser.DOMAIN_SYSTEM_RATINGS);
                        builder.setDomain(DroidContentRatingsParser.DOMAIN_RRT_RATINGS);
                        //Log.w(TAG, "list_custom add:"+ rating_system.toString());

                        list_custom.add(builder.build());
                    }
                    //Log.w(TAG, "list_custom size:"+ list_custom.size());
                    mRRT5ContentRatingSystems.clear();
                    if (list_custom != null) {
                        mRRT5ContentRatingSystems.addAll(list_custom);
                    }
                    Log.w(TAG, "rrt5 list update finish");
                }
                break;
            }
        }
    };

    public void updateRRT5() {
        if (isSupportRRT5()) {
               Log.w(TAG, "updateRRT5 xml");
               Message msg = mMsgHandler.obtainMessage(MSG_GET_RRT5_RATING_LIST, (isRRT5updateSuccess() ? EVENT_RRT_SCAN_END : EVENT_RRT_SCAN_START), 0);
               mMsgHandler.sendMessage(msg);
        }
    }
    /**************************end: add for RRT5 ************************************/
}
