/*
 * Copyright (C) 2013 The Android Open Source Project
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

package android.renderscript.cts;

import android.renderscript.*;
import android.util.Log;

public class IntrinsicHistogram extends IntrinsicBase {
    private Allocation mAin;
    private Allocation mAout;

    @Override
    protected void tearDown() throws Exception {
        if (mAin != null) {
            mAin.destroy();
        }
        if (mAout != null) {
            mAout.destroy();
        }
        super.tearDown();
    }

    private void createAllocations(int inVSize, int outVSize, int w, int h) {
        Element e1;
        Element e2;

        switch (inVSize) {
        case 4: e1 = Element.U8_4(mRS); break;
        case 3: e1 = Element.U8_3(mRS); break;
        case 2: e1 = Element.U8_2(mRS); break;
        default: e1 = Element.U8(mRS); break;
        }

        switch (outVSize) {
        case 4: e2 = Element.I32_4(mRS); break;
        case 3: e2 = Element.I32_3(mRS); break;
        case 2: e2 = Element.I32_2(mRS); break;
        default: e2 = Element.I32(mRS); break;
        }

        Type.Builder tb = new Type.Builder(mRS, e1);
        tb.setX(w).setY(h);

        mAin = Allocation.createTyped(mRS, tb.create());
        mAout = Allocation.createSized(mRS, e2, 256);
    }

    private void testNorm(int inVSize, int outVSize, int w, int h, boolean clip) {
        createAllocations(inVSize, outVSize, w, h);

        int invs2 = (inVSize != 3) ? inVSize : 4;
        int outvs2 = (outVSize != 3) ? outVSize : 4;

        byte i[] = new byte[w * h * invs2];
        int res[] = new int[256 * outvs2];
        int ref[] = new int[256 * outvs2];

        java.util.Random r = new java.util.Random();
        r.nextBytes(i);

        int x1 = 0, y1 = 0, x2 = w, y2 = h;
        if (clip) {
            x1 = 11;
            y1 = 11;
            x2 = w - 11;
            y2 = h - 11;
        }

        for (int y = y1; y < y2; y++) {
            for (int x = x1; x < x2; x++) {
                int ct = (y * w + x) * invs2;

                int v = 0;
                for (int c = 0; c < inVSize; c++) {
                    int t = i[ct + c];
                    if (t < 0) t = 256 + t;

                    if (c < outVSize) {
                        ref[(t * outvs2) + c] ++;
                    }
                }
            }
        }

        mAin.copyFrom(i);
        ScriptIntrinsicHistogram hist =
                ScriptIntrinsicHistogram.create(mRS, mAin.getType().getElement());
        hist.setOutput(mAout);
        hist.forEach(mAin, makeClipper(x1, y1, x2, y2));

        mAin.copyFrom(i);
        mAout.copyTo(res);
        for (int ct=0; ct < res.length; ct++) {
            assertTrue(res[ct] == ref[ct]);
        }

        hist.destroy();
    }

    public void test_norm_4_4() {
        testNorm(4, 4, 101, 101, false);
    }
    public void test_norm_4_3() {
        testNorm(4, 3, 101, 101, false);
    }
    public void test_norm_4_2() {
        testNorm(4, 2, 101, 101, false);
    }
    public void test_norm_4_1() {
        testNorm(4, 1, 101, 101, false);
    }

    public void test_norm_3_3() {
        testNorm(3, 3, 101, 101, false);
    }
    public void test_norm_3_2() {
        testNorm(3, 2, 101, 101, false);
    }
    public void test_norm_3_1() {
        testNorm(3, 1, 101, 101, false);
    }

    public void test_norm_2_2() {
        testNorm(2, 2, 101, 101, false);
    }
    public void test_norm_2_1() {
        testNorm(2, 1, 101, 101, false);
    }

    public void test_norm_1_1() {
        testNorm(1, 1, 101, 101, false);
    }


    public void test_norm_4_4C() {
        testNorm(4, 4, 101, 101, true);
    }
    public void test_norm_4_3C() {
        testNorm(4, 3, 101, 101, true);
    }
    public void test_norm_4_2C() {
        testNorm(4, 2, 101, 101, true);
    }
    public void test_norm_4_1C() {
        testNorm(4, 1, 101, 101, true);
    }

    public void test_norm_3_3C() {
        testNorm(3, 3, 101, 101, true);
    }
    public void test_norm_3_2C() {
        testNorm(3, 2, 101, 101, true);
    }
    public void test_norm_3_1C() {
        testNorm(3, 1, 101, 101, true);
    }

    public void test_norm_2_2C() {
        testNorm(2, 2, 101, 101, true);
    }
    public void test_norm_2_1C() {
        testNorm(2, 1, 101, 101, true);
    }

    public void test_norm_1_1C() {
        testNorm(1, 1, 101, 101, true);
    }



    private void testDot(int inVSize, int w, int h, boolean clip) {
        createAllocations(inVSize, 1, w, h);

        int invs2 = (inVSize != 3) ? inVSize : 4;

        byte i[] = new byte[w * h * invs2];
        int res[] = new int[256];
        int ref[] = new int[256];

        java.util.Random r = new java.util.Random();
        r.nextBytes(i);

        float dotVals[] = {0.2501f, 0.1251f, 0.06251f, 0.1251f};
        int doti[] = new int[4];
        for (int ct=0; ct < 4; ct++) {
            doti[ct] = (int)((dotVals[ct] * 256.f) + 0.5f);
        }

        int x1 = 0, y1 = 0, x2 = w, y2 = h;
        if (clip) {
            x1 = 11;
            y1 = 11;
            x2 = w - 11;
            y2 = h - 11;
        }

        for (int y = y1; y < y2; y++) {
            for (int x = x1; x < x2; x++) {
                int ct = (y * w + x) * invs2;

                int v = 0;
                for (int c = 0; c < inVSize; c++) {
                    int t = i[ct + c];
                    if (t < 0) t = 256 + t;
                    v += doti[c] * t;
                }
                ref[(v + 0x7f) >> 8] ++;
            }
        }

        mAin.copyFrom(i);
        ScriptIntrinsicHistogram hist =
                ScriptIntrinsicHistogram.create(mRS, mAin.getType().getElement());
        hist.setOutput(mAout);
        hist.setDotCoefficients(dotVals[0], dotVals[1], dotVals[2], dotVals[3]);
        hist.forEach_Dot(mAin, makeClipper(x1, y1, x2, y2));

        mAin.copyFrom(i);
        mAout.copyTo(res);
        for (int ct=0; ct < res.length; ct++) {
            assertTrue(res[ct] == ref[ct]);
        }

        hist.destroy();
    }

    public void test_dot_1() {
        testDot(1, 101, 101, false);
    }
    public void test_dot_2() {
        testDot(2, 101, 101, false);
    }
    public void test_dot_3() {
        testDot(3, 101, 101, false);
    }
    public void test_dot_4() {
        testDot(4, 101, 101, false);
    }


    public void test_dot_1C() {
        testDot(1, 101, 101, true);
    }
    public void test_dot_2C() {
        testDot(2, 101, 101, true);
    }
    public void test_dot_3C() {
        testDot(3, 101, 101, true);
    }
    public void test_dot_4C() {
        testDot(4, 101, 101, true);
    }

}
