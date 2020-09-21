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

package com.android.tools.metalava

import org.junit.Ignore
import org.junit.Test

@SuppressWarnings("ALL") // Sample code
class ExtractAnnotationsTest : DriverTest() {

    @Test
    fun `Check java typedef extraction and warning about non-source retention of typedefs`() {
        check(
            sourceFiles = *arrayOf(
                java(
            """
                    package test.pkg;

                    import android.annotation.IntDef;
                    import android.annotation.IntRange;

                    import java.lang.annotation.Retention;
                    import java.lang.annotation.RetentionPolicy;

                    @SuppressWarnings({"UnusedDeclaration", "WeakerAccess"})
                    public class IntDefTest {
                        @IntDef({STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT})
                        @IntRange(from = 20)
                        private @interface DialogStyle {}

                        public static final int STYLE_NORMAL = 0;
                        public static final int STYLE_NO_TITLE = 1;
                        public static final int STYLE_NO_FRAME = 2;
                        public static final int STYLE_NO_INPUT = 3;
                        public static final int UNRELATED = 3;

                        public void setStyle(@DialogStyle int style, int theme) {
                        }

                        public void testIntDef(int arg) {
                        }
                        @IntDef(value = {STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT, 3, 3 + 1}, flag=true)
                        @Retention(RetentionPolicy.SOURCE)
                        private @interface DialogFlags {}

                        public void setFlags(Object first, @DialogFlags int flags) {
                        }

                        public static final String TYPE_1 = "type1";
                        public static final String TYPE_2 = "type2";
                        public static final String UNRELATED_TYPE = "other";

                        public static class Inner {
                            public void setInner(@DialogFlags int flags) {
                            }
                        }
                    }
                    """
                ).indented(),
                intDefAnnotationSource,
                intRangeAnnotationSource
            ),
            warnings = "src/test/pkg/IntDefTest.java:11: error: This typedef annotation class should have @Retention(RetentionPolicy.SOURCE) [AnnotationExtraction:146]",
            extractAnnotations = mapOf("test.pkg" to """
                <?xml version="1.0" encoding="UTF-8"?>
                <root>
                  <item name="test.pkg.IntDefTest void setFlags(java.lang.Object, int) 1">
                    <annotation name="androidx.annotation.IntDef">
                      <val name="value" val="{test.pkg.IntDefTest.STYLE_NORMAL, test.pkg.IntDefTest.STYLE_NO_TITLE, test.pkg.IntDefTest.STYLE_NO_FRAME, test.pkg.IntDefTest.STYLE_NO_INPUT, 3, 4}" />
                      <val name="flag" val="true" />
                    </annotation>
                  </item>
                  <item name="test.pkg.IntDefTest void setStyle(int, int) 0">
                    <annotation name="androidx.annotation.IntDef">
                      <val name="value" val="{test.pkg.IntDefTest.STYLE_NORMAL, test.pkg.IntDefTest.STYLE_NO_TITLE, test.pkg.IntDefTest.STYLE_NO_FRAME, test.pkg.IntDefTest.STYLE_NO_INPUT}" />
                    </annotation>
                  </item>
                  <item name="test.pkg.IntDefTest.Inner void setInner(int) 0">
                    <annotation name="androidx.annotation.IntDef">
                      <val name="value" val="{test.pkg.IntDefTest.STYLE_NORMAL, test.pkg.IntDefTest.STYLE_NO_TITLE, test.pkg.IntDefTest.STYLE_NO_FRAME, test.pkg.IntDefTest.STYLE_NO_INPUT, 3, 4}" />
                      <val name="flag" val="true" />
                    </annotation>
                  </item>
                </root>
                """
            )
        )
    }

    @Test
    fun `Check Kotlin and referencing hidden constants from typedef`() {
        check(
            sourceFiles = *arrayOf(
                kotlin(
                    """
                    @file:Suppress("unused", "UseExpressionBody")

                    package test.pkg

                    import android.annotation.LongDef

                    const val STYLE_NORMAL = 0L
                    const val STYLE_NO_TITLE = 1L
                    const val STYLE_NO_FRAME = 2L
                    const val STYLE_NO_INPUT = 3L
                    const val UNRELATED = 3L
                    private const val HIDDEN = 4

                    const val TYPE_1 = "type1"
                    const val TYPE_2 = "type2"
                    const val UNRELATED_TYPE = "other"

                    class LongDefTest {

                        /** @hide */
                        @LongDef(STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT, HIDDEN)
                        @Retention(AnnotationRetention.SOURCE)
                        private annotation class DialogStyle

                        fun setStyle(@DialogStyle style: Int, theme: Int) {}

                        fun testLongDef(arg: Int) {
                        }

                        @LongDef(STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT, 3L, 3L + 1L, flag = true)
                        @Retention(AnnotationRetention.SOURCE)
                        private annotation class DialogFlags

                        fun setFlags(first: Any, @DialogFlags flags: Int) {}

                        class Inner {
                            fun setInner(@DialogFlags flags: Int) {}
                            fun isNull(value: String?): Boolean
                        }
                    }"""
                ).indented(),
                longDefAnnotationSource
            ),
            warnings = "src/test/pkg/LongDefTest.kt:12: error: Typedef class references hidden field field LongDefTestKt.HIDDEN: removed from typedef metadata [HiddenTypedefConstant:148]",
            extractAnnotations = mapOf("test.pkg" to """
                <?xml version="1.0" encoding="UTF-8"?>
                <root>
                  <item name="test.pkg.LongDefTest void setFlags(java.lang.Object, int) 1">
                    <annotation name="androidx.annotation.LongDef">
                      <val name="flag" val="true" />
                      <val name="value" val="{test.pkg.LongDefTestKt.STYLE_NORMAL, test.pkg.LongDefTestKt.STYLE_NO_TITLE, test.pkg.LongDefTestKt.STYLE_NO_FRAME, test.pkg.LongDefTestKt.STYLE_NO_INPUT, 3, 4}" />
                    </annotation>
                  </item>
                  <item name="test.pkg.LongDefTest void setStyle(int, int) 0">
                    <annotation name="androidx.annotation.LongDef">
                      <val name="value" val="{test.pkg.LongDefTestKt.STYLE_NORMAL, test.pkg.LongDefTestKt.STYLE_NO_TITLE, test.pkg.LongDefTestKt.STYLE_NO_FRAME, test.pkg.LongDefTestKt.STYLE_NO_INPUT}" />
                    </annotation>
                  </item>
                  <item name="test.pkg.LongDefTest.Inner void setInner(int) 0">
                    <annotation name="androidx.annotation.LongDef">
                      <val name="flag" val="true" />
                      <val name="value" val="{test.pkg.LongDefTestKt.STYLE_NORMAL, test.pkg.LongDefTestKt.STYLE_NO_TITLE, test.pkg.LongDefTestKt.STYLE_NO_FRAME, test.pkg.LongDefTestKt.STYLE_NO_INPUT, 3, 4}" />
                    </annotation>
                  </item>
                </root>
                """
            )
        )
    }

    @Ignore("Not working reliably")
    @Test
    fun `Include merged annotations in exported source annotations`() {
        check(
            compatibilityMode = false,
            outputKotlinStyleNulls = false,
            includeSystemApiAnnotations = false,
            omitCommonPackages = false,
            warnings = "error: Unexpected reference to Nonexistent.Field [AnnotationExtraction:146]",
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    public class MyTest {
                        public void test(int arg) { }
                    }"""
                )
            ),
            mergeXmlAnnotations = """<?xml version="1.0" encoding="UTF-8"?>
                <root>
                  <item name="test.pkg.MyTest void test(int) 0">
                    <annotation name="org.intellij.lang.annotations.MagicConstant">
                      <val name="intValues" val="{java.util.Calendar.ERA, java.util.Calendar.YEAR, java.util.Calendar.MONTH, java.util.Calendar.WEEK_OF_YEAR, Nonexistent.Field}" />
                    </annotation>
                  </item>
                </root>
                """,
            extractAnnotations = mapOf("test.pkg" to """
                <?xml version="1.0" encoding="UTF-8"?>
                <root>
                  <item name="test.pkg.MyTest void test(int) 0">
                    <annotation name="androidx.annotation.IntDef">
                      <val name="value" val="{java.util.Calendar.ERA, java.util.Calendar.YEAR, java.util.Calendar.MONTH, java.util.Calendar.WEEK_OF_YEAR}" />
                    </annotation>
                  </item>
                </root>
                """
            )
        )
    }
}