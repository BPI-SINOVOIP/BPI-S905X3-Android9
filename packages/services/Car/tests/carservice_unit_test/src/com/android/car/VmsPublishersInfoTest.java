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

package com.android.car;

import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import java.util.Arrays;
import java.util.Map;

@SmallTest
public class VmsPublishersInfoTest extends AndroidTestCase {
    public static final byte[] MOCK_INFO_0 = new byte[]{2, 3, 5, 7, 11, 13, 17};
    public static final byte[] SAME_MOCK_INFO_0 = new byte[]{2, 3, 5, 7, 11, 13, 17};
    public static final byte[] MOCK_INFO_1 = new byte[]{2, 3, 5, 7, 11, 13, 17, 19};

    private VmsPublishersInfo mVmsPublishersInfo;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mVmsPublishersInfo = new VmsPublishersInfo();
    }

    // Test one info sanity
    public void testSingleInfo() throws Exception {
        int id = mVmsPublishersInfo.getIdForInfo(MOCK_INFO_0);
        assertEquals(0, id);

        byte[] info = mVmsPublishersInfo.getPublisherInfo(id);
        assertTrue(Arrays.equals(MOCK_INFO_0, info));
    }

    // Test one info sanity - wrong ID fails.
    public void testSingleInfoWrongId() throws Exception {
        int id = mVmsPublishersInfo.getIdForInfo(MOCK_INFO_0);
        assertEquals(0, id);

        try {
            byte[] info = mVmsPublishersInfo.getPublisherInfo(id + 1);
        }
        catch (NullPointerException e) {
            return;
        }
        fail();
    }

    // Test two infos.
    public void testTwoInfos() throws Exception {
        int id0 = mVmsPublishersInfo.getIdForInfo(MOCK_INFO_0);
        int id1 = mVmsPublishersInfo.getIdForInfo(MOCK_INFO_1);
        assertEquals(0, id0);
        assertEquals(1, id1);

        byte[] info0 = mVmsPublishersInfo.getPublisherInfo(id0);
        byte[] info1 = mVmsPublishersInfo.getPublisherInfo(id1);
        assertTrue(Arrays.equals(MOCK_INFO_0, info0));
        assertTrue(Arrays.equals(MOCK_INFO_1, info1));
    }

    // Test same info twice get the same ID.
    public void testSingleInfoInsertedTwice() throws Exception {
        int id = mVmsPublishersInfo.getIdForInfo(MOCK_INFO_0);
        assertEquals(0, id);

        int sameId = mVmsPublishersInfo.getIdForInfo(SAME_MOCK_INFO_0);
        assertEquals(sameId, id);
    }
}
