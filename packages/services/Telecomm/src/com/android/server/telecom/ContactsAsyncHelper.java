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

package com.android.server.telecom;

import android.app.Notification;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.telecom.Log;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;

// TODO: Needed for move to system service: import com.android.internal.R;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

/**
 * Helper class for loading contacts photo asynchronously.
 */
public class ContactsAsyncHelper {
    private static final String LOG_TAG = ContactsAsyncHelper.class.getSimpleName();

    /**
     * Interface for a WorkerHandler result return.
     */
    public interface OnImageLoadCompleteListener {
        /**
         * Called when the image load is complete.
         *
         * @param token Integer passed in {@link ContactsAsyncHelper#startObtainPhotoAsync(int,
         * Context, Uri, OnImageLoadCompleteListener, Object)}.
         * @param photo Drawable object obtained by the async load.
         * @param photoIcon Bitmap object obtained by the async load.
         * @param cookie Object passed in {@link ContactsAsyncHelper#startObtainPhotoAsync(int,
         * Context, Uri, OnImageLoadCompleteListener, Object)}. Can be null iff. the original
         * cookie is null.
         */
        public void onImageLoadComplete(int token, Drawable photo, Bitmap photoIcon,
                Object cookie);
    }

    /**
     * Interface to enable stubbing of the call to openInputStream
     */
    public interface ContentResolverAdapter {
        InputStream openInputStream(Context context, Uri uri) throws FileNotFoundException;
    }

    // constants
    private static final int EVENT_LOAD_IMAGE = 1;

    /** Handler run on a worker thread to load photo asynchronously. */
    private Handler mThreadHandler;
    private final ContentResolverAdapter mContentResolverAdapter;

    public ContactsAsyncHelper(ContentResolverAdapter contentResolverAdapter) {
        mContentResolverAdapter = contentResolverAdapter;
    }

    private static final class WorkerArgs {
        public Context context;
        public Uri displayPhotoUri;
        public Drawable photo;
        public Bitmap photoIcon;
        public Object cookie;
        public OnImageLoadCompleteListener listener;
    }

    /**
     * Thread worker class that handles the task of opening the stream and loading
     * the images.
     */
    private class WorkerHandler extends Handler {
        public WorkerHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            WorkerArgs args = (WorkerArgs) msg.obj;

            switch (msg.arg1) {
                case EVENT_LOAD_IMAGE:
                    InputStream inputStream = null;
                    try {
                        try {
                            inputStream = mContentResolverAdapter.openInputStream(
                                    args.context, args.displayPhotoUri);
                        } catch (Exception e) {
                            Log.e(this, e, "Error opening photo input stream");
                        }

                        if (inputStream != null) {
                            args.photo = Drawable.createFromStream(inputStream,
                                    args.displayPhotoUri.toString());

                            // This assumes Drawable coming from contact database is usually
                            // BitmapDrawable and thus we can have (down)scaled version of it.
                            args.photoIcon = getPhotoIconWhenAppropriate(args.context, args.photo);

                            Log.d(this, "Loading image: " + msg.arg1 +
                                    " token: " + msg.what + " image URI: " + args.displayPhotoUri);
                        } else {
                            args.photo = null;
                            args.photoIcon = null;
                            Log.d(this, "Problem with image: " + msg.arg1 +
                                    " token: " + msg.what + " image URI: " + args.displayPhotoUri +
                                    ", using default image.");
                        }
                    } finally {
                        if (inputStream != null) {
                            try {
                                inputStream.close();
                            } catch (IOException e) {
                                Log.e(this, e, "Unable to close input stream.");
                            }
                        }
                    }

                    // Listener will synchronize as needed
                    Log.d(this, "Notifying listener: " + args.listener.toString() +
                            " image: " + args.displayPhotoUri + " completed");
                    args.listener.onImageLoadComplete(msg.what, args.photo, args.photoIcon,
                                args.cookie);
                    break;
                default:
                    break;
            }
        }

        /**
         * Returns a Bitmap object suitable for {@link Notification}'s large icon. This might
         * return null when the given Drawable isn't BitmapDrawable, or if the system fails to
         * create a scaled Bitmap for the Drawable.
         */
        private Bitmap getPhotoIconWhenAppropriate(Context context, Drawable photo) {
            if (!(photo instanceof BitmapDrawable)) {
                return null;
            }
            int iconSize = context.getResources()
                    .getDimensionPixelSize(R.dimen.notification_icon_size);
            Bitmap orgBitmap = ((BitmapDrawable) photo).getBitmap();
            int orgWidth = orgBitmap.getWidth();
            int orgHeight = orgBitmap.getHeight();
            int longerEdge = orgWidth > orgHeight ? orgWidth : orgHeight;
            // We want downscaled one only when the original icon is too big.
            if (longerEdge > iconSize) {
                float ratio = ((float) longerEdge) / iconSize;
                int newWidth = (int) (orgWidth / ratio);
                int newHeight = (int) (orgHeight / ratio);
                // If the longer edge is much longer than the shorter edge, the latter may
                // become 0 which will cause a crash.
                if (newWidth <= 0 || newHeight <= 0) {
                    Log.w(this, "Photo icon's width or height become 0.");
                    return null;
                }

                // It is sure ratio >= 1.0f in any case and thus the newly created Bitmap
                // should be smaller than the original.
                return Bitmap.createScaledBitmap(orgBitmap, newWidth, newHeight, true);
            } else {
                return orgBitmap;
            }
        }
    }

    /**
     * Starts an asynchronous image load. After finishing the load,
     * {@link OnImageLoadCompleteListener#onImageLoadComplete(int, Drawable, Bitmap, Object)}
     * will be called.
     *
     * @param token Arbitrary integer which will be returned as the first argument of
     * {@link OnImageLoadCompleteListener#onImageLoadComplete(int, Drawable, Bitmap, Object)}
     * @param context Context object used to do the time-consuming operation.
     * @param displayPhotoUri Uri to be used to fetch the photo
     * @param listener Callback object which will be used when the asynchronous load is done.
     * Can be null, which means only the asynchronous load is done while there's no way to
     * obtain the loaded photos.
     * @param cookie Arbitrary object the caller wants to remember, which will become the
     * fourth argument of {@link OnImageLoadCompleteListener#onImageLoadComplete(int, Drawable,
     * Bitmap, Object)}. Can be null, at which the callback will also has null for the argument.
     */
    public void startObtainPhotoAsync(int token, Context context, Uri displayPhotoUri,
            OnImageLoadCompleteListener listener, Object cookie) {
        ensureAsyncHandlerStarted();

        // in case the source caller info is null, the URI will be null as well.
        // just update using the placeholder image in this case.
        if (displayPhotoUri == null) {
            Log.wtf(LOG_TAG, "Uri is missing");
            return;
        }

        // Added additional Cookie field in the callee to handle arguments
        // sent to the callback function.

        // setup arguments
        WorkerArgs args = new WorkerArgs();
        args.cookie = cookie;
        args.context = context;
        args.displayPhotoUri = displayPhotoUri;
        args.listener = listener;

        // setup message arguments
        Message msg = mThreadHandler.obtainMessage(token);
        msg.arg1 = EVENT_LOAD_IMAGE;
        msg.obj = args;

        Log.d(LOG_TAG, "Begin loading image: " + args.displayPhotoUri +
                ", displaying default image for now.");

        // notify the thread to begin working
        mThreadHandler.sendMessage(msg);
    }

    private void ensureAsyncHandlerStarted() {
        if (mThreadHandler == null) {
            HandlerThread thread = new HandlerThread("ContactsAsyncWorker");
            thread.start();
            mThreadHandler = new WorkerHandler(thread.getLooper());
        }
    }
}
