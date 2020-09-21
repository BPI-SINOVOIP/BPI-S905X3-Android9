package junitparams.internal;

import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.TestClass;

import junitparams.JUnitParamsRunner;
import junitparams.Parameters;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.Assert.*;

@RunWith(JUnitParamsRunner.class)
public class TestMethodTest {
    TestMethod plainTestMethod;
    TestMethod arrayTestMethod;

    @Before
    public void setUp() throws Exception {
        plainTestMethod = new TestMethod(new FrameworkMethod(TestMethodTest.class.getMethod("forOthersToWork", new Class[]{String.class})),
                new TestClass(this.getClass()));
        arrayTestMethod = new TestMethod(new FrameworkMethod(TestMethodTest.class.getMethod("forOthersToWorkWithArray",
                new Class[]{(new String[]{}).getClass()})),
                new TestClass(this.getClass()));
    }

    @Test
    @Parameters({"a","b"})
    public void forOthersToWork(String arg) throws Exception {
        assertThat(arg).isIn("a","b");
    }


    @Test
    @Parameters({"a,b","b,a"})
    public void forOthersToWorkWithArray(String... arg) throws Exception {
        assertThat(arg).containsOnlyOnce("a","b");
    }

    @Test
    @Ignore
    public void flatTestMethodStructure() throws Exception {
        System.setProperty("JUnitParams.flat", "true");

        Description description = plainTestMethod.describableFrameworkMethod().getDescription();

        assertEquals("for_others_to_work(junitparams.internal.TestMethodTest)", description.getDisplayName());
        assertTrue(description.getChildren().isEmpty());
        System.clearProperty("JUnitParams.flat");
    }


    // Android-changed: CTS and AndroidJUnitRunner rely on specific format to test names, changing
    // them will prevent CTS and AndroidJUnitRunner from working properly; see b/36541809
    @Ignore
    @Test
    public void hierarchicalTestMethodStructure() throws Exception {
        System.clearProperty("JUnitParams.flat");
        Description description = plainTestMethod.describableFrameworkMethod().getDescription();

        assertEquals("forOthersToWork", description.getDisplayName());
        assertEquals("[0] a (forOthersToWork)(junitparams.internal.TestMethodTest)", description.getChildren().get(0).getDisplayName());
        assertEquals("[1] b (forOthersToWork)(junitparams.internal.TestMethodTest)", description.getChildren().get(1).getDisplayName());
    }

    // Android-changed: CTS and AndroidJUnitRunner rely on specific format to test names, changing
    // them will prevent CTS and AndroidJUnitRunner from working properly; see b/36541809
    @Ignore
    @Test
    public void hierarchicalArrayTestMethodStructure() throws Exception {
        System.clearProperty("JUnitParams.flat");
        Description description = arrayTestMethod.describableFrameworkMethod().getDescription();

        assertEquals("forOthersToWorkWithArray", description.getDisplayName());
        assertEquals("[0] a,b (forOthersToWorkWithArray)(junitparams.internal.TestMethodTest)",
                description.getChildren().get(0).getDisplayName());
        assertEquals("[1] b,a (forOthersToWorkWithArray)(junitparams.internal.TestMethodTest)",
                description.getChildren().get(1).getDisplayName());
    }
    
    @Test
    @Parameters
    public void testVarargs(String... strings){
    	assertArrayEquals("Hello world".split(" "), strings);
    }
    
    protected Object[] parametersForTestVarargs(){
        return new Object[]{
                new Object[]{"Hello", "world"}
    	};
    }
    
    @Test
    @Parameters
    public void testVarargsCustomClass(Pair... pairs){
		assertEquals(pairs[0].x, pairs[0].y);
		assertEquals(pairs[1].x, pairs[1].y);
		assertNotEquals(pairs[2].x, pairs[2].y);
    }
    
    protected Object[] parametersForTestVarargsCustomClass(){
        return new Object[]{
                new Object[]{new Pair(0, 0), new Pair(1, 1), new Pair(2, 3)}
    	};
    }
    
    @Test
    @Parameters
    public void testVarargsMoreArgs(int sumOfX, int sumOfY, Pair... pairs){
        int sumOfXFromPairs = 0;
        int sumOfYFromPairs = 0;
        for (Pair pair : pairs) {
            sumOfXFromPairs += pair.x;
            sumOfYFromPairs += pair.y;
        }
        assertEquals(sumOfX, sumOfXFromPairs);
        assertEquals(sumOfY, sumOfYFromPairs);
    }
    
    protected Object parametersForTestVarargsMoreArgs(){
        return new Object[]{new Object[]{40, 50, new Pair(17, 21), new Pair(12, 18), new Pair(11, 11)}, new Object[]{10, 20, new Pair(3,
                15), new Pair(7, 5)}};
    }

    @Test
    @Parameters
    public void testVargsMoreArgsOfTheSameType(Pair sum, Pair... pairs) {
        assertEquals(sum.x, pairs[0].x + pairs[1].x);
        assertEquals(sum.y, pairs[0].y + pairs[1].y);
    }

    protected Object parametersForTestVargsMoreArgsOfTheSameType(){
        return new Object[]{new Object[]{new Pair(10, 30), new Pair(7, 17), new Pair(3, 13)}, new Object[]{new Pair(20, 40), new Pair(18,
                21), new Pair(2, 19)}};
    }

    @Test
    @Parameters(method = "nullArray")
    public void varargsCheckPassesWithNullArray(boolean isNull, String... array) throws Exception {
        assertEquals(isNull, array == null);
    }

    private Object nullArray() {
        return new Object[] {
                new Object[] { false, new String[] { "1", "2" } },
                new Object[] { true, null },
        };
    }

    private class Pair{
    	int x,y;
    	public Pair(int x, int y){
    		this.x = x;
    		this.y = y;
    	}
    }
}
