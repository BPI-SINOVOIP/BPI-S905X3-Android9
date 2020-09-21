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

package dot.junit.opcodes.rem_int_lit8;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_1;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_2;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_3;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_4;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_5;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_6;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_7;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_8;
import dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_9;

public class Test_rem_int_lit8 extends DxTestCase {

    /**
     * @title Arguments = 8, 4
     */
    public void testN1() {
        T_rem_int_lit8_1 t = new T_rem_int_lit8_1();
        assertEquals(0, t.run(8));
    }

    /**
     * @title Arguments = 123, 4
     */
    public void testN2() {
        T_rem_int_lit8_1 t = new T_rem_int_lit8_1();
        assertEquals(3, t.run(123));
    }

    /**
     * @title Dividend = 0
     */
    public void testN3() {
        T_rem_int_lit8_1 t = new T_rem_int_lit8_1();
        assertEquals(0, t.run(0));
    }

    /**
     * @title Dividend is negative
     */
    public void testN4() {
        T_rem_int_lit8_1 t = new T_rem_int_lit8_1();
        assertEquals(-2, t.run(-10));
    }

    /**
     * @title Divisor is negative
     */
    public void testN5() {
        T_rem_int_lit8_2 t = new T_rem_int_lit8_2();
        assertEquals(0, t.run(123));
    }

    /**
     * @title Both Dividend and divisor are negative
     */
    public void testN6() {
        T_rem_int_lit8_3 t = new T_rem_int_lit8_3();
        assertEquals(-3, t.run(-123));
    }

    /**
     * @title Arguments = Byte.MIN_VALUE, -1
     */
    public void testB1() {
        T_rem_int_lit8_5 t = new T_rem_int_lit8_5();
        assertEquals(0, t.run(Byte.MIN_VALUE));
    }

    /**
     * @title Arguments = Byte.MIN_VALUE, 1
     */
    public void testB2() {
        T_rem_int_lit8_6 t = new T_rem_int_lit8_6();
        assertEquals(0, t.run(Byte.MIN_VALUE));
    }

    /**
     * @title Arguments = Byte.MAX_VALUE, 1
     */
    public void testB3() {
        T_rem_int_lit8_6 t = new T_rem_int_lit8_6();
        assertEquals(0, t.run(Byte.MAX_VALUE));
    }

    /**
     * @title Arguments = Short.MIN_VALUE, 127
     */
    public void testB4() {
        T_rem_int_lit8_7 t = new T_rem_int_lit8_7();
        assertEquals(-2, t.run(Short.MIN_VALUE));
    }

    /**
     * @title Arguments = 1, 127
     */
    public void testB5() {
        T_rem_int_lit8_7 t = new T_rem_int_lit8_7();
        assertEquals(1, t.run(1));
    }

    /**
     * @title Arguments = 1, -128
     */
    public void testB6() {
        T_rem_int_lit8_8 t = new T_rem_int_lit8_8();
        assertEquals(1, t.run(1));
    }

    /**
     * @title Divisor is 0
     */
    public void testE1() {
        loadAndRun("dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_9", ArithmeticException.class,
                   1);
    }

    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_10", VerifyError.class);
    }

    

    /**
     * @constraint B1 
     * @title types of arguments - int, double
     */
    public void testVFE2() {
        load("dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_11", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long, int
     */
    public void testVFE3() {
        load("dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_12", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - reference, int
     */
    public void testVFE4() {
        load("dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_13", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - float, int. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE5() {
        load("dot.junit.opcodes.rem_int_lit8.d.T_rem_int_lit8_4", VerifyError.class);
    }

}
