/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.testapp.multiconfig;

import android.databinding.ViewDataBinding;
import android.databinding.testapp.BaseLandDataBinderTest;
import android.databinding.testapp.BR;
import android.databinding.testapp.R;
import android.databinding.testapp.databinding.BasicBindingBinding;
import android.databinding.testapp.databinding.ConditionalBindingBinding;
import android.databinding.testapp.databinding.IncludedLayoutBinding;
import android.databinding.testapp.databinding.MultiResLayoutBinding;
import android.databinding.testapp.vo.NotBindableVo;

import android.content.pm.ActivityInfo;
import android.test.UiThreadTest;
import android.view.View;
import android.widget.TextView;

public class LandscapeConfigTest extends BaseLandDataBinderTest<MultiResLayoutBinding> {

    public LandscapeConfigTest() {
        super(MultiResLayoutBinding.class, ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        initBinder();
        getInstrumentation().waitForIdleSync();
    }

    public void testSharedViewIdAndVariableInheritance()
            throws InterruptedException, NoSuchMethodException, NoSuchFieldException {
        assertEquals("MultiResLayoutBindingLandImpl", mBinder.getClass().getSimpleName());
        assertPublicField(TextView.class, "objectInLandTextView");
        assertPublicField(TextView.class, "objectInDefaultTextView");
        assertPublicField(View.class, "objectInDefaultTextView2");

        assertField(NotBindableVo.class, "mObjectInLand");
        assertField(NotBindableVo.class, "mObjectInDefault");

        // includes
        assertPublicField(ViewDataBinding.class, "includedLayoutConflict");
        assertPublicField(BasicBindingBinding.class, "includedLayoutShared");
        assertPublicField(ConditionalBindingBinding.class, "includedLayoutPort");
        assertPublicField(ConditionalBindingBinding.class, "includedLayoutLand");
    }

    @UiThreadTest
    public void testSetVariable() throws Throwable {
        assertTrue(mBinder.setVariable(BR.objectInBoth, null));
        assertTrue(mBinder.setVariable(BR.objectInDefault, null));
        assertTrue(mBinder.setVariable(BR.objectInLand, null));
        assertFalse(mBinder.setVariable(BR.obj, null));
    }
}
