/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.display.cts;

import static org.junit.Assert.*;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.Presentation;
import android.app.UiAutomation;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.hardware.display.DisplayManager;
import android.hardware.display.DisplayManager.DisplayListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.support.test.rule.ActivityTestRule;
import android.test.InstrumentationTestCase;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.Display.HdrCapabilities;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;

import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Scanner;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;


@RunWith(AndroidJUnit4.class)
public class DisplayTest {
    // The CTS package brings up an overlay display on the target device (see AndroidTest.xml).
    // The overlay display parameters must match the ones defined there which are
    // 181x161/214 (wxh/dpi).  It only matters that these values are different from any real
    // display.

    private static final int SECONDARY_DISPLAY_WIDTH = 181;
    private static final int SECONDARY_DISPLAY_HEIGHT = 161;
    private static final int SECONDARY_DISPLAY_DPI = 214;
    private static final float SCALE_DENSITY_LOWER_BOUND =
            (float)(SECONDARY_DISPLAY_DPI - 1) / DisplayMetrics.DENSITY_DEFAULT;
    private static final float SCALE_DENSITY_UPPER_BOUND =
            (float)(SECONDARY_DISPLAY_DPI + 1) / DisplayMetrics.DENSITY_DEFAULT;
    // Matches com.android.internal.R.string.display_manager_overlay_display_name.
    private static final String OVERLAY_DISPLAY_NAME_PREFIX = "Overlay #";
    private static final String OVERLAY_DISPLAY_TYPE = "type OVERLAY";

    private DisplayManager mDisplayManager;
    private WindowManager mWindowManager;
    private UiModeManager mUiModeManager;
    private Context mContext;

    // To test display mode switches.
    private TestPresentation mPresentation;

    private Activity mScreenOnActivity;

    @Rule
    public ActivityTestRule<DisplayTestActivity> mDisplayTestActivity =
            new ActivityTestRule<>(DisplayTestActivity.class,
                    false /* initialTouchMode */, false /* launchActivity */);

    @Before
    public void setUp() throws Exception {
        mScreenOnActivity = launchScreenOnActivity();
        mContext = InstrumentationRegistry.getInstrumentation().getContext();
        mDisplayManager = (DisplayManager)mContext.getSystemService(Context.DISPLAY_SERVICE);
        mWindowManager = (WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE);
        mUiModeManager = (UiModeManager)mContext.getSystemService(Context.UI_MODE_SERVICE);
    }

    @After
    public void tearDown() throws Exception {
        if (mScreenOnActivity != null) {
            mScreenOnActivity.finish();
        }
    }

    private void enableAppOps() {
        StringBuilder cmd = new StringBuilder();
        cmd.append("appops set ");
        cmd.append(InstrumentationRegistry.getInstrumentation().getContext().getPackageName());
        cmd.append(" android:system_alert_window allow");
        InstrumentationRegistry.getInstrumentation().getUiAutomation()
                .executeShellCommand(cmd.toString());

        StringBuilder query = new StringBuilder();
        query.append("appops get ");
        query.append(InstrumentationRegistry.getInstrumentation().getContext().getPackageName());
        query.append(" android:system_alert_window");
        String queryStr = query.toString();

        String result = "No operations.";
        while (result.contains("No operations")) {
            ParcelFileDescriptor pfd = InstrumentationRegistry.getInstrumentation()
                    .getUiAutomation().executeShellCommand(queryStr);
            InputStream inputStream = new FileInputStream(pfd.getFileDescriptor());
            result = convertStreamToString(inputStream);
        }
    }

    private String convertStreamToString(InputStream is) {
        try (java.util.Scanner s = new Scanner(is).useDelimiter("\\A")) {
            return s.hasNext() ? s.next() : "";
        }
    }

    /** Check if the display is an overlay display, created by this test. */
    private boolean isSecondaryDisplay(Display display) {
        return display.toString().contains(OVERLAY_DISPLAY_TYPE);
    }

    /** Get the overlay display, created by this test. */
    private Display getSecondaryDisplay(Display[] displays) {
        for (Display display : displays) {
            if (isSecondaryDisplay(display)) {
                return display;
            }
        }
        return null;
    }

    /**
     * Verify that the getDisplays method returns both a default and an overlay display.
     */
    @Test
    public void testGetDisplays() {
        Display[] displays = mDisplayManager.getDisplays();
        assertNotNull(displays);
        assertTrue(2 <= displays.length);
        boolean hasDefaultDisplay = false;
        boolean hasSecondaryDisplay = false;
        for (Display display : displays) {
            if (display.getDisplayId() == Display.DEFAULT_DISPLAY) {
                hasDefaultDisplay = true;
            }
            if (isSecondaryDisplay(display)) {
                hasSecondaryDisplay = true;
            }
        }
        assertTrue(hasDefaultDisplay);
        assertTrue(hasSecondaryDisplay);
    }

    /**
     * Verify that the WindowManager returns the default display.
     */
    @Presubmit
    @Test
    public void testDefaultDisplay() {
        assertEquals(Display.DEFAULT_DISPLAY, mWindowManager.getDefaultDisplay().getDisplayId());
    }

    /**
     * Verify default display's HDR capability.
     */
    @Test
    public void testDefaultDisplayHdrCapability() {
        Display display = mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY);
        HdrCapabilities cap = display.getHdrCapabilities();
        int[] hdrTypes = cap.getSupportedHdrTypes();
        for (int type : hdrTypes) {
            assertTrue(type >= 1 && type <= 3);
        }
        assertFalse(cap.getDesiredMaxLuminance() < -1.0f);
        assertFalse(cap.getDesiredMinLuminance() < -1.0f);
        assertFalse(cap.getDesiredMaxAverageLuminance() < -1.0f);
        if (hdrTypes.length > 0) {
            assertTrue(display.isHdr());
        } else {
            assertFalse(display.isHdr());
        }
    }

    /**
     * Verify that there is a secondary display.
     */
    @Test
    public void testSecondaryDisplay() {
        Display display = getSecondaryDisplay(mDisplayManager.getDisplays());
        assertNotNull(display);
        assertTrue(Display.DEFAULT_DISPLAY != display.getDisplayId());
    }

    /**
     * Test the properties of the secondary Display.
     */
    @Test
    public void testGetDisplayAttrs() {
        Display display = getSecondaryDisplay(mDisplayManager.getDisplays());

        assertEquals(SECONDARY_DISPLAY_WIDTH, display.getWidth());
        assertEquals(SECONDARY_DISPLAY_HEIGHT, display.getHeight());

        Point outSize = new Point();
        display.getSize(outSize);
        assertEquals(SECONDARY_DISPLAY_WIDTH, outSize.x);
        assertEquals(SECONDARY_DISPLAY_HEIGHT, outSize.y);

        assertEquals(0, display.getOrientation());

        assertEquals(PixelFormat.RGBA_8888, display.getPixelFormat());

        assertTrue(0 < display.getRefreshRate());

        assertTrue(display.getName().contains(OVERLAY_DISPLAY_NAME_PREFIX));

        assertFalse(display.isWideColorGamut());
    }

    /**
     * Test that the getMetrics method fills in correct values.
     */
    @Test
    public void testGetMetrics() {
        testGetMetrics(mDisplayManager);
    }

    /**
     * Tests getting metrics from the Activity context.
     */
    @Test
    public void testActivityContextGetMetrics() {
        final Activity activity = launchActivity(mDisplayTestActivity);
        final DisplayManager dm =
                (DisplayManager) activity.getSystemService(Context.DISPLAY_SERVICE);
        testGetMetrics(dm);
    }

    public void testGetMetrics(DisplayManager manager) {
        Display display = getSecondaryDisplay(manager.getDisplays());

        Point outSize = new Point();
        display.getSize(outSize);

        DisplayMetrics outMetrics = new DisplayMetrics();
        outMetrics.setToDefaults();
        display.getMetrics(outMetrics);

        assertEquals(SECONDARY_DISPLAY_WIDTH, outMetrics.widthPixels);
        assertEquals(SECONDARY_DISPLAY_HEIGHT, outMetrics.heightPixels);

        // The scale is in [0.1, 3], and density is the scale factor.
        assertTrue(SCALE_DENSITY_LOWER_BOUND <= outMetrics.density
                && outMetrics.density <= SCALE_DENSITY_UPPER_BOUND);
        assertTrue(SCALE_DENSITY_LOWER_BOUND <= outMetrics.scaledDensity
                && outMetrics.scaledDensity <= SCALE_DENSITY_UPPER_BOUND);

        assertEquals(SECONDARY_DISPLAY_DPI, outMetrics.densityDpi);
        assertEquals((float)SECONDARY_DISPLAY_DPI, outMetrics.xdpi, 0.0001f);
        assertEquals((float)SECONDARY_DISPLAY_DPI, outMetrics.ydpi, 0.0001f);
    }

    /**
     * Test that the getFlags method returns no flag bits set for the overlay display.
     */
    @Test
    public void testFlags() {
        Display display = getSecondaryDisplay(mDisplayManager.getDisplays());

        assertEquals(Display.FLAG_PRESENTATION, display.getFlags());
    }

    /**
     * Tests that the mode-related attributes and methods work as expected.
     */
    @Test
    public void testMode() {
        Display display = getSecondaryDisplay(mDisplayManager.getDisplays());
        assertEquals(2, display.getSupportedModes().length);
        Display.Mode mode = display.getMode();
        assertEquals(display.getSupportedModes()[0], mode);
        assertEquals(SECONDARY_DISPLAY_WIDTH, mode.getPhysicalWidth());
        assertEquals(SECONDARY_DISPLAY_HEIGHT, mode.getPhysicalHeight());
        assertEquals(display.getRefreshRate(), mode.getRefreshRate(), 0.0001f);
    }

    /**
     * Tests that mode switch requests are correctly executed.
     */
    @Test
    public void testModeSwitch() throws Exception {
        // Standalone VR devices globally ignore SYSTEM_ALERT_WINDOW via AppOps.
        // Skip this test, which depends on a Presentation SYSTEM_ALERT_WINDOW to pass.
        if (mUiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_VR_HEADSET) {
            return;
        }

        enableAppOps();
        final Display display = getSecondaryDisplay(mDisplayManager.getDisplays());
        Display.Mode[] modes = display.getSupportedModes();
        assertEquals(2, modes.length);
        Display.Mode mode = display.getMode();
        assertEquals(modes[0], mode);
        final Display.Mode newMode = modes[1];

        Handler handler = new Handler(Looper.getMainLooper());

        // Register for display events.
        final CountDownLatch changeSignal = new CountDownLatch(1);
        mDisplayManager.registerDisplayListener(new DisplayListener() {
            @Override
            public void onDisplayAdded(int displayId) {}

            @Override
            public void onDisplayChanged(int displayId) {
                if (displayId == display.getDisplayId()) {
                    changeSignal.countDown();
                }
            }

            @Override
            public void onDisplayRemoved(int displayId) {}
        }, handler);

        // Show the presentation.
        final CountDownLatch presentationSignal = new CountDownLatch(1);
        handler.post(new Runnable() {
            @Override
            public void run() {
                mPresentation = new TestPresentation(
                        InstrumentationRegistry.getInstrumentation().getContext(),
                        display, newMode.getModeId());
                mPresentation.show();
                presentationSignal.countDown();
            }
        });
        assertTrue(presentationSignal.await(5, TimeUnit.SECONDS));

        // Wait until the display change is effective.
        assertTrue(changeSignal.await(5, TimeUnit.SECONDS));

        assertEquals(newMode, display.getMode());
        handler.post(new Runnable() {
            @Override
            public void run() {
                mPresentation.dismiss();
            }
        });
    }

    /**
     * Used to force mode changes on a display.
     * <p>
     * Note that due to limitations of the Presentation class, the modes must have the same size
     * otherwise the presentation will be automatically dismissed.
     */
    private static final class TestPresentation extends Presentation {

        private final int mModeId;

        public TestPresentation(Context context, Display display, int modeId) {
            super(context, display);
            mModeId = modeId;
        }

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            View content = new View(getContext());
            content.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            content.setBackgroundColor(Color.RED);
            setContentView(content);

            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.preferredDisplayModeId = mModeId;
            params.type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
            params.setTitle("CtsTestPresentation");
            getWindow().setAttributes(params);
        }
    }

    private Activity launchScreenOnActivity() {
        Class clazz = ScreenOnActivity.class;
        String targetPackage =
                InstrumentationRegistry.getInstrumentation().getContext().getPackageName();
        Instrumentation.ActivityResult result =
                new Instrumentation.ActivityResult(0, new Intent());
        Instrumentation.ActivityMonitor monitor =
                new Instrumentation.ActivityMonitor(clazz.getName(), result, false);
        InstrumentationRegistry.getInstrumentation().addMonitor(monitor);
        launchActivity(targetPackage, clazz, null);
        return monitor.waitForActivity();
    }

    private Activity launchActivity(ActivityTestRule activityRule) {
        final Activity activity = activityRule.launchActivity(null);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        return activity;
    }

    /**
     * Utility method for launching an activity. Copied from InstrumentationTestCase since
     * InstrumentationRegistry does not provide these APIs anymore.
     *
     * <p>The {@link Intent} used to launch the Activity is:
     *  action = {@link Intent#ACTION_MAIN}
     *  extras = null, unless a custom bundle is provided here
     * All other fields are null or empty.
     *
     * <p><b>NOTE:</b> The parameter <i>pkg</i> must refer to the package identifier of the
     * package hosting the activity to be launched, which is specified in the AndroidManifest.xml
     * file.  This is not necessarily the same as the java package name.
     *
     * @param pkg The package hosting the activity to be launched.
     * @param activityCls The activity class to launch.
     * @param extras Optional extra stuff to pass to the activity.
     * @return The activity, or null if non launched.
     */
    private final <T extends Activity> T launchActivity(
            String pkg,
            Class<T> activityCls,
            Bundle extras) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        if (extras != null) {
            intent.putExtras(extras);
        }
        return launchActivityWithIntent(pkg, activityCls, intent);
    }

    /**
     * Utility method for launching an activity with a specific Intent.
     *
     * <p><b>NOTE:</b> The parameter <i>pkg</i> must refer to the package identifier of the
     * package hosting the activity to be launched, which is specified in the AndroidManifest.xml
     * file.  This is not necessarily the same as the java package name.
     *
     * @param pkg The package hosting the activity to be launched.
     * @param activityCls The activity class to launch.
     * @param intent The intent to launch with
     * @return The activity, or null if non launched.
     */
    @SuppressWarnings("unchecked")
    private final <T extends Activity> T launchActivityWithIntent(
            String pkg,
            Class<T> activityCls,
            Intent intent) {
        intent.setClassName(pkg, activityCls.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        T activity = (T) InstrumentationRegistry.getInstrumentation().startActivitySync(intent);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        return activity;
    }
}
