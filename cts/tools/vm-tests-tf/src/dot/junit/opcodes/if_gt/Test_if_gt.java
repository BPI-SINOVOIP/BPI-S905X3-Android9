package dot.junit.opcodes.if_gt;

import dot.junit.DxTestCase;
import dot.junit.DxUtil;
import dot.junit.opcodes.if_gt.d.T_if_gt_1;
import dot.junit.opcodes.if_gt.d.T_if_gt_3;

public class Test_if_gt extends DxTestCase {

    /**
     * @title Case: 5 < 6
     */
    public void testN1() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1234, t.run(5, 6));
    }

    /**
     * @title Case: 0x0f0e0d0c = 0x0f0e0d0c
     */
    public void testN2() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1234, t.run(0x0f0e0d0c, 0x0f0e0d0c));
    }

    /**
     * @title Case: 5 > -5
     */
    public void testN3() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1, t.run(5, -5));
    }

    /**
     * @title Arguments = Integer.MAX_VALUE, Integer.MAX_VALUE
     */
    public void testB1() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1234, t.run(Integer.MAX_VALUE, Integer.MAX_VALUE));
    }

    /**
     * @title Arguments = Integer.MIN_VALUE, Integer.MAX_VALUE
     */
    public void testB2() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1234, t.run(Integer.MIN_VALUE, Integer.MAX_VALUE));
    }
    
    /**
     * @title Arguments = Integer.MAX_VALUE, Integer.MIN_VALUE
     */
    public void testB3() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1, t.run(Integer.MAX_VALUE, Integer.MIN_VALUE));
    }

    /**
     * @title Arguments = 0, Integer.MIN_VALUE
     */
    public void testB4() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1, t.run(0, Integer.MIN_VALUE));
    }

    /**
     * @title Arguments = 0, 0
     */
    public void testB5() {
        T_if_gt_1 t = new T_if_gt_1();
        assertEquals(1234, t.run(0, 0));
    }
    
    /**
     * @constraint A23 
     * @title  number of registers
     */
    public void testVFE1() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_4", VerifyError.class);
    }
    

    /**
     * @constraint B1 
     * @title  types of arguments - int, double
     */
    public void testVFE2() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_5", VerifyError.class);
    }

    /**
     * @constraint B1 
     * @title  types of arguments - long, int
     */
    public void testVFE3() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_6", VerifyError.class);
    }
    
    /**
     * @constraint B1 
     * @title  types of arguments - int, reference
     */
    public void testVFE4() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_7", VerifyError.class);
    }
    
    /**
     * @constraint A6 
     * @title  branch target shall be inside the method
     */
    public void testVFE5() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_9", VerifyError.class);
    }

    /**
     * @constraint A6 
     * @title  branch target shall not be "inside" instruction
     */
    public void testVFE6() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_10", VerifyError.class);
    }

    /**
     * @constraint n/a 
     * @title  branch target shall not be 0
     */
    public void testVFE7() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_11", VerifyError.class);
    }

    /**
     * @constraint B1
     * @title Types of arguments - int, float. The verifier checks that ints
     * and floats are not used interchangeably.
     */
    public void testVFE8() {
        load("dot.junit.opcodes.if_gt.d.T_if_gt_3", VerifyError.class);
    }

}
