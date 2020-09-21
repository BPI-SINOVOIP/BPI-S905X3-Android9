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

package com.android.settings.accounts;

import static com.google.common.truth.Truth.assertThat;

import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.lang.reflect.Modifier;

@RunWith(SettingsRobolectricTestRunner.class)
public class RemoveUserFragmentTest {

    @Test
    public void testClassModifier_shouldBePublic() {
        final int modifiers = RemoveUserFragment.class.getModifiers();

        assertThat(Modifier.isPublic(modifiers)).isTrue();
    }
}
