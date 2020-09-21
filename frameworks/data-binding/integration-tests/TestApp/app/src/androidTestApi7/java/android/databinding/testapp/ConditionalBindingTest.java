package android.databinding.testapp;

import android.databinding.testapp.databinding.ConditionalBindingBinding;
import android.databinding.testapp.vo.ConditionalVo;
import android.databinding.testapp.vo.NotBindableVo;

import android.test.UiThreadTest;

public class ConditionalBindingTest extends BaseDataBinderTest<ConditionalBindingBinding>{

    public ConditionalBindingTest() {
        super(ConditionalBindingBinding.class);
    }

    @UiThreadTest
    public void test1() {
        initBinder();
        testCorrectness(true, true);
    }

    @UiThreadTest
    public void testTernary() throws Throwable {
        ConditionalVo obj4 = new ConditionalVo();
        initBinder();
        mBinder.setObj4(obj4);
        mBinder.executePendingBindings();
        assertEquals("hello", mBinder.textView1.getText().toString());
        obj4.setUseHello(true);
        mBinder.executePendingBindings();
        assertEquals("Hello World", mBinder.textView1.getText().toString());
    }

    @UiThreadTest
    public void testNullListener() throws Throwable {
        ConditionalVo obj4 = new ConditionalVo();
        initBinder();
        mBinder.setObj4(obj4);
        mBinder.executePendingBindings();
        mBinder.view1.callOnClick();
        assertFalse(obj4.wasClicked);
        mBinder.setCond1(true);
        mBinder.executePendingBindings();
        mBinder.view1.callOnClick();
        assertTrue(obj4.wasClicked);
    }

    private void testCorrectness(boolean cond1, boolean cond2) {
        NotBindableVo o1 = new NotBindableVo("a");
        NotBindableVo o2 = new NotBindableVo("b");
        NotBindableVo o3 = new NotBindableVo("c");
        mBinder.setObj1(o1);
        mBinder.setObj2(o2);
        mBinder.setObj3(o3);
        mBinder.setCond1(cond1);
        mBinder.setCond2(cond2);
        mBinder.executePendingBindings();
        final String text = mBinder.textView.getText().toString();
        assertEquals(cond1 && cond2, "a".equals(text));
        assertEquals(cond1 && !cond2, "b".equals(text));
        assertEquals(!cond1, "c".equals(text));
    }
}
