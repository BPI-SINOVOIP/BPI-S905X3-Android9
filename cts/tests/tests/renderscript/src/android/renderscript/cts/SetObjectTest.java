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

package android.renderscript.cts;

import android.renderscript.*;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.util.Log;

import junit.framework.Assert;

public class SetObjectTest extends RSBaseCompute {
    int ObjectNum = 1;
    private Allocation mIn;
    private Allocation mOut;

    Element element;
    Type type;
    Allocation allocation;
    Sampler sampler;
    Script script;

    private ScriptC_set_object ms_set;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        element = Element.BOOLEAN(mRS);

        type = new Type.Builder(mRS, Element.I8(mRS)).setX(1).create();
        allocation = Allocation.createTyped(mRS, type);
        sampler = new Sampler.Builder(mRS).create();
        script = new ScriptC_set_object(mRS);


        ms_set = new ScriptC_set_object(mRS);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mIn != null) {
            mIn.destroy();
        }
        if (mOut != null) {
            mOut.destroy();
        }
        allocation.destroy();
        script.destroy();
        ms_set.destroy();
        super.tearDown();
    }

    /**
     * rsSetObject test
     */
    public void testSetObjectElement() {
        ScriptField__set_object_element_input field = new ScriptField__set_object_element_input(
                mRS, 1);
        ScriptField__set_object_element_input.Item mItem = new ScriptField__set_object_element_input.Item();
        mItem.element = element;
        field.set(mItem, 0, true);

        mIn = field.getAllocation();
        mOut = Allocation.createSized(mRS, Element.I32(mRS), ObjectNum);
        try {
            ms_set.forEach_set_object_element(mIn, mOut);
        } catch (RSRuntimeException e) {
            Log.i("compare", "rsSetObject root fail");
        }
        int[] tmpArray = new int[ObjectNum];
        mOut.copyTo(tmpArray);

        Assert.assertTrue("rsSetObject element test fail: " + "Expect 1;value "
                + tmpArray[0], tmpArray[0] == 1);
    }

    public void testSetObjectType() {
        ScriptField__set_object_type_input field = new ScriptField__set_object_type_input(mRS, 1);
        ScriptField__set_object_type_input.Item mItem = new ScriptField__set_object_type_input.Item();
        mItem.type = type;
        field.set(mItem, 0, true);

        mIn = field.getAllocation();
        mOut = Allocation.createSized(mRS, Element.I32(mRS), ObjectNum);

        try {
            ms_set.forEach_set_object_type(mIn, mOut);
        } catch (RSRuntimeException e) {
            Log.i("compare", "rsSetObject root fail");
        }
        int[] tmpArray = new int[ObjectNum];
        mOut.copyTo(tmpArray);

        Assert.assertTrue(
                "rsSetObject type test fail: " + "Expect 1;value " + tmpArray[0],
                tmpArray[0] == 1);
    }

    public void testSetObjectAllocation() {
        ScriptField__set_object_allocation_input field = new ScriptField__set_object_allocation_input(
                mRS, 1);
        ScriptField__set_object_allocation_input.Item mItem = new ScriptField__set_object_allocation_input.Item();
        mItem.allocation = allocation;
        field.set(mItem, 0, true);

        mIn = field.getAllocation();
        mOut = Allocation.createSized(mRS, Element.I32(mRS), ObjectNum);

        try {
            ms_set.forEach_set_object_allocation(mIn, mOut);
        } catch (RSRuntimeException e) {
            Log.i("compare", "rsSetObject root fail");
        }
        int[] tmpArray = new int[ObjectNum];
        mOut.copyTo(tmpArray);

        Assert.assertTrue("rsSetObject allocation test fail: " + "Expect 1;value "
                + tmpArray[0], tmpArray[0] == 1);
    }

    public void testSetObjectSampler() {
        ScriptField__set_object_sampler_input field = new ScriptField__set_object_sampler_input(
                mRS, 1);
        ScriptField__set_object_sampler_input.Item mItem = new ScriptField__set_object_sampler_input.Item();
        mItem.sampler = sampler;
        field.set(mItem, 0, true);

        mIn = field.getAllocation();
        mOut = Allocation.createSized(mRS, Element.I32(mRS), ObjectNum);

        try {
            ms_set.forEach_set_object_sampler(mIn, mOut);
        } catch (RSRuntimeException e) {
            Log.i("compare", "rsSetObject root fail");
        }
        int[] tmpArray = new int[ObjectNum];
        mOut.copyTo(tmpArray);

        Assert.assertTrue("rsSetObject sampler test fail: " + "Expect 1;value "
                + tmpArray[0], tmpArray[0] == 1);
    }

    public void testSetObjectScript() {
        ScriptField__set_object_script_input field = new ScriptField__set_object_script_input(
                mRS, 1);
        ScriptField__set_object_script_input.Item mItem = new ScriptField__set_object_script_input.Item();
        mItem.script = script;
        field.set(mItem, 0, true);

        mIn = field.getAllocation();
        mOut = Allocation.createSized(mRS, Element.I32(mRS), ObjectNum);

        try {
            ms_set.forEach_set_object_script(mIn, mOut);
        } catch (RSRuntimeException e) {
            Log.i("compare", "rsSetObject root fail");
        }
        int[] tmpArray = new int[ObjectNum];
        mOut.copyTo(tmpArray);

        Assert.assertTrue("rsSetObject script test fail: " + "Expect 1;value "
                + tmpArray[0], tmpArray[0] == 1);
    }
}
