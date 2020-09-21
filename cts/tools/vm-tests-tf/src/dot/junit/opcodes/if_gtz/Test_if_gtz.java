package dot.junit.opcodes.if_gtz;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.if_gtz.d.T_if_gtz_1;
import dot.junit.opcodes.if_gtz.d.T_if_gtz_2;

public class Test_if_gtz extends DxTestCase {

    /**
     * @title Argument = 5
     */
    public void testN1() {
        T_if_gtz_1 t = new T_if_gtz_1();
        assertEquals(1, t.run(5));
    }

    /**
     * @title Argument = -5
     */
    public void testN2() {
        T_if_gtz_1 t = new T_if_gtz_1();
        assertEquals(1234, t.run(-5));
    }

    /**
     * @title Arguments = Integer.MAX_VALUE
     */
    public void testB1() {
        T_if_gtz_1 t = new T_if_gtz_1();
        assertEquals(1, t.run(Integer.MAX_VALUE));
    }

    /**
     * @title Arguments = Integer.MIN_VALUE
     */
    public void testB2() {
        T_if_gtz_1 t = new T_if_gtz_1();
        assertEquals(1234, t.run(Integer.MIN_VALUE));
    }
    
    /**
     * @title Arguments = 0
     */
    public void testB3() {
        T_if_gtz_1 t = new T_if_gtz_1();
        assertEquals(1234, t.run(0));
    }
    
    /**
     * @constraint A23 
     * @title  number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_3", VerifyError.class);
    }


    /**
     * @constraint B1 
     * @title  types of arguments - double
     */
    public void testVFE2() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_4", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title  types of arguments - long
     */
    public void testVFE3() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_5", VerifyError.class);
    }
    
    /**
     * @constraint B1 
     * @title  types of arguments - reference
     */
    public void testVFE4() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_6", VerifyError.class);
    }
    
    /**
     * @constraint A6 
     * @title  branch target shall be inside the method
     */
    public void testVFE5() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_8", VerifyError.class);
    }

    /**
     * @constraint A6 
     * @title  branch target shall not be "inside" instruction
     */
    public void testVFE6() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_9", VerifyError.class);
    }
    
    /**
     * @constraint n/a
     * @title  branch must not be 0
     */
    public void testVFE7() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_10", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Type of argument - float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE8() {
        load("dot.junit.opcodes.if_gtz.d.T_if_gtz_2", VerifyError.class);
    }

}
