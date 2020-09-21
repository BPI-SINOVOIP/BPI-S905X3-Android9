/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.systemui.qs.customize;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.support.test.filters.SmallTest;
import android.testing.AndroidTestingRunner;
import android.testing.TestableLooper;
import android.testing.TestableLooper.RunWithLooper;

import com.android.systemui.SysuiTestCase;
import com.android.systemui.qs.QSTileHost;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Collections;

@RunWith(AndroidTestingRunner.class)
@RunWithLooper
@SmallTest
public class TileAdapterTest extends SysuiTestCase {

    private TileAdapter mTileAdapter;

    @Before
    public void setup() throws Exception {
        TestableLooper.get(this).runWithLooper(() -> mTileAdapter = new TileAdapter(mContext));
    }

    @Test
    public void testResetNotifiesHost() {
        QSTileHost host = mock(QSTileHost.class);
        mTileAdapter.resetTileSpecs(host, Collections.emptyList());
        verify(host).changeTiles(any(), any());
    }
}
