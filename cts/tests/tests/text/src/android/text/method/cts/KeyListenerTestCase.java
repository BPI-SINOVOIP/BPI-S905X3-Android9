/*
 * Copyright (C) 2013 The Android Open Source Project
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

package android.text.method.cts;

import static android.provider.Settings.System.TEXT_AUTO_CAPS;

import android.app.AppOpsManager;
import android.app.Instrumentation;
import android.content.ContentResolver;
import android.content.Context;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.text.cts.R;
import android.text.method.KeyListener;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.EditText;

import com.android.compatibility.common.util.PollingCheck;
import com.android.compatibility.common.util.SystemUtil;

import org.junit.Before;
import org.junit.Rule;

import java.io.IOException;

/**
 * Base class for various KeyListener tests.
 */
public abstract class KeyListenerTestCase {
    private static final String TAG = "KeyListenerTestCase";

    protected KeyListenerCtsActivity mActivity;
    protected Instrumentation mInstrumentation;
    protected EditText mTextView;
    private int mAutoCapSetting;

    @Rule
    public ActivityTestRule<KeyListenerCtsActivity> mActivityRule =
            new ActivityTestRule<>(KeyListenerCtsActivity.class);

    @Before
    public void setup() throws IOException {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mActivity = mActivityRule.getActivity();
        mTextView = mActivity.findViewById(R.id.keylistener_textview);

        PollingCheck.waitFor(5000, mActivity::hasWindowFocus);
    }

    protected void enableAutoCapSettings() throws IOException {
        grantWriteSettingsPermission();
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        final Context context = instrumentation.getContext();
        instrumentation.runOnMainSync(() -> {
            final ContentResolver resolver = context.getContentResolver();
            mAutoCapSetting = Settings.System.getInt(resolver, TEXT_AUTO_CAPS, 1);
            try {
                Settings.System.putInt(resolver, TEXT_AUTO_CAPS, 1);
            } catch (SecurityException e) {
                Log.e(TAG, "Cannot set TEXT_AUTO_CAPS to 1", e);
                // ignore
            }
        });
        instrumentation.waitForIdleSync();
    }

    protected void resetAutoCapSettings() throws IOException {
        grantWriteSettingsPermission();
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        final Context context = instrumentation.getContext();
        instrumentation.runOnMainSync(() -> {
            final ContentResolver resolver = context.getContentResolver();
            try {
                Settings.System.putInt(resolver, TEXT_AUTO_CAPS, mAutoCapSetting);
            } catch (SecurityException e) {
                Log.e(TAG, "Cannot set TEXT_AUTO_CAPS to previous value", e);
                // ignore
            }
        });
        instrumentation.waitForIdleSync();
    }

    /**
     * Synchronously sets mTextView's key listener on the UI thread.
     */
    protected void setKeyListenerSync(final KeyListener keyListener) {
        mInstrumentation.runOnMainSync(() -> mTextView.setKeyListener(keyListener));
        mInstrumentation.waitForIdleSync();
    }

    protected static KeyEvent getKey(int keycode, int metaState) {
        long currentTime = System.currentTimeMillis();
        return new KeyEvent(currentTime, currentTime, KeyEvent.ACTION_DOWN, keycode,
                0 /* repeat */, metaState);
    }

    private void grantWriteSettingsPermission() throws IOException {
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(),
                "appops set " + mActivity.getPackageName() + " "
                        + AppOpsManager.OPSTR_WRITE_SETTINGS + " allow");
    }
}
