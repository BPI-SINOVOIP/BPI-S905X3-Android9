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
import android.renderscript.RenderScript;
import java.util.Random;

public class UT_struct_field extends UnitTest {

    Random r;

    public UT_struct_field(Context ctx) {
        super("Structure-typed Fields", ctx);
        r = new Random(0);
    }

    private ScriptField_InnerOne.Item makeInnerOne() {
        ScriptField_InnerOne.Item innerOne = new ScriptField_InnerOne.Item();
        innerOne.x = r.nextInt();
        innerOne.y = r.nextInt();
        innerOne.f = r.nextFloat();
        return innerOne;
    }

    private ScriptField_InnerTwo.Item makeInnerTwo() {
        ScriptField_InnerTwo.Item innerTwo = new ScriptField_InnerTwo.Item();
        innerTwo.z = (byte)r.nextInt();
        innerTwo.innerOne = makeInnerOne();
        return innerTwo;
    }

    public void run() {
        RenderScript pRS = createRenderScript(true);
        ScriptC_struct_field s = new ScriptC_struct_field(pRS);

        ScriptField_Outer.Item outer = new ScriptField_Outer.Item();
        outer.innerOneA = makeInnerOne();
        outer.l = r.nextLong();
        outer.innerOneB = makeInnerOne();
        for (int i = 0; i < 3; ++i)
            outer.innerTwo3[i] = makeInnerTwo();
        for (int i = 0; i < 2; ++i)
            outer.innerTwo2[i] = makeInnerTwo();
        for (int i = 0; i < 4; ++i)
            outer.innerOne4[i] = makeInnerOne();
        outer.innerOneC = makeInnerOne();
        s.set_outer(outer);

        s.invoke_checkOuter(
            outer.innerOneA.x, outer.innerOneA.y, outer.innerOneA.f,
            outer.l,
            outer.innerOneB.x, outer.innerOneB.y, outer.innerOneB.f,
            outer.innerTwo3[0].z,
            outer.innerTwo3[0].innerOne.x, outer.innerTwo3[0].innerOne.y, outer.innerTwo3[0].innerOne.f,
            outer.innerTwo3[1].z,
            outer.innerTwo3[1].innerOne.x, outer.innerTwo3[1].innerOne.y, outer.innerTwo3[1].innerOne.f,
            outer.innerTwo3[2].z,
            outer.innerTwo3[2].innerOne.x, outer.innerTwo3[2].innerOne.y, outer.innerTwo3[2].innerOne.f,
            outer.innerTwo2[0].z,
            outer.innerTwo2[0].innerOne.x, outer.innerTwo2[0].innerOne.y, outer.innerTwo2[0].innerOne.f,
            outer.innerTwo2[1].z,
            outer.innerTwo2[1].innerOne.x, outer.innerTwo2[1].innerOne.y, outer.innerTwo2[1].innerOne.f,
            outer.innerOne4[0].x, outer.innerOne4[0].y, outer.innerOne4[0].f,
            outer.innerOne4[1].x, outer.innerOne4[1].y, outer.innerOne4[1].f,
            outer.innerOne4[2].x, outer.innerOne4[2].y, outer.innerOne4[2].f,
            outer.innerOne4[3].x, outer.innerOne4[3].y, outer.innerOne4[3].f,
            outer.innerOneC.x, outer.innerOneC.y, outer.innerOneC.f);

        pRS.finish();
        pRS.destroy();
    }
}
