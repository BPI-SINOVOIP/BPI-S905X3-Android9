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
package android.app.cts;

import android.app.ActionBar;
import android.app.ActionBar.Tab;
import android.app.ActionBar.TabListener;
import android.app.FragmentTransaction;
import android.app.stubs.ActionBarActivity;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.view.KeyEvent;
import android.view.ViewConfiguration;
import android.view.Window;

import java.util.concurrent.TimeUnit;

public class ActionBarTest extends ActivityInstrumentationTestCase2<ActionBarActivity> {

    private ActionBarActivity mActivity;
    private ActionBar mBar;

    public ActionBarTest() {
        super(ActionBarActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        mBar = mActivity.getActionBar();
    }

    @UiThreadTest
    public void testAddTab() {
        if (mBar == null) {
            return;
        }
        assertEquals(0, mBar.getTabCount());

        Tab t1 = createTab("Tab 1");
        mBar.addTab(t1);
        assertEquals(1, mBar.getTabCount());
        assertEquals(t1, mBar.getSelectedTab());
        assertEquals(t1, mBar.getTabAt(0));

        Tab t2 = createTab("Tab 2");
        mBar.addTab(t2);
        assertEquals(2, mBar.getTabCount());
        assertEquals(t1, mBar.getSelectedTab());
        assertEquals(t2, mBar.getTabAt(1));

        Tab t3 = createTab("Tab 3");
        mBar.addTab(t3, true);
        assertEquals(3, mBar.getTabCount());
        assertEquals(t3, mBar.getSelectedTab());
        assertEquals(t3, mBar.getTabAt(2));

        Tab t4 = createTab("Tab 2.5");
        mBar.addTab(t4, 2);
        assertEquals(4, mBar.getTabCount());
        assertEquals(t4, mBar.getTabAt(2));
        assertEquals(t3, mBar.getTabAt(3));

        Tab t5 = createTab("Tab 0.5");
        mBar.addTab(t5, 0, true);
        assertEquals(5, mBar.getTabCount());
        assertEquals(t5, mBar.getSelectedTab());
        assertEquals(t5, mBar.getTabAt(0));
        assertEquals(t1, mBar.getTabAt(1));
        assertEquals(t2, mBar.getTabAt(2));
        assertEquals(t4, mBar.getTabAt(3));
        assertEquals(t3, mBar.getTabAt(4));
    }

    public void testOptionsMenuKey() throws Exception {
        boolean hasPermanentMenuKey = ViewConfiguration.get(getActivity()).hasPermanentMenuKey();
        if (!mActivity.getWindow().hasFeature(Window.FEATURE_OPTIONS_PANEL)
                || hasPermanentMenuKey) {
            return;
        }
        final boolean menuIsVisible[] = {false};
        mActivity.getActionBar().addOnMenuVisibilityListener(
                isVisible -> menuIsVisible[0] = isVisible);
        // Wait here for test activity to gain focus before sending keyevent.
        // Visibility listener needs the action bar to be visible.
        getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_MENU);
        getInstrumentation().waitForIdleSync();
        assertTrue(menuIsVisible[0]);
        assertTrue(mActivity.windowFocusSignal.await(1000, TimeUnit.MILLISECONDS));
        getInstrumentation().sendKeyDownUpSync(KeyEvent.KEYCODE_MENU);
        getInstrumentation().waitForIdleSync();
        assertTrue(mActivity.windowFocusSignal.await(1000, TimeUnit.MILLISECONDS));
        assertFalse(menuIsVisible[0]);
    }

    public void testOpenOptionsMenu() {
        boolean hasPermanentMenuKey = ViewConfiguration.get(getActivity()).hasPermanentMenuKey();
        if (!mActivity.getWindow().hasFeature(Window.FEATURE_OPTIONS_PANEL)
                || hasPermanentMenuKey) {
            return;
        }
        final boolean menuIsVisible[] = {false};
        mActivity.getActionBar().addOnMenuVisibilityListener(
                isVisible -> menuIsVisible[0] = isVisible);
        getInstrumentation().runOnMainSync(() -> mActivity.openOptionsMenu());
        getInstrumentation().waitForIdleSync();
        assertTrue(menuIsVisible[0]);
        getInstrumentation().runOnMainSync(() -> mActivity.closeOptionsMenu());
        getInstrumentation().waitForIdleSync();
        assertFalse(menuIsVisible[0]);
    }

    private Tab createTab(String name) {
        return mBar.newTab().setText("Tab 1").setTabListener(new TestTabListener());
    }

    static class TestTabListener implements TabListener {
        @Override
        public void onTabSelected(Tab tab, FragmentTransaction ft) {
        }

        @Override
        public void onTabUnselected(Tab tab, FragmentTransaction ft) {
        }

        @Override
        public void onTabReselected(Tab tab, FragmentTransaction ft) {
        }
    }
}
