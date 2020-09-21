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
package android.support.test.launcherhelper;

import android.app.Instrumentation;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.system.helpers.CommandsHelper;

import junit.framework.Assert;

public class AutoLauncherStrategy implements IAutoLauncherStrategy {

    private static final String LOG_TAG = AutoLauncherStrategy.class.getSimpleName();
    private static final String CAR_LENSPICKER = "com.android.car.carlauncher";

    private static final long APP_INIT_WAIT = 10000;
    private static final int OPEN_FACET_RETRY_TIME = 5;

    //todo: Remove x and y axis and use resource ID's.
    private static final int FACET_APPS = 560;
    private static final int MAP_FACET = 250;

    private static final BySelector R_ID_LENSPICKER_PAGE_DOWN =
            By.res(CAR_LENSPICKER, "page_down");
    private static final BySelector R_ID_LENSPICKER_LIST =
            By.res(CAR_LENSPICKER, "list_view");

    protected UiDevice mDevice;
    private Instrumentation mInstrumentation;

    @Override
    public String getSupportedLauncherPackage() {
        return CAR_LENSPICKER;
    }

    @Override
    public void setUiDevice(UiDevice uiDevice) {
        mDevice = uiDevice;
    }

    @Override
    public void setInstrumentation(Instrumentation instrumentation) {
        mInstrumentation = instrumentation;
    }

    @Override
    public void open() {

    }

    @Override
    public void openDialFacet() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @Override
    public void openMediaFacet(String appName) {
        BySelector button = By.clickable(true).hasDescendant(By.text(appName));
        for (int tries = 3; tries >= 0; tries--) {
            // TODO: Switch this to intents. It doesn't appear to work via intents on my system.
            CommandsHelper.getInstance(mInstrumentation).executeShellCommand(
                    "am start -n com.android.support.car.lenspicker/.LensPickerActivity"
                            + " --esa categories android.intent.category.APP_MUSIC");
            mDevice.wait(Until.findObject(button), APP_INIT_WAIT);
            if (mDevice.hasObject(button)) {
                break;
            }
        }
        UiObject2 choice = mDevice.findObject(button);
        Assert.assertNotNull("Unable to find application " + appName, choice);
        choice.click();
        mDevice.wait(Until.gone(button), APP_INIT_WAIT);
        Assert.assertFalse("Failed to exit media menu.", mDevice.hasObject(button));
        mDevice.waitForIdle(APP_INIT_WAIT);
    }

    @Override
    public void openSettingsFacet(String appName) {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @Override
    public void openMapsFacet(String appName) {
        CommandsHelper.getInstance(mInstrumentation).executeShellCommand(
                "input tap " + MAP_FACET + " " + FACET_APPS);
    }

    @Override
    public void openHomeFacet() {
        UiDevice.getInstance(mInstrumentation).pressHome();
    }

    public void openApp(String appName) {
        do {
            // TODO: Switch this to intents. It doesn't appear to work via intents on my system.
            CommandsHelper.getInstance(mInstrumentation).executeShellCommand(
                    "am start -n com.android.car.carlauncher/.AppGridActivity");
        }
        //R_ID_LENSPICKER_LIST to open app if scrollContainer not avilable.
        while (!mDevice.hasObject(R_ID_LENSPICKER_PAGE_DOWN)
                && !mDevice.hasObject(R_ID_LENSPICKER_LIST));

        UiObject2 scrollContainer = mDevice.findObject(R_ID_LENSPICKER_PAGE_DOWN);

        if (scrollContainer != null) {

            if (!mDevice.hasObject(By.text(appName))) {
                do {
                    scrollContainer.scroll(Direction.DOWN, 1.0f);
                }
                while (!mDevice.hasObject(By.text(appName)) && scrollContainer.isEnabled());
            }
        }

        UiObject2 application = mDevice.wait(Until.findObject(By.text(appName)), APP_INIT_WAIT);
        if (application != null) {
            application.click();
            mDevice.waitForIdle();
        } else {
            Assert.fail("Unable to find application " + appName);
        }
    }

    @SuppressWarnings("unused")
    @Override
    public UiObject2 openAllApps(boolean reset) {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getAllAppsButtonSelector() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getAllAppsSelector() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public Direction getAllAppsScrollDirection() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public UiObject2 openAllWidgets(boolean reset) {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getAllWidgetsSelector() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public Direction getAllWidgetsScrollDirection() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getWorkspaceSelector() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getHotSeatSelector() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public Direction getWorkspaceScrollDirection() {
        throw new UnsupportedOperationException(
                "The feature not supported on Auto");
    }

    @SuppressWarnings("unused")
    @Override
    public long launch(String appName, String packageName) {
        return 0;
    }
}
