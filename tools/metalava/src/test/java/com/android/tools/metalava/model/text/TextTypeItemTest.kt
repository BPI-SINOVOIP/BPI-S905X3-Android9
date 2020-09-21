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

import com.google.common.truth.Truth.assertThat
import org.junit.Test

class TextTypeItemTest {
    @Test
    fun testTypeString() {
        val full =
            "@android.support.annotation.Nullable java.util.List<@android.support.annotation.Nullable java.lang.String>"
        assertThat(TextTypeItem.toTypeString(full, false, false, false)).isEqualTo(
            "java.util.List<java.lang.String>"
        )
        assertThat(TextTypeItem.toTypeString(full, false, true, false)).isEqualTo(
            "java.util.List<@android.support.annotation.Nullable java.lang.String>"
        )
        assertThat(TextTypeItem.toTypeString(full, false, false, true)).isEqualTo(
            "java.util.List"
        )
        assertThat(
            TextTypeItem.toTypeString(
                full,
                true,
                true,
                false
            )
        ).isEqualTo("@android.support.annotation.Nullable java.util.List<@android.support.annotation.Nullable java.lang.String>")
        assertThat(
            TextTypeItem.toTypeString(
                full,
                true,
                true,
                true
            )
        ).isEqualTo("@android.support.annotation.Nullable java.util.List")
        assertThat(TextTypeItem.toTypeString("int", false, false, false)).isEqualTo("int")

        assertThat(
            TextTypeItem.toTypeString(
                "java.util.List<java.util.Number>[]",
                false,
                false,
                erased = true
            )
        ).isEqualTo("java.util.List[]")
    }
}