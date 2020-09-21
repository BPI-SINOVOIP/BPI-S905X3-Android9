/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.support.test.launcherhelper;

import android.graphics.Point;
import android.os.Build;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;

import junit.framework.Assert;

/**
 * Implementation of {@link ILauncherStrategy} to support Nexus launcher
 */
public class NexusLauncherStrategy extends BaseLauncher3Strategy {

    private static final String LAUNCHER_PKG = "com.google.android.apps.nexuslauncher";

    @Override
    public String getSupportedLauncherPackage() {
        return LAUNCHER_PKG;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getAllAppsSelector() {
        return By.res(getSupportedLauncherPackage(), "apps_view");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getAllAppsButtonSelector() {
        throw new UnsupportedOperationException("UI element no longer exists.");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getHotSeatSelector() {
        return By.res(getSupportedLauncherPackage(), "hotseat");
    }

    private BySelector getLauncherOverviewSelector() {
        return By.res(LAUNCHER_PKG, "overview_panel");
    }

    @Override
    public void open() {
        mDevice.pressHome();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 openAllApps(boolean reset) {
        // If not on all apps or to reset, then go to launcher and re-open all apps.
        if (!mDevice.hasObject(getAllAppsSelector()) || mDevice.hasObject(
                getLauncherOverviewSelector()) || reset) {
            // Restart from the launcher home screen.
            open();
            mDevice.waitForIdle();
            Assert.assertTrue("openAllApps: can't go to home screen",
                    !mDevice.hasObject(getAllAppsSelector()) && !mDevice.hasObject(
                            getLauncherOverviewSelector()));
            if (Build.VERSION.FIRST_SDK_INT >= Build.VERSION_CODES.O) {
                int midX = mDevice.getDisplayWidth() / 2;
                int height = mDevice.getDisplayHeight();
                // Swipe from 5/6ths down the screen to 1/6th down the screen.
                mDevice.swipe(
                        midX,
                        height * 5 / 6,
                        midX,
                        height / 6,
                        (height * 2 / 3) / 100); // 100 px/step
            } else {
                // Swipe from the hotseat to near the top, e.g. 10% of the screen.
                UiObject2 hotseat = mDevice.wait(Until.findObject(getHotSeatSelector()), 2500);
                Point start = hotseat.getVisibleCenter();
                int endY = (int) (mDevice.getDisplayHeight() * 0.1f);
                mDevice.swipe(
                        start.x,
                        start.y,
                        start.x,
                        endY,
                        (start.y - endY) / 100); // 100 px/step
            }
        }
        UiObject2 allAppsContainer = mDevice.wait(Until.findObject(getAllAppsSelector()), 2500);
        Assert.assertNotNull("openAllApps: did not find all apps container", allAppsContainer);
        return allAppsContainer;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 openAllWidgets(boolean reset) {
        if (!mDevice.hasObject(getAllWidgetsSelector())) {
            open();
            // trigger the wallpapers/widgets/settings view
            mDevice.pressMenu();
            mDevice.waitForIdle();
            UiObject2 optionsPopup = mDevice.findObject(
                    By.res(getSupportedLauncherPackage(), "deep_shortcuts_container"));
            optionsPopup.findObject(By.text("Widgets")).click();
        }
        UiObject2 allWidgetsContainer = mDevice.wait(
                Until.findObject(getAllWidgetsSelector()), 2000);
        Assert.assertNotNull("openAllWidgets: did not find all widgets container",
                allWidgetsContainer);
        if (reset) {
            CommonLauncherHelper.getInstance(mDevice).scrollBackToBeginning(
                    allWidgetsContainer, Direction.reverse(getAllWidgetsScrollDirection()));
        }
        return allWidgetsContainer;
    }
}
