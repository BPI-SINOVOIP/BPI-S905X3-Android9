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

package com.android.tv.droidlogic.rrt5;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.ArrayMap;
import android.util.SparseIntArray;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.ImageView;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.dialog.WebDialogFragment;
import com.android.tv.license.LicenseUtils;
import com.android.tv.parental.ContentRatingSystem;
import com.android.tv.parental.ContentRatingSystem.Rating;
import com.android.tv.parental.ParentalControlSettings;
import com.android.tv.ui.sidepanel.CheckBoxItem;
import com.android.tv.ui.sidepanel.DividerItem;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.util.TvSettings;
import com.android.tv.util.TvSettings.ContentRatingLevel;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import android.util.Log;
import com.android.tv.ui.sidepanel.ActionItem;

public class DroidRatingsFragment extends SideFragment {

    private static final String TRACKER_LABEL = "DroidRatingsFragment";
    private static final String TAG = "DroidRatingsFragment";
    private int mItemsSize;


    private final Map<String, List<RatingItem>> mContentRatingSystemItemMap = new ArrayMap<>();
    private ParentalControlSettings mParentalControlSettings;

    public static String getDescription(MainActivity tvActivity) {
            return tvActivity.getString(R.string.option_rrt5_description);
    }

    @Override
    protected String getTitle() {
        return getString(R.string.option_rrt5);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();

        items.add(new ActionItem(getString(R.string.rrt5_update_title),
            getString(R.string.rrt5_update_title_description)) {
                @Override
                protected void onSelected() {
                    getMainActivity().mQuickKeyInfo.updateRRT5XmlResource();
                }
            });

        mContentRatingSystemItemMap.clear();

        List<ContentRatingSystem> contentRatingSystems =
                getMainActivity().getContentRatingsManager().getRRT5ContentRatingSystems();
        Collections.sort(contentRatingSystems, ContentRatingSystem.DISPLAY_NAME_COMPARATOR);

        for (ContentRatingSystem s : contentRatingSystems) {
                List<RatingItem> ratingItems = new ArrayList<>();
                //boolean hasSubRating = false;
                Log.d(TAG,"add contentRatingSystem: "+ s.getDisplayName());
                items.add(new DividerItem(s.getDisplayName()));
                for (Rating rating : s.getRatings()) {
                    RatingItem item = new RatingItem(s, rating) ;
                    items.add(item);
                    ratingItems.add(item);
                    Log.d(TAG, "add rating = " + rating.getTitle());
                }
                mContentRatingSystemItemMap.put(s.getId(), ratingItems);

        }

        mItemsSize = items.size();

        return items;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mParentalControlSettings = getMainActivity().getParentalControlSettings();
        mParentalControlSettings.loadRatings();
    }

    @Override
    public void onResume() {
        super.onResume();

        if (getSelectedPosition() >= mItemsSize) {
            setSelectedPosition(mItemsSize - 1);
        }
    }

    private void updateDependentRatingItems(ContentRatingSystem.Order order,
            int selectedRatingOrderIndex, String contentRatingSystemId, boolean isChecked) {
        Log.d(TAG,"updateDependentRatingItems:"+contentRatingSystemId);
        List<RatingItem> ratingItems = mContentRatingSystemItemMap.get(contentRatingSystemId);
        if (ratingItems != null) {
            for (RatingItem item : ratingItems) {
                int ratingOrderIndex = item.getRatingOrderIndex(order);
                //Log.d(TAG,"ratingOrderIndex:"+ratingOrderIndex);
                if (ratingOrderIndex != -1
                        && ((ratingOrderIndex > selectedRatingOrderIndex && isChecked)
                        || (ratingOrderIndex < selectedRatingOrderIndex && !isChecked))) {
                    item.setRatingBlocked(isChecked);
                }
            }
        }
    }

    //private class RatingLevelItem extends RadioButtonItem

    private class RatingItem extends CheckBoxItem {
        protected final ContentRatingSystem mContentRatingSystem;
        protected final Rating mRating;
        private final Drawable mIcon;
        private CompoundButton mCompoundButton;
        private final List<ContentRatingSystem.Order> mOrders = new ArrayList<>();
        private final List<Integer> mOrderIndexes = new ArrayList<>();

        private RatingItem(ContentRatingSystem contentRatingSystem, Rating rating) {
            super(rating.getTitle(), rating.getDescription());
            mContentRatingSystem = contentRatingSystem;
            mRating = rating;
            mIcon = rating.getIcon();
            for (ContentRatingSystem.Order order : mContentRatingSystem.getOrders()) {
                int orderIndex = order.getRatingIndex(mRating);
                if (orderIndex != -1) {
                    mOrders.add(order);
                    mOrderIndexes.add(orderIndex);
                }
            }
        }

        @Override
        protected void onBind(View view) {
            super.onBind(view);

            mCompoundButton = (CompoundButton) view.findViewById(getCompoundButtonId());
            mCompoundButton.setVisibility(View.VISIBLE);

            ImageView imageView = (ImageView) view.findViewById(R.id.icon);
            if (mIcon != null) {
                imageView.setVisibility(View.VISIBLE);
                imageView.setImageDrawable(mIcon);
            } else {
                imageView.setVisibility(View.GONE);
            }
        }

        @Override
        protected void onUnbind() {
            super.onUnbind();
            mCompoundButton = null;
        }

        @Override
        protected void onUpdate() {
            super.onUpdate();
            mCompoundButton.setButtonDrawable(getButtonDrawable());
            setChecked(mParentalControlSettings.isRatingBlocked(mContentRatingSystem, mRating));
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            mParentalControlSettings.setRatingBlocked(
                    mContentRatingSystem, mRating, isChecked());
            // Automatically check/uncheck dependent ratings.
            for (int i = 0; i < mOrders.size(); i++) {
                updateDependentRatingItems(mOrders.get(i), mOrderIndexes.get(i),
                        mContentRatingSystem.getId(), isChecked());
            }
        }

        @Override
        protected int getResourceId() {
            return R.layout.option_item_rating;
        }

        protected int getButtonDrawable() {
            return R.drawable.btn_lock_material_anim;
        }

        private int getRatingOrderIndex(ContentRatingSystem.Order order) {
            int orderIndex = mOrders.indexOf(order);
            return orderIndex == -1 ? -1 : mOrderIndexes.get(orderIndex);
        }

        protected void setRatingBlocked(boolean isChecked) {
            if (isChecked() == isChecked) {
                return;
            }
            mParentalControlSettings.setRatingBlocked(mContentRatingSystem, mRating, isChecked);
            notifyUpdated();
        }
    }

}
