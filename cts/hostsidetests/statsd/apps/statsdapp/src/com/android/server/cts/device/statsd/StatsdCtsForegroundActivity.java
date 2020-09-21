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

import com.android.server.cts.device.statsd.AtomTests;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Point;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

/** An activity (to be run as a foreground process) which performs one of a number of actions. */
public class StatsdCtsForegroundActivity extends Activity {
    private static final String TAG = StatsdCtsForegroundActivity.class.getSimpleName();

    public static final String KEY_ACTION = "action";
    public static final String ACTION_END_IMMEDIATELY = "action.end_immediately";
    public static final String ACTION_SLEEP_WHILE_TOP = "action.sleep_top";
    public static final String ACTION_SHOW_APPLICATION_OVERLAY = "action.show_application_overlay";
    public static final String ACTION_CRASH = "action.crash";

    public static final int SLEEP_OF_ACTION_SLEEP_WHILE_TOP = 2_000;
    public static final int SLEEP_OF_ACTION_SHOW_APPLICATION_OVERLAY = 2_000;

    @Override
    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        Intent intent = this.getIntent();
        if (intent == null) {
            Log.e(TAG, "Intent was null.");
            finish();
        }

        String action = intent.getStringExtra(KEY_ACTION);
        Log.i(TAG, "Starting " + action + " from foreground activity.");

        switch (action) {
            case ACTION_END_IMMEDIATELY:
                finish();
                break;
            case ACTION_SLEEP_WHILE_TOP:
                doSleepWhileTop();
                break;
            case ACTION_SHOW_APPLICATION_OVERLAY:
                doShowApplicationOverlay();
                break;
            case ACTION_CRASH:
                doCrash();
                break;
            default:
                Log.e(TAG, "Intent had invalid action " + action);
                finish();
        }
    }

    /** Does nothing, but asynchronously. */
    private void doSleepWhileTop() {
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                AtomTests.sleep(SLEEP_OF_ACTION_SLEEP_WHILE_TOP);
                return null;
            }

            @Override
            protected void onPostExecute(Void nothing) {
                finish();
            }
        }.execute();
    }

    private void doShowApplicationOverlay() {
        // Adapted from BatteryStatsBgVsFgActions.java.
        final WindowManager wm = getSystemService(WindowManager.class);
        Point size = new Point();
        wm.getDefaultDisplay().getSize(size);

        WindowManager.LayoutParams wmlp = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE);
        wmlp.width = size.x / 4;
        wmlp.height = size.y / 4;
        wmlp.gravity = Gravity.CENTER | Gravity.LEFT;
        wmlp.setTitle(getPackageName());

        ViewGroup.LayoutParams vglp = new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT);

        View v = new View(this);
        v.setBackgroundColor(Color.GREEN);
        v.setLayoutParams(vglp);
        wm.addView(v, wmlp);

        // The overlay continues long after the finish. The following is just to end the activity.
        AtomTests.sleep(SLEEP_OF_ACTION_SHOW_APPLICATION_OVERLAY);
        finish();
    }

    private void doCrash() {
        Log.e(TAG, "About to crash the app with 1/0 " + (long)1/0);
    }
}
