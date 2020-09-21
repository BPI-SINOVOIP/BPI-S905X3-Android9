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

import static android.view.View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR;
import static android.view.WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
import static android.view.WindowManager.LayoutParams.FLAG_DIM_BEHIND;
import static android.view.WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS;
import static android.view.WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
import static android.view.inputmethod.cts.util.LightNavigationBarVerifier.expectLightNavigationBarNotSupported;
import static android.view.inputmethod.cts.util.LightNavigationBarVerifier.expectLightNavigationBarSupported;
import static android.view.inputmethod.cts.util.NavigationBarColorVerifier.expectNavigationBarColorNotSupported;
import static android.view.inputmethod.cts.util.NavigationBarColorVerifier.expectNavigationBarColorSupported;
import static android.view.inputmethod.cts.util.TestUtils.getOnMainSync;

import static com.android.cts.mockime.ImeEventStreamTestUtils.expectBindInput;
import static com.android.cts.mockime.ImeEventStreamTestUtils.expectEvent;
import static com.android.cts.mockime.ImeEventStreamTestUtils.waitForInputViewLayoutStable;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assume.assumeTrue;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.UiAutomation;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Process;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.cts.util.EndToEndImeTestBase;
import android.view.inputmethod.cts.util.ImeAwareEditText;
import android.view.inputmethod.cts.util.NavigationBarInfo;
import android.view.inputmethod.cts.util.TestActivity;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.cts.mockime.ImeEventStream;
import com.android.cts.mockime.ImeLayoutInfo;
import com.android.cts.mockime.ImeSettings;
import com.android.cts.mockime.MockImeSession;

import org.junit.BeforeClass;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.TimeUnit;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class NavigationBarColorTest extends EndToEndImeTestBase {
    private static final long TIMEOUT = TimeUnit.SECONDS.toMillis(5);
    private static final long LAYOUT_STABLE_THRESHOLD = TimeUnit.SECONDS.toMillis(3);

    private static final String TEST_MARKER = "android.view.inputmethod.cts.NavigationBarColorTest";

    private static void updateSystemUiVisibility(@NonNull View view, int flags, int mask) {
        final int currentFlags = view.getSystemUiVisibility();
        final int newFlags = (currentFlags & ~mask) | (flags & mask);
        if (currentFlags != newFlags) {
            view.setSystemUiVisibility(newFlags);
        }
    }

    @BeforeClass
    public static void initializeNavigationBarInfo() throws Exception {
        // Make sure that NavigationBarInfo is initialized before
        // EndToEndImeTestBase#showStateInitializeActivity().
        NavigationBarInfo.getInstance();
    }

    // TODO(b/37502066): Merge this back to initializeNavigationBarInfo() once b/37502066 is fixed.
    @Before
    public void checkNavigationBar() throws Exception {
        assumeTrue("This test does not make sense if there is no navigation bar",
                NavigationBarInfo.getInstance().hasBottomNavigationBar());

        assumeTrue("This test does not make sense if custom navigation bar color is not supported"
                        + " even for typical Activity",
                NavigationBarInfo.getInstance().supportsNavigationBarColor());
    }

    /**
     * Represents test scenarios regarding how a {@link android.view.Window} that has
     * {@link android.view.WindowManager.LayoutParams#FLAG_DIM_BEHIND} interacts with a different
     * {@link android.view.Window} that has
     * {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR}.
     */
    private enum DimmingTestMode {
        /**
         * No {@link AlertDialog} is shown when testing.
         */
        NO_DIMMING_DIALOG,
        /**
         * An {@link AlertDialog} that has dimming effect is shown above the IME window.
         */
        DIMMING_DIALOG_ABOVE_IME,
        /**
         * An {@link AlertDialog} that has dimming effect is shown behind the IME window.
         */
        DIMMING_DIALOG_BEHIND_IME,
    }

    @NonNull
    public TestActivity launchTestActivity(@ColorInt int navigationBarColor,
            boolean lightNavigationBar, @NonNull DimmingTestMode dimmingTestMode) {
        return TestActivity.startSync(activity -> {
            final View contentView;
            switch (dimmingTestMode) {
                case NO_DIMMING_DIALOG:
                case DIMMING_DIALOG_ABOVE_IME: {
                    final LinearLayout layout = new LinearLayout(activity);
                    layout.setOrientation(LinearLayout.VERTICAL);
                    final ImeAwareEditText editText = new ImeAwareEditText(activity);
                    editText.setPrivateImeOptions(TEST_MARKER);
                    editText.setHint("editText");
                    editText.requestFocus();
                    editText.scheduleShowSoftInput();
                    layout.addView(editText);
                    contentView = layout;
                    break;
                }
                case DIMMING_DIALOG_BEHIND_IME: {
                    final View view = new View(activity);
                    view.setLayoutParams(new ViewGroup.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.MATCH_PARENT));
                    contentView = view;
                    break;
                }
                default:
                    throw new IllegalStateException("unknown mode=" + dimmingTestMode);
            }
            activity.getWindow().setNavigationBarColor(navigationBarColor);
            updateSystemUiVisibility(contentView,
                    lightNavigationBar ? SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR : 0,
                    SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
            return contentView;
        });
    }

    private AutoCloseable showDialogIfNecessary(
            @NonNull Activity activity, @NonNull DimmingTestMode dimmingTestMode) {
        switch (dimmingTestMode) {
            case NO_DIMMING_DIALOG:
                // Dialog is not necessary.
                return () -> { };
            case DIMMING_DIALOG_ABOVE_IME: {
                final AlertDialog alertDialog = getOnMainSync(() -> {
                    final TextView textView = new TextView(activity);
                    textView.setText("Dummy");
                    textView.requestFocus();
                    final AlertDialog dialog = new AlertDialog.Builder(activity)
                            .setView(textView)
                            .create();
                    dialog.getWindow().setFlags(FLAG_DIM_BEHIND | FLAG_ALT_FOCUSABLE_IM,
                            FLAG_DIM_BEHIND | FLAG_NOT_FOCUSABLE | FLAG_ALT_FOCUSABLE_IM);
                    dialog.show();
                    return dialog;
                });
                // Note: Dialog#dismiss() is a thread safe method so we don't need to call this from
                // the UI thread.
                return () -> alertDialog.dismiss();
            }
            case DIMMING_DIALOG_BEHIND_IME: {
                final AlertDialog alertDialog = getOnMainSync(() -> {
                    final ImeAwareEditText editText = new ImeAwareEditText(activity);
                    editText.setPrivateImeOptions(TEST_MARKER);
                    editText.setHint("editText");
                    editText.requestFocus();
                    editText.scheduleShowSoftInput();
                    final AlertDialog dialog = new AlertDialog.Builder(activity)
                            .setView(editText)
                            .create();
                    dialog.getWindow().setFlags(FLAG_DIM_BEHIND,
                            FLAG_DIM_BEHIND | FLAG_NOT_FOCUSABLE | FLAG_ALT_FOCUSABLE_IM);
                    dialog.show();
                    return dialog;
                });
                // Note: Dialog#dismiss() is a thread safe method so we don't need to call this from
                // the UI thread.
                return () -> alertDialog.dismiss();
            }
            default:
                throw new IllegalStateException("unknown mode=" + dimmingTestMode);
        }
    }

    @NonNull
    private ImeSettings.Builder imeSettingForSolidNavigationBar(@ColorInt int navigationBarColor,
            boolean lightNavigationBar) {
        final ImeSettings.Builder builder = new ImeSettings.Builder();
        builder.setNavigationBarColor(navigationBarColor);
        if (lightNavigationBar) {
            builder.setInputViewSystemUiVisibility(SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
        }
        return builder;
    }

    @NonNull
    private ImeSettings.Builder imeSettingForFloatingIme(@ColorInt int navigationBarColor,
            boolean lightNavigationBar) {
        final ImeSettings.Builder builder = new ImeSettings.Builder();
        builder.setWindowFlags(0, FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        // As documented, Window#setNavigationBarColor() is actually ignored when the IME window
        // does not have FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS.  We are calling setNavigationBarColor()
        // to ensure it.
        builder.setNavigationBarColor(navigationBarColor);
        if (lightNavigationBar) {
            // As documented, SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR is actually ignored when the IME
            // window does not have FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS.  We set this flag just to
            // ensure it.
            builder.setInputViewSystemUiVisibility(SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
        }
        return builder;
    }

    @NonNull
    private Bitmap getNavigationBarBitmap(@NonNull ImeSettings.Builder builder,
            @ColorInt int appNavigationBarColor, boolean appLightNavigationBar,
            int navigationBarHeight, @NonNull DimmingTestMode dimmingTestMode)
            throws Exception {
        final UiAutomation uiAutomation =
                InstrumentationRegistry.getInstrumentation().getUiAutomation();
        try (MockImeSession imeSession = MockImeSession.create(
                InstrumentationRegistry.getContext(), uiAutomation, builder)) {
            final ImeEventStream stream = imeSession.openEventStream();

            final TestActivity activity = launchTestActivity(
                    appNavigationBarColor, appLightNavigationBar, dimmingTestMode);

            // Show AlertDialog if necessary, based on the dimming test mode.
            try (AutoCloseable dialogCloser = showDialogIfNecessary(
                    activity, dimmingTestMode)) {
                // Wait until the MockIme gets bound to the TestActivity.
                expectBindInput(stream, Process.myPid(), TIMEOUT);

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

                final Bitmap bitmap = uiAutomation.takeScreenshot();
                return Bitmap.createBitmap(bitmap, 0, bitmap.getHeight() - navigationBarHeight,
                        bitmap.getWidth(), navigationBarHeight);
            }
        }
    }

    @Test
    public void testSetNavigationBarColor() throws Exception {
        final NavigationBarInfo info = NavigationBarInfo.getInstance();

        // Make sure that Window#setNavigationBarColor() works for IMEs.
        expectNavigationBarColorSupported(color ->
                getNavigationBarBitmap(imeSettingForSolidNavigationBar(color, false),
                        Color.BLACK, false, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.NO_DIMMING_DIALOG));

        // Make sure that IME's navigation bar can be transparent
        expectNavigationBarColorSupported(color ->
                getNavigationBarBitmap(imeSettingForSolidNavigationBar(Color.TRANSPARENT, false),
                        color, false, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.NO_DIMMING_DIALOG));

        // Make sure that Window#setNavigationBarColor() is ignored when
        // FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS is unset
        expectNavigationBarColorNotSupported(color ->
                getNavigationBarBitmap(imeSettingForFloatingIme(color, false),
                        Color.BLACK, false, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.NO_DIMMING_DIALOG));
    }

    @Test
    public void testLightNavigationBar() throws Exception {
        final NavigationBarInfo info = NavigationBarInfo.getInstance();

        assumeTrue("This test does not make sense if light navigation bar is not supported"
                + " even for typical Activity", info.supportsLightNavigationBar());

        // Make sure that SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR works for IMEs (Bug 69002467).
        expectLightNavigationBarSupported((color, lightMode) ->
                getNavigationBarBitmap(imeSettingForSolidNavigationBar(color, lightMode),
                        Color.BLACK, false, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.NO_DIMMING_DIALOG));

        // Make sure that IMEs can opt-out navigation bar custom rendering, including
        // SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR, by un-setting FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS flag
        // so that it can be controlled by the target application instead (Bug 69111208).
        expectLightNavigationBarSupported((color, lightMode) ->
                getNavigationBarBitmap(imeSettingForFloatingIme(Color.BLACK, false),
                        color, lightMode, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.NO_DIMMING_DIALOG));
    }

    @Test
    public void testDimmingWindow() throws Exception {
        final NavigationBarInfo info = NavigationBarInfo.getInstance();

        assumeTrue("This test does not make sense if dimming windows do not affect light "
                + " light navigation bar for typical Activities",
                info.supportsDimmingWindowLightNavigationBarOverride());

        // Make sure that SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR works for IMEs, even if a dimming
        // window is shown behind the IME window.
        expectLightNavigationBarSupported((color, lightMode) ->
                getNavigationBarBitmap(imeSettingForSolidNavigationBar(color, lightMode),
                        Color.BLACK, false, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.DIMMING_DIALOG_BEHIND_IME));

        // If a dimming window is shown above the IME window, IME window's
        // SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR should be canceled.
        expectLightNavigationBarNotSupported((color, lightMode) ->
                getNavigationBarBitmap(imeSettingForSolidNavigationBar(color, lightMode),
                        Color.BLACK, false, info.getBottomNavigationBerHeight(),
                        DimmingTestMode.DIMMING_DIALOG_ABOVE_IME));
    }
}
