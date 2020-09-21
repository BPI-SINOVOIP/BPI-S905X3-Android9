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
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_INSET_DECOR;
import static android.view.WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;

import static org.hamcrest.Matchers.is;

import android.app.Activity;
import android.content.Intent;
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

import com.android.compatibility.common.util.PollingCheck;

import org.hamcrest.Matcher;
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
public class LocationInWindowTests {

    @Rule
    public final ErrorCollector mErrorCollector = new ErrorCollector();

    @Rule
    public final ActivityTestRule<TestActivity> mActivity =
            new ActivityTestRule<>(TestActivity.class, false /* initialTouchMode */,
                    false /* launchActivity */);

    private LayoutParams mLayoutParams;

    @Before
    public void setUp() {
        mLayoutParams = new LayoutParams(MATCH_PARENT, MATCH_PARENT, LayoutParams.TYPE_APPLICATION,
                LayoutParams.FLAG_LAYOUT_IN_SCREEN | LayoutParams.FLAG_LAYOUT_INSET_DECOR,
                PixelFormat.TRANSLUCENT);
    }

    @Test
    public void testLocationInWindow_appWindow() {
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationInWindow_appWindow_fullscreen() {
        mLayoutParams.flags |= LayoutParams.FLAG_FULLSCREEN;
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationInWindow_floatingWindow() {
        mLayoutParams.height = 100;
        mLayoutParams.width = 100;
        mLayoutParams.gravity = Gravity.CENTER;
        mLayoutParams.flags &= ~(FLAG_LAYOUT_IN_SCREEN | FLAG_LAYOUT_INSET_DECOR);
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationInWindow_appWindow_displayCutoutNever() {
        mLayoutParams.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
        runTest(mLayoutParams);
    }

    @Test
    public void testLocationInWindow_appWindow_displayCutoutShortEdges() {
        mLayoutParams.layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        runTest(mLayoutParams);
    }

    private void runTest(LayoutParams lp) {
        final TestActivity activity = launchAndWait(mActivity, lp);
        PollingCheck.waitFor(() -> getOnMainSync(activity::isEnterAnimationComplete));

        runOnMainSync(() -> {
            assertThatViewGetLocationInWindowIsCorrect(activity.mView);
        });
    }

    private void assertThatViewGetLocationInWindowIsCorrect(View v) {
        final float[] expected = new float[] {0, 0};

        v.getMatrix().mapPoints(expected);
        expected[0] += v.getLeft();
        expected[1] += v.getTop();

        final View parent = (View)v.getParent();
        expected[0] -= parent.getScrollX();
        expected[1] -= parent.getScrollY();

        final Point parentLocation = locationInWindowAsPoint(parent);
        expected[0] += parentLocation.x;
        expected[1] += parentLocation.y;

        Point actual = locationInWindowAsPoint(v);

        assertThat("getLocationInWindow returned wrong location",
                actual, is(new Point((int) expected[0], (int) expected[1])));
    }

    private Point locationInWindowAsPoint(View v) {
        final int[] out = new int[2];
        v.getLocationInWindow(out);
        return new Point(out[0], out[1]);
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

    public static class TestActivity extends Activity {

        static final String EXTRA_LAYOUT_PARAMS = "extra.layout_params";
        private View mView;
        private boolean mEnterAnimationComplete;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            getWindow().requestFeature(Window.FEATURE_NO_TITLE);

            FrameLayout frame = new FrameLayout(this);
            frame.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
            frame.setPadding(1, 2, 3, 4);
            setContentView(frame);

            mView = new View(this);
            final FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(1, 1);
            lp.leftMargin = 6;
            lp.topMargin = 7;
            frame.addView(mView, lp);
            mView.setTranslationX(8);
            mView.setTranslationY(9);
            mView.setScrollX(10);
            mView.setScrollY(11);
            frame.setScrollX(12);
            frame.setScrollY(13);

            if (getIntent() != null
                    && getIntent().getParcelableExtra(EXTRA_LAYOUT_PARAMS) != null) {
                getWindow().setAttributes(getIntent().getParcelableExtra(EXTRA_LAYOUT_PARAMS));
            }
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
}
