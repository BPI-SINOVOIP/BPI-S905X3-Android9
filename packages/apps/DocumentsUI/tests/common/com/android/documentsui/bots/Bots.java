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

package com.android.documentsui.bots;

import static junit.framework.Assert.assertNotNull;

import android.app.UiAutomation;
import android.content.Context;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiSelector;
import android.support.test.uiautomator.Until;

/**
 * Handy collection of bots for working with Files app.
 */
public final class Bots {

    private static final int TIMEOUT = 5000;

    public final BreadBot breadcrumb;
    public final DirectoryListBot directory;
    public final SortHeaderBot sortHeader;
    public final KeyboardBot keyboard;
    public final SidebarBot roots;
    public final SearchBot search;
    public final GestureBot gesture;
    public final MenuBot menu;
    public final UiBot main;
    public final InspectorBot inspector;

    public Bots(UiDevice device, UiAutomation automation, Context context, int timeout) {
        main = new UiBot(device, context, TIMEOUT);
        breadcrumb = new BreadBot(device, context, TIMEOUT, main);
        roots = new SidebarBot(device, context, TIMEOUT);
        directory = new DirectoryListBot(device, automation, context, TIMEOUT);
        sortHeader = new SortHeaderBot(device, context, TIMEOUT);
        keyboard = new KeyboardBot(device, context, TIMEOUT);
        search = new SearchBot(device, context, TIMEOUT);
        gesture = new GestureBot(device, automation, context, TIMEOUT);
        menu = new MenuBot(device, context, TIMEOUT);
        inspector = new InspectorBot(device, context, TIMEOUT);
    }

    /**
     * A test helper class that provides support for controlling directory list
     * and making assertions against the state of it.
     */
    static abstract class BaseBot {
        public final UiDevice mDevice;
        final Context mContext;
        final int mTimeout;

        BaseBot(UiDevice device, Context context, int timeout) {
            mDevice = device;
            mContext = context;
            mTimeout = timeout;
        }

        /**
         * Asserts that the specified view or one of its descendents has focus.
         */
        protected void assertHasFocus(String resourceName) {
            UiObject2 candidate = mDevice.findObject(By.res(resourceName));
            assertNotNull("Expected " + resourceName + " to have focus, but it didn't.",
                candidate.findObject(By.focused(true)));
        }

        protected UiObject2 find(BySelector selector) {
            mDevice.wait(Until.findObject(selector), mTimeout);
            return mDevice.findObject(selector);
        }

        protected UiObject findObject(String resourceId) {
            final UiSelector object = new UiSelector().resourceId(resourceId);
            return mDevice.findObject(object);
        }

        protected UiObject findObject(String parentResourceId, String childResourceId) {
            final UiSelector selector = new UiSelector()
                    .resourceId(parentResourceId)
                    .childSelector(new UiSelector().resourceId(childResourceId));
            return mDevice.findObject(selector);
        }

        protected void waitForIdle() {
            mDevice.waitForIdle(mTimeout);
        }
    }

}
