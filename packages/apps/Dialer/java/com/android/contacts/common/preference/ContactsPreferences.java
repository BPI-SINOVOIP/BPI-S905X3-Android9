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

package com.android.contacts.common.preference;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.text.TextUtils;
import com.android.contacts.common.R;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.dialer.strictmode.StrictModeUtils;

/** Manages user preferences for contacts. */
public class ContactsPreferences implements OnSharedPreferenceChangeListener {

  /** The value for the DISPLAY_ORDER key to show the given name first. */
  public static final int DISPLAY_ORDER_PRIMARY = 1;

  /** The value for the DISPLAY_ORDER key to show the family name first. */
  public static final int DISPLAY_ORDER_ALTERNATIVE = 2;

  public static final String DISPLAY_ORDER_KEY = "android.contacts.DISPLAY_ORDER";

  /** The value for the SORT_ORDER key corresponding to sort by given name first. */
  public static final int SORT_ORDER_PRIMARY = 1;

  public static final String SORT_ORDER_KEY = "android.contacts.SORT_ORDER";

  /** The value for the SORT_ORDER key corresponding to sort by family name first. */
  public static final int SORT_ORDER_ALTERNATIVE = 2;

  public static final String PREF_DISPLAY_ONLY_PHONES = "only_phones";

  public static final boolean PREF_DISPLAY_ONLY_PHONES_DEFAULT = false;

  /**
   * Value to use when a preference is unassigned and needs to be read from the shared preferences
   */
  private static final int PREFERENCE_UNASSIGNED = -1;

  private final Context mContext;
  private final SharedPreferences mPreferences;
  private int mSortOrder = PREFERENCE_UNASSIGNED;
  private int mDisplayOrder = PREFERENCE_UNASSIGNED;
  private String mDefaultAccount = null;
  private ChangeListener mListener = null;
  private Handler mHandler;
  private String mDefaultAccountKey;
  private String mDefaultAccountSavedKey;

  public ContactsPreferences(Context context) {
    mContext = context;
    mHandler = new Handler();
    mPreferences =
        mContext
            .getApplicationContext()
            .getSharedPreferences(context.getPackageName(), Context.MODE_PRIVATE);
    mDefaultAccountKey =
        mContext.getResources().getString(R.string.contact_editor_default_account_key);
    mDefaultAccountSavedKey =
        mContext.getResources().getString(R.string.contact_editor_anything_saved_key);
    maybeMigrateSystemSettings();
  }

  private boolean isSortOrderUserChangeable() {
    return mContext.getResources().getBoolean(R.bool.config_sort_order_user_changeable);
  }

  private int getDefaultSortOrder() {
    if (mContext.getResources().getBoolean(R.bool.config_default_sort_order_primary)) {
      return SORT_ORDER_PRIMARY;
    } else {
      return SORT_ORDER_ALTERNATIVE;
    }
  }

  public int getSortOrder() {
    if (!isSortOrderUserChangeable()) {
      return getDefaultSortOrder();
    }
    if (mSortOrder == PREFERENCE_UNASSIGNED) {
      mSortOrder = mPreferences.getInt(SORT_ORDER_KEY, getDefaultSortOrder());
    }
    return mSortOrder;
  }

  public void setSortOrder(int sortOrder) {
    mSortOrder = sortOrder;
    final Editor editor = mPreferences.edit();
    editor.putInt(SORT_ORDER_KEY, sortOrder);
    StrictModeUtils.bypass(editor::commit);
  }

  private boolean isDisplayOrderUserChangeable() {
    return mContext.getResources().getBoolean(R.bool.config_display_order_user_changeable);
  }

  private int getDefaultDisplayOrder() {
    if (mContext.getResources().getBoolean(R.bool.config_default_display_order_primary)) {
      return DISPLAY_ORDER_PRIMARY;
    } else {
      return DISPLAY_ORDER_ALTERNATIVE;
    }
  }

  public int getDisplayOrder() {
    if (!isDisplayOrderUserChangeable()) {
      return getDefaultDisplayOrder();
    }
    if (mDisplayOrder == PREFERENCE_UNASSIGNED) {
      mDisplayOrder = mPreferences.getInt(DISPLAY_ORDER_KEY, getDefaultDisplayOrder());
    }
    return mDisplayOrder;
  }

  public void setDisplayOrder(int displayOrder) {
    mDisplayOrder = displayOrder;
    final Editor editor = mPreferences.edit();
    editor.putInt(DISPLAY_ORDER_KEY, displayOrder);
    StrictModeUtils.bypass(editor::commit);
  }

  private boolean isDefaultAccountUserChangeable() {
    return mContext.getResources().getBoolean(R.bool.config_default_account_user_changeable);
  }

  private String getDefaultAccount() {
    if (!isDefaultAccountUserChangeable()) {
      return mDefaultAccount;
    }
    if (TextUtils.isEmpty(mDefaultAccount)) {
      final String accountString = mPreferences.getString(mDefaultAccountKey, mDefaultAccount);
      if (!TextUtils.isEmpty(accountString)) {
        final AccountWithDataSet accountWithDataSet = AccountWithDataSet.unstringify(accountString);
        mDefaultAccount = accountWithDataSet.name;
      }
    }
    return mDefaultAccount;
  }

  private void setDefaultAccount(AccountWithDataSet accountWithDataSet) {
    mDefaultAccount = accountWithDataSet == null ? null : accountWithDataSet.name;
    final Editor editor = mPreferences.edit();
    if (TextUtils.isEmpty(mDefaultAccount)) {
      editor.remove(mDefaultAccountKey);
    } else {
      editor.putString(mDefaultAccountKey, accountWithDataSet.stringify());
    }
    editor.putBoolean(mDefaultAccountSavedKey, true);
    StrictModeUtils.bypass(editor::commit);
  }

  public void registerChangeListener(ChangeListener listener) {
    if (mListener != null) {
      unregisterChangeListener();
    }

    mListener = listener;

    // Reset preferences to "unknown" because they may have changed while the
    // listener was unregistered.
    mDisplayOrder = PREFERENCE_UNASSIGNED;
    mSortOrder = PREFERENCE_UNASSIGNED;
    mDefaultAccount = null;

    mPreferences.registerOnSharedPreferenceChangeListener(this);
  }

  public void unregisterChangeListener() {
    if (mListener != null) {
      mListener = null;
    }

    mPreferences.unregisterOnSharedPreferenceChangeListener(this);
  }

  @Override
  public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, final String key) {
    // This notification is not sent on the Ui thread. Use the previously created Handler
    // to switch to the Ui thread
    mHandler.post(
        new Runnable() {
          @Override
          public void run() {
            refreshValue(key);
          }
        });
  }

  /**
   * Forces the value for the given key to be looked up from shared preferences and notifies the
   * registered {@link ChangeListener}
   *
   * @param key the {@link SharedPreferences} key to look up
   */
  public void refreshValue(String key) {
    if (DISPLAY_ORDER_KEY.equals(key)) {
      mDisplayOrder = PREFERENCE_UNASSIGNED;
      mDisplayOrder = getDisplayOrder();
    } else if (SORT_ORDER_KEY.equals(key)) {
      mSortOrder = PREFERENCE_UNASSIGNED;
      mSortOrder = getSortOrder();
    } else if (mDefaultAccountKey.equals(key)) {
      mDefaultAccount = null;
      mDefaultAccount = getDefaultAccount();
    }
    if (mListener != null) {
      mListener.onChange();
    }
  }

  /**
   * If there are currently no preferences (which means this is the first time we are run), For sort
   * order and display order, check to see if there are any preferences stored in system settings
   * (pre-L) which can be copied into our own SharedPreferences. For default account setting, check
   * to see if there are any preferences stored in the previous SharedPreferences which can be
   * copied into current SharedPreferences.
   */
  private void maybeMigrateSystemSettings() {
    if (!mPreferences.contains(SORT_ORDER_KEY)) {
      int sortOrder = getDefaultSortOrder();
      try {
        sortOrder = Settings.System.getInt(mContext.getContentResolver(), SORT_ORDER_KEY);
      } catch (SettingNotFoundException e) {
      }
      setSortOrder(sortOrder);
    }

    if (!mPreferences.contains(DISPLAY_ORDER_KEY)) {
      int displayOrder = getDefaultDisplayOrder();
      try {
        displayOrder = Settings.System.getInt(mContext.getContentResolver(), DISPLAY_ORDER_KEY);
      } catch (SettingNotFoundException e) {
      }
      setDisplayOrder(displayOrder);
    }

    if (!mPreferences.contains(mDefaultAccountKey)) {
      final SharedPreferences previousPrefs =
          PreferenceManager.getDefaultSharedPreferences(mContext.getApplicationContext());
      final String defaultAccount = previousPrefs.getString(mDefaultAccountKey, null);
      if (!TextUtils.isEmpty(defaultAccount)) {
        final AccountWithDataSet accountWithDataSet =
            AccountWithDataSet.unstringify(defaultAccount);
        setDefaultAccount(accountWithDataSet);
      }
    }
  }

  public interface ChangeListener {

    void onChange();
  }
}
