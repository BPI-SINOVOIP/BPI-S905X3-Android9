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

package com.android.car.media.common;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.media.browse.MediaBrowser;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.os.Bundle;
import android.os.Handler;
import android.service.media.MediaBrowserService;
import android.support.annotation.ColorInt;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Consumer;
import java.util.stream.Collectors;

/**
 * This represents a source of media content. It provides convenient methods to access media source
 * metadata, such as primary color and application name.
 *
 * <p>It also allows consumers to subscribe to its {@link android.service.media.MediaBrowserService}
 * if such service is implemented by the source.
 */
public class MediaSource {
    private static final String TAG = "MediaSource";

    /** Third-party defined application theme to use **/
    private static final String THEME_META_DATA_NAME =
            "com.google.android.gms.car.application.theme";
    /** Mark used to indicate that we couldn't find a color and the default one should be used */
    private static final int DEFAULT_COLOR = 0;
    /** Number of times we will retry obtaining the list of children of a certain node */
    private static final int CHILDREN_SUBSCRIPTION_RETRIES = 3;
    /** Time between retries while trying to obtain the list of children of a certain node */
    private static final int CHILDREN_SUBSCRIPTION_RETRY_TIME_MS = 1000;

    private final String mPackageName;
    @Nullable
    private final String mBrowseServiceClassName;
    @Nullable
    private final MediaBrowser mBrowser;
    private final Context mContext;
    private final Handler mHandler = new Handler();
    private List<Observer> mObservers = new ArrayList<>();
    private CharSequence mName;
    private String mRootNode;
    private @ColorInt int mPrimaryColor;
    private @ColorInt int mAccentColor;
    private @ColorInt int mPrimaryColorDark;

    /**
     * Custom media sources which should not be templatized.
     */
    private static final Set<String> CUSTOM_MEDIA_SOURCES = new HashSet<>();

    /**
     * An observer of this media source.
     */
    public abstract static class Observer {
        /**
         * This method is called if a successful connection to the {@link MediaBrowserService} is
         * made for this source. A connection is initiated as soon as there is at least one
         * {@link Observer} subscribed by using {@link MediaSource#subscribe(Observer)}.
         *
         * @param success true if the connection was successful or false otherwise.
         */
        protected void onBrowseConnected(boolean success) {};

        /**
         * This method is called if the connection to the {@link MediaBrowserService} is lost.
         */
        protected void onBrowseDisconnected() {};
    }

    /**
     * A subscription to a collection of items
     */
    public interface ItemsSubscription {
        /**
         * This method is called whenever media items are loaded or updated.
         *
         * @param mediaSource {@link MediaSource} these items belongs to
         * @param parentId identifier of the items parent.
         * @param items items loaded, or null if there was an error trying to load them.
         */
        void onChildrenLoaded(MediaSource mediaSource, String parentId,
                @Nullable List<MediaItemMetadata> items);
    }

    private final MediaBrowser.ConnectionCallback mConnectionCallback =
            new MediaBrowser.ConnectionCallback() {
                @Override
                public void onConnected() {
                    MediaSource.this.notify(observer -> observer.onBrowseConnected(true));
                }

                @Override
                public void onConnectionSuspended() {
                    MediaSource.this.notify(Observer::onBrowseDisconnected);
                }

                @Override
                public void onConnectionFailed() {
                    MediaSource.this.notify(observer -> observer.onBrowseConnected(false));
                }
            };

    /**
     * Creates a {@link MediaSource} for the given application package name
     */
    public MediaSource(Context context, String packageName) {
        mContext = context;
        mPackageName = packageName;
        mBrowseServiceClassName = getBrowseServiceClassName(packageName);
        if (mBrowseServiceClassName != null) {
            mBrowser = new MediaBrowser(mContext,
                    new ComponentName(mPackageName, mBrowseServiceClassName),
                    mConnectionCallback,
                    null);
        } else {
            // This media source doesn't provide browsing.
            mBrowser = null;
        }
        extractComponentInfo(mPackageName, mBrowseServiceClassName);
    }

    /**
     * @return the classname corresponding to a {@link MediaBrowserService} in the
     * media source, or null if the media source doesn't implement {@link MediaBrowserService}.
     * A non-null result doesn't imply that this service is accessible. The consumer code should
     * attempt to connect and handle rejections gracefully.
     */
    @Nullable
    private String getBrowseServiceClassName(String packageName) {
        PackageManager packageManager = mContext.getPackageManager();
        Intent intent = new Intent();
        intent.setAction(MediaBrowserService.SERVICE_INTERFACE);
        intent.setPackage(packageName);
        List<ResolveInfo> resolveInfos = packageManager.queryIntentServices(intent,
                PackageManager.GET_RESOLVED_FILTER);
        if (resolveInfos == null || resolveInfos.isEmpty()) {
            return null;
        }
        return resolveInfos.get(0).serviceInfo.name;
    }

    /**
     * Subscribes to this media source. This allows consuming browse information.

     * @return true if a subscription could be added, or false otherwise (for example, if the
     * {@link MediaBrowserService} is not available for this source.
     */
    public boolean subscribe(Observer observer) {
        if (mBrowser == null) {
            return false;
        }
        mObservers.add(observer);
        if (!mBrowser.isConnected()) {
            try {
                mBrowser.connect();
            } catch (IllegalStateException ex) {
                // Ignore: MediaBrowse could be in an intermediate state (not connected, but not
                // disconnected either.). In this situation, trying to connect again can throw
                // this exception, but there is no way to know without trying.
            }
        } else {
            observer.onBrowseConnected(true);
        }
        return true;
    }

    /**
     * Unsubscribe from this media source
     */
    public void unsubscribe(Observer observer) {
        mObservers.remove(observer);
        if (mObservers.isEmpty()) {
            // TODO(b/77640010): Review MediaBrowse disconnection.
            // Some media sources are not responding correctly to MediaBrowser#disconnect(). We
            // are keeping the connection going.
            //   mBrowser.disconnect();
        }
    }

    private Map<String, ChildrenSubscription> mChildrenSubscriptions = new HashMap<>();

    /**
     * Obtains the root node in the browse tree of this node.
     */
    public String getRoot() {
        if (mRootNode == null) {
            mRootNode = mBrowser.getRoot();
        }
        return mRootNode;
    }

    /**
     * Subscribes to changes on the list of media item children of the given parent. Multiple
     * subscription can be added to the same node. If the node has been already loaded, then all
     * new subscription will immediately obtain a copy of the last obtained list.
     *
     * @param parentId parent of the children to load, or null to indicate children of the root
     *                 node.
     * @param callback callback used to provide updates on the subscribed node.
     * @throws IllegalStateException if browsing is not available or it is not connected.
     */
    public void subscribeChildren(@Nullable String parentId, ItemsSubscription callback) {
        if (mBrowser == null) {
            throw new IllegalStateException("Browsing is not available for this source: "
                    + getName());
        }
        if (mRootNode == null && !mBrowser.isConnected()) {
            throw new IllegalStateException("Subscribing to the root node can only be done while "
                    + "connected: " + getName());
        }
        mRootNode = mBrowser.getRoot();

        String itemId = parentId != null ? parentId : mRootNode;
        ChildrenSubscription subscription = mChildrenSubscriptions.get(itemId);
        if (subscription != null) {
            subscription.add(callback);
        } else {
            subscription = new ChildrenSubscription(mBrowser, itemId);
            subscription.add(callback);
            mChildrenSubscriptions.put(itemId, subscription);
            subscription.start(CHILDREN_SUBSCRIPTION_RETRIES,
                    CHILDREN_SUBSCRIPTION_RETRY_TIME_MS);
        }
    }

    /**
     * Unsubscribes to changes on the list of media items children of the given parent
     *
     * @param parentId parent to unsubscribe, or null to unsubscribe from the root node.
     * @param callback callback to remove
     * @throws IllegalStateException if browsing is not available or it is not connected.
     */
    public void unsubscribeChildren(@Nullable String parentId, ItemsSubscription callback) {
        // If we are not connected
        if (mBrowser == null) {
            throw new IllegalStateException("Browsing is not available for this source: "
                    + getName());
        }
        if (parentId == null && mRootNode == null) {
            // If we are trying to unsubscribe from root, but we haven't determine it's Id, then
            // there is nothing we can do.
            return;
        }

        String itemId = parentId != null ? parentId : mRootNode;
        ChildrenSubscription subscription = mChildrenSubscriptions.get(itemId);
        if (subscription != null) {
            subscription.remove(callback);
            if (subscription.count() == 0) {
                subscription.stop();
                mChildrenSubscriptions.remove(itemId);
            }
        }
    }

    /**
     * {@link MediaBrowser.SubscriptionCallback} wrapper used to overcome the lack of a reliable
     * method to obtain the initial list of children of a given node.
     * <p>
     * When some 3rd party apps go through configuration changes (i.e., in the case of user-switch),
     * they leave subscriptions in an intermediate state where neither
     * {@link MediaBrowser.SubscriptionCallback#onChildrenLoaded(String, List)} nor
     * {@link MediaBrowser.SubscriptionCallback#onError(String)} are invoked.
     * <p>
     * This wrapper works around this problem by retrying the subscription a given number of times
     * if no data is received after a certain amount of time. This process is started by calling
     * {@link #start(int, int)}, passing the number of retries and delay between them as
     * parameters.
     */
    private class ChildrenSubscription extends MediaBrowser.SubscriptionCallback {
        private List<MediaItemMetadata> mItems;
        private boolean mIsDataLoaded;
        private List<ItemsSubscription> mSubscriptions = new ArrayList<>();
        private String mParentId;
        private int mRetries;
        private int mRetryDelay;
        private MediaBrowser mMediaBrowser;
        private Runnable mRetryRunnable = new Runnable() {
            @Override
            public void run() {
                if (!mIsDataLoaded) {
                    if (mRetries > 0) {
                        mRetries--;
                        mMediaBrowser.unsubscribe(mParentId);
                        mMediaBrowser.subscribe(mParentId, ChildrenSubscription.this);
                        mHandler.postDelayed(this, mRetryDelay);
                    } else {
                        mItems = null;
                        mIsDataLoaded = true;
                        notifySubscriptions();
                    }
                }
            }
        };

        /**
         * Creates a subscription to the list of children of a certain media browse item
         *
         * @param mediaBrowser {@link MediaBrowser} used to create the subscription
         * @param parentId identifier of the parent node to subscribe to
         */
        ChildrenSubscription(@NonNull MediaBrowser mediaBrowser, String parentId) {
            mParentId = parentId;
            mMediaBrowser = mediaBrowser;
        }

        /**
         * Adds a subscriber to this list of children
         */
        void add(ItemsSubscription subscription) {
            mSubscriptions.add(subscription);
            if (mIsDataLoaded) {
                subscription.onChildrenLoaded(MediaSource.this, mParentId, mItems);
            }
        }

        /**
         * Removes a subscriber previously added with {@link #add(ItemsSubscription)}
         */
        void remove(ItemsSubscription subscription) {
            mSubscriptions.remove(subscription);
        }

        /**
         * Number of subscribers currently registered
         */
        int count() {
            return mSubscriptions.size();
        }

        /**
         * Starts trying to obtain the list of children
         *
         * @param retries number of times to retry. If children are not obtained in this time then
         *                the {@link ItemsSubscription#onChildrenLoaded(MediaSource, String, List)}
         *                will be invoked with a NULL list.
         * @param retryDelay time between retries in milliseconds
         */
        void start(int retries, int retryDelay) {
            if (mIsDataLoaded) {
                notifySubscriptions();
                mMediaBrowser.subscribe(mParentId, this);
            } else {
                mRetries = retries;
                mRetryDelay = retryDelay;
                mHandler.post(mRetryRunnable);
            }
        }

        /**
         * Stops retrying
         */
        void stop() {
            mHandler.removeCallbacks(mRetryRunnable);
            mMediaBrowser.unsubscribe(mParentId);
        }

        @Override
        public void onChildrenLoaded(String parentId,
                List<MediaBrowser.MediaItem> children) {
            mHandler.removeCallbacks(mRetryRunnable);
            mItems = children.stream()
                    .map(child -> new MediaItemMetadata(child))
                    .collect(Collectors.toList());
            mIsDataLoaded = true;
            notifySubscriptions();
        }

        @Override
        public void onChildrenLoaded(String parentId, List<MediaBrowser.MediaItem> children,
                Bundle options) {
            onChildrenLoaded(parentId, children);
        }

        @Override
        public void onError(String parentId) {
            mHandler.removeCallbacks(mRetryRunnable);
            mItems = null;
            mIsDataLoaded = true;
            notifySubscriptions();
        }

        @Override
        public void onError(String parentId, Bundle options) {
            onError(parentId);
        }

        private void notifySubscriptions() {
            for (ItemsSubscription subscription : mSubscriptions) {
                subscription.onChildrenLoaded(MediaSource.this, mParentId, mItems);
            }
        }
    }

    private void extractComponentInfo(@NonNull String packageName,
            @Nullable String browseServiceClassName) {
        TypedArray ta = null;
        try {
            ApplicationInfo applicationInfo =
                    mContext.getPackageManager().getApplicationInfo(packageName,
                            PackageManager.GET_META_DATA);
            ServiceInfo serviceInfo = browseServiceClassName != null
                    ? mContext.getPackageManager().getServiceInfo(
                        new ComponentName(packageName, browseServiceClassName),
                        PackageManager.GET_META_DATA)
                    : null;

            // Get the proper app name, check service label, then application label.
            if (serviceInfo != null && serviceInfo.labelRes != 0) {
                mName = serviceInfo.loadLabel(mContext.getPackageManager());
            } else if (applicationInfo.labelRes != 0) {
                mName = applicationInfo.loadLabel(mContext.getPackageManager());
            } else {
                mName = null;
            }

            // Get the proper theme, check theme for service, then application.
            Context packageContext = mContext.createPackageContext(packageName, 0);
            int appTheme = applicationInfo.metaData != null
                    ? applicationInfo.metaData.getInt(THEME_META_DATA_NAME)
                    : 0;
            appTheme = appTheme == 0
                    ? applicationInfo.theme
                    : appTheme;
            packageContext.setTheme(appTheme);
            Resources.Theme theme = packageContext.getTheme();
            ta = theme.obtainStyledAttributes(new int[] {
                    android.R.attr.colorPrimary,
                    android.R.attr.colorAccent,
                    android.R.attr.colorPrimaryDark
            });
            mPrimaryColor = ta.getColor(0, DEFAULT_COLOR);
            mAccentColor = ta.getColor(1, DEFAULT_COLOR);
            mPrimaryColorDark = ta.getColor(2, DEFAULT_COLOR);
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Unable to update media client package attributes.", e);
            mPrimaryColor = DEFAULT_COLOR;
            mAccentColor = DEFAULT_COLOR;
            mPrimaryColorDark = DEFAULT_COLOR;
        } finally {
            if (ta != null) {
                ta.recycle();
            }
        }
    }

    /**
     * @return media source primary color, or the given default color if the source metadata
     * is not available.
     */
    public @ColorInt int getPrimaryColor(@ColorInt int defaultColor) {
        return mPrimaryColor != DEFAULT_COLOR ? mPrimaryColor : defaultColor;
    }

    /**
     * @return media source accent color, or the given default color if the source metadata
     * is not available.
     */
    public @ColorInt int getAccentColor(@ColorInt int defaultColor) {
        return mAccentColor != DEFAULT_COLOR ? mAccentColor : defaultColor;
    }

    /**
     * @return media source primary dark color, or the given default color if the source metadata
     * is not available.
     */
    public @ColorInt int getPrimaryColorDark(@ColorInt int defaultColor) {
        return mPrimaryColorDark != DEFAULT_COLOR ? mPrimaryColorDark : defaultColor;
    }

    private void notify(Consumer<Observer> notification) {
        mHandler.post(() -> {
            List<Observer> observers = new ArrayList<>(mObservers);
            for (Observer observer : observers) {
                notification.accept(observer);
            }
        });
    }

    /**
     * @return media source human readable name.
     */
    public CharSequence getName() {
        return mName;
    }

    /**
     * @return the package name that identifies this media source.
     */
    public String getPackageName() {
        return mPackageName;
    }

    /**
     * @return a {@link ComponentName} referencing this media source's {@link MediaBrowserService},
     * or NULL if this media source doesn't implement such service.
     */
    @Nullable
    public ComponentName getBrowseServiceComponentName() {
        if (mBrowseServiceClassName != null) {
            return new ComponentName(mPackageName, mBrowseServiceClassName);
        } else {
            return null;
        }
    }

    /**
     * Returns a {@link MediaController} that allows controlling this media source, or NULL
     * if the media source doesn't support browsing or the browser is not connected.
     */
    @Nullable
    public MediaController getMediaController() {
        if (mBrowser == null || !mBrowser.isConnected()) {
            return null;
        }

        MediaSession.Token token = mBrowser.getSessionToken();
        return new MediaController(mContext, token);
    }

    /**
     * Returns this media source's icon as a {@link Drawable}
     */
    public Drawable getPackageIcon() {
        try {
            return mContext.getPackageManager().getApplicationIcon(getPackageName());
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }

    /**
     * Returns this media source's icon cropped to a circle.
     */
    public Bitmap getRoundPackageIcon() {
        Drawable packageIcon = getPackageIcon();
        return packageIcon != null
                ? getRoundCroppedBitmap(drawableToBitmap(getPackageIcon()))
                : null;
    }

    /**
     * Indicates if this media source should not be templatize.
     */
    public boolean isCustom() {
        return CUSTOM_MEDIA_SOURCES.contains(mPackageName);
    }

    private Bitmap drawableToBitmap(Drawable drawable) {
        Bitmap bitmap = null;

        if (drawable instanceof BitmapDrawable) {
            BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
            if (bitmapDrawable.getBitmap() != null) {
                return bitmapDrawable.getBitmap();
            }
        }

        if (drawable.getIntrinsicWidth() <= 0 || drawable.getIntrinsicHeight() <= 0) {
            bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
        } else {
            bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                    drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        }

        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return bitmap;
    }

    private Bitmap getRoundCroppedBitmap(Bitmap bitmap) {
        Bitmap output = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(),
                Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(output);

        final int color = 0xff424242;
        final Paint paint = new Paint();
        final Rect rect = new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());

        paint.setAntiAlias(true);
        canvas.drawARGB(0, 0, 0, 0);
        paint.setColor(color);
        canvas.drawCircle(bitmap.getWidth() / 2, bitmap.getHeight() / 2,
                bitmap.getWidth() / 2f, paint);
        paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
        canvas.drawBitmap(bitmap, rect, rect, paint);
        return output;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        MediaSource that = (MediaSource) o;
        return Objects.equals(mPackageName, that.mPackageName)
                && Objects.equals(mBrowseServiceClassName, that.mBrowseServiceClassName);
    }

    /** @return the current media browser. This media browser might not be connected yet. */
    public MediaBrowser getMediaBrowser() {
        return mBrowser;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mPackageName, mBrowseServiceClassName);
    }

    @Override
    public String toString() {
        return getPackageName();
    }
}
