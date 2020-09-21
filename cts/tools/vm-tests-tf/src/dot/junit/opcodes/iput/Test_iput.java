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

package dot.junit.opcodes.iput;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.iput.d.T_iput_1;
import dot.junit.opcodes.iput.d.T_iput_10;
import dot.junit.opcodes.iput.d.T_iput_11;
import dot.junit.opcodes.iput.d.T_iput_12;
import dot.junit.opcodes.iput.d.T_iput_13;
import dot.junit.opcodes.iput.d.T_iput_14;
import dot.junit.opcodes.iput.d.T_iput_15;
import dot.junit.opcodes.iput.d.T_iput_17;
import dot.junit.opcodes.iput.d.T_iput_19;
import dot.junit.opcodes.iput.d.T_iput_5;
import dot.junit.opcodes.iput.d.T_iput_7;
import dot.junit.opcodes.iput.d.T_iput_8;
import dot.junit.opcodes.iput.d.T_iput_9;

public class Test_iput extends DxTestCase {

    /**
     * @title type - int
     */
    public void testN1() {
        T_iput_1 t = new T_iput_1();
        assertEquals(0, t.st_i1);
        t.run();
        assertEquals(1000000, t.st_i1);
    }

    /**
     * @title type - float
     */
    public void testN2() {
        T_iput_19 t = new T_iput_19();
        assertEquals(0.0f, t.st_f1);
        t.run();
        assertEquals(3.14f, t.st_f1);
    }


    /**
     * @title modification of final field
     */
    public void testN3() {
        T_iput_12 t = new T_iput_12();
        assertEquals(0, t.st_i1);
        t.run();
        assertEquals(1000000, t.st_i1);
    }

    /**
     * @title modification of protected field from subclass
     */
    public void testN4() {
        //@uses dot.junit.opcodes.iput.d.T_iput_1
        //@uses dot.junit.opcodes.iput.d.T_iput_14
        T_iput_14 t = new T_iput_14();
        assertEquals(0, t.getProtectedField());
        t.run();
        assertEquals(1000000, t.getProtectedField());
    }


    /**
     * @title expected NullPointerException
     */
    public void testE2() {
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_13", NullPointerException.class);
    }

    /**
     * @constraint A11
     * @title constant pool index
     */
    public void testVFE1() {
        load("dot.junit.opcodes.iput.d.T_iput_3", VerifyError.class);
    }

    /**
     *
     * @constraint A23
     * @title number of registers
     */
    public void testVFE2() {
        load("dot.junit.opcodes.iput.d.T_iput_4", VerifyError.class);
    }


    /**
     *
     * @constraint B14
     * @title put integer into long field - only field with same name but
     * different type exists
     */
    public void testVFE5() {
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_17", NoSuchFieldError.class);
    }

    /**
     * @constraint B1
     * @title Trying to put float into integer field. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE6() {
        load("dot.junit.opcodes.iput.d.T_iput_5", VerifyError.class);
    }

    /**
     *
     * @constraint B14
     * @title type of field doesn't match opcode - attempt to modify double field
     * with single-width register
     */
    public void testVFE7() {
        load("dot.junit.opcodes.iput.d.T_iput_18", VerifyError.class);
    }

    /**
     * @constraint A11
     * @title Attempt to set static field.
     */
    public void testVFE8() {
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_7", IncompatibleClassChangeError.class);
    }

    /**
     * @constraint B12
     * @title Attempt to modify inaccessible protected field.
     */
    public void testVFE9() {
        //@uses dot.junit.opcodes.iput.TestStubs
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_8", IllegalAccessError.class);
    }

    /**
     * @constraint n/a
     * @title Attempt to modify field of undefined class.
     */
    public void testVFE10() {
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_9", NoClassDefFoundError.class);
    }

    /**
     * @constraint n/a
     * @title Attempt to modify undefined field.
     */
    public void testVFE11() {
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_10", NoSuchFieldError.class);
    }

    /**
     * @constraint n/a
     * @title Attempt to modify superclass' private field from subclass.
     */
    public void testVFE12() {
        //@uses dot.junit.opcodes.iput.d.T_iput_1
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_15", IllegalAccessError.class);
    }

    /**
     * @constraint B1
     * @title iput shall not work for wide numbers
     */
    public void testVFE13() {
        load("dot.junit.opcodes.iput.d.T_iput_2", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title iput shall not work for reference fields
     */
    public void testVFE14() {
        load("dot.junit.opcodes.iput.d.T_iput_20", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title iput shall not work for short fields
     */
    public void testVFE15() {
        load("dot.junit.opcodes.iput.d.T_iput_21", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title iput shall not work for boolean fields
     */
    public void testVFE16() {
        load("dot.junit.opcodes.iput.d.T_iput_22", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title iput shall not work for char fields
     */
    public void testVFE17() {
        load("dot.junit.opcodes.iput.d.T_iput_23", VerifyError.class);
    }

    /**
     *
     * @constraint B1
     * @title iput shall not work for byte fields
     */
    public void testVFE18() {
        load("dot.junit.opcodes.iput.d.T_iput_24", VerifyError.class);
    }


    /**
     * @constraint B6
     * @title instance fields may only be accessed on already initialized instances.
     */
    public void testVFE30() {
        load("dot.junit.opcodes.iput.d.T_iput_30", VerifyError.class);
    }

    /**
     * @constraint N/A
     * @title instance fields may only be accessed on reference registers
     */
    public void testVFE31() {
        load("dot.junit.opcodes.iput.d.T_iput_31", VerifyError.class);
    }

    /**
     * @constraint n/a
     * @title Modification of final field in other class
     */
    public void testE5() {
        //@uses dot.junit.opcodes.iput.TestStubs
        loadAndRun("dot.junit.opcodes.iput.d.T_iput_11", IllegalAccessError.class);
    }
}
