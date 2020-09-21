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
package com.android.documentsui.selection;

import static android.support.v4.util.Preconditions.checkArgument;

import android.support.annotation.Nullable;
import android.view.MotionEvent;

import com.android.documentsui.selection.ItemDetailsLookup.ItemDetails;

/**
 * BandPredicate that consults ItemDetails, premitting band selection to start
 * when the event is not in the drag region of the item.
 */
public final class DefaultBandPredicate extends BandPredicate {

    private final ItemDetailsLookup mDetailsLookup;

    public DefaultBandPredicate(ItemDetailsLookup detailsLookup) {
        checkArgument(detailsLookup != null);

        mDetailsLookup = detailsLookup;
    }

    @Override
    public boolean canInitiate(MotionEvent e) {
        @Nullable ItemDetails details = mDetailsLookup.getItemDetails(e);
        return (details != null)
            ? !details.inDragRegion(e)
            : true;
    }
}
