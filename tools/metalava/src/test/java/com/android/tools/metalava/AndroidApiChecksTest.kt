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

class AndroidApiChecksTest : DriverTest() {
    @Test
    fun `Flag TODO documentation`() {
        check(
            warnings = """
                src/android/pkg/Test.java:3: lint: Documentation mentions 'TODO' [Todo:128]
                src/android/pkg/Test.java:5: lint: Documentation mentions 'TODO' [Todo:128]
                """,
            sourceFiles = *arrayOf(
                // Nothing in outside of Android
                java(
                    """
                    package test.pkg;
                    /** TODO: Some comment here */
                    public class Ignored1 {
                    }
                    """
                ),
                // Nothing in android.icu
                java(
                    """
                    package android.icu;
                    /** TODO: Some comment here */
                    public class Ignored2 {
                    }
                    """
                ),
                java(
                    """
                    package android.pkg;

                    /** TODO: Some comment here */
                    public class Test {
                       /** TODO(ldap): Some comment here */
                        public void fun() {
                            // TODO: Code doesn't count
                        }
                    }
                    """
                )
            )
        )
    }

    @Test
    fun `Document Permissions`() {
        check(
            warnings = """
                src/android/pkg/PermissionTest.java:10: lint: Method 'test0' documentation mentions permissions without declaring @RequiresPermission [RequiresPermission:125]
                src/android/pkg/PermissionTest.java:19: lint: Method 'test1' documentation mentions permissions already declared by @RequiresPermission [RequiresPermission:125]
                """,
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.pkg;

                    import android.Manifest;
                    import android.annotation.RequiresPermission;

                    public class PermissionTest {

                        // Flag methods that talk about permissions in the documentation
                        // but isn't annotated
                        /**
                        * Blah blah.
                        * Requires permission: {@link android.Manifest.permission#READ_PHONE_STATE}
                        */
                        public void test0() {
                        }

                        // Flag methods which has permission annotation, but has
                        // documentation mentioning other permissions that are not listed
                        /** Blah blah blah ACCESS_COARSE_LOCATION */
                        @RequiresPermission(Manifest.permission.ACCESS_COARSE_LOCATION)
                        public void test1() {
                        }

                        // TODO: Flag methods which has permission annotation, but where one
                        // of the permissions is annotated but not mentioned
                        @RequiresPermission(allOf = Manifest.permission.ACCESS_COARSE_LOCATION)
                        public void test2() {
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
                        }
                    }
                    """
                ),
                requiresPermissionSource
            ),
            checkDoclava1 = false
        )
    }

    @Test
    fun `Document Intent Actions`() {
        check(
            warnings = """
                src/android/pkg/IntentActionTest.java:27: lint: Field 'BAR_FOO_ERROR_ACTION' is missing @SdkConstant(SdkConstantType.ACTIVITY_INTENT_ACTION) [SdkConstant:127]
                src/android/pkg/IntentActionTest.java:16: lint: Field 'FOO_BAR_ERROR_ACTION' is missing @BroadcastBehavior [BroadcastBehavior:126]
                src/android/pkg/IntentActionTest.java:16: lint: Field 'FOO_BAR_ERROR_ACTION' is missing @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION) [SdkConstant:127]
                """,
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.pkg;

                    import android.Manifest;
                    import android.annotation.SdkConstant;
                    import android.annotation.SdkConstant.SdkConstantType;
                    import android.annotation.BroadcastBehavior;

                    public class IntentActionTest {
                        /**
                         * Broadcast Action: Foo Bar has started.
                         */
                        @SdkConstant(SdkConstantType.BROADCAST_INTENT_ACTION)
                        @BroadcastBehavior(includeBackground = true)
                        public static final String FOO_BAR_OK_ACTION = "android.something.FOO_BAR";

                        /**
                         * Broadcast Action: Foo Bar has started.
                         */
                        public static final String FOO_BAR_ERROR_ACTION = "android.something.FOO_BAR";

                        /**
                         * Activity Action: Bar the Foo.
                         */
                        @SdkConstant(SdkConstantType.ACTIVITY_INTENT_ACTION)
                        public static final String BAR_FOO_OK_ACTION = "android.something.BAR_FOO";

                        /**
                         * Activity Action: Bar the Foo.
                         */
                        public static final String BAR_FOO_ERROR_ACTION = "android.something.BAR_FOO";
                    }
                    """
                ),
                sdkConstantSource,
                broadcastBehaviorSource
            ),
            checkDoclava1 = false
        )
    }

    @Test
    fun `Check Warnings for missing nullness annotations`() {
        check(
            warnings = """
                src/android/pkg/NullMentions.java:18: warning: Parameter 'param1' of 'method3' documentation mentions 'null' without declaring @NonNull or @Nullable [Nullable:123]
                src/android/pkg/NullMentions.java:19: warning: Return value of 'method4' documentation mentions 'null' without declaring @NonNull or @Nullable [Nullable:123]
                src/android/pkg/NullMentions.java:8: warning: Field 'field2' documentation mentions 'null' without declaring @NonNull or @Nullable [Nullable:123]
                """,
            extraArguments = arrayOf("--warning", "Nullable"), // Hidden by default
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.pkg;

                    import android.annotation.Nullable;

                    public class NullMentions {
                        /** Blah blah */
                        public Object field1;
                        /** Blah blah. Never null. */
                        public Object field2;
                        /** Blah blah */
                        public Object method1() { return null; }
                        /** Blah blah. Sometimes null. */
                        public Object method2() { return null; }
                        /** Blah blah. Never null. */
                        public Object method2(Object param1) { return null; }
                        /** Blah blah. Never null.
                         *  @param param1 Sometimes null. */
                        public Object method3(Object param1) { return null; }
                        /** Blah blah. Never null.
                         *  @return Sometimes null. */
                        public Object method4(Object param1) { return null; }
                        /** Blah blah. Never null.
                         *  @param param1 Sometimes null.
                         *  @return Sometimes null. */
                        public @Nullable Object method5(@Nullable Object param1) { return null; }
                    }
                    """
                ),
                nullableSource
            ),
            checkDoclava1 = false
        )
    }

    @Test
    fun `Check IntDef Warnings`() {
        check(
            warnings = """
                src/android/pkg/NullMentions.java:15: warning: Field 'field1' documentation mentions constants without declaring an @IntDef [IntDef:124]
                """,
            extraArguments = arrayOf("--warning", "IntDef"), // Hidden by default
            sourceFiles = *arrayOf(
                java(
                    """
                    package android.pkg;

                    import android.annotation.IntDef;
                    import android.annotation.Nullable;
                    import java.lang.annotation.Retention;
                    import java.lang.annotation.RetentionPolicy;

                    public class NullMentions {
                        @IntDef({CONSTANT_ONE, CONSTANT_TWO})
                        @Retention(RetentionPolicy.SOURCE)
                        private @interface MyStyle {}

                        public static final int CONSTANT_ONE = 1;
                        public static final int CONSTANT_TWO = 12;
                        /** Should be CONSTANT_ONE or CONSTANT_TWO */
                        public int field1; // WARN

                        /** Should be CONSTANT_ONE or CONSTANT_TWO */
                        @MyStyle
                        public int field2; // OK
                    }
                    """
                ),
                intDefAnnotationSource
            ),
            checkDoclava1 = false
        )
    }
}
