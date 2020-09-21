/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.contacts.list;

import android.text.TextUtils;
import android.widget.SectionIndexer;

import java.util.Arrays;

/**
 * A section indexer that is configured with precomputed section titles and
 * their respective counts.
 */
public class ContactsSectionIndexer implements SectionIndexer {

    protected static final String BLANK_HEADER_STRING = "\u2026"; // ellipsis

    private String[] mSections;
    private int[] mPositions;
    private int mCount;

    /**
     * Constructor.
     *
     * @param sections a non-null array
     * @param counts a non-null array of the same size as <code>sections</code>
     */
    public ContactsSectionIndexer(String[] sections, int[] counts) {
        if (sections == null || counts == null) {
            throw new NullPointerException();
        }

        if (sections.length != counts.length) {
            throw new IllegalArgumentException(
                    "The sections and counts arrays must have the same length");
        }

        // TODO process sections/counts based on current locale and/or specific section titles

        this.mSections = sections;
        mPositions = new int[counts.length];
        int position = 0;
        for (int i = 0; i < counts.length; i++) {
            // Enforce that there will be no null or empty sections.
            if (TextUtils.isEmpty(mSections[i])) {
                mSections[i] = BLANK_HEADER_STRING;
            } else if (!mSections[i].equals(BLANK_HEADER_STRING)) {
                mSections[i] = mSections[i].trim();
            }

            mPositions[i] = position;
            position += counts[i];
        }
        mCount = position;
    }

    public Object[] getSections() {
        return mSections;
    }

    public int[] getPositions() {
        return mPositions;
    }

    public int getPositionForSection(int section) {
        if (section < 0 || section >= mSections.length) {
            return -1;
        }

        return mPositions[section];
    }

    public int getSectionForPosition(int position) {
        if (position < 0 || position >= mCount) {
            return -1;
        }

        int index = Arrays.binarySearch(mPositions, position);

        /*
         * Consider this example: section positions are 0, 3, 5; the supplied
         * position is 4. The section corresponding to position 4 starts at
         * position 3, so the expected return value is 1. Binary search will not
         * find 4 in the array and thus will return -insertPosition-1, i.e. -3.
         * To get from that number to the expected value of 1 we need to negate
         * and subtract 2.
         */
        return index >= 0 ? index : -index - 2;
    }

    public void setFavoritesHeader(int numberOfItemsToAdd) {
        if (mSections != null) {
            // Don't do anything if the header is already set properly.
            if (mSections.length > 0 && mSections[0].isEmpty()) {
                return;
            }

            // Since the section indexer isn't aware of the profile at the top, we need to add a
            // special section at the top for it and shift everything else down.
            String[] tempSections = new String[mSections.length + 1];
            int[] tempPositions = new int[mPositions.length + 1];
            // Favorites section is empty to hide fast scroll preview.
            tempSections[0] = "";
            tempPositions[0] = 0;
            for (int i = 1; i <= mPositions.length; i++) {
                tempSections[i] = mSections[i - 1];
                tempPositions[i] = mPositions[i - 1] + numberOfItemsToAdd;
            }
            mSections = tempSections;
            mPositions = tempPositions;
            mCount = mCount + numberOfItemsToAdd;
        }
    }
}
