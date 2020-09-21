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
 * limitations under the License
 */

package android.server.wm;

import static android.server.wm.DisplayCutoutTests.TestActivity.EXTRA_CUTOUT_MODE;
import static android.server.wm.DisplayCutoutTests.TestDef.Which.DISPATCHED;
import static android.server.wm.DisplayCutoutTests.TestDef.Which.ROOT;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;

import static org.hamcrest.Matchers.everyItem;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.greaterThanOrEqualTo;
import static org.hamcrest.Matchers.hasItem;
import static org.hamcrest.Matchers.hasSize;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.lessThanOrEqualTo;
import static org.hamcrest.Matchers.not;
import static org.hamcrest.Matchers.notNullValue;
import static org.hamcrest.Matchers.nullValue;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Bundle;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.DisplayCutout;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;

import com.android.compatibility.common.util.PollingCheck;

import org.hamcrest.CustomTypeSafeMatcher;
import org.hamcrest.FeatureMatcher;
import org.hamcrest.Matcher;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ErrorCollector;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;
import java.util.function.Predicate;
import java.util.function.Supplier;
import java.util.stream.Collectors;

/**
 * Build/Install/Run:
 *     atest CtsWindowManagerDeviceTestCases:DisplayCutoutTests
 */
@RunWith(AndroidJUnit4.class)
@Presubmit
public class DisplayCutoutTests {

    @Rule
    public final ErrorCollector mErrorCollector = new ErrorCollector();

    @Rule
    public final ActivityTestRule<TestActivity> mDisplayCutoutActivity =
            new ActivityTestRule<>(TestActivity.class, false /* initialTouchMode */,
                    false /* launchActivity */);

    @Test
    public void testDisplayCutout_default_portrait() {
        runTest(LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT, (activity, insets, displayCutout, which) -> {
            if (displayCutout == null) {
                return;
            }
            if (which == ROOT) {
                assertThat("cutout must be contained within system bars in default mode",
                        safeInsets(displayCutout), insetsLessThanOrEqualTo(stableInsets(insets)));
            } else if (which == DISPATCHED) {
                assertThat("must not dipatch to hierarchy in default mode",
                        displayCutout, nullValue());
            }
        });
    }

    @Test
    public void testDisplayCutout_landscape() {
        // TODO add landscape variants
    }

    @Test
    public void testDisplayCutout_shortEdges_portrait() {
        runTest(LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES, (a, insets, displayCutout, which) -> {
            // common assertions in runTest are enough.
        });
    }

    @Test
    public void testDisplayCutout_never_portrait() {
        runTest(LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER, (a, insets, displayCutout, which) -> {
            assertThat("must not layout in cutout area in never mode", displayCutout, nullValue());
        });
    }

    private void runTest(int cutoutMode, TestDef test) {
        final TestActivity activity = launchAndWait(mDisplayCutoutActivity,
                cutoutMode);

        WindowInsets insets = getOnMainSync(activity::getRootInsets);
        WindowInsets dispatchedInsets = getOnMainSync(activity::getDispatchedInsets);
        Assert.assertThat("test setup failed, no insets at root", insets, notNullValue());
        Assert.assertThat("test setup failed, no insets dispatched",
                dispatchedInsets, notNullValue());

        final DisplayCutout displayCutout = insets.getDisplayCutout();
        final DisplayCutout dispatchedDisplayCutout = dispatchedInsets.getDisplayCutout();

        if (displayCutout != null) {
            commonAsserts(activity, insets, displayCutout);
        }
        test.run(activity, insets, displayCutout, ROOT);

        if (dispatchedDisplayCutout != null) {
            commonAsserts(activity, dispatchedInsets, dispatchedDisplayCutout);
        }
        test.run(activity, dispatchedInsets, dispatchedDisplayCutout, DISPATCHED);
    }

    private void commonAsserts(TestActivity activity, WindowInsets insets, DisplayCutout cutout) {
        assertSafeInsetsValid(cutout);
        assertThat("systemWindowInsets (also known as content insets) must be at least as large as "
                        + "cutout safe insets",
                safeInsets(cutout), insetsLessThanOrEqualTo(systemWindowInsets(insets)));
        assertOnlyShortEdgeHasInsets(activity, cutout);
        assertCutoutsAreWithinSafeInsets(activity, cutout);
        assertBoundsAreNonEmpty(cutout);
        assertAtMostOneCutoutPerEdge(activity, cutout);
    }

    private void assertSafeInsetsValid(DisplayCutout displayCutout) {
        //noinspection unchecked
        assertThat("all safe insets must be non-negative", safeInsets(displayCutout),
                insetValues(everyItem((Matcher)greaterThanOrEqualTo(0))));
        assertThat("at least one safe inset must be positive,"
                        + " otherwise WindowInsets.getDisplayCutout()) must return null",
                safeInsets(displayCutout), insetValues(hasItem(greaterThan(0))));
    }

    private void assertCutoutsAreWithinSafeInsets(TestActivity a, DisplayCutout cutout) {
        final Rect safeRect = getSafeRect(a, cutout);

        assertThat("safe insets must not cover the entire screen", safeRect.isEmpty(), is(false));
        for (Rect boundingRect : cutout.getBoundingRects()) {
            assertThat("boundingRects must not extend beyond safeInsets",
                    boundingRect, not(intersectsWith(safeRect)));
        }
    }

    private void assertAtMostOneCutoutPerEdge(TestActivity a, DisplayCutout cutout) {
        final Rect safeRect = getSafeRect(a, cutout);

        assertThat("must not have more than one left cutout",
                boundsWith(cutout, (r) -> r.right <= safeRect.left), hasSize(lessThanOrEqualTo(1)));
        assertThat("must not have more than one top cutout",
                boundsWith(cutout, (r) -> r.bottom <= safeRect.top), hasSize(lessThanOrEqualTo(1)));
        assertThat("must not have more than one right cutout",
                boundsWith(cutout, (r) -> r.left >= safeRect.right), hasSize(lessThanOrEqualTo(1)));
        assertThat("must not have more than one bottom cutout",
                boundsWith(cutout, (r) -> r.top >= safeRect.bottom), hasSize(lessThanOrEqualTo(1)));
    }

    private void assertBoundsAreNonEmpty(DisplayCutout cutout) {
        for (Rect boundingRect : cutout.getBoundingRects()) {
            assertThat("rect in boundingRects must not be empty",
                    boundingRect.isEmpty(), is(false));
        }
    }

    private void assertOnlyShortEdgeHasInsets(TestActivity activity,
            DisplayCutout displayCutout) {
        final Point displaySize = new Point();
        runOnMainSync(() -> activity.getDecorView().getDisplay().getRealSize(displaySize));
        if (displaySize.y > displaySize.x) {  // Portrait display
            assertThat("left edge has a cutout despite being long edge",
                    displayCutout.getSafeInsetLeft(), is(0));
            assertThat("right edge has a cutout despite being long edge",
                    displayCutout.getSafeInsetRight(), is(0));
        }
        if (displaySize.y < displaySize.x) {  // Landscape display
            assertThat("top edge has a cutout despite being long edge",
                    displayCutout.getSafeInsetTop(), is(0));
            assertThat("bottom edge has a cutout despite being long edge",
                    displayCutout.getSafeInsetBottom(), is(0));
        }
    }

    private List<Rect> boundsWith(DisplayCutout cutout, Predicate<Rect> predicate) {
        return cutout.getBoundingRects().stream().filter(predicate).collect(Collectors.toList());
    }


    private static Rect safeInsets(DisplayCutout displayCutout) {
        return new Rect(displayCutout.getSafeInsetLeft(), displayCutout.getSafeInsetTop(),
                displayCutout.getSafeInsetRight(), displayCutout.getSafeInsetBottom());
    }

    private static Rect systemWindowInsets(WindowInsets insets) {
        return new Rect(insets.getSystemWindowInsetLeft(), insets.getSystemWindowInsetTop(),
                insets.getSystemWindowInsetRight(), insets.getSystemWindowInsetBottom());
    }

    private static Rect stableInsets(WindowInsets insets) {
        return new Rect(insets.getStableInsetLeft(), insets.getStableInsetTop(),
                insets.getStableInsetRight(), insets.getStableInsetBottom());
    }

    private Rect getSafeRect(TestActivity a, DisplayCutout cutout) {
        final Rect safeRect = safeInsets(cutout);
        safeRect.bottom = getOnMainSync(() -> a.getDecorView().getHeight()) - safeRect.bottom;
        safeRect.right = getOnMainSync(() -> a.getDecorView().getWidth()) - safeRect.right;
        return safeRect;
    }

    private static Matcher<Rect> insetsLessThanOrEqualTo(Rect max) {
        return new CustomTypeSafeMatcher<Rect>("must be smaller on each side than " + max) {
            @Override
            protected boolean matchesSafely(Rect actual) {
                return actual.left <= max.left && actual.top <= max.top
                        && actual.right <= max.right && actual.bottom <= max.bottom;
            }
        };
    }

    private static Matcher<Rect> intersectsWith(Rect safeRect) {
        return new CustomTypeSafeMatcher<Rect>("intersects with " + safeRect) {
            @Override
            protected boolean matchesSafely(Rect item) {
                return Rect.intersects(safeRect, item);
            }
        };
    }

    private static Matcher<Rect> insetValues(Matcher<Iterable<? super Integer>> valuesMatcher) {
        return new FeatureMatcher<Rect, Iterable<Integer>>(valuesMatcher, "inset values",
                "inset values") {
            @Override
            protected Iterable<Integer> featureValueOf(Rect actual) {
                return Arrays.asList(actual.left, actual.top, actual.right, actual.bottom);
            }
        };
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

    private <T extends Activity> T launchAndWait(ActivityTestRule<T> rule, int cutoutMode) {
        final T activity = rule.launchActivity(
                new Intent().putExtra(EXTRA_CUTOUT_MODE, cutoutMode));
        PollingCheck.waitFor(activity::hasWindowFocus);
        return activity;
    }

    public static class TestActivity extends Activity {

        static final String EXTRA_CUTOUT_MODE = "extra.cutout_mode";
        private WindowInsets mDispatchedInsets;

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            getWindow().requestFeature(Window.FEATURE_NO_TITLE);
            if (getIntent() != null) {
                getWindow().getAttributes().layoutInDisplayCutoutMode = getIntent().getIntExtra(
                        EXTRA_CUTOUT_MODE, LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT);
            }
            View view = new View(this);
            view.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
            view.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
            view.setOnApplyWindowInsetsListener((v, insets) -> mDispatchedInsets = insets);
            setContentView(view);
        }

        View getDecorView() {
            return getWindow().getDecorView();
        }

        WindowInsets getRootInsets() {
            return getWindow().getDecorView().getRootWindowInsets();
        }

        WindowInsets getDispatchedInsets() {
            return mDispatchedInsets;
        }
    }

    interface TestDef {
        void run(TestActivity a, WindowInsets insets, DisplayCutout cutout, Which whichInsets);

        enum Which {
            DISPATCHED, ROOT
        }
    }
}
