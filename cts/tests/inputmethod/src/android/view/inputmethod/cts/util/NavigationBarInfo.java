/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.view.inputmethod.cts.util;

import static android.support.test.InstrumentationRegistry.getInstrumentation;
import static android.view.WindowManager.LayoutParams.FLAG_DIM_BEHIND;
import static android.view.inputmethod.cts.util.TestUtils.getOnMainSync;

import android.app.AlertDialog;
import android.app.Instrumentation;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.Rect;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.support.test.InstrumentationRegistry;
import android.util.Size;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.widget.TextView;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * A utility class to write tests that depend on some capabilities related to navigation bar.
 */
public class NavigationBarInfo {
    private static final long BEFORE_SCREENSHOT_WAIT = TimeUnit.SECONDS.toMillis(1);

    private final boolean mHasBottomNavigationBar;
    private final int mBottomNavigationBerHeight;
    private final boolean mSupportsNavigationBarColor;
    private final boolean mSupportsLightNavigationBar;
    private final boolean mSupportsDimmingWindowLightNavigationBarOverride;

    private NavigationBarInfo(boolean hasBottomNavigationBar, int bottomNavigationBerHeight,
            boolean supportsNavigationBarColor, boolean supportsLightNavigationBar,
            boolean supportsDimmingWindowLightNavigationBarOverride) {
        mHasBottomNavigationBar = hasBottomNavigationBar;
        mBottomNavigationBerHeight = bottomNavigationBerHeight;
        mSupportsNavigationBarColor = supportsNavigationBarColor;
        mSupportsLightNavigationBar = supportsLightNavigationBar;
        mSupportsDimmingWindowLightNavigationBarOverride =
                supportsDimmingWindowLightNavigationBarOverride;
    }

    @Nullable
    private static NavigationBarInfo sInstance;

    /**
     * Returns a {@link NavigationBarInfo} instance.
     *
     * <p>As a performance optimizations, this method internally caches the previous result and
     * returns the same result if this gets called multiple times.</p>
     *
     * <p>Note: The caller should be aware that this method may launch {@link TestActivity}
     * internally.</p>
     *
     * @return {@link NavigationBarInfo} obtained with {@link TestActivity}.
     */
    @NonNull
    public static NavigationBarInfo getInstance() throws Exception {
        if (sInstance != null) {
            return sInstance;
        }

        final int actualBottomInset;
        {
            final AtomicReference<View> viewRef = new AtomicReference<>();
            TestActivity.startSync(activity -> {
                final View view = new View(activity);
                view.setLayoutParams(new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                viewRef.set(view);
                return view;
            }, Intent.FLAG_ACTIVITY_NO_ANIMATION);

            final View view = viewRef.get();

            final WindowInsets windowInsets = getOnMainSync(() -> view.getRootWindowInsets());
            if (!windowInsets.hasStableInsets() || windowInsets.getStableInsetBottom() <= 0) {
                return new NavigationBarInfo(false, 0, false, false, false);
            }
            final Size displaySize = getOnMainSync(() -> {
                final Point size = new Point();
                view.getDisplay().getRealSize(size);
                return new Size(size.x, size.y);
            });

            final Rect viewBoundsOnScreen = getOnMainSync(() -> {
                final int[] xy = new int[2];
                view.getLocationOnScreen(xy);
                final int x = xy[0];
                final int y = xy[1];
                return new Rect(x, y, x + view.getWidth(), y + view.getHeight());
            });
            actualBottomInset = displaySize.getHeight() - viewBoundsOnScreen.bottom;
            if (actualBottomInset != windowInsets.getStableInsetBottom()) {
                sInstance = new NavigationBarInfo(false, 0, false, false, false);
                return sInstance;
            }
        }

        final boolean colorSupported = NavigationBarColorVerifier.verify(
                color -> getBottomNavigationBarBitmapForActivity(
                        color, false /* lightNavigationBar */, actualBottomInset,
                        false /* showDimmingDialog */)).getResult()
                == NavigationBarColorVerifier.ResultType.SUPPORTED;

        final boolean lightModeSupported = LightNavigationBarVerifier.verify(
                (color, lightNavigationBar) -> getBottomNavigationBarBitmapForActivity(
                        color, lightNavigationBar, actualBottomInset,
                        false /* showDimmingDialog */)).getResult()
                == LightNavigationBarVerifier.ResultType.SUPPORTED;

        final boolean dimmingSupported = lightModeSupported && LightNavigationBarVerifier.verify(
                (color, lightNavigationBar) -> getBottomNavigationBarBitmapForActivity(
                        color, lightNavigationBar, actualBottomInset,
                        true /* showDimmingDialog */)).getResult()
                == LightNavigationBarVerifier.ResultType.NOT_SUPPORTED;

        sInstance = new NavigationBarInfo(
                true, actualBottomInset, colorSupported, lightModeSupported, dimmingSupported);
        return sInstance;
    }

    @NonNull
    private static Bitmap getBottomNavigationBarBitmapForActivity(
            @ColorInt int navigationBarColor, boolean lightNavigationBar,
            int bottomNavigationBarHeight, boolean showDimmingDialog) throws Exception {
        final Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();


        final TestActivity testActivity = TestActivity.startSync(activity -> {
            final View view = new View(activity);
            activity.getWindow().setNavigationBarColor(navigationBarColor);

            // Set/unset SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR if necessary.
            final int currentVis = view.getSystemUiVisibility();
            final int newVis = (currentVis & ~View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR)
                    | (lightNavigationBar ? View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR : 0);
            if (currentVis != newVis) {
                view.setSystemUiVisibility(newVis);
            }

            view.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            return view;
        });
        instrumentation.waitForIdleSync();

        final AlertDialog dialog;
        if (showDimmingDialog) {
            dialog = getOnMainSync(() -> {
                final TextView textView = new TextView(testActivity);
                textView.setText("Dimming Window");
                final AlertDialog alertDialog = new AlertDialog.Builder(testActivity)
                        .setView(textView)
                        .create();
                alertDialog.getWindow().setFlags(FLAG_DIM_BEHIND, FLAG_DIM_BEHIND);
                alertDialog.show();
                return alertDialog;
            });
        } else {
            dialog = null;
        }

        Thread.sleep(BEFORE_SCREENSHOT_WAIT);

        final Bitmap fullBitmap = getInstrumentation().getUiAutomation().takeScreenshot();
        final Bitmap bottomNavBarBitmap = Bitmap.createBitmap(fullBitmap, 0,
                fullBitmap.getHeight() - bottomNavigationBarHeight, fullBitmap.getWidth(),
                bottomNavigationBarHeight);
        if (dialog != null) {
            // Dialog#dismiss() is a thread safe method so we don't need to call this from the UI
            // thread.
            dialog.dismiss();
        }
        return bottomNavBarBitmap;
    }

    /**
     * @return {@code true} if this device seems to have bottom navigation bar.
     */
    public boolean hasBottomNavigationBar() {
        return mHasBottomNavigationBar;
    }

    /**
     * @return height of the bottom navigation bar. Valid only when
     *         {@link #hasBottomNavigationBar()} returns {@code true}
     */
    public int getBottomNavigationBerHeight() {
        return mBottomNavigationBerHeight;
    }

    /**
     * @return {@code true} if {@link android.view.Window#setNavigationBarColor(int)} seem to take
     *         effect on this device. Valid only when {@link #hasBottomNavigationBar()} returns
     *         {@code true}
     */
    public boolean supportsNavigationBarColor() {
        return mSupportsNavigationBarColor;
    }

    /**
     * @return {@code true} if {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} seem to
     *         take effect on this device. Valid only when {@link #hasBottomNavigationBar()} returns
     *         {@code true}
     */
    public boolean supportsLightNavigationBar() {
        return mSupportsLightNavigationBar;
    }

    /**
     * @return {@code true} if {@link android.view.View#SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR} will be
     *         canceled when a {@link android.view.Window} with
     *         {@link android.view.WindowManager.LayoutParams#FLAG_DIM_BEHIND} is shown.
     */
    public boolean supportsDimmingWindowLightNavigationBarOverride() {
        return mSupportsDimmingWindowLightNavigationBarOverride;
    }
}
