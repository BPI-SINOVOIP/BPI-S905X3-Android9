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


package androidx.fragment.app;

import android.os.Parcelable;

import androidx.lifecycle.ViewModelStore;

import java.util.List;

/**
 * FragmentManagerNonConfig stores the retained instance fragments across
 * activity recreation events.
 *
 * <p>Apps should treat objects of this type as opaque, returned by
 * and passed to the state save and restore process for fragments in
 * {@link FragmentController#retainNonConfig()} and
 * {@link FragmentController#restoreAllState(Parcelable, FragmentManagerNonConfig)}.</p>
 */
public class FragmentManagerNonConfig {
    private final List<Fragment> mFragments;
    private final List<FragmentManagerNonConfig> mChildNonConfigs;
    private final List<ViewModelStore> mViewModelStores;

    FragmentManagerNonConfig(List<Fragment> fragments,
            List<FragmentManagerNonConfig> childNonConfigs,
            List<ViewModelStore> viewModelStores) {
        mFragments = fragments;
        mChildNonConfigs = childNonConfigs;
        mViewModelStores = viewModelStores;
    }

    /**
     * @return the retained instance fragments returned by a FragmentManager
     */
    List<Fragment> getFragments() {
        return mFragments;
    }

    /**
     * @return the FragmentManagerNonConfigs from any applicable fragment's child FragmentManager
     */
    List<FragmentManagerNonConfig> getChildNonConfigs() {
        return mChildNonConfigs;
    }

    /**
     * @return the ViewModelStores for all fragments associated with the FragmentManager
     */
    List<ViewModelStore> getViewModelStores() {
        return mViewModelStores;
    }
}
