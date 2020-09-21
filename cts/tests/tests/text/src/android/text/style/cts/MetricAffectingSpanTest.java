/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.text.style.cts;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.TextPaint;
import android.text.style.CharacterStyle;
import android.text.style.MetricAffectingSpan;
import android.text.style.SuperscriptSpan;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class MetricAffectingSpanTest {
    @Test
    public void testGetUnderlying() {
        MetricAffectingSpan metricAffectingSpan = new MyMetricAffectingSpan();
        assertSame(metricAffectingSpan, metricAffectingSpan.getUnderlying());

        metricAffectingSpan = new SuperscriptSpan();
        CharacterStyle result = CharacterStyle.wrap(metricAffectingSpan);
        assertNotNull(result);
        assertTrue(result instanceof MetricAffectingSpan);
        assertSame(metricAffectingSpan, result.getUnderlying());
    }

    private class MyMetricAffectingSpan extends MetricAffectingSpan {
        @Override
        public void updateMeasureState(TextPaint p) {
        }

        @Override
        public void updateDrawState(TextPaint tp) {
        }
    }
}
