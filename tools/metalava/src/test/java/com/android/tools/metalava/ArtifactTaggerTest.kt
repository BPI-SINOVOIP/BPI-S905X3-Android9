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

class ArtifactTaggerTest : DriverTest() {

    @Test
    fun `Tag API`() {
        check(
            checkDoclava1 = false,
            sourceFiles = *arrayOf(
                java(
                    """
                    package test.pkg.foo;
                    /** My Foo class documentation. */
                    public class Foo { // registered in both the foo and bar libraries: should get duplicate warnings
                        public class Inner {
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg.bar;
                    /** My Bar class documentation. */
                    public class Bar {
                        public class Inner {
                        }
                    }
                    """
                ),
                java(
                    """
                    package test.pkg.baz;
                    /** Extra class not registered in artifact files: should be flagged */
                    public class Missing {
                    }
                    """
                )
            ),
            artifacts = mapOf(
                "my.library.group:foo:1.0.0" to """
                    package test.pkg.foo {
                      public class Foo {
                        ctor public Foo();
                      }
                      public class Foo.Inner {
                        ctor public Foo.Inner();
                      }
                    }
                """,
                "my.library.group:bar:3.1.4" to """
                    package test.pkg.bar {
                      public class Bar {
                        ctor public Bar();
                      }
                      public class Bar.Inner {
                        ctor public Bar.Inner();
                      }
                    }
                    package test.pkg.foo {
                      public class Foo { // duplicate registration: should generate warning
                      }
                    }
                """
            ),
            extraArguments = arrayOf("--error", "NoArtifactData,BrokenArtifactFile"),
            warnings = """
                src/test/pkg/foo/Foo.java:2: error: Class test.pkg.foo.Foo belongs to multiple artifacts: my.library.group:foo:1.0.0 and my.library.group:bar:3.1.4 [BrokenArtifactFile:130]
                src/test/pkg/foo/Foo.java:4: error: Class test.pkg.foo.Foo.Inner belongs to multiple artifacts: my.library.group:foo:1.0.0 and my.library.group:bar:3.1.4 [BrokenArtifactFile:130]
                src/test/pkg/baz/Missing.java:2: error: No registered artifact signature file referenced class test.pkg.baz.Missing [NoArtifactData:129]
            """,
            stubs = arrayOf(
                """
                package test.pkg.foo;
                /**
                 * My Foo class documentation.
                 * @artifactId my.library.group:foo:1.0.0
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class Foo {
                public Foo() { throw new RuntimeException("Stub!"); }
                /**
                 * @artifactId my.library.group:foo:1.0.0
                 */
                @SuppressWarnings({"unchecked", "deprecation", "all"})
                public class Inner {
                public Inner() { throw new RuntimeException("Stub!"); }
                }
                }
                """
            )

        )
    }
}
