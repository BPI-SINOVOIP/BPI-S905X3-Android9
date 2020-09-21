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

package dot.junit.opcodes.xor_int_2addr;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.xor_int_2addr.d.T_xor_int_2addr_1;
import dot.junit.opcodes.xor_int_2addr.d.T_xor_int_2addr_4;

public class Test_xor_int_2addr extends DxTestCase {
    /**
     * @title Arguments = 15, 8
     */
    public void testN1() {
         T_xor_int_2addr_1 t = new T_xor_int_2addr_1();
         assertEquals(7, t.run(15, 8));
    }
    
    /**
     * @title Arguments = 0xfffffff8, 0xfffffff1
     */
    public void testN2() {
         T_xor_int_2addr_1 t = new T_xor_int_2addr_1();
         assertEquals(9, t.run(0xfffffff8, 0xfffffff1));
    }

    /**
     * @title Arguments = 0xcafe, -1
     */
    public void testN3() {
         T_xor_int_2addr_1 t = new T_xor_int_2addr_1();
         assertEquals(0xFFFF3501, t.run(0xcafe, -1));
    }

    /**
     * @title Arguments = 0, -1
     */
    public void testB1() {
        T_xor_int_2addr_1 t = new T_xor_int_2addr_1();
        assertEquals(-1, t.run(0, -1));
    }

    /**
     * @title Arguments = Integer.MAX_VALUE, Integer.MIN_VALUE
     */
    public void testB2() {
        T_xor_int_2addr_1 t = new T_xor_int_2addr_1();
        assertEquals(0xffffffff, t.run(Integer.MAX_VALUE, Integer.MIN_VALUE));
    }
    
    

    /**
     * @constraint B1 
     * @title types of arguments - long, int
     */
    public void testVFE1() {
        load("dot.junit.opcodes.xor_int_2addr.d.T_xor_int_2addr_2", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title  types of arguments - reference, int
     */
    public void testVFE2() {
        load("dot.junit.opcodes.xor_int_2addr.d.T_xor_int_2addr_3", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE3() {
        load("dot.junit.opcodes.xor_int_2addr.d.T_xor_int_2addr_5", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE4() {
        load("dot.junit.opcodes.xor_int_2addr.d.T_xor_int_2addr_4", VerifyError.class);
    }
}
