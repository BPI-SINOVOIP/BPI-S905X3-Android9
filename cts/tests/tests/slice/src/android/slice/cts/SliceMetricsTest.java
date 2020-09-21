/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.slice.cts;

import android.app.slice.SliceMetrics;
import android.content.Context;
import android.metrics.LogMaker;
import android.metrics.MetricsReader;
import android.net.Uri;
import android.support.test.InstrumentationRegistry;
import android.support.test.metricshelper.MetricsAsserts;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import com.android.internal.logging.nano.MetricsProto.MetricsEvent;

@RunWith(AndroidJUnit4.class)
public class SliceMetricsTest {

    private static final Uri BASE_URI = Uri.parse("content://android.slice.cts.local/");
    private static final Uri SUB_SLICE_URI = Uri.parse("content://android.slice.cts.local/sub");
    private final Context mContext = InstrumentationRegistry.getContext();

    private MetricsReader mMetricsReader;
    private SliceMetrics mSliceMetrics;

    @Before
    public void setup() {
        mSliceMetrics = new SliceMetrics(mContext, BASE_URI);
        mMetricsReader = new MetricsReader();
        mMetricsReader.checkpoint(); // clear out old logs
    }

    @Test
    public void testLogVisible() {
        mSliceMetrics.logVisible();
        MetricsAsserts.assertHasLog("Missing slice visible log", mMetricsReader,
            getLogMaker().setCategory(MetricsEvent.SLICE).setType(MetricsEvent.TYPE_OPEN));
    }

    @Test
    public void testLogHidden() {
        mSliceMetrics.logHidden();
        MetricsAsserts.assertHasLog("Missing slice hidden log", mMetricsReader,
            getLogMaker().setCategory(MetricsEvent.SLICE).setType(MetricsEvent.TYPE_CLOSE));
    }

    @Test
    public void testLogOnTouch() {
        mSliceMetrics.logTouch(0, SUB_SLICE_URI);
        MetricsAsserts.assertHasLog("Missing slice touch log", mMetricsReader,
            getLogMaker().setCategory(MetricsEvent.SLICE)
                .setType(MetricsEvent.TYPE_ACTION)
                .addTaggedData(MetricsEvent.FIELD_SUBSLICE_AUTHORITY, SUB_SLICE_URI.getAuthority())
                .addTaggedData(MetricsEvent.FIELD_SUBSLICE_PATH, SUB_SLICE_URI.getPath()));
    }

    private LogMaker getLogMaker() {
        LogMaker logMaker = new LogMaker(MetricsEvent.VIEW_UNKNOWN);
        logMaker.addTaggedData(MetricsEvent.FIELD_SLICE_AUTHORITY, BASE_URI.getAuthority());
        logMaker.addTaggedData(MetricsEvent.FIELD_SLICE_PATH, BASE_URI.getPath());
        return logMaker;
    }
}
