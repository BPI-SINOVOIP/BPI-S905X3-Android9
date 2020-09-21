package com.xtremelabs.robolectric.shadows;

import static com.xtremelabs.robolectric.Robolectric.shadowOf;
import static com.xtremelabs.robolectric.Robolectric.Reflection.newInstanceOf;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.ViewTreeObserver;
import android.view.animation.Animation;

import com.xtremelabs.robolectric.Robolectric;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;
import com.xtremelabs.robolectric.internal.RealObject;

import java.io.PrintStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

/**
 * Shadow implementation of {@code View} that simulates the behavior of this
 * class.
 * <p/>
 * Supports listeners, focusability (but not focus order), resource loading,
 * visibility, onclick, tags, and tracks the size and shape of the view.
 */
@SuppressWarnings({"UnusedDeclaration"})
@Implements(View.class)
public class ShadowView {
    @RealObject
    protected View realView;

    private int id;
    ShadowView parent;
    protected Context context;
    private boolean selected;
    private View.OnClickListener onClickListener;
    private View.OnLongClickListener onLongClickListener;
    private Object tag;
    private boolean enabled = true;
    private int visibility = View.VISIBLE;
    private boolean filterTouchesWhenObscured = false;
    int left;
    int top;
    int right;
    int bottom;
    private int paddingLeft;
    private int paddingTop;
    private int paddingRight;
    private int paddingBottom;
    private ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams(0, 0);
    private final Map<Integer, Object> tags = new HashMap<Integer, Object>();
    private boolean clickable;
    protected boolean focusable;
    boolean focusableInTouchMode;
    private int backgroundResourceId = -1;
    private int backgroundColor;
    protected View.OnKeyListener onKeyListener;
    private boolean isFocused;
    private View.OnFocusChangeListener onFocusChangeListener;
    private boolean wasInvalidated;
    private View.OnTouchListener onTouchListener;
    protected AttributeSet attributeSet;
    private boolean drawingCacheEnabled;
    public Point scrollToCoordinates;
    private boolean didRequestLayout;
    private Drawable background;
    private Animation animation;
    private ViewTreeObserver viewTreeObserver;
    private MotionEvent lastTouchEvent;
    private int nextFocusDownId = View.NO_ID;
    private CharSequence contentDescription = null;
    private int measuredWidth = 0;
    private int measuredHeight = 0;

    public void __constructor__(Context context) {
        __constructor__(context, null);
    }

    public void __constructor__(Context context, AttributeSet attributeSet) {
        __constructor__(context, attributeSet, 0);
    }

    public void __constructor__(Context context, AttributeSet attributeSet, int defStyle) {
        this.context = context;
        this.attributeSet = attributeSet;

        if (attributeSet != null) {
            applyAttributes();
        }
    }

    public void applyAttributes() {
        applyIdAttribute();
        applyVisibilityAttribute();
        applyFilterTouchesWhenObscuredAttribute();
        applyClickableAttribute();
        applyFocusableAttribute();
        applyEnabledAttribute();
        applyBackgroundAttribute();
        applyTagAttribute();
        applyOnClickAttribute();
        applyContentDescriptionAttribute();
    }

    @Implementation
    public void setId(int id) {
        this.id = id;
    }

    @Implementation
    public void setClickable(boolean clickable) {
        this.clickable = clickable;
    }

    /**
     * Also sets focusable in touch mode to false if {@code focusable} is false, which is the Android behavior.
     *
     * @param focusable the new status of the {@code View}'s focusability
     */
    @Implementation
    public void setFocusable(boolean focusable) {
        this.focusable = focusable;
        if (!focusable) {
            setFocusableInTouchMode(false);
        }
    }

    @Implementation
    public final boolean isFocusableInTouchMode() {
        return focusableInTouchMode;
    }

    /**
     * Also sets focusable to true if {@code focusableInTouchMode} is true, which is the Android behavior.
     *
     * @param focusableInTouchMode the new status of the {@code View}'s touch mode focusability
     */
    @Implementation
    public void setFocusableInTouchMode(boolean focusableInTouchMode) {
        this.focusableInTouchMode = focusableInTouchMode;
        if (focusableInTouchMode) {
            setFocusable(true);
        }
    }

    @Implementation(i18nSafe = false)
    public void setContentDescription(CharSequence contentDescription) {
        this.contentDescription = contentDescription;
    }

    @Implementation
    public boolean isFocusable() {
        return focusable;
    }

    @Implementation
    public int getId() {
        return id;
    }

    @Implementation
    public CharSequence getContentDescription() {
        return contentDescription;
    }

    /**
     * Simulates the inflating of the requested resource.
     *
     * @param context  the context from which to obtain a layout inflater
     * @param resource the ID of the resource to inflate
     * @param root     the {@code ViewGroup} to add the inflated {@code View} to
     * @return the inflated View
     */
    @Implementation
    public static View inflate(Context context, int resource, ViewGroup root) {
        return ShadowLayoutInflater.from(context).inflate(resource, root);
    }

    /**
     * Finds this {@code View} if it's ID is passed in, returns {@code null} otherwise
     *
     * @param id the id of the {@code View} to find
     * @return the {@code View}, if found, {@code null} otherwise
     */
    @Implementation
    public View findViewById(int id) {
        if (id == this.id) {
            return realView;
        }

        return null;
    }

    @Implementation
    public View findViewWithTag(Object obj) {
        if (obj.equals(realView.getTag())) {
            return realView;
        }

        return null;
    }

    @Implementation
    public View getRootView() {
        ShadowView root = this;
        while (root.parent != null) {
            root = root.parent;
        }
        return root.realView;
    }

    @Implementation
    public ViewGroup.LayoutParams getLayoutParams() {
        return layoutParams;
    }

    @Implementation
    public void setLayoutParams(ViewGroup.LayoutParams params) {
        layoutParams = params;
    }

    @Implementation
    public final ViewParent getParent() {
        return parent == null ? null : (ViewParent) parent.realView;
    }

    @Implementation
    public final Context getContext() {
        return context;
    }

    @Implementation
    public Resources getResources() {
        return context.getResources();
    }

    @Implementation
    public void setBackgroundResource(int backgroundResourceId) {
        this.backgroundResourceId = backgroundResourceId;
        setBackgroundDrawable(getResources().getDrawable(backgroundResourceId));
    }

    /**
     * Non-Android accessor.
     *
     * @return the resource ID of this views background
     */
    public int getBackgroundResourceId() {
        return backgroundResourceId;
    }

    @Implementation
    public void setBackgroundColor(int color) {
        backgroundColor = color;
        setBackgroundDrawable(new ColorDrawable(getResources().getColor(color)));
    }

    /**
     * Non-Android accessor.
     *
     * @return the resource color ID of this views background
     */
    public int getBackgroundColor() {
        return backgroundColor;
    }

    @Implementation
    public void setBackgroundDrawable(Drawable d) {
        this.background = d;
    }

    @Implementation
    public Drawable getBackground() {
        return background;
    }

    @Implementation
    public int getVisibility() {
        return visibility;
    }

    @Implementation
    public void setVisibility(int visibility) {
        this.visibility = visibility;
    }

    @Implementation
    public boolean getFilterTouchesWhenObscured() {
        return filterTouchesWhenObscured;
    }

    @Implementation
    public void setFilterTouchesWhenObscured(boolean enabled) {
        this.filterTouchesWhenObscured = enabled;
    }

    @Implementation
    public void setSelected(boolean selected) {
        this.selected = selected;
    }

    @Implementation
    public boolean isSelected() {
        return this.selected;
    }

    @Implementation
    public boolean isEnabled() {
        return this.enabled;
    }

    @Implementation
    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }

    @Implementation
    public void setOnClickListener(View.OnClickListener onClickListener) {
        this.onClickListener = onClickListener;
    }

    @Implementation
    public boolean performClick() {
        if (onClickListener != null) {
            onClickListener.onClick(realView);
            return true;
        } else {
            return false;
        }
    }

    @Implementation
    public void setOnLongClickListener(View.OnLongClickListener onLongClickListener) {
        this.onLongClickListener = onLongClickListener;
    }

    @Implementation
    public boolean performLongClick() {
        if (onLongClickListener != null) {
            onLongClickListener.onLongClick(realView);
            return true;
        } else {
            return false;
        }
    }

    @Implementation
    public void setOnKeyListener(View.OnKeyListener onKeyListener) {
        this.onKeyListener = onKeyListener;
    }

    @Implementation
    public Object getTag() {
        return this.tag;
    }

    @Implementation
    public void setTag(Object tag) {
        this.tag = tag;
    }

    @Implementation
    public final int getHeight() {
        return bottom - top;
    }

    @Implementation
    public final int getWidth() {
        return right - left;
    }

    @Implementation
    public final int getMeasuredWidth() {
        return measuredWidth;
    }

    @Implementation
    public final int getMeasuredHeight() {
        return measuredHeight;
    }
    
    @Implementation
    public final void setMeasuredDimension(int measuredWidth, int measuredHeight) {
    	this.measuredWidth = measuredWidth;
    	this.measuredHeight = measuredHeight;
    }
    
    @Implementation
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    	setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec),
    			MeasureSpec.getSize(heightMeasureSpec));
    }
    
    @Implementation
    public final void measure(int widthMeasureSpec, int heightMeasureSpec) {
    	// We really want to invoke the onMeasure method of the real view,
    	// as the real View likely contains an implementation of onMeasure
    	// worthy of test, rather the default shadow implementation.
    	// But Android declares onMeasure as protected.
    	try {
    		Method onMeasureMethod = realView.getClass().getDeclaredMethod("onMeasure", Integer.TYPE, Integer.TYPE );
    		onMeasureMethod.setAccessible(true);
    		onMeasureMethod.invoke( realView, widthMeasureSpec, heightMeasureSpec );
    	} catch ( NoSuchMethodException e ) { 
    		// use default shadow implementation
    		onMeasure(widthMeasureSpec, heightMeasureSpec);
    	} catch ( IllegalAccessException e ) { 
    		throw new RuntimeException(e);
    	} catch ( InvocationTargetException e ) { 
    		throw new RuntimeException(e); 
    	} 
    }
    
    @Implementation
    public final void layout(int l, int t, int r, int b) {
        left = l;
        top = t;
        right = r;
        bottom = b;
// todo:       realView.onLayout();
    }

    @Implementation
    public void setPadding(int left, int top, int right, int bottom) {
        paddingLeft = left;
        paddingTop = top;
        paddingRight = right;
        paddingBottom = bottom;
    }

    @Implementation
    public int getPaddingTop() {
        return paddingTop;
    }

    @Implementation
    public int getPaddingLeft() {
        return paddingLeft;
    }

    @Implementation
    public int getPaddingRight() {
        return paddingRight;
    }

    @Implementation
    public int getPaddingBottom() {
        return paddingBottom;
    }

    @Implementation
    public Object getTag(int key) {
        return tags.get(key);
    }

    @Implementation
    public void setTag(int key, Object value) {
        tags.put(key, value);
    }

    @Implementation
    public void requestLayout() {
        didRequestLayout = true;
    }

    public boolean didRequestLayout() {
        return didRequestLayout;
    }

    @Implementation
    public final boolean requestFocus() {
        return requestFocus(View.FOCUS_DOWN);
    }

    @Implementation
    public final boolean requestFocus(int direction) {
        setViewFocus(true);
        return true;
    }

    public void setViewFocus(boolean hasFocus) {
        this.isFocused = hasFocus;

        try {
            Class rectClass = Class.forName("android.graphics.Rect");
            Method method = View.class.getDeclaredMethod("onFocusChanged", Boolean.TYPE, Integer.TYPE,
                rectClass);
            method.setAccessible(true);
            method.invoke(realView, this.isFocused, 0, null);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }

        if (onFocusChangeListener != null) {
            onFocusChangeListener.onFocusChange(realView, hasFocus);
        }
    }

    @Implementation
    public int getNextFocusDownId() {
        return nextFocusDownId;
    }

    @Implementation
    public void setNextFocusDownId(int nextFocusDownId) {
        this.nextFocusDownId = nextFocusDownId;
    }

    @Implementation
    public boolean isFocused() {
        return isFocused;
    }

    @Implementation
    public boolean hasFocus() {
        return isFocused;
    }

    @Implementation
    public void clearFocus() {
        setViewFocus(false);
    }

    @Implementation
    public View findFocus() {
        return hasFocus() ? realView : null;
    }

    @Implementation
    public void setOnFocusChangeListener(View.OnFocusChangeListener listener) {
        onFocusChangeListener = listener;
    }

    @Implementation
    public View.OnFocusChangeListener getOnFocusChangeListener() {
        return onFocusChangeListener;
    }

    @Implementation
    public void invalidate() {
        wasInvalidated = true;
    }

    @Implementation
    public boolean onTouchEvent(MotionEvent event) {
        lastTouchEvent = event;
        return false;
    }

    @Implementation
    public void setOnTouchListener(View.OnTouchListener onTouchListener) {
        this.onTouchListener = onTouchListener;
    }

    @Implementation
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (onTouchListener != null && onTouchListener.onTouch(realView, event)) {
            return true;
        }
        return realView.onTouchEvent(event);
    }

    public MotionEvent getLastTouchEvent() {
        return lastTouchEvent;
    }

    @Implementation
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (onKeyListener != null) {
            return onKeyListener.onKey(realView, event.getKeyCode(), event);
        }
        return false;
    }

    /**
     * Returns a string representation of this {@code View}. Unless overridden, it will be an empty string.
     * <p/>
     * Robolectric extension.
     */
    public String innerText() {
        return "";
    }

    /**
     * Dumps the status of this {@code View} to {@code System.out}
     */
    public void dump() {
        dump(System.out, 0);
    }

    /**
     * Dumps the status of this {@code View} to {@code System.out} at the given indentation level
     */
    public void dump(PrintStream out, int indent) {
        dumpFirstPart(out, indent);
        out.println("/>");
    }

    protected void dumpFirstPart(PrintStream out, int indent) {
        dumpIndent(out, indent);

        out.print("<" + realView.getClass().getSimpleName());
        if (id > 0) {
            out.print(" id=\"" + shadowOf(context).getResourceLoader().getNameForId(id) + "\"");
        }
    }

    protected void dumpIndent(PrintStream out, int indent) {
        for (int i = 0; i < indent; i++) out.print(" ");
    }

    /**
     * @return left side of the view
     */
    @Implementation
    public int getLeft() {
        return left;
    }

    /**
     * @return top coordinate of the view
     */
    @Implementation
    public int getTop() {
        return top;
    }

    /**
     * @return right side of the view
     */
    @Implementation
    public int getRight() {
        return right;
    }

    /**
     * @return bottom coordinate of the view
     */
    @Implementation
    public int getBottom() {
        return bottom;
    }

    /**
     * @return whether the view is clickable
     */
    @Implementation
    public boolean isClickable() {
        return clickable;
    }

    /**
     * Non-Android accessor.
     *
     * @return whether or not {@link #invalidate()} has been called
     */
    public boolean wasInvalidated() {
        return wasInvalidated;
    }

    /**
     * Clears the wasInvalidated flag
     */
    public void clearWasInvalidated() {
        wasInvalidated = false;
    }

    @Implementation
    public void setLeft(int left) {
        this.left = left;
    }

    @Implementation
    public void setTop(int top) {
        this.top = top;
    }

    @Implementation
    public void setRight(int right) {
        this.right = right;
    }

    @Implementation
    public void setBottom(int bottom) {
        this.bottom = bottom;
    }

    /**
     * Non-Android accessor.
     */
    public void setPaddingLeft(int paddingLeft) {
        this.paddingLeft = paddingLeft;
    }

    /**
     * Non-Android accessor.
     */
    public void setPaddingTop(int paddingTop) {
        this.paddingTop = paddingTop;
    }

    /**
     * Non-Android accessor.
     */
    public void setPaddingRight(int paddingRight) {
        this.paddingRight = paddingRight;
    }

    /**
     * Non-Android accessor.
     */
    public void setPaddingBottom(int paddingBottom) {
        this.paddingBottom = paddingBottom;
    }

    /**
     * Non-Android accessor.
     */
    public void setFocused(boolean focused) {
        isFocused = focused;
    }

    /**
     * Non-Android accessor.
     *
     * @return true if this object and all of its ancestors are {@code View.VISIBLE}, returns false if this or
     *         any ancestor is not {@code View.VISIBLE}
     */
    public boolean derivedIsVisible() {
        View parent = realView;
        while (parent != null) {
            if (parent.getVisibility() != View.VISIBLE) {
                return false;
            }
            parent = (View) parent.getParent();
        }
        return true;
    }

    /**
     * Utility method for clicking on views exposing testing scenarios that are not possible when using the actual app.
     *
     * @throws RuntimeException if the view is disabled or if the view or any of its parents are not visible.
     */
    public boolean checkedPerformClick() {
        if (!derivedIsVisible()) {
            throw new RuntimeException("View is not visible and cannot be clicked");
        }
        if (!realView.isEnabled()) {
            throw new RuntimeException("View is not enabled and cannot be clicked");
        }

        return realView.performClick();
    }

    public void applyFocus() {
        if (noParentHasFocus(realView)) {
            Boolean focusRequested = attributeSet.getAttributeBooleanValue("android", "focus", false);
            if (focusRequested || realView.isFocusableInTouchMode()) {
                realView.requestFocus();
            }
        }
    }

    private void applyIdAttribute() {
        Integer id = attributeSet.getAttributeResourceValue("android", "id", 0);
        if (getId() == 0) {
            setId(id);
        }
    }

    private void applyTagAttribute() {
        Object tag = attributeSet.getAttributeValue("android", "tag");
        if (tag != null) {
            setTag(tag);
        }
    }

    private void applyVisibilityAttribute() {
        String visibility = attributeSet.getAttributeValue("android", "visibility");
        if (visibility != null) {
            if (visibility.equals("gone")) {
                setVisibility(View.GONE);
            } else if (visibility.equals("invisible")) {
                setVisibility(View.INVISIBLE);
            }
        }
    }

    private void applyFilterTouchesWhenObscuredAttribute() {
        setFilterTouchesWhenObscured(attributeSet.getAttributeBooleanValue(
                "android", "filterTouchesWhenObscured", false));
    }

    private void applyClickableAttribute() {
        setClickable(attributeSet.getAttributeBooleanValue("android", "clickable", false));
    }

    private void applyFocusableAttribute() {
        setFocusable(attributeSet.getAttributeBooleanValue("android", "focusable", false));
    }

    private void applyEnabledAttribute() {
        setEnabled(attributeSet.getAttributeBooleanValue("android", "enabled", true));
    }

    private void applyBackgroundAttribute() {
        String source = attributeSet.getAttributeValue("android", "background");
        if (source != null) {
            if (source.startsWith("@drawable/")) {
                setBackgroundResource(attributeSet.getAttributeResourceValue("android", "background", 0));
            }
        }
    }

    private void applyOnClickAttribute() {
        final String handlerName = attributeSet.getAttributeValue("android",
                "onClick");
        if (handlerName == null) {
            return;
        }

        /* good part of following code has been directly copied from original
         * android source */
        setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Method mHandler;
                try {
                    mHandler = getContext().getClass().getMethod(handlerName,
                            View.class);
                } catch (NoSuchMethodException e) {
                    int id = getId();
                    String idText = id == View.NO_ID ? "" : " with id '"
                            + shadowOf(context).getResourceLoader()
                            .getNameForId(id) + "'";
                    throw new IllegalStateException("Could not find a method " +
                            handlerName + "(View) in the activity "
                            + getContext().getClass() + " for onClick handler"
                            + " on view " + realView.getClass() + idText, e);
                }

                try {
                    mHandler.invoke(getContext(), realView);
                } catch (IllegalAccessException e) {
                    throw new IllegalStateException("Could not execute non "
                            + "public method of the activity", e);
                } catch (InvocationTargetException e) {
                    throw new IllegalStateException("Could not execute "
                            + "method of the activity", e);
                }
            }
        });
    }

    private void applyContentDescriptionAttribute() {
        String contentDescription = attributeSet.getAttributeValue("android", "contentDescription");
        if (contentDescription != null) {
            if (contentDescription.startsWith("@string/")) {
                int resId = attributeSet.getAttributeResourceValue("android", "contentDescription", 0);
                contentDescription = context.getResources().getString(resId);
            }
            setContentDescription(contentDescription);
        }
    }

    private boolean noParentHasFocus(View view) {
        while (view != null) {
            if (view.hasFocus()) return false;
            view = (View) view.getParent();
        }
        return true;
    }

    /**
     * Non-android accessor.  Returns touch listener, if set.
     *
     * @return
     */
    public View.OnTouchListener getOnTouchListener() {
        return onTouchListener;
    }

    /**
     * Non-android accessor.  Returns click listener, if set.
     *
     * @return
     */
    public View.OnClickListener getOnClickListener() {
        return onClickListener;
    }

    @Implementation
    public void setDrawingCacheEnabled(boolean drawingCacheEnabled) {
        this.drawingCacheEnabled = drawingCacheEnabled;
    }

    @Implementation
    public boolean isDrawingCacheEnabled() {
        return drawingCacheEnabled;
    }

    @Implementation
    public Bitmap getDrawingCache() {
        return Robolectric.newInstanceOf(Bitmap.class);
    }

    @Implementation
    public void post(Runnable action) {
        Robolectric.getUiThreadScheduler().post(action);
    }

    @Implementation
    public void postDelayed(Runnable action, long delayMills) {
        Robolectric.getUiThreadScheduler().postDelayed(action, delayMills);
    }

    @Implementation
    public void postInvalidateDelayed(long delayMilliseconds) {
        Robolectric.getUiThreadScheduler().postDelayed(new Runnable() {
            @Override
            public void run() {
                realView.invalidate();
            }
        }, delayMilliseconds);
    }

    @Implementation
    public Animation getAnimation() {
        return animation;
    }

    @Implementation
    public void setAnimation(Animation anim) {
        animation = anim;
    }

    @Implementation
    public void startAnimation(Animation anim) {
        setAnimation(anim);
        animation.start();
    }

    @Implementation
    public void clearAnimation() {
        if (animation != null) {
            animation.cancel();
            animation = null;
        }
    }

    @Implementation
    public void scrollTo(int x, int y) {
        this.scrollToCoordinates = new Point(x, y);
    }

    @Implementation
    public int getScrollX() {
        return scrollToCoordinates != null ? scrollToCoordinates.x : 0;
    }

    @Implementation
    public int getScrollY() {
        return scrollToCoordinates != null ? scrollToCoordinates.y : 0;
    }

    @Implementation
    public ViewTreeObserver getViewTreeObserver() {
        if (viewTreeObserver == null) {
            viewTreeObserver = newInstanceOf(ViewTreeObserver.class);
        }
        return viewTreeObserver;
    }

    @Implementation
    public void onAnimationEnd() {
    }

    /*
     * Non-Android accessor.
     */
    public void finishedAnimation() {
        try {
            Method onAnimationEnd = realView.getClass().getDeclaredMethod("onAnimationEnd", new Class[0]);
            onAnimationEnd.setAccessible(true);
            onAnimationEnd.invoke(realView);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
