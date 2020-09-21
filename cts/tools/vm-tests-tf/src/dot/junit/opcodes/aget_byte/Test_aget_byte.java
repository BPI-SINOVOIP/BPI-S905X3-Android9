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

package dot.junit.opcodes.aget_byte;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.aget_byte.d.T_aget_byte_1;
import dot.junit.opcodes.aget_byte.d.T_aget_byte_8;


public class Test_aget_byte extends DxTestCase {
    /**
     * @title get byte from array
     */
    public void testN1() {
        T_aget_byte_1 t = new T_aget_byte_1();
        byte[] arr = new byte[2];
        arr[1] = 100;
        assertEquals(100, t.run(arr, 1));
    }

    /**
     * @title get byte from array
     */
    public void testN2() {
        T_aget_byte_1 t = new T_aget_byte_1();
        byte[] arr = new byte[2];
        arr[0] = 100;
        assertEquals(100, t.run(arr, 0));
    }

    /**
     * @title expected ArrayIndexOutOfBoundsException
     */
    public void testE1() {
        loadAndRun("dot.junit.opcodes.aget_byte.d.T_aget_byte_1",
                   ArrayIndexOutOfBoundsException.class, new byte[2], 2);
    }

    /**
     * @title expected NullPointerException
     */
    public void testE2() {
        loadAndRun("dot.junit.opcodes.aget_byte.d.T_aget_byte_1", NullPointerException.class, null,
                   2);
    }

    /**
     * @title expected ArrayIndexOutOfBoundsException (negative index)
     */
    public void testE3() {
        loadAndRun("dot.junit.opcodes.aget_byte.d.T_aget_byte_1",
                   ArrayIndexOutOfBoundsException.class, new byte[2], -1);
    }

    /**
     * @constraint B1
     * @title types of arguments - array, double
     */
    public void testVFE1() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_2", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - array, long
     */
    public void testVFE2() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_3", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - Object, short
     */
    public void testVFE3() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_4", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - double[], int
     */
    public void testVFE4() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_5", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - int[], int
     */
    public void testVFE5() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_6", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - array, reference
     */
    public void testVFE6() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_7", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE7() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_9", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Type of index argument - float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE8() {
        load("dot.junit.opcodes.aget_byte.d.T_aget_byte_8", VerifyError.class);
    }

}
