package com.xtremelabs.robolectric.shadows;

import android.app.Activity;
import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.view.*;
import android.view.View.MeasureSpec;
import android.view.animation.Animation;
import android.widget.LinearLayout;
import com.xtremelabs.robolectric.R;
import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.WithTestDefaultsRunner;
import com.xtremelabs.robolectric.util.*;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;
import static junit.framework.Assert.assertEquals;
import static org.hamcrest.CoreMatchers.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.junit.Assert.*;

@RunWith(WithTestDefaultsRunner.class)
public class ViewTest {
    private View view;

    @Before
    public void setUp() throws Exception {
        view = new View(new Activity());
    }

    @Test
    public void testHasEmptyLayoutParams() throws Exception {
        ViewGroup.LayoutParams layoutParams = view.getLayoutParams();
        assertThat(layoutParams, notNullValue());
    }

    @Test
    public void layout_shouldAffectWidthAndHeight() throws Exception {
        assertThat(view.getWidth(), equalTo(0));
        assertThat(view.getHeight(), equalTo(0));

        view.layout(100, 200, 303, 404);
        assertThat(view.getWidth(), equalTo(303 - 100));
        assertThat(view.getHeight(), equalTo(404 - 200));
    }

    @Test
    public void shouldFocus() throws Exception {
        final Transcript transcript = new Transcript();

        view.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                transcript.add(hasFocus ? "Gained focus" : "Lost focus");
            }
        });

        assertFalse(view.isFocused());
        assertFalse(view.hasFocus());
        transcript.assertNoEventsSoFar();

        view.requestFocus();
        assertTrue(view.isFocused());
        assertTrue(view.hasFocus());
        transcript.assertEventsSoFar("Gained focus");

        view.clearFocus();
        assertFalse(view.isFocused());
        assertFalse(view.hasFocus());
        transcript.assertEventsSoFar("Lost focus");
    }

    @Test
    public void shouldNotBeFocusableByDefault() throws Exception {
        assertFalse(view.isFocusable());

        view.setFocusable(true);
        assertTrue(view.isFocusable());
    }

    @Test
    public void shouldKnowIfThisOrAncestorsAreVisible() throws Exception {
        assertTrue(shadowOf(view).derivedIsVisible());

        ViewGroup grandParent = new LinearLayout(null);
        ViewGroup parent = new LinearLayout(null);
        grandParent.addView(parent);
        parent.addView(view);

        grandParent.setVisibility(View.GONE);

        assertFalse(shadowOf(view).derivedIsVisible());
    }

    @Test
    public void shouldInflateMergeRootedLayoutAndNotCreateReferentialLoops() throws Exception {
        LinearLayout root = new LinearLayout(null);
        root.inflate(new Activity(), R.layout.inner_merge, root);
        for (int i = 0; i < root.getChildCount(); i++) {
            View child = root.getChildAt(i);
            assertNotSame(root, child);
        }
    }

    @Test
    public void performLongClick_shouldClickOnView() throws Exception {
        TestOnLongClickListener clickListener = new TestOnLongClickListener();
        view.setOnLongClickListener(clickListener);
        shadowOf(view).performLongClick();

        assertTrue(clickListener.clicked);
    }

    @Test
    public void checkedClick_shouldClickOnView() throws Exception {
        TestOnClickListener clickListener = new TestOnClickListener();
        view.setOnClickListener(clickListener);
        shadowOf(view).checkedPerformClick();

        assertTrue(clickListener.clicked);
    }

    @Test(expected = RuntimeException.class)
    public void checkedClick_shouldThrowIfViewIsNotVisible() throws Exception {
        ViewGroup grandParent = new LinearLayout(null);
        ViewGroup parent = new LinearLayout(null);
        grandParent.addView(parent);
        parent.addView(view);
        grandParent.setVisibility(View.GONE);

        shadowOf(view).checkedPerformClick();
    }

    @Test(expected = RuntimeException.class)
    public void checkedClick_shouldThrowIfViewIsDisabled() throws Exception {
        view.setEnabled(false);
        shadowOf(view).checkedPerformClick();
    }

    @Test
    public void getBackground_shouldReturnNullIfNoBackgroundHasBeenSet() throws Exception {
        assertThat(view.getBackground(), nullValue());
    }

    @Test
    public void shouldSetBackgroundColor() {
        view.setBackgroundColor(R.color.android_red);
        int intColor = view.getResources().getColor(R.color.android_red);

        assertThat((ColorDrawable) view.getBackground(), equalTo(new ColorDrawable(intColor)));
    }

    @Test
    public void shouldSetBackgroundResource() throws Exception {
        view.setBackgroundResource(R.drawable.an_image);
        assertThat(view.getBackground(), equalTo(view.getResources().getDrawable(R.drawable.an_image)));
    }

    @Test
    public void shouldRecordBackgroundColor() {
        int[] colors = {0, 1, 727};

        for (int color : colors) {
            view.setBackgroundColor(color);
            assertThat(shadowOf(view).getBackgroundColor(), equalTo(color));
        }
    }

    @Test
    public void shouldPostActionsToTheMessageQueue() throws Exception {
        Robolectric.pauseMainLooper();

        TestRunnable runnable = new TestRunnable();
        view.post(runnable);
        assertFalse(runnable.wasRun);

        Robolectric.unPauseMainLooper();
        assertTrue(runnable.wasRun);
    }

    @Test
    public void shouldPostInvalidateDelayed() throws Exception {
        Robolectric.pauseMainLooper();

        view.postInvalidateDelayed(100);
        ShadowView shadowView = shadowOf(view);
        assertFalse(shadowView.wasInvalidated());

        Robolectric.unPauseMainLooper();
        assertTrue(shadowView.wasInvalidated());
    }

    @Test
    public void shouldPostActionsToTheMessageQueueWithDelay() throws Exception {
        Robolectric.pauseMainLooper();

        TestRunnable runnable = new TestRunnable();
        view.postDelayed(runnable, 1);
        assertFalse(runnable.wasRun);

        Robolectric.getUiThreadScheduler().advanceBy(1);
        assertTrue(runnable.wasRun);
    }

    @Test
    public void shouldSupportAllConstructors() throws Exception {
        new View(null);
        new View(null, null);
        new View(null, null, 0);
    }

    @Test
    public void shouldSetAnimation() throws Exception {
        Animation anim = new TestAnimation();
        view.setAnimation(anim);
        assertThat(view.getAnimation(), sameInstance(anim));
    }

    @Test
    public void shouldStartAndClearAnimation() throws Exception {
        Animation anim = new TestAnimation();
        TestAnimationListener listener = new TestAnimationListener();
        anim.setAnimationListener(listener);
        assertThat(listener.wasStartCalled, equalTo(false));
        assertThat(listener.wasRepeatCalled, equalTo(false));
        assertThat(listener.wasEndCalled, equalTo(false));
        view.startAnimation(anim);
        assertThat(listener.wasStartCalled, equalTo(true));
        assertThat(listener.wasRepeatCalled, equalTo(false));
        assertThat(listener.wasEndCalled, equalTo(false));
        view.clearAnimation();
        assertThat(listener.wasStartCalled, equalTo(true));
        assertThat(listener.wasRepeatCalled, equalTo(false));
        assertThat(listener.wasEndCalled, equalTo(true));
    }

    @Test
    public void shouldFindViewWithTag() {
        view.setTag("tagged");
        assertThat(view.findViewWithTag("tagged"), sameInstance(view));
    }

    @Test
    public void shouldFindViewWithTag_whenViewOverridesGetTag() throws Exception {
        View view = new View(Robolectric.application) {
            @Override
            public Object getTag() {
                return "blarg";
            }
        };
        assertThat(view.findViewWithTag("blarg"), sameInstance(view));
    }

    @Test
    public void scrollTo_shouldStoreTheScrolledCoordinates() throws Exception {
        view.scrollTo(1, 2);
        assertThat(shadowOf(view).scrollToCoordinates, equalTo(new Point(1, 2)));
    }

    @Test
    public void shouldScrollTo() throws Exception {
        view.scrollTo(7, 6);

        assertEquals(7, view.getScrollX());
        assertEquals(6, view.getScrollY());
    }

    @Test
    public void shouldGetScrollXAndY() {
        assertEquals(0, view.getScrollX());
        assertEquals(0, view.getScrollY());
    }

    @Test
    public void getViewTreeObserver_shouldReturnTheSameObserverFromMultipleCalls() throws Exception {
        ViewTreeObserver observer = view.getViewTreeObserver();
        assertThat(observer, instanceOf(ViewTreeObserver.class));
        assertThat(view.getViewTreeObserver(), sameInstance(observer));
    }

    @Test
    public void dispatchTouchEvent_sendsMotionEventToOnTouchEvent() throws Exception {
        TouchableView touchableView = new TouchableView(null);
        MotionEvent event = MotionEvent.obtain(0L, 0L, MotionEvent.ACTION_DOWN, 12f, 34f, 0);
        touchableView.dispatchTouchEvent(event);
        assertThat(touchableView.event, sameInstance(event));
        view.dispatchTouchEvent(event);
        assertThat(shadowOf(view).getLastTouchEvent(), sameInstance(event));
    }

    @Test
    public void dispatchTouchEvent_listensToTrueFromListener() throws Exception {
        view.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                return true;
            }
        });
        MotionEvent event = MotionEvent.obtain(0L, 0L, MotionEvent.ACTION_DOWN, 12f, 34f, 0);
        view.dispatchTouchEvent(event);
        assertThat(shadowOf(view).getLastTouchEvent(), nullValue());
    }

    @Test
    public void dispatchTouchEvent_listensToFalseFromListener() throws Exception {
        view.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                return false;
            }
        });
        MotionEvent event = MotionEvent.obtain(0L, 0L, MotionEvent.ACTION_DOWN, 12f, 34f, 0);
        view.dispatchTouchEvent(event);
        assertThat(shadowOf(view).getLastTouchEvent(), sameInstance(event));
    }

    @Test
    public void dispatchFocusEvent_sendsFocusToOnFocusChanged() throws Exception {
        FocusableView focusableView = new FocusableView(null);
        focusableView.requestFocus();
        assertThat(focusableView.onFocusChangedWasCalled, equalTo(true));
    }

    @Test
    public void test_nextFocusDownId() throws Exception {
        assertEquals(View.NO_ID, view.getNextFocusDownId());

        view.setNextFocusDownId(R.id.icon);
        assertEquals(R.id.icon, view.getNextFocusDownId());
    }

    @Test
    public void dispatchOnAnimationEnd() throws Exception {
        TestView view1 = new TestView(new Activity());
        assertFalse(view1.onAnimationEndWasCalled);
        shadowOf(view1).finishedAnimation();
        assertTrue(view1.onAnimationEndWasCalled);
    }
    
    @Test
    public void test_measuredDimension() {
    	// View does not provide its own onMeasure implementation
    	TestView view1 = new TestView(new Activity());

    	assertThat(view1.getHeight(), equalTo(0));
    	assertThat(view1.getWidth(), equalTo(0)); 
    	assertThat(view1.getMeasuredHeight(), equalTo(0));
    	assertThat(view1.getMeasuredWidth(), equalTo(0));    
    	
    	view1.measure( MeasureSpec.makeMeasureSpec(150, MeasureSpec.AT_MOST),
    				   MeasureSpec.makeMeasureSpec(300, MeasureSpec.AT_MOST) );
    	
    	assertThat(view1.getHeight(), equalTo(0));
    	assertThat(view1.getWidth(), equalTo(0)); 
    	assertThat(view1.getMeasuredHeight(), equalTo(300));
    	assertThat(view1.getMeasuredWidth(), equalTo(150));  
    }
    
    @Test
    public void test_measuredDimensionCustomView() {
       	// View provides its own onMeasure implementation
    	TestView2 view2 = new TestView2(new Activity());

    	assertThat(view2.getHeight(), equalTo(0));
    	assertThat(view2.getWidth(), equalTo(0)); 
    	assertThat(view2.getMeasuredHeight(), equalTo(0));
    	assertThat(view2.getMeasuredWidth(), equalTo(0));    
    	
    	view2.measure( MeasureSpec.makeMeasureSpec(1000, MeasureSpec.AT_MOST),
    				   MeasureSpec.makeMeasureSpec(600, MeasureSpec.AT_MOST) );
    	
    	assertThat(view2.getHeight(), equalTo(0));
    	assertThat(view2.getWidth(), equalTo(0)); 
    	assertThat(view2.getMeasuredHeight(), equalTo(400));
    	assertThat(view2.getMeasuredWidth(), equalTo(800));  
    }

    @Test
    public void testFilterTouchesWhenObscured() {
        assertFalse(view.getFilterTouchesWhenObscured());

        view.setFilterTouchesWhenObscured(true);
        assertTrue(view.getFilterTouchesWhenObscured());

        view.setFilterTouchesWhenObscured(false);
        assertFalse(view.getFilterTouchesWhenObscured());
    }

    @Test
    public void testFilterTouchesWhenObscuredWhenLoadedFromXml() {
        LinearLayout root = new LinearLayout(null);
        ShadowView.inflate(new Activity(), R.layout.views, root);

        View defaultView = root.findViewById(R.id.default_view);
        assertFalse(defaultView.getFilterTouchesWhenObscured());

        View filterFalseView = root.findViewById(R.id.filter_touches_false_view);
        assertFalse(filterFalseView.getFilterTouchesWhenObscured());

        View filterTrueView = root.findViewById(R.id.filter_touches_true_view);
        assertTrue(filterTrueView.getFilterTouchesWhenObscured());
    }

    @Test
    public void testClickable() {
        assertFalse(view.isClickable());

        view.setClickable(true);
        assertTrue(view.isClickable());

        view.setClickable(false);
        assertFalse(view.isClickable());
    }

    @Test
    public void testClickableWhenLoadedFromXml() {
        LinearLayout root = new LinearLayout(null);
        ShadowView.inflate(new Activity(), R.layout.views, root);

        View defaultView = root.findViewById(R.id.default_view);
        assertFalse(defaultView.isClickable());

        View clickableFalseView = root.findViewById(R.id.clickable_false_view);
        assertFalse(clickableFalseView.isClickable());

        View clickableTrueView = root.findViewById(R.id.clickable_true_view);
        assertTrue(clickableTrueView.isClickable());
    }

    @Test
    public void testFocusable() {
        assertFalse(view.isFocusable());

        view.setFocusable(true);
        assertTrue(view.isFocusable());

        view.setFocusable(false);
        assertFalse(view.isFocusable());
    }

    @Test
    public void testFocusableWhenLoadedFromXml() {
        LinearLayout root = new LinearLayout(null);
        ShadowView.inflate(new Activity(), R.layout.views, root);

        View defaultView = root.findViewById(R.id.default_view);
        assertFalse(defaultView.isFocusable());

        View focusableFalseView = root.findViewById(R.id.focusable_false_view);
        assertFalse(focusableFalseView.isFocusable());

        View focusableTrueView = root.findViewById(R.id.focusable_true_view);
        assertTrue(focusableTrueView.isFocusable());
    }

    private static class TestAnimation extends Animation {
    }

    private static class TouchableView extends View {
        MotionEvent event;

        public TouchableView(Context context) {
            super(context);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            this.event = event;
            return false;
        }
    }

    private static class TestView extends View {
        boolean onAnimationEndWasCalled;

        public TestView(Context context) {
            super(context);
        }

        @Override
        protected void onAnimationEnd() {
            super.onAnimationEnd();
            onAnimationEndWasCalled = true;
        }
    }

    private static class FocusableView extends View {
      boolean onFocusChangedWasCalled;

      public FocusableView(Context context) {
        super(context);
      }

      @Override
      protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);

        onFocusChangedWasCalled = true;
      }
    }
    
    private static class TestView2 extends View {
    	public TestView2(Context context) {
            super(context);
        }

		@Override
		protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
			super.onMeasure(800, 400);
		}
    }
}
