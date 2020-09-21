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

package com.android.server.wm;

import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY;
import static android.view.WindowManager.LayoutParams.TYPE_BASE_APPLICATION;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Matchers.eq;

import android.platform.test.annotations.Presubmit;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.mockito.Mock;

import java.util.function.Consumer;

/**
 * Tests for {@link WindowContainer#forAllWindows} and various implementations.
 */
@SmallTest
@Presubmit
@RunWith(AndroidJUnit4.class)
public class WindowContainerTraversalTests extends WindowTestsBase {

    @Test
    public void testDockedDividerPosition() throws Exception {
        final WindowState splitScreenWindow = createWindowOnStack(null,
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD, TYPE_BASE_APPLICATION,
                mDisplayContent, "splitScreenWindow");
        final WindowState splitScreenSecondaryWindow = createWindowOnStack(null,
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD,
                TYPE_BASE_APPLICATION, mDisplayContent, "splitScreenSecondaryWindow");

        sWm.mInputMethodTarget = splitScreenWindow;

        Consumer<WindowState> c = mock(Consumer.class);
        mDisplayContent.forAllWindows(c, false);

        verify(c).accept(eq(mDockedDividerWindow));
        verify(c).accept(eq(mImeWindow));
    }
}
