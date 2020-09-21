/*
 * Copyright 2018 The Android Open Source Project
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

package androidx.media;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.media.MediaBrowserCompat.MediaItem;

import androidx.annotation.CallSuper;
import androidx.annotation.GuardedBy;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.media.MediaBrowserServiceCompat.BrowserRoot;
import androidx.media.MediaSession2.ControllerInfo;
import androidx.media.SessionToken2.TokenType;

import java.util.List;

/**
 * Base class for media session services, which is the service version of the {@link MediaSession2}.
 * <p>
 * It's highly recommended for an app to use this instead of {@link MediaSession2} if it wants
 * to keep media playback in the background.
 * <p>
 * Here's the benefits of using {@link MediaSessionService2} instead of
 * {@link MediaSession2}.
 * <ul>
 * <li>Another app can know that your app supports {@link MediaSession2} even when your app
 * isn't running.
 * <li>Another app can start playback of your app even when your app isn't running.
 * </ul>
 * For example, user's voice command can start playback of your app even when it's not running.
 * <p>
 * To extend this class, adding followings directly to your {@code AndroidManifest.xml}.
 * <pre>
 * &lt;service android:name="component_name_of_your_implementation" &gt;
 *   &lt;intent-filter&gt;
 *     &lt;action android:name="android.media.MediaSessionService2" /&gt;
 *   &lt;/intent-filter&gt;
 * &lt;/service&gt;</pre>
 * <p>
 * A {@link MediaSessionService2} is another form of {@link MediaSession2}. IDs shouldn't
 * be shared between the {@link MediaSessionService2} and {@link MediaSession2}. By
 * default, an empty string will be used for ID of the service. If you want to specify an ID,
 * declare metadata in the manifest as follows.
 * <pre>
 * &lt;service android:name="component_name_of_your_implementation" &gt;
 *   &lt;intent-filter&gt;
 *     &lt;action android:name="android.media.MediaSessionService2" /&gt;
 *   &lt;/intent-filter&gt;
 *   &lt;meta-data android:name="android.media.session"
 *       android:value="session_id"/&gt;
 * &lt;/service&gt;</pre>
 * <p>
 * It's recommended for an app to have a single {@link MediaSessionService2} declared in the
 * manifest. Otherwise, your app might be shown twice in the list of the Auto/Wearable, or another
 * app fails to pick the right session service when it wants to start the playback this app.
 * <p>
 * If there's conflicts with the session ID among the services, services wouldn't be available for
 * any controllers.
 * <p>
 * Topic covered here:
 * <ol>
 * <li><a href="#ServiceLifecycle">Service Lifecycle</a>
 * <li><a href="#Permissions">Permissions</a>
 * </ol>
 * <div class="special reference">
 * <a name="ServiceLifecycle"></a>
 * <h3>Service Lifecycle</h3>
 * <p>
 * Session service is bounded service. When a {@link MediaController2} is created for the
 * session service, the controller binds to the session service. {@link #onCreateSession(String)}
 * may be called after the {@link #onCreate} if the service hasn't created yet.
 * <p>
 * After the binding, session's
 * {@link MediaSession2.SessionCallback#onConnect(MediaSession2, ControllerInfo)}
 * will be called to accept or reject connection request from a controller. If the connection is
 * rejected, the controller will unbind. If it's accepted, the controller will be available to use
 * and keep binding.
 * <p>
 * When playback is started for this session service, {@link #onUpdateNotification()}
 * is called and service would become a foreground service. It's needed to keep playback after the
 * controller is destroyed. The session service becomes background service when the playback is
 * stopped.
 * <p>
 * The service is destroyed when the session is closed, or no media controller is bounded to the
 * session while the service is not running as a foreground service.
 * <a name="Permissions"></a>
 * <h3>Permissions</h3>
 * <p>
 * Any app can bind to the session service with controller, but the controller can be used only if
 * the session service accepted the connection request through
 * {@link MediaSession2.SessionCallback#onConnect(MediaSession2, ControllerInfo)}.
 */
public abstract class MediaSessionService2 extends Service {
    /**
     * This is the interface name that a service implementing a session service should say that it
     * support -- that is, this is the action it uses for its intent filter.
     */
    public static final String SERVICE_INTERFACE = "android.media.MediaSessionService2";

    /**
     * Name under which a MediaSessionService2 component publishes information about itself.
     * This meta-data must provide a string value for the ID.
     */
    public static final String SERVICE_META_DATA = "android.media.session";

    // Stub BrowserRoot for accepting any connction here.
    // See MyBrowserService#onGetRoot() for detail.
    static final BrowserRoot sDefaultBrowserRoot = new BrowserRoot(SERVICE_INTERFACE, null);

    private final MediaBrowserServiceCompat mBrowserServiceCompat;

    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private NotificationManager mNotificationManager;
    @GuardedBy("mLock")
    private Intent mStartSelfIntent;
    @GuardedBy("mLock")
    private boolean mIsRunningForeground;
    @GuardedBy("mLock")
    private MediaSession2 mSession;

    public MediaSessionService2() {
        super();
        mBrowserServiceCompat = createBrowserServiceCompat();
    }

    MediaBrowserServiceCompat createBrowserServiceCompat() {
        return new MyBrowserService();
    }

    /**
     * Default implementation for {@link MediaSessionService2} to initialize session service.
     * <p>
     * Override this method if you need your own initialization. Derived classes MUST call through
     * to the super class's implementation of this method.
     */
    @CallSuper
    @Override
    public void onCreate() {
        super.onCreate();
        mBrowserServiceCompat.attachToBaseContext(this);
        mBrowserServiceCompat.onCreate();
        SessionToken2 token = new SessionToken2(this,
                new ComponentName(getPackageName(), getClass().getName()));
        if (token.getType() != getSessionType()) {
            throw new RuntimeException("Expected session type " + getSessionType()
                    + " but was " + token.getType());
        }
        MediaSession2 session = onCreateSession(token.getId());
        synchronized (mLock) {
            mNotificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            mStartSelfIntent = new Intent(this, getClass());
            mSession = session;
            if (mSession == null || !token.getId().equals(mSession.getToken().getId())) {
                throw new RuntimeException("Expected session with id " + token.getId()
                        + ", but got " + mSession);
            }
            mBrowserServiceCompat.setSessionToken(mSession.getToken().getSessionCompatToken());
        }
    }

    @TokenType int getSessionType() {
        return SessionToken2.TYPE_SESSION_SERVICE;
    }

    /**
     * Called when another app requested to start this service to get {@link MediaSession2}.
     * <p>
     * Session service will accept or reject the connection with the
     * {@link MediaSession2.SessionCallback} in the created session.
     * <p>
     * Service wouldn't run if {@code null} is returned or session's ID doesn't match with the
     * expected ID that you've specified through the AndroidManifest.xml.
     * <p>
     * This method will be called on the main thread.
     *
     * @param sessionId session id written in the AndroidManifest.xml.
     * @return a new session
     * @see MediaSession2.Builder
     * @see #getSession()
     */
    public @NonNull abstract MediaSession2 onCreateSession(String sessionId);

    /**
     * Called when the playback state of this session is changed so notification needs update.
     * Override this method to show or cancel your own notification UI.
     * <p>
     * With the notification returned here, the service become foreground service when the playback
     * is started. It becomes background service after the playback is stopped.
     *
     * @return a {@link MediaNotification}. If it's {@code null}, notification wouldn't be shown.
     */
    public @Nullable MediaNotification onUpdateNotification() {
        return null;
    }

    /**
     * Get instance of the {@link MediaSession2} that you've previously created with the
     * {@link #onCreateSession} for this service.
     * <p>
     * This may be {@code null} before the {@link #onCreate()} is finished.
     *
     * @return created session
     */
    public final @Nullable MediaSession2 getSession() {
        synchronized (mLock) {
            return mSession;
        }
    }

    /**
     * Default implementation for {@link MediaSessionService2} to handle incoming binding
     * request. If the request is for getting the session, the intent will have action
     * {@link #SERVICE_INTERFACE}.
     * <p>
     * Override this method if this service also needs to handle binder requests other than
     * {@link #SERVICE_INTERFACE}. Derived classes MUST call through to the super class's
     * implementation of this method.
     *
     * @param intent
     * @return Binder
     */
    @CallSuper
    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        if (MediaSessionService2.SERVICE_INTERFACE.equals(intent.getAction())
                || MediaBrowserServiceCompat.SERVICE_INTERFACE.equals(intent.getAction())) {
            // Change the intent action for browser service.
            Intent browserServiceIntent = new Intent(intent);
            browserServiceIntent.setAction(MediaSessionService2.SERVICE_INTERFACE);
            return mBrowserServiceCompat.onBind(intent);
        }
        return null;
    }

    MediaBrowserServiceCompat getServiceCompat() {
        return mBrowserServiceCompat;
    }

    /**
     * Returned by {@link #onUpdateNotification()} for making session service foreground service
     * to keep playback running in the background. It's highly recommended to show media style
     * notification here.
     */
    public static class MediaNotification {
        private final int mNotificationId;
        private final Notification mNotification;

        /**
         * Default constructor
         *
         * @param notificationId notification id to be used for
         *      {@link NotificationManager#notify(int, Notification)}.
         * @param notification a notification to make session service foreground service. Media
         *      style notification is recommended here.
         */
        public MediaNotification(int notificationId, @NonNull Notification notification) {
            if (notification == null) {
                throw new IllegalArgumentException("notification shouldn't be null");
            }
            mNotificationId = notificationId;
            mNotification = notification;
        }

        /**
         * Gets the id of the id.
         *
         * @return the notification id
         */
        public int getNotificationId() {
            return mNotificationId;
        }

        /**
         * Gets the notification.
         *
         * @return the notification
         */
        public @NonNull Notification getNotification() {
            return mNotification;
        }
    }

    private static class MyBrowserService extends MediaBrowserServiceCompat {
        @Override
        public BrowserRoot onGetRoot(String clientPackageName, int clientUid, Bundle rootHints) {
            // Returns *stub* root here. Here's the reason.
            //   1. A non-null BrowserRoot should be returned here to keep the binding
            //   2. MediaSessionService2 is defined as the simplified version of the library
            //      service with no browsing feature, so shouldn't allow MediaBrowserServiceCompat
            //      specific operations.
            return sDefaultBrowserRoot;
        }

        @Override
        public void onLoadChildren(String parentId, Result<List<MediaItem>> result) {
            // Disallow loading children.
        }
    }
}
