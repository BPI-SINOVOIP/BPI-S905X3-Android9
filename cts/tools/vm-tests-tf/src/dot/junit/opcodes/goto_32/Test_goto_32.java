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

package dot.junit.opcodes.goto_32;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.goto_32.d.T_goto_32_1;

public class Test_goto_32 extends DxTestCase {
       /**
        * @title check forward and backward goto. This test also tests constraint C17 allowing to have
        * backward goto as a last opcode in the method.
        */
       public void testN1() {
           T_goto_32_1 t = new T_goto_32_1();
           assertEquals(0, t.run(20));
       }

       /**
        * @constraint A6 
        * @title branch target is inside instruction
        */
       public void testVFE1() {
        load("dot.junit.opcodes.goto_32.d.T_goto_32_2", VerifyError.class);
       }

       /**
        * @constraint A6 
        * @title branch target shall be inside the method
        */
       public void testVFE2() {
        load("dot.junit.opcodes.goto_32.d.T_goto_32_3", VerifyError.class);
       }

       /**
        *  
        * @constraint n/a 
        * @title zero offset - no exception expected
        */
       public void testVFE3() {
           load("dot.junit.opcodes.goto_32.d.T_goto_32_4", null);
       }

}
