/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.fragment.cts;

import android.app.FragmentHostCallback;
import android.os.Handler;
import android.view.View;

class HostCallbacks extends FragmentHostCallback<FragmentTestActivity> {
    private final FragmentTestActivity mActivity;

    public HostCallbacks(FragmentTestActivity activity, Handler handler, int windowAnimations) {
        super(activity, handler, windowAnimations);
        mActivity = activity;
    }

    @Override
    public FragmentTestActivity onGetHost() {
        return mActivity;
    }

    @Override
    public View onFindViewById(int id) {
        return mActivity.findViewById(id);
    }
}
