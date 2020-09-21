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

package dot.junit.opcodes.move_object_16;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.move_object_16.d.T_move_object_16_1;

public class Test_move_object_16 extends DxTestCase {
    /**
     * @title v4999 -> v4000 -> v1
     */
    public void testN1() {
        T_move_object_16_1 t = new T_move_object_16_1(); 
        assertEquals(t.run(), t);
    }
    
    /**
     * @constraint A23 
     * @title  number of registers - src is not valid
     */
    public void testVFE1() {
        load("dot.junit.opcodes.move_object_16.d.T_move_object_16_3", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers - dst is not valid
     */
    public void testVFE2() {
        load("dot.junit.opcodes.move_object_16.d.T_move_object_16_4", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title src register contains integer
     */
    public void testVFE3() {
        load("dot.junit.opcodes.move_object_16.d.T_move_object_16_5", VerifyError.class);
    }
    
    /**
     * @constraint B1 
     * @title src register contains wide
     */
    public void testVFE4() {
        load("dot.junit.opcodes.move_object_16.d.T_move_object_16_6", VerifyError.class);
    }
    
    /**
     * @constraint B1 
     * @title src register is a part of reg pair
     */
    public void testVFE5() {
        load("dot.junit.opcodes.move_object_16.d.T_move_object_16_7", VerifyError.class);
    }
    
    /**
     * @constraint B18 
     * @title When writing to a register that is one half of a 
     * register pair, but not touching the other half, the old register pair gets broken 
     * up, and the other register involved in it becomes undefined.
     */
    public void testVFE6() {
        load("dot.junit.opcodes.move_object_16.d.T_move_object_16_8", VerifyError.class);
    }
}
