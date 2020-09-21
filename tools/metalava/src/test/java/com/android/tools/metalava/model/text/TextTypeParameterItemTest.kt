/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tools.metalava.model.text

import com.google.common.truth.Truth
import org.junit.Test

class TextTypeParameterItemTest {
    @Test
    fun testTypeParameterNames() {
        Truth.assertThat(TextTypeParameterItem.bounds(null).toString()).isEqualTo("[]")
        Truth.assertThat(TextTypeParameterItem.bounds("").toString()).isEqualTo("[]")
        Truth.assertThat(TextTypeParameterItem.bounds("X").toString()).isEqualTo("[]")
        Truth.assertThat(TextTypeParameterItem.bounds("DEF extends T").toString()).isEqualTo("[T]")
        Truth.assertThat(TextTypeParameterItem.bounds("T extends java.lang.Comparable<? super T>").toString())
            .isEqualTo("[java.lang.Comparable]")
        Truth.assertThat(TextTypeParameterItem.bounds("T extends java.util.List<Number> & java.util.RandomAccess").toString())
            .isEqualTo("[java.util.List, java.util.RandomAccess]")
    }
}