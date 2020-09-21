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

package dot.junit.opcodes.move_result;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.move_result.d.T_move_result_1;

public class Test_move_result extends DxTestCase {
    /**
     * @title tests move-result functionality
     */
    public void testN1() {
        assertTrue(T_move_result_1.run());
    }


    /**
     * @constraint A23 
     * @title number of registers - dest is not valid
     */
    public void testVFE1() {
        load("dot.junit.opcodes.move_result.d.T_move_result_2", VerifyError.class);
    }


    /**
     * @constraint B1 
     * @title  reference
     */
    public void testVFE2() {
        load("dot.junit.opcodes.move_result.d.T_move_result_3", VerifyError.class);
    }
    
    /**
     * @constraint B1 
     * @title wide
     */
    public void testVFE3() {
        load("dot.junit.opcodes.move_result.d.T_move_result_4", VerifyError.class);
    }

    
    /**
     * @constraint B18 
     * @title When writing to a register that is one half of a 
     * register pair, but not touching the other half, the old register pair gets broken 
     * up, and the other register involved in it becomes undefined.
     */
    public void testVFE4() {
        load("dot.junit.opcodes.move_result.d.T_move_result_5", VerifyError.class);
    }
    
    /**
     * @constraint B19 
     * @title  move-result instruction must be immediately preceded 
     * (in the insns array) by an <invoke-kind> instruction
     */
    public void testVFE5() {
        load("dot.junit.opcodes.move_result.d.T_move_result_6", VerifyError.class);
    }
    
    /**
     * @constraint B20 
     * @title move-result instruction must be immediately preceded 
     * (in actual control flow) by an <invoke-kind> instruction
     */
    public void testVFE6() {
        load("dot.junit.opcodes.move_result.d.T_move_result_7", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE7() {
        load("dot.junit.opcodes.move_result.d.T_move_result_8", VerifyError.class);
    }
}
