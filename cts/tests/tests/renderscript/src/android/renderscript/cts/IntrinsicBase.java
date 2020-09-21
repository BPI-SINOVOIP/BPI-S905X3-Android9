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

import android.util.Log;
import android.renderscript.RenderScript;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.Type;
import android.renderscript.Script;

public class IntrinsicBase extends RSBaseCompute {
    protected final String TAG = "Img";

    protected Allocation mAllocSrc;
    protected Allocation mAllocRef;
    protected Allocation mAllocDst;
    protected ScriptC_verify mVerify;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mVerify = new ScriptC_verify(mRS);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mVerify != null) {
            mVerify.destroy();
        }

        if (mAllocSrc != null) {
            mAllocSrc.destroy();
        }

        if (mAllocRef != null) {
            mAllocRef.destroy();
        }

        if (mAllocDst != null) {
            mAllocDst.destroy();
        }

        super.tearDown();
    }

    protected Element makeElement(Element.DataType dt, int vecSize) {
        Element e;
        if (vecSize > 1) {
            e = Element.createVector(mRS, dt, vecSize);
        } else {
            if (dt == Element.DataType.UNSIGNED_8) {
                e = Element.U8(mRS);
            } else {
                e = Element.F32(mRS);
            }
        }
        return e;
    }

    protected Allocation makeAllocation(int w, int h, Element e, boolean clear) {
        Type.Builder tb = new Type.Builder(mRS, e);
        tb.setX(w);
        tb.setY(h);
        Type t = tb.create();
        Allocation a = Allocation.createTyped(mRS, t);

        if (clear) {
            final int s = a.getBytesSize();
            byte[] b = new byte[s];
            a.copyFromUnchecked(b);
        }

        return a;
    }

    protected Allocation makeAllocation(int w, int h, Element e) {
        return makeAllocation(w, h, e, true);
    }

    protected void makeSource(int w, int h, Element e) {
        mAllocSrc = makeAllocation(w, h, e);

        java.util.Random r = new java.util.Random(100);

        int vs = e.getVectorSize();
        if (vs == 3) vs = 4;
        if (e.getDataType() == Element.DataType.FLOAT_32) {
            float f[] = new float[w * h * vs];
            for (int y=0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    for (int v = 0; v < vs; v++) {
                        f[(y * w + x) * vs + v] = r.nextFloat();
                    }
                }
            }
            mAllocSrc.copyFromUnchecked(f);
        }

        if (e.getDataType() == Element.DataType.UNSIGNED_8) {
            byte f[] = new byte[w * h * vs];
            for (int y=0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    for (int v = 0; v < vs; v++) {
                        f[(y * w + x) * vs + v] = (byte)r.nextInt(256);
                    }
                }
            }
            mAllocSrc.copyFromUnchecked(f);
        }
    }

    protected void makeBuffers(int w, int h, Element e) {
        makeSource(w, h, e);

        mAllocRef = makeAllocation(w, h, e);
        mAllocDst = makeAllocation(w, h, e);
    }


    protected void checkError() {
        mRS.finish();
        mVerify.invoke_checkError();
        waitForMessage();
        checkForErrors();
    }

    protected Script.LaunchOptions makeClipper(int x, int y, int x2, int y2) {
        Script.LaunchOptions lo = new Script.LaunchOptions();
        lo.setX(x, x2);
        lo.setY(y, y2);
        return lo;
    }

}
