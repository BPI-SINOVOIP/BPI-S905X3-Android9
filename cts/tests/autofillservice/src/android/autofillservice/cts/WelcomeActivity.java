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
package android.autofillservice.cts;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.os.Bundle;
import androidx.annotation.Nullable;
import android.support.test.uiautomator.UiObject2;
import android.text.TextUtils;
import android.util.Log;
import android.widget.TextView;

/**
 * Activity that displays a "Welcome USER" message after login.
 */
public class WelcomeActivity extends AbstractAutoFillActivity {

    private static WelcomeActivity sInstance;

    private static final String TAG = "WelcomeActivity";

    static final String EXTRA_MESSAGE = "message";
    static final String ID_WELCOME = "welcome";

    private static int sPendingIntentId;
    private static PendingIntent sPendingIntent;

    private TextView mWelcome;

    public WelcomeActivity() {
        sInstance = this;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.welcome_activity);

        mWelcome = (TextView) findViewById(R.id.welcome);

        final Intent intent = getIntent();
        final String message = intent.getStringExtra(EXTRA_MESSAGE);

        if (!TextUtils.isEmpty(message)) {
            mWelcome.setText(message);
        }

        Log.d(TAG, "Message: " + message);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        Log.v(TAG, "Setting sInstance to null onDestroy()");
        sInstance = null;
    }

    static void finishIt(UiBot uiBot) {
        if (sInstance != null) {
            Log.d(TAG, "So long and thanks for all the fish!");
            sInstance.finish();
            uiBot.assertGoneByRelativeId(ID_WELCOME, Timeouts.ACTIVITY_RESURRECTION);
        }
        if (sPendingIntent != null) {
            sPendingIntent.cancel();
        }
    }

    // TODO: reuse in other places
    static void assertShowingDefaultMessage(UiBot uiBot) throws Exception {
        assertShowing(uiBot, null);
    }

    // TODO: reuse in other places
    static void assertShowing(UiBot uiBot, @Nullable String expectedMessage) throws Exception {
        final UiObject2 activity = uiBot.assertShownByRelativeId(ID_WELCOME);
        if (expectedMessage == null) {
            expectedMessage = "Welcome to the jungle!";
        }
        assertWithMessage("wrong text on '%s'", activity).that(activity.getText())
                .isEqualTo(expectedMessage);
    }

    public static IntentSender createSender(Context context, String message) {
        if (sPendingIntent != null) {
            throw new IllegalArgumentException("Already have pending intent (id="
                    + sPendingIntentId + "): " + sPendingIntent);
        }
        ++sPendingIntentId;
        Log.v(TAG, "createSender: id=" + sPendingIntentId + " message=" + message);
        final Intent intent = new Intent(context, WelcomeActivity.class)
                .putExtra(EXTRA_MESSAGE, message)
                .setFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
        sPendingIntent = PendingIntent.getActivity(context, sPendingIntentId, intent, 0);
        return sPendingIntent.getIntentSender();
    }
}
