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
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.CursorLoader;
import android.support.v4.content.Loader;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.car.widget.AlphaJumpBucketer;
import androidx.car.widget.IAlphaJumpAdapter;
import androidx.car.widget.ListItemAdapter;
import androidx.car.widget.PagedListView;

import com.android.car.dialer.ContactDetailsFragment;
import com.android.car.dialer.R;
import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.telecom.TelecomUtils;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * Contact Fragment.
 */
public class ContactListFragment extends Fragment implements LoaderManager.LoaderCallbacks<Cursor>,
        ContactListItemProvider.OnShowContactDetailListener {
    private static final int CONTACT_LOADER_ID = 1;

    private PagedListView mPagedListView;

    public static ContactListFragment newInstance() {
        ContactListFragment fragment = new ContactListFragment();
        return fragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LoaderManager loaderManager = LoaderManager.getInstance(this);
        loaderManager.initLoader(CONTACT_LOADER_ID, null, this);
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View fragmentView = inflater.inflate(R.layout.contact_list_fragment, container, false);
        mPagedListView = fragmentView.findViewById(R.id.list_view);
        ((TextView) fragmentView.findViewById(R.id.title)).setText(
                getContext().getString(R.string.contacts_title));
        return fragmentView;
    }

    @Override
    public Loader<Cursor> onCreateLoader(int i, @Nullable Bundle bundle) {
        return new CursorLoader(
                getContext(),
                ContactsContract.Data.CONTENT_URI,
                null,
                ContactsContract.Data.MIMETYPE + " = '" + ContactsContract.CommonDataKinds.Phone
                        .CONTENT_ITEM_TYPE + "'",
                null,
                ContactsContract.Contacts.DISPLAY_NAME + " ASC "
        );
    }

    @Override
    public void onLoadFinished(@NonNull Loader loader, Cursor cursor) {
        List<ContactItem> contactItems = new ArrayList<>();
        while (cursor.moveToNext()) {
            String number = PhoneLoader.getPhoneNumber(cursor, getContext().getContentResolver());

            int idColumnIndex = PhoneLoader.getIdColumnIndex(cursor);
            int id = cursor.getInt(idColumnIndex);

            int displayNameColumnIndex = cursor.getColumnIndex(
                    ContactsContract.Contacts.DISPLAY_NAME);
            String displayName = cursor.getString(displayNameColumnIndex);

            int lookupKeyColumnIndex = cursor.getColumnIndex(
                    ContactsContract.Contacts.LOOKUP_KEY);
            String lookupKey = cursor.getString(lookupKeyColumnIndex);

            contactItems.add(new ContactItem(number, id, displayName, null, lookupKey));
        }

        ListItemAdapter contactListAdapter = new ListItemAdapter(getContext(),
                new ContactListItemProvider(getContext(), contactItems, ContactListFragment.this));
        mPagedListView.setAdapter(contactListAdapter);
        contactListAdapter.notifyDataSetChanged();
    }

    @Override
    public void onLoaderReset(@NonNull Loader loader) {
    }

    @Override
    public void onShowContactDetail(int contactId, String lookupKey) {
        View contactDetailContainer =
                getView().findViewById(R.id.contact_detail_container);
        contactDetailContainer.setVisibility(View.VISIBLE);
        setActivityActionBarVisibility(View.GONE);

        final Uri uri = ContactsContract.Contacts.getLookupUri(contactId, lookupKey);
        Fragment contactDetailFragment = ContactDetailsFragment.newInstance(uri, null);
        getChildFragmentManager().beginTransaction().replace(R.id.contact_detail_fragment_container,
                contactDetailFragment).commit();

        getView().findViewById(R.id.back_button).setOnClickListener((v) -> {
            getChildFragmentManager().beginTransaction().remove(contactDetailFragment).commit();
            contactDetailContainer.setVisibility(View.GONE);
            setActivityActionBarVisibility(View.VISIBLE);
        });

    }

    private void setActivityActionBarVisibility(int visibility) {
        View actionBar = getActivity().getWindow().getDecorView().findViewById(R.id.car_toolbar);
        if (actionBar != null) {
            actionBar.setVisibility(visibility);
        }
    }

    /**
     * Pojo which holds a contact entry information.
     */
    public static class ContactItem {
        public final String mNumber;
        public final int mId;
        public final String mDisplayName;
        public final Bitmap mIcon;
        public final String mLookupKey;

        private ContactItem(String number, int id, String displayName, Bitmap icon,
                String lookupKey) {
            mNumber = number;
            mId = id;
            mDisplayName = displayName;
            mIcon = icon;
            mLookupKey = lookupKey;
        }
    }

    /**
     * Use this Adapter to enabled AlphaJump.
     */
    private class ContactListAdapter extends ListItemAdapter implements IAlphaJumpAdapter {

        List<ContactItem> mContactItems;

        public ContactListAdapter(Context context,
                List<ContactItem> contactItems) {
            super(context,
                    new ContactListItemProvider(context, contactItems, ContactListFragment.this));
            mContactItems = contactItems;
        }

        public ContactListAdapter(Context context, List<ContactItem> contactItems,
                int backgroundStyle) {
            super(context,
                    new ContactListItemProvider(context, contactItems, ContactListFragment.this),
                    backgroundStyle);
            mContactItems = contactItems;
        }

        @Override
        public Collection<Bucket> getAlphaJumpBuckets() {
            AlphaJumpBucketer alphaJumpBucketer = new AlphaJumpBucketer();
            List<String> values = new ArrayList<>();
            for (ContactItem contactItem : mContactItems) {
                values.add(contactItem.mDisplayName);
            }

            return alphaJumpBucketer.createBuckets(values.toArray(new String[]{}));
        }

        @Override
        public void onAlphaJumpEnter() {
        }

        @Override
        public void onAlphaJumpLeave(Bucket bucket) {
        }
    }
}
