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

package dot.junit.opcodes.const_wide_32;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.const_wide_32.d.T_const_wide_32_1;

public class Test_const_wide_32 extends DxTestCase {
    /**
     * @title const-wide/32 v253, 20000000
     */
    public void testN1() {
        T_const_wide_32_1 t = new T_const_wide_32_1();
         long a = 10000000l;
         long b = 10000000l;
        assertEquals(a + b, t.run());
    }

    /**
     * @constraint A24 
     * @title  number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.const_wide_32.d.T_const_wide_32_3", VerifyError.class);
    }
    
    /**
     * @constraint B11 
     * @title  When writing to a register that is one half of a register 
     * pair, but not touching the other half, the old register pair gets broken up, and the 
     * other register involved in it becomes undefined
     */
    public void testVFE2() {
        load("dot.junit.opcodes.const_wide_32.d.T_const_wide_32_4", VerifyError.class);
    }

}
