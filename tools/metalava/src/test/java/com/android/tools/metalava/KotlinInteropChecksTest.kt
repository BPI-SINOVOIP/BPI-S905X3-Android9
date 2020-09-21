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

package com.android.tools.metalava

import org.junit.Test

class KotlinInteropChecksTest : DriverTest() {
    @Test
    fun `Hard Kotlin keywords`() {
        check(
            extraArguments = arrayOf("--check-kotlin-interop"),
            warnings = """
                src/test/pkg/Test.java:5: warning: Avoid method names that are Kotlin hard keywords ("fun"); see https://android.github.io/kotlin-guides/interop.html#no-hard-keywords [KotlinKeyword:141]
                src/test/pkg/Test.java:6: warning: Avoid parameter names that are Kotlin hard keywords ("typealias"); see https://android.github.io/kotlin-guides/interop.html#no-hard-keywords [KotlinKeyword:141]
                src/test/pkg/Test.java:7: warning: Avoid field names that are Kotlin hard keywords ("object"); see https://android.github.io/kotlin-guides/interop.html#no-hard-keywords [KotlinKeyword:141]
                """,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.ParameterName;

                    public class Test {
                        public void fun() { }
                        public void foo(int fun, @ParameterName("typealias") int internalName) { }
                        public Object object = null;
                    }
                    """
                ),
                supportParameterName
            )
        )
    }

    @Test
    fun `Sam-compatible parameters should be last`() {
        check(
            extraArguments = arrayOf("--check-kotlin-interop"),
            warnings = """
                src/test/pkg/Test.java:10: warning: SAM-compatible parameters (such as parameter 1, "run", in test.pkg.Test.error) should be last to improve Kotlin interoperability; see https://kotlinlang.org/docs/reference/java-interop.html#sam-conversions [SamShouldBeLast:142]
                src/test/pkg/test.kt:7: warning: lambda parameters (such as parameter 1, "bar", in test.pkg.TestKt.error) should be last to improve Kotlin interoperability; see https://kotlinlang.org/docs/reference/java-interop.html#sam-conversions [SamShouldBeLast:142]
                """,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    public class Test {
                        public void ok1() { }
                        public void ok1(int x) { }
                        public void ok2(int x, int y) { }
                        public void ok3(Runnable run) { }
                        public void ok4(int x, Runnable run) { }
                        public void ok5(Runnable run1, Runnable run2) { }
                        public void ok6(java.util.List list, boolean b) { }
                        public void error(Runnable run, int x) { }
                    }
                    """
                ),
                kotlin(
                    """
                    package test.pkg

                    fun ok1(bar: (Int) -> Int) { }
                    fun ok2(foo: Int) { }
                    fun ok3(foo: Int, bar: (Int) -> Int) { }
                    fun ok4(foo: Int, bar: (Int) -> Int, baz: (Int) -> Int) { }
                    fun error(bar: (Int) -> Int, foo: Int) { }
                """
                )
            )
        )
    }

    @Test
    fun `Companion object methods should be marked with JvmStatic`() {
        check(
            extraArguments = arrayOf("--check-kotlin-interop"),
            warnings = """
                src/test/pkg/Foo.kt:7: warning: Companion object constants like INTEGER_ONE should be marked @JvmField for Java interoperability; see https://android.github.io/kotlin-guides/interop.html#companion-constants [MissingJvmstatic:143]
                src/test/pkg/Foo.kt:13: warning: Companion object methods like missing should be marked @JvmStatic for Java interoperability; see https://android.github.io/kotlin-guides/interop.html#companion-functions [MissingJvmstatic:143]
                """,
            sourceFiles = *arrayOf(
                kotlin(
                    """
                    package test.pkg

                    @SuppressWarnings("all")
                    class Foo {
                        fun ok1() { }
                        companion object {
                            const val INTEGER_ONE = 1
                            var BIG_INTEGER_ONE = BigInteger.ONE
                            @JvmStatic val WRONG = 2 // not yet flagged
                            @JvmStatic @JvmField val WRONG2 = 2 // not yet flagged
                            @JvmField val ok3 = 3

                            fun missing() { }

                            @JvmStatic
                            fun ok2() { }
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Methods with default parameters should specify JvmOverloads`() {
        check(
            extraArguments = arrayOf("--check-kotlin-interop"),
            warnings = """
                src/test/pkg/Foo.kt:8: warning: A Kotlin method with default parameter values should be annotated with @JvmOverloads for better Java interoperability; see https://android.github.io/kotlin-guides/interop.html#function-overloads-for-defaults [MissingJvmstatic:143]
                """,
            sourceFiles = *arrayOf(
                kotlin(
                    """
                    package test.pkg

                    class Foo {
                        fun ok1() { }
                        fun ok2(int: Int) { }
                        fun ok3(int: Int, int2: Int) { }
                        @JvmOverloads fun ok4(int: Int = 0, int2: Int = 0) { }
                        fun error(int: Int = 0, int2: Int = 0) { }
                        fun String.ok4(int: Int = 0, int2: Int = 0) { }
                        inline fun ok5(int: Int, int2: Int) { }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Methods which throw exceptions should document them`() {
        check(
            extraArguments = arrayOf("--check-kotlin-interop"),
            warnings = """
                src/test/pkg/Foo.kt:6: error: Method Foo.error_throws_multiple_times appears to be throwing java.io.FileNotFoundException; this should be recorded with a @Throws annotation; see https://android.github.io/kotlin-guides/interop.html#document-exceptions [DocumentExceptions:145]
                src/test/pkg/Foo.kt:16: error: Method Foo.error_throwsCheckedExceptionWithWrongExceptionClassInThrows appears to be throwing java.io.FileNotFoundException; this should be recorded with a @Throws annotation; see https://android.github.io/kotlin-guides/interop.html#document-exceptions [DocumentExceptions:145]
                src/test/pkg/Foo.kt:37: error: Method Foo.error_throwsRuntimeExceptionDocsMissing appears to be throwing java.lang.UnsupportedOperationException; this should be listed in the documentation; see https://android.github.io/kotlin-guides/interop.html#document-exceptions [DocumentExceptions:145]
                src/test/pkg/Foo.kt:43: error: Method Foo.error_missingSpecificAnnotation appears to be throwing java.lang.UnsupportedOperationException; this should be listed in the documentation; see https://android.github.io/kotlin-guides/interop.html#document-exceptions [DocumentExceptions:145]
                """,
            sourceFiles = *arrayOf(
                kotlin(
                    """
                    package test.pkg
                    import java.io.FileNotFoundException
                    import java.lang.UnsupportedOperationException

                    class Foo {
                        fun error_throws_multiple_times(x: Int) {
                            if (x < 0) {
                                throw java.io.FileNotFoundException("Something")
                            }
                            if (x > 10) { // make sure we don't list this twice
                                throw FileNotFoundException("Something")
                            }
                        }


                        @Throws(Exception::class)
                        fun error_throwsCheckedExceptionWithWrongExceptionClassInThrows(x: Int) {
                            if (x < 0) {
                                throw java.io.FileNotFoundException("Something")
                            }
                        }

                        @Throws(FileNotFoundException::class)
                        fun ok_hasThrows1(x: Int) {
                            if (x < 0) {
                                throw java.io.FileNotFoundException("Something")
                            }
                        }

                        @Throws(UnsupportedOperationException::class, FileNotFoundException::class)
                        fun ok_hasThrows2(x: Int) {
                            if (x < 0) {
                                throw java.io.FileNotFoundException("Something")
                            }
                        }

                        fun error_throwsRuntimeExceptionDocsMissing(x: Int) {
                            if (x < 0) {
                                throw UnsupportedOperationException("Something")
                            }
                        }

                        /** This method throws FileNotFoundException if blah blah blah */
                        fun error_missingSpecificAnnotation(x: Int) {
                            if (x < 0) {
                                throw UnsupportedOperationException("Something")
                            }
                        }

                        /** This method throws UnsupportedOperationException if blah blah blah */
                        fun ok_docsPresent(x: Int) {
                            if (x < 0) {
                                throw UnsupportedOperationException("Something")
                            }
                        }

                        fun ok_exceptionCaught(x: Int) {
                            try {
                                if (s.startsWith(" ")) {
                                    throw NumberFormatException()
                                }
                                println("Hello")
                            } catch (e: NumberFormatException) {}
                        }

                        fun ok_exceptionCaught2(x: Int) {
                            try {
                                if (s.startsWith(" ")) {
                                    throw NumberFormatException()
                                }
                                println("Hello")
                            } catch (e: Exception) {}
                        }

                        // TODO: What about something where you call in Java a method
                        // known to throw something (e.g. Integer.parseInt) and you don't catch it; should you
                        // pass it on? Hard to say; if the logic is complicated it may
                        // be the case that it can never happen, and this might be an annoying false positive.
                    }
                    """
                )
            )
        )
    }
}
