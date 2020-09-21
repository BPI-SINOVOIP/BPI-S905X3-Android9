/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.animation.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.animation.ArgbEvaluator;
import android.animation.FloatArrayEvaluator;
import android.animation.FloatEvaluator;
import android.animation.IntArrayEvaluator;
import android.animation.IntEvaluator;
import android.animation.PointFEvaluator;
import android.animation.RectEvaluator;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.Rect;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Tests for the various Evaluator classes in android.animation
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class EvaluatorTest {
    private static final float EPSILON = 0.001f;

    @Test
    public void testFloatEvaluator() {
        float start = 0.0f;
        float end = 1.0f;
        float fraction = 0.5f;
        FloatEvaluator floatEvaluator = new FloatEvaluator();

        float result = floatEvaluator.evaluate(0, start, end);
        assertEquals(start, result, EPSILON);

        result = floatEvaluator.evaluate(fraction, start, end);
        assertEquals(.5f, result, EPSILON);

        result = floatEvaluator.evaluate(1, start, end);
        assertEquals(end, result, EPSILON);
    }

    @Test
    public void testFloatArrayEvaluator() {
        FloatArrayEvaluator evaluator = new FloatArrayEvaluator();
        floatArrayEvaluatorTestImpl(evaluator, null);

        float[] reusableArray = new float[2];
        FloatArrayEvaluator evaluator2 = new FloatArrayEvaluator(reusableArray);
        floatArrayEvaluatorTestImpl(evaluator2, reusableArray);
    }

    private void floatArrayEvaluatorTestImpl(FloatArrayEvaluator evaluator, float[] reusedArray) {
        float[] start = {0f, 0f};
        float[] end = {.8f, 1.0f};
        float fraction = 0.5f;

        float[] result = evaluator.evaluate(0, start, end);
        assertEquals(start[0], result[0], EPSILON);
        assertEquals(start[1], result[1], EPSILON);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(.4f, result[0], EPSILON);
        assertEquals(.5f, result[1], EPSILON);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end[0], result[0], EPSILON);
        assertEquals(end[1], result[1], EPSILON);

        if (reusedArray != null) {
            assertEquals(reusedArray, result);
        }
    }

    @Test
    public void testArgbEvaluator() throws Throwable {
        final int START =  0xffFF8080;
        final int END = 0xff8080FF;
        int aSTART = Color.alpha(START);
        int rSTART = Color.red(START);
        int gSTART = Color.green(START);
        int bSTART = Color.blue(START);
        int aEND = Color.alpha(END);
        int rEND = Color.red(END);
        int gEND = Color.green(END);
        int bEND = Color.blue(END);

        final ArgbEvaluator evaluator = new ArgbEvaluator();

        int result = (Integer) evaluator.evaluate(0, START, END);
        int aResult = Color.alpha(result);
        int rResult = Color.red(result);
        int gResult = Color.green(result);
        int bResult = Color.blue(result);
        assertEquals(aSTART, aResult);
        assertEquals(rSTART, rResult);
        assertEquals(gSTART, gResult);
        assertEquals(bSTART, bResult);

        result = (Integer) evaluator.evaluate(.5f, START, END);
        aResult = Color.alpha(result);
        rResult = Color.red(result);
        gResult = Color.green(result);
        bResult = Color.blue(result);
        assertEquals(0xff, aResult);
        assertEquals(0x80, gResult);
        if (rSTART < rEND) {
            assertTrue(rResult > rSTART && rResult < rEND);
        } else {
            assertTrue(rResult < rSTART && rResult > rEND);
        }
        if (bSTART < bEND) {
            assertTrue(bResult > bSTART && bResult < bEND);
        } else {
            assertTrue(bResult < bSTART && bResult > bEND);
        }

        result = (Integer) evaluator.evaluate(1, START, END);
        aResult = Color.alpha(result);
        rResult = Color.red(result);
        gResult = Color.green(result);
        bResult = Color.blue(result);
        assertEquals(aEND, aResult);
        assertEquals(rEND, rResult);
        assertEquals(gEND, gResult);
        assertEquals(bEND, bResult);
    }

    @Test
    public void testIntEvaluator() throws Throwable {
        final int start = 0;
        final int end = 100;
        final float fraction = 0.5f;
        final IntEvaluator intEvaluator = new IntEvaluator();

        int result = intEvaluator.evaluate(0, start, end);
        assertEquals(start, result);

        result = intEvaluator.evaluate(fraction, start, end);
        assertEquals(50, result);

        result = intEvaluator.evaluate(1, start, end);
        assertEquals(end, result);
    }

    @Test
    public void testIntArrayEvaluator() {
        IntArrayEvaluator evaluator = new IntArrayEvaluator();
        intArrayEvaluatorTestImpl(evaluator, null);

        int[] reusableArray = new int[2];
        IntArrayEvaluator evaluator2 = new IntArrayEvaluator(reusableArray);
        intArrayEvaluatorTestImpl(evaluator2, reusableArray);
    }

    private void intArrayEvaluatorTestImpl(IntArrayEvaluator evaluator, int[] reusedArray) {
        int[] start = {0, 0};
        int[] end = {80, 100};
        float fraction = 0.5f;

        int[] result = evaluator.evaluate(0, start, end);
        assertEquals(start[0], result[0]);
        assertEquals(start[1], result[1]);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(40, result[0]);
        assertEquals(50, result[1]);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end[0], result[0]);
        assertEquals(end[1], result[1]);

        if (reusedArray != null) {
            assertEquals(reusedArray, result);
        }
    }

    @Test
    public void testRectEvaluator() throws Throwable {
        final RectEvaluator evaluator = new RectEvaluator();
        rectEvaluatorTestImpl(evaluator, null);

        Rect reusableRect = new Rect();
        final RectEvaluator evaluator2 = new RectEvaluator(reusableRect);
        rectEvaluatorTestImpl(evaluator2, reusableRect);
    }

    private void rectEvaluatorTestImpl(RectEvaluator evaluator, Rect reusedRect) {
        final Rect start = new Rect(0, 0, 0, 0);
        final Rect end = new Rect(100, 200, 300, 400);
        final float fraction = 0.5f;

        Rect result = evaluator.evaluate(0, start, end);
        assertEquals(start.left, result.left, EPSILON);
        assertEquals(start.top, result.top, EPSILON);
        assertEquals(start.right, result.right, EPSILON);
        assertEquals(start.bottom, result.bottom, 001f);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(50, result.left, EPSILON);
        assertEquals(100, result.top, EPSILON);
        assertEquals(150, result.right, EPSILON);
        assertEquals(200, result.bottom, EPSILON);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end.left, result.left, EPSILON);
        assertEquals(end.top, result.top, EPSILON);
        assertEquals(end.right, result.right, EPSILON);
        assertEquals(end.bottom, result.bottom, EPSILON);

        if (reusedRect != null) {
            assertEquals(reusedRect, result);
        }
    }

    @Test
    public void testPointFEvaluator() throws Throwable {
        final PointFEvaluator evaluator = new PointFEvaluator();
        pointFEvaluatorTestImpl(evaluator, null);

        PointF reusablePoint = new PointF();
        final PointFEvaluator evaluator2 = new PointFEvaluator(reusablePoint);
        pointFEvaluatorTestImpl(evaluator2, reusablePoint);
    }

    private void pointFEvaluatorTestImpl(PointFEvaluator evaluator, PointF reusedPoint) {
        final PointF start = new PointF(0, 0);
        final PointF end = new PointF(100, 200);
        final float fraction = 0.5f;

        PointF result = evaluator.evaluate(0, start, end);
        assertEquals(start.x, result.x, EPSILON);
        assertEquals(start.y, result.y, EPSILON);

        result = evaluator.evaluate(fraction, start, end);
        assertEquals(50, result.x, EPSILON);
        assertEquals(100, result.y, EPSILON);

        result = evaluator.evaluate(1, start, end);
        assertEquals(end.x, result.x, EPSILON);
        assertEquals(end.y, result.y, EPSILON);

        if (reusedPoint != null) {
            assertEquals(reusedPoint, result);
        }
    }
}

