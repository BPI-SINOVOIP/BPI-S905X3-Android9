/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.platform.systemui.tests.jank;

import android.content.pm.PackageManager;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.launcherhelper.ILauncherStrategy;
import android.support.test.launcherhelper.LauncherStrategyFactory;
import android.support.test.timeresulthelper.TimeResultLogger;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.system.helpers.OverviewHelper;

import java.io.File;
import java.io.IOException;

/*
 * LauncherTwoJankTests cover the old launcher, and
 * LauncherJankTests cover the new GEL Launcher.
 */
public class LauncherJankTests extends JankTestBase {

    private static final int TIMEOUT = 5000;
    // short transitions should be repeated within the test function, otherwise frame stats
    // captured are not really meaningful in a statistical sense
    private static final int INNER_LOOP = 3;
    private static final int FLING_SPEED = 12000;
    private static final String SYSTEMUI_PACKAGE = "com.android.systemui";
    private UiDevice mDevice;
    private PackageManager pm;
    private ILauncherStrategy mLauncherStrategy = null;
    private static final File TIMESTAMP_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(),"autotester.log");
    private static final File RESULTS_FILE = new File(Environment.getExternalStorageDirectory()
            .getAbsolutePath(),"results.log");

    @Override
    public void setUp() {
        mDevice = UiDevice.getInstance(getInstrumentation());
        pm = getInstrumentation().getContext().getPackageManager();
        try {
            mDevice.setOrientationNatural();
        } catch (RemoteException e) {
            throw new RuntimeException("failed to freeze device orientaion", e);
        }
        mLauncherStrategy = LauncherStrategyFactory.getInstance(mDevice).getLauncherStrategy();
    }

    public String getLauncherPackage() {
        return mDevice.getLauncherPackageName();
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    public void goHome() throws UiObjectNotFoundException {
        mLauncherStrategy.open();
    }

    private BySelector getAppListViewSelector() {
        return By.res(getLauncherPackage(), "apps_list_view");
    }

    /** Swipes from Recents to All Apps. */
    private void fromRecentsToAllApps() {
        assertTrue(mDevice.hasObject(SystemUiJankTests.getLauncherOverviewSelector(mDevice)));
        assertTrue(!mDevice.hasObject(getAppListViewSelector()));

        // Swipe from the hotseat to near the top, e.g. 10% of the screen.
        UiObject2 predictionRow = mDevice.wait(
                Until.findObject(By.res(getLauncherPackage(), "prediction_row")),
                2500);
        Point start = predictionRow.getVisibleCenter();
        int endY = (int) (mDevice.getDisplayHeight() * 0.1f);
        mDevice.swipe(start.x, start.y, start.x, endY, (start.y - endY) / 100); // 100 px/step

        UiObject2 allAppsContainer = mDevice.wait(Until.findObject(getAppListViewSelector()), 2500);
        assertNotNull("openAllApps: did not find all apps container", allAppsContainer);
        assertTrue(!mDevice.hasObject(SystemUiJankTests.getLauncherOverviewSelector(mDevice)));
    }

    /** Swipes from All Apps to Recents. */
    private void fromAllAppsToRecents() { // add state assertions
        assertTrue(!mDevice.hasObject(SystemUiJankTests.getLauncherOverviewSelector(mDevice)));
        assertTrue(mDevice.hasObject(getAppListViewSelector()));

        // Swipe from the search box to the bottom.
        UiObject2 qsb = mDevice.wait(
                Until.findObject(By.res(getLauncherPackage(), "search_container_all_apps")), 2500);
        Point start = qsb.getVisibleCenter();
        int endY = (int) (mDevice.getDisplayHeight() * 0.6);
        mDevice.swipe(start.x, start.y, start.x, endY, (endY - start.y) / 100); // 100 px/step

        UiObject2 recentsView = mDevice.wait(
                Until.findObject(SystemUiJankTests.getLauncherOverviewSelector(mDevice)), 2500);
        assertNotNull(recentsView);
        assertTrue(!mDevice.hasObject(getAppListViewSelector()));
    }

    public void resetAndOpenRecents() throws UiObjectNotFoundException, RemoteException {
        goHome();
        mDevice.pressRecentApps();
    }

    public void prepareOpenAllAppsContainer() throws IOException {
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        OverviewHelper.getInstance().populateManyRecentApps();
    }

    public void afterTestOpenAllAppsContainer(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    /** Starts from the home screen, and measures jank while opening the all apps container. */
    @JankTest(expectedFrames=100, beforeTest="prepareOpenAllAppsContainer",
            beforeLoop="resetAndOpenRecents", afterTest="afterTestOpenAllAppsContainer")
    @GfxMonitor(processName="#getLauncherPackage")
    public void testOpenAllAppsContainer() throws UiObjectNotFoundException {
        for (int i = 0; i < INNER_LOOP * 2; i++) {
            fromRecentsToAllApps();
            mDevice.waitForIdle();
            fromAllAppsToRecents();
            mDevice.waitForIdle();
        }
    }

    public void openAllApps() throws UiObjectNotFoundException, IOException {
        mLauncherStrategy.openAllApps(false);
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestAllAppsContainerSwipe(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    /** Starts from the all apps container, and measures jank while swiping between pages */
    @JankTest(beforeTest="openAllApps", afterTest="afterTestAllAppsContainerSwipe",
            expectedFrames=100)
    @GfxMonitor(processName="#getLauncherPackage")
    public void testAllAppsContainerSwipe() {
        UiObject2 allApps = mDevice.findObject(mLauncherStrategy.getAllAppsSelector());
        final int allAppsHeight = allApps.getVisibleBounds().height();
        Direction dir = mLauncherStrategy.getAllAppsScrollDirection();
        for (int i = 0; i < INNER_LOOP * 2; i++) {
            if (dir == Direction.DOWN) {
                // Start the gesture in the center to avoid starting at elements near the top.
                allApps.setGestureMargins(0, 0, 0, allAppsHeight / 2);
            }
            allApps.fling(dir, FLING_SPEED);
            if (dir == Direction.DOWN) {
                // Start the gesture in the center, for symmetry.
                allApps.setGestureMargins(0, allAppsHeight / 2, 0, 0);
            }
            allApps.fling(Direction.reverse(dir), FLING_SPEED);
        }
    }

    public void makeHomeScrollable() throws UiObjectNotFoundException, IOException {
        goHome();
        UiObject2 homeScreen = mDevice.findObject(mLauncherStrategy.getWorkspaceSelector());
        Rect r = homeScreen.getVisibleBounds();
        if (!homeScreen.isScrollable()) {
            // Add the Chrome icon to the first launcher screen.
            // This is specifically for Bullhead, where you can't add an icon
            // to the second launcher screen without one on the first.
            UiObject2 chrome = mDevice.findObject(By.text("Chrome"));
            Point dest = new Point(mDevice.getDisplayWidth()/2, r.centerY());
            chrome.drag(dest, 2000);
            // Drag Camera icon to next screen
            UiObject2 camera = mDevice.findObject(By.text("Camera"));
            dest = new Point(mDevice.getDisplayWidth(), r.centerY());
            camera.drag(dest, 2000);
        }
        mDevice.waitForIdle();
        assertTrue("home screen workspace still not scrollable", homeScreen.isScrollable());
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestHomeScreenSwipe(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    /** Starts from the home screen, and measures jank while swiping between pages */
    @JankTest(beforeTest="makeHomeScrollable", afterTest="afterTestHomeScreenSwipe",
              expectedFrames=100)
    @GfxMonitor(processName="#getLauncherPackage")
    public void testHomeScreenSwipe() {
        UiObject2 workspace = mDevice.findObject(mLauncherStrategy.getWorkspaceSelector());
        Direction dir = mLauncherStrategy.getWorkspaceScrollDirection();
        for (int i = 0; i < INNER_LOOP * 2; i++) {
            workspace.fling(dir);
            workspace.fling(Direction.reverse(dir));
        }
    }

    public void openAllWidgets() throws UiObjectNotFoundException, IOException {
        mLauncherStrategy.openAllWidgets(true);
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterTestWidgetsContainerFling(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    /** Starts from the widgets container, and measures jank while swiping between pages */
    @JankTest(beforeTest="openAllWidgets", afterTest="afterTestWidgetsContainerFling",
              expectedFrames=100)
    @GfxMonitor(processName="#getLauncherPackage")
    public void testWidgetsContainerFling() {
        UiObject2 allWidgets = mDevice.findObject(mLauncherStrategy.getAllWidgetsSelector());
        Direction dir = mLauncherStrategy.getAllWidgetsScrollDirection();
        for (int i = 0; i < INNER_LOOP; i++) {
            allWidgets.fling(dir, FLING_SPEED);
            allWidgets.fling(Direction.reverse(dir), FLING_SPEED);
        }
    }

    public void beforeOpenCloseMessagesApp() throws UiObjectNotFoundException, IOException {
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterOpenCloseMessagesApp(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    private void pressUiHome() throws RemoteException {
        mDevice.findObject(By.res(SYSTEMUI_PACKAGE, "home")).click();
        mDevice.waitForIdle();
    }

    /**
     * Opens and closes the Messages app repeatedly, measuring jank for synchronized app
     * transitions.
     */
    @JankTest(beforeTest="beforeOpenCloseMessagesApp", afterTest="afterOpenCloseMessagesApp",
            expectedFrames=90)
    @GfxMonitor(processName="#getLauncherPackage")
    public void testOpenCloseMessagesApp() throws Exception {
        for (int i = 0; i < INNER_LOOP; i++) {
            mLauncherStrategy.launch("Messages", "com.google.android.apps.messaging");
            pressUiHome();
        }
    }

    public void beforeOpenMessagesAppFromRecents() throws UiObjectNotFoundException, IOException {
        goHome();
        mLauncherStrategy.launch("Messages", "com.google.android.apps.messaging");
        TimeResultLogger.writeTimeStampLogStart(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
    }

    public void afterOpenMessagesAppFromRecents(Bundle metrics) throws IOException {
        TimeResultLogger.writeTimeStampLogEnd(String.format("%s-%s",
                getClass().getSimpleName(), getName()), TIMESTAMP_FILE);
        TimeResultLogger.writeResultToFile(String.format("%s-%s",
                getClass().getSimpleName(), getName()), RESULTS_FILE, metrics);
        super.afterTest(metrics);
    }

    /**
     * Opens the Messages app repeatedly from recents, measuring jank for synchronized app
     * transitions.
     */
    @JankTest(beforeTest="beforeOpenMessagesAppFromRecents",
            afterTest="afterOpenMessagesAppFromRecents", expectedFrames=80)
    @GfxMonitor(processName="#getLauncherPackage")
    public void testOpenMessagesAppFromRecents() throws Exception {
        for (int i = 0; i < INNER_LOOP; i++) {
            SystemUiJankTests.openRecents(getInstrumentation().getTargetContext(), mDevice);
            mLauncherStrategy.launch("Messages", "com.google.android.apps.messaging");
        }
    }
}
