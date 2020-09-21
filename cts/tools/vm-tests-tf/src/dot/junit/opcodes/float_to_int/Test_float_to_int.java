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

package dot.junit.opcodes.float_to_int;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.float_to_int.d.T_float_to_int_1;
import dot.junit.opcodes.float_to_int.d.T_float_to_int_5;

public class Test_float_to_int extends DxTestCase {

    /**
     * @title Argument = 2.999999f
     */
    public void testN1() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(2, t.run(2.999999f));
    }

    /**
     * @title Argument = 1
     */
    public void testN2() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(1, t.run(1f));
    }

    /**
     * @title Argument = -1
     */
    public void testN3() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(-1, t.run(-1f));
    }

    /**
     * @title Argument = -0f
     */
    public void testB1() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(0, t.run(-0f));
    }

    /**
     * @title Argument = Float.MAX_VALUE
     */
    public void testB2() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(Integer.MAX_VALUE, t.run(Float.MAX_VALUE));
    }

    /**
     * @title Argument = Float.MIN_VALUE
     */
    public void testB3() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(0, t.run(Float.MIN_VALUE));
    }

    /**
     * @title Argument = NaN
     */
    public void testB4() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(0, t.run(Float.NaN));
    }

    /**
     * @title Argument = POSITIVE_INFINITY
     */
    public void testB5() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(Integer.MAX_VALUE, t.run(Float.POSITIVE_INFINITY));
    }

    /**
     * @title Argument = NEGATIVE_INFINITY
     */
    public void testB6() {
        T_float_to_int_1 t = new T_float_to_int_1();
        assertEquals(Integer.MIN_VALUE, t.run(Float.NEGATIVE_INFINITY));
    }


    /**
     * @constraint B1 
     * @title type of argument - double
     */
    public void testVFE2() {
        load("dot.junit.opcodes.float_to_int.d.T_float_to_int_2", VerifyError.class);
    }

    /**
     * 
     * @constraint B1 
     * @title  type of argument - long
     */
    public void testVFE3() {
        load("dot.junit.opcodes.float_to_int.d.T_float_to_int_3", VerifyError.class);
    }

    /**
     * 
     * @constraint B1 
     * @title type of argument - reference
     */
    public void testVFE4() {
        load("dot.junit.opcodes.float_to_int.d.T_float_to_int_4", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE5() {
        load("dot.junit.opcodes.float_to_int.d.T_float_to_int_6", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Type of argument - int. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE6() {
        load("dot.junit.opcodes.float_to_int.d.T_float_to_int_5", VerifyError.class);
    }
}
