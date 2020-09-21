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

package dot.junit.opcodes.and_int;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.and_int.d.T_and_int_1;
import dot.junit.opcodes.and_int.d.T_and_int_5;


public class Test_and_int extends DxTestCase {
    /**
     * @title Arguments = 15, 8
     */
    public void testN1() {
        T_and_int_1 t = new T_and_int_1();
        assertEquals(8, t.run(15, 8));
    }

    /**
     * @title Arguments = 0xfffffff8, 0xfffffff1
     */
    public void testN2() {
        T_and_int_1 t = new T_and_int_1();
        assertEquals(0xfffffff0, t.run(0xfffffff8, 0xfffffff1));
    }

    /**
     * @title Arguments = 0xcafe & -1
     */
    public void testN3() {
        T_and_int_1 t = new T_and_int_1();
        assertEquals(0xcafe, t.run(0xcafe, -1));
    }

    /**
     * @title Arguments = 0 & -1
     */
    public void testB1() {
        T_and_int_1 t = new T_and_int_1();
        assertEquals(0, t.run(0, -1));
    }

    /**
     * @title Arguments = Integer.MAX_VALUE & Integer.MIN_VALUE
     */
    public void testB2() {
        T_and_int_1 t = new T_and_int_1();
        assertEquals(0, t.run(Integer.MAX_VALUE, Integer.MIN_VALUE));
    }

    

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE1() {
        load("dot.junit.opcodes.and_int.d.T_and_int_5", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - double & int
     */
    public void testVFE2() {
        load("dot.junit.opcodes.and_int.d.T_and_int_2", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long & int
     */
    public void testVFE3() {
        load("dot.junit.opcodes.and_int.d.T_and_int_3", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - int & reference
     */
    public void testVFE4() {
        load("dot.junit.opcodes.and_int.d.T_and_int_4", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE5() {
        load("dot.junit.opcodes.and_int.d.T_and_int_6", VerifyError.class);
    }
}
