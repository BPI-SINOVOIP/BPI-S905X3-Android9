package com.xtremelabs.robolectric.shadows;

import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.view.View;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;
import com.xtremelabs.robolectric.internal.RealObject;

@Implements(Fragment.class)
public class ShadowFragment {
    @RealObject
    protected Fragment realFragment;

    protected View view;
    protected FragmentActivity activity;
    private String tag;
    private Bundle savedInstanceState;
    private int containerViewId;
    private boolean shouldReplace;
    private Bundle arguments;
    private boolean attached;
    private boolean hidden;
    private boolean added;

    public void setView(View view) {
        this.view = view;
    }

    public void setActivity(FragmentActivity activity) {
        this.activity = activity;
    }

    @Implementation
    public View getView() {
        return view;
    }

    @Implementation
    public FragmentActivity getActivity() {
        return activity;
    }

    @Implementation
    public void startActivity(Intent intent) {
        new FragmentActivity().startActivity(intent);
    }

    @Implementation
    public void startActivityForResult(Intent intent, int requestCode) {
        activity.startActivityForResult(intent, requestCode);
    }

    @Implementation
    public FragmentManager getFragmentManager() {
        return activity.getSupportFragmentManager();
    }

    @Implementation
    public FragmentManager getChildFragmentManager() {
        // Doesn't follow support library behavior, but is correct enough
        // for robolectric v1.
        return getFragmentManager();
    }

    @Implementation
    public String getTag() {
        return tag;
    }

    @Implementation
    public Resources getResources() {
        if (activity == null) {
            throw new IllegalStateException("Fragment " + this + " not attached to Activity");
        }
        return activity.getResources();
    }

    public void setTag(String tag) {
        this.tag = tag;
    }

    public void setSavedInstanceState(Bundle savedInstanceState) {
        this.savedInstanceState = savedInstanceState;
    }

    public Bundle getSavedInstanceState() {
        return savedInstanceState;
    }

    public void setContainerViewId(int containerViewId) {
        this.containerViewId = containerViewId;
    }

    public int getContainerViewId() {
        return containerViewId;
    }

    public void setShouldReplace(boolean shouldReplace) {
        this.shouldReplace = shouldReplace;
    }

    public boolean getShouldReplace() {
        return shouldReplace;
    }

    @Implementation
    public Bundle getArguments() {
        return arguments;
    }

    @Implementation
    public void setArguments(Bundle arguments) {
        this.arguments = arguments;
    }

    @Implementation
    public final String getString(int resId) {
        if (activity == null) {
          throw new IllegalStateException("Fragment " + this + " not attached to Activity");
        }
        return activity.getString(resId);
    }

    @Implementation
    public final String getString(int resId, Object... formatArgs) {
        if (activity == null) {
          throw new IllegalStateException("Fragment " + this + " not attached to Activity");
        }
        return activity.getString(resId, formatArgs);
    }

    public void setAttached(boolean isAttached) {
        attached = isAttached;
    }

    public boolean isAttached() {
        return attached;
    }

    public void setHidden(boolean isHidden) {
        hidden = isHidden;
    }

    public void setAdded(boolean isAdded) {
        added = isAdded;
    }

    @Implementation
    public boolean isAdded() {
        return getActivity() != null && added;
    }

    @Implementation
    public boolean isHidden() {
        return hidden;
    }

    @Implementation
    public boolean isVisible() {
        return isAdded() && !isHidden() && getView() != null
                && getView().getVisibility() == View.VISIBLE;
    }
}
