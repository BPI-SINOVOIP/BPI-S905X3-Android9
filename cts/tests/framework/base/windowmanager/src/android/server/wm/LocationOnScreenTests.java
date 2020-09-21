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

package android.server.wm;

import static android.server.wm.LocationOnScreenTests.TestActivity.EXTRA_LAYOUT_PARAMS;
import static android.server.wm.LocationOnScreenTests.TestActivity.TEST_COLOR_1;
import static android.server.wm.LocationOnScreenTests.TestActivity.TEST_COLOR_2;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;

import static org.hamcrest.Matchers.is;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.FlakyTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager.LayoutParams;
import android.widget.FrameLayout;

import com.android.compatibility.common.util.BitmapUtils;
import com.android.compatibility.common.util.PollingCheck;

import org.hamcrest.Matcher;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.junit.runner.RunWith;

import java.util.function.Supplier;

@RunWith(AndroidJUnit4.class)
@SmallTest
@Presubmit
@FlakyTest(detail = "until proven non-flaky")
public class LocationOnScreenTests {

    @Rule
    public final ErrorCollector mErrorCollector = new ErrorCollector();

    @Rule
    public final ActivityTestRule<TestActivity> mDisplayCutoutActivity =
            new ActivityTestRule<>(TestActivity.class, false /* initialTouchMode */,
                    false /* launchActivity */);

    private LayoutParams mLayoutParams;
    private Context mContext;

    @Before
    public void setUp() {
        mContext = InstrumentationRegistry.getContext();
        mLayoutParams = new LayoutParams(MATCH_PARENT, MATCH_PARENT, LayoutParams.TYPE_APPLICATION,
                LayoutParams.FLAG_LAYOUT_IN_SCREEN | LayoutParams.FLAG_LAYOUT_INSET_DECOR,
                PixelFormat.TRANSLUCENT);
    }

    @Test
    public void testLocationOnDisplay_appWindow() {
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationOnDisplay_appWindow_fullscreen() {
        mLayoutParams.flags |= LayoutParams.FLAG_FULLSCREEN;
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationOnDisplay_floatingWindow() {
        mLayoutParams.height = 50;
        mLayoutParams.width = 50;
        mLayoutParams.gravity = Gravity.CENTER;
        mLayoutParams.flags &= ~(FLAG_LAYOUT_IN_SCREEN | FLAG_LAYOUT_INSET_DECOR);
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationOnDisplay_appWindow_displayCutoutNever() {
        mLayoutParams.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationOnDisplay_appWindow_displayCutoutShortEdges() {
        mLayoutParams.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        runTest(mLayoutParams);
    }

    private void runTest(LayoutParams lp) {
        final TestActivity activity = launchAndWait(mDisplayCutoutActivity, lp);
        PollingCheck.waitFor(() -> getOnMainSync(activity::isEnterAnimationComplete));

        Point actual = getOnMainSync(activity::getViewLocationOnScreen);
        Point expected = findTestColorsInScreenshot(actual);

        assertThat("View.locationOnScreen returned incorrect value", actual, is(expected));
    }

    private <T> void assertThat(String reason, T actual, Matcher<? super T> matcher) {
        mErrorCollector.checkThat(reason, actual, matcher);
    }

    private <R> R getOnMainSync(Supplier<R> f) {
        final Object[] result = new Object[1];
        runOnMainSync(() -> result[0] = f.get());
        //noinspection unchecked
        return (R) result[0];
    }

    private void runOnMainSync(Runnable runnable) {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(runnable);
    }

    private <T extends Activity> T launchAndWait(ActivityTestRule<T> rule,
            LayoutParams lp) {
        final T activity = rule.launchActivity(
                new Intent().putExtra(EXTRA_LAYOUT_PARAMS, lp));
        PollingCheck.waitFor(activity::hasWindowFocus);
        return activity;
    }

    private Point findTestColorsInScreenshot(Point guess) {
        final Bitmap screenshot =
                InstrumentationRegistry.getInstrumentation().getUiAutomation().takeScreenshot();

        // We have a good guess from locationOnScreen - check there first to avoid having to go over
        // the entire bitmap. Also increases robustness in the extremely unlikely case that those
        // colors are visible elsewhere.
        if (isTestColors(screenshot, guess.x, guess.y)) {
            return guess;
        }

        for (int y = 0; y < screenshot.getHeight(); y++) {
            for (int x = 0; x < screenshot.getWidth() - 1; x++) {
                if (isTestColors(screenshot, x, y)) {
                    return new Point(x, y);
                }
            }
        }
        String path = mContext.getExternalFilesDir(null).getPath();
        String file = "location_on_screen_failure.png";
        BitmapUtils.saveBitmap(screenshot, path, file);
        Assert.fail("No match found for TEST_COLOR_1 and TEST_COLOR_2 pixels. Check "
                + path + "/" + file);
        return null;
    }

    private boolean isTestColors(Bitmap screenshot, int x, int y) {
        return screenshot.getPixel(x, y) == TEST_COLOR_1
                && screenshot.getPixel(x + 1, y) == TEST_COLOR_2;
    }

    public static class TestActivity extends Activity {

        static final int TEST_COLOR_1 = 0xff123456;
        static final int TEST_COLOR_2 = 0xfffedcba;
        static final String EXTRA_LAYOUT_PARAMS = "extra.layout_params";
        private View mView;
        private boolean mEnterAnimationComplete;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            getWindow().requestFeature(Window.FEATURE_NO_TITLE);

            FrameLayout frame = new FrameLayout(this);
            frame.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
            setContentView(frame);

            mView = new TestView(this);
            frame.addView(mView, new FrameLayout.LayoutParams(2, 1, Gravity.CENTER));

            if (getIntent() != null
                    && getIntent().getParcelableExtra(EXTRA_LAYOUT_PARAMS) != null) {
                getWindow().setAttributes(getIntent().getParcelableExtra(EXTRA_LAYOUT_PARAMS));
            }
        }

        public Point getViewLocationOnScreen() {
            final int[] location = new int[2];
            mView.getLocationOnScreen(location);
            return new Point(location[0], location[1]);
        }

        public boolean isEnterAnimationComplete() {
            return mEnterAnimationComplete;
        }

        @Override
        public void onEnterAnimationComplete() {
            super.onEnterAnimationComplete();
            mEnterAnimationComplete = true;
        }
    }

    private static class TestView extends View {
        private Paint mPaint = new Paint();

        public TestView(Context context) {
            super(context);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);

            mPaint.setColor(TEST_COLOR_1);
            canvas.drawRect(0, 0, 1, 1, mPaint);
            mPaint.setColor(TEST_COLOR_2);
            canvas.drawRect(1, 0, 2, 1, mPaint);
        }
    }
}
