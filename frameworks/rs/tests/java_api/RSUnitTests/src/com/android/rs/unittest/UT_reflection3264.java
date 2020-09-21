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

package com.android.rs.unittest;

import android.content.Context;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;
import android.renderscript.Short4;
import android.renderscript.Type;
import android.util.Log;
import java.util.Random;

import static junit.framework.Assert.assertEquals;

public class UT_reflection3264 extends UnitTest {
  private static final String TAG = "reflection3264";

  public UT_reflection3264(Context ctx) {
    super("reflection3264", ctx);
  }

  private final int xSize = 17, ySize = 23;  // arbitrary values

  private final long seed = 20170609;  // arbitrary value

  private static long unsigned(int v) {
    if (v >= 0)
      return (long)v;
    else
      return (long)v + ((long)1 << 32);
  }

  private static short unsigned(byte v) {
    if (v >= 0)
      return (short)v;
    else
      return (short)((short)v + (short)(1 << 8));
  }

  public void run() {
    Random r = new Random(seed);

    RenderScript pRS = createRenderScript(true);
    ScriptC_reflection3264 s = new ScriptC_reflection3264(pRS);

    Type.Builder typeBuilder = new Type.Builder(pRS, Element.U8_4(pRS));
    typeBuilder.setX(xSize).setY(ySize);
    Allocation inputAllocation = Allocation.createTyped(pRS, typeBuilder.create());
    byte[] inputArray = new byte[xSize * ySize * 4];
    r.nextBytes(inputArray);
    inputAllocation.copyFrom(inputArray);

    ScriptField_user_t.Item usrData = new ScriptField_user_t.Item();
    usrData.ans = new Short4(
        unsigned((byte)r.nextInt()),
        unsigned((byte)r.nextInt()),
        unsigned((byte)r.nextInt()),
        unsigned((byte)r.nextInt()));
    s.set_expect_ans(usrData.ans);
    usrData.x = unsigned(r.nextInt());
    s.set_expect_x(usrData.x);
    usrData.y = unsigned(r.nextInt());
    s.set_expect_y(usrData.y);

    usrData.alloc = inputAllocation;

    Allocation outputAllocation = Allocation.createTyped(pRS, typeBuilder.create());

    s.set_expect_dAlloc_GetDimX(xSize);
    s.set_expect_sAlloc_GetDimX(xSize);
    final int dXOff = r.nextInt();
    s.set_expect_dXOff(dXOff);
    final int dMip = r.nextInt();
    s.set_expect_dMip(dMip);
    final int count = r.nextInt();
    s.set_expect_count(count);
    final int sXOff = r.nextInt();
    s.set_expect_sXOff(sXOff);
    final int sMip = r.nextInt();
    s.set_expect_sMip(sMip);
    s.invoke_args(outputAllocation, dXOff, dMip, count, inputAllocation, sXOff, sMip);

    s.forEach_root(outputAllocation, usrData);
    byte[] outputArray = new byte[xSize * ySize * 4];
    outputAllocation.copyTo(outputArray);

    for (int i = 0; i < xSize; ++i)
      for (int j = 0; j < ySize; ++j) {
        int idx = j * xSize + i;
        assertEquals("[" + i + "][" + j + "]", inputArray[idx], outputArray[idx]);
      }

    s.invoke_check_asserts();
    pRS.finish();
    inputAllocation.destroy();
    outputAllocation.destroy();
    s.destroy();
    pRS.destroy();
  }
}
