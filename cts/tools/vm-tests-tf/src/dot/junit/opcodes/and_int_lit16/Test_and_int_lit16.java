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

package dot.junit.opcodes.and_int_lit16;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_5;
import dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_1;
import dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_2;
import dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_3;
import dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_4;
import dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_9;

public class Test_and_int_lit16 extends DxTestCase {
    /**
     * @title Arguments = 15 & 8
     */
    public void testN1() {
        T_and_int_lit16_1 t = new T_and_int_lit16_1();
        assertEquals(8, t.run());
    }

    /**
     * @title Arguments = 0xfffffff8 & -8
     */
    public void testN2() {
        T_and_int_lit16_2 t = new T_and_int_lit16_2();
        assertEquals(-8, t.run());
    }

    /**
     * @title Arguments = 0xcafe & -1
     */
    public void testN3() {
        T_and_int_lit16_3 t = new T_and_int_lit16_3();
        assertEquals(0xcafe, t.run());
    }

    /**
     * @title Arguments = 0 & -1
     */
    public void testB1() {
        T_and_int_lit16_4 t = new T_and_int_lit16_4();
        assertEquals(0, t.run());
    }

    /**
     * @title Arguments = Short.MAX_VALUE & Short.MIN_VALUE
     */
    public void testB2() {
        T_and_int_lit16_5 t = new T_and_int_lit16_5();
        assertEquals(0, t.run());
    }

    

    /**
     * @constraint B1 
     * @title  types of arguments - double & int
     */
    public void testVFE1() {
        load("dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_6", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long & int
     */
    public void testVFE2() {
        load("dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_7", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - int & reference
     */
    public void testVFE3() {
        load("dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_8", VerifyError.class);
    }

    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE5() {
        load("dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_10", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE6() {
        load("dot.junit.opcodes.and_int_lit16.d.T_and_int_lit16_9", VerifyError.class);
    }
}
