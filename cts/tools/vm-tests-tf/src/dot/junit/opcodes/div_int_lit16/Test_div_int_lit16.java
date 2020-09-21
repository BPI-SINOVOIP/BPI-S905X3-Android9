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

package dot.junit.opcodes.div_int_lit16;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_1;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_10;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_11;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_12;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_13;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_17;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_2;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_3;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_4;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_5;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_6;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_7;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_8;
import dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_9;


public class Test_div_int_lit16 extends DxTestCase {
    /**
     * @title Arguments = 8 / 4
     */
    public void testN1() {
        T_div_int_lit16_1 t = new T_div_int_lit16_1();
        assertEquals(2, t.run());
    }

    /**
     * @title Rounding
     */
    public void testN2() {
        T_div_int_lit16_2 t = new T_div_int_lit16_2();
        assertEquals(268435455, t.run());
    }

    /**
     * @title Dividend = 0
     */
    public void testN3() {
        T_div_int_lit16_3 t = new T_div_int_lit16_3();
        assertEquals(0, t.run());
    }

    /**
     * @title Dividend is negative
     */
    public void testN4() {
        T_div_int_lit16_4 t = new T_div_int_lit16_4();
        assertEquals(-3, t.run());
    }

    /**
     * @title Divisor is negative
     */
    public void testN5() {
        T_div_int_lit16_5 t = new T_div_int_lit16_5();
        assertEquals(-357913941, t.run());
    }

    /**
     * @title Both Dividend and divisor are negative
     */
    public void testN6() {
        T_div_int_lit16_6 t = new T_div_int_lit16_6();
        assertEquals(5965, t.run());
    }

    /**
     * @title Arguments = Integer.MIN_VALUE / -1
     */
    public void testB1() {
        T_div_int_lit16_7 t = new T_div_int_lit16_7();
        // result is MIN_VALUE because overflow occurs in this case
        assertEquals(Integer.MIN_VALUE, t.run());
    }

    /**
     * @title Arguments = Integer.MIN_VALUE / 1
     */
    public void testB2() {
        T_div_int_lit16_8 t = new T_div_int_lit16_8();
        assertEquals(Integer.MIN_VALUE, t.run());
    }

    /**
     * @title Arguments = Integer.MAX_VALUE / 1
     */
    public void testB3() {
        T_div_int_lit16_9 t = new T_div_int_lit16_9();
        assertEquals(Integer.MAX_VALUE, t.run());
    }

    /**
     * @title Arguments = Integer.MIN_VALUE / Short.MAX_VALUE
     */
    public void testB4() {
        T_div_int_lit16_10 t = new T_div_int_lit16_10();
        assertEquals(-65538, t.run());
    }

    /**
     * @title Arguments = 1 / Short.MAX_VALUE
     */
    public void testB5() {
        T_div_int_lit16_11 t = new T_div_int_lit16_11();
        assertEquals(0, t.run());
    }

    /**
     * @title Arguments = 1 / Short.MIN_VALUE
     */
    public void testB6() {
        T_div_int_lit16_12 t = new T_div_int_lit16_12();
        assertEquals(0, t.run());
    }

    /**
     * @title Divisor is 0
     */
    public void testE1() {
        loadAndRun("dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_13",
                   ArithmeticException.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - int / double
     */
    public void testVFE1() {
        load("dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_14", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long / int
     */
    public void testVFE2() {
        load("dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_15", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - reference / int
     */
    public void testVFE3() {
        load("dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_16", VerifyError.class);
    }

    /**
     * @constraint A23 
     * @title  number of registers
     */
    public void testVFE4() {
        load("dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_18", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE5() {
        load("dot.junit.opcodes.div_int_lit16.d.T_div_int_lit16_17", VerifyError.class);
    }
}
