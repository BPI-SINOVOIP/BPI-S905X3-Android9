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

import static com.android.cts.mockime.ImeEventStreamTestUtils.EventFilterMode.CHECK_EXIT_EVENT_ONLY;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectBindInput;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectEvent;

import android.os.Process;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.InputType;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.cts.util.EndToEndImeTestBase;
import android.view.inputmethod.cts.util.TestActivity;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.SearchView;

import com.android.compatibility.common.util.CtsTouchUtils;
import com.android.cts.mockime.ImeEventStream;
import com.android.cts.mockime.ImeSettings;
import com.android.cts.mockime.MockImeSession;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class SearchViewTest extends EndToEndImeTestBase {
    static final long TIMEOUT = TimeUnit.SECONDS.toMillis(5);

    public SearchView launchTestActivity() {
        final AtomicReference<SearchView> searchViewRef = new AtomicReference<>();
        TestActivity.startSync(activity -> {
            final LinearLayout layout = new LinearLayout(activity);
            layout.setOrientation(LinearLayout.VERTICAL);

            final EditText initialFocusedEditText = new EditText(activity);
            initialFocusedEditText.setHint("initialFocusedTextView");

            final SearchView searchView = new SearchView(activity);
            searchViewRef.set(searchView);

            searchView.setQueryHint("hint");
            searchView.setIconifiedByDefault(false);
            searchView.setInputType(InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
            searchView.setImeOptions(EditorInfo.IME_ACTION_DONE);

            layout.addView(initialFocusedEditText);
            layout.addView(searchView);
            return layout;
        });
        return searchViewRef.get();
    }

    @Test
    public void testTapThenSetQuery() throws Exception {
        try (MockImeSession imeSession = MockImeSession.create(
                InstrumentationRegistry.getContext(),
                InstrumentationRegistry.getInstrumentation().getUiAutomation(),
                new ImeSettings.Builder())) {
            final ImeEventStream stream = imeSession.openEventStream();

            final SearchView searchView = launchTestActivity();

            // Emulate tap event on SearchView
            CtsTouchUtils.emulateTapOnViewCenter(
                    InstrumentationRegistry.getInstrumentation(), searchView);

            // Expect input to bind since EditText is focused.
            expectBindInput(stream, Process.myPid(), TIMEOUT);

            // Wait until "showSoftInput" gets called with a real InputConnection
            expectEvent(stream, event ->
                    "showSoftInput".equals(event.getEventName())
                            && !event.getExitState().hasDummyInputConnection(),
                    CHECK_EXIT_EVENT_ONLY, TIMEOUT);

            // Make sure that "setQuery" triggers "hideSoftInput" in the IME side.
            InstrumentationRegistry.getInstrumentation().runOnMainSync(
                    () -> searchView.setQuery("test", true /* submit */));
            expectEvent(stream, event -> "hideSoftInput".equals(event.getEventName()), TIMEOUT);
        }
    }
}
