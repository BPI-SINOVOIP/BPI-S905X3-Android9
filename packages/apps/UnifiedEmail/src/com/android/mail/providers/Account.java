/**
 * Copyright (c) 2012, Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.mail.providers;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;

import com.android.mail.content.CursorCreator;
import com.android.mail.content.ObjectCursor;
import com.android.mail.providers.UIProvider.AccountCapabilities;
import com.android.mail.providers.UIProvider.AccountColumns;
import com.android.mail.providers.UIProvider.SyncStatus;
import com.android.mail.utils.LogTag;
import com.android.mail.utils.LogUtils;
import com.android.mail.utils.Utils;
import com.google.android.mail.common.base.Preconditions;
import com.google.android.mail.common.base.Strings;
import com.google.common.base.Objects;
import com.google.common.collect.Lists;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Account implements Parcelable {
    private static final String SETTINGS_KEY = "settings";

    /**
     * Human readable account name. Not guaranteed to be the account's email address, nor to match
     * the system account manager.
     */
    private final String displayName;

    /**
     * The real name associated with the account, e.g. "John Doe"
     */
    private final String senderName;

    /**
     * Account manager name. MUST MATCH SYSTEM ACCOUNT MANAGER NAME
     */
    private final String accountManagerName;

    /**
     * An unique ID to represent this account.
     */
    private String accountId;

    /**
     * Account type. MUST MATCH SYSTEM ACCOUNT MANAGER TYPE
     */
    private final String type;

    /**
     * Cached android.accounts.Account based on the above two values
     */
    private android.accounts.Account amAccount;

    /**
     * The version of the UI provider schema from which this account provider
     * will return results.
     */
    public final int providerVersion;

    /**
     * The uri to directly access the information for this account.
     */
    public final Uri uri;

    /**
     * The possible capabilities that this account supports.
     */
    public final int capabilities;

    /**
     * The content provider uri to return the list of top level folders for this
     * account.
     */
    public final Uri folderListUri;
    /**
     * The content provider uri to return the list of all real folders for this
     * account.
     */
    public Uri fullFolderListUri;
    /**
     * The content provider uri to return the list of all real and synthetic folders for this
     * account.
     */
    public Uri allFolderListUri;
    /**
     * The content provider uri that can be queried for search results.
     */
    public final Uri searchUri;

    /**
     * The custom from addresses for this account or null if there are none.
     */
    public String accountFromAddresses;

    /**
     * The content provider uri that can be used to expunge message from this
     * account. NOTE: This might be better to be an update operation on the
     * messageUri.
     */
    public final Uri expungeMessageUri;

    /**
     * The content provider uri that can be used to undo the last operation
     * performed.
     */
    public final Uri undoUri;

    /**
     * Uri for EDIT intent that will cause the settings screens for this account type to be
     * shown.
     */
    public final Uri settingsIntentUri;

    /**
     * Uri for VIEW intent that will cause the help screens for this account type to be
     * shown.
     */
    public final Uri helpIntentUri;

    /**
     * Uri for VIEW intent that will cause the send feedback screens for this account type to be
     * shown.
     */
    public final Uri sendFeedbackIntentUri;

    /**
     * Uri for VIEW intent that will cause the reauthentication screen for this account to be
     * shown.
     */
    public final Uri reauthenticationIntentUri;

    /**
     * The sync status of the account
     */
    public final int syncStatus;

    /**
     * Uri for VIEW intent that will cause the compose screen for this account type to be
     * shown.
     */
    public final Uri composeIntentUri;

    public final String mimeType;

    /**
     * URI for recent folders for this account.
     */
    public final Uri recentFolderListUri;

    /**
     * The color used for this account in combined view (Email)
     */
    public final int color;
    /**
     * URI for default recent folders for this account, if any.
     */
    public final Uri defaultRecentFolderListUri;

    /**
     * Settings object for this account.
     */
    public final Settings settings;

    /**
     * URI for forcing a manual sync of this account.
     */
    public final Uri manualSyncUri;

    /**
     * URI for account type specific supplementary account info on outgoing links, if any.
     */
    public final Uri viewIntentProxyUri;

    /**
     * URI for querying for the account cookies to be used when displaying inline content in a
     * conversation
     */
    public final Uri accountCookieQueryUri;

    /**
     * URI to be used with an update() ContentResolver call with a {@link ContentValues} object
     * where the keys are from the {@link AccountColumns.SettingsColumns}, and the values are the
     * new values.
     */
    public final Uri updateSettingsUri;

    /**
     * Whether message transforms (HTML DOM manipulation) feature is enabled.
     */
    public final int enableMessageTransforms;

    /**
     * Sync authority used by the mail app.  This can be used in
     * {@link ContentResolver#getSyncAutomatically} calls to check for whether sync is enabled
     * for this account and mail app.
     */
    public final String syncAuthority;

    public final Uri quickResponseUri;

    /**
     * Fragment class name for account settings
     */
    public final String settingsFragmentClass;

    /**
     * Nonzero value indicates that this account is on a security hold.
     */
    public final int securityHold;

    /**
     * Uri to launch the account security activity.
     */
    public final String accountSecurityUri;

    /**
     * Transient cache of parsed {@link #accountFromAddresses}, plus an entry for the main account
     * address.
     */
    private transient List<ReplyFromAccount> mReplyFroms;

    private static final String LOG_TAG = LogTag.getLogTag();

    /**
     * A custom {@coder Builder} class which client could override.
     */
    private static Class<? extends Builder> sBuilderClass;
    private static Builder sBuilder;

    /**
     * Return a serialized String for this account.
     */
    public synchronized String serialize() {
        JSONObject json = new JSONObject();
        try {
            json.put(AccountColumns.NAME, displayName);
            json.put(AccountColumns.TYPE, type);
            json.put(AccountColumns.SENDER_NAME, senderName);
            json.put(AccountColumns.ACCOUNT_MANAGER_NAME, accountManagerName);
            json.put(AccountColumns.ACCOUNT_ID, accountId);
            json.put(AccountColumns.PROVIDER_VERSION, providerVersion);
            json.put(AccountColumns.URI, uri);
            json.put(AccountColumns.CAPABILITIES, capabilities);
            json.put(AccountColumns.FOLDER_LIST_URI, folderListUri);
            json.put(AccountColumns.FULL_FOLDER_LIST_URI, fullFolderListUri);
            json.put(AccountColumns.ALL_FOLDER_LIST_URI, allFolderListUri);
            json.put(AccountColumns.SEARCH_URI, searchUri);
            json.put(AccountColumns.ACCOUNT_FROM_ADDRESSES, accountFromAddresses);
            json.put(AccountColumns.EXPUNGE_MESSAGE_URI, expungeMessageUri);
            json.put(AccountColumns.UNDO_URI, undoUri);
            json.put(AccountColumns.SETTINGS_INTENT_URI, settingsIntentUri);
            json.put(AccountColumns.HELP_INTENT_URI, helpIntentUri);
            json.put(AccountColumns.SEND_FEEDBACK_INTENT_URI, sendFeedbackIntentUri);
            json.put(AccountColumns.REAUTHENTICATION_INTENT_URI, reauthenticationIntentUri);
            json.put(AccountColumns.SYNC_STATUS, syncStatus);
            json.put(AccountColumns.COMPOSE_URI, composeIntentUri);
            json.put(AccountColumns.MIME_TYPE, mimeType);
            json.put(AccountColumns.RECENT_FOLDER_LIST_URI, recentFolderListUri);
            json.put(AccountColumns.COLOR, color);
            json.put(AccountColumns.DEFAULT_RECENT_FOLDER_LIST_URI, defaultRecentFolderListUri);
            json.put(AccountColumns.MANUAL_SYNC_URI, manualSyncUri);
            json.put(AccountColumns.VIEW_INTENT_PROXY_URI, viewIntentProxyUri);
            json.put(AccountColumns.ACCOUNT_COOKIE_QUERY_URI, accountCookieQueryUri);
            json.put(AccountColumns.UPDATE_SETTINGS_URI, updateSettingsUri);
            json.put(AccountColumns.ENABLE_MESSAGE_TRANSFORMS, enableMessageTransforms);
            json.put(AccountColumns.SYNC_AUTHORITY, syncAuthority);
            json.put(AccountColumns.QUICK_RESPONSE_URI, quickResponseUri);
            json.put(AccountColumns.SETTINGS_FRAGMENT_CLASS, settingsFragmentClass);
            json.put(AccountColumns.SECURITY_HOLD, securityHold);
            json.put(AccountColumns.ACCOUNT_SECURITY_URI, accountSecurityUri);
            if (settings != null) {
                json.put(SETTINGS_KEY, settings.toJSON());
            }
        } catch (JSONException e) {
            LogUtils.wtf(LOG_TAG, e, "Could not serialize account with name %s",
                    displayName);
        }
        return json.toString();
    }

    public static class Builder {
        public Account buildFrom(Cursor cursor) {
            return new Account(cursor);
        }

        public Account buildFrom(JSONObject json) throws JSONException {
            return new Account(json);
        }

        public Account buildFrom(Parcel in, ClassLoader loader) {
            return new Account(in, loader);
        }
    }

    public static synchronized Builder builder() {
        if (sBuilderClass == null) {
            sBuilderClass = Builder.class;
        }
        if (sBuilder == null) {
            try {
                sBuilder = sBuilderClass.newInstance();
            } catch (InstantiationException | IllegalAccessException e) {
                LogUtils.w(LogUtils.TAG, e, "Can't initialize account builder");
                sBuilder = new Builder();
            }
        }
        return sBuilder;
    }

    /**
     * Overrides the default {@code Account.Builder}
     */
    public static synchronized void setBuilderClass(Class<? extends Builder> builderClass) {
        Preconditions.checkState(sBuilderClass == null);
        sBuilderClass = builderClass;
    }

    /**
     * Create a new instance of an Account object using a serialized instance created previously
     * using {@link #serialize()}. This returns null if the serialized instance was invalid or does
     * not represent a valid account object.
     *
     * @param serializedAccount JSON encoded account object
     * @return Account object
     */
    public static Account newInstance(String serializedAccount) {
        // The heavy lifting is done by Account(name, type, json). This method
        // is a wrapper to check for errors and exceptions and return back a null in cases
        // something breaks.
        try {
            final JSONObject json = new JSONObject(serializedAccount);
            return builder().buildFrom(json);
        } catch (JSONException e) {
            LogUtils.w(LOG_TAG, e, "Could not create an account from this input: \"%s\"",
                    serializedAccount);
            return null;
        }
    }

    /**
     * Construct a new Account instance from a previously serialized string.
     *
     * <p>
     * This is private. Public uses should go through the safe {@link #newInstance(String)} method.
     * </p>
     * @param json {@link JSONObject} representing a valid account.
     * @throws JSONException
     */
    protected Account(JSONObject json) throws JSONException {
        displayName = (String) json.get(UIProvider.AccountColumns.NAME);
        type = (String) json.get(UIProvider.AccountColumns.TYPE);
        senderName = json.optString(AccountColumns.SENDER_NAME, null);
        final String amName = json.optString(AccountColumns.ACCOUNT_MANAGER_NAME);
        // We need accountManagerName to be filled in, but we might be dealing with an old cache
        // entry which doesn't have it, so use the display name instead in that case as a fallback
        if (TextUtils.isEmpty(amName)) {
            accountManagerName = displayName;
        } else {
            accountManagerName = amName;
        }
        accountId = json.optString(UIProvider.AccountColumns.ACCOUNT_ID, accountManagerName);
        providerVersion = json.getInt(AccountColumns.PROVIDER_VERSION);
        uri = Uri.parse(json.optString(AccountColumns.URI));
        capabilities = json.getInt(AccountColumns.CAPABILITIES);
        folderListUri = Utils
                .getValidUri(json.optString(AccountColumns.FOLDER_LIST_URI));
        fullFolderListUri = Utils.getValidUri(json
                .optString(AccountColumns.FULL_FOLDER_LIST_URI));
        allFolderListUri = Utils.getValidUri(json
                .optString(AccountColumns.ALL_FOLDER_LIST_URI));
        searchUri = Utils.getValidUri(json.optString(AccountColumns.SEARCH_URI));
        accountFromAddresses = json.optString(AccountColumns.ACCOUNT_FROM_ADDRESSES, "");
        expungeMessageUri = Utils.getValidUri(json
                .optString(AccountColumns.EXPUNGE_MESSAGE_URI));
        undoUri = Utils.getValidUri(json.optString(AccountColumns.UNDO_URI));
        settingsIntentUri = Utils.getValidUri(json
                .optString(AccountColumns.SETTINGS_INTENT_URI));
        helpIntentUri = Utils.getValidUri(json.optString(AccountColumns.HELP_INTENT_URI));
        sendFeedbackIntentUri = Utils.getValidUri(json
                .optString(AccountColumns.SEND_FEEDBACK_INTENT_URI));
        reauthenticationIntentUri = Utils.getValidUri(
                json.optString(AccountColumns.REAUTHENTICATION_INTENT_URI));
        syncStatus = json.optInt(AccountColumns.SYNC_STATUS);
        composeIntentUri = Utils.getValidUri(json.optString(AccountColumns.COMPOSE_URI));
        mimeType = json.optString(AccountColumns.MIME_TYPE);
        recentFolderListUri = Utils.getValidUri(json
                .optString(AccountColumns.RECENT_FOLDER_LIST_URI));
        color = json.optInt(AccountColumns.COLOR, 0);
        defaultRecentFolderListUri = Utils.getValidUri(json
                .optString(AccountColumns.DEFAULT_RECENT_FOLDER_LIST_URI));
        manualSyncUri = Utils
                .getValidUri(json.optString(AccountColumns.MANUAL_SYNC_URI));
        viewIntentProxyUri = Utils
                .getValidUri(json.optString(AccountColumns.VIEW_INTENT_PROXY_URI));
        accountCookieQueryUri = Utils.getValidUri(
                json.optString(AccountColumns.ACCOUNT_COOKIE_QUERY_URI));
        updateSettingsUri = Utils.getValidUri(
                json.optString(AccountColumns.UPDATE_SETTINGS_URI));
        enableMessageTransforms = json.optInt(AccountColumns.ENABLE_MESSAGE_TRANSFORMS);
        syncAuthority = json.optString(AccountColumns.SYNC_AUTHORITY);
        quickResponseUri = Utils.getValidUri(json.optString(AccountColumns.QUICK_RESPONSE_URI));
        settingsFragmentClass = json.optString(AccountColumns.SETTINGS_FRAGMENT_CLASS, "");
        securityHold = json.optInt(AccountColumns.SECURITY_HOLD);
        accountSecurityUri = json.optString(AccountColumns.ACCOUNT_SECURITY_URI);

        final Settings jsonSettings = Settings.newInstance(json.optJSONObject(SETTINGS_KEY));
        if (jsonSettings != null) {
            settings = jsonSettings;
        } else {
            LogUtils.e(LOG_TAG, new Throwable(),
                    "Unexpected null settings in Account(name, type, jsonAccount)");
            settings = Settings.EMPTY_SETTINGS;
        }
    }

    protected Account(Cursor cursor) {
        displayName = cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.NAME));
        senderName = cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.SENDER_NAME));
        type = cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.TYPE));
        accountManagerName = cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.ACCOUNT_MANAGER_NAME));
        accountId = cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.ACCOUNT_ID));
        accountFromAddresses = Strings.nullToEmpty(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.ACCOUNT_FROM_ADDRESSES)));

        final int capabilitiesColumnIndex =
                cursor.getColumnIndex(UIProvider.AccountColumns.CAPABILITIES);
        if (capabilitiesColumnIndex != -1) {
            capabilities = cursor.getInt(capabilitiesColumnIndex);
        } else {
            capabilities = 0;
        }

        providerVersion =
                cursor.getInt(cursor.getColumnIndex(UIProvider.AccountColumns.PROVIDER_VERSION));
        uri = Uri.parse(cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.URI)));
        folderListUri = Uri.parse(
                cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.FOLDER_LIST_URI)));
        fullFolderListUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.FULL_FOLDER_LIST_URI)));
        allFolderListUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.ALL_FOLDER_LIST_URI)));
        searchUri = Utils.getValidUri(
                cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.SEARCH_URI)));
        expungeMessageUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.EXPUNGE_MESSAGE_URI)));
        undoUri = Utils.getValidUri(
                cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.UNDO_URI)));
        settingsIntentUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.SETTINGS_INTENT_URI)));
        helpIntentUri = Utils.getValidUri(
                cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.HELP_INTENT_URI)));
        sendFeedbackIntentUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.SEND_FEEDBACK_INTENT_URI)));
        reauthenticationIntentUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.REAUTHENTICATION_INTENT_URI)));
        syncStatus = cursor.getInt(cursor.getColumnIndex(UIProvider.AccountColumns.SYNC_STATUS));
        composeIntentUri = Utils.getValidUri(
                cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.COMPOSE_URI)));
        mimeType = cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.MIME_TYPE));
        recentFolderListUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.RECENT_FOLDER_LIST_URI)));
        color = cursor.getInt(cursor.getColumnIndex(UIProvider.AccountColumns.COLOR));
        defaultRecentFolderListUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.DEFAULT_RECENT_FOLDER_LIST_URI)));
        manualSyncUri = Utils.getValidUri(
                cursor.getString(cursor.getColumnIndex(UIProvider.AccountColumns.MANUAL_SYNC_URI)));
        viewIntentProxyUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.VIEW_INTENT_PROXY_URI)));
        accountCookieQueryUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.ACCOUNT_COOKIE_QUERY_URI)));
        updateSettingsUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(UIProvider.AccountColumns.UPDATE_SETTINGS_URI)));
        enableMessageTransforms = cursor.getInt(
                cursor.getColumnIndex(AccountColumns.ENABLE_MESSAGE_TRANSFORMS));
        syncAuthority = cursor.getString(
                cursor.getColumnIndex(AccountColumns.SYNC_AUTHORITY));
        if (TextUtils.isEmpty(syncAuthority)) {
            // NOTE: this is actually expected in Email for the "combined view" account only
            LogUtils.e(LOG_TAG, "Unexpected empty syncAuthority from cursor");
        }
        quickResponseUri = Utils.getValidUri(cursor.getString(
                cursor.getColumnIndex(AccountColumns.QUICK_RESPONSE_URI)));
        settingsFragmentClass = cursor.getString(cursor.getColumnIndex(
                AccountColumns.SETTINGS_FRAGMENT_CLASS));
        final int securityHoldIndex = cursor.getColumnIndex(AccountColumns.SECURITY_HOLD);
        securityHold = (securityHoldIndex >= 0 ?
                cursor.getInt(cursor.getColumnIndex(AccountColumns.SECURITY_HOLD)) : 0);
        final int accountSecurityIndex =
                cursor.getColumnIndex(AccountColumns.ACCOUNT_SECURITY_URI);
        accountSecurityUri = (accountSecurityIndex >= 0 ?
                cursor.getString(accountSecurityIndex) : "");
        settings = new Settings(cursor);
    }

    /**
     * Returns an array of all Accounts located at this cursor. This method returns a zero length
     * array if no account was found.  This method does not close the cursor.
     * @param cursor cursor pointing to the list of accounts
     * @return the array of all accounts stored at this cursor.
     */
    public static Account[] getAllAccounts(ObjectCursor<Account> cursor) {
        final int initialLength = cursor.getCount();
        if (initialLength <= 0 || !cursor.moveToFirst()) {
            // Return zero length account array rather than null
            return new Account[0];
        }

        final Account[] allAccounts = new Account[initialLength];
        int i = 0;
        do {
            allAccounts[i++] = cursor.getModel();
        } while (cursor.moveToNext());
        // Ensure that the length of the array is accurate
        assert (i == initialLength);
        return allAccounts;
    }

    public android.accounts.Account getAccountManagerAccount() {
        if (amAccount == null) {
            // We don't really need to synchronize this
            // as worst case is we'd create an extra identical object and throw it away
            amAccount = new android.accounts.Account(accountManagerName, type);
        }
        return amAccount;
    }

    public boolean supportsCapability(int capability) {
        return (capabilities & capability) != 0;
    }

    /**
     * @return <tt>true</tt> if this mail account can be searched in any way (locally on the device,
     *      remotely on the server, or remotely on the server within the current folder)
     */
    public boolean supportsSearch() {
        return supportsCapability(AccountCapabilities.LOCAL_SEARCH)
                || supportsCapability(AccountCapabilities.SERVER_SEARCH)
                || supportsCapability(AccountCapabilities.FOLDER_SERVER_SEARCH);
    }

    public boolean isAccountSyncRequired() {
        return (syncStatus & SyncStatus.INITIAL_SYNC_NEEDED) == SyncStatus.INITIAL_SYNC_NEEDED;
    }

    public boolean isAccountInitializationRequired() {
        return (syncStatus & SyncStatus.ACCOUNT_INITIALIZATION_REQUIRED) ==
                SyncStatus.ACCOUNT_INITIALIZATION_REQUIRED;
    }

    /**
     * Returns true when when the UI provider has indicated that the account has been initialized,
     * and sync is not required.
     */
    public boolean isAccountReady() {
        return !isAccountInitializationRequired() && !isAccountSyncRequired();
    }

    /**
     * @return The account manager account type.
     */
    public String getType() {
        return type;
    }

    protected Account(Parcel in, ClassLoader loader) {
        displayName = in.readString();
        senderName = in.readString();
        type = in.readString();
        accountManagerName = in.readString();
        providerVersion = in.readInt();
        uri = in.readParcelable(null);
        capabilities = in.readInt();
        folderListUri = in.readParcelable(null);
        fullFolderListUri = in.readParcelable(null);
        allFolderListUri = in.readParcelable(null);
        searchUri = in.readParcelable(null);
        accountFromAddresses = in.readString();
        expungeMessageUri = in.readParcelable(null);
        undoUri = in.readParcelable(null);
        settingsIntentUri = in.readParcelable(null);
        helpIntentUri = in.readParcelable(null);
        sendFeedbackIntentUri = in.readParcelable(null);
        reauthenticationIntentUri = in.readParcelable(null);
        syncStatus = in.readInt();
        composeIntentUri = in.readParcelable(null);
        mimeType = in.readString();
        recentFolderListUri = in.readParcelable(null);
        color = in.readInt();
        defaultRecentFolderListUri = in.readParcelable(null);
        manualSyncUri = in.readParcelable(null);
        viewIntentProxyUri = in.readParcelable(null);
        accountCookieQueryUri = in.readParcelable(null);
        updateSettingsUri = in.readParcelable(null);
        enableMessageTransforms = in.readInt();
        syncAuthority = in.readString();
        if (TextUtils.isEmpty(syncAuthority)) {
            LogUtils.e(LOG_TAG, "Unexpected empty syncAuthority from Parcel");
        }
        quickResponseUri = in.readParcelable(null);
        settingsFragmentClass = in.readString();
        securityHold = in.readInt();
        accountSecurityUri = in.readString();
        final int hasSettings = in.readInt();
        if (hasSettings == 0) {
            LogUtils.e(LOG_TAG, new Throwable(), "Unexpected null settings in Account(Parcel)");
            settings = Settings.EMPTY_SETTINGS;
        } else {
            settings = in.readParcelable(loader);
        }
        accountId = in.readString();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(displayName);
        dest.writeString(senderName);
        dest.writeString(type);
        dest.writeString(accountManagerName);
        dest.writeInt(providerVersion);
        dest.writeParcelable(uri, 0);
        dest.writeInt(capabilities);
        dest.writeParcelable(folderListUri, 0);
        dest.writeParcelable(fullFolderListUri, 0);
        dest.writeParcelable(allFolderListUri, 0);
        dest.writeParcelable(searchUri, 0);
        dest.writeString(accountFromAddresses);
        dest.writeParcelable(expungeMessageUri, 0);
        dest.writeParcelable(undoUri, 0);
        dest.writeParcelable(settingsIntentUri, 0);
        dest.writeParcelable(helpIntentUri, 0);
        dest.writeParcelable(sendFeedbackIntentUri, 0);
        dest.writeParcelable(reauthenticationIntentUri, 0);
        dest.writeInt(syncStatus);
        dest.writeParcelable(composeIntentUri, 0);
        dest.writeString(mimeType);
        dest.writeParcelable(recentFolderListUri, 0);
        dest.writeInt(color);
        dest.writeParcelable(defaultRecentFolderListUri, 0);
        dest.writeParcelable(manualSyncUri, 0);
        dest.writeParcelable(viewIntentProxyUri, 0);
        dest.writeParcelable(accountCookieQueryUri, 0);
        dest.writeParcelable(updateSettingsUri, 0);
        dest.writeInt(enableMessageTransforms);
        dest.writeString(syncAuthority);
        dest.writeParcelable(quickResponseUri, 0);
        dest.writeString(settingsFragmentClass);
        dest.writeInt(securityHold);
        dest.writeString(accountSecurityUri);
        if (settings == null) {
            LogUtils.e(LOG_TAG, "unexpected null settings object in writeToParcel");
            dest.writeInt(0);
        } else {
            dest.writeInt(1);
            dest.writeParcelable(settings, 0);
        }
        dest.writeString(accountId);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public String toString() {
        // JSON is readable enough.
        return serialize();
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }

        if ((o == null) || (o.getClass() != this.getClass())) {
            return false;
        }

        final Account other = (Account) o;
        return TextUtils.equals(displayName, other.displayName) &&
                TextUtils.equals(senderName, other.senderName) &&
                TextUtils.equals(accountManagerName, other.accountManagerName) &&
                TextUtils.equals(accountId, other.accountId) &&
                TextUtils.equals(type, other.type) &&
                capabilities == other.capabilities &&
                providerVersion == other.providerVersion &&
                Objects.equal(uri, other.uri) &&
                Objects.equal(folderListUri, other.folderListUri) &&
                Objects.equal(fullFolderListUri, other.fullFolderListUri) &&
                Objects.equal(allFolderListUri, other.allFolderListUri) &&
                Objects.equal(searchUri, other.searchUri) &&
                Objects.equal(accountFromAddresses, other.accountFromAddresses) &&
                Objects.equal(expungeMessageUri, other.expungeMessageUri) &&
                Objects.equal(undoUri, other.undoUri) &&
                Objects.equal(settingsIntentUri, other.settingsIntentUri) &&
                Objects.equal(helpIntentUri, other.helpIntentUri) &&
                Objects.equal(sendFeedbackIntentUri, other.sendFeedbackIntentUri) &&
                Objects.equal(reauthenticationIntentUri, other.reauthenticationIntentUri) &&
                (syncStatus == other.syncStatus) &&
                Objects.equal(composeIntentUri, other.composeIntentUri) &&
                TextUtils.equals(mimeType, other.mimeType) &&
                Objects.equal(recentFolderListUri, other.recentFolderListUri) &&
                color == other.color &&
                Objects.equal(defaultRecentFolderListUri, other.defaultRecentFolderListUri) &&
                Objects.equal(viewIntentProxyUri, other.viewIntentProxyUri) &&
                Objects.equal(accountCookieQueryUri, other.accountCookieQueryUri) &&
                Objects.equal(updateSettingsUri, other.updateSettingsUri) &&
                Objects.equal(enableMessageTransforms, other.enableMessageTransforms) &&
                Objects.equal(syncAuthority, other.syncAuthority) &&
                Objects.equal(quickResponseUri, other.quickResponseUri) &&
                Objects.equal(settingsFragmentClass, other.settingsFragmentClass) &&
                Objects.equal(securityHold, other.securityHold) &&
                Objects.equal(accountSecurityUri, other.accountSecurityUri) &&
                Objects.equal(settings, other.settings);
    }

    /**
     * Returns true if the two accounts differ in sync or server-side settings.
     * This is <b>not</b> a replacement for {@link #equals(Object)}.
     * @param other Account object to compare
     * @return true if the two accounts differ in sync or server-side settings
     */
    public final boolean settingsDiffer(Account other) {
        // If the other object doesn't exist, they differ significantly.
        if (other == null) {
            return true;
        }
        // Check all the server-side settings, the user-side settings and the sync status.
        return ((this.syncStatus != other.syncStatus)
                || !Objects.equal(accountFromAddresses, other.accountFromAddresses)
                || color != other.color
                || (this.settings.hashCode() != other.settings.hashCode()));
    }

    @Override
    public int hashCode() {
        return Objects.hashCode(displayName,
                senderName,
                accountManagerName,
                type,
                capabilities,
                providerVersion,
                uri,
                folderListUri,
                fullFolderListUri,
                allFolderListUri,
                searchUri,
                accountFromAddresses,
                expungeMessageUri,
                undoUri,
                settingsIntentUri,
                helpIntentUri,
                sendFeedbackIntentUri,
                reauthenticationIntentUri,
                syncStatus,
                composeIntentUri,
                mimeType,
                recentFolderListUri,
                color,
                defaultRecentFolderListUri,
                viewIntentProxyUri,
                accountCookieQueryUri,
                updateSettingsUri,
                enableMessageTransforms,
                syncAuthority,
                quickResponseUri,
                securityHold,
                accountSecurityUri);
    }

    /**
     * Returns whether two Accounts match, as determined by their base URIs.
     * <p>For a deep object comparison, use {@link #equals(Object)}.
     *
     */
    public boolean matches(Account other) {
        return other != null && Objects.equal(uri, other.uri);
    }

    public List<ReplyFromAccount> getReplyFroms() {

        if (mReplyFroms == null) {
            mReplyFroms = Lists.newArrayList();

            // skip if sending is unsupported
            if (supportsCapability(AccountCapabilities.VIRTUAL_ACCOUNT)) {
                return mReplyFroms;
            }

            // add the main account address
            mReplyFroms.add(new ReplyFromAccount(this, uri, getEmailAddress(), getSenderName(),
                    getEmailAddress(), false /* isDefault */, false /* isCustom */));

            if (!TextUtils.isEmpty(accountFromAddresses)) {
                try {
                    JSONArray accounts = new JSONArray(accountFromAddresses);

                    for (int i = 0, len = accounts.length(); i < len; i++) {
                        final ReplyFromAccount a = ReplyFromAccount.deserialize(this,
                                accounts.getJSONObject(i));
                        if (a != null) {
                            mReplyFroms.add(a);
                        }
                    }

                } catch (JSONException e) {
                    LogUtils.e(LOG_TAG, e, "Unable to parse accountFromAddresses. name=%s",
                            displayName);
                }
            }
        }
        return mReplyFroms;
    }

    /**
     * @param fromAddress a raw email address, e.g. "user@domain.com"
     * @return if the address belongs to this Account (either as the main address or as a
     * custom-from)
     */
    public boolean ownsFromAddress(String fromAddress) {
        for (ReplyFromAccount replyFrom : getReplyFroms()) {
            if (TextUtils.equals(replyFrom.address, fromAddress)) {
                return true;
            }
        }

        return false;
    }

    /**
     * The display name of the account is the alias the user has chosen to rename the account to.
     * By default it is the email address of the account, but could also be user-entered values like
     * "Work Account" or "Old ISP POP3 account".
     *
     * Account renaming only applies to Email, so a Gmail account should always return the primary
     * email address of the account.
     *
     * @return Account display name
     */
    public String getDisplayName() {
        return displayName;
    }

    /**
     * The primary email address associated with this account, which is also used as the account
     * manager account name.
     * @return email address
     */
    public String getEmailAddress() {
        return accountManagerName;
    }

    /**
     * The account id is an unique id to represent this account.
     */
    public String getAccountId() {
        LogUtils.d(LogUtils.TAG, "getAccountId = %s for email %s", accountId, accountManagerName);
        return accountId;
    }

    /**
     * Returns the real name associated with the account, e.g. "John Doe" or null if no such name
     * has been configured
     * @return sender name
     */
    public String getSenderName() {
        return senderName;
    }

    @SuppressWarnings("hiding")
    public static final ClassLoaderCreator<Account> CREATOR = new ClassLoaderCreator<Account>() {
        @Override
        public Account createFromParcel(Parcel source, ClassLoader loader) {
            return builder().buildFrom(source, loader);
        }

        @Override
        public Account createFromParcel(Parcel source) {
            return builder().buildFrom(source, null);
        }

        @Override
        public Account[] newArray(int size) {
            return new Account[size];
        }
    };

    /**
     * Creates a {@link Map} where the column name is the key and the value is the value, which can
     * be used for populating a {@link MatrixCursor}.
     */
    public Map<String, Object> getValueMap() {
        // ImmutableMap.Builder does not allow null values
        final Map<String, Object> map = new HashMap<String, Object>();

        map.put(AccountColumns._ID, 0);
        map.put(AccountColumns.NAME, displayName);
        map.put(AccountColumns.SENDER_NAME, senderName);
        map.put(AccountColumns.TYPE, type);
        map.put(AccountColumns.ACCOUNT_MANAGER_NAME, accountManagerName);
        map.put(AccountColumns.ACCOUNT_ID, accountId);
        map.put(AccountColumns.PROVIDER_VERSION, providerVersion);
        map.put(AccountColumns.URI, uri);
        map.put(AccountColumns.CAPABILITIES, capabilities);
        map.put(AccountColumns.FOLDER_LIST_URI, folderListUri);
        map.put(AccountColumns.FULL_FOLDER_LIST_URI, fullFolderListUri);
        map.put(AccountColumns.ALL_FOLDER_LIST_URI, allFolderListUri);
        map.put(AccountColumns.SEARCH_URI, searchUri);
        map.put(AccountColumns.ACCOUNT_FROM_ADDRESSES, accountFromAddresses);
        map.put(AccountColumns.EXPUNGE_MESSAGE_URI, expungeMessageUri);
        map.put(AccountColumns.UNDO_URI, undoUri);
        map.put(AccountColumns.SETTINGS_INTENT_URI, settingsIntentUri);
        map.put(AccountColumns.HELP_INTENT_URI, helpIntentUri);
        map.put(AccountColumns.SEND_FEEDBACK_INTENT_URI, sendFeedbackIntentUri);
        map.put(AccountColumns.REAUTHENTICATION_INTENT_URI, reauthenticationIntentUri);
        map.put(AccountColumns.SYNC_STATUS, syncStatus);
        map.put(AccountColumns.COMPOSE_URI, composeIntentUri);
        map.put(AccountColumns.MIME_TYPE, mimeType);
        map.put(AccountColumns.RECENT_FOLDER_LIST_URI, recentFolderListUri);
        map.put(AccountColumns.DEFAULT_RECENT_FOLDER_LIST_URI, defaultRecentFolderListUri);
        map.put(AccountColumns.MANUAL_SYNC_URI, manualSyncUri);
        map.put(AccountColumns.VIEW_INTENT_PROXY_URI, viewIntentProxyUri);
        map.put(AccountColumns.ACCOUNT_COOKIE_QUERY_URI, accountCookieQueryUri);
        map.put(AccountColumns.COLOR, color);
        map.put(AccountColumns.UPDATE_SETTINGS_URI, updateSettingsUri);
        map.put(AccountColumns.ENABLE_MESSAGE_TRANSFORMS, enableMessageTransforms);
        map.put(AccountColumns.SYNC_AUTHORITY, syncAuthority);
        map.put(AccountColumns.QUICK_RESPONSE_URI, quickResponseUri);
        map.put(AccountColumns.SETTINGS_FRAGMENT_CLASS, settingsFragmentClass);
        map.put(AccountColumns.SECURITY_HOLD, securityHold);
        map.put(AccountColumns.ACCOUNT_SECURITY_URI, accountSecurityUri);
        settings.getValueMap(map);

        return map;
    }

    /**
     * Public object that knows how to construct Accounts given Cursors.
     */
    public final static CursorCreator<Account> FACTORY = new CursorCreator<Account>() {
        @Override
        public Account createFromCursor(Cursor c) {
            return builder().buildFrom(c);
        }

        @Override
        public String toString() {
            return "Account CursorCreator";
        }
    };
}
