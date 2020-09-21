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

package dot.junit.opcodes.aput_object;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.aput_object.d.T_aput_object_1;
import dot.junit.opcodes.aput_object.d.T_aput_object_12;
import dot.junit.opcodes.aput_object.d.T_aput_object_2;
import dot.junit.opcodes.aput_object.d.T_aput_object_3;
import dot.junit.opcodes.aput_object.d.T_aput_object_4;

public class Test_aput_object extends DxTestCase {
    /**
     * @title put reference into array
     */
    public void testN1() {
        T_aput_object_1 t = new T_aput_object_1();
        String[] arr = new String[2];
        t.run(arr, 0, "hello");
        assertEquals("hello", arr[0]);
    }

    /**
     * @title put reference into array
     */
    public void testN2() {
        T_aput_object_1 t = new T_aput_object_1();
        String[] value = {"world", null, ""};
        String[] arr = new String[2];
        for (int i = 0; i < value.length; i++) {
            t.run(arr, 1, value[i]);
            assertEquals(value[i], arr[1]);
        }
    }

    /**
     * @title put reference into array
     */
    public void testN3() {
        T_aput_object_2 t = new T_aput_object_2();
        Integer[] arr = new Integer[2];
        Integer value = new Integer(12345);
        t.run(arr, 0, value);
        assertEquals(value, arr[0]);
    }

    /**
     * @title Check assignement compatibility rules
     */
    public void testN4() {
        T_aput_object_3 t = new T_aput_object_3();
        assertEquals(3, t.run());

    }

    /**
     * @title expected ArrayIndexOutOfBoundsException
     */
    public void testE1() {
        loadAndRun("dot.junit.opcodes.aput_object.d.T_aput_object_1",
                   ArrayIndexOutOfBoundsException.class, new String[2], 2, "abc");
    }

    /**
     * @title expected ArrayIndexOutOfBoundsException (negative index)
     */
    public void testE2() {
        loadAndRun("dot.junit.opcodes.aput_object.d.T_aput_object_1",
                   ArrayIndexOutOfBoundsException.class, new String[2], -1, "abc");

    }

    /**
     * @title expected NullPointerException
     */
    public void testE3() {
        loadAndRun("dot.junit.opcodes.aput_object.d.T_aput_object_1", NullPointerException.class,
                   null, 0, "abc");
    }

    /**
     * @title expected ArrayStoreException
     */
    public void testE4() {
        loadAndRun("dot.junit.opcodes.aput_object.d.T_aput_object_4", ArrayStoreException.class,
                   new String[2], 0, new Integer(1));
    }


    /**
     * @constraint B1 
     * @title types of arguments - array, double, String
     */
    public void testVFE1() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_5", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - array, int, long
     */
    public void testVFE2() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_6", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - object, int, String
     */
    public void testVFE3() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_7", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - float[], int, String
     */
    public void testVFE4() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_8", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long[], int, String
     */
    public void testVFE5() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_9", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - array, reference, String
     */
    public void testVFE6() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_10", VerifyError.class);
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE7() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_11", VerifyError.class);
    }
    
    /**
     * @constraint B15 
     * @title put integer into array of references
     */
    public void testVFE8() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_13", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Type of index argument - float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE9() {
        load("dot.junit.opcodes.aput_object.d.T_aput_object_12", VerifyError.class);
    }
}
