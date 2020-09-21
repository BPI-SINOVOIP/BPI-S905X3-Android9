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

package android.view.inputmethod.cts;

import static com.android.cts.mockime.ImeEventStreamTestUtils.expectBindInput;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectEvent;
import static com.android.cts.mockime.ImeEventStreamTestUtils.waitForInputViewLayoutStable;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.graphics.Rect;
import android.os.Process;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextUtils;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.cts.util.EndToEndImeTestBase;
import android.view.inputmethod.cts.util.TestActivity;
import android.widget.EditText;
import android.widget.LinearLayout;

import com.android.compatibility.common.util.CtsTouchUtils;
import com.android.cts.mockime.ImeEventStream;
import com.android.cts.mockime.ImeLayoutInfo;
import com.android.cts.mockime.ImeSettings;
import com.android.cts.mockime.MockImeSession;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class OnScreenPositionTest extends EndToEndImeTestBase {
    private static final long TIMEOUT = TimeUnit.SECONDS.toMillis(5);
    private static final long LAYOUT_STABLE_THRESHOLD = TimeUnit.SECONDS.toMillis(3);

    private static final String TEST_MARKER = "android.view.inputmethod.cts.OnScreenPositionTest";

    public EditText launchTestActivity() {
        final AtomicReference<EditText> editTextRef = new AtomicReference<>();
        TestActivity.startSync(activity -> {
            final LinearLayout layout = new LinearLayout(activity);
            layout.setOrientation(LinearLayout.VERTICAL);

            final EditText editText = new EditText(activity);
            editText.setPrivateImeOptions(TEST_MARKER);
            editText.setHint("editText");
            editText.requestFocus();
            editTextRef.set(editText);

            layout.addView(editText);
            return layout;
        });
        return editTextRef.get();
    }

    private static final int EXPECTED_KEYBOARD_HEIGHT = 100;

    /**
     * Regression test for Bug 33308065.
     */
    @Test
    public void testImeIsNotBehindNavBar() throws Exception {
        try (MockImeSession imeSession = MockImeSession.create(
                InstrumentationRegistry.getContext(),
                InstrumentationRegistry.getInstrumentation().getUiAutomation(),
                new ImeSettings.Builder()
                        .setInputViewHeightWithoutSystemWindowInset(EXPECTED_KEYBOARD_HEIGHT))) {
            final ImeEventStream stream = imeSession.openEventStream();

            final EditText editText = launchTestActivity();

            // Wait until the MockIme gets bound to the TestActivity.
            expectBindInput(stream, Process.myPid(), TIMEOUT);

            // Emulate tap event
            CtsTouchUtils.emulateTapOnViewCenter(
                    InstrumentationRegistry.getInstrumentation(), editText);

            // Wait until "onStartInput" gets called for the EditText.
            expectEvent(stream, event -> {
                if (!TextUtils.equals("onStartInputView", event.getEventName())) {
                    return false;
                }
                final EditorInfo editorInfo = event.getArguments().getParcelable("editorInfo");
                return TextUtils.equals(TEST_MARKER, editorInfo.privateImeOptions);
            }, TIMEOUT);

            // Wait until MockIme's layout becomes stable.
            final ImeLayoutInfo lastLayout =
                    waitForInputViewLayoutStable(stream, LAYOUT_STABLE_THRESHOLD);
            assertNotNull(lastLayout);

            // We consider that the screenRectWithoutNavBar is a union of those two rects.
            // See the following methods for details.
            //  - DecorView#getColorViewTopInset(int, int)
            //  - DecorView#getColorViewBottomInset(int, int)
            //  - DecorView#getColorViewRightInset(int, int)
            //  - DecorView#getColorViewLeftInset(int, int)
            final Rect screenRectWithoutNavBar = lastLayout.getScreenRectWithoutStableInset();
            screenRectWithoutNavBar.union(lastLayout.getScreenRectWithoutSystemWindowInset());

            final Rect keyboardViewBounds = lastLayout.getInputViewBoundsInScreen();
            // By default, the above region must contain the keyboard view region.
            assertTrue("screenRectWithoutNavBar(" + screenRectWithoutNavBar + ") must"
                    + " contain keyboardViewBounds(" + keyboardViewBounds + ")",
                    screenRectWithoutNavBar.contains(keyboardViewBounds));

            // Make sure that the keyboard height is expected.  Here we assume that the expected
            // height is small enough for all the Android-based devices to show.
            assertEquals(EXPECTED_KEYBOARD_HEIGHT,
                    lastLayout.getInputViewBoundsInScreen().height());
        }
    }
}
