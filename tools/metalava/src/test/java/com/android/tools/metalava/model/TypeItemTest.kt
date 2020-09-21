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

package com.android.tools.metalava.model

import com.android.tools.metalava.JAVA_LANG_STRING
import com.android.tools.metalava.options
import com.google.common.truth.Truth.assertThat
import org.junit.Test

class TypeItemTest {
    @Test
    fun test() {
        options.omitCommonPackages = true
        assertThat(TypeItem.shortenTypes("@android.support.annotation.Nullable")).isEqualTo("@Nullable")
        assertThat(TypeItem.shortenTypes(JAVA_LANG_STRING)).isEqualTo("String")
        assertThat(TypeItem.shortenTypes("java.lang.reflect.Method")).isEqualTo("java.lang.reflect.Method")
        assertThat(TypeItem.shortenTypes("java.util.List<java.lang.String>")).isEqualTo("java.util.List<java.lang.String>")
        assertThat(TypeItem.shortenTypes("java.util.List<@android.support.annotation.NonNull java.lang.String>")).isEqualTo(
            "java.util.List<@NonNull java.lang.String>"
        )
    }
}