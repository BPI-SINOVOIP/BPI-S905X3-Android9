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
package android.transition.cts;

import static org.junit.Assert.assertSame;

import android.graphics.Path;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.transition.PatternPathMotion;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class PatternPathMotionTest extends PathMotionTest {
    @Test
    public void testStraightPath() {
        Path pattern = new Path();
        pattern.moveTo(100, 500);
        pattern.lineTo(300, 1000);

        PatternPathMotion pathMotion = new PatternPathMotion(pattern);
        assertPathMatches(pattern, pathMotion.getPatternPath());

        Path expected = new Path();
        expected.moveTo(0, 0);
        expected.lineTo(100, 100);

        assertPathMatches(expected, pathMotion.getPath(0, 0, 100, 100));
    }

    @Test
    public void testCurve() {
        Path pattern = new Path();
        pattern.addArc(0, 0, 100, 100, 0, 180);

        PatternPathMotion pathMotion = new PatternPathMotion(pattern);
        assertPathMatches(pattern, pathMotion.getPatternPath());

        Path expected = new Path();
        expected.addArc(-50, 0, 50, 100, -90, 180);

        assertPathMatches(expected, pathMotion.getPath(0, 0, 0, 100));
    }

    @Test
    public void testSetPatternPath() {
        Path pattern = new Path();
        pattern.addArc(0, 0, 100, 100, 0, 180);

        PatternPathMotion patternPathMotion = new PatternPathMotion();
        patternPathMotion.setPatternPath(pattern);
        assertSame(pattern, patternPathMotion.getPatternPath());
    }
}

