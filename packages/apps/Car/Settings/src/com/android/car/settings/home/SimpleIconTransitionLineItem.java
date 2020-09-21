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
 * limitations under the License
 */

package com.android.car.settings.home;

import android.annotation.DrawableRes;
import android.annotation.StringRes;
import android.content.Context;
import android.view.View;

import com.android.car.list.SimpleIconLineItem;
import com.android.car.settings.common.BaseFragment;

/**
 * This class extends {@link SimpleIconLineItem} and adds the onClick behavior to
 * trigger the fragmentController to launch a new fragment when clicked.
 * The fragment passed in is what will be launched.
 */
public class SimpleIconTransitionLineItem extends SimpleIconLineItem {

    private BaseFragment.FragmentController mFragmentController;
    private BaseFragment mFragment;

    public SimpleIconTransitionLineItem(
            @StringRes int title,
            @DrawableRes int iconRes,
            Context context,
            CharSequence desc,
            BaseFragment fragment,
            BaseFragment.FragmentController fragmentController) {
        super(title, iconRes, context, desc);
        mFragment = fragment;
        mFragmentController = fragmentController;
    }

    public void onClick(View view) {
        mFragmentController.launchFragment(mFragment);
    }

}
