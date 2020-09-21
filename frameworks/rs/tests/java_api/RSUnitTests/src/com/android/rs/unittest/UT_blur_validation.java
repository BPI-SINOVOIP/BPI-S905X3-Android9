/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.rs.unittest;

import android.content.Context;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;
import android.renderscript.RSIllegalArgumentException;
import android.renderscript.ScriptIntrinsicBlur;
import android.renderscript.Type;
import android.util.Log;

// Tests that ScriptIntrinsicBlur properly throws exception if input or output
// are set to 1D Allocations.
public class UT_blur_validation extends UnitTest {
    private static final String TAG = "ScriptIntrinsicBlur validation";
    private RenderScript RS;
    private Allocation input1D, output1D;
    private Allocation input2D, output2D;
    private ScriptIntrinsicBlur scriptBlur;

    public UT_blur_validation(Context ctx) {
        super(TAG, ctx);
    }

    private void cleanup() {
        RS.finish();
        input1D.destroy();
        input2D.destroy();
        output1D.destroy();
        output2D.destroy();
        scriptBlur.destroy();
        RS.destroy();
    }

    public void run() {
        RS = createRenderScript(false);

        final int width  = 100;
        final int height = 100;

        input1D = Allocation.createSized(RS,
                                         Element.U8(RS),
                                         width * height,
                                         Allocation.USAGE_SCRIPT);

        output1D = Allocation.createTyped(RS, input1D.getType());

        Type.Builder typeBuilder = new Type.Builder(RS, Element.U8(RS));
        typeBuilder.setX(width);
        typeBuilder.setY(height);
        Type ty = typeBuilder.create();

        input2D  = Allocation.createTyped(RS, ty);
        output2D = Allocation.createTyped(RS, ty);

        scriptBlur = ScriptIntrinsicBlur.create(RS, Element.U8(RS));

        scriptBlur.setRadius(25f);
        boolean failed = false;
        try {
            scriptBlur.setInput(input1D);
        } catch (RSIllegalArgumentException e) {
            scriptBlur.setInput(input2D);
            try {
                scriptBlur.forEach(output1D);
            } catch (RSIllegalArgumentException e1) {
                scriptBlur.forEach(output2D);
                cleanup();
                passTest();
                return;
            }
            Log.e(TAG, "setting 1d output does not trigger exception");
            cleanup();
            failTest();
            return;
        }

        Log.e(TAG, "setting 1d input does not trigger exception");
        cleanup();
        failTest();
    }
}
