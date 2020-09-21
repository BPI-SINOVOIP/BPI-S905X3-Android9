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
import android.graphics.drawable.Drawable;

import androidx.car.widget.ListItem;
import androidx.car.widget.ListItemProvider;
import androidx.car.widget.TextListItem;

import java.util.ArrayList;
import java.util.List;

import com.android.car.dialer.R;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.dialer.ui.listitem.ContactListItem;

/**
 * Provides ListItem for contact list.
 */
public class ContactListItemProvider extends ListItemProvider {

    private final List<TextListItem> mItems = new ArrayList<>();
    private final OnShowContactDetailListener mOnShowContactDetailListener;

    public interface OnShowContactDetailListener {
        void onShowContactDetail(int contactId, String lookupKey);
    }

    public ContactListItemProvider(Context context, List<ContactListFragment.ContactItem> items,
            OnShowContactDetailListener onShowContactDetailListener) {
        mOnShowContactDetailListener = onShowContactDetailListener;
        for (ContactListFragment.ContactItem contactItem : items) {
            ContactListItem textListItem = new ContactListItem(context, contactItem);
            textListItem.setPrimaryActionIcon(
                    new BitmapDrawable(context.getResources(), contactItem.mIcon), true);
            textListItem.setTitle(contactItem.mDisplayName);
            textListItem.setOnClickListener(
                    (v) -> UiCallManager.get().safePlaceCall(contactItem.mNumber, true));
            Drawable supplementalIconDrawable = context.getDrawable(R.drawable.ic_contact);
            supplementalIconDrawable.setTint(context.getColor(R.color.car_tint));
            int iconSize = context.getResources().getDimensionPixelSize(
                    R.dimen.car_primary_icon_size);
            supplementalIconDrawable.setBounds(0, 0, iconSize, iconSize);
            textListItem.setSupplementalIcon(supplementalIconDrawable, true, (v) -> {
                mOnShowContactDetailListener.onShowContactDetail(contactItem.mId,
                        contactItem.mLookupKey);
            });
            mItems.add(textListItem);
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
