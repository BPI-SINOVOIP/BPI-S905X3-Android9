/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.view.inputmethod.cts;

import static android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE;
import static android.view.inputmethod.cts.util.TestUtils.getOnMainSync;
import static android.view.inputmethod.cts.util.TestUtils.waitOnMainUntil;

import static com.android.cts.mockime.ImeEventStreamTestUtils.EventFilterMode.CHECK_EXIT_EVENT_ONLY;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectCommand;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectEvent;
import static com.android.cts.mockime.ImeEventStreamTestUtils.notExpectEvent;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.app.Instrumentation;
import android.inputmethodservice.InputMethodService;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.inputmethod.cts.util.EndToEndImeTestBase;
import android.view.inputmethod.cts.util.TestActivity;
import android.widget.EditText;
import android.widget.LinearLayout;

import com.android.cts.mockime.ImeCommand;
import com.android.cts.mockime.ImeEvent;
import com.android.cts.mockime.ImeEventStream;
import com.android.cts.mockime.ImeSettings;
import com.android.cts.mockime.MockImeSession;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Predicate;

/**
 * Tests for {@link InputMethodService} methods.
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class InputMethodServiceTest extends EndToEndImeTestBase {
    private static final long TIMEOUT = TimeUnit.SECONDS.toMillis(5);
    private static final long EXPECTED_TIMEOUT = TimeUnit.SECONDS.toMillis(2);

    private Instrumentation mInstrumentation;

    private static Predicate<ImeEvent> backKeyDownMatcher(boolean expectedReturnValue) {
        return event -> {
            if (!TextUtils.equals("onKeyDown", event.getEventName())) {
                return false;
            }
            final int keyCode = event.getArguments().getInt("keyCode");
            if (keyCode != KeyEvent.KEYCODE_BACK) {
                return false;
            }
            return event.getReturnBooleanValue() == expectedReturnValue;
        };
    }

    @Before
    public void setup() {
        mInstrumentation = InstrumentationRegistry.getInstrumentation();
    }

    private TestActivity createTestActivity(final int windowFlags) {
        return TestActivity.startSync(activity -> {
            final LinearLayout layout = new LinearLayout(activity);
            layout.setOrientation(LinearLayout.VERTICAL);

            final EditText editText = new EditText(activity);
            editText.setText("Editable");
            layout.addView(editText);
            editText.requestFocus();

            activity.getWindow().setSoftInputMode(windowFlags);
            return layout;
        });
    }

    private void verifyImeConsumesBackButton(int backDisposition) throws Exception {
        try (MockImeSession imeSession = MockImeSession.create(
                InstrumentationRegistry.getContext(),
                InstrumentationRegistry.getInstrumentation().getUiAutomation(),
                new ImeSettings.Builder())) {
            final ImeEventStream stream = imeSession.openEventStream();

            final TestActivity testActivity = createTestActivity(SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            expectEvent(stream, event -> "onStartInputView".equals(event.getEventName()), TIMEOUT);

            final ImeCommand command = imeSession.callSetBackDisposition(backDisposition);
            expectCommand(stream, command, TIMEOUT);

            testActivity.setIgnoreBackKey(true);
            assertEquals(0,
                    (long) getOnMainSync(() -> testActivity.getOnBackPressedCallCount()));
            mInstrumentation.sendKeyDownUpSync(KeyEvent.KEYCODE_BACK);

            expectEvent(stream, backKeyDownMatcher(true), CHECK_EXIT_EVENT_ONLY, TIMEOUT);

            // Make sure TestActivity#onBackPressed() is NOT called.
            try {
                waitOnMainUntil(() -> testActivity.getOnBackPressedCallCount() > 0,
                        EXPECTED_TIMEOUT);
                fail("Activity#onBackPressed() should not be called");
            } catch (TimeoutException e){
                // This is fine.  We actually expect timeout.
            }
        }
    }

    @Test
    public void testSetBackDispositionDefault() throws Exception {
        verifyImeConsumesBackButton(InputMethodService.BACK_DISPOSITION_DEFAULT);
    }

    @Test
    public void testSetBackDispositionWillNotDismiss() throws Exception {
        verifyImeConsumesBackButton(InputMethodService.BACK_DISPOSITION_WILL_NOT_DISMISS);
    }

    @Test
    public void testSetBackDispositionWillDismiss() throws Exception {
        verifyImeConsumesBackButton(InputMethodService.BACK_DISPOSITION_WILL_DISMISS);
    }

    @Test
    public void testSetBackDispositionAdjustNothing() throws Exception {
        verifyImeConsumesBackButton(InputMethodService.BACK_DISPOSITION_ADJUST_NOTHING);
    }

    @Test
    public void testRequestHideSelf() throws Exception {
        try (MockImeSession imeSession = MockImeSession.create(
                InstrumentationRegistry.getContext(),
                InstrumentationRegistry.getInstrumentation().getUiAutomation(),
                new ImeSettings.Builder())) {
            final ImeEventStream stream = imeSession.openEventStream();

            createTestActivity(SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            expectEvent(stream, event -> "onStartInputView".equals(event.getEventName()), TIMEOUT);

            imeSession.callRequestHideSelf(0);
            expectEvent(stream, event -> "hideSoftInput".equals(event.getEventName()), TIMEOUT);
            expectEvent(stream, event -> "onFinishInputView".equals(event.getEventName()), TIMEOUT);
        }
    }

    @Test
    public void testRequestShowSelf() throws Exception {
        try (MockImeSession imeSession = MockImeSession.create(
                InstrumentationRegistry.getContext(),
                InstrumentationRegistry.getInstrumentation().getUiAutomation(),
                new ImeSettings.Builder())) {
            final ImeEventStream stream = imeSession.openEventStream();

            createTestActivity(SOFT_INPUT_STATE_ALWAYS_HIDDEN);
            notExpectEvent(
                    stream, event -> "onStartInputView".equals(event.getEventName()), TIMEOUT);

            imeSession.callRequestShowSelf(0);
            expectEvent(stream, event -> "showSoftInput".equals(event.getEventName()), TIMEOUT);
            expectEvent(stream, event -> "onStartInputView".equals(event.getEventName()), TIMEOUT);
        }
    }
}
