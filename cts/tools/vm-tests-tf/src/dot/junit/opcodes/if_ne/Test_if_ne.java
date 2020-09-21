package dot.junit.opcodes.if_ne;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.if_ne.d.T_if_ne_1;
import dot.junit.opcodes.if_ne.d.T_if_ne_2;
import dot.junit.opcodes.if_ne.d.T_if_ne_4;

public class Test_if_ne extends DxTestCase {

    /**
     * @title Arguments = 5, 6
     */
    public void testN1() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1, t.run(5, 6));
    }

    /**
     * @title Arguments = 0x0f0e0d0c, 0x0f0e0d0c
     */
    public void testN2() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1234, t.run(0x0f0e0d0c, 0x0f0e0d0c));
    }

    /**
     * @title Arguments = 5, -5
     */
    public void testN3() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1, t.run(5, -5));
    }

    /**
     * @title Arguments = 0x01001234, 0x1234
     */
    public void testN4() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1, t.run(0x01001234, 0x1234));
    }
    
    /**
     * @title compare references
     */
    public void testN5() {
        T_if_ne_2 t = new T_if_ne_2();
        String a = "a";
        String b = "b";
        assertEquals(1, t.run(a, b));
    }

    /**
     * @title compare references
     */
    public void testN6() {
        T_if_ne_2 t = new T_if_ne_2();
        String a = "a";
        assertEquals(1234, t.run(a, a));
    }

    /**
     * @title Arguments = Integer.MAX_VALUE, Integer.MAX_VALUE
     */
    public void testB1() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1234, t.run(Integer.MAX_VALUE, Integer.MAX_VALUE));
    }

    /**
     * @title Arguments = Integer.MIN_VALUE, Integer.MIN_VALUE
     */
    public void testB2() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1234, t.run(Integer.MIN_VALUE, Integer.MIN_VALUE));
    }

    /**
     * @title Arguments = 0, 1234567
     */
    public void testB3() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1, t.run(0, 1234567));
    }

    /**
     * @title Arguments = 0, 0
     */
    public void testB4() {
        T_if_ne_1 t = new T_if_ne_1();
        assertEquals(1234, t.run(0, 0));
    }
    
    /**
     * @title Compare with null
     */
    public void testB5() {
        T_if_ne_2 t = new T_if_ne_2();
        String a = "a";
        assertEquals(1, t.run(null, a));
    }
    
    /**
     * @constraint A23 
     * @title number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_5", VerifyError.class);
    }



    /**
     * @constraint B1 
     * @title types of arguments - int, double
     */
    public void testVFE2() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_7", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title types of arguments - long, int
     */
    public void testVFE3() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_8", VerifyError.class);
    }
    
    /**
     * @constraint B1 
     * @title  types of arguments - int, reference
     */
    public void testVFE4() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_9", VerifyError.class);
    }

    /**
     * @constraint A6 
     * @title  branch target shall be inside the method
     */
    public void testVFE5() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_10", VerifyError.class);
    }

    /**
     * @constraint A6 
     * @title branch target shall not be "inside" instruction
     */
    public void testVFE6() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_11", VerifyError.class);
    }

    /**
     * @constraint n/a 
     * @title branch target shall not be 0
     */
    public void testVFE7() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_12", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE8() {
        load("dot.junit.opcodes.if_ne.d.T_if_ne_4", VerifyError.class);
    }

}
