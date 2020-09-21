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

import org.junit.Test

class AnnotationsMergerTest : DriverTest() {

    // TODO: Test what happens when we have conflicting data
    //   - NULLABLE_SOURCE on one non null on the other
    //   - annotation specified with different parameters (e.g @Size(4) vs @Size(6))

    @Test
    fun `Signature files contain annotations`() {
        check(
            compatibilityMode = false,
            outputKotlinStyleNulls = false,
            includeSystemApiAnnotations = false,
            omitCommonPackages = false,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    import androidx.annotation.NonNull;
                    import androidx.annotation.Nullable;
                    import android.annotation.IntRange;
                    import androidx.annotation.UiThread;

                    @UiThread
                    public class MyTest {
                        public @Nullable Number myNumber;
                        public @Nullable Double convert(@NonNull Float f) { return null; }
                        public @IntRange(from=10,to=20) int clamp(int i) { return 10; }
                    }"""
                ),
                uiThreadSource,
                intRangeAnnotationSource,
                supportNonNullSource,
                supportNullableSource
            ),
            // Skip the annotations themselves from the output
            extraArguments = arrayOf(
                "--hide-package", "android.annotation",
                "--hide-package", "androidx.annotation",
                "--hide-package", "android.support.annotation"
            ),
            api = """
                package test.pkg {
                  @androidx.annotation.UiThread public class MyTest {
                    ctor public MyTest();
                    method @androidx.annotation.IntRange(from=10, to=20) public int clamp(int);
                    method @androidx.annotation.Nullable public java.lang.Double convert(@androidx.annotation.NonNull java.lang.Float);
                    field @androidx.annotation.Nullable public java.lang.Number myNumber;
                  }
                }
                """
        )
    }

    @Test
    fun `Merged class and method annotations with no arguments`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    public class MyTest {
                        public Number myNumber;
                        public Double convert(Float f) { return null; }
                        public int clamp(int i) { return 10; }
                    }
                    """
                )
            ),
            compatibilityMode = false,
            outputKotlinStyleNulls = false,
            omitCommonPackages = false,
            mergeXmlAnnotations = """<?xml version="1.0" encoding="UTF-8"?>
                <root>
                  <item name="test.pkg.MyTest">
                    <annotation name="android.support.annotation.UiThread" />
                  </item>
                  <item name="test.pkg.MyTest java.lang.Double convert(java.lang.Float)">
                    <annotation name="android.support.annotation.Nullable" />
                  </item>
                  <item name="test.pkg.MyTest java.lang.Double convert(java.lang.Float) 0">
                    <annotation name="android.support.annotation.NonNull" />
                  </item>
                  <item name="test.pkg.MyTest myNumber">
                    <annotation name="android.support.annotation.Nullable" />
                  </item>
                  <item name="test.pkg.MyTest int clamp(int)">
                    <annotation name="android.support.annotation.IntRange">
                      <val name="from" val="10" />
                      <val name="to" val="20" />
                    </annotation>
                  </item>
                  <item name="test.pkg.MyTest int clamp(int) 0">
                    <annotation name='org.jetbrains.annotations.Range'>
                      <val name="from" val="-1"/>
                      <val name="to" val="java.lang.Integer.MAX_VALUE"/>
                    </annotation>
                  </item>
                  </root>
                """,
            api = """
                package test.pkg {
                  @androidx.annotation.UiThread public class MyTest {
                    ctor public MyTest();
                    method @androidx.annotation.IntRange(from=10, to=20) public int clamp(@androidx.annotation.IntRange(from=-1L, to=java.lang.Integer.MAX_VALUE) int);
                    method @androidx.annotation.Nullable public java.lang.Double convert(@androidx.annotation.NonNull java.lang.Float);
                    field @androidx.annotation.Nullable public java.lang.Number myNumber;
                  }
                }
                """
        )
    }

    @Test
    fun `Merge jaif files`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    public interface Appendable {
                        Appendable append(CharSequence csq) throws IOException;
                        String reverse(String s);
                    }
                    """
                )
            ),
            compatibilityMode = false,
            outputKotlinStyleNulls = false,
            omitCommonPackages = false,
            mergeJaifAnnotations = """
                //
                // Copyright (C) 2017 The Android Open Source Project
                //
                package test.pkg:
                class Appendable:
                    method append(Ljava/lang/CharSequence;)Ltest/pkg/Appendable;:
                        parameter #0:
                          type: @libcore.util.Nullable
                        // Is expected to return self
                        return: @libcore.util.NonNull
                """,
            api = """
                package test.pkg {
                  public interface Appendable {
                    method @androidx.annotation.NonNull public test.pkg.Appendable append(@androidx.annotation.Nullable java.lang.CharSequence);
                    method public java.lang.String reverse(java.lang.String);
                  }
                }
                """
        )
    }

    @Test
    fun `Merge signature files`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    public interface Appendable {
                        Appendable append(CharSequence csq) throws IOException;
                    }
                    """
                )
            ),
            compatibilityMode = false,
            outputKotlinStyleNulls = false,
            omitCommonPackages = false,
            mergeSignatureAnnotations = """
                package test.pkg {
                  public interface Appendable {
                    method public test.pkg.Appendable append(java.lang.CharSequence?);
                    method public test.pkg.Appendable append2(java.lang.CharSequence?);
                    method public java.lang.String! reverse(java.lang.String!);
                  }
                  public interface RandomClass {
                    method public test.pkg.Appendable append(java.lang.CharSequence);
                  }
                }
                """,
            api = """
                package test.pkg {
                  public interface Appendable {
                    method @androidx.annotation.NonNull public test.pkg.Appendable append(@androidx.annotation.Nullable java.lang.CharSequence);
                  }
                }
                """
        )
    }
}
