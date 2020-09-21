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
 * limitations under the License
 */

package com.android.settings.testutils.shadow;

import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowInputMethodManager;

import java.util.Collections;
import java.util.List;

/*
 * Shadow for {@link InputMethodManager} that has accessors for installed input methods.
 */
@Implements(InputMethodManager.class)
public class ShadowInputMethodManagerWithMethodList extends ShadowInputMethodManager {

    private List<InputMethodInfo> mInputMethodInfos = Collections.emptyList();

    @Implementation
    public static InputMethodManager getInstance() {
        return ShadowInputMethodManager.peekInstance();
    }

    @Implementation
    public List<InputMethodInfo> getInputMethodList() {
        return mInputMethodInfos;
    }

    // Non-Android setter.
    public void setInputMethodList(List<InputMethodInfo> inputMethodInfos) {
        mInputMethodInfos = inputMethodInfos;
    }
}
