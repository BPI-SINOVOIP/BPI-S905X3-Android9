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

package com.android.uibench.janktests;

import android.os.Bundle;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.GfxFrameStatsMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.uiautomator.UiDevice;
import android.text.TextUtils;
import android.view.KeyEvent;

import static com.android.uibench.janktests.UiBenchJankTestsHelper.SHORT_EXPECTED_FRAMES;
import static com.android.uibench.janktests.UiBenchJankTestsHelper.PACKAGE_NAME;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * Jank benchmark tests for leanback
 */
public class UiBenchLeanbackJankTests extends JankTestBase {

    public static final String EXTRA_BITMAP_UPLOAD = "extra_bitmap_upload";
    public static final String EXTRA_SHOW_FAST_LANE = "extra_show_fast_lane";

    /** Annotation for test option */
    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    public @interface Option {
        /**
         * Name of the activity to launch "e.g. "leanback.BrowseActivity"
         */
        String activity();

        /**
         * Bitmap upload is enabled
         */
        boolean extraBitmapUpload() default true;

        /**
         * Initially show fast lane
         */
        boolean extraShowFastLane() default true;

        /**
         * Expected text component indicate the activity is loaded
         */
        String expectedText() default "Row";
    }

    protected UiDevice mDevice;
    protected UiBenchJankTestsHelper mHelper;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mDevice.setOrientationNatural();
        mHelper = UiBenchJankTestsHelper.getInstance(
                this.getInstrumentation().getContext(), mDevice);
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    void scrollDownAndUp() {
        for (int i = 0; i < 3; i++) {
            mHelper.pressKeyCode(KeyEvent.KEYCODE_DPAD_DOWN);
        }
        for (int i = 0; i < 3; i++) {
            mHelper.pressKeyCode(KeyEvent.KEYCODE_DPAD_UP);
        }
    }

    @Override
    public void beforeTest() throws Exception {
        Method method = getClass().getMethod(getName());
        Option option = method.getAnnotation(Option.class);
        Bundle extrasBundle = new Bundle();
        extrasBundle.putBoolean(EXTRA_BITMAP_UPLOAD, option.extraBitmapUpload());
        extrasBundle.putBoolean(EXTRA_SHOW_FAST_LANE, option.extraShowFastLane());
        mHelper.launchActivity(option.activity(), extrasBundle, option.expectedText());
    }

    /**
     * Vertically scroll BrowseFragment in the fast lane
     */
    @JankTest(expectedFrames = SHORT_EXPECTED_FRAMES)
    @Option(activity = "leanback.BrowseActivity")
    @GfxMonitor(processName = PACKAGE_NAME)
    @GfxFrameStatsMonitor(processName = PACKAGE_NAME)
    public void testBrowseFastLaneScroll() {
        scrollDownAndUp();
    }

    /**
     * Vertically scroll BrowseFragment in the content (fast lane closed)
     */
    @JankTest(expectedFrames = SHORT_EXPECTED_FRAMES)
    @Option(activity = "leanback.BrowseActivity", extraShowFastLane = false)
    @GfxMonitor(processName = PACKAGE_NAME)
    @GfxFrameStatsMonitor(processName = PACKAGE_NAME)
    public void testBrowseContentScroll() {
        scrollDownAndUp();
    }

    /**
     * Vertically scroll BrowseFragment in the fast lane
     * option: no bitmap upload
     */
    @JankTest(expectedFrames = SHORT_EXPECTED_FRAMES)
    @Option(activity = "leanback.BrowseActivity", extraBitmapUpload = false)
    @GfxMonitor(processName = PACKAGE_NAME)
    @GfxFrameStatsMonitor(processName = PACKAGE_NAME)
    public void testBrowseFastLaneScrollNoBitmapUpload() {
        scrollDownAndUp();
    }

    /**
     * Vertically scroll BrowseFragment in the content (fast lane closed)
     * option: no bitmap upload
     */
    @JankTest(expectedFrames = SHORT_EXPECTED_FRAMES)
    @Option(activity = "leanback.BrowseActivity", extraBitmapUpload = false,
            extraShowFastLane = false)
    @GfxMonitor(processName = PACKAGE_NAME)
    @GfxFrameStatsMonitor(processName = PACKAGE_NAME)
    public void testBrowseContentScrollNoBitmapUpload() {
        scrollDownAndUp();
    }

}
