/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.server.pm;

import android.annotation.NonNull;
import android.annotation.UserIdInt;
import android.app.ActivityManager;
import android.app.ActivityManagerInternal;
import android.app.AppGlobals;
import android.app.IApplicationThread;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.ILauncherApps;
import android.content.pm.IOnAppsChangedListener;
import android.content.pm.LauncherApps.ShortcutQuery;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManagerInternal;
import android.content.pm.ParceledListSlice;
import android.content.pm.ResolveInfo;
import android.content.pm.ShortcutInfo;
import android.content.pm.ShortcutServiceInternal;
import android.content.pm.ShortcutServiceInternal.ShortcutChangeListener;
import android.content.pm.UserInfo;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IInterface;
import android.os.ParcelFileDescriptor;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.UserManagerInternal;
import android.provider.Settings;
import android.util.Log;
import android.util.Slog;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.content.PackageMonitor;
import com.android.internal.os.BackgroundThread;
import com.android.internal.util.Preconditions;
import com.android.server.LocalServices;
import com.android.server.SystemService;

import java.util.Collections;
import java.util.List;

/**
 * Service that manages requests and callbacks for launchers that support
 * managed profiles.
 */
public class LauncherAppsService extends SystemService {

    private final LauncherAppsImpl mLauncherAppsImpl;

    public LauncherAppsService(Context context) {
        super(context);
        mLauncherAppsImpl = new LauncherAppsImpl(context);
    }

    @Override
    public void onStart() {
        publishBinderService(Context.LAUNCHER_APPS_SERVICE, mLauncherAppsImpl);
    }

    static class BroadcastCookie {
        public final UserHandle user;
        public final String packageName;
        public final int callingUid;
        public final int callingPid;

        BroadcastCookie(UserHandle userHandle, String packageName, int callingPid, int callingUid) {
            this.user = userHandle;
            this.packageName = packageName;
            this.callingUid = callingUid;
            this.callingPid = callingPid;
        }
    }

    @VisibleForTesting
    static class LauncherAppsImpl extends ILauncherApps.Stub {
        private static final boolean DEBUG = false;
        private static final String TAG = "LauncherAppsService";
        private final Context mContext;
        private final UserManager mUm;
        private final UserManagerInternal mUserManagerInternal;
        private final ActivityManagerInternal mActivityManagerInternal;
        private final ShortcutServiceInternal mShortcutServiceInternal;
        private final PackageCallbackList<IOnAppsChangedListener> mListeners
                = new PackageCallbackList<IOnAppsChangedListener>();

        private final MyPackageMonitor mPackageMonitor = new MyPackageMonitor();

        private final Handler mCallbackHandler;

        public LauncherAppsImpl(Context context) {
            mContext = context;
            mUm = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
            mUserManagerInternal = Preconditions.checkNotNull(
                    LocalServices.getService(UserManagerInternal.class));
            mActivityManagerInternal = Preconditions.checkNotNull(
                    LocalServices.getService(ActivityManagerInternal.class));
            mShortcutServiceInternal = Preconditions.checkNotNull(
                    LocalServices.getService(ShortcutServiceInternal.class));
            mShortcutServiceInternal.addListener(mPackageMonitor);
            mCallbackHandler = BackgroundThread.getHandler();
        }

        @VisibleForTesting
        int injectBinderCallingUid() {
            return getCallingUid();
        }

        @VisibleForTesting
        int injectBinderCallingPid() {
            return getCallingPid();
        }

        final int injectCallingUserId() {
            return UserHandle.getUserId(injectBinderCallingUid());
        }

        @VisibleForTesting
        long injectClearCallingIdentity() {
            return Binder.clearCallingIdentity();
        }

        // Injection point.
        @VisibleForTesting
        void injectRestoreCallingIdentity(long token) {
            Binder.restoreCallingIdentity(token);
        }

        private int getCallingUserId() {
            return UserHandle.getUserId(injectBinderCallingUid());
        }

        /*
         * @see android.content.pm.ILauncherApps#addOnAppsChangedListener(
         *          android.content.pm.IOnAppsChangedListener)
         */
        @Override
        public void addOnAppsChangedListener(String callingPackage, IOnAppsChangedListener listener)
                throws RemoteException {
            verifyCallingPackage(callingPackage);
            synchronized (mListeners) {
                if (DEBUG) {
                    Log.d(TAG, "Adding listener from " + Binder.getCallingUserHandle());
                }
                if (mListeners.getRegisteredCallbackCount() == 0) {
                    if (DEBUG) {
                        Log.d(TAG, "Starting package monitoring");
                    }
                    startWatchingPackageBroadcasts();
                }
                mListeners.unregister(listener);
                mListeners.register(listener, new BroadcastCookie(UserHandle.of(getCallingUserId()),
                        callingPackage, injectBinderCallingPid(), injectBinderCallingUid()));
            }
        }

        /*
         * @see android.content.pm.ILauncherApps#removeOnAppsChangedListener(
         *          android.content.pm.IOnAppsChangedListener)
         */
        @Override
        public void removeOnAppsChangedListener(IOnAppsChangedListener listener)
                throws RemoteException {
            synchronized (mListeners) {
                if (DEBUG) {
                    Log.d(TAG, "Removing listener from " + Binder.getCallingUserHandle());
                }
                mListeners.unregister(listener);
                if (mListeners.getRegisteredCallbackCount() == 0) {
                    stopWatchingPackageBroadcasts();
                }
            }
        }

        /**
         * Register a receiver to watch for package broadcasts
         */
        private void startWatchingPackageBroadcasts() {
            mPackageMonitor.register(mContext, UserHandle.ALL, true, mCallbackHandler);
        }

        /**
         * Unregister package broadcast receiver
         */
        private void stopWatchingPackageBroadcasts() {
            if (DEBUG) {
                Log.d(TAG, "Stopped watching for packages");
            }
            mPackageMonitor.unregister();
        }

        void checkCallbackCount() {
            synchronized (mListeners) {
                if (DEBUG) {
                    Log.d(TAG, "Callback count = " + mListeners.getRegisteredCallbackCount());
                }
                if (mListeners.getRegisteredCallbackCount() == 0) {
                    stopWatchingPackageBroadcasts();
                }
            }
        }

        /**
         * Checks if the calling user is in the same group as {@code targetUser}, and allowed
         * to access it.
         *
         * @return TRUE if the calling user can access {@code targetUserId}.  FALSE if not *but
         * they're still in the same profile group*.
         *
         * @throws SecurityException if the calling user and {@code targetUser} are not in the same
         * group.
         */
        private boolean canAccessProfile(int targetUserId, String message) {
            final int callingUserId = injectCallingUserId();

            if (targetUserId == callingUserId) return true;

            long ident = injectClearCallingIdentity();
            try {
                final UserInfo callingUserInfo = mUm.getUserInfo(callingUserId);
                if (callingUserInfo != null && callingUserInfo.isManagedProfile()) {
                    Slog.w(TAG, message + " for another profile "
                            + targetUserId + " from " + callingUserId + " not allowed");
                    return false;
                }
            } finally {
                injectRestoreCallingIdentity(ident);
            }

            return mUserManagerInternal.isProfileAccessible(injectCallingUserId(), targetUserId,
                    message, true);
        }

        @VisibleForTesting // We override it in unit tests
        void verifyCallingPackage(String callingPackage) {
            int packageUid = -1;
            try {
                packageUid = AppGlobals.getPackageManager().getPackageUid(callingPackage,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE
                                | PackageManager.MATCH_UNINSTALLED_PACKAGES,
                        UserHandle.getUserId(getCallingUid()));
            } catch (RemoteException ignore) {
            }
            if (packageUid < 0) {
                Log.e(TAG, "Package not found: " + callingPackage);
            }
            if (packageUid != injectBinderCallingUid()) {
                throw new SecurityException("Calling package name mismatch");
            }
        }

        @Override
        public ParceledListSlice<ResolveInfo> getLauncherActivities(String callingPackage,
                String packageName, UserHandle user)
                throws RemoteException {
            return queryActivitiesForUser(callingPackage,
                    new Intent(Intent.ACTION_MAIN)
                            .addCategory(Intent.CATEGORY_LAUNCHER)
                            .setPackage(packageName),
                    user);
        }

        @Override
        public ActivityInfo resolveActivity(
                String callingPackage, ComponentName component, UserHandle user)
                throws RemoteException {
            if (!canAccessProfile(user.getIdentifier(), "Cannot resolve activity")) {
                return null;
            }

            final int callingUid = injectBinderCallingUid();
            long ident = Binder.clearCallingIdentity();
            try {
                final PackageManagerInternal pmInt =
                        LocalServices.getService(PackageManagerInternal.class);
                return pmInt.getActivityInfo(component,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE,
                        callingUid, user.getIdentifier());
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        }

        @Override
        public ParceledListSlice getShortcutConfigActivities(
                String callingPackage, String packageName, UserHandle user)
                throws RemoteException {
            return queryActivitiesForUser(callingPackage,
                    new Intent(Intent.ACTION_CREATE_SHORTCUT).setPackage(packageName), user);
        }

        private ParceledListSlice<ResolveInfo> queryActivitiesForUser(String callingPackage,
                Intent intent, UserHandle user) {
            if (!canAccessProfile(user.getIdentifier(), "Cannot retrieve activities")) {
                return null;
            }

            final int callingUid = injectBinderCallingUid();
            long ident = injectClearCallingIdentity();
            try {
                final PackageManagerInternal pmInt =
                        LocalServices.getService(PackageManagerInternal.class);
                List<ResolveInfo> apps = pmInt.queryIntentActivities(intent,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE,
                        callingUid, user.getIdentifier());
                return new ParceledListSlice<>(apps);
            } finally {
                injectRestoreCallingIdentity(ident);
            }
        }

        @Override
        public IntentSender getShortcutConfigActivityIntent(String callingPackage,
                ComponentName component, UserHandle user) throws RemoteException {
            ensureShortcutPermission(callingPackage);
            if (!canAccessProfile(user.getIdentifier(), "Cannot check package")) {
                return null;
            }
            Preconditions.checkNotNull(component);

            // All right, create the sender.
            Intent intent = new Intent(Intent.ACTION_CREATE_SHORTCUT).setComponent(component);
            final long identity = Binder.clearCallingIdentity();
            try {
                final PendingIntent pi = PendingIntent.getActivityAsUser(
                        mContext, 0, intent, PendingIntent.FLAG_ONE_SHOT
                                | PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_CANCEL_CURRENT,
                        null, user);
                return pi == null ? null : pi.getIntentSender();
            } finally {
                Binder.restoreCallingIdentity(identity);
            }
        }

        @Override
        public boolean isPackageEnabled(String callingPackage, String packageName, UserHandle user)
                throws RemoteException {
            if (!canAccessProfile(user.getIdentifier(), "Cannot check package")) {
                return false;
            }

            final int callingUid = injectBinderCallingUid();
            long ident = Binder.clearCallingIdentity();
            try {
                final PackageManagerInternal pmInt =
                        LocalServices.getService(PackageManagerInternal.class);
                PackageInfo info = pmInt.getPackageInfo(packageName,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE,
                        callingUid, user.getIdentifier());
                return info != null && info.applicationInfo.enabled;
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        }

        @Override
        public Bundle getSuspendedPackageLauncherExtras(String packageName,
                UserHandle user) {
            if (!canAccessProfile(user.getIdentifier(), "Cannot get launcher extras")) {
                return null;
            }
            final PackageManagerInternal pmi =
                    LocalServices.getService(PackageManagerInternal.class);
            return pmi.getSuspendedPackageLauncherExtras(packageName, user.getIdentifier());
        }

        @Override
        public ApplicationInfo getApplicationInfo(
                String callingPackage, String packageName, int flags, UserHandle user)
                throws RemoteException {
            if (!canAccessProfile(user.getIdentifier(), "Cannot check package")) {
                return null;
            }

            final int callingUid = injectBinderCallingUid();
            long ident = Binder.clearCallingIdentity();
            try {
                final PackageManagerInternal pmInt =
                        LocalServices.getService(PackageManagerInternal.class);
                ApplicationInfo info = pmInt.getApplicationInfo(packageName, flags,
                        callingUid, user.getIdentifier());
                return info;
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        }

        private void ensureShortcutPermission(@NonNull String callingPackage) {
            verifyCallingPackage(callingPackage);
            if (!mShortcutServiceInternal.hasShortcutHostPermission(getCallingUserId(),
                    callingPackage, injectBinderCallingPid(), injectBinderCallingUid())) {
                throw new SecurityException("Caller can't access shortcut information");
            }
        }

        @Override
        public ParceledListSlice getShortcuts(String callingPackage, long changedSince,
                String packageName, List shortcutIds, ComponentName componentName, int flags,
                UserHandle targetUser) {
            ensureShortcutPermission(callingPackage);
            if (!canAccessProfile(targetUser.getIdentifier(), "Cannot get shortcuts")) {
                return new ParceledListSlice<>(Collections.EMPTY_LIST);
            }
            if (shortcutIds != null && packageName == null) {
                throw new IllegalArgumentException(
                        "To query by shortcut ID, package name must also be set");
            }

            // TODO(b/29399275): Eclipse compiler requires explicit List<ShortcutInfo> cast below.
            return new ParceledListSlice<>((List<ShortcutInfo>)
                    mShortcutServiceInternal.getShortcuts(getCallingUserId(),
                            callingPackage, changedSince, packageName, shortcutIds,
                            componentName, flags, targetUser.getIdentifier(),
                            injectBinderCallingPid(), injectBinderCallingUid()));
        }

        @Override
        public void pinShortcuts(String callingPackage, String packageName, List<String> ids,
                UserHandle targetUser) {
            ensureShortcutPermission(callingPackage);
            if (!canAccessProfile(targetUser.getIdentifier(), "Cannot pin shortcuts")) {
                return;
            }

            mShortcutServiceInternal.pinShortcuts(getCallingUserId(),
                    callingPackage, packageName, ids, targetUser.getIdentifier());
        }

        @Override
        public int getShortcutIconResId(String callingPackage, String packageName, String id,
                int targetUserId) {
            ensureShortcutPermission(callingPackage);
            if (!canAccessProfile(targetUserId, "Cannot access shortcuts")) {
                return 0;
            }

            return mShortcutServiceInternal.getShortcutIconResId(getCallingUserId(),
                    callingPackage, packageName, id, targetUserId);
        }

        @Override
        public ParcelFileDescriptor getShortcutIconFd(String callingPackage,
                String packageName, String id, int targetUserId) {
            ensureShortcutPermission(callingPackage);
            if (!canAccessProfile(targetUserId, "Cannot access shortcuts")) {
                return null;
            }

            return mShortcutServiceInternal.getShortcutIconFd(getCallingUserId(),
                    callingPackage, packageName, id, targetUserId);
        }

        @Override
        public boolean hasShortcutHostPermission(String callingPackage) {
            verifyCallingPackage(callingPackage);
            return mShortcutServiceInternal.hasShortcutHostPermission(getCallingUserId(),
                    callingPackage, injectBinderCallingPid(), injectBinderCallingUid());
        }

        @Override
        public boolean startShortcut(String callingPackage, String packageName, String shortcutId,
                Rect sourceBounds, Bundle startActivityOptions, int targetUserId) {
            verifyCallingPackage(callingPackage);
            if (!canAccessProfile(targetUserId, "Cannot start activity")) {
                return false;
            }

            // Even without the permission, pinned shortcuts are always launchable.
            if (!mShortcutServiceInternal.isPinnedByCaller(getCallingUserId(),
                    callingPackage, packageName, shortcutId, targetUserId)) {
                ensureShortcutPermission(callingPackage);
            }

            final Intent[] intents = mShortcutServiceInternal.createShortcutIntents(
                    getCallingUserId(), callingPackage, packageName, shortcutId, targetUserId,
                    injectBinderCallingPid(), injectBinderCallingUid());
            if (intents == null || intents.length == 0) {
                return false;
            }
            // Note the target activity doesn't have to be exported.

            intents[0].addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intents[0].setSourceBounds(sourceBounds);

            return startShortcutIntentsAsPublisher(
                    intents, packageName, startActivityOptions, targetUserId);
        }

        private boolean startShortcutIntentsAsPublisher(@NonNull Intent[] intents,
                @NonNull String publisherPackage, Bundle startActivityOptions, int userId) {
            final int code;
            try {
                code = mActivityManagerInternal.startActivitiesAsPackage(publisherPackage,
                        userId, intents, startActivityOptions);
                if (ActivityManager.isStartResultSuccessful(code)) {
                    return true; // Success
                } else {
                    Log.e(TAG, "Couldn't start activity, code=" + code);
                }
                return false;
            } catch (SecurityException e) {
                if (DEBUG) {
                    Slog.d(TAG, "SecurityException while launching intent", e);
                }
                return false;
            }
        }

        @Override
        public boolean isActivityEnabled(
                String callingPackage, ComponentName component, UserHandle user)
                throws RemoteException {
            if (!canAccessProfile(user.getIdentifier(), "Cannot check component")) {
                return false;
            }

            final int callingUid = injectBinderCallingUid();
            long ident = Binder.clearCallingIdentity();
            try {
                final PackageManagerInternal pmInt =
                        LocalServices.getService(PackageManagerInternal.class);
                ActivityInfo info = pmInt.getActivityInfo(component,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE,
                        callingUid, user.getIdentifier());
                return info != null;
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        }

        @Override
        public void startActivityAsUser(IApplicationThread caller, String callingPackage,
                ComponentName component, Rect sourceBounds,
                Bundle opts, UserHandle user) throws RemoteException {
            if (!canAccessProfile(user.getIdentifier(), "Cannot start activity")) {
                return;
            }

            Intent launchIntent = new Intent(Intent.ACTION_MAIN);
            launchIntent.addCategory(Intent.CATEGORY_LAUNCHER);
            launchIntent.setSourceBounds(sourceBounds);
            launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
            launchIntent.setPackage(component.getPackageName());

            boolean canLaunch = false;

            final int callingUid = injectBinderCallingUid();
            long ident = Binder.clearCallingIdentity();
            try {
                final PackageManagerInternal pmInt =
                        LocalServices.getService(PackageManagerInternal.class);
                ActivityInfo info = pmInt.getActivityInfo(component,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE,
                        callingUid, user.getIdentifier());
                if (!info.exported) {
                    throw new SecurityException("Cannot launch non-exported components "
                            + component);
                }

                // Check that the component actually has Intent.CATEGORY_LAUCNCHER
                // as calling startActivityAsUser ignores the category and just
                // resolves based on the component if present.
                List<ResolveInfo> apps = pmInt.queryIntentActivities(launchIntent,
                        PackageManager.MATCH_DIRECT_BOOT_AWARE
                                | PackageManager.MATCH_DIRECT_BOOT_UNAWARE,
                        callingUid, user.getIdentifier());
                final int size = apps.size();
                for (int i = 0; i < size; ++i) {
                    ActivityInfo activityInfo = apps.get(i).activityInfo;
                    if (activityInfo.packageName.equals(component.getPackageName()) &&
                            activityInfo.name.equals(component.getClassName())) {
                        // Found an activity with category launcher that matches
                        // this component so ok to launch.
                        launchIntent.setPackage(null);
                        launchIntent.setComponent(component);
                        canLaunch = true;
                        break;
                    }
                }
                if (!canLaunch) {
                    throw new SecurityException("Attempt to launch activity without "
                            + " category Intent.CATEGORY_LAUNCHER " + component);
                }
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
            mActivityManagerInternal.startActivityAsUser(caller, callingPackage,
                    launchIntent, opts, user.getIdentifier());
        }

        @Override
        public void showAppDetailsAsUser(IApplicationThread caller,
                String callingPackage, ComponentName component,
                Rect sourceBounds, Bundle opts, UserHandle user) throws RemoteException {
            if (!canAccessProfile(user.getIdentifier(), "Cannot show app details")) {
                return;
            }

            final Intent intent;
            long ident = Binder.clearCallingIdentity();
            try {
                String packageName = component.getPackageName();
                intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                        Uri.fromParts("package", packageName, null));
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                intent.setSourceBounds(sourceBounds);
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
            mActivityManagerInternal.startActivityAsUser(caller, callingPackage,
                    intent, opts, user.getIdentifier());
        }

        /** Checks if user is a profile of or same as listeningUser.
         * and the user is enabled. */
        private boolean isEnabledProfileOf(UserHandle listeningUser, UserHandle user,
                String debugMsg) {
            return mUserManagerInternal.isProfileAccessible(listeningUser.getIdentifier(),
                    user.getIdentifier(), debugMsg, false);
        }

        @VisibleForTesting
        void postToPackageMonitorHandler(Runnable r) {
            mCallbackHandler.post(r);
        }

        private class MyPackageMonitor extends PackageMonitor implements ShortcutChangeListener {

            // TODO Simplify with lambdas.

            @Override
            public void onPackageAdded(String packageName, int uid) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackageAdded")) continue;
                        try {
                            listener.onPackageAdded(user, packageName);
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackageAdded(packageName, uid);
            }

            @Override
            public void onPackageRemoved(String packageName, int uid) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackageRemoved")) continue;
                        try {
                            listener.onPackageRemoved(user, packageName);
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackageRemoved(packageName, uid);
            }

            @Override
            public void onPackageModified(String packageName) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackageModified")) continue;
                        try {
                            listener.onPackageChanged(user, packageName);
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackageModified(packageName);
            }

            @Override
            public void onPackagesAvailable(String[] packages) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackagesAvailable")) continue;
                        try {
                            listener.onPackagesAvailable(user, packages, isReplacing());
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackagesAvailable(packages);
            }

            @Override
            public void onPackagesUnavailable(String[] packages) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackagesUnavailable")) continue;
                        try {
                            listener.onPackagesUnavailable(user, packages, isReplacing());
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackagesUnavailable(packages);
            }

            @Override
            public void onPackagesSuspended(String[] packages, Bundle launcherExtras) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackagesSuspended")) continue;
                        try {
                            listener.onPackagesSuspended(user, packages, launcherExtras);
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackagesSuspended(packages, launcherExtras);
            }

            @Override
            public void onPackagesUnsuspended(String[] packages) {
                UserHandle user = new UserHandle(getChangingUserId());
                final int n = mListeners.beginBroadcast();
                try {
                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onPackagesUnsuspended")) continue;
                        try {
                            listener.onPackagesUnsuspended(user, packages);
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } finally {
                    mListeners.finishBroadcast();
                }

                super.onPackagesUnsuspended(packages);
            }

            @Override
            public void onShortcutChanged(@NonNull String packageName,
                    @UserIdInt int userId) {
                postToPackageMonitorHandler(() -> onShortcutChangedInner(packageName, userId));
            }

            private void onShortcutChangedInner(@NonNull String packageName,
                    @UserIdInt int userId) {
                final int n = mListeners.beginBroadcast();
                try {
                    final UserHandle user = UserHandle.of(userId);

                    for (int i = 0; i < n; i++) {
                        IOnAppsChangedListener listener = mListeners.getBroadcastItem(i);
                        BroadcastCookie cookie = (BroadcastCookie) mListeners.getBroadcastCookie(i);
                        if (!isEnabledProfileOf(cookie.user, user, "onShortcutChanged")) continue;

                        final int launcherUserId = cookie.user.getIdentifier();

                        // Make sure the caller has the permission.
                        if (!mShortcutServiceInternal.hasShortcutHostPermission(
                                launcherUserId, cookie.packageName,
                                cookie.callingPid, cookie.callingUid)) {
                            continue;
                        }
                        // Each launcher has a different set of pinned shortcuts, so we need to do a
                        // query in here.
                        // (As of now, only one launcher has the permission at a time, so it's bit
                        // moot, but we may change the permission model eventually.)
                        final List<ShortcutInfo> list =
                                mShortcutServiceInternal.getShortcuts(launcherUserId,
                                        cookie.packageName,
                                        /* changedSince= */ 0, packageName, /* shortcutIds=*/ null,
                                        /* component= */ null,
                                        ShortcutQuery.FLAG_GET_KEY_FIELDS_ONLY
                                        | ShortcutQuery.FLAG_MATCH_ALL_KINDS_WITH_ALL_PINNED
                                        , userId, cookie.callingPid, cookie.callingUid);
                        try {
                            listener.onShortcutChanged(user, packageName,
                                    new ParceledListSlice<>(list));
                        } catch (RemoteException re) {
                            Slog.d(TAG, "Callback failed ", re);
                        }
                    }
                } catch (RuntimeException e) {
                    // When the user is locked we get IllegalState, so just catch all.
                    Log.w(TAG, e.getMessage(), e);
                } finally {
                    mListeners.finishBroadcast();
                }
            }
        }

        class PackageCallbackList<T extends IInterface> extends RemoteCallbackList<T> {
            @Override
            public void onCallbackDied(T callback, Object cookie) {
                checkCallbackCount();
            }
        }
    }
}
