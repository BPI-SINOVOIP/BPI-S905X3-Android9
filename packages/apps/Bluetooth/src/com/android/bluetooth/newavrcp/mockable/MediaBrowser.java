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

package com.android.bluetooth.avrcp;

import android.content.ComponentName;
import android.content.Context;
import android.media.session.MediaSession;
import android.os.Bundle;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;

/**
 * Provide a mockable interface in order to test classes that use MediaBrowser.
 * We need this class due to the fact that the MediaController class is marked as final and
 * there is no way to currently mock final classes in Android. Once this is possible this class
 * can be deleted.
 */
public class MediaBrowser {
    android.media.browse.MediaBrowser mDelegate;

    /**
     * Wrap a real MediaBrowser object
     */
    public MediaBrowser(android.media.browse.MediaBrowser delegate) {
        mDelegate = delegate;
    }

    /**
     * Create a real MediaBrowser object and wrap it
     */
    public MediaBrowser(Context context, ComponentName serviceComponent,
            ConnectionCallback callback, Bundle rootHints) {
        mDelegate = new android.media.browse.MediaBrowser(context, serviceComponent, callback,
                rootHints);
    }

    /**
     * Wrapper for MediaBrowser.ConnectionCallback
     */
    public abstract static class ConnectionCallback extends
            android.media.browse.MediaBrowser.ConnectionCallback {}

    /**
     * Wrapper for MediaBrowser.ItemCallback
     */
    public abstract static class ItemCallback extends
            android.media.browse.MediaBrowser.ItemCallback {}

    /**
     * Wrapper for MediaBrowser.SubscriptionCallback
     */
    public abstract static class SubscriptionCallback extends
            android.media.browse.MediaBrowser.SubscriptionCallback {}

    /**
     * Wrapper for MediaBrowser.connect()
     */
    public void connect() {
        mDelegate.connect();
    }

    /**
     * Wrapper for MediaBrowser.disconnect()
     */
    public void disconnect() {
        mDelegate.disconnect();
    }

    /**
     * Wrapper for MediaBrowser.getExtras()
     */
    public Bundle getExtras() {
        return mDelegate.getExtras();
    }

    /**
     * Wrapper for MediaBrowser.getItem(String mediaId, ItemCallback callback)
     */
    public void getItem(String mediaId, ItemCallback callback) {
        mDelegate.getItem(mediaId, callback);
    }

    /**
     * Wrapper for MediaBrowser.getRoot()
     */
    public String getRoot() {
        return mDelegate.getRoot();
    }

    /**
     * Wrapper for MediaBrowser.getServiceComponent()
     */
    public ComponentName getServiceComponent() {
        return mDelegate.getServiceComponent();
    }

    /**
     * Wrapper for MediaBrowser.getSessionToken()
     */
    public MediaSession.Token getSessionToken() {
        return mDelegate.getSessionToken();
    }
    /**
     * Wrapper for MediaBrowser.isConnected()
     */
    public boolean isConnected() {
        return mDelegate.isConnected();
    }

    /**
     * Wrapper for MediaBrowser.subscribe(String parentId, Bundle options,
     * SubscriptionCallback callback)
     */
    public void subscribe(String parentId, Bundle options, SubscriptionCallback callback) {
        mDelegate.subscribe(parentId, options, callback);
    }

    /**
     * Wrapper for MediaBrowser.subscribe(String parentId, SubscriptionCallback callback)
     */
    public void subscribe(String parentId, SubscriptionCallback callback) {
        mDelegate.subscribe(parentId, callback);
    }

    /**
     * Wrapper for MediaBrowser.unsubscribe(String parentId)
     */
    public void unsubscribe(String parentId) {
        mDelegate.unsubscribe(parentId);
    }


    /**
     * Wrapper for MediaBrowser.unsubscribe(String parentId, SubscriptionCallback callback)
     */
    public void unsubscribe(String parentId, SubscriptionCallback callback) {
        mDelegate.unsubscribe(parentId, callback);
    }

    /**
     * A function that allows Mockito to capture the constructor arguments when using
     * MediaBrowserFactory.make()
     */
    @VisibleForTesting
    public void testInit(Context context, ComponentName serviceComponent,
            ConnectionCallback callback, Bundle rootHints) {
        // This is only used by Mockito to capture the constructor arguments on creation
        Log.wtfStack("AvrcpMockMediaBrowser", "This function should never be called");
    }
}
