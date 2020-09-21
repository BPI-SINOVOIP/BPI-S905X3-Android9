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

package com.android.systemui.statusbar.phone;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.os.SystemClock;
import android.service.notification.StatusBarNotification;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.MotionEvent;
import android.view.View;

import com.android.systemui.SysuiTestCase;
import com.android.systemui.statusbar.DragDownHelper;
import com.android.systemui.statusbar.ExpandableNotificationRow;
import com.android.systemui.statusbar.NotificationData;
import com.android.systemui.statusbar.StatusBarState;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class StatusBarWindowViewTest extends SysuiTestCase {

    private StatusBarWindowView mView;
    private StatusBar mStatusBar;
    private DragDownHelper mDragDownHelper;

    @Before
    public void setUp() {
        mView = new StatusBarWindowView(getContext(), null);
        mStatusBar = mock(StatusBar.class);
        mView.setService(mStatusBar);
        mDragDownHelper = mock(DragDownHelper.class);
        mView.setDragDownHelper(mDragDownHelper);
    }

    @Test
    public void testDragDownHelperCalledWhenDraggingDown() throws Exception {
        when(mStatusBar.getBarState()).thenReturn(StatusBarState.SHADE);
        when(mDragDownHelper.isDraggingDown()).thenReturn(true);
        long now = SystemClock.elapsedRealtime();
        MotionEvent ev = MotionEvent.obtain(now, now, MotionEvent.ACTION_UP, 0 /* x */, 0 /* y */,
                0 /* meta */);
        mView.onTouchEvent(ev);
        verify(mDragDownHelper).onTouchEvent(ev);
        ev.recycle();
    }
}
