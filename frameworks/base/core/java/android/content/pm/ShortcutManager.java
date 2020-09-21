/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.content.pm;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SystemService;
import android.annotation.TestApi;
import android.annotation.UserIdInt;
import android.app.Activity;
import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.graphics.drawable.AdaptiveIconDrawable;
import android.os.Build.VERSION_CODES;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;

import com.android.internal.annotations.VisibleForTesting;

import java.util.List;

/**
 * The ShortcutManager performs operations on an app's set of <em>shortcuts</em>. The
 * {@link ShortcutInfo} class contains information about each of the shortcuts themselves.
 *
 * <p>An app's shortcuts represent specific tasks and actions that users can perform within your
 * app. When a user selects a shortcut in the currently-active launcher, your app opens an activity
 * other than the app's starting activity, provided that the currently-active launcher supports app
 * shortcuts.</p>
 *
 * <p>The types of shortcuts that you create for your app depend on the app's key use cases. For
 * example, an email app may publish the "compose new email" shortcut, which allows the app to
 * directly open the compose activity.</p>
 *
 * <p class="note"><b>Note:</b> Only main activities&mdash;activities that handle the
 * {@link Intent#ACTION_MAIN} action and the {@link Intent#CATEGORY_LAUNCHER} category&mdash;can
 * have shortcuts. If an app has multiple main activities, you need to define the set of shortcuts
 * for <em>each</em> activity.
 *
 * <p>This page discusses the implementation details of the <code>ShortcutManager</code> class. For
 * definitions of key terms and guidance on performing operations on shortcuts within your app, see
 * the <a href="/guide/topics/ui/shortcuts.html">App Shortcuts</a> feature guide.
 *
 * <h3>Shortcut characteristics</h3>
 *
 * This section describes in-depth details about each shortcut type's usage and availability.
 *
 * <p class="note"><b>Important security note:</b> All shortcut information is stored in
 * <a href="/training/articles/direct-boot.html">credential encrypted storage</a>, so your app
 * cannot access a user's shortcuts until after they've unlocked the device.
 *
 * <h4>Static and dynamic shortcuts</h4>
 *
 * <p>Static shortcuts and dynamic shortcuts are shown in a supported launcher when the user
 * performs a specific gesture. On currently-supported launchers, the gesture is a long-press on the
 * app's launcher icon, but the actual gesture may be different on other launcher apps.
 *
 * <p>The {@link LauncherApps} class provides APIs for launcher apps to access shortcuts.
 *
 * <h4>Pinned shortcuts</h4>
 *
 * <p>Because pinned shortcuts appear in the launcher itself, they're always visible. A pinned
 * shortcut is removed from the launcher only in the following situations:
 * <ul>
 *     <li>The user removes it.
 *     <li>The publisher app associated with the shortcut is uninstalled.
 *     <li>The user selects <b>Clear data</b> from the publisher app's <i>Storage</i> screen, within
 *     the system's <b>Settings</b> app.
 * </ul>
 *
 * <p>Because the system performs
 * <a href="/guide/topics/ui/shortcuts.html#backup-and-restore">backup and restore</a> on pinned
 * shortcuts automatically, these shortcuts' IDs should contain either stable, constant strings or
 * server-side identifiers, rather than identifiers generated locally that might not make sense on
 * other devices.
 *
 * <h3>Shortcut display order</h3>
 *
 * <p>When the launcher displays an app's shortcuts, they should appear in the following order:
 *
 * <ol>
 *   <li><b>Static shortcuts:</b> Shortcuts whose {@link ShortcutInfo#isDeclaredInManifest()} method
 *   returns {@code true}.</li>
 *   <li><b>Dynamic shortcuts:</b> Shortcuts whose {@link ShortcutInfo#isDynamic()} method returns
 *   {@code true}.</li>
 * </ol>
 *
 * <p>Within each shortcut type (static and dynamic), shortcuts are sorted in order of increasing
 * rank according to {@link ShortcutInfo#getRank()}.</p>
 *
 * <h4>Shortcut ranks</h4>
 *
 * <p>Shortcut ranks are non-negative, sequential integers that determine the order in which
 * shortcuts appear, assuming that the shortcuts are all in the same category. You can update ranks
 * of existing shortcuts when you call {@link #updateShortcuts(List)},
 * {@link #addDynamicShortcuts(List)}, or {@link #setDynamicShortcuts(List)}.
 *
 * <p class="note"><b>Note:</b> Ranks are auto-adjusted so that they're unique for each type of
 * shortcut (static or dynamic). For example, if there are 3 dynamic shortcuts with ranks 0, 1 and
 * 2, adding another dynamic shortcut with a rank of 1 represents a request to place this shortcut
 * at the second position. In response, the third and fourth shortcuts move closer to the bottom of
 * the shortcut list, with their ranks changing to 2 and 3, respectively.
 *
 * <h3>Options for static shortcuts</h3>
 *
 * The following list includes descriptions for the different attributes within a static shortcut.
 * You must provide a value for {@code android:shortcutId} and {@code android:shortcutShortLabel};
 * all other values are optional.
 *
 * <dl>
 *   <dt>{@code android:shortcutId}</dt>
 *   <dd><p>A string literal, which represents the shortcut when a {@code ShortcutManager} object
 *   performs operations on it.</p>
 *   <p class="note"><b>Note: </b>You cannot set this attribute's value to a resource string, such
 *   as <code>@string/foo</code>.</p>
 *   </dd>
 *
 *   <dt>{@code android:enabled}</dt>
 *   <dd><p>Whether the user can interact with the shortcut from a supported launcher.</p>
 *   <p>The default value is {@code true}. If you set it to {@code false}, you should also set
 *   {@code android:shortcutDisabledMessage} to a message that explains why you've disabled the
 *   shortcut. If you don't think you need to provide such a message, it's easiest to just remove
 *   the shortcut from the XML file entirely, rather than changing the values of the shortcut's
 *   {@code android:enabled} and {@code android:shortcutDisabledMessage} attributes.
 *   </dd>
 *
 *   <dt>{@code android:icon}</dt>
 *   <dd><p>The <a href="/topic/performance/graphics/index.html">bitmap</a> or
 *   <a href="/guide/practices/ui_guidelines/icon_design_adaptive.html">adaptive icon</a> that the
 *   launcher uses when displaying the shortcut to the user. This value can be either the path to an
 *   image or the resource file that contains the image. Use adaptive icons whenever possible to
 *   improve performance and consistency.</p>
 *   <p class="note"><b>Note: </b>Shortcut icons cannot include
 *   <a href="/training/material/drawables.html#DrawableTint">tints</a>.
 *   </dd>
 *
 *   <dt>{@code android:shortcutShortLabel}</dt>
 *   <dd><p>A concise phrase that describes the shortcut's purpose. For more information, see
 *   {@link ShortcutInfo.Builder#setShortLabel(CharSequence)}.</p>
 *   <p class="note"><b>Note: </b>This attribute's value must be a resource string, such as
 *   <code>@string/shortcut_short_label</code>.</p>
 *   </dd>
 *
 *   <dt>{@code android:shortcutLongLabel}</dt>
 *   <dd><p>An extended phrase that describes the shortcut's purpose. If there's enough space, the
 *   launcher displays this value instead of {@code android:shortcutShortLabel}. For more
 *   information, see {@link ShortcutInfo.Builder#setLongLabel(CharSequence)}.</p>
 *   <p class="note"><b>Note: </b>This attribute's value must be a resource string, such as
 *   <code>@string/shortcut_long_label</code>.</p>
 *   </dd>
 *
 *   <dt>{@code android:shortcutDisabledMessage}</dt>
 *   <dd><p>The message that appears in a supported launcher when the user attempts to launch a
 *   disabled shortcut. The message should explain to the user why the shortcut is now disabled.
 *   This attribute's value has no effect if {@code android:enabled} is {@code true}.</p>
 *   <p class="note"><b>Note: </b>This attribute's value must be a resource string, such as
 *   <code>@string/shortcut_disabled_message</code>.</p>
 *   </dd>
 * </dl>
 *
 * <h3>Inner elements that define static shortcuts</h3>
 *
 * <p>The XML file that lists an app's static shortcuts supports the following elements inside each
 * {@code <shortcut>} element. You must include an {@code intent} inner element for each
 * static shortcut that you define.</p>
 *
 * <dl>
 *   <dt>{@code intent}</dt>
 *   <dd><p>The action that the system launches when the user selects the shortcut. This intent must
 *   provide a value for the {@code android:action} attribute.</p>
 *   <p>You can provide multiple intents for a single shortcut. If you do so, the last defined
 *   activity is launched, and the other activities are placed in the
 *   <a href="/guide/components/tasks-and-back-stack.html">back stack</a>. See
 *   <a href="/guide/topics/ui/shortcuts.html#static">Using Static Shortcuts</a> and the
 *   {@link android.app.TaskStackBuilder} class reference for details.</p>
 *   <p class="note"><b>Note:</b> This {@code intent} element cannot include string resources.</p>
 *   <p>To learn more about how to configure intents, see
 *   <a href="{@docRoot}guide/topics/ui/settings.html#Intents">Using intents</a>.</p>
 *   </dd>
 *
 *   <dt>{@code categories}</dt>
 *   <dd><p>Provides a grouping for the types of actions that your app's shortcuts perform, such as
 *   creating new chat messages.</p>
 *   <p>For a list of supported shortcut categories, see the {@link ShortcutInfo} class reference
 *   for a list of supported shortcut categories.
 *   </dd>
 * </dl>
 *
 * <h3>Updating shortcuts</h3>
 *
 * <p>Each app's launcher icon can contain at most {@link #getMaxShortcutCountPerActivity()} number
 * of static and dynamic shortcuts combined. There is no limit to the number of pinned shortcuts
 * that an app can create, though.
 *
 * <p>When a dynamic shortcut is pinned, even when the publisher removes it as a dynamic shortcut,
 * the pinned shortcut is still visible and launchable.  This allows an app to have more than
 * {@link #getMaxShortcutCountPerActivity()} number of shortcuts.
 *
 * <p>As an example, suppose {@link #getMaxShortcutCountPerActivity()} is 5:
 * <ol>
 *     <li>A chat app publishes 5 dynamic shortcuts for the 5 most recent
 *     conversations (c1, c2, ..., c5).
 *
 *     <li>The user pins all 5 of the shortcuts.
 *
 *     <li>Later, the user has started 3 additional conversations (c6, c7, and c8), so the publisher
 *     app re-publishes its dynamic shortcuts. The new dynamic shortcut list is: c4, c5, ..., c8.
 *     <p>The publisher app has to remove c1, c2, and c3 because it can't have more than 5 dynamic
 *     shortcuts. However, c1, c2, and c3 are still pinned shortcuts that the user can access and
 *     launch.
 *     <p>At this point, the user can access a total of 8 shortcuts that link to activities in the
 *     publisher app, including the 3 pinned shortcuts, even though an app can have at most 5
 *     dynamic shortcuts.
 *
 *     <li>The app can use {@link #updateShortcuts(List)} to update <em>any</em> of the existing
 *     8 shortcuts, when, for example, the chat peers' icons have changed.
 *     <p>The {@link #addDynamicShortcuts(List)} and {@link #setDynamicShortcuts(List)} methods
 *     can also be used to update existing shortcuts with the same IDs, but they <b>cannot</b> be
 *     used for updating non-dynamic, pinned shortcuts because these 2 methods try to convert the
 *     given lists of shortcuts to dynamic shortcuts.
 * </ol>
 *
 * <h3>Shortcut intents</h3>
 *
 * <p>
 * Dynamic shortcuts can be published with any set of {@link Intent#addFlags Intent} flags.
 * Typically, {@link Intent#FLAG_ACTIVITY_CLEAR_TASK} is specified, possibly along with other
 * flags; otherwise, if the app is already running, the app is simply brought to
 * the foreground, and the target activity might not appear.
 *
 * <p>Static shortcuts <b>cannot</b> have custom intent flags.
 * The first intent of a static shortcut will always have {@link Intent#FLAG_ACTIVITY_NEW_TASK}
 * and {@link Intent#FLAG_ACTIVITY_CLEAR_TASK} set. This means, when the app is already running, all
 * the existing activities in your app are destroyed when a static shortcut is launched.
 * If this behavior is not desirable, you can use a <em>trampoline activity</em>, or an invisible
 * activity that starts another activity in {@link Activity#onCreate}, then calls
 * {@link Activity#finish()}:
 * <ol>
 *     <li>In the <code>AndroidManifest.xml</code> file, the trampoline activity should include the
 *     attribute assignment {@code android:taskAffinity=""}.
 *     <li>In the shortcuts resource file, the intent within the static shortcut should reference
 *     the trampoline activity.
 * </ol>
 *
 * <h3>Rate limiting</h3>
 *
 * <p>When <a href="/guide/topics/ui/shortcuts.html#rate-limit">rate limiting</a> is active,
 * {@link #isRateLimitingActive()} returns {@code true}.
 *
 * <p>Rate limiting is reset upon certain events, so even background apps can call these APIs until
 * the rate limit is reached again. These events include the following:
 * <ul>
 *   <li>An app comes to the foreground.
 *   <li>The system locale changes.
 *   <li>The user performs the <a href="/guide/topics/ui/notifiers/notifications.html#direct">inline
 *   reply</a> action on a notification.
 * </ul>
 *
 * <h3>Handling system locale changes</h3>
 *
 * <p>Apps should update dynamic and pinned shortcuts when they receive the
 * {@link Intent#ACTION_LOCALE_CHANGED} broadcast, indicating that the system locale has changed.
 * <p>When the system locale changes, <a href="/guide/topics/ui/shortcuts.html#rate-limit">rate
 * limiting</a> is reset, so even background apps can add and update dynamic shortcuts until the
 * rate limit is reached again.
 *
 * <h3>Retrieving class instances</h3>
 * <!-- Provides a heading for the content filled in by the @SystemService annotation below -->
 */
@SystemService(Context.SHORTCUT_SERVICE)
public class ShortcutManager {
    private static final String TAG = "ShortcutManager";

    private final Context mContext;
    private final IShortcutService mService;

    /**
     * @hide
     */
    public ShortcutManager(Context context, IShortcutService service) {
        mContext = context;
        mService = service;
    }

    /**
     * @hide
     */
    @TestApi
    public ShortcutManager(Context context) {
        this(context, IShortcutService.Stub.asInterface(
                ServiceManager.getService(Context.SHORTCUT_SERVICE)));
    }

    /**
     * Publish the list of shortcuts.  All existing dynamic shortcuts from the caller app
     * will be replaced.  If there are already pinned shortcuts with the same IDs,
     * the mutable pinned shortcuts are updated.
     *
     * <p>This API will be rate-limited.
     *
     * @return {@code true} if the call has succeeded. {@code false} if the call is rate-limited.
     *
     * @throws IllegalArgumentException if {@link #getMaxShortcutCountPerActivity()} is exceeded,
     * or when trying to update immutable shortcuts.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public boolean setDynamicShortcuts(@NonNull List<ShortcutInfo> shortcutInfoList) {
        try {
            return mService.setDynamicShortcuts(mContext.getPackageName(),
                    new ParceledListSlice(shortcutInfoList), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return all dynamic shortcuts from the caller app.
     *
     * <p>This API is intended to be used for examining what shortcuts are currently published.
     * Re-publishing returned {@link ShortcutInfo}s via APIs such as
     * {@link #setDynamicShortcuts(List)} may cause loss of information such as icons.
     *
     * @throws IllegalStateException when the user is locked.
     */
    @NonNull
    public List<ShortcutInfo> getDynamicShortcuts() {
        try {
            return mService.getDynamicShortcuts(mContext.getPackageName(), injectMyUserId())
                    .getList();
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return all static (manifest) shortcuts from the caller app.
     *
     * <p>This API is intended to be used for examining what shortcuts are currently published.
     * Re-publishing returned {@link ShortcutInfo}s via APIs such as
     * {@link #setDynamicShortcuts(List)} may cause loss of information such as icons.
     *
     * @throws IllegalStateException when the user is locked.
     */
    @NonNull
    public List<ShortcutInfo> getManifestShortcuts() {
        try {
            return mService.getManifestShortcuts(mContext.getPackageName(), injectMyUserId())
                    .getList();
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Publish the list of dynamic shortcuts.  If there are already dynamic or pinned shortcuts with
     * the same IDs, each mutable shortcut is updated.
     *
     * <p>This API will be rate-limited.
     *
     * @return {@code true} if the call has succeeded. {@code false} if the call is rate-limited.
     *
     * @throws IllegalArgumentException if {@link #getMaxShortcutCountPerActivity()} is exceeded,
     * or when trying to update immutable shortcuts.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public boolean addDynamicShortcuts(@NonNull List<ShortcutInfo> shortcutInfoList) {
        try {
            return mService.addDynamicShortcuts(mContext.getPackageName(),
                    new ParceledListSlice(shortcutInfoList), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Delete dynamic shortcuts by ID.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public void removeDynamicShortcuts(@NonNull List<String> shortcutIds) {
        try {
            mService.removeDynamicShortcuts(mContext.getPackageName(), shortcutIds,
                    injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Delete all dynamic shortcuts from the caller app.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public void removeAllDynamicShortcuts() {
        try {
            mService.removeAllDynamicShortcuts(mContext.getPackageName(), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return all pinned shortcuts from the caller app.
     *
     * <p>This API is intended to be used for examining what shortcuts are currently published.
     * Re-publishing returned {@link ShortcutInfo}s via APIs such as
     * {@link #setDynamicShortcuts(List)} may cause loss of information such as icons.
     *
     * @throws IllegalStateException when the user is locked.
     */
    @NonNull
    public List<ShortcutInfo> getPinnedShortcuts() {
        try {
            return mService.getPinnedShortcuts(mContext.getPackageName(), injectMyUserId())
                    .getList();
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Update all existing shortcuts with the same IDs.  Target shortcuts may be pinned and/or
     * dynamic, but they must not be immutable.
     *
     * <p>This API will be rate-limited.
     *
     * @return {@code true} if the call has succeeded. {@code false} if the call is rate-limited.
     *
     * @throws IllegalArgumentException If trying to update immutable shortcuts.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public boolean updateShortcuts(@NonNull List<ShortcutInfo> shortcutInfoList) {
        try {
            return mService.updateShortcuts(mContext.getPackageName(),
                    new ParceledListSlice(shortcutInfoList), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Disable pinned shortcuts.  For more details, see the Javadoc for the {@link ShortcutManager}
     * class.
     *
     * @throws IllegalArgumentException If trying to disable immutable shortcuts.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public void disableShortcuts(@NonNull List<String> shortcutIds) {
        try {
            mService.disableShortcuts(mContext.getPackageName(), shortcutIds,
                    /* disabledMessage =*/ null, /* disabledMessageResId =*/ 0,
                    injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * @hide old signature, kept for unit testing.
     */
    public void disableShortcuts(@NonNull List<String> shortcutIds, int disabledMessageResId) {
        try {
            mService.disableShortcuts(mContext.getPackageName(), shortcutIds,
                    /* disabledMessage =*/ null, disabledMessageResId,
                    injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * @hide old signature, kept for unit testing.
     */
    public void disableShortcuts(@NonNull List<String> shortcutIds, String disabledMessage) {
        disableShortcuts(shortcutIds, (CharSequence) disabledMessage);
    }

    /**
     * Disable pinned shortcuts, showing the user a custom error message when they try to select
     * the disabled shortcuts.
     * For more details, see the Javadoc for the {@link ShortcutManager} class.
     *
     * @throws IllegalArgumentException If trying to disable immutable shortcuts.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public void disableShortcuts(@NonNull List<String> shortcutIds, CharSequence disabledMessage) {
        try {
            mService.disableShortcuts(mContext.getPackageName(), shortcutIds,
                    disabledMessage, /* disabledMessageResId =*/ 0,
                    injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Re-enable pinned shortcuts that were previously disabled.  If the target shortcuts
     * are already enabled, this method does nothing.
     *
     * @throws IllegalArgumentException If trying to enable immutable shortcuts.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public void enableShortcuts(@NonNull List<String> shortcutIds) {
        try {
            mService.enableShortcuts(mContext.getPackageName(), shortcutIds, injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }


    /**
     * @hide old signature, kept for unit testing.
     */
    public int getMaxShortcutCountForActivity() {
        return getMaxShortcutCountPerActivity();
    }

    /**
     * Return the maximum number of static and dynamic shortcuts that each launcher icon
     * can have at a time.
     */
    public int getMaxShortcutCountPerActivity() {
        try {
            return mService.getMaxShortcutCountPerActivity(
                    mContext.getPackageName(), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return the number of times the caller app can call the rate-limited APIs
     * before the rate limit counter is reset.
     *
     * @see #getRateLimitResetTime()
     *
     * @hide
     */
    public int getRemainingCallCount() {
        try {
            return mService.getRemainingCallCount(mContext.getPackageName(), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return when the rate limit count will be reset next time, in milliseconds since the epoch.
     *
     * @see #getRemainingCallCount()
     * @see System#currentTimeMillis()
     *
     * @hide
     */
    public long getRateLimitResetTime() {
        try {
            return mService.getRateLimitResetTime(mContext.getPackageName(), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return {@code true} when rate-limiting is active for the caller app.
     *
     * <p>See the class level javadoc for details.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public boolean isRateLimitingActive() {
        try {
            return mService.getRemainingCallCount(mContext.getPackageName(), injectMyUserId())
                    == 0;
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return the max width for icons, in pixels.
     *
     * <p> Note that this method returns max width of icon's visible part. Hence, it does not take
     * into account the inset introduced by {@link AdaptiveIconDrawable}. To calculate bitmap image
     * to function as {@link AdaptiveIconDrawable}, multiply
     * 1 + 2 * {@link AdaptiveIconDrawable#getExtraInsetFraction()} to the returned size.
     */
    public int getIconMaxWidth() {
        try {
            // TODO Implement it properly using xdpi.
            return mService.getIconMaxDimensions(mContext.getPackageName(), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return the max height for icons, in pixels.
     */
    public int getIconMaxHeight() {
        try {
            // TODO Implement it properly using ydpi.
            return mService.getIconMaxDimensions(mContext.getPackageName(), injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Apps that publish shortcuts should call this method whenever the user
     * selects the shortcut containing the given ID or when the user completes
     * an action in the app that is equivalent to selecting the shortcut.
     * For more details, see the Javadoc for the {@link ShortcutManager} class
     *
     * <p>The information is accessible via {@link UsageStatsManager#queryEvents}
     * Typically, launcher apps use this information to build a prediction model
     * so that they can promote the shortcuts that are likely to be used at the moment.
     *
     * @throws IllegalStateException when the user is locked.
     */
    public void reportShortcutUsed(String shortcutId) {
        try {
            mService.reportShortcutUsed(mContext.getPackageName(), shortcutId,
                    injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Return {@code TRUE} if the app is running on a device whose default launcher supports
     * {@link #requestPinShortcut(ShortcutInfo, IntentSender)}.
     *
     * <p>The return value may change in subsequent calls if the user changes the default launcher
     * app.
     *
     * <p><b>Note:</b> See also the support library counterpart
     * {@link android.support.v4.content.pm.ShortcutManagerCompat#isRequestPinShortcutSupported(
     * Context)}, which supports Android versions lower than {@link VERSION_CODES#O} using the
     * legacy private intent {@code com.android.launcher.action.INSTALL_SHORTCUT}.
     *
     * @see #requestPinShortcut(ShortcutInfo, IntentSender)
     */
    public boolean isRequestPinShortcutSupported() {
        try {
            return mService.isRequestPinItemSupported(injectMyUserId(),
                    LauncherApps.PinItemRequest.REQUEST_TYPE_SHORTCUT);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Request to create a pinned shortcut.  The default launcher will receive this request and
     * ask the user for approval.  If the user approves it, the shortcut will be created, and
     * {@code resultIntent} will be sent. If a request is denied by the user, however, no response
     * will be sent to the caller.
     *
     * <p>Only apps with a foreground activity or a foreground service can call this method.
     * Otherwise, it'll throw {@link IllegalStateException}.
     *
     * <p>It's up to the launcher to decide how to handle previous pending requests when the same
     * package calls this API multiple times in a row. One possible strategy is to ignore any
     * previous requests.
     *
     * <p><b>Note:</b> See also the support library counterpart
     * {@link android.support.v4.content.pm.ShortcutManagerCompat#requestPinShortcut(
     * Context, ShortcutInfoCompat, IntentSender)},
     * which supports Android versions lower than {@link VERSION_CODES#O} using the
     * legacy private intent {@code com.android.launcher.action.INSTALL_SHORTCUT}.
     *
     * @param shortcut Shortcut to pin.  If an app wants to pin an existing (either static
     *     or dynamic) shortcut, then it only needs to have an ID. Although other fields don't have
     *     to be set, the target shortcut must be enabled.
     *
     *     <p>If it's a new shortcut, all the mandatory fields, such as a short label, must be
     *     set.
     * @param resultIntent If not null, this intent will be sent when the shortcut is pinned.
     *    Use {@link android.app.PendingIntent#getIntentSender()} to create an {@link IntentSender}.
     *    To avoid background execution limits, use an unexported, manifest-declared receiver.
     *    For more details, see the overview documentation for the {@link ShortcutManager} class.
     *
     * @return {@code TRUE} if the launcher supports this feature.  Note the API will return without
     *    waiting for the user to respond, so getting {@code TRUE} from this API does *not* mean
     *    the shortcut was pinned successfully.  {@code FALSE} if the launcher doesn't support this
     *    feature.
     *
     * @see #isRequestPinShortcutSupported()
     * @see IntentSender
     * @see android.app.PendingIntent#getIntentSender()
     *
     * @throws IllegalArgumentException if a shortcut with the same ID exists and is disabled.
     * @throws IllegalStateException The caller doesn't have a foreground activity or a foreground
     * service, or the device is locked.
     */
    public boolean requestPinShortcut(@NonNull ShortcutInfo shortcut,
            @Nullable IntentSender resultIntent) {
        try {
            return mService.requestPinShortcut(mContext.getPackageName(), shortcut,
                    resultIntent, injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Returns an Intent which can be used by the default launcher to pin a shortcut containing the
     * given {@link ShortcutInfo}. This method should be used by an Activity to set a result in
     * response to {@link Intent#ACTION_CREATE_SHORTCUT}.
     *
     * @param shortcut New shortcut to pin.  If an app wants to pin an existing (either dynamic
     *     or manifest) shortcut, then it only needs to have an ID, and other fields don't have to
     *     be set, in which case, the target shortcut must be enabled.
     *     If it's a new shortcut, all the mandatory fields, such as a short label, must be
     *     set.
     * @return The intent that should be set as the result for the calling activity, or
     *     <code>null</code> if the current launcher doesn't support shortcuts.
     *
     * @see Intent#ACTION_CREATE_SHORTCUT
     *
     * @throws IllegalArgumentException if a shortcut with the same ID exists and is disabled.
     */
    public Intent createShortcutResultIntent(@NonNull ShortcutInfo shortcut) {
        try {
            return mService.createShortcutResultIntent(mContext.getPackageName(), shortcut,
                    injectMyUserId());
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /**
     * Called internally when an app is considered to have come to the foreground
     * even when technically it's not.  This method resets the throttling for this package.
     * For example, when the user sends an "inline reply" on a notification, the system UI will
     * call it.
     *
     * @hide
     */
    public void onApplicationActive(@NonNull String packageName, @UserIdInt int userId) {
        try {
            mService.onApplicationActive(packageName, userId);
        } catch (RemoteException e) {
            throw e.rethrowFromSystemServer();
        }
    }

    /** @hide injection point */
    @VisibleForTesting
    protected int injectMyUserId() {
        return mContext.getUserId();
    }
}
