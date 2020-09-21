/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.server.cts.device.statsd;

import android.app.Activity;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.widget.VideoView;

public class VideoPlayerActivity extends Activity {
    private static final String TAG = VideoPlayerActivity.class.getSimpleName();

    public static final String KEY_ACTION = "action";
    public static final String ACTION_PLAY_VIDEO = "action.play_video";
    public static final String ACTION_PLAY_VIDEO_PICTURE_IN_PICTURE_MODE =
            "action.play_video_picture_in_picture_mode";

    public static final int DELAY_MILLIS = 2000;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = this.getIntent();
        if (intent == null) {
            Log.e(TAG, "Intent was null.");
            finish();
        }

        String action = intent.getStringExtra(KEY_ACTION);
        Log.i(TAG, "Starting " + action + " from foreground activity.");

        switch (action) {
            case ACTION_PLAY_VIDEO:
                playVideo();
                break;
            case ACTION_PLAY_VIDEO_PICTURE_IN_PICTURE_MODE:
                playVideo();
                this.enterPictureInPictureMode();
                break;
            default:
                Log.e(TAG, "Intent had invalid action " + action);
                finish();
        }
        delay();
    }

    private void playVideo() {
        setContentView(R.layout.activity_main);
        VideoView videoView = (VideoView)findViewById(R.id.video_player_view);
        videoView.setVideoPath("android.resource://" + getPackageName() + "/" + R.raw.colors_video);
        videoView.start();
    }

    private void delay() {
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                SystemClock.sleep(DELAY_MILLIS);
                return null;
            }
            @Override
            protected void onPostExecute(Void nothing) {
                finish();
            }
        }.execute();
    }
}



