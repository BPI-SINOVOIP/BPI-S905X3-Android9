package com.android.cts.content;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SyncRequest;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Process;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.util.Log;

import com.android.compatibility.common.util.SystemUtil;

import java.io.IOException;

public class Utils {
    private static final String LOG_TAG = Utils.class.getSimpleName();

    public static final long SYNC_TIMEOUT_MILLIS = 20000; // 20 sec
    public static final String TOKEN_TYPE_REMOVE_ACCOUNTS = "TOKEN_TYPE_REMOVE_ACCOUNTS";
    public static final String SYNC_ACCOUNT_TYPE = "com.stub";
    public static final String ALWAYS_SYNCABLE_AUTHORITY = "com.android.cts.stub.provider";
    public static final String NOT_ALWAYS_SYNCABLE_AUTHORITY = "com.android.cts.stub.provider2";

    public static boolean hasDataConnection() {
        ConnectivityManager connectivityManager = getContext().getSystemService(
                ConnectivityManager.class);
        NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();
        return activeNetwork != null && activeNetwork.isConnectedOrConnecting();
    }

    public static boolean hasNotificationSupport() {
        return !getContext().getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_LEANBACK);
    }

    public static Context getContext() {
        return InstrumentationRegistry.getInstrumentation().getContext();
    }

    public static boolean isWatch() {
        return (getContext().getResources().getConfiguration().uiMode
                & Configuration.UI_MODE_TYPE_MASK) == Configuration.UI_MODE_TYPE_WATCH;
    }

    public static UiDevice getUiDevice() {
        return UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
    }

    public static void waitForSyncManagerAccountChangeUpdate() {
        // Wait for the sync manager to be notified for the new account.
        // Unfortunately, there is no way to detect this event, sigh...
        SystemClock.sleep(SYNC_TIMEOUT_MILLIS);
    }

    public static void allowSyncAdapterRunInBackgroundAndDataInBackground() throws IOException {
        // Allow us to run in the background
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(),
                "cmd deviceidle whitelist +" + getContext().getPackageName());
        // Allow us to use data in the background
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(),
                "cmd netpolicy add restrict-background-whitelist " + Process.myUid());
    }

    public static  void disallowSyncAdapterRunInBackgroundAndDataInBackground() throws IOException {
        // Allow us to run in the background
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(),
                "cmd deviceidle whitelist -" + getContext().getPackageName());
        // Allow us to use data in the background
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(),
                "cmd netpolicy remove restrict-background-whitelist " + Process.myUid());
    }

    public static class ClosableAccount implements AutoCloseable {
        public final Account account;

        private ClosableAccount(@NonNull Account account) {
            this.account = account;
        }

        @Override
        public void close() throws Exception {
            AccountManager accountManager = getContext().getSystemService(AccountManager.class);

            accountManager.getAuthToken(account, TOKEN_TYPE_REMOVE_ACCOUNTS, null, false, null,
                    null);
        }
    }

    public static ClosableAccount withAccount(@NonNull Activity activity)
            throws AuthenticatorException, OperationCanceledException, IOException {
        AccountManager accountManager = getContext().getSystemService(AccountManager.class);
        Bundle result = accountManager.addAccount(SYNC_ACCOUNT_TYPE, null, null, null,
                activity, null, null).getResult();
        Account addedAccount = new Account(
                result.getString(AccountManager.KEY_ACCOUNT_NAME),
                result.getString(AccountManager.KEY_ACCOUNT_TYPE));
        Log.i(LOG_TAG, "Added account " + addedAccount);

        waitForSyncManagerAccountChangeUpdate();

        return new ClosableAccount(addedAccount);
    }

    public static SyncRequest requestSync(String authority) {
        Bundle extras = new Bundle();
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_DO_NOT_RETRY, true);
        extras.putBoolean(ContentResolver.SYNC_EXTRAS_PRIORITY, true);
        extras.getBoolean(ContentResolver.SYNC_EXTRAS_IGNORE_SETTINGS, true);
        SyncRequest request = new SyncRequest.Builder()
                .setSyncAdapter(null, authority)
                .syncOnce()
                .setExtras(extras)
                .setExpedited(true)
                .setManual(true)
                .build();
        ContentResolver.requestSync(request);

        return request;
    }
}
