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

package dot.junit.opcodes.cmpl_float;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.cmpl_float.d.T_cmpl_float_6;
import dot.junit.opcodes.cmpl_float.d.T_cmpl_float_1;

public class Test_cmpl_float extends DxTestCase {
    /**
     * @title Arguments = 3.14f, 2.7f
     */
    public void testN1() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(1, t.run(3.14f, 2.7f));
    }

    /**
     * @title Arguments = -3.14f, 2.7f
     */
    public void testN2() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(-1, t.run(-3.14f, 2.7f));
    }

    /**
     * @title Arguments = 3.14, 3.14
     */
    public void testN3() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(0, t.run(3.14f, 3.14f));
    }

    /**
     * @title Arguments = Float.NaN, Float.MAX_VALUE
     */
    public void testB1() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(-1, t.run(Float.NaN, Float.MAX_VALUE));
    }

    /**
     * @title Arguments = +0, -0
     */
    public void testB2() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(0, t.run(+0f, -0f));
    }

    /**
     * @title Arguments = Float.NEGATIVE_INFINITY, Float.MIN_VALUE
     */
    public void testB3() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(-1, t.run(Float.NEGATIVE_INFINITY, Float.MIN_VALUE));
    }

    /**
     * @title Arguments = Float.POSITIVE_INFINITY, Float.MAX_VALUE
     */
    public void testB4() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(1, t.run(Float.POSITIVE_INFINITY, Float.MAX_VALUE));
    }

    /**
     * @title Arguments = Float.POSITIVE_INFINITY,
     * Float.NEGATIVE_INFINITY
     */
    public void testB5() {
        T_cmpl_float_1 t = new T_cmpl_float_1();
        assertEquals(1, t.run(Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY));
    }

    

    /**
     * @constraint B1 
     * @title  types of arguments - float, double
     */
    public void testVFE1() {
        load("dot.junit.opcodes.cmpl_float.d.T_cmpl_float_2", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title types of arguments - long, float
     */
    public void testVFE2() {
        load("dot.junit.opcodes.cmpl_float.d.T_cmpl_float_3", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title  types of arguments - reference, float
     */
    public void testVFE3() {
        load("dot.junit.opcodes.cmpl_float.d.T_cmpl_float_4", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE4() {
        load("dot.junit.opcodes.cmpl_float.d.T_cmpl_float_5", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE5() {
        load("dot.junit.opcodes.cmpl_float.d.T_cmpl_float_6", VerifyError.class);
    }

}
