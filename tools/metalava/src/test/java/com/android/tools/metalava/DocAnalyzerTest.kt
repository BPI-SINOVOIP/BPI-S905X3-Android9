package com.android.tools.metalava

import com.android.tools.lint.checks.infrastructure.TestFiles.source
import com.android.tools.metalava.model.psi.trimDocIndent
import com.google.common.io.Files
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Ignore
import org.junit.Test
import java.io.File

/** Tests for the [DocAnalyzer] which enhances the docs */
class DocAnalyzerTest : DriverTest() {
    // TODO: Test @StringDef

    @Test
    fun `Basic documentation generation test`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.Nullable;
                    import android.annotation.NonNull;
                    public class Foo {
                        /** These are the docs for method1. */
                        @Nullable public Double method1(@NonNull Double factor1, @NonNull Double factor2) { }
                        /** These are the docs for method2. It can sometimes return null. */
                        @Nullable public Double method2(@NonNull Double factor1, @NonNull Double factor2) { }
                        @Nullable public Double method3(@NonNull Double factor1, @NonNull Double factor2) { }
                    }
                    """
                ),

                nonNullSource,
                nullableSource
            ),
            checkCompilation = false, // needs androidx.annotations in classpath
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class Foo {
                public Foo() { throw new RuntimeException("Stub!"); }
                /**
                 * These are the docs for method1.
                 * @param factor1 This value must never be {@code null}.
                 * @param factor2 This value must never be {@code null}.
                 * @return This value may be {@code null}.
                 */
                @androidx.annotation.Nullable public java.lang.Double method1(@androidx.annotation.NonNull java.lang.Double factor1, @androidx.annotation.NonNull java.lang.Double factor2) { throw new RuntimeException("Stub!"); }
                /**
                 * These are the docs for method2. It can sometimes return null.
                 * @param factor1 This value must never be {@code null}.
                 * @param factor2 This value must never be {@code null}.
                 */
                @androidx.annotation.Nullable public java.lang.Double method2(@androidx.annotation.NonNull java.lang.Double factor1, @androidx.annotation.NonNull java.lang.Double factor2) { throw new RuntimeException("Stub!"); }
                /**
                 * @param factor1 This value must never be {@code null}.
                 * @param factor2 This value must never be {@code null}.
                 * @return This value may be {@code null}.
                 */
                @androidx.annotation.Nullable public java.lang.Double method3(@androidx.annotation.NonNull java.lang.Double factor1, @androidx.annotation.NonNull java.lang.Double factor2) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Fix first sentence handling`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.annotation;

                    import static java.lang.annotation.ElementType.*;
                    import static java.lang.annotation.RetentionPolicy.CLASS;
                    import java.lang.annotation.*;

                    /**
                     * Denotes that an integer parameter, field or method return value is expected
                     * to be a String resource reference (e.g. {@code android.R.string.ok}).
                     */
                    @Documented
                    @Retention(CLASS)
                    @Target({METHOD, PARAMETER, FIELD, LOCAL_VARIABLE})
                    public @interface StringRes {
                    }
                    """
                )
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package android.annotation;
                /**
                 * Denotes that an integer parameter, field or method return value is expected
                 * to be a String resource reference (e.g.&nbsp;{@code android.R.string.ok}).
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public @interface StringRes {
                }
                """
            )
        )
    }

    @Test
    fun `Fix typo replacement`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    /** This is an API for Andriod */
                    public class Foo {
                    }
                    """
                )
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            warnings = "src/test/pkg/Foo.java:2: warning: Replaced Andriod with Android in documentation for class test.pkg.Foo [Typo:131]",
            stubs = arrayOf(
                """
                package test.pkg;
                /** This is an API for Android */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class Foo {
                public Foo() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Document Permissions`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    import android.Manifest;
                    import android.annotation.RequiresPermission;

                    public class PermissionTest {
                        @RequiresPermission(Manifest.permission.ACCESS_COARSE_LOCATION)
                        public void test1() {
                        }

                        @RequiresPermission(allOf = Manifest.permission.ACCESS_COARSE_LOCATION)
                        public void test2() {
                        }

                        @RequiresPermission(anyOf = {Manifest.permission.ACCESS_COARSE_LOCATION, Manifest.permission.ACCESS_FINE_LOCATION})
                        public void test3() {
                        }

                        @RequiresPermission(allOf = {Manifest.permission.ACCESS_COARSE_LOCATION, Manifest.permission.ACCOUNT_MANAGER})
                        public void test4() {
                        }

                        @RequiresPermission(value=Manifest.permission.WATCH_APPOPS, conditional=true) // b/73559440
                        public void test5() {
                        }
                    }
                    """
                ),
                java(
                    """
                    package android;

                    public abstract class Manifest {
                        public static final class permission {
                            public static final String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                            public static final String ACCESS_FINE_LOCATION = "android.permission.ACCESS_FINE_LOCATION";
                            public static final String ACCOUNT_MANAGER = "android.permission.ACCOUNT_MANAGER";
                            public static final String WATCH_APPOPS = "android.permission.WATCH_APPOPS";
                        }
                    }
                    """
                ),
                requiresPermissionSource
            ),
            checkCompilation = false, // needs androidx.annotations in classpath
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                import android.Manifest;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class PermissionTest {
                public PermissionTest() { throw new RuntimeException("Stub!"); }
                /**
                 * Requires {@link android.Manifest.permission#ACCESS_COARSE_LOCATION}
                 */
                @androidx.annotation.RequiresPermission(android.Manifest.permission.ACCESS_COARSE_LOCATION) public void test1() { throw new RuntimeException("Stub!"); }
                /**
                 * Requires {@link android.Manifest.permission#ACCESS_COARSE_LOCATION}
                 */
                @androidx.annotation.RequiresPermission(allOf=android.Manifest.permission.ACCESS_COARSE_LOCATION) public void test2() { throw new RuntimeException("Stub!"); }
                /**
                 * Requires {@link android.Manifest.permission#ACCESS_COARSE_LOCATION} or {@link android.Manifest.permission#ACCESS_FINE_LOCATION}
                 */
                @androidx.annotation.RequiresPermission(anyOf={android.Manifest.permission.ACCESS_COARSE_LOCATION, android.Manifest.permission.ACCESS_FINE_LOCATION}) public void test3() { throw new RuntimeException("Stub!"); }
                /**
                 * Requires {@link android.Manifest.permission#ACCESS_COARSE_LOCATION} and {@link android.Manifest.permission#ACCOUNT_MANAGER}
                 */
                @androidx.annotation.RequiresPermission(allOf={android.Manifest.permission.ACCESS_COARSE_LOCATION, android.Manifest.permission.ACCOUNT_MANAGER}) public void test4() { throw new RuntimeException("Stub!"); }
                @androidx.annotation.RequiresPermission(value=android.Manifest.permission.WATCH_APPOPS, conditional=true) public void test5() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Document ranges`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;

                    import android.Manifest;
                    import android.annotation.IntRange;

                    public class RangeTest {
                        @IntRange(from = 10)
                        public int test1(@IntRange(from = 20) int range2) { return 15; }

                        @IntRange(from = 10, to = 20)
                        public int test2() { return 15; }

                        @IntRange(to = 100)
                        public int test3() { return 50; }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * @param range2 Value is 20 or greater
                 * @return Value is 10 or greater
                 */
                @androidx.annotation.IntRange(from=10) public int test1(@androidx.annotation.IntRange(from=20) int range2) { throw new RuntimeException("Stub!"); }
                /**
                 * @return Value is between 10 and 20 inclusive
                 */
                @androidx.annotation.IntRange(from=10, to=20) public int test2() { throw new RuntimeException("Stub!"); }
                /**
                 * @return Value is 100 or less
                 */
                @androidx.annotation.IntRange(to=100) public int test3() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Merging in documentation snippets from annotation memberDoc and classDoc`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.UiThread;
                    import androidx.annotation.WorkerThread;
                    @UiThread
                    public class RangeTest {
                        @WorkerThread
                        public int test1() { }
                    }
                    """
                ),
                uiThreadSource,
                workerThreadSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                /** Methods in this class must be called on the thread that originally created
                 *            this UI element, unless otherwise noted. This is typically the
                 *            main thread of your app. * */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                @androidx.annotation.UiThread public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /** This method may take several seconds to complete, so it should
                 *            only be called from a worker thread. */
                @androidx.annotation.WorkerThread public int test1() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Warn about multiple threading annotations`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.UiThread;
                    import androidx.annotation.WorkerThread;
                    public class RangeTest {
                        @UiThread @WorkerThread
                        public int test1() { }
                    }
                    """
                ),
                uiThreadSource,
                workerThreadSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            warnings = "src/test/pkg/RangeTest.java:5: warning: Found more than one threading annotation on method test.pkg.RangeTest.test1(); the auto-doc feature does not handle this correctly [MultipleThreadAnnotations:133]",
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /** This method must be called on the thread that originally created
                 *            this UI element. This is typically the main thread of your app.
                 * This method may take several seconds to complete, so it should
                 *  *            only be called from a worker thread.
                 */
                @androidx.annotation.UiThread @androidx.annotation.WorkerThread public int test1() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Typedefs`() {
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
                    public class TypedefTest {
                        @IntDef({STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT})
                        @Retention(RetentionPolicy.SOURCE)
                        private @interface DialogStyle {}

                        public static final int STYLE_NORMAL = 0;
                        public static final int STYLE_NO_TITLE = 1;
                        public static final int STYLE_NO_FRAME = 2;
                        public static final int STYLE_NO_INPUT = 3;
                        public static final int STYLE_UNRELATED = 3;

                        public void setStyle(@DialogStyle int style, int theme) {
                        }

                        @IntDef(value = {STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT, 2, 3 + 1},
                        flag=true)
                        @Retention(RetentionPolicy.SOURCE)
                        private @interface DialogFlags {}

                        public void setFlags(Object first, @DialogFlags int flags) {
                        }
                    }
                    """
                ),
                intRangeAnnotationSource,
                intDefAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class TypedefTest {
                public TypedefTest() { throw new RuntimeException("Stub!"); }
                /**
                 * @param style Value is {@link test.pkg.TypedefTest#STYLE_NORMAL}, {@link test.pkg.TypedefTest#STYLE_NO_TITLE}, {@link test.pkg.TypedefTest#STYLE_NO_FRAME}, or {@link test.pkg.TypedefTest#STYLE_NO_INPUT}
                 */
                public void setStyle(int style, int theme) { throw new RuntimeException("Stub!"); }
                /**
                 * @param flags Value is either <code>0</code> or a combination of {@link test.pkg.TypedefTest#STYLE_NORMAL}, {@link test.pkg.TypedefTest#STYLE_NO_TITLE}, {@link test.pkg.TypedefTest#STYLE_NO_FRAME}, {@link test.pkg.TypedefTest#STYLE_NO_INPUT}, 2, and 3 + 1
                 */
                public void setFlags(java.lang.Object first, int flags) { throw new RuntimeException("Stub!"); }
                public static final int STYLE_NORMAL = 0; // 0x0
                public static final int STYLE_NO_FRAME = 2; // 0x2
                public static final int STYLE_NO_INPUT = 3; // 0x3
                public static final int STYLE_NO_TITLE = 1; // 0x1
                public static final int STYLE_UNRELATED = 3; // 0x3
                }
                """
            )
        )
    }

    @Test
    fun `Typedefs combined with ranges`() {
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
                    public class TypedefTest {
                        @IntDef({STYLE_NORMAL, STYLE_NO_TITLE, STYLE_NO_FRAME, STYLE_NO_INPUT})
                        @IntRange(from = 20)
                        @Retention(RetentionPolicy.SOURCE)
                        private @interface DialogStyle {}

                        public static final int STYLE_NORMAL = 0;
                        public static final int STYLE_NO_TITLE = 1;
                        public static final int STYLE_NO_FRAME = 2;

                        public void setStyle(@DialogStyle int style, int theme) {
                        }
                    }
                    """
                ),
                intRangeAnnotationSource,
                intDefAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class TypedefTest {
                public TypedefTest() { throw new RuntimeException("Stub!"); }
                /**
                 * @param style Value is {@link test.pkg.TypedefTest#STYLE_NORMAL}, {@link test.pkg.TypedefTest#STYLE_NO_TITLE}, {@link test.pkg.TypedefTest#STYLE_NO_FRAME}, or STYLE_NO_INPUT
                 * Value is 20 or greater
                 */
                public void setStyle(int style, int theme) { throw new RuntimeException("Stub!"); }
                public static final int STYLE_NORMAL = 0; // 0x0
                public static final int STYLE_NO_FRAME = 2; // 0x2
                public static final int STYLE_NO_TITLE = 1; // 0x1
                }
                """
            )
        )
    }

    @Test
    fun `Create method documentation from nothing`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresPermission;
                    @SuppressWarnings("WeakerAccess")
                    public class RangeTest {
                        public static final String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                        @RequiresPermission(ACCESS_COARSE_LOCATION)
                        public void test1() {
                        }
                    }
                    """
                ),
                requiresPermissionSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * Requires {@link test.pkg.RangeTest#ACCESS_COARSE_LOCATION}
                 */
                @androidx.annotation.RequiresPermission(test.pkg.RangeTest.ACCESS_COARSE_LOCATION) public void test1() { throw new RuntimeException("Stub!"); }
                public static final java.lang.String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                }
                """
            )
        )
    }

    @Test
    fun `Warn about missing field`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresPermission;
                    public class RangeTest {
                        @RequiresPermission("MyPermission")
                        public void test1() {
                        }
                    }
                    """
                ),
                requiresPermissionSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            warnings = "src/test/pkg/RangeTest.java:4: lint: Cannot find permission field for \"MyPermission\" required by method test.pkg.RangeTest.test1() (may be hidden or removed) [MissingPermission:132]",
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * Requires "MyPermission"
                 */
                @androidx.annotation.RequiresPermission("MyPermission") public void test1() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Add to existing single-line method documentation`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresPermission;
                    @SuppressWarnings("WeakerAccess")
                    public class RangeTest {
                        public static final String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                        /** This is the existing documentation. */
                        @RequiresPermission(ACCESS_COARSE_LOCATION)
                        public int test1() { }
                    }
                    """
                ),
                requiresPermissionSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * Requires {@link test.pkg.RangeTest#ACCESS_COARSE_LOCATION}
                 */
                @androidx.annotation.RequiresPermission(test.pkg.RangeTest.ACCESS_COARSE_LOCATION) public int test1() { throw new RuntimeException("Stub!"); }
                public static final java.lang.String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                }
                """
            )
        )
    }

    @Test
    fun `Add to existing multi-line method documentation`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresPermission;
                    @SuppressWarnings("WeakerAccess")
                    public class RangeTest {
                        public static final String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                        /**
                         * This is the existing documentation.
                         * Multiple lines of it.
                         */
                        @RequiresPermission(ACCESS_COARSE_LOCATION)
                        public int test1() { }
                    }
                    """
                ),
                requiresPermissionSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * Multiple lines of it.
                 * Requires {@link test.pkg.RangeTest#ACCESS_COARSE_LOCATION}
                 */
                @androidx.annotation.RequiresPermission(test.pkg.RangeTest.ACCESS_COARSE_LOCATION) public int test1() { throw new RuntimeException("Stub!"); }
                public static final java.lang.String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                }
                """
            )
        )
    }

    @Test
    fun `Add new parameter when no doc exists`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.IntRange;
                    public class RangeTest {
                        public int test1(int parameter1, @IntRange(from = 10) int parameter2, int parameter3) { }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * @param parameter2 Value is 10 or greater
                 */
                public int test1(int parameter1, @androidx.annotation.IntRange(from=10) int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Add to method when there are existing parameter docs and appear before these`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresPermission;
                    @SuppressWarnings("WeakerAccess")
                    public class RangeTest {
                        public static final String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                        /**
                        * This is the existing documentation.
                        * @param parameter1 docs for parameter1
                        * @param parameter2 docs for parameter2
                        * @param parameter3 docs for parameter2
                        * @return return value documented here
                        */
                        @RequiresPermission(ACCESS_COARSE_LOCATION)
                        public int test1(int parameter1, int parameter2, int parameter3) { }
                    }
                        """
                ),
                requiresPermissionSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * Requires {@link test.pkg.RangeTest#ACCESS_COARSE_LOCATION}
                 * @param parameter1 docs for parameter1
                 * @param parameter2 docs for parameter2
                 * @param parameter3 docs for parameter2
                 * @return return value documented here
                 */
                @androidx.annotation.RequiresPermission(test.pkg.RangeTest.ACCESS_COARSE_LOCATION) public int test1(int parameter1, int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                public static final java.lang.String ACCESS_COARSE_LOCATION = "android.permission.ACCESS_COARSE_LOCATION";
                }
                """
            )
        )
    }

    @Test
    fun `Add new parameter when doc exists but no param doc`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.IntRange;
                    public class RangeTest {
                        /**
                        * This is the existing documentation.
                        * @return return value documented here
                        */
                        public int test1(int parameter1, @IntRange(from = 10) int parameter2, int parameter3) { }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * @param parameter2 Value is 10 or greater
                 * @return return value documented here
                 */
                public int test1(int parameter1, @androidx.annotation.IntRange(from=10) int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Add new parameter, sorted correctly between existing ones`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.IntRange;
                    public class RangeTest {
                        /**
                        * This is the existing documentation.
                        * @param parameter1 docs for parameter1
                        * @param parameter3 docs for parameter2
                        * @return return value documented here
                        */
                        public int test1(int parameter1, @IntRange(from = 10) int parameter2, int parameter3) { }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * @param parameter1 docs for parameter1
                 * @param parameter3 docs for parameter2
                 * @param parameter2 Value is 10 or greater
                 * @return return value documented here
                 */
                public int test1(int parameter1, @androidx.annotation.IntRange(from=10) int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Add to existing parameter`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.IntRange;
                    public class RangeTest {
                        /**
                        * This is the existing documentation.
                        * @param parameter1 docs for parameter1
                        * @param parameter2 docs for parameter2
                        * @param parameter3 docs for parameter2
                        * @return return value documented here
                        */
                        public int test1(int parameter1, @IntRange(from = 10) int parameter2, int parameter3) { }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * @param parameter1 docs for parameter1
                 * @param parameter2 docs for parameter2
                 * Value is 10 or greater
                 * @param parameter3 docs for parameter2
                 * @return return value documented here
                 */
                public int test1(int parameter1, @androidx.annotation.IntRange(from=10) int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Add new return value`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.IntRange;
                    public class RangeTest {
                        @IntRange(from = 10)
                        public int test1(int parameter1, int parameter2, int parameter3) { }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * @return Value is 10 or greater
                 */
                @androidx.annotation.IntRange(from=10) public int test1(int parameter1, int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Add to existing return value (ensuring it appears last)`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.IntRange;
                    public class RangeTest {
                        /**
                        * This is the existing documentation.
                        * @return return value documented here
                        */
                        @IntRange(from = 10)
                        public int test1(int parameter1, int parameter2, int parameter3) { }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class RangeTest {
                public RangeTest() { throw new RuntimeException("Stub!"); }
                /**
                 * This is the existing documentation.
                 * @return return value documented here
                 * Value is 10 or greater
                 */
                @androidx.annotation.IntRange(from=10) public int test1(int parameter1, int parameter2, int parameter3) { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `test documentation trim utility`() {
        assertEquals(
            "/**\n * This is a comment\n * This is a second comment\n */",
            trimDocIndent(
                """/**
         * This is a comment
         * This is a second comment
         */
        """.trimIndent()
            )
        )
    }

    @Ignore("This test for some reason is flaky; it works when run directly but sometimes fails when running all tests")
    @Test
    fun `Merge API levels`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.widget;

                    public class Toolbar {
                        /**
                        * Existing documentation for {@linkplain #getCurrentContentInsetEnd()} here.
                        * @return blah blah blah
                        */
                        public int getCurrentContentInsetEnd() {
                            return 0;
                        }
                    }
                    """
                ),
                intRangeAnnotationSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            applyApiLevelsXml = """
                    <?xml version="1.0" encoding="utf-8"?>
                    <api version="2">
                        <class name="android/widget/Toolbar" since="21">
                            <method name="&lt;init>(Landroid/content/Context;)V"/>
                            <method name="collapseActionView()V"/>
                            <method name="getContentInsetStartWithNavigation()I" since="24"/>
                            <method name="getCurrentContentInsetEnd()I" since="24"/>
                            <method name="getCurrentContentInsetLeft()I" since="24"/>
                            <method name="getCurrentContentInsetRight()I" since="24"/>
                            <method name="getCurrentContentInsetStart()I" since="24"/>
                        </class>
                    </api>
                    """,
            stubs = arrayOf(
                """
                package android.widget;
                /**
                 * Requires API level 21
                 * @since 5.0 Lollipop (21)
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class Toolbar {
                public Toolbar() { throw new RuntimeException("Stub!"); }
                /**
                 * Existing documentation for {@linkplain #getCurrentContentInsetEnd()} here.
                 * Requires API level 24
                 * @since 7.0 Nougat (24)
                 * @return blah blah blah
                 */
                public int getCurrentContentInsetEnd() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Merge deprecation levels`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.hardware;
                    /**
                     * The Camera class is used to set image capture settings, start/stop preview.
                     *
                     * @deprecated We recommend using the new {@link android.hardware.camera2} API for new
                     *             applications.*
                    */
                    @Deprecated
                    public class Camera {
                       /** @deprecated Use something else. */
                       public static final String ACTION_NEW_VIDEO = "android.hardware.action.NEW_VIDEO";
                    }
                    """
                )
            ),
            applyApiLevelsXml = """
                    <?xml version="1.0" encoding="utf-8"?>
                    <api version="2">
                        <class name="android/hardware/Camera" since="1" deprecated="21">
                            <method name="&lt;init>()V"/>
                            <method name="addCallbackBuffer([B)V" since="8"/>
                            <method name="getLogo()Landroid/graphics/drawable/Drawable;"/>
                            <field name="ACTION_NEW_VIDEO" since="14" deprecated="25"/>
                        </class>
                    </api>
                    """,
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package android.hardware;
                /**
                 * The Camera class is used to set image capture settings, start/stop preview.
                 *
                 * @deprecated
                 * <p class="caution"><strong>This class was deprecated in API level 21.</strong></p>
                 *  We recommend using the new {@link android.hardware.camera2} API for new
                 *             applications.*
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                @Deprecated public class Camera {
                public Camera() { throw new RuntimeException("Stub!"); }
                /**
                 *
                 * Requires API level 14
                 * @deprecated
                 * <p class="caution"><strong>This class was deprecated in API level 21.</strong></p>
                 *  Use something else.
                 * @since 4.0 IceCreamSandwich (14)
                 */
                @Deprecated public static final java.lang.String ACTION_NEW_VIDEO = "android.hardware.action.NEW_VIDEO";
                }
                """
            )
        )
    }

    @Test
    fun `Generate overview html docs`() {
        // If a codebase provides overview.html files in the a public package,
        // make sure that we include this in the exported stubs folder as well!
        check(
            checkDoclava1 = false,
            sourceFiles = *arrayOf(
                source("src/test/visible/overview.html", "<html>My docs</html>"),
                java(
                    """
                    package test.visible;
                    public class MyClass {
                        public void test() { }
                    }
                    """
                )
            ),
            stubs = arrayOf(
                """
                <html>My docs</html>
                """,
                """
                package test.visible;
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class MyClass {
                public MyClass() { throw new RuntimeException("Stub!"); }
                public void test() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Check RequiresFeature handling`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresFeature;
                    import android.content.pm.PackageManager;
                    @SuppressWarnings("WeakerAccess")
                    @RequiresFeature(PackageManager.FEATURE_LOCATION)
                    public class LocationManager {
                    }
                    """
                ),
                java(
                    """
                    package android.content.pm;
                    public abstract class PackageManager {
                        public static final String FEATURE_LOCATION = "android.hardware.location";
                    }
                    """
                ),

                requiresFeatureSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                import android.content.pm.PackageManager;
                /**
                 * Requires the {@link android.content.pm.PackageManager#FEATURE_LOCATION PackageManager#FEATURE_LOCATION} feature which can be detected using {@link android.content.pm.PackageManager#hasSystemFeature(String) PackageManager.hasSystemFeature(String)}.
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class LocationManager {
                public LocationManager() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Check RequiresApi handling`() {
        check(
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import androidx.annotation.RequiresApi;
                    @RequiresApi(value = 21)
                    public class MyClass1 {
                    }
                    """
                ),

                requiresApiSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                /**
                 * Requires API level 21
                 * @since 5.0 Lollipop (21)
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                @androidx.annotation.RequiresApi(21) public class MyClass1 {
                public MyClass1() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )
    }

    @Test
    fun `Invoke external documentation tool`() {
        val jdkPath = getJdkPath()
        if (jdkPath == null) {
            println("Ignoring external doc test: JDK not found")
            return
        }
        val javadoc = File(jdkPath, "bin/javadoc")
        if (!javadoc.isFile) {
            println("Ignoring external doc test: javadoc not found *or* not running on Linux/OSX")
            return
        }
        val androidJar = getAndroidJar(API_LEVEL)?.path
        if (androidJar == null) {
            println("Ignoring external doc test: android.jar not found")
            return
        }

        val dir = Files.createTempDir()
        val html = "$dir/javadoc"
        val sourceList = "$dir/sources.txt"

        check(
            extraArguments = arrayOf(
                "--write-stubs-source-list",
                sourceList,
                "--generate-documentation",
                javadoc.path,
                "-sourcepath",
                "STUBS_DIR",
                "-d",
                html,
                "-bootclasspath",
                androidJar,
                "STUBS_SOURCE_LIST"
            ),
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg;
                    import android.annotation.RequiresFeature;
                    import android.content.pm.PackageManager;
                    @SuppressWarnings("WeakerAccess")
                    @RequiresFeature(PackageManager.FEATURE_LOCATION)
                    public class LocationManager {
                    }
                    """
                ),
                java(
                    """
                    package android.content.pm;
                    public abstract class PackageManager {
                        public static final String FEATURE_LOCATION = "android.hardware.location";
                        public boolean hasSystemFeature(String name) {
                            return false;
                        }
                    }
                    """
                ),

                requiresFeatureSource
            ),
            checkCompilation = true,
            checkDoclava1 = false,
            stubs = arrayOf(
                """
                package test.pkg;
                import android.content.pm.PackageManager;
                /**
                 * Requires the {@link android.content.pm.PackageManager#FEATURE_LOCATION PackageManager#FEATURE_LOCATION} feature which can be detected using {@link android.content.pm.PackageManager#hasSystemFeature(String) PackageManager.hasSystemFeature(String)}.
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class LocationManager {
                public LocationManager() { throw new RuntimeException("Stub!"); }
                }
                """
            )
        )

        val doc = File(html, "test/pkg/LocationManager.html").readText(Charsets.UTF_8)
        assertTrue(
            "Did not find matching javadoc fragment in LocationManager.html: actual content is\n$doc",
            doc.contains(
                """
                <hr>
                <br>
                <pre>public class <span class="typeNameLabel">LocationManager</span>
                extends java.lang.Object</pre>
                <div class="block">Requires the <a href="../../android/content/pm/PackageManager.html#FEATURE_LOCATION"><code>PackageManager#FEATURE_LOCATION</code></a> feature which can be detected using <a href="../../android/content/pm/PackageManager.html#hasSystemFeature-java.lang.String-"><code>PackageManager.hasSystemFeature(String)</code></a>.</div>
                </li>
                </ul>
                """.trimIndent()
            )
        )

        dir.deleteRecursively()
    }
}