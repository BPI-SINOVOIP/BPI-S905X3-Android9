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

package com.example.android.wearable.watchface;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.support.v4.app.ActivityCompat;
import android.support.wearable.provider.WearableCalendarContract;
import android.support.wearable.watchface.CanvasWatchFaceService;
import android.support.wearable.watchface.WatchFaceService;
import android.support.wearable.watchface.WatchFaceStyle;
import android.text.DynamicLayout;
import android.text.Editable;
import android.text.Html;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.TextPaint;
import android.text.format.DateUtils;
import android.util.Log;
import android.view.SurfaceHolder;

/**
 * Proof of concept sample watch face that demonstrates how a watch face can load calendar data.
 */
public class CalendarWatchFaceService extends CanvasWatchFaceService {
    private static final String TAG = "CalendarWatchFace";

    @Override
    public Engine onCreateEngine() {
        return new Engine();
    }

    private class Engine extends CanvasWatchFaceService.Engine {

        static final int BACKGROUND_COLOR = Color.BLACK;
        static final int FOREGROUND_COLOR = Color.WHITE;
        static final int TEXT_SIZE = 25;
        static final int MSG_LOAD_MEETINGS = 0;

        /** Editable string containing the text to draw with the number of meetings in bold. */
        final Editable mEditable = new SpannableStringBuilder();

        /** Width specified when {@link #mLayout} was created. */
        int mLayoutWidth;

        /** Layout to wrap {@link #mEditable} onto multiple lines. */
        DynamicLayout mLayout;

        /** Paint used to draw text. */
        final TextPaint mTextPaint = new TextPaint();

        int mNumMeetings;
        private boolean mCalendarPermissionApproved;
        private String mCalendarNotApprovedMessage;

        private AsyncTask<Void, Void, Integer> mLoadMeetingsTask;

        private boolean mIsReceiverRegistered;

        /** Handler to load the meetings once a minute in interactive mode. */
        final Handler mLoadMeetingsHandler = new Handler() {
            @Override
            public void handleMessage(Message message) {
                switch (message.what) {
                    case MSG_LOAD_MEETINGS:

                        cancelLoadMeetingTask();

                        // Loads meetings.
                        if (mCalendarPermissionApproved) {
                            mLoadMeetingsTask = new LoadMeetingsTask();
                            mLoadMeetingsTask.execute();
                        }
                        break;
                }
            }
        };

        private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (Intent.ACTION_PROVIDER_CHANGED.equals(intent.getAction())
                        && WearableCalendarContract.CONTENT_URI.equals(intent.getData())) {
                    mLoadMeetingsHandler.sendEmptyMessage(MSG_LOAD_MEETINGS);
                }
            }
        };

        @Override
        public void onCreate(SurfaceHolder holder) {
            super.onCreate(holder);
            Log.d(TAG, "onCreate");

            mCalendarNotApprovedMessage =
                    getResources().getString(R.string.calendar_permission_not_approved);

            /* Accepts tap events to allow permission changes by user. */
            setWatchFaceStyle(new WatchFaceStyle.Builder(CalendarWatchFaceService.this)
                    .setCardPeekMode(WatchFaceStyle.PEEK_MODE_VARIABLE)
                    .setBackgroundVisibility(WatchFaceStyle.BACKGROUND_VISIBILITY_INTERRUPTIVE)
                    .setShowSystemUiTime(false)
                    .setAcceptsTapEvents(true)
                    .build());

            mTextPaint.setColor(FOREGROUND_COLOR);
            mTextPaint.setTextSize(TEXT_SIZE);

            // Enables app to handle 23+ (M+) style permissions.
            mCalendarPermissionApproved =
                    ActivityCompat.checkSelfPermission(
                            getApplicationContext(),
                            Manifest.permission.READ_CALENDAR) == PackageManager.PERMISSION_GRANTED;

            if (mCalendarPermissionApproved) {
                mLoadMeetingsHandler.sendEmptyMessage(MSG_LOAD_MEETINGS);
            }
        }

        @Override
        public void onDestroy() {
            mLoadMeetingsHandler.removeMessages(MSG_LOAD_MEETINGS);
            super.onDestroy();
        }

        /*
         * Captures tap event (and tap type) and increments correct tap type total.
         */
        @Override
        public void onTapCommand(int tapType, int x, int y, long eventTime) {
            if (Log.isLoggable(TAG, Log.DEBUG)) {
                Log.d(TAG, "Tap Command: " + tapType);
            }

            // Ignore lint error (fixed in wearable support library 1.4)
            if (tapType == WatchFaceService.TAP_TYPE_TAP && !mCalendarPermissionApproved) {
                Intent permissionIntent = new Intent(
                        getApplicationContext(),
                        CalendarWatchFacePermissionActivity.class);
                permissionIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(permissionIntent);
            }
        }

        @Override
        public void onDraw(Canvas canvas, Rect bounds) {
            // Create or update mLayout if necessary.
            if (mLayout == null || mLayoutWidth != bounds.width()) {
                mLayoutWidth = bounds.width();
                mLayout = new DynamicLayout(mEditable, mTextPaint, mLayoutWidth,
                        Layout.Alignment.ALIGN_NORMAL, 1 /* spacingMult */, 0 /* spacingAdd */,
                        false /* includePad */);
            }

            // Update the contents of mEditable.
            mEditable.clear();

            if (mCalendarPermissionApproved) {
                mEditable.append(Html.fromHtml(getResources().getQuantityString(
                        R.plurals.calendar_meetings, mNumMeetings, mNumMeetings)));
            } else {
                mEditable.append(Html.fromHtml(mCalendarNotApprovedMessage));
            }

            // Draw the text on a solid background.
            canvas.drawColor(BACKGROUND_COLOR);
            mLayout.draw(canvas);
        }

        @Override
        public void onVisibilityChanged(boolean visible) {
            Log.d(TAG, "onVisibilityChanged()");
            super.onVisibilityChanged(visible);
            if (visible) {

                // Enables app to handle 23+ (M+) style permissions.
                mCalendarPermissionApproved = ActivityCompat.checkSelfPermission(
                        getApplicationContext(),
                        Manifest.permission.READ_CALENDAR) == PackageManager.PERMISSION_GRANTED;

                if (mCalendarPermissionApproved) {
                    IntentFilter filter = new IntentFilter(Intent.ACTION_PROVIDER_CHANGED);
                    filter.addDataScheme("content");
                    filter.addDataAuthority(WearableCalendarContract.AUTHORITY, null);
                    registerReceiver(mBroadcastReceiver, filter);
                    mIsReceiverRegistered = true;

                    mLoadMeetingsHandler.sendEmptyMessage(MSG_LOAD_MEETINGS);
                }
            } else {
                if (mIsReceiverRegistered) {
                    unregisterReceiver(mBroadcastReceiver);
                    mIsReceiverRegistered = false;
                }
                mLoadMeetingsHandler.removeMessages(MSG_LOAD_MEETINGS);
            }
        }

        private void onMeetingsLoaded(Integer result) {
            if (result != null) {
                mNumMeetings = result;
                invalidate();
            }
        }

        private void cancelLoadMeetingTask() {
            if (mLoadMeetingsTask != null) {
                mLoadMeetingsTask.cancel(true);
            }
        }

        /**
         * Asynchronous task to load the meetings from the content provider and report the number of
         * meetings back via {@link #onMeetingsLoaded}.
         */
        private class LoadMeetingsTask extends AsyncTask<Void, Void, Integer> {
            private PowerManager.WakeLock mWakeLock;

            @Override
            protected Integer doInBackground(Void... voids) {
                PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
                mWakeLock = powerManager.newWakeLock(
                        PowerManager.PARTIAL_WAKE_LOCK, "CalendarWatchFaceWakeLock");
                mWakeLock.acquire();

                long begin = System.currentTimeMillis();
                Uri.Builder builder =
                        WearableCalendarContract.Instances.CONTENT_URI.buildUpon();
                ContentUris.appendId(builder, begin);
                ContentUris.appendId(builder, begin + DateUtils.DAY_IN_MILLIS);
                final Cursor cursor = getContentResolver().query(builder.build(),
                        null, null, null, null);
                int numMeetings = cursor.getCount();

                Log.d(TAG, "Num meetings: " + numMeetings);

                return numMeetings;
            }

            @Override
            protected void onPostExecute(Integer result) {
                releaseWakeLock();
                onMeetingsLoaded(result);
            }

            @Override
            protected void onCancelled() {
                releaseWakeLock();
            }

            private void releaseWakeLock() {
                if (mWakeLock != null) {
                    mWakeLock.release();
                    mWakeLock = null;
                }
            }
        }
    }
}