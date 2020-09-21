/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.turbine.binder;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.turbine.binder.bound.SourceTypeBoundClass;
import com.google.turbine.binder.sym.ClassSymbol;
import com.google.turbine.diag.TurbineError;
import com.google.turbine.lower.IntegrationTestSupport;
import com.google.turbine.model.TurbineFlag;
import com.google.turbine.parse.Parser;
import com.google.turbine.tree.Tree;
import java.io.OutputStream;
import java.lang.annotation.ElementType;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.jar.JarEntry;
import java.util.jar.JarOutputStream;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class BinderTest {

  @Rule public final TemporaryFolder temporaryFolder = new TemporaryFolder();

  private static final ImmutableList<Path> BOOTCLASSPATH =
      ImmutableList.of(Paths.get(System.getProperty("java.home")).resolve("lib/rt.jar"));

  @Test
  public void hello() throws Exception {
    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "package a;", //
            "public class A {",
            "  public class Inner1 extends b.B {",
            "  }",
            "  public class Inner2 extends A.Inner1 {",
            "  }",
            "}"));
    units.add(
        parseLines(
            "package b;", //
            "import a.A;",
            "public class B extends A {",
            "}"));

    ImmutableMap<ClassSymbol, SourceTypeBoundClass> bound =
        Binder.bind(units, Collections.emptyList(), BOOTCLASSPATH).units();

    assertThat(bound.keySet())
        .containsExactly(
            new ClassSymbol("a/A"),
            new ClassSymbol("a/A$Inner1"),
            new ClassSymbol("a/A$Inner2"),
            new ClassSymbol("b/B"));

    SourceTypeBoundClass a = bound.get(new ClassSymbol("a/A"));
    assertThat(a.superclass()).isEqualTo(new ClassSymbol("java/lang/Object"));
    assertThat(a.interfaces()).isEmpty();

    assertThat(bound.get(new ClassSymbol("a/A$Inner1")).superclass())
        .isEqualTo(new ClassSymbol("b/B"));

    assertThat(bound.get(new ClassSymbol("a/A$Inner2")).superclass())
        .isEqualTo(new ClassSymbol("a/A$Inner1"));

    SourceTypeBoundClass b = bound.get(new ClassSymbol("b/B"));
    assertThat(b.superclass()).isEqualTo(new ClassSymbol("a/A"));
  }

  @Test
  public void interfaces() throws Exception {
    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "package com.i;", //
            "public interface I {",
            "  public class IInner {",
            "  }",
            "}"));
    units.add(
        parseLines(
            "package b;", //
            "class B implements com.i.I {",
            "  class BInner extends IInner {}",
            "}"));

    ImmutableMap<ClassSymbol, SourceTypeBoundClass> bound =
        Binder.bind(units, Collections.emptyList(), BOOTCLASSPATH).units();

    assertThat(bound.keySet())
        .containsExactly(
            new ClassSymbol("com/i/I"),
            new ClassSymbol("com/i/I$IInner"),
            new ClassSymbol("b/B"),
            new ClassSymbol("b/B$BInner"));

    assertThat(bound.get(new ClassSymbol("b/B")).interfaces())
        .containsExactly(new ClassSymbol("com/i/I"));

    assertThat(bound.get(new ClassSymbol("b/B$BInner")).superclass())
        .isEqualTo(new ClassSymbol("com/i/I$IInner"));
    assertThat(bound.get(new ClassSymbol("b/B$BInner")).interfaces()).isEmpty();
  }

  @Test
  public void imports() throws Exception {
    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "package com.test;", //
            "public class Test {",
            "  public static class Inner {}",
            "}"));
    units.add(
        parseLines(
            "package other;", //
            "import com.test.Test.Inner;",
            "import no.such.Class;", // imports are resolved lazily on-demand
            "public class Foo extends Inner {",
            "}"));

    ImmutableMap<ClassSymbol, SourceTypeBoundClass> bound =
        Binder.bind(units, Collections.emptyList(), BOOTCLASSPATH).units();

    assertThat(bound.get(new ClassSymbol("other/Foo")).superclass())
        .isEqualTo(new ClassSymbol("com/test/Test$Inner"));
  }

  @Test
  public void cycle() throws Exception {
    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "package a;", //
            "import b.B;",
            "public class A extends B.Inner {",
            "  class Inner {}",
            "}"));
    units.add(
        parseLines(
            "package b;", //
            "import a.A;",
            "public class B extends A.Inner {",
            "  class Inner {}",
            "}"));

    try {
      Binder.bind(units, Collections.emptyList(), BOOTCLASSPATH);
      fail();
    } catch (TurbineError e) {
      assertThat(e.getMessage()).contains("cycle in class hierarchy: a/A -> b/B -> a/A");
    }
  }

  @Test
  public void annotationDeclaration() throws Exception {
    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "package com.test;", //
            "public @interface Annotation {",
            "}"));

    ImmutableMap<ClassSymbol, SourceTypeBoundClass> bound =
        Binder.bind(units, Collections.emptyList(), BOOTCLASSPATH).units();

    SourceTypeBoundClass a = bound.get(new ClassSymbol("com/test/Annotation"));
    assertThat(a.access())
        .isEqualTo(
            TurbineFlag.ACC_PUBLIC
                | TurbineFlag.ACC_INTERFACE
                | TurbineFlag.ACC_ABSTRACT
                | TurbineFlag.ACC_ANNOTATION);
    assertThat(a.superclass()).isEqualTo(new ClassSymbol("java/lang/Object"));
    assertThat(a.interfaces()).containsExactly(new ClassSymbol("java/lang/annotation/Annotation"));
  }

  @Test
  public void helloBytecode() throws Exception {
    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "package a;", //
            "import java.util.Map.Entry;",
            "public class A implements Entry {",
            "}"));

    ImmutableMap<ClassSymbol, SourceTypeBoundClass> bound =
        Binder.bind(units, Collections.emptyList(), BOOTCLASSPATH).units();

    SourceTypeBoundClass a = bound.get(new ClassSymbol("a/A"));
    assertThat(a.interfaces()).containsExactly(new ClassSymbol("java/util/Map$Entry"));
  }

  @Test
  public void incompleteClasspath() throws Exception {

    Map<String, byte[]> lib =
        IntegrationTestSupport.runJavac(
            ImmutableMap.of(
                "A.java", "class A {}",
                "B.java", "class B extends A {}"),
            ImmutableList.of(),
            BOOTCLASSPATH);

    // create a jar containing only B
    Path libJar = temporaryFolder.newFile("lib.jar").toPath();
    try (OutputStream os = Files.newOutputStream(libJar);
        JarOutputStream jos = new JarOutputStream(os)) {
      jos.putNextEntry(new JarEntry("B.class"));
      jos.write(lib.get("B"));
    }

    List<Tree.CompUnit> units = new ArrayList<>();
    units.add(
        parseLines(
            "import java.lang.annotation.Target;",
            "import java.lang.annotation.ElementType;",
            "public class C implements B {",
            "  @Target(ElementType.TYPE_USE)",
            "  @interface A {};",
            "}"));

    ImmutableMap<ClassSymbol, SourceTypeBoundClass> bound =
        Binder.bind(units, ImmutableList.of(libJar), BOOTCLASSPATH).units();

    SourceTypeBoundClass a = bound.get(new ClassSymbol("C$A"));
    assertThat(a.annotationMetadata().target()).containsExactly(ElementType.TYPE_USE);
  }

  private Tree.CompUnit parseLines(String... lines) {
    return Parser.parse(Joiner.on('\n').join(lines));
  }
}
