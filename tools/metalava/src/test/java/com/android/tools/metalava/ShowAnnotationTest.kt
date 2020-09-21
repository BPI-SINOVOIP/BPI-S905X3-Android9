package com.android.tools.metalava

import org.junit.Test

/** Tests for the --show-annotation functionality */
class ShowAnnotationTest : DriverTest() {

    @Test
    fun `Basic showAnnotation test`() {
        check(
            includeSystemApiAnnotations = true,
            checkDoclava1 = true,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.SystemApi;
                    public class Foo {
                        public void method1() { }

                        /**
                         * @hide Only for use by WebViewProvider implementations
                         */
                        @SystemApi
                        public void method2() { }

                        /**
                         * @hide Always hidden
                         */
                        public void method3() { }

                        @SystemApi
                        public void method4() { }

                    }
                    """
                ),
                java(
                    """
                    package foo.bar;
                    public class Bar {
                    }
                """
                ),
                systemApiSource
            ),

            extraArguments = arrayOf(
                "--hide-package", "android.annotation",
                "--hide-package", "android.support.annotation"
            ),

            api = """
                package test.pkg {
                  public class Foo {
                    method public void method2();
                    method public void method4();
                  }
                }
                """
        )
    }

    @Test
    fun `Basic showAnnotation with showUnannotated test`() {
        check(
            includeSystemApiAnnotations = true,
            showUnannotated = true,
            checkDoclava1 = true,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.SystemApi;
                    public class Foo {
                        public void method1() { }

                        /**
                         * @hide Only for use by WebViewProvider implementations
                         */
                        @SystemApi
                        public void method2() { }

                        /**
                         * @hide Always hidden
                         */
                        public void method3() { }

                        @SystemApi
                        public void method4() { }

                    }
                    """
                ),
                java(
                    """
                    package foo.bar;
                    public class Bar {
                    }
                """
                ),
                systemApiSource
            ),

            extraArguments = arrayOf(
                "--hide-package", "android.annotation",
                "--hide-package", "android.support.annotation"
            ),

            api = """
                package foo.bar {
                  public class Bar {
                    ctor public Bar();
                  }
                }
                package test.pkg {
                  public class Foo {
                    ctor public Foo();
                    method public void method1();
                    method public void method2();
                    method public void method4();
                  }
                }
                """
        )
    }

    @Test
    fun `Check @TestApi handling`() {
        check(
            includeSystemApiAnnotations = true,
            checkDoclava1 = true,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.TestApi;

                    /**
                     * Blah blah blah
                     * @hide
                     */
                    @TestApi
                    public class Bar {
                    }
                    """
                ),
                testApiSource
            ),

            extraArguments = arrayOf(
                "--show-annotation", "android.annotation.TestApi",
                "--hide-package", "android.annotation",
                "--hide-package", "android.support.annotation"
            ),

            api = """
                package test.pkg {
                  public class Bar {
                    ctor public Bar();
                  }
                }
                """
        )
    }
}