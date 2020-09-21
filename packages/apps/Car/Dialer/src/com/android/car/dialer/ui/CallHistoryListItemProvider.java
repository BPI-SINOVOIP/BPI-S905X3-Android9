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
 * limitations under the License.
 */
package com.android.car.dialer.ui;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.dialer.ui.listitem.CallLogListItem;

import java.util.ArrayList;
import java.util.List;

public class CallHistoryListItemProvider extends ListItemProvider {

    private List<TextListItem> mItems = new ArrayList<>();

    public void setCallHistoryListItems(Context context,
            List<CallLogListingTask.CallLogItem> items) {
        for (CallLogListingTask.CallLogItem callLogItem : items) {
            TextListItem callLogListItem = new CallLogListItem(context, callLogItem);

            callLogListItem.setPrimaryActionIcon(
                    new BitmapDrawable(context.getResources(), callLogItem.mIcon), true);
            callLogListItem.setTitle(callLogItem.mTitle);
            callLogListItem.setBody(callLogItem.mText);
            callLogListItem.setOnClickListener(
                    (v) -> UiCallManager.get().safePlaceCall(callLogItem.mNumber, false));

            mItems.add(callLogListItem);
        }
    }

    @Override
    public ListItem get(int position) {
        return mItems.get(position);
    }

    @Override
    public int size() {
        return mItems.size();
    }
}
