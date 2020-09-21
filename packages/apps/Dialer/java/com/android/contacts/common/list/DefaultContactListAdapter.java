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
package com.android.contacts.common.list;

import android.content.Context;
import android.content.CursorLoader;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.net.Uri.Builder;
import android.preference.PreferenceManager;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Directory;
import android.provider.ContactsContract.SearchSnippets;
import android.text.TextUtils;
import android.view.View;
import com.android.contacts.common.compat.ContactsCompat;
import com.android.contacts.common.preference.ContactsPreferences;
import java.util.ArrayList;
import java.util.List;

/** A cursor adapter for the {@link ContactsContract.Contacts#CONTENT_TYPE} content type. */
public class DefaultContactListAdapter extends ContactListAdapter {

  public DefaultContactListAdapter(Context context) {
    super(context);
  }

  @Override
  public void configureLoader(CursorLoader loader, long directoryId) {
    String sortOrder = null;
    if (isSearchMode()) {
      String query = getQueryString();
      if (query == null) {
        query = "";
      }
      query = query.trim();
      if (TextUtils.isEmpty(query)) {
        // Regardless of the directory, we don't want anything returned,
        // so let's just send a "nothing" query to the local directory.
        loader.setUri(Contacts.CONTENT_URI);
        loader.setProjection(getProjection(false));
        loader.setSelection("0");
      } else {
        final Builder builder = ContactsCompat.getContentUri().buildUpon();
        appendSearchParameters(builder, query, directoryId);
        loader.setUri(builder.build());
        loader.setProjection(getProjection(true));
      }
    } else {
      final ContactListFilter filter = getFilter();
      configureUri(loader, directoryId, filter);
      loader.setProjection(getProjection(false));
      configureSelection(loader, directoryId, filter);
    }

    if (getSortOrder() == ContactsPreferences.SORT_ORDER_PRIMARY) {
      if (sortOrder == null) {
        sortOrder = Contacts.SORT_KEY_PRIMARY;
      } else {
        sortOrder += ", " + Contacts.SORT_KEY_PRIMARY;
      }
    } else {
      if (sortOrder == null) {
        sortOrder = Contacts.SORT_KEY_ALTERNATIVE;
      } else {
        sortOrder += ", " + Contacts.SORT_KEY_ALTERNATIVE;
      }
    }
    loader.setSortOrder(sortOrder);
  }

  private void appendSearchParameters(Builder builder, String query, long directoryId) {
    builder.appendPath(query); // Builder will encode the query
    builder.appendQueryParameter(ContactsContract.DIRECTORY_PARAM_KEY, String.valueOf(directoryId));
    if (directoryId != Directory.DEFAULT && directoryId != Directory.LOCAL_INVISIBLE) {
      builder.appendQueryParameter(
          ContactsContract.LIMIT_PARAM_KEY,
          String.valueOf(getDirectoryResultLimit(getDirectoryById(directoryId))));
    }
    builder.appendQueryParameter(SearchSnippets.DEFERRED_SNIPPETING_KEY, "1");
  }

  protected void configureUri(CursorLoader loader, long directoryId, ContactListFilter filter) {
    Uri uri = Contacts.CONTENT_URI;

    if (directoryId == Directory.DEFAULT && isSectionHeaderDisplayEnabled()) {
      uri = ContactListAdapter.buildSectionIndexerUri(uri);
    }

    // The "All accounts" filter is the same as the entire contents of Directory.DEFAULT
    if (filter != null
        && filter.filterType != ContactListFilter.FILTER_TYPE_CUSTOM
        && filter.filterType != ContactListFilter.FILTER_TYPE_SINGLE_CONTACT) {
      final Uri.Builder builder = uri.buildUpon();
      builder.appendQueryParameter(
          ContactsContract.DIRECTORY_PARAM_KEY, String.valueOf(Directory.DEFAULT));
      if (filter.filterType == ContactListFilter.FILTER_TYPE_ACCOUNT) {
        filter.addAccountQueryParameterToUrl(builder);
      }
      uri = builder.build();
    }

    loader.setUri(uri);
  }

  private void configureSelection(CursorLoader loader, long directoryId, ContactListFilter filter) {
    if (filter == null) {
      return;
    }

    if (directoryId != Directory.DEFAULT) {
      return;
    }

    StringBuilder selection = new StringBuilder();
    List<String> selectionArgs = new ArrayList<String>();

    switch (filter.filterType) {
      case ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS:
        {
          // We have already added directory=0 to the URI, which takes care of this
          // filter
          break;
        }
      case ContactListFilter.FILTER_TYPE_SINGLE_CONTACT:
        {
          // We have already added the lookup key to the URI, which takes care of this
          // filter
          break;
        }
      case ContactListFilter.FILTER_TYPE_STARRED:
        {
          selection.append(Contacts.STARRED + "!=0");
          break;
        }
      case ContactListFilter.FILTER_TYPE_WITH_PHONE_NUMBERS_ONLY:
        {
          selection.append(Contacts.HAS_PHONE_NUMBER + "=1");
          break;
        }
      case ContactListFilter.FILTER_TYPE_CUSTOM:
        {
          selection.append(Contacts.IN_VISIBLE_GROUP + "=1");
          if (isCustomFilterForPhoneNumbersOnly()) {
            selection.append(" AND " + Contacts.HAS_PHONE_NUMBER + "=1");
          }
          break;
        }
      case ContactListFilter.FILTER_TYPE_ACCOUNT:
        {
          // We use query parameters for account filter, so no selection to add here.
          break;
        }
    }
    loader.setSelection(selection.toString());
    loader.setSelectionArgs(selectionArgs.toArray(new String[0]));
  }

  @Override
  protected void bindView(View itemView, int partition, Cursor cursor, int position) {
    super.bindView(itemView, partition, cursor, position);
    final ContactListItemView view = (ContactListItemView) itemView;

    view.setHighlightedPrefix(isSearchMode() ? getUpperCaseQueryString() : null);

    bindSectionHeaderAndDivider(view, position, cursor);

    if (isQuickContactEnabled()) {
      bindQuickContact(
          view,
          partition,
          cursor,
          ContactQuery.CONTACT_PHOTO_ID,
          ContactQuery.CONTACT_PHOTO_URI,
          ContactQuery.CONTACT_ID,
          ContactQuery.CONTACT_LOOKUP_KEY,
          ContactQuery.CONTACT_DISPLAY_NAME);
    } else {
      if (getDisplayPhotos()) {
        bindPhoto(view, partition, cursor);
      }
    }

    bindNameAndViewId(view, cursor);
    bindPresenceAndStatusMessage(view, cursor);

    if (isSearchMode()) {
      bindSearchSnippet(view, cursor);
    } else {
      view.setSnippet(null);
    }
  }

  private boolean isCustomFilterForPhoneNumbersOnly() {
    // TODO: this flag should not be stored in shared prefs.  It needs to be in the db.
    SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
    return prefs.getBoolean(
        ContactsPreferences.PREF_DISPLAY_ONLY_PHONES,
        ContactsPreferences.PREF_DISPLAY_ONLY_PHONES_DEFAULT);
  }
}
