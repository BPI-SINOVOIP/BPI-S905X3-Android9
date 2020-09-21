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

package com.android.tv.settings.connectivity.util;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.v17.leanback.widget.BaseGridView;
import android.support.v17.leanback.widget.FacetProvider;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v17.leanback.widget.ItemAlignmentFacet;
import android.support.v17.leanback.widget.VerticalGridView;
import android.view.View;

/**
 * Utilities to align the ActionGridView so that the baseline of the title view matches with
 * the keyline of the fragment.
 */
public class GuidedActionsAlignUtil {

    /**
     * As we want to align to the mean line of the text view, we should always provide a customized
     * viewholder with the new facet when we are creating a GuidedActionStylist.
     */
    public static class SetupViewHolder extends GuidedActionsStylist.ViewHolder implements
            FacetProvider {
        public SetupViewHolder(View v) {
            super(v);
        }

        // Provide a customized ItemAlignmentFacet so that the mean line of textView is matched.
        // Here we use mean line of the textview to work as the baseline to be matched with
        // guidance title baseline.
        @Override
        public Object getFacet(Class facet) {
            if (facet.equals(ItemAlignmentFacet.class)) {
                ItemAlignmentFacet.ItemAlignmentDef alignedDef =
                        new ItemAlignmentFacet.ItemAlignmentDef();
                alignedDef.setItemAlignmentViewId(
                        android.support.v17.leanback.R.id.guidedactions_item_title);
                alignedDef.setAlignedToTextViewBaseline(false);
                alignedDef.setItemAlignmentOffset(0);
                alignedDef.setItemAlignmentOffsetWithPadding(true);
                // 50 refers to 50 percent, which refers to mid position of textView.
                alignedDef.setItemAlignmentOffsetPercent(50);
                ItemAlignmentFacet f = new ItemAlignmentFacet();
                f.setAlignmentDefs(new ItemAlignmentFacet.ItemAlignmentDef[]{alignedDef});
                return f;
            }
            return null;
        }
    }

    private static float getKeyLinePercent(Context context) {
        TypedArray ta = context.getTheme().obtainStyledAttributes(
                android.support.v17.leanback.R.styleable.LeanbackGuidedStepTheme);
        float percent = ta.getFloat(
                android.support.v17.leanback.R.styleable.LeanbackGuidedStepTheme_guidedStepKeyline,
                40);
        ta.recycle();
        return percent;
    }

    /**
     * Align the gridView.
     *
     * @param guidedActionsStylist the GuidedActionsStylist that needs to be aligned.
     */
    public static void align(GuidedActionsStylist guidedActionsStylist) {
        final VerticalGridView gridView = guidedActionsStylist.getActionsGridView();
        gridView.setWindowAlignment(BaseGridView.WINDOW_ALIGN_HIGH_EDGE);
        gridView.setWindowAlignmentPreferKeyLineOverHighEdge(true);
    }
}
