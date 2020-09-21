/*
 * Copyright 2018 The Android Open Source Project
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


package androidx.legacy.app;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import android.os.Build;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.content.ContextCompat;
import androidx.core.view.GravityCompat;
import androidx.core.view.ViewCompat;
import androidx.drawerlayout.widget.DrawerLayout;

import java.lang.reflect.Method;

/**
 * This class provides a handy way to tie together the functionality of
 * {@link DrawerLayout} and the framework <code>ActionBar</code> to implement the recommended
 * design for navigation drawers.
 *
 * <p>To use <code>ActionBarDrawerToggle</code>, create one in your Activity and call through
 * to the following methods corresponding to your Activity callbacks:</p>
 *
 * <ul>
 * <li>{@link Activity#onConfigurationChanged(android.content.res.Configuration) onConfigurationChanged}</li>
 * <li>{@link Activity#onOptionsItemSelected(android.view.MenuItem) onOptionsItemSelected}</li>
 * </ul>
 *
 * <p>Call {@link #syncState()} from your <code>Activity</code>'s
 * {@link Activity#onPostCreate(android.os.Bundle) onPostCreate} to synchronize the indicator
 * with the state of the linked DrawerLayout after <code>onRestoreInstanceState</code>
 * has occurred.</p>
 *
 * <p><code>ActionBarDrawerToggle</code> can be used directly as a
 * {@link DrawerLayout.DrawerListener}, or if you are already providing your own listener,
 * call through to each of the listener methods from your own.</p>
 *
 */
@Deprecated
public class ActionBarDrawerToggle implements DrawerLayout.DrawerListener {

    /**
     * Allows an implementing Activity to return an {@link ActionBarDrawerToggle.Delegate} to use
     * with ActionBarDrawerToggle.
     *
     * @deprecated Use ActionBarDrawerToggle.DelegateProvider in support-v7-appcompat.
     */
    @Deprecated
    public interface DelegateProvider {

        /**
         * @return Delegate to use for ActionBarDrawableToggles, or null if the Activity
         *         does not wish to override the default behavior.
         */
        @Nullable
        Delegate getDrawerToggleDelegate();
    }

    /**
     * @deprecated Use ActionBarDrawerToggle.DelegateProvider in support-v7-appcompat.
     */
    @Deprecated
    public interface Delegate {
        /**
         * @return Up indicator drawable as defined in the Activity's theme, or null if one is not
         *         defined.
         */
        @Nullable
        Drawable getThemeUpIndicator();

        /**
         * Set the Action Bar's up indicator drawable and content description.
         *
         * @param upDrawable     - Drawable to set as up indicator
         * @param contentDescRes - Content description to set
         */
        void setActionBarUpIndicator(Drawable upDrawable, @StringRes int contentDescRes);

        /**
         * Set the Action Bar's up indicator content description.
         *
         * @param contentDescRes - Content description to set
         */
        void setActionBarDescription(@StringRes int contentDescRes);
    }

    private static final String TAG = "ActionBarDrawerToggle";

    private static final int[] THEME_ATTRS = new int[] {
            android.R.attr.homeAsUpIndicator
    };

    /** Fraction of its total width by which to offset the toggle drawable. */
    private static final float TOGGLE_DRAWABLE_OFFSET = 1 / 3f;

    // android.R.id.home as defined by public API in v11
    private static final int ID_HOME = 0x0102002c;

    final Activity mActivity;
    private final Delegate mActivityImpl;
    private final DrawerLayout mDrawerLayout;
    private boolean mDrawerIndicatorEnabled = true;
    private boolean mHasCustomUpIndicator;

    private Drawable mHomeAsUpIndicator;
    private Drawable mDrawerImage;
    private SlideDrawable mSlider;
    private final int mDrawerImageResource;
    private final int mOpenDrawerContentDescRes;
    private final int mCloseDrawerContentDescRes;

    private SetIndicatorInfo mSetIndicatorInfo;

    /**
     * Construct a new ActionBarDrawerToggle.
     *
     * <p>The given {@link Activity} will be linked to the specified {@link DrawerLayout}.
     * The provided drawer indicator drawable will animate slightly off-screen as the drawer
     * is opened, indicating that in the open state the drawer will move off-screen when pressed
     * and in the closed state the drawer will move on-screen when pressed.</p>
     *
     * <p>String resources must be provided to describe the open/close drawer actions for
     * accessibility services.</p>
     *
     * @param activity The Activity hosting the drawer
     * @param drawerLayout The DrawerLayout to link to the given Activity's ActionBar
     * @param drawerImageRes A Drawable resource to use as the drawer indicator
     * @param openDrawerContentDescRes A String resource to describe the "open drawer" action
     *                                 for accessibility
     * @param closeDrawerContentDescRes A String resource to describe the "close drawer" action
     *                                  for accessibility
     */
    public ActionBarDrawerToggle(Activity activity, DrawerLayout drawerLayout,
            @DrawableRes int drawerImageRes, @StringRes int openDrawerContentDescRes,
            @StringRes int closeDrawerContentDescRes) {
        this(activity, drawerLayout, !assumeMaterial(activity), drawerImageRes,
                openDrawerContentDescRes, closeDrawerContentDescRes);
    }

    private static boolean assumeMaterial(Context context) {
        return context.getApplicationInfo().targetSdkVersion >= 21
                && (Build.VERSION.SDK_INT >= 21);
    }

    /**
     * Construct a new ActionBarDrawerToggle.
     *
     * <p>The given {@link Activity} will be linked to the specified {@link DrawerLayout}.
     * The provided drawer indicator drawable will animate slightly off-screen as the drawer
     * is opened, indicating that in the open state the drawer will move off-screen when pressed
     * and in the closed state the drawer will move on-screen when pressed.</p>
     *
     * <p>String resources must be provided to describe the open/close drawer actions for
     * accessibility services.</p>
     *
     * @param activity The Activity hosting the drawer
     * @param drawerLayout The DrawerLayout to link to the given Activity's ActionBar
     * @param animate True to animate the drawer indicator along with the drawer's position.
     *                Material apps should set this to false.
     * @param drawerImageRes A Drawable resource to use as the drawer indicator
     * @param openDrawerContentDescRes A String resource to describe the "open drawer" action
     *                                 for accessibility
     * @param closeDrawerContentDescRes A String resource to describe the "close drawer" action
     *                                  for accessibility
     */
    public ActionBarDrawerToggle(Activity activity, DrawerLayout drawerLayout, boolean animate,
            @DrawableRes int drawerImageRes, @StringRes int openDrawerContentDescRes,
            @StringRes int closeDrawerContentDescRes) {
        mActivity = activity;

        // Allow the Activity to provide an impl
        if (activity instanceof DelegateProvider) {
            mActivityImpl = ((DelegateProvider) activity).getDrawerToggleDelegate();
        } else {
            mActivityImpl = null;
        }

        mDrawerLayout = drawerLayout;
        mDrawerImageResource = drawerImageRes;
        mOpenDrawerContentDescRes = openDrawerContentDescRes;
        mCloseDrawerContentDescRes = closeDrawerContentDescRes;

        mHomeAsUpIndicator = getThemeUpIndicator();
        mDrawerImage = ContextCompat.getDrawable(activity, drawerImageRes);
        mSlider = new SlideDrawable(mDrawerImage);
        mSlider.setOffset(animate ? TOGGLE_DRAWABLE_OFFSET : 0);
    }

    /**
     * Synchronize the state of the drawer indicator/affordance with the linked DrawerLayout.
     *
     * <p>This should be called from your <code>Activity</code>'s
     * {@link Activity#onPostCreate(android.os.Bundle) onPostCreate} method to synchronize after
     * the DrawerLayout's instance state has been restored, and any other time when the state
     * may have diverged in such a way that the ActionBarDrawerToggle was not notified.
     * (For example, if you stop forwarding appropriate drawer events for a period of time.)</p>
     */
    public void syncState() {
        if (mDrawerLayout.isDrawerOpen(GravityCompat.START)) {
            mSlider.setPosition(1);
        } else {
            mSlider.setPosition(0);
        }

        if (mDrawerIndicatorEnabled) {
            setActionBarUpIndicator(mSlider, mDrawerLayout.isDrawerOpen(GravityCompat.START)
                    ? mCloseDrawerContentDescRes : mOpenDrawerContentDescRes);
        }
    }

    /**
     * Set the up indicator to display when the drawer indicator is not
     * enabled.
     * <p>
     * If you pass <code>null</code> to this method, the default drawable from
     * the theme will be used.
     *
     * @param indicator A drawable to use for the up indicator, or null to use
     *                  the theme's default
     * @see #setDrawerIndicatorEnabled(boolean)
     */
    public void setHomeAsUpIndicator(Drawable indicator) {
        if (indicator == null) {
            mHomeAsUpIndicator = getThemeUpIndicator();
            mHasCustomUpIndicator = false;
        } else {
            mHomeAsUpIndicator = indicator;
            mHasCustomUpIndicator = true;
        }

        if (!mDrawerIndicatorEnabled) {
            setActionBarUpIndicator(mHomeAsUpIndicator, 0);
        }
    }

    /**
     * Set the up indicator to display when the drawer indicator is not
     * enabled.
     * <p>
     * If you pass 0 to this method, the default drawable from the theme will
     * be used.
     *
     * @param resId Resource ID of a drawable to use for the up indicator, or 0
     *              to use the theme's default
     * @see #setDrawerIndicatorEnabled(boolean)
     */
    public void setHomeAsUpIndicator(int resId) {
        Drawable indicator = null;
        if (resId != 0) {
            indicator = ContextCompat.getDrawable(mActivity, resId);
        }

        setHomeAsUpIndicator(indicator);
    }

    /**
     * Enable or disable the drawer indicator. The indicator defaults to enabled.
     *
     * <p>When the indicator is disabled, the <code>ActionBar</code> will revert to displaying
     * the home-as-up indicator provided by the <code>Activity</code>'s theme in the
     * <code>android.R.attr.homeAsUpIndicator</code> attribute instead of the animated
     * drawer glyph.</p>
     *
     * @param enable true to enable, false to disable
     */
    public void setDrawerIndicatorEnabled(boolean enable) {
        if (enable != mDrawerIndicatorEnabled) {
            if (enable) {
                setActionBarUpIndicator(mSlider, mDrawerLayout.isDrawerOpen(GravityCompat.START)
                        ? mCloseDrawerContentDescRes : mOpenDrawerContentDescRes);
            } else {
                setActionBarUpIndicator(mHomeAsUpIndicator, 0);
            }
            mDrawerIndicatorEnabled = enable;
        }
    }

    /**
     * @return true if the enhanced drawer indicator is enabled, false otherwise
     * @see #setDrawerIndicatorEnabled(boolean)
     */
    public boolean isDrawerIndicatorEnabled() {
        return mDrawerIndicatorEnabled;
    }

    /**
     * This method should always be called by your <code>Activity</code>'s
     * {@link Activity#onConfigurationChanged(android.content.res.Configuration) onConfigurationChanged}
     * method.
     *
     * @param newConfig The new configuration
     */
    public void onConfigurationChanged(Configuration newConfig) {
        // Reload drawables that can change with configuration
        if (!mHasCustomUpIndicator) {
            mHomeAsUpIndicator = getThemeUpIndicator();
        }
        mDrawerImage = ContextCompat.getDrawable(mActivity, mDrawerImageResource);
        syncState();
    }

    /**
     * This method should be called by your <code>Activity</code>'s
     * {@link Activity#onOptionsItemSelected(android.view.MenuItem) onOptionsItemSelected} method.
     * If it returns true, your <code>onOptionsItemSelected</code> method should return true and
     * skip further processing.
     *
     * @param item the MenuItem instance representing the selected menu item
     * @return true if the event was handled and further processing should not occur
     */
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item != null && item.getItemId() == ID_HOME && mDrawerIndicatorEnabled) {
            if (mDrawerLayout.isDrawerVisible(GravityCompat.START)) {
                mDrawerLayout.closeDrawer(GravityCompat.START);
            } else {
                mDrawerLayout.openDrawer(GravityCompat.START);
            }
            return true;
        }
        return false;
    }

    /**
     * {@link DrawerLayout.DrawerListener} callback method. If you do not use your
     * ActionBarDrawerToggle instance directly as your DrawerLayout's listener, you should call
     * through to this method from your own listener object.
     *
     * @param drawerView The child view that was moved
     * @param slideOffset The new offset of this drawer within its range, from 0-1
     */
    @Override
    public void onDrawerSlide(View drawerView, float slideOffset) {
        float glyphOffset = mSlider.getPosition();
        if (slideOffset > 0.5f) {
            glyphOffset = Math.max(glyphOffset, Math.max(0.f, slideOffset - 0.5f) * 2);
        } else {
            glyphOffset = Math.min(glyphOffset, slideOffset * 2);
        }
        mSlider.setPosition(glyphOffset);
    }

    /**
     * {@link DrawerLayout.DrawerListener} callback method. If you do not use your
     * ActionBarDrawerToggle instance directly as your DrawerLayout's listener, you should call
     * through to this method from your own listener object.
     *
     * @param drawerView Drawer view that is now open
     */
    @Override
    public void onDrawerOpened(View drawerView) {
        mSlider.setPosition(1);
        if (mDrawerIndicatorEnabled) {
            setActionBarDescription(mCloseDrawerContentDescRes);
        }
    }

    /**
     * {@link DrawerLayout.DrawerListener} callback method. If you do not use your
     * ActionBarDrawerToggle instance directly as your DrawerLayout's listener, you should call
     * through to this method from your own listener object.
     *
     * @param drawerView Drawer view that is now closed
     */
    @Override
    public void onDrawerClosed(View drawerView) {
        mSlider.setPosition(0);
        if (mDrawerIndicatorEnabled) {
            setActionBarDescription(mOpenDrawerContentDescRes);
        }
    }

    /**
     * {@link DrawerLayout.DrawerListener} callback method. If you do not use your
     * ActionBarDrawerToggle instance directly as your DrawerLayout's listener, you should call
     * through to this method from your own listener object.
     *
     * @param newState The new drawer motion state
     */
    @Override
    public void onDrawerStateChanged(int newState) {
    }

    private Drawable getThemeUpIndicator() {
        if (mActivityImpl != null) {
            return mActivityImpl.getThemeUpIndicator();
        }
        if (Build.VERSION.SDK_INT >= 18) {
            final ActionBar actionBar = mActivity.getActionBar();
            final Context context;
            if (actionBar != null) {
                context = actionBar.getThemedContext();
            } else {
                context = mActivity;
            }

            final TypedArray a = context.obtainStyledAttributes(null, THEME_ATTRS,
                    android.R.attr.actionBarStyle, 0);
            final Drawable result = a.getDrawable(0);
            a.recycle();
            return result;
        } else {
            final TypedArray a = mActivity.obtainStyledAttributes(THEME_ATTRS);
            final Drawable result = a.getDrawable(0);
            a.recycle();
            return result;
        }
    }

    private void setActionBarUpIndicator(Drawable upDrawable, int contentDescRes) {
        if (mActivityImpl != null) {
            mActivityImpl.setActionBarUpIndicator(upDrawable, contentDescRes);
            return;
        }
        if (Build.VERSION.SDK_INT >= 18) {
            final ActionBar actionBar = mActivity.getActionBar();
            if (actionBar != null) {
                actionBar.setHomeAsUpIndicator(upDrawable);
                actionBar.setHomeActionContentDescription(contentDescRes);
            }
        } else {
            if (mSetIndicatorInfo == null) {
                mSetIndicatorInfo = new SetIndicatorInfo(mActivity);
            }
            if (mSetIndicatorInfo.mSetHomeAsUpIndicator != null) {
                try {
                    final ActionBar actionBar = mActivity.getActionBar();
                    mSetIndicatorInfo.mSetHomeAsUpIndicator.invoke(actionBar, upDrawable);
                    mSetIndicatorInfo.mSetHomeActionContentDescription.invoke(
                            actionBar, contentDescRes);
                } catch (Exception e) {
                    Log.w(TAG, "Couldn't set home-as-up indicator via JB-MR2 API", e);
                }
            } else if (mSetIndicatorInfo.mUpIndicatorView != null) {
                mSetIndicatorInfo.mUpIndicatorView.setImageDrawable(upDrawable);
            } else {
                Log.w(TAG, "Couldn't set home-as-up indicator");
            }
        }
    }

    private void setActionBarDescription(int contentDescRes) {
        if (mActivityImpl != null) {
            mActivityImpl.setActionBarDescription(contentDescRes);
            return;
        }
        if (Build.VERSION.SDK_INT >= 18) {
            final ActionBar actionBar = mActivity.getActionBar();
            if (actionBar != null) {
                actionBar.setHomeActionContentDescription(contentDescRes);
            }
        } else {
            if (mSetIndicatorInfo == null) {
                mSetIndicatorInfo = new SetIndicatorInfo(mActivity);
            }
            if (mSetIndicatorInfo.mSetHomeAsUpIndicator != null) {
                try {
                    final ActionBar actionBar = mActivity.getActionBar();
                    mSetIndicatorInfo.mSetHomeActionContentDescription.invoke(
                            actionBar, contentDescRes);
                    // For API 19 and earlier, we need to manually force the
                    // action bar to generate a new content description.
                    actionBar.setSubtitle(actionBar.getSubtitle());
                } catch (Exception e) {
                    Log.w(TAG, "Couldn't set content description via JB-MR2 API", e);
                }
            }
        }
    }

    private static class SetIndicatorInfo {
        Method mSetHomeAsUpIndicator;
        Method mSetHomeActionContentDescription;
        ImageView mUpIndicatorView;

        SetIndicatorInfo(Activity activity) {
            try {
                mSetHomeAsUpIndicator = ActionBar.class.getDeclaredMethod("setHomeAsUpIndicator",
                        Drawable.class);
                mSetHomeActionContentDescription = ActionBar.class.getDeclaredMethod(
                        "setHomeActionContentDescription", Integer.TYPE);

                // If we got the method we won't need the stuff below.
                return;
            } catch (NoSuchMethodException e) {
                // Oh well. We'll use the other mechanism below instead.
            }

            final View home = activity.findViewById(android.R.id.home);
            if (home == null) {
                // Action bar doesn't have a known configuration, an OEM messed with things.
                return;
            }

            final ViewGroup parent = (ViewGroup) home.getParent();
            final int childCount = parent.getChildCount();
            if (childCount != 2) {
                // No idea which one will be the right one, an OEM messed with things.
                return;
            }

            final View first = parent.getChildAt(0);
            final View second = parent.getChildAt(1);
            final View up = first.getId() == android.R.id.home ? second : first;

            if (up instanceof ImageView) {
                // Jackpot! (Probably...)
                mUpIndicatorView = (ImageView) up;
            }
        }
    }

    private class SlideDrawable extends InsetDrawable implements Drawable.Callback {
        private final boolean mHasMirroring = Build.VERSION.SDK_INT > 18;
        private final Rect mTmpRect = new Rect();

        private float mPosition;
        private float mOffset;

        SlideDrawable(Drawable wrapped) {
            super(wrapped, 0);
        }

        /**
         * Sets the current position along the offset.
         *
         * @param position a value between 0 and 1
         */
        public void setPosition(float position) {
            mPosition = position;
            invalidateSelf();
        }

        public float getPosition() {
            return mPosition;
        }

        /**
         * Specifies the maximum offset when the position is at 1.
         *
         * @param offset maximum offset as a fraction of the drawable width,
         *            positive to shift left or negative to shift right.
         * @see #setPosition(float)
         */
        public void setOffset(float offset) {
            mOffset = offset;
            invalidateSelf();
        }

        @Override
        public void draw(@NonNull Canvas canvas) {
            copyBounds(mTmpRect);
            canvas.save();

            // Layout direction must be obtained from the activity.
            final boolean isLayoutRTL = ViewCompat.getLayoutDirection(
                    mActivity.getWindow().getDecorView()) == ViewCompat.LAYOUT_DIRECTION_RTL;
            final int flipRtl = isLayoutRTL ? -1 : 1;
            final int width = mTmpRect.width();
            canvas.translate(-mOffset * width * mPosition * flipRtl, 0);

            // Force auto-mirroring if it's not supported by the platform.
            if (isLayoutRTL && !mHasMirroring) {
                canvas.translate(width, 0);
                canvas.scale(-1, 1);
            }

            super.draw(canvas);
            canvas.restore();
        }
    }
}
