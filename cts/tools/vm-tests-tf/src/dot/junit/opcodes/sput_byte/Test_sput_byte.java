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

package dot.junit.opcodes.sput_byte;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_1;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_10;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_11;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_12;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_13;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_14;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_15;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_17;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_7;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_8;
import dot.junit.opcodes.sput_byte.d.T_sput_byte_9;

public class Test_sput_byte extends DxTestCase {
    /**
     * @title put byte into static field
     */
    public void testN1() {
        T_sput_byte_1 t = new T_sput_byte_1();
        assertEquals(0, T_sput_byte_1.st_i1);
        t.run();
        assertEquals(77, T_sput_byte_1.st_i1);
    }


    /**
     * @title modification of final field
     */
    public void testN2() {
        T_sput_byte_12 t = new T_sput_byte_12();
        assertEquals(0, T_sput_byte_12.st_i1);
        t.run();
        assertEquals(77, T_sput_byte_12.st_i1);
    }

    /**
     * @title modification of protected field from subclass
     */
    public void testN4() {
        //@uses dot.junit.opcodes.sput_byte.d.T_sput_byte_1
        //@uses dot.junit.opcodes.sput_byte.d.T_sput_byte_14
        T_sput_byte_14 t = new T_sput_byte_14();
        assertEquals(0, T_sput_byte_14.getProtectedField());
        t.run();
        assertEquals(77, T_sput_byte_14.getProtectedField());
    }


    /**
     * @title initialization of referenced class throws exception
     */
    public void testE6() {
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_13",
                   ExceptionInInitializerError.class);
    }

    /**
     * @constraint A12
     * @title constant pool index
     */
    public void testVFE1() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_3", VerifyError.class);
    }

    /**
     *
     * @constraint A23
     * @title number of registers
     */
    public void testVFE2() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_4", VerifyError.class);
    }


    /**
     *
     * @constraint B13
     * @title put byte into long field - only field with same name but
     * different type exists
     */
    public void testVFE5() {
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_17", NoSuchFieldError.class);
    }

    /**
     *
     * @constraint B13
     * @title type of field doesn't match opcode - attempt to modify double
     * field with single-width register
     */
    public void testVFE7() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_18", VerifyError.class);
    }

    /**
     *
     * @constraint A12
     * @title Attempt to set non-static field.
     */
    public void testVFE8() {
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_7",
                   IncompatibleClassChangeError.class);
    }

    /**
     * @constraint n/a
     * @title Attempt to modify inaccessible field.
     */
    public void testVFE9() {
        //@uses dot.junit.opcodes.sput_byte.TestStubs
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_8", IllegalAccessError.class);
    }

    /**
     * @constraint n/a
     * @title Attempt to modify field of undefined class.
     */
    public void testVFE10() {
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_9", NoClassDefFoundError.class);
    }

    /**
     * @constraint n/a
     * @title Attempt to modify undefined field.
     */
    public void testVFE11() {
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_10", NoSuchFieldError.class);
    }



    /**
     * @constraint n/a
     * @title Attempt to modify superclass' private field from subclass.
     */
    public void testVFE12() {
        //@uses dot.junit.opcodes.sput_byte.d.T_sput_byte_1
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_15", IllegalAccessError.class);
    }


    /**
     * @constraint B1
     * @title sput-byte shall not work for wide numbers
     */
    public void testVFE13() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_2", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title sput-byte shall not work for reference fields
     */
    public void testVFE14() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_20", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title sput-byte shall not work for short fields
     */
    public void testVFE15() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_21", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title sput-byte shall not work for int fields
     */
    public void testVFE16() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_22", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title sput-byte shall not work for char fields
     */
    public void testVFE17() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_23", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title sput-byte shall not work for boolean fields
     */
    public void testVFE18() {
        load("dot.junit.opcodes.sput_byte.d.T_sput_byte_24", VerifyError.class);
    }

    /**
     * @constraint n/a
     * @title Modification of final field in other class
     */
    public void testVFE19() {
        //@uses dot.junit.opcodes.sput_byte.TestStubs
        loadAndRun("dot.junit.opcodes.sput_byte.d.T_sput_byte_11", IllegalAccessError.class);
    }

}
