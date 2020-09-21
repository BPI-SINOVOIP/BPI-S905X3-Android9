/*
 * Copyright (C) 2008 The Android Open Source Project
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

package dot.junit.opcodes.aput_wide;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.aput_wide.d.T_aput_wide_1;
import dot.junit.opcodes.aput_wide.d.T_aput_wide_2;
import dot.junit.opcodes.aput_wide.d.T_aput_wide_6;
import dot.junit.opcodes.aput_wide.d.T_aput_wide_9;


public class Test_aput_wide extends DxTestCase {
    /**
     * @title put long into array
     */
    public void testN1() {
        T_aput_wide_1 t = new T_aput_wide_1();
        long[] arr = new long[2];
        t.run(arr, 1, 100000000000l);
        assertEquals(100000000000l, arr[1]);
    }

    /**
     * @title put long into array
     */
    public void testN2() {
        T_aput_wide_1 t = new T_aput_wide_1();
        long[] arr = new long[2];
        t.run(arr, 0, 100000000000l);
        assertEquals(100000000000l, arr[0]);
    }

    /**
     * @title put double into array
     */
    public void testN3() {
        T_aput_wide_2 t = new T_aput_wide_2();
        double[] arr = new double[2];
        t.run(arr, 0, 3.1415d);
        assertEquals(3.1415d, arr[0]);
    }

    /**
     * @title expected ArrayIndexOutOfBoundsException
     */
    public void testE1() {
        loadAndRun("dot.junit.opcodes.aput_wide.d.T_aput_wide_1",
                   ArrayIndexOutOfBoundsException.class, new long[2], 2, 100000000000l);
    }

    /**
     * @title expected NullPointerException
     */
    public void testE2() {
        loadAndRun("dot.junit.opcodes.aput_wide.d.T_aput_wide_1", NullPointerException.class,
                   null, 2, 100000000000l);
    }

    /**
     * @title expected ArrayIndexOutOfBoundsException (negative index)
     */
    public void testE3() {
        loadAndRun("dot.junit.opcodes.aput_wide.d.T_aput_wide_1",
                   ArrayIndexOutOfBoundsException.class, new long[2], -1, 100000000000l);
    }


    /**
     * @constraint B1 
     * @title types of arguments - array, double, long
     */
    public void testVFE1() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_3", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - array, int, int
     */
    public void testVFE2() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_4", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - object, int, long
     */
    public void testVFE3() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_5", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Type of index argument - float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE4() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_9", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - int[], int, long
     */
    public void testVFE5() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_7", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - array, reference, long
     */
    public void testVFE6() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_8", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE7() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_10", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Try to put long into double[]. The verifier checks that longs
     * and doubles are not used interchangeably.
     */
    public void testVFE8() {
        load("dot.junit.opcodes.aput_wide.d.T_aput_wide_6", VerifyError.class);
    }
}
