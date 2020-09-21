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
package com.android.managedprovisioning.common;

import static android.graphics.PorterDuff.Mode.SRC_ATOP;
import static android.support.test.espresso.matcher.ViewMatchers.assertThat;

import static junit.framework.Assert.assertTrue;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.CoreMatchers.instanceOf;

import android.annotation.LayoutRes;
import android.app.Activity;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.view.ContextThemeWrapper;
import android.widget.ImageView;
import android.widget.ProgressBar;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.preprovisioning.anim.ColorMatcher;
import com.android.managedprovisioning.preprovisioning.anim.SwiperThemeMatcher;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

public class CustomizationVerifier {
    private final Activity mActivity;

    public CustomizationVerifier(Activity activity) {
        mActivity = activity;
    }

    public void assertStatusBarColorCorrect(int targetColor) {
        int statusBarColor = mActivity.getWindow().getStatusBarColor();
        assertThat(statusBarColor, equalTo(targetColor));
    }

    // Disabled for b/73157813.
    public void assertSwiperColorCorrect(int targetSwiperColor) {
//        Drawable expectedDrawable = makeExpectedTopInfoDrawable(targetSwiperColor);
//        ImageView animatedView = mActivity.findViewById(R.id.animated_info);
//        Drawable actualDrawable = animatedView.getDrawable();
//        assertDrawableEquals(expectedDrawable, actualDrawable);
    }

    public void assertDefaultLogoCorrect(int targetColor) {
        Drawable actualLogo = extractLogo();
        Drawable expectedLogo = makeDefaultLogo(targetColor);
        assertThat(actualLogo.getConstantState(), equalTo(expectedLogo.getConstantState()));
        assertThat(actualLogo.getColorFilter(), equalTo(expectedLogo.getColorFilter()));
    }

    public void assertCustomLogoCorrect(Bitmap targetLogo) {
        Bitmap actualLogo = ((BitmapDrawable) extractLogo()).getBitmap();
        assertThat(targetLogo, bitmapEqualTo(actualLogo));
    }

    public void assertNextButtonColorCorrect(int targetColor) {
        ColorStateList actual = mActivity.findViewById(R.id.next_button).getBackgroundTintList();
        ColorStateList expected = ColorStateList.valueOf(targetColor);
        assertThat(actual, equalTo(expected));
    }

    public void assertProgressBarColorCorrect(@LayoutRes int progressBarLayoutId, int targetColor) {
        ProgressBar progressBar = mActivity.findViewById(progressBarLayoutId);

        ColorStateList expected = ColorStateList.valueOf(targetColor);
        assertThat(progressBar.getIndeterminateTintList(), equalTo(expected));
        assertThat(progressBar.getProgressBackgroundTintList(), equalTo(expected));
    }

    private Matcher<Bitmap> bitmapEqualTo(Bitmap expected) {
        return new BaseMatcher<Bitmap>() {
            @Override
            public void describeTo(Description description) {
                description.appendText("Bitmap different from expected.");
            }

            @Override
            public boolean matches(Object actual) {
                return expected.sameAs((Bitmap) actual);
            }
        };
    }

    private Drawable makeDefaultLogo(int color) {
        Drawable logo = mActivity.getDrawable(R.drawable.ic_enterprise_blue_24dp);
        logo.setColorFilter(new PorterDuffColorFilter(color, SRC_ATOP));
        return logo;
    }

    private Drawable makeExpectedTopInfoDrawable(int color) {
        int swiperTheme = new SwiperThemeMatcher(mActivity, new ColorMatcher()).findTheme(color);
        ContextThemeWrapper wrapper = new ContextThemeWrapper(mActivity, swiperTheme);
        return mActivity.getResources().getDrawable(
                R.drawable.topinfo_animation, wrapper.getTheme());
    }

    private Drawable extractLogo() {
        return ((ImageView) mActivity.findViewById(
                com.android.setupwizardlib.R.id.suw_layout_icon)).getDrawable();
    }

    private Bitmap drawableToBitmap(Drawable drawable) {
        Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return bitmap;
    }

    private void assertDrawableEquals(Drawable expected, Drawable actual) {
        assertTrue(drawableToBitmap(expected).sameAs(drawableToBitmap(actual)));
    }
}