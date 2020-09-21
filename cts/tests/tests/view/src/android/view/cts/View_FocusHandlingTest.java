/*
 * Copyright (C) 2009 The Android Open Source Project
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.MediumTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class View_FocusHandlingTest {
    @Rule
    public ActivityTestRule<FocusHandlingCtsActivity> mActivityRule =
            new ActivityTestRule<>(FocusHandlingCtsActivity.class);

    @UiThreadTest
    @Test
    public void testFocusHandling() {
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
        instrumentation.setInTouchMode(false);
        Activity activity = mActivityRule.getActivity();

        View v1 = activity.findViewById(R.id.view1);
        View v2 = activity.findViewById(R.id.view2);
        View v3 = activity.findViewById(R.id.view3);
        View v4 = activity.findViewById(R.id.view4);

        assertNotNull(v1);
        assertNotNull(v2);
        assertNotNull(v3);
        assertNotNull(v4);

        // test isFocusable and setFocusable
        assertFalse(v1.isFocusable());
        assertFalse(v2.isFocusable());
        assertFalse(v3.isFocusable());
        assertFalse(v4.isFocusable());

        v1.setFocusable(true);
        v2.setFocusable(true);
        v3.setFocusable(true);
        v4.setFocusable(true);

        assertTrue(v1.isFocusable());
        assertTrue(v2.isFocusable());
        assertTrue(v3.isFocusable());
        assertTrue(v4.isFocusable());

        v1.setNextFocusRightId(R.id.view2);
        v1.setNextFocusDownId(R.id.view3);

        v2.setNextFocusLeftId(R.id.view1);
        v2.setNextFocusDownId(R.id.view4);

        v3.setNextFocusRightId(R.id.view4);
        v3.setNextFocusUpId(R.id.view1);

        v4.setNextFocusLeftId(R.id.view3);
        v4.setNextFocusUpId(R.id.view2);

        // test getNextFocus***Id
        assertEquals(R.id.view2, v1.getNextFocusRightId());
        assertEquals(R.id.view3, v1.getNextFocusDownId());

        assertEquals(R.id.view1, v2.getNextFocusLeftId());
        assertEquals(R.id.view4, v2.getNextFocusDownId());

        assertEquals(R.id.view1, v3.getNextFocusUpId());
        assertEquals(R.id.view4, v3.getNextFocusRightId());

        assertEquals(R.id.view2, v4.getNextFocusUpId());
        assertEquals(R.id.view3, v4.getNextFocusLeftId());

        // test focusSearch
        assertSame(v2, v1.focusSearch(View.FOCUS_RIGHT));
        assertSame(v3, v1.focusSearch(View.FOCUS_DOWN));

        assertSame(v1, v2.focusSearch(View.FOCUS_LEFT));
        assertSame(v4, v2.focusSearch(View.FOCUS_DOWN));

        assertSame(v1, v3.focusSearch(View.FOCUS_UP));
        assertSame(v4, v3.focusSearch(View.FOCUS_RIGHT));

        assertSame(v2, v4.focusSearch(View.FOCUS_UP));
        assertSame(v3, v4.focusSearch(View.FOCUS_LEFT));

        assertTrue(v1.requestFocus());
        assertTrue(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertFalse(v4.hasFocus());

        v1.setVisibility(View.INVISIBLE);
        assertFalse(v1.hasFocus());
        assertTrue(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertFalse(v4.hasFocus());

        v2.setVisibility(View.INVISIBLE);
        assertFalse(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertTrue(v3.hasFocus());
        assertFalse(v4.hasFocus());

        v3.setVisibility(View.INVISIBLE);
        assertFalse(v1.isFocused());
        assertFalse(v2.isFocused());
        assertFalse(v3.isFocused());
        assertTrue(v4.isFocused());

        v4.setVisibility(View.INVISIBLE);
        assertFalse(v1.isFocused());
        assertFalse(v2.isFocused());
        assertFalse(v3.isFocused());
        assertFalse(v4.isFocused());

        v1.setVisibility(View.VISIBLE);
        v2.setVisibility(View.VISIBLE);
        v3.setVisibility(View.VISIBLE);
        v4.setVisibility(View.VISIBLE);
        assertTrue(v1.isFocused());
        assertFalse(v2.isFocused());
        assertFalse(v3.isFocused());
        assertFalse(v4.isFocused());

        v1.requestFocus();
        assertTrue(v1.isFocused());

        // test scenario: a view will not actually take focus if it is not focusable
        v2.setFocusable(false);
        v3.setFocusable(false);

        assertTrue(v1.isFocusable());
        assertFalse(v2.isFocusable());
        assertFalse(v3.isFocusable());
        assertTrue(v4.isFocusable());

        v1.setVisibility(View.INVISIBLE);
        assertFalse(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertTrue(v4.hasFocus());

        // compare isFocusable and hasFocusable
        assertTrue(v1.isFocusable());
        assertFalse(v1.hasFocusable());

        // test requestFocus
        v1.setVisibility(View.VISIBLE);
        v2.setFocusable(true);
        v3.setFocusable(true);

        assertTrue(v1.hasFocusable());
        assertTrue(v2.hasFocusable());
        assertTrue(v3.hasFocusable());
        assertTrue(v4.hasFocusable());

        assertTrue(v2.requestFocus(View.FOCUS_UP));
        assertFalse(v1.hasFocus());
        assertTrue(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertFalse(v4.hasFocus());

        assertTrue(v1.requestFocus(View.FOCUS_LEFT, null));
        assertTrue(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertFalse(v4.hasFocus());

        assertTrue(v3.requestFocus());
        assertFalse(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertTrue(v3.hasFocus());
        assertFalse(v4.hasFocus());

        assertTrue(v4.requestFocusFromTouch());
        assertFalse(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertTrue(v4.hasFocus());

        // test clearFocus
        v4.clearFocus();
        assertTrue(v1.hasFocus());
        assertFalse(v2.hasFocus());
        assertFalse(v3.hasFocus());
        assertFalse(v4.hasFocus());

        assertSame(v1, v1.findFocus());
        assertNull(v2.findFocus());
        assertNull(v3.findFocus());
        assertNull(v4.findFocus());

        // test visibility with a nested focusable
        ViewGroup vg = (ViewGroup) activity.findViewById(R.id.auto_test_area);
        vg.setVisibility(View.INVISIBLE);
        View sub = new View(activity);
        vg.addView(sub, 10, 10);
        sub.setFocusable(View.FOCUSABLE);
        for (View v : new View[]{v1, v2, v3, v4}) {
            v.setFocusable(false);
        }
        assertNull(vg.getRootView().findFocus());
        vg.setVisibility(View.VISIBLE);
        assertSame(sub, vg.getRootView().findFocus());
    }

    @UiThreadTest
    @Test
    public void testEnabledHandling() {
        Activity activity = mActivityRule.getActivity();

        View v1 = activity.findViewById(R.id.view1);
        View v2 = activity.findViewById(R.id.view2);
        View v3 = activity.findViewById(R.id.view3);
        View v4 = activity.findViewById(R.id.view4);

        for (View v : new View[]{v1, v2, v3, v4}) v.setFocusable(true);

        assertTrue(v1.requestFocus());

        // disabled view should not be focusable
        assertTrue(v1.hasFocus());
        v1.setEnabled(false);
        assertFalse(v1.hasFocus());
        v1.requestFocus();
        assertFalse(v1.hasFocus());
        v1.setEnabled(true);
        v1.requestFocus();
        assertTrue(v1.hasFocus());

        // an enabled view should not take focus if not visible OR not enabled
        v1.setEnabled(false);
        v1.setVisibility(View.INVISIBLE);
        assertFalse(v1.hasFocus());
        v1.setEnabled(true);
        v1.requestFocus();
        assertFalse(v1.hasFocus());
        v1.setEnabled(false);
        v1.setVisibility(View.VISIBLE);
        v1.requestFocus();
        assertFalse(v1.hasFocus());
        v1.setEnabled(true);
        v1.requestFocus();
        assertTrue(v1.hasFocus());

        // test hasFocusable
        ViewGroup parent = (ViewGroup) v1.getParent();
        assertTrue(parent.hasFocusable());
        for (View v : new View[]{v1, v2, v3, v4}) v.setEnabled(false);
        assertFalse(v1.isFocused());
        assertFalse(v2.isFocused());
        assertFalse(v3.isFocused());
        assertFalse(v4.isFocused());
        assertFalse(parent.hasFocusable());

        // a view enabled while nothing has focus should get focus if not in touch mode.
        InstrumentationRegistry.getInstrumentation().setInTouchMode(false);
        for (View v : new View[]{v1, v2, v3, v4}) v.setEnabled(true);
        assertEquals(true, v1.isFocused());

        // enabled state is restricted to the view only (not children)
        v2.requestFocus();
        parent.setEnabled(false);
        assertTrue(v2.isFocused());
    }

    @Test
    public void testSizeHandling() {
        Activity activity = mActivityRule.getActivity();
        Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();

        View v5 = new Button(activity);
        ViewGroup layout = activity.findViewById(R.id.auto_test_area);

        // Test requestFocus before first layout focuses if non-0 size
        activity.runOnUiThread(() -> {
            layout.addView(v5, 30, 30);
            assertTrue(isZeroSize(v5));
            assertTrue(v5.requestFocus());
        });
        instrumentation.waitForIdleSync();
        assertFalse(isZeroSize(v5));
        assertTrue(v5.isFocused());

        // Test resize to 0 defocuses
        activity.runOnUiThread(() -> {
            v5.setRight(v5.getLeft());
            assertEquals(0, v5.getWidth());
        });
        instrumentation.waitForIdleSync();
        assertTrue(isZeroSize(v5));
        assertFalse(v5.isFocused());

        // Test requestFocus on laid-out 0-size fails
        activity.runOnUiThread(() -> assertFalse(v5.requestFocus()));

        activity.runOnUiThread(() -> layout.removeAllViews());

        // Test requestFocus before first layout focuses a child if non-0 size
        LinearLayout ll0 = new LinearLayout(activity);
        ll0.setFocusable(true);
        ll0.setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
        View butInScroll = new Button(activity);
        activity.runOnUiThread(() -> {
            ll0.addView(butInScroll, 40, 40);
            layout.addView(ll0, 100, 100);
            assertTrue(isZeroSize(butInScroll));
            assertTrue(ll0.requestFocus());
        });
        instrumentation.waitForIdleSync();
        assertFalse(isZeroSize(butInScroll));
        assertTrue(butInScroll.isFocused());

        // Test focusableViewAvailable on resize to non-0 size
        activity.runOnUiThread(() -> {
            butInScroll.setRight(v5.getLeft());
        });
        instrumentation.waitForIdleSync();
        assertTrue(isZeroSize(butInScroll));
        assertTrue(ll0.isFocused());

        activity.runOnUiThread(() -> layout.removeAllViews());
        instrumentation.waitForIdleSync();

        // Test requestFocus before first layout defocuses if still 0 size
        LinearLayout ll = new LinearLayout(activity);
        View zeroSizeBut = new Button(activity);
        activity.runOnUiThread(() -> {
            ll.addView(zeroSizeBut, 30, 0);
            layout.addView(ll, 100, 100);
            assertTrue(zeroSizeBut.requestFocus());
        });
        instrumentation.waitForIdleSync();
        assertTrue(isZeroSize(zeroSizeBut));
        assertFalse(zeroSizeBut.isFocused());

        activity.runOnUiThread(() -> layout.removeAllViews());
        instrumentation.waitForIdleSync();

        // Test requestFocus before first layout focuses parent if child is still 0 size
        LinearLayout ll2 = new LinearLayout(activity);
        ll2.setFocusable(true);
        ll2.setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
        View zeroButInAfter = new Button(activity);
        activity.runOnUiThread(() -> {
            ll2.addView(zeroButInAfter, 40, 0);
            layout.addView(ll2, 100, 100);
            assertTrue(ll2.requestFocus());
            assertTrue(zeroButInAfter.isFocused());
        });
        instrumentation.waitForIdleSync();
        assertTrue(isZeroSize(zeroButInAfter));
        assertFalse(zeroButInAfter.isFocused());
        assertTrue(ll2.isFocused());

        activity.runOnUiThread(() -> layout.removeAllViews());
        instrumentation.waitForIdleSync();

        // Test that we don't focus anything above/outside of where we requested focus
        LinearLayout ll3 = new LinearLayout(activity);
        Button outside = new Button(activity);
        LinearLayout sub = new LinearLayout(activity);
        Button inside = new Button(activity);
        activity.runOnUiThread(() -> {
            ll3.addView(outside, 40, 40);
            sub.addView(inside, 30, 0);
            ll3.addView(sub, 40, 40);
            layout.addView(ll3, 100, 100);
            assertTrue(sub.requestFocus());
            assertTrue(inside.isFocused());
        });
        instrumentation.waitForIdleSync();
        assertTrue(isZeroSize(inside));
        assertTrue(outside.isFocusable() && !isZeroSize(outside));
        assertNull(layout.getRootView().findFocus());
    }

    private boolean isZeroSize(View view) {
        return view.getWidth() <= 0 || view.getHeight() <= 0;
    }

    @UiThreadTest
    @Test
    public void testFocusAuto() {
        Activity activity = mActivityRule.getActivity();

        activity.getLayoutInflater().inflate(R.layout.focus_handling_auto_layout,
                (ViewGroup) activity.findViewById(R.id.auto_test_area));

        View def = activity.findViewById(R.id.focusabledefault);
        View auto = activity.findViewById(R.id.focusableauto);
        View click = activity.findViewById(R.id.onlyclickable);
        View clickNotFocus = activity.findViewById(R.id.clickablenotfocusable);

        assertEquals(View.FOCUSABLE_AUTO, auto.getFocusable());
        assertFalse(auto.isFocusable());
        assertFalse(def.isFocusable());
        assertTrue(click.isFocusable());
        assertFalse(clickNotFocus.isFocusable());

        View test = new View(activity);
        assertFalse(test.isFocusable());
        test.setClickable(true);
        assertTrue(test.isFocusable());
        test.setFocusable(View.NOT_FOCUSABLE);
        assertFalse(test.isFocusable());
        test.setFocusable(View.FOCUSABLE_AUTO);
        assertTrue(test.isFocusable());
        test.setClickable(false);
        assertFalse(test.isFocusable());

        // Make sure setFocusable(boolean) unsets FOCUSABLE_AUTO.
        auto.setFocusable(true);
        assertSame(View.FOCUSABLE, auto.getFocusable());
        auto.setFocusable(View.FOCUSABLE_AUTO);
        assertSame(View.FOCUSABLE_AUTO, auto.getFocusable());
        auto.setFocusable(false);
        assertSame(View.NOT_FOCUSABLE, auto.getFocusable());
    }

    @UiThreadTest
    @Test
    public void testFocusableInTouchMode() {
        final Activity activity = mActivityRule.getActivity();
        View singleView = new View(activity);
        assertFalse("Must not be focusable by default", singleView.isFocusable());
        singleView.setFocusableInTouchMode(true);
        assertSame("setFocusableInTouchMode(true) must imply explicit focusable",
                View.FOCUSABLE, singleView.getFocusable());

        activity.getLayoutInflater().inflate(R.layout.focus_handling_auto_layout,
                (ViewGroup) activity.findViewById(R.id.auto_test_area));
        View focusTouchModeView = activity.findViewById(R.id.focusabletouchmode);
        assertSame("focusableInTouchMode=\"true\" must imply explicit focusable",
                View.FOCUSABLE, focusTouchModeView.getFocusable());
    }

    @UiThreadTest
    @Test
    public void testHasFocusable() {
        final Activity activity = mActivityRule.getActivity();
        final ViewGroup group = (ViewGroup) activity.findViewById(R.id.auto_test_area);

        View singleView = new View(activity);
        group.addView(singleView);

        testHasFocusable(singleView);

        group.removeView(singleView);

        View groupView = new FrameLayout(activity);
        group.addView(groupView);

        testHasFocusable(groupView);
    }

    private void testHasFocusable(View view) {
        assertEquals("single view was not auto-focusable", View.FOCUSABLE_AUTO,
                view.getFocusable());
        assertFalse("single view unexpectedly hasFocusable", view.hasFocusable());
        assertFalse("single view unexpectedly hasExplicitFocusable", view.hasExplicitFocusable());

        view.setClickable(true);
        assertTrue("single view doesn't hasFocusable", view.hasFocusable());
        assertFalse("single view unexpectedly hasExplicitFocusable", view.hasExplicitFocusable());

        view.setClickable(false);
        assertFalse("single view unexpectedly hasFocusable", view.hasFocusable());
        assertFalse("single view unexpectedly hasExplicitFocusable", view.hasExplicitFocusable());

        view.setFocusable(View.NOT_FOCUSABLE);
        assertFalse("single view unexpectedly hasFocusable", view.hasFocusable());
        assertFalse("single view unexpectedly hasExplicitFocusable", view.hasExplicitFocusable());

        view.setFocusable(View.FOCUSABLE);
        assertTrue("single view doesn't hasFocusable", view.hasFocusable());
        assertTrue("single view doesn't hasExplicitFocusable", view.hasExplicitFocusable());

        view.setFocusable(View.FOCUSABLE_AUTO);
        view.setFocusableInTouchMode(true);
        assertTrue("single view doesn't hasFocusable", view.hasFocusable());
        assertTrue("single view doesn't hasExplicitFocusable", view.hasExplicitFocusable());
    }

    @UiThreadTest
    @Test
    public void testFocusAfterDescendantsTransfer() throws Throwable {
        final Activity activity = mActivityRule.getActivity();
        final ViewGroup group = (ViewGroup) activity.findViewById(R.id.auto_test_area);
        ViewGroup root = (ViewGroup) activity.findViewById(R.id.main_view);
        group.setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
        group.setFocusableInTouchMode(true);
        group.setKeyboardNavigationCluster(true);
        group.requestFocus();
        assertTrue(group.isFocused());

        LinearLayout mid = new LinearLayout(activity);
        Button but = new Button(activity);
        but.setRight(but.getLeft() + 10);
        but.setBottom(but.getTop() + 10);
        but.setFocusableInTouchMode(true);
        but.setVisibility(View.INVISIBLE);
        mid.addView(but, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);

        group.addView(mid, ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        but.setVisibility(View.VISIBLE);
        assertSame(root.findFocus(), but);
        assertFalse(group.isFocused());

        assertFalse(root.restoreFocusNotInCluster());
        assertSame(root.findFocus(), but);
    }
}
