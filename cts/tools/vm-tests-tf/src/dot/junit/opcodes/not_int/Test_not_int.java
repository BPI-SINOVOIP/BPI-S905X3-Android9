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

package dot.junit.opcodes.not_int;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.not_int.d.T_not_int_1;
import dot.junit.opcodes.not_int.d.T_not_int_2;

public class Test_not_int extends DxTestCase {
    
    /**
     * @title Argument = 5; 256
     */
    public void testN1() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(-6, t.run(5));
        assertEquals(-257, t.run(256));
    }
    
    /**
     * @title Argument = -5, -256
     */
    public void testN2() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(4, t.run(-5));
        assertEquals(255, t.run(-256));
    }
    
    /**
     * @title Argument = 0xcafe; 0x12c
     */
    public void testN3() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(-0xcaff, t.run(0xcafe));
        assertEquals(-0x12d, t.run(0x12c));
    }

    /**
     * @title Argument = Integer.MAX_VALUE
     */
    public void testB1() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(Integer.MIN_VALUE, t.run(Integer.MAX_VALUE));
    }
    
    /**
     * @title Argument = Integer.MIN_VALUE
     */
    public void testB2() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(Integer.MAX_VALUE, t.run(Integer.MIN_VALUE));
    }
    
    /**
     * @title Argument = 1
     */
    public void testB3() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(-2, t.run(1));
    }
    
    /**
     * @title Argument = 0
     */
    public void testB4() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(-1, t.run(0));
    }
    
    /**
     * @title Argument = -1
     */
    public void testB5() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(0, t.run(-1));
    }
    
    /**
     * @title Argument = Short.MAX_VALUE
     */
    public void testB6() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(Short.MIN_VALUE, t.run(Short.MAX_VALUE));
    }
    
    /**
     * @title Argument = Short.MIN_VALUE
     */
    public void testB7() {
        T_not_int_1 t = new T_not_int_1();
        assertEquals(Short.MAX_VALUE, t.run(Short.MIN_VALUE));
    }

    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.not_int.d.T_not_int_3", VerifyError.class);
    }

    

    /**
     * @constraint B1 
     * @title types of arguments - double
     */
    public void testVFE2() {
        load("dot.junit.opcodes.not_int.d.T_not_int_4", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long
     */
    public void testVFE3() {
        load("dot.junit.opcodes.not_int.d.T_not_int_5", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - reference
     */
    public void testVFE4() {
        load("dot.junit.opcodes.not_int.d.T_not_int_6", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE5() {
        load("dot.junit.opcodes.not_int.d.T_not_int_2", VerifyError.class);
    }

}
