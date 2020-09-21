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

package dot.junit.opcodes.cmp_long;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.cmp_long.d.T_cmp_long_1;
import dot.junit.opcodes.cmp_long.d.T_cmp_long_2;

public class Test_cmp_long extends DxTestCase {
    /**
     * @title Arguments = 111234567891l > 111234567890l
     */
    public void testN1() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(1, t.run(111234567891l, 111234567890l));
    }

    /**
     * @title Arguments = 112234567890 = 112234567890
     */
    public void testN2() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(0, t.run(112234567890l, 112234567890l));
    }

    /**
     * @title Arguments = 112234567890 < 998876543210
     */
    public void testN3() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(-1, t.run(112234567890l, 998876543210l));
    }

    /**
     * @title Arguments = Long.MAX_VALUE > Long.MIN_VALUE
     */
    public void testB1() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(1, t.run(Long.MAX_VALUE, Long.MIN_VALUE));
    }

    /**
     * @title Arguments = Long.MIN_VALUE < Long.MAX_VALUE
     */
    public void testB2() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(-1, t.run(Long.MIN_VALUE, Long.MAX_VALUE));
    }

    /**
     * @title Arguments = 1 > 0
     */
    public void testB3() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(1, t.run(1l, 0l));
    }

    /**
     * @title Arguments = 0 > -1
     */
    public void testB4() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(1, t.run(0l, -1l));
    }

    /**
     * @title Arguments = -1 < 0
     */
    public void testB5() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(-1, t.run(-1l, 0l));
    }

    /**
     * @title Arguments = 0 = 0
     */
    public void testB6() {
        T_cmp_long_1 t = new T_cmp_long_1();
        assertEquals(0, t.run(0l, 0l));
    }


    /**
     * @constraint B1
     * @title types of arguments - float, long
     */
    public void testVFE1() {
        load("dot.junit.opcodes.cmp_long.d.T_cmp_long_3", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - int, long
     */
    public void testVFE2() {
        load("dot.junit.opcodes.cmp_long.d.T_cmp_long_4", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - reference, long
     */
    public void testVFE3() {
        load("dot.junit.opcodes.cmp_long.d.T_cmp_long_5", VerifyError.class);
    }
    
    /**
     * @constraint A24 
     * @title number of registers
     */
    public void testVFE4() {
        load("dot.junit.opcodes.cmp_long.d.T_cmp_long_6", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - long, double. The verifier checks that longs
     * and doubles are not used interchangeably.
     */
    public void testVFE5() {
        load("dot.junit.opcodes.cmp_long.d.T_cmp_long_2", VerifyError.class);
    }
}
