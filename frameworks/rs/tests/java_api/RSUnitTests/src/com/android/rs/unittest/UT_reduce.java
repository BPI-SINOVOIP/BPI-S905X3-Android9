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

/* UT_reduce_backward.java is a much simpler version of this test
 * case that exercises pragmas after the functions (backward
 * reference), whereas this test case exercises the pragmas before
 * the functions (forward reference).
 */

package com.android.rs.unittest;

import android.content.Context;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.Float2;
import android.renderscript.Int2;
import android.renderscript.Int3;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicHistogram;
import android.renderscript.Type;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Random;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

public class UT_reduce extends UnitTest {
    private static final String TAG = "reduce";

    public UT_reduce(Context ctx) {
        super("reduce", ctx);
    }

    private static class timing {
        timing(long myJavaStart, long myJavaEnd, long myRsStart,
               long myCopyStart, long myKernelStart, long myRsEnd,
               Allocation... myInputs) {
            javaStart = myJavaStart;
            javaEnd = myJavaEnd;
            rsStart = myRsStart;
            copyStart = myCopyStart;
            kernelStart = myKernelStart;
            rsEnd = myRsEnd;

            inputBytes = 0;
            for (Allocation input : myInputs)
                inputBytes += input.getBytesSize();

            inputCells = (myInputs.length > 0) ? myInputs[0].getType().getCount() : 0;
        }

        timing(long myInputCells) {
            inputCells = myInputCells;
        }

        private long javaStart = -1;
        private long javaEnd = -1;
        private long rsStart = -1;
        private long copyStart = -1;
        private long kernelStart = -1;
        private long rsEnd = -1;
        private long inputBytes = -1;
        private long inputCells = -1;

        public long javaTime() {
            return javaEnd - javaStart;
        }

        public long rsTime() {
            return rsEnd - rsStart;
        }

        public long kernelTime() {
            return rsEnd - kernelStart;
        }

        public long overheadTime() {
            return kernelStart - rsStart;
        }

        public long allocationTime() {
            return copyStart - rsStart;
        }

        public long copyTime() {
            return kernelStart - copyStart;
        }

        public static String string(long myJavaStart, long myJavaEnd, long myRsStart,
                                    long myCopyStart, long myKernelStart, long myRsEnd,
                                    Allocation... myInputs) {
            return (new timing(myJavaStart, myJavaEnd, myRsStart,
                    myCopyStart, myKernelStart, myRsEnd, myInputs)).string();
        }

        public static String string(long myInputCells) {
            return (new timing(myInputCells)).string();
        }

        public String string() {
            String result;
            if (javaStart >= 0) {
                result = "(java " + javaTime() + "ms, rs " + rsTime() + "ms = overhead " +
                        overheadTime() + "ms (alloc " + allocationTime() + "ms + copy " +
                        copyTime() + "ms) + kernel+get() " + kernelTime() + "ms)";
                if (inputCells > 0)
                    result += " ";
            } else {
                result = "";
            }
            if (inputCells > 0) {
                result += "(" + fmt.format(inputCells) + " cells";
                if (inputBytes > 0)
                    result += ", " + fmt.format(inputBytes) + " bytes";
                result += ")";
            }
            return result;
        }

        private static java.text.DecimalFormat fmt;

        static {
            fmt = new java.text.DecimalFormat("###,###");
        }
    }

    private byte[] createInputArrayByte(int len, int seed) {
        byte[] array = new byte[len];
        (new Random(seed)).nextBytes(array);
        return array;
    }

    private float[] createInputArrayFloat(int len, Random rand) {
        float[] array = new float[len];
        for (int i = 0; i < len; ++i) {
            final float val = rand.nextFloat();
            array[i] = rand.nextBoolean() ? val : -val;
        }
        return array;
    }

    private float[] createInputArrayFloat(int len, int seed) {
        return createInputArrayFloat(len, new Random(seed));
    }

    private float[] createInputArrayFloatWithInfs(int len, int infs, int seed) {
        Random rand = new Random(seed);
        float[] array = createInputArrayFloat(len, rand);
        for (int i = 0; i < infs; ++i)
            array[rand.nextInt(len)] = (rand.nextBoolean() ? Float.POSITIVE_INFINITY : Float.NEGATIVE_INFINITY);
        return array;
    }

    private int[] createInputArrayInt(int len, int seed) {
        Random rand = new Random(seed);
        int[] array = new int[len];
        for (int i = 0; i < len; ++i)
            array[i] = rand.nextInt();
        return array;
    }

    private int[] createInputArrayInt(int len, int seed, int eltRange) {
        Random rand = new Random(seed);
        int[] array = new int[len];
        for (int i = 0; i < len; ++i)
            array[i] = rand.nextInt(eltRange);
        return array;
    }

    private long[] intArrayToLong(final int[] input) {
        final long[] output = new long[input.length];

        for (int i = 0; i < input.length; ++i)
            output[i] = input[i];

        return output;
    }

    private <T extends Number> boolean result(String testName, final timing t,
                                              T javaResult, T rsResult) {
        final boolean success = javaResult.equals(rsResult);
        String status = (success ? "PASSED" : "FAILED");
        if (success && (t != null))
            status += " " + t.string();
        Log.i(TAG, testName + ": java " + javaResult + ", rs " + rsResult + ": " + status);
        return success;
    }

    private boolean result(String testName, final timing t,
                           final float[] javaResult, final float[] rsResult) {
        if (javaResult.length != rsResult.length) {
            Log.i(TAG, testName + ": java length " + javaResult.length +
                    ", rs length " + rsResult.length + ": FAILED");
            return false;
        }
        for (int i = 0; i < javaResult.length; ++i) {
            if (javaResult[i] != rsResult[i]) {
                Log.i(TAG, testName + "[" + i + "]: java " + javaResult[i] +
                        ", rs " + rsResult[i] + ": FAILED");
                return false;
            }
        }
        String status = "PASSED";
        if (t != null)
            status += " " + t.string();
        Log.i(TAG, testName + ": " + status);
        return true;
    }

    private boolean result(String testName, final timing t,
                           final long[] javaResult, final long[] rsResult) {
        if (javaResult.length != rsResult.length) {
            Log.i(TAG, testName + ": java length " + javaResult.length +
                    ", rs length " + rsResult.length + ": FAILED");
            return false;
        }
        for (int i = 0; i < javaResult.length; ++i) {
            if (javaResult[i] != rsResult[i]) {
                Log.i(TAG, testName + "[" + i + "]: java " + javaResult[i] +
                        ", rs " + rsResult[i] + ": FAILED");
                return false;
            }
        }
        String status = "PASSED";
        if (t != null)
            status += " " + t.string();
        Log.i(TAG, testName + ": " + status);
        return true;
    }

    private boolean result(String testName, final timing t,
                           final int[] javaResult, final int[] rsResult) {
        return result(testName, t, intArrayToLong(javaResult), intArrayToLong(rsResult));
    }

    private boolean result(String testName, final timing t, Int2 javaResult, Int2 rsResult) {
        final boolean success = (javaResult.x == rsResult.x) && (javaResult.y == rsResult.y);
        String status = (success ? "PASSED" : "FAILED");
        if (success && (t != null))
            status += " " + t.string();
        Log.i(TAG,
                testName +
                        ": java (" + javaResult.x + ", " + javaResult.y + ")" +
                        ", rs (" + rsResult.x + ", " + rsResult.y + ")" +
                        ": " + status);
        return success;
    }

    private boolean result(String testName, final timing t, Float2 javaResult, Float2 rsResult) {
        final boolean success = (javaResult.x == rsResult.x) && (javaResult.y == rsResult.y);
        String status = (success ? "PASSED" : "FAILED");
        if (success && (t != null))
            status += " " + t.string();
        Log.i(TAG,
                testName +
                        ": java (" + javaResult.x + ", " + javaResult.y + ")" +
                        ", rs (" + rsResult.x + ", " + rsResult.y + ")" +
                        ": " + status);
        return success;
    }

    ///////////////////////////////////////////////////////////////////

    private int addint(int[] input) {
        int result = 0;
        for (int idx = 0; idx < input.length; ++idx)
            result += input[idx];
        return result;
    }

    private boolean addint1D_array(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final int[] input = createInputArrayInt(size[0], seed, Integer.MAX_VALUE / size[0]);

        final int javaResult = addint(input);
        final int rsResult = s.reduce_addint(input).get();

        return result("addint1D_array", new timing(size[0]), javaResult, rsResult);
    }

    private boolean addint1D(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final int[] inputArray = createInputArrayInt(size[0], seed, Integer.MAX_VALUE / size[0]);

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final int javaResult = addint(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocation = Allocation.createSized(RS, Element.I32(RS), inputArray.length);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copyFrom(inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final int rsResult = s.reduce_addint(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final boolean success =
                result("addint1D",
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        javaResult, rsResult);
        inputAllocation.destroy();
        return success;
    }

    private boolean addint2D(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final int dimX = size[0];
        final int dimY = size[1];

        final int[] inputArray = createInputArrayInt(dimX * dimY, seed, Integer.MAX_VALUE / (dimX * dimY));

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final int javaResult = addint(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Type.Builder typeBuilder = new Type.Builder(RS, Element.I32(RS));
        typeBuilder.setX(dimX).setY(dimY);
        Allocation inputAllocation = Allocation.createTyped(RS, typeBuilder.create());

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copy2DRangeFrom(0, 0, dimX, dimY, inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final int rsResult = s.reduce_addint(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final boolean success =
                result("addint2D",
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        javaResult, rsResult);
        inputAllocation.destroy();
        return success;
    }

    private boolean addint3D(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final int dimX = size[0];
        final int dimY = size[1];
        final int dimZ = size[2];

        final int[] inputArray = createInputArrayInt(dimX * dimY * dimZ, seed, Integer.MAX_VALUE / (dimX * dimY * dimZ));

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final int javaResult = addint(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Type.Builder typeBuilder = new Type.Builder(RS, Element.I32(RS));
        typeBuilder.setX(dimX).setY(dimY).setZ(dimZ);
        Allocation inputAllocation = Allocation.createTyped(RS, typeBuilder.create());

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copy3DRangeFrom(0, 0, 0, dimX, dimY, dimZ, inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final int rsResult = s.reduce_addint(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final boolean success =
                result("addint3D",
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        javaResult, rsResult);
        inputAllocation.destroy();
        return success;
    }

    //-----------------------------------------------------------------

    private boolean patternInterleavedReduce(RenderScript RS, ScriptC_reduce s) {
        // Run two reduce operations without forcing completion between them.
        // We want to ensure that the driver can handle this, and that
        // temporary Allocations created to run the reduce operations survive
        // until get().

        boolean pass = true;

        final int inputSize = (1 << 18);

        final int[] input1 = createInputArrayInt(123, Integer.MAX_VALUE / inputSize);
        final int[] input2 = createInputArrayInt(456, Integer.MAX_VALUE / inputSize);

        final int javaResult1 = addint(input1);
        final int javaResult2 = addint(input2);

        final ScriptC_reduce.result_int rsResultFuture1 = s.reduce_addint(input1);
        final ScriptC_reduce.result_int rsResultFuture2 = s.reduce_addint(input2);

        pass &= result("patternInterleavedReduce (1)", new timing(inputSize),
                javaResult1, rsResultFuture1.get());
        pass &= result("patternInterleavedReduce (2)", new timing(inputSize),
                javaResult2, rsResultFuture2.get());

        return pass;
    }

    //-----------------------------------------------------------------

    private int[] sillySumIntoDecArray(final int[] input) {
        final int resultScalar = addint(input);
        final int[] result = new int[4];
        for (int i = 0; i < 4; ++i)
            result[i] = resultScalar / (i + 1);
        return result;
    }

    private int[] sillySumIntoIncArray(final int[] input) {
        final int resultScalar = addint(input);
        final int[] result = new int[4];
        for (int i = 0; i < 4; ++i)
            result[i] = resultScalar / (4 - i);
        return result;
    }

    private boolean patternDuplicateAnonymousResult(RenderScript RS, ScriptC_reduce s) {
        // Ensure that we can have two kernels with the same anonymous result type.

        boolean pass = true;

        final int inputSize = 1000;
        final int[] input = createInputArrayInt(149, Integer.MAX_VALUE / inputSize);

        final int[] javaResultDec = sillySumIntoDecArray(input);
        final int[] rsResultDec = s.reduce_sillySumIntoDecArray(input).get();
        pass &= result("patternDuplicateAnonymousResult (Dec)", new timing(inputSize),
                javaResultDec, rsResultDec);

        final int[] javaResultInc = sillySumIntoIncArray(input);
        final int[] rsResultInc = s.reduce_sillySumIntoIncArray(input).get();
        pass &= result("patternDuplicateAnonymousResult (Inc)", new timing(inputSize),
                javaResultInc, rsResultInc);

        return pass;
    }

    ///////////////////////////////////////////////////////////////////

    private float findMinAbs(float[] input) {
        float accum = input[0];
        for (int idx = 1; idx < input.length; ++idx) {
            final float val = input[idx];
            if (Math.abs(val) < Math.abs(accum))
                accum = val;
        }
        return accum;
    }

    static interface ReduceFindMinAbs {
        float run(Allocation input);
    }

    private boolean findMinAbs(RenderScript RS, float[] inputArray, String testName, ReduceFindMinAbs reduction) {
        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final float javaResult = findMinAbs(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocation = Allocation.createSized(RS, Element.F32(RS), inputArray.length);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copyFrom(inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final float rsResult = reduction.run(inputAllocation);
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        // Note that the Java and RenderScript algorithms are not
        // guaranteed to find the same results -- but the results
        // should have the same absolute value.

        final boolean success =
                result(testName,
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        Math.abs(javaResult), Math.abs(rsResult));
        inputAllocation.destroy();
        return success;
    }

    private boolean findMinAbsBool(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        return findMinAbs(RS, createInputArrayFloat(size[0], seed), "findMinAbsBool",
                (Allocation input) -> s.reduce_findMinAbsBool(input).get());
    }

    private boolean findMinAbsBoolInf(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        return findMinAbs(RS, createInputArrayFloatWithInfs(size[0], 1 + size[0] / 1000, seed), "findMinAbsBoolInf",
                (Allocation input) -> s.reduce_findMinAbsBool(input).get());
    }

    private boolean findMinAbsNaN(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        return findMinAbs(RS, createInputArrayFloat(size[0], seed), "findMinAbsNaN",
                (Allocation input) -> s.reduce_findMinAbsNaN(input).get());
    }

    private boolean findMinAbsNaNInf(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        return findMinAbs(RS, createInputArrayFloatWithInfs(size[0], 1 + size[0] / 1000, seed), "findMinAbsNaNInf",
                (Allocation input) -> s.reduce_findMinAbsNaN(input).get());
    }

    ///////////////////////////////////////////////////////////////////

    private Int2 findMinAndMax(float[] input) {
        float minVal = Float.POSITIVE_INFINITY;
        int minIdx = -1;
        float maxVal = Float.NEGATIVE_INFINITY;
        int maxIdx = -1;

        for (int idx = 0; idx < input.length; ++idx) {
            if ((minIdx < 0) || (input[idx] < minVal)) {
                minVal = input[idx];
                minIdx = idx;
            }
            if ((maxIdx < 0) || (input[idx] > maxVal)) {
                maxVal = input[idx];
                maxIdx = idx;
            }
        }

        return new Int2(minIdx, maxIdx);
    }

    private boolean findMinAndMax_array(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final float[] input = createInputArrayFloat(size[0], seed);

        final Int2 javaResult = findMinAndMax(input);
        final Int2 rsResult = s.reduce_findMinAndMax(input).get();

        // Note that the Java and RenderScript algorithms are not
        // guaranteed to find the same cells -- but they should
        // find cells of the same value.
        final Float2 javaVal = new Float2(input[javaResult.x], input[javaResult.y]);
        final Float2 rsVal = new Float2(input[rsResult.x], input[rsResult.y]);

        return result("findMinAndMax_array", new timing(size[0]), javaVal, rsVal);
    }

    private boolean findMinAndMax(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final float[] inputArray = createInputArrayFloat(size[0], seed);

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final Int2 javaResult = findMinAndMax(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocation = Allocation.createSized(RS, Element.F32(RS), inputArray.length);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copyFrom(inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final Int2 rsResult = s.reduce_findMinAndMax(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        // Note that the Java and RenderScript algorithms are not
        // guaranteed to find the same cells -- but they should
        // find cells of the same value.
        final Float2 javaVal = new Float2(inputArray[javaResult.x], inputArray[javaResult.y]);
        final Float2 rsVal = new Float2(inputArray[rsResult.x], inputArray[rsResult.y]);

        final boolean success =
                result("findMinAndMax",
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        javaVal, rsVal);
        inputAllocation.destroy();
        return success;
    }

    //-----------------------------------------------------------------

    private boolean patternFindMinAndMaxInf(RenderScript RS, ScriptC_reduce s) {
        // Run this kernel on an input consisting solely of a single infinity.

        final float[] input = new float[1];
        input[0] = Float.POSITIVE_INFINITY;

        final Int2 javaResult = findMinAndMax(input);
        final Int2 rsResult = s.reduce_findMinAndMax(input).get();

        // Note that the Java and RenderScript algorithms are not
        // guaranteed to find the same cells -- but they should
        // find cells of the same value.
        final Float2 javaVal = new Float2(input[javaResult.x], input[javaResult.y]);
        final Float2 rsVal = new Float2(input[rsResult.x], input[rsResult.y]);

        return result("patternFindMinAndMaxInf", new timing(1), javaVal, rsVal);
    }

    ///////////////////////////////////////////////////////////////////

    // Both the input and the result are linearized representations of matSize*matSize matrices.
    private float[] findMinMat(final float[] inputArray, final int matSize) {
        final int matSizeSquared = matSize * matSize;

        float[] result = new float[matSizeSquared];
        for (int i = 0; i < matSizeSquared; ++i)
            result[i] = Float.POSITIVE_INFINITY;

        for (int i = 0; i < inputArray.length; ++i)
            result[i % matSizeSquared] = Math.min(result[i % matSizeSquared], inputArray[i]);

        return result;
    }

    static interface ReduceFindMinMat {
        float[] run(Allocation input);
    }

    private boolean findMinMat(RenderScript RS, int seed, int[] inputSize,
                               int matSize, Element matElement, ReduceFindMinMat reduction) {
        final int length = inputSize[0];
        final int matSizeSquared = matSize * matSize;

        final float[] inputArray = createInputArrayFloat(matSizeSquared * length, seed);

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final float[] javaResult = findMinMat(inputArray, matSize);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocation = Allocation.createSized(RS, matElement, length);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copyFromUnchecked(inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final float[] rsResult = reduction.run(inputAllocation);
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final boolean success =
                result("findMinMat" + matSize,
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        javaResult, rsResult);
        inputAllocation.destroy();
        return success;
    }

    private boolean findMinMat2(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        return findMinMat(RS, seed, size, 2, Element.MATRIX_2X2(RS),
                (Allocation input) -> s.reduce_findMinMat2(input).get());
    }

    private boolean findMinMat4(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        return findMinMat(RS, seed, size, 4, Element.MATRIX_4X4(RS),
                (Allocation input) -> s.reduce_findMinMat4(input).get());
    }

    ///////////////////////////////////////////////////////////////////

    private int fz(final int[] input) {
        for (int i = 0; i < input.length; ++i)
            if (input[i] == 0)
                return i;
        return -1;
    }

    private boolean fz_array(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final int inputLen = size[0];
        int[] input = createInputArrayInt(inputLen, seed + 0);
        // just in case we got unlucky
        input[(new Random(seed + 1)).nextInt(inputLen)] = 0;

        final int rsResult = s.reduce_fz(input).get();

        final boolean success = (input[rsResult] == 0);
        Log.i(TAG,
                "fz_array: input[" + rsResult + "] == " + input[rsResult] + ": " +
                        (success ? "PASSED " + timing.string(size[0]) : "FAILED"));
        return success;
    }

    private boolean fz(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final int inputLen = size[0];
        int[] inputArray = createInputArrayInt(inputLen, seed + 0);
        // just in case we got unlucky
        inputArray[(new Random(seed + 1)).nextInt(inputLen)] = 0;

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final int javaResult = fz(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocation = Allocation.createSized(RS, Element.I32(RS), inputArray.length);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copyFrom(inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final int rsResult = s.reduce_fz(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final boolean success = (inputArray[rsResult] == 0);
        String status = (success ? "PASSED" : "FAILED");
        if (success)
            status += " " + timing.string(javaTimeStart, javaTimeEnd, rsTimeStart,
                    copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation);
        Log.i(TAG,
                "fz: java input[" + javaResult + "] == " + inputArray[javaResult] +
                        ", rs input[" + rsResult + "] == " + inputArray[javaResult] + ": " + status);
        inputAllocation.destroy();
        return success;
    }

    ///////////////////////////////////////////////////////////////////

    private boolean fz2(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final int dimX = size[0], dimY = size[1];
        final int inputLen = dimX * dimY;

        int[] inputArray = createInputArrayInt(inputLen, seed + 0);
        // just in case we got unlucky
        inputArray[(new Random(seed + 1)).nextInt(inputLen)] = 0;

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final int javaResultLinear = fz(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final Int2 javaResult = new Int2(javaResultLinear % dimX, javaResultLinear / dimX);
        final int javaCellVal = inputArray[javaResult.x + dimX * javaResult.y];

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Type.Builder typeBuilder = new Type.Builder(RS, Element.I32(RS));
        typeBuilder.setX(dimX).setY(dimY);
        Allocation inputAllocation = Allocation.createTyped(RS, typeBuilder.create());

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copy2DRangeFrom(0, 0, dimX, dimY, inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final Int2 rsResult = s.reduce_fz2(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final int rsCellVal = inputArray[rsResult.x + dimX * rsResult.y];
        final boolean success = (rsCellVal == 0);
        String status = (success ? "PASSED" : "FAILED");
        if (success)
            status += " " + timing.string(javaTimeStart, javaTimeEnd, rsTimeStart,
                    copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation);
        Log.i(TAG,
                "fz2: java input[" + javaResult.x + ", " + javaResult.y + "] == " + javaCellVal +
                        ", rs input[" + rsResult.x + ", " + rsResult.y + "] == " + rsCellVal + ": " + status);
        inputAllocation.destroy();
        return success;
    }

    ///////////////////////////////////////////////////////////////////

    private boolean fz3(RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        final int dimX = size[0], dimY = size[1], dimZ = size[2];
        final int inputLen = dimX * dimY * dimZ;

        int[] inputArray = createInputArrayInt(inputLen, seed + 0);
        // just in case we got unlucky
        inputArray[(new Random(seed + 1)).nextInt(inputLen)] = 0;

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final int javaResultLinear = fz(inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final Int3 javaResult = new Int3(
                javaResultLinear % dimX,
                (javaResultLinear / dimX) % dimY,
                javaResultLinear / (dimX * dimY));
        final int javaCellVal = inputArray[javaResult.x + dimX * javaResult.y + dimX * dimY * javaResult.z];

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Type.Builder typeBuilder = new Type.Builder(RS, Element.I32(RS));
        typeBuilder.setX(dimX).setY(dimY).setZ(dimZ);
        Allocation inputAllocation = Allocation.createTyped(RS, typeBuilder.create());

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copy3DRangeFrom(0, 0, 0, dimX, dimY, dimZ, inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final Int3 rsResult = s.reduce_fz3(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final int rsCellVal = inputArray[rsResult.x + dimX * rsResult.y + dimX * dimY * rsResult.z];
        final boolean success = (rsCellVal == 0);
        String status = (success ? "PASSED" : "FAILED");
        if (success)
            status += " " + timing.string(javaTimeStart, javaTimeEnd, rsTimeStart,
                    copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation);
        Log.i(TAG,
                "fz3: java input[" + javaResult.x + ", " + javaResult.y + ", " + javaResult.z + "] == " + javaCellVal +
                        ", rs input[" + rsResult.x + ", " + rsResult.y + ", " + rsResult.z + "] == " + rsCellVal + ": " + status);
        inputAllocation.destroy();
        return success;
    }

    ///////////////////////////////////////////////////////////////////

    private static final int histogramBucketCount = 256;

    private long[] histogram(RenderScript RS, final byte[] inputArray) {
        Allocation inputAllocation = Allocation.createSized(RS, Element.U8(RS), inputArray.length);
        inputAllocation.copyFrom(inputArray);

        Allocation outputAllocation = Allocation.createSized(RS, Element.U32(RS), histogramBucketCount);

        ScriptIntrinsicHistogram scriptHsg = ScriptIntrinsicHistogram.create(RS, Element.U8(RS));
        scriptHsg.setOutput(outputAllocation);
        scriptHsg.forEach(inputAllocation);

        int[] outputArrayMistyped = new int[histogramBucketCount];
        outputAllocation.copyTo(outputArrayMistyped);

        long[] outputArray = new long[histogramBucketCount];
        for (int i = 0; i < histogramBucketCount; ++i)
            outputArray[i] = outputArrayMistyped[i] & (long) 0xffffffff;

        inputAllocation.destroy();
        outputAllocation.destroy();

        scriptHsg.destroy();
        return outputArray;
    }

    private boolean histogram_array(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final byte[] inputArray = createInputArrayByte(size[0], seed);

        final long[] javaResult = histogram(RS, inputArray);
        assertEquals("javaResult length", histogramBucketCount, javaResult.length);
        final long[] rsResult = s.reduce_histogram(inputArray).get();
        assertEquals("rsResult length", histogramBucketCount, rsResult.length);

        return result("histogram_array", new timing(size[0]), javaResult, rsResult);
    }

    private boolean histogram(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final byte[] inputArray = createInputArrayByte(size[0], seed);

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final long[] javaResult = histogram(RS, inputArray);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();
        assertEquals("javaResult length", histogramBucketCount, javaResult.length);

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocation = Allocation.createSized(RS, Element.U8(RS), inputArray.length);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocation.copyFrom(inputArray);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final long[] rsResult = s.reduce_histogram(inputAllocation).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();
        assertEquals("rsResult length", histogramBucketCount, rsResult.length);

        // NOTE: The "java time" is actually for the RenderScript histogram intrinsic
        final boolean success =
                result("histogram",
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart,
                                copyTimeStart, kernelTimeStart, rsTimeEnd, inputAllocation),
                        javaResult, rsResult);
        inputAllocation.destroy();
        return success;
    }

    //-----------------------------------------------------------------

    private boolean patternRedundantGet(RenderScript RS, ScriptC_reduce s) {
        // Ensure that get() can be called multiple times on the same
        // result, and returns the same object each time.

        boolean pass = true;

        final int inputLength = 1 << 18;
        final byte[] inputArray = createInputArrayByte(inputLength, 789);

        final long[] javaResult = histogram(RS, inputArray);
        assertEquals("javaResult length", histogramBucketCount, javaResult.length);

        final ScriptC_reduce.resultArray256_uint rsResultFuture = s.reduce_histogram(inputArray);
        final long[] rsResult1 = rsResultFuture.get();
        assertEquals("rsResult1 length", histogramBucketCount, rsResult1.length);
        pass &= result("patternRedundantGet (1)", new timing(inputLength), javaResult, rsResult1);

        final long[] rsResult2 = rsResultFuture.get();
        pass &= result("patternRedundantGet (2)", new timing(inputLength), javaResult, rsResult2);

        final boolean success = (rsResult1 == rsResult2);
        Log.i(TAG, "patternRedundantGet (object equality): " + (success ? "PASSED" : "FAILED"));
        pass &= success;

        return pass;
    }

    //-----------------------------------------------------------------

    private Int2 mode(RenderScript RS, final byte[] inputArray) {
        long[] hsg = histogram(RS, inputArray);

        int modeIdx = 0;
        for (int i = 1; i < hsg.length; ++i)
            if (hsg[i] > hsg[modeIdx]) modeIdx = i;
        return new Int2(modeIdx, (int) hsg[modeIdx]);
    }

    private boolean mode_array(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final byte[] inputArray = createInputArrayByte(size[0], seed);

        final Int2 javaResult = mode(RS, inputArray);
        final Int2 rsResult = s.reduce_mode(inputArray).get();

        return result("mode", new timing(size[0]), javaResult, rsResult);
    }

    ///////////////////////////////////////////////////////////////////

    private long sumgcd(final int in1[], final int in2[]) {
        assertEquals("sumgcd input lengths", in1.length, in2.length);

        long sum = 0;
        for (int i = 0; i < in1.length; ++i) {
            int a = in1[i], b = in2[i];

            while (b != 0) {
                final int aNew = b;
                final int bNew = a % b;

                a = aNew;
                b = bNew;
            }

            sum += a;
        }
        return sum;
    }

    private boolean sumgcd(RenderScript RS, ScriptC_reduce s, int seed, int size[]) {
        final int len = size[0];

        final int[] inputArrayA = createInputArrayInt(len, seed + 0);
        final int[] inputArrayB = createInputArrayInt(len, seed + 1);

        final long javaTimeStart = java.lang.System.currentTimeMillis();
        final long javaResult = sumgcd(inputArrayA, inputArrayB);
        final long javaTimeEnd = java.lang.System.currentTimeMillis();

        final long rsTimeStart = java.lang.System.currentTimeMillis();

        Allocation inputAllocationA = Allocation.createSized(RS, Element.I32(RS), len);
        Allocation inputAllocationB = Allocation.createSized(RS, Element.I32(RS), len);

        final long copyTimeStart = java.lang.System.currentTimeMillis();

        inputAllocationA.copyFrom(inputArrayA);
        inputAllocationB.copyFrom(inputArrayB);

        final long kernelTimeStart = java.lang.System.currentTimeMillis();
        final long rsResult = s.reduce_sumgcd(inputAllocationA, inputAllocationB).get();
        final long rsTimeEnd = java.lang.System.currentTimeMillis();

        final boolean success =
                result("sumgcd",
                        new timing(javaTimeStart, javaTimeEnd, rsTimeStart, copyTimeStart, kernelTimeStart, rsTimeEnd,
                                inputAllocationA, inputAllocationB),
                        javaResult, rsResult);
        inputAllocationA.destroy();
        inputAllocationB.destroy();
        return success;
    }

    ///////////////////////////////////////////////////////////////////

    // Return an array of sparse integer values from 0 to maxVal inclusive.
    // The array consists of all values k*sparseness (k a nonnegative integer)
    // that are less than maxVal, and maxVal itself.  For example, if maxVal
    // is 20 and sparseness is 6, then the result is { 0, 6, 12, 18, 20 };
    // and if maxVal is 20 and sparseness is 10, then the result is { 0, 10, 20 }.
    //
    // The elements of the array are sorted in increasing order.
    //
    // maxVal     -- must be nonnegative
    // sparseness -- must be positive
    private static int[] computeSizePoints(int maxVal, int sparseness) {
        assertTrue((maxVal >= 0) && (sparseness > 0));

        final boolean maxValIsExtra = ((maxVal % sparseness) != 0);
        int[] result = new int[1 + maxVal / sparseness + (maxValIsExtra ? 1 : 0)];

        for (int i = 0; i * sparseness <= maxVal; ++i)
            result[i] = i * sparseness;
        if (maxValIsExtra)
            result[result.length - 1] = maxVal;

        return result;
    }

    private static final int maxSeedsPerTest = 10;

    static interface Test {
        // A test execution is characterized by two properties: A seed
        // and a size.
        //
        // The seed is used for generating pseudorandom input data.
        // Ideally, we use different seeds for different tests and for
        // different executions of the same test at different sizes.
        // A test with multiple blocks of input data (i.e., for a
        // reduction with multiple inputs) may want multiple seeds; it
        // may use the seeds seed..seed+maxSeedsPerTest-1.
        //
        // The size indicates the amount of input data.  It is the number
        // of cells in a particular dimension of the iteration space.
        boolean run(RenderScript RS, ScriptC_reduce s, int seed, int[] size);
    }

    static class TestDescription {
        public TestDescription(String myTestName, Test myTest, int mySeed, int[] myDefSize,
                               int myLog2MaxSize, int mySparseness) {
            testName = myTestName;
            test = myTest;
            seed = mySeed;
            defSize = myDefSize;
            log2MaxSize = myLog2MaxSize;
            sparseness = mySparseness;
        }

        public TestDescription(String myTestName, Test myTest, int mySeed, int[] myDefSize, int myLog2MaxSize) {
            testName = myTestName;
            test = myTest;
            seed = mySeed;
            defSize = myDefSize;
            log2MaxSize = myLog2MaxSize;
            sparseness = 1;
        }

        public TestDescription(String myTestName, Test myTest, int mySeed, int[] myDefSize) {
            testName = myTestName;
            test = myTest;
            seed = mySeed;
            defSize = myDefSize;
            log2MaxSize = -1;
            sparseness = 1;
        }

        public final String testName;

        public final Test test;

        // When executing the test, scale this up by maxSeedsPerTest.
        public final int seed;

        // If we're only going to run the test once, what size should
        // we use?  The length of the array is the number of
        // dimensions of the input data.
        public final int[] defSize;

        // If we're going to run the test over a range of sizes, what
        // is the maximum size to use?  (This constrains the number of
        // cells of the input data, not the number of cells ALONG A
        // PARTICULAR DIMENSION of the input data.)
        public final int log2MaxSize;

        // If we're going to run the test "exhaustively" over a range
        // of sizes, what is the size of a step through the range?
        //
        // For 1D, must be 1.
        public final int sparseness;
    }

    private boolean run(TestDescription td, RenderScript RS, ScriptC_reduce s, int seed, int[] size) {
        String arrayContent = "";
        for (int i = 0; i < size.length; ++i) {
            if (i != 0)
                arrayContent += ", ";
            arrayContent += size[i];
        }
        Log.i(TAG, "Running " + td.testName + "(seed = " + seed + ", size[] = {" + arrayContent + "})");
        return td.test.run(RS, s, seed, size);
    }

    private final TestDescription[] correctnessTests = {
            // alloc and array variants of the same test will use the same
            // seed, in case results need to be compared.

            new TestDescription("addint1D", this::addint1D, 0, new int[]{100000}, 20),
            new TestDescription("addint1D_array", this::addint1D_array, 0, new int[]{100000}, 20),
            new TestDescription("addint2D", this::addint2D, 1, new int[]{450, 225}, 20, 5),
            new TestDescription("addint3D", this::addint3D, 2, new int[]{37, 48, 49}, 20, 7),

            // Bool and NaN variants of the same test will use the same
            // seed, in case results need to be compared.
            new TestDescription("findMinAbsBool", this::findMinAbsBool, 3, new int[]{100000}, 20),
            new TestDescription("findMinAbsNaN", this::findMinAbsNaN, 3, new int[]{100000}, 20),
            new TestDescription("findMinAbsBoolInf", this::findMinAbsBoolInf, 4, new int[]{100000}, 20),
            new TestDescription("findMinAbsNaNInf", this::findMinAbsNaNInf, 4, new int[]{100000}, 20),

            new TestDescription("findMinAndMax", this::findMinAndMax, 5, new int[]{100000}, 20),
            new TestDescription("findMinAndMax_array", this::findMinAndMax_array, 5, new int[]{100000}, 20),
            new TestDescription("findMinMat2", this::findMinMat2, 6, new int[]{25000}, 17),
            new TestDescription("findMinMat4", this::findMinMat4, 7, new int[]{10000}, 15),
            new TestDescription("fz", this::fz, 8, new int[]{100000}, 20),
            new TestDescription("fz_array", this::fz_array, 8, new int[]{100000}, 20),
            new TestDescription("fz2", this::fz2, 9, new int[]{225, 450}, 20, 5),
            new TestDescription("fz3", this::fz3, 10, new int[]{59, 48, 37}, 20, 7),
            new TestDescription("histogram", this::histogram, 11, new int[]{100000}, 20),
            new TestDescription("histogram_array", this::histogram_array, 11, new int[]{100000}, 20),
            // might want to add: new TestDescription("mode", this::mode, 12, new int[]{100000}, 20),
            new TestDescription("mode_array", this::mode_array, 12, new int[]{100000}, 20),
            new TestDescription("sumgcd", this::sumgcd, 13, new int[]{1 << 16}, 20)
    };

    private boolean runCorrectnessQuick(RenderScript RS, ScriptC_reduce s) {
        boolean pass = true;

        for (TestDescription td : correctnessTests) {
            pass &= run(td, RS, s, maxSeedsPerTest * td.seed, td.defSize);
        }

        return pass;
    }

    // NOTE: Each test execution gets maxSeedsPerTest, and there are
    // up to 3 + 5*log2MaxSize test executions in the full (as opposed
    // to quick) correctness run of a particular test description, and
    // we need an additional seed for pseudorandom size generation.
    // Assuming log2MaxSize does not exceed 32, then it should be
    // sufficient to reserve 1 + (3+5*32)*maxSeedsPerTest seeds per
    // TestDescription.
    //
    // See runCorrectness1D().
    private static final int seedsPerTestDescriptionCorrectness1D = 1 + (3 + 5 * 32) * maxSeedsPerTest;

    // NOTE: Each test execution gets maxSeedsPerTest, and there are
    // about 11*((log2MaxSize+1)**2) test executions in the full (as
    // opposed to quick) correctness run of a particular test
    // description, and we need a seed for pseudorandom size
    // generation.  Assuming log2MaxSize does not exceed 32, then it
    // should be sufficient to reserve 1 + 11*1089*maxSeedsPerTest
    // seeds per TestDescription.
    //
    // See runCorrectness2D().
    private static final int seedsPerTestDescriptionCorrectness2D = 1 + (11 * 1089) * maxSeedsPerTest;

    // NOTE: Each test execution gets maxSeedsPerTest, and there are
    // about 27*((log2MaxSize+1)**3) + 6*((log2MaxSize+1)**2) test
    // executions in the full (as opposed to quick) correctness run of
    // a particular test description, and we need a seed for (c).
    // Assuming log2MaxSize does not exceed 32, then it should
    // be sufficient to reserve 1 + (27*(33**3) + 6*(33**2))*maxSeedsPerTest
    // seeds per TestDescription, which can be simplified upwards to
    // 1 + (28*(33**3))*maxSeedsPerTest seeds per TestDescription.
    private static final int seedsPerTestDescriptionCorrectness3D = 1 + (28 * 35937) * maxSeedsPerTest;

    // Each test execution gets a certain number of seeds, and a full
    // (as opposed to quick) correctness run of a particular
    // TestDescription consists of some number of executions (each of
    // which needs up to maxSeedsPerTest) and may require some
    // additional seeds.
    private static final int seedsPerTestDescriptionCorrectness =
            Math.max(seedsPerTestDescriptionCorrectness1D,
                    Math.max(seedsPerTestDescriptionCorrectness2D,
                            seedsPerTestDescriptionCorrectness3D));

    private boolean runCorrectness(RenderScript RS, ScriptC_reduce s) {
        boolean pass = true;

        for (TestDescription td : correctnessTests) {
            switch (td.defSize.length) {
                case 1:
                    pass &= runCorrectness1D(td, RS, s);
                    break;
                case 2:
                    pass &= runCorrectness2D(td, RS, s);
                    break;
                case 3:
                    pass &= runCorrectness3D(td, RS, s);
                    break;
                default:
                    assertTrue("unexpected defSize.length " + td.defSize.length, false);
                    pass &= false;
                    break;
            }
        }

        return pass;
    }

    private boolean runCorrectness1D(TestDescription td, RenderScript RS, ScriptC_reduce s) {
        assertEquals(1, td.sparseness);
        final int log2MaxSize = td.log2MaxSize;
        assertTrue(log2MaxSize >= 0);

        boolean pass = true;

        // We will execute the test with the following sizes:
        // (a) Each power of 2 from zero (2**0) up to log2MaxSize (2**log2MaxSize)
        // (b) Each size from (a) +/-1
        // (c) 2 random sizes between each pair of adjacent points in (a)
        int[] testSizes = new int[
            /* a */ (1 + log2MaxSize) +
            /* b */ 2 * (1 + log2MaxSize) +
            /* c */ 2 * log2MaxSize];
        // See seedsPerTestDescriptionCorrectness1D

        final int seedForPickingTestSizes = td.seed * seedsPerTestDescriptionCorrectness;

        int nextTestIdx = 0;

        // Fill in (a) and (b)
        for (int i = 0; i <= log2MaxSize; ++i) {
            final int pwrOf2 = 1 << i;
            testSizes[nextTestIdx++] = pwrOf2;      /* a */
            testSizes[nextTestIdx++] = pwrOf2 - 1;  /* b */
            testSizes[nextTestIdx++] = pwrOf2 + 1;  /* b */
        }

        // Fill in (c)
        Random r = new Random(seedForPickingTestSizes);
        for (int i = 0; i < log2MaxSize; ++i) {
            final int lo = (1 << i) + 1;
            final int hi = 1 << (i + 1);

            if (lo < hi) {
                for (int j = 0; j < 2; ++j) {
                    testSizes[nextTestIdx++] = r.nextInt(hi - lo) + lo;
                }
            }
        }

        Arrays.sort(testSizes);

        int[] lastTestSizeArg = new int[]{-1};
        for (int i = 0; i < testSizes.length; ++i) {
            if ((testSizes[i] > 0) && (testSizes[i] != lastTestSizeArg[0])) {
                lastTestSizeArg[0] = testSizes[i];
                final int seedForTestExecution = seedForPickingTestSizes + 1 + i * maxSeedsPerTest;
                pass &= run(td, RS, s, seedForTestExecution, lastTestSizeArg);
            }
        }

        return pass;
    }

    private boolean runCorrectness2D(TestDescription td, RenderScript RS, ScriptC_reduce s) {
        final int log2MaxSize = td.log2MaxSize, maxSize = 1 << log2MaxSize, sparseness = td.sparseness;
        assertTrue((log2MaxSize >= 0) && (sparseness >= 1));

        boolean pass = true;

        final int[] sizePoints = computeSizePoints(log2MaxSize, sparseness);

        // We will execute the test with the following sizes:
        // (a) Each dimension at a power of 2 from sizePoints[]
        ///    such that the sum of the exponents does not exceed
        //     log2MaxSize
        // (b) Each size from (a) with one or both dimensions +/-1,
        //     except where this would exceed 2**log2MaxSize
        // (c) Approximately 2*(sizePoints.length**2) random sizes
        ArrayList<int[]> testSizesList = new ArrayList<int[]>();
        // See seedsPerTestDescriptionCorrectness2D

        final int seedForPickingTestSizes = td.seed * seedsPerTestDescriptionCorrectness;

        // Fill in (a) and (b)
        for (int i : sizePoints) {
            final int iPwrOf2 = 1 << i;
            for (int iDelta = -1; iDelta <= 1; ++iDelta) {
                final int iSize = iPwrOf2 + iDelta;
                for (int j : sizePoints) {
                    final int jPwrOf2 = 1 << j;
                    for (int jDelta = -1; jDelta <= 1; ++jDelta) {
                        final int jSize = jPwrOf2 + jDelta;
                        if ((long) iSize * (long) jSize <= maxSize)
                            testSizesList.add(new int[]{iSize, jSize});
                    }
                }
            }
        }

        // Fill in (c)
        Random r = new Random(seedForPickingTestSizes);
        for (int i : sizePoints) {
            for (int j : sizePoints) {
                final int size0 = 1 + r.nextInt(1 << i);
                final int size1 = 1 + r.nextInt(maxSize / size0);

                testSizesList.add(new int[]{size0, size1});
                testSizesList.add(new int[]{size1, size0});
            }
        }

        int[][] testSizes = testSizesList.toArray(new int[0][]);
        Arrays.sort(testSizes,
                (a, b) -> {
                    final int comp0 = ((Integer) a[0]).compareTo(b[0]);
                    return (comp0 != 0 ? comp0 : ((Integer) a[1]).compareTo(b[1]));
                });

        int[] lastTestSizeArg = null;
        for (int i = 0; i < testSizes.length; ++i) {
            if ((testSizes[i][0] <= 0) || (testSizes[i][1] <= 0))
                continue;
            if ((lastTestSizeArg != null) &&
                    (testSizes[i][0] == lastTestSizeArg[0]) &&
                    (testSizes[i][1] == lastTestSizeArg[1]))
                continue;
            lastTestSizeArg = testSizes[i];
            final int seedForTestExecution = seedForPickingTestSizes + 1 + i * maxSeedsPerTest;
            pass &= run(td, RS, s, seedForTestExecution, lastTestSizeArg);
        }

        return pass;
    }

    private boolean runCorrectness3D(TestDescription td, RenderScript RS, ScriptC_reduce s) {
        final int log2MaxSize = td.log2MaxSize, maxSize = 1 << log2MaxSize, sparseness = td.sparseness;
        assertTrue((log2MaxSize >= 0) && (sparseness >= 1));

        boolean pass = true;

        final int[] sizePoints = computeSizePoints(log2MaxSize, sparseness);

        // We will execute the test with the following sizes:
        // (a) Each dimension at a power of 2 from sizePoints[]
        ///    such that the sum of the exponents does not exceed
        //     log2MaxSize
        // (b) Each size from (a) with one or both dimensions +/-1,
        //     except where this would exceed 2**log2MaxSize
        // (c) Approximately 6*(sizePoints.length**2) random sizes
        ArrayList<int[]> testSizesList = new ArrayList<int[]>();
        // See seedsPerTestDescriptionCorrectness3D

        final int seedForPickingTestSizes = td.seed * seedsPerTestDescriptionCorrectness;

        // Fill in (a) and (b)
        for (int i : sizePoints) {
            final int iPwrOf2 = 1 << i;
            for (int iDelta = -1; iDelta <= 1; ++iDelta) {
                final int iSize = iPwrOf2 + iDelta;
                for (int j : sizePoints) {
                    final int jPwrOf2 = 1 << j;
                    for (int jDelta = -1; jDelta <= 1; ++jDelta) {
                        final int jSize = jPwrOf2 + jDelta;
                        for (int k : sizePoints) {
                            final int kPwrOf2 = 1 << k;
                            for (int kDelta = -1; kDelta <= 1; ++kDelta) {
                                final int kSize = kPwrOf2 + kDelta;
                                if ((long) iSize * (long) jSize * (long) kSize <= maxSize)
                                    testSizesList.add(new int[]{iSize, jSize, kSize});
                            }
                        }
                    }
                }
            }
        }

        // Fill in (c)
        Random r = new Random(seedForPickingTestSizes);
        for (int i : sizePoints) {
            for (int j : sizePoints) {
                final int size0 = 1 + r.nextInt(1 << i);
                final int size1 = 1 + r.nextInt(Math.min(1 << j, maxSize / size0));
                final int size2 = 1 + r.nextInt(maxSize / (size0 * size1));

                testSizesList.add(new int[]{size0, size1, size2});
                testSizesList.add(new int[]{size0, size2, size1});
                testSizesList.add(new int[]{size1, size0, size2});
                testSizesList.add(new int[]{size1, size2, size0});
                testSizesList.add(new int[]{size2, size0, size1});
                testSizesList.add(new int[]{size2, size1, size0});
            }
        }

        int[][] testSizes = testSizesList.toArray(new int[0][]);
        Arrays.sort(testSizes,
                (a, b) -> {
                    int comp = ((Integer) a[0]).compareTo(b[0]);
                    if (comp == 0)
                        comp = ((Integer) a[1]).compareTo(b[1]);
                    if (comp == 0)
                        comp = ((Integer) a[2]).compareTo(b[2]);
                    return comp;
                });

        int[] lastTestSizeArg = null;
        for (int i = 0; i < testSizes.length; ++i) {
            if ((testSizes[i][0] <= 0) || (testSizes[i][1] <= 0) || (testSizes[i][2] <= 0))
                continue;
            if ((lastTestSizeArg != null) &&
                    (testSizes[i][0] == lastTestSizeArg[0]) &&
                    (testSizes[i][1] == lastTestSizeArg[1]) &&
                    (testSizes[i][2] == lastTestSizeArg[2]))
                continue;

            // Apply Z-dimension limiting.
            //
            // The Z dimension is always handled specially by GPU
            // drivers, and a high value for this dimension can have
            // serious performance implications.  For example, Cuda
            // and OpenCL encourage Z to be the smallest dimension.
            if (testSizes[i][2] > 1024)
                continue;

            lastTestSizeArg = testSizes[i];
            final int seedForTestExecution = seedForPickingTestSizes + 1 + i * maxSeedsPerTest;
            pass &= run(td, RS, s, seedForTestExecution, lastTestSizeArg);
        }

        return pass;
    }

    private final TestDescription[] performanceTests = {
            new TestDescription("addint1D", this::addint1D, 0, new int[]{100000 << 10}),
            new TestDescription("addint2D", this::addint2D, 1, new int[]{450 << 5, 225 << 5}),
            new TestDescription("addint3D", this::addint3D, 2, new int[]{37 << 3, 48 << 3, 49 << 3}),
            new TestDescription("findMinAndMax", this::findMinAndMax, 3, new int[]{100000 << 9}),
            new TestDescription("fz", this::fz, 4, new int[]{100000 << 10}),
            new TestDescription("fz2", this::fz2, 5, new int[]{225 << 5, 450 << 5}),
            new TestDescription("fz3", this::fz3, 6, new int[]{59 << 3, 48 << 3, 37 << 3}),
            new TestDescription("histogram", this::histogram, 7, new int[]{100000 << 10}),
            // might want to add: new TestDescription("mode", this::mode, 8, new int[]{100000}),
            new TestDescription("sumgcd", this::sumgcd, 9, new int[]{1 << 21})
    };

    private boolean runPerformanceQuick(RenderScript RS, ScriptC_reduce s) {
        boolean pass = true;

        for (TestDescription td : performanceTests) {
            pass &= run(td, RS, s, maxSeedsPerTest * td.seed, td.defSize);
        }

        return pass;
    }

    private boolean runCorrectnessPatterns(RenderScript RS, ScriptC_reduce s) {
        // Test some very specific usage patterns.
        boolean pass = true;

        pass &= patternDuplicateAnonymousResult(RS, s);
        pass &= patternFindMinAndMaxInf(RS, s);
        pass &= patternInterleavedReduce(RS, s);
        pass &= patternRedundantGet(RS, s);

        return pass;
    }

    public void run() {
        RenderScript pRS = createRenderScript(false);
        ScriptC_reduce s = new ScriptC_reduce(pRS);
        s.set_negInf(Float.NEGATIVE_INFINITY);
        s.set_posInf(Float.POSITIVE_INFINITY);

        boolean pass = true;

        pass &= runCorrectnessPatterns(pRS, s);
        pass &= runCorrectnessQuick(pRS, s);
        pass &= runCorrectness(pRS, s);
        // pass &= runPerformanceQuick(pRS, s);

        pRS.finish();
        s.destroy();
        pRS.destroy();

        Log.i(TAG, pass ? "PASSED" : "FAILED");
        if (pass)
            passTest();
        else
            failTest();
    }
}

// TODO: Add machinery for easily running fuller (i.e., non-sparse) testing.
