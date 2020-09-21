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

package dot.junit.opcodes.opc_const;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.opc_const.d.T_opc_const_1;
import dot.junit.opcodes.opc_const.d.T_opc_const_2;

public class Test_opc_const extends DxTestCase {
    /**
     * @title const v1, 1.54
     */
    public void testN1() {
        T_opc_const_1 t = new T_opc_const_1();
        float a = 1.5f;
        float b = 0.04f;
        assertEquals(a + b, t.run(), 0f);
        assertEquals(1.54f, t.run(), 0f);
    }
    
    /**
     * @title const v254, 20000000 
     */
    public void testN2() {
        T_opc_const_2 t = new T_opc_const_2();
         int a = 10000000;
         int b = 10000000;
        assertEquals(a + b, t.run());
    }

    /**
     * @constraint B1 
     * @title number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.opc_const.d.T_opc_const_3", VerifyError.class);
    }
    
    /**
     * @constraint B11 
     * @title When writing to a register that is one half of a register 
     * pair, but not touching the other half, the old register pair gets broken up, and the 
     * other register involved in it becomes undefined
     */
    public void testVFE2() {
        load("dot.junit.opcodes.opc_const.d.T_opc_const_4", VerifyError.class);
    }

}
