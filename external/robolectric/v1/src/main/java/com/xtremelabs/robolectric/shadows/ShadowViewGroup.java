package com.xtremelabs.robolectric.shadows;

import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation.AnimationListener;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;

/**
 * Shadow for {@code ViewGroup} that simulates its implementation
 */
@SuppressWarnings({"UnusedDeclaration"})
@Implements(ViewGroup.class)
public class ShadowViewGroup extends ShadowView {
    private List<View> children = new ArrayList<View>();
    private AnimationListener animListener;
    private boolean disallowInterceptTouchEvent = false;

    @Implementation
    @Override
    public View findViewById(int id) {
        if (id == getId()) {
            return realView;
        }

        for (View child : children) {
            View found = child.findViewById(id);
            if (found != null) {
                return found;
            }
        }
        return null;
    }

    @Implementation
    @Override
    public View findViewWithTag(Object obj) {
        if (obj.equals(realView.getTag())) {
            return realView;
        }

        for (View child : children) {
            View found = child.findViewWithTag(obj);
            if (found != null) {
                return found;
            }
        }

        return null;
    }

    @Implementation
    public void addView(View child) {
        ((ViewGroup) realView).addView(child, -1);
    }

    @Implementation
    public void addView(View child, int index) {
        if (index == -1) {
            children.add(child);
        } else {
            children.add(index, child);
        }
        shadowOf(child).parent = this;
    }

    @Implementation
    public void addView(View child, int width, int height) {
        ((ViewGroup) realView).addView(child, -1);
    }

    @Implementation
    public void addView(View child, ViewGroup.LayoutParams params) {
        ((ViewGroup) realView).addView(child, -1);
    }

    @Implementation
    public void addView(View child, int index, ViewGroup.LayoutParams params) {
        ((ViewGroup) realView).addView(child, index);
    }

    @Implementation
    public int indexOfChild(View child) {
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            if (children.get(i) == child) {
                return i;
            }
        }
        return -1;
    }

    @Implementation
    public int getChildCount() {
        return children.size();
    }

    @Implementation
    public View getChildAt(int index) {
    	if( index >= children.size() ){ return null; }
        return children.get(index);
    }

    @Implementation
    public void removeAllViews() {
        for (View child : children) {
            shadowOf(child).parent = null;
        }
        children.clear();
    }

    @Implementation
    public void removeViewAt(int position) {
        shadowOf(children.remove(position)).parent = null;
    }

    @Implementation
    public void removeView(View view) {
        removeViewInLayout(view);
    }

    @Implementation
    public void removeViewInLayout(View view) {
        int index = indexOfChild(view);
        if (index >= 0) {
            removeViewAt(index);
        }
    }

    @Implementation
    public void removeViews(int start, int count) {
        removeViewsInLayout(start, count);
    }

    @Implementation
    public void removeViewsInLayout(int start, int count) {
        int lastIndex = start + count - 1;
        for (int i = lastIndex; i >= start; i--) {
            removeViewAt(i);
        }
    }

    @Override
    @Implementation
    public boolean hasFocus() {
        if (super.hasFocus()) return true;

        for (View child : children) {
            if (child.hasFocus()) return true;
        }

        return false;
    }

    @Implementation
    @Override
    public void clearFocus() {
        if (hasFocus()) {
            super.clearFocus();

            for (View child : children) {
                child.clearFocus();
            }
        }
    }

    @Implementation
    @Override
    public View findFocus() {
        if (super.hasFocus()) {
            return this.realView;
        }

        for (View child : children) {
            View focusedView = child.findFocus();
            if (focusedView != null) {
                return focusedView;
            }
        }

        return null;
    }

    /**
     * Returns a string representation of this {@code ViewGroup} by concatenating all of the strings contained in all
     * of the descendants of this {@code ViewGroup}.
     * <p/>
     * Robolectric extension.
     */
    @Override
    public String innerText() {
        String innerText = "";
        String delimiter = "";

        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            String childText = shadowOf(child).innerText();
            if (childText.length() > 0) {
                innerText += delimiter;
                delimiter = " ";
            }
            innerText += childText;
        }
        return innerText;
    }

    /**
     * Non-Android method that dumps the state of this {@code ViewGroup} to {@code System.out}
     */
    @Override
    public void dump(PrintStream out, int indent) {
        dumpFirstPart(out, indent);
        if (children.size() > 0) {
            out.println(">");

            for (View child : children) {
                shadowOf(child).dump(out, indent + 2);
            }

            dumpIndent(out, indent);
            out.println("</" + realView.getClass().getSimpleName() + ">");
        } else {
            out.println("/>");
        }
    }

    @Implementation
    public void setLayoutAnimationListener(AnimationListener listener) {
        animListener = listener;
    }

    @Implementation
    public AnimationListener getLayoutAnimationListener() {
        return animListener;
    }

    @Implementation
    public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
        disallowInterceptTouchEvent = disallowIntercept;
    }

    public boolean getDisallowInterceptTouchEvent() {
        return disallowInterceptTouchEvent;
    }
}
