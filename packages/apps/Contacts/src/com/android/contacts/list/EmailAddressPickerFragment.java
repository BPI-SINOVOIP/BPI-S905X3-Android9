/*
 * Copyright (C) 2011 The Android Open Source Project
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

import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.contacts.R;

/**
 * Fragment containing an email list for picking.
 */
public class EmailAddressPickerFragment extends ContactEntryListFragment<ContactEntryListAdapter> {
    private OnEmailAddressPickerActionListener mListener;

    public EmailAddressPickerFragment() {
        setQuickContactEnabled(false);
        setPhotoLoaderEnabled(true);
        setSectionHeaderDisplayEnabled(true);
        setDirectorySearchMode(DirectoryListLoader.SEARCH_MODE_DATA_SHORTCUT);
    }

    public void setOnEmailAddressPickerActionListener(OnEmailAddressPickerActionListener listener) {
        mListener = listener;
    }

    @Override
    protected void onItemClick(int position, long id) {
        EmailAddressListAdapter adapter = (EmailAddressListAdapter)getAdapter();
        if (getAdapter().getItem(position) == null) {
            return;
        }
        pickEmailAddress(adapter.getDataUri(position));
    }

    @Override
    protected ContactEntryListAdapter createListAdapter() {
        EmailAddressListAdapter adapter = new EmailAddressListAdapter(getActivity());
        adapter.setSectionHeaderDisplayEnabled(true);
        adapter.setDisplayPhotos(true);
        return adapter;
    }

    @Override
    protected View inflateView(LayoutInflater inflater, ViewGroup container) {
        return inflater.inflate(R.layout.contact_list_content, null);
    }

    @Override
    protected void onCreateView(LayoutInflater inflater, ViewGroup container) {
        super.onCreateView(inflater, container);

        setVisibleScrollbarEnabled(!isLegacyCompatibilityMode());
    }

    private void pickEmailAddress(Uri uri) {
        mListener.onPickEmailAddressAction(uri);
    }
}
