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
import android.renderscript.Type;

public class UT_alloc_supportlib extends UnitTest {
    private Type T;
    private Type mTFaces;
    private Type mTLOD;
    private Type mTFacesLOD;
    private Allocation mAFaces;
    private Allocation mALOD;
    private Allocation mAFacesLOD;

    public UT_alloc_supportlib(Context ctx) {
        super("Alloc (Support Lib)", ctx);
    }

    private void initializeGlobals(RenderScript RS, ScriptC_alloc_supportlib s) {
        Type.Builder typeBuilder = new Type.Builder(RS, Element.I32(RS));
        int X = 5;
        int Y = 7;
        int Z = 0;
        s.set_dimX(X);
        s.set_dimY(Y);
        s.set_dimZ(Z);
        typeBuilder.setX(X).setY(Y);
        T = typeBuilder.create();
        Allocation A = Allocation.createTyped(RS, T);
        s.bind_a(A);
        s.set_aRaw(A);

        typeBuilder = new Type.Builder(RS, Element.I32(RS));
        typeBuilder.setX(X).setY(Y).setFaces(true);
        mTFaces = typeBuilder.create();
        mAFaces = Allocation.createTyped(RS, mTFaces);
        s.set_aFaces(mAFaces);
        typeBuilder.setFaces(false).setMipmaps(true);
        mTLOD = typeBuilder.create();
        mALOD = Allocation.createTyped(RS, mTLOD);
        s.set_aLOD(mALOD);
        typeBuilder.setFaces(true).setMipmaps(true);
        mTFacesLOD = typeBuilder.create();
        mAFacesLOD = Allocation.createTyped(RS, mTFacesLOD);
        s.set_aFacesLOD(mAFacesLOD);

        return;
    }

    public void run() {
        RenderScript pRS = createRenderScript(true);
        ScriptC_alloc_supportlib s = new ScriptC_alloc_supportlib(pRS);
        initializeGlobals(pRS, s);
        s.forEach_root(s.get_aRaw());
        s.invoke_alloc_supportlib_test();
        pRS.finish();
        T.destroy();
        s.get_a().destroy();
        mAFaces.destroy();
        mALOD.destroy();
        mAFacesLOD.destroy();
        mTFaces.destroy();
        mTLOD.destroy();
        mTFacesLOD.destroy();
        s.destroy();
        pRS.destroy();
    }
}
