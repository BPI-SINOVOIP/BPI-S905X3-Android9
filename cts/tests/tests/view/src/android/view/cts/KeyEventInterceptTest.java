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

package android.view.cts;

import static org.junit.Assert.fail;

import android.app.Instrumentation;
import android.app.UiAutomation;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.KeyEvent;

import com.android.compatibility.common.util.PollingCheck;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;


/**
 * Certain KeyEvents should never be delivered to apps. These keys are:
 *      KEYCODE_ASSIST
 *      KEYCODE_VOICE_ASSIST
 *      KEYCODE_HOME
 * This test launches an Activity and inject KeyEvents with the corresponding key codes.
 * The test will fail if any of these keys are received by the activity.
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class KeyEventInterceptTest {
    private KeyEventInterceptTestActivity mActivity;
    private Instrumentation mInstrumentation;

    @Rule
    public ActivityTestRule<KeyEventInterceptTestActivity> mActivityRule =
            new ActivityTestRule<>(KeyEventInterceptTestActivity.class);

    @Before
    public void setup() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
        mActivity = mActivityRule.getActivity();
        PollingCheck.waitFor(mActivity::hasWindowFocus);
    }

    @Test
    public void testKeyCodeAssist() {
        testKey(KeyEvent.KEYCODE_ASSIST);
    }

    @Test
    public void testKeyCodeVoiceAssist() {
        testKey(KeyEvent.KEYCODE_VOICE_ASSIST);
    }

    @Test
    public void testKeyCodeHome() {
        testKey(KeyEvent.KEYCODE_HOME);
    }

    private void testKey(int keyCode) {
        sendKey(keyCode);
        assertKeyNotReceived();
    }

    private void sendKey(int keyCodeToSend) {
        long downTime = SystemClock.uptimeMillis();
        injectEvent(new KeyEvent(downTime, downTime, KeyEvent.ACTION_DOWN,
                keyCodeToSend, 0, 0));
        injectEvent(new KeyEvent(downTime, downTime + 1, KeyEvent.ACTION_UP,
                keyCodeToSend, 0, 0));
    }

    private void injectEvent(KeyEvent event) {
        final UiAutomation automation = mInstrumentation.getUiAutomation();
        automation.injectInputEvent(event, true);
        event.recycle();
    }

    private void assertKeyNotReceived() {
        try {
            KeyEvent keyEvent = mActivity.mKeyEvents.poll(1, TimeUnit.SECONDS);
            if (keyEvent == null) {
                return;
            }
            fail("Should not have received " + KeyEvent.keyCodeToString(keyEvent.getKeyCode()));
        } catch (InterruptedException ex) {
            fail("BlockingQueue.poll(..) was unexpectedly interrupted");
        }
    }
}
