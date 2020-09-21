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

package com.google.turbine.deps;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.base.Joiner;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterables;
import com.google.turbine.binder.Binder;
import com.google.turbine.binder.Binder.BindingResult;
import com.google.turbine.diag.SourceFile;
import com.google.turbine.lower.IntegrationTestSupport;
import com.google.turbine.lower.Lower;
import com.google.turbine.lower.Lower.Lowered;
import com.google.turbine.parse.Parser;
import com.google.turbine.proto.DepsProto;
import com.google.turbine.tree.Tree.CompUnit;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.jar.JarEntry;
import java.util.jar.JarOutputStream;
import java.util.stream.Collectors;
import java.util.stream.StreamSupport;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class DependenciesTest {

  static final ImmutableSet<Path> BOOTCLASSPATH =
      ImmutableSet.of(Paths.get(System.getProperty("java.home")).resolve("lib/rt.jar"));

  @Rule public final TemporaryFolder temporaryFolder = new TemporaryFolder();

  class LibraryBuilder {
    final Map<String, String> sources = new LinkedHashMap<>();
    private ImmutableList<Path> classpath = ImmutableList.of();

    LibraryBuilder addSourceLines(String path, String... lines) {
      sources.put(path, Joiner.on('\n').join(lines));
      return this;
    }

    LibraryBuilder setClasspath(Path... classpath) {
      this.classpath = ImmutableList.copyOf(classpath);
      return this;
    }

    Path compileToJar(String path) throws Exception {
      Path lib = temporaryFolder.newFile(path).toPath();
      Map<String, byte[]> classes =
          IntegrationTestSupport.runJavac(sources, classpath, BOOTCLASSPATH);
      try (JarOutputStream jos = new JarOutputStream(Files.newOutputStream(lib))) {
        for (Map.Entry<String, byte[]> entry : classes.entrySet()) {
          jos.putNextEntry(new JarEntry(entry.getKey() + ".class"));
          jos.write(entry.getValue());
        }
      }
      return lib;
    }
  }

  static class DepsBuilder {
    List<Path> classpath;
    List<CompUnit> units = new ArrayList<>();

    DepsBuilder setClasspath(Path... classpath) {
      this.classpath = ImmutableList.copyOf(classpath);
      return this;
    }

    DepsBuilder addSourceLines(String path, String... lines) {
      units.add(Parser.parse(new SourceFile(path, Joiner.on('\n').join(lines))));
      return this;
    }

    DepsProto.Dependencies run() throws IOException {
      BindingResult bound = Binder.bind(units, classpath, BOOTCLASSPATH);

      Lowered lowered = Lower.lowerAll(bound.units(), bound.classPathEnv());

      return Dependencies.collectDeps(
          Optional.of("//test"),
          ImmutableSet.copyOf(Iterables.transform(BOOTCLASSPATH, Path::toString)),
          bound,
          lowered);
    }
  }

  private Map<Path, DepsProto.Dependency.Kind> depsMap(DepsProto.Dependencies deps) {
    return StreamSupport.stream(deps.getDependencyList().spliterator(), false)
        .collect(Collectors.toMap(d -> Paths.get(d.getPath()), DepsProto.Dependency::getKind));
  }

  @Test
  public void simple() throws Exception {
    Path liba =
        new LibraryBuilder().addSourceLines("A.java", "class A {}").compileToJar("liba.jar");
    DepsProto.Dependencies deps =
        new DepsBuilder()
            .setClasspath(liba)
            .addSourceLines("Test.java", "class Test extends A {}")
            .run();

    assertThat(depsMap(deps)).isEqualTo(ImmutableMap.of(liba, DepsProto.Dependency.Kind.EXPLICIT));
  }

  @Test
  public void excluded() throws Exception {
    Path liba =
        new LibraryBuilder()
            .addSourceLines(
                "A.java", //
                "class A {}")
            .compileToJar("liba.jar");
    Path libb =
        new LibraryBuilder()
            .setClasspath(liba)
            .addSourceLines(
                "B.java", //
                "class B {",
                "  public static final A a = new A();",
                "}")
            .compileToJar("libb.jar");
    DepsProto.Dependencies deps =
        new DepsBuilder()
            .setClasspath(liba, libb)
            .addSourceLines("Test.java", "class Test extends B {}")
            .run();

    assertThat(depsMap(deps)).isEqualTo(ImmutableMap.of(libb, DepsProto.Dependency.Kind.EXPLICIT));
  }

  @Test
  public void transitive() throws Exception {
    Path liba =
        new LibraryBuilder()
            .addSourceLines(
                "A.java", //
                "class A {",
                "  public static final class Y {}",
                "}")
            .compileToJar("liba.jar");
    Path libb =
        new LibraryBuilder()
            .setClasspath(liba)
            .addSourceLines("B.java", "class B extends A {}")
            .compileToJar("libb.jar");
    DepsProto.Dependencies deps =
        new DepsBuilder()
            .setClasspath(liba, libb)
            .addSourceLines(
                "Test.java", //
                "class Test extends B {",
                "  public static class X extends Y {}",
                "}")
            .run();
    assertThat(depsMap(deps))
        .isEqualTo(
            ImmutableMap.of(
                libb, DepsProto.Dependency.Kind.EXPLICIT,
                liba, DepsProto.Dependency.Kind.EXPLICIT));
  }

  @Test
  public void closure() throws Exception {
    Path libi =
        new LibraryBuilder()
            .addSourceLines(
                "i/I.java",
                "package i;", //
                "public interface I {}")
            .compileToJar("libi.jar");
    Path liba =
        new LibraryBuilder()
            .setClasspath(libi)
            .addSourceLines(
                "a/A.java", //
                "package a;",
                "import i.I;",
                "public class A implements I {}")
            .compileToJar("liba.jar");
    Path libb =
        new LibraryBuilder()
            .setClasspath(liba, libi)
            .addSourceLines(
                "b/B.java", //
                "package b;",
                "import a.A;",
                "public class B extends A {}")
            .compileToJar("libb.jar");
    {
      DepsProto.Dependencies deps =
          new DepsBuilder()
              .setClasspath(liba, libb, libi)
              .addSourceLines(
                  "Test.java", //
                  "import b.B;",
                  "class Test extends B {}")
              .run();
      assertThat(depsMap(deps))
          .isEqualTo(
              ImmutableMap.of(
                  libi, DepsProto.Dependency.Kind.EXPLICIT,
                  libb, DepsProto.Dependency.Kind.EXPLICIT,
                  liba, DepsProto.Dependency.Kind.EXPLICIT));
    }
    {
      // partial classpath
      DepsProto.Dependencies deps =
          new DepsBuilder()
              .setClasspath(liba, libb)
              .addSourceLines(
                  "Test.java", //
                  "import b.B;",
                  "class Test extends B {}")
              .run();
      assertThat(depsMap(deps))
          .isEqualTo(
              ImmutableMap.of(
                  libb, DepsProto.Dependency.Kind.EXPLICIT,
                  liba, DepsProto.Dependency.Kind.EXPLICIT));
    }
  }

  @Test
  public void unreducedClasspathTest() throws IOException {
    ImmutableList<String> classpath =
        ImmutableList.of(
            "a.jar", "b.jar", "c.jar", "d.jar", "e.jar", "f.jar", "g.jar", "h.jar", "i.jar",
            "j.jar");
    ImmutableMap<String, String> directJarsToTargets = ImmutableMap.of();
    ImmutableList<String> depsArtifacts = ImmutableList.of();
    assertThat(Dependencies.reduceClasspath(classpath, directJarsToTargets, depsArtifacts))
        .isEqualTo(classpath);
  }

  @Test
  public void reducedClasspathTest() throws IOException {
    Path cdeps = temporaryFolder.newFile("c.jdeps").toPath();
    Path ddeps = temporaryFolder.newFile("d.jdeps").toPath();
    Path gdeps = temporaryFolder.newFile("g.jdeps").toPath();
    ImmutableList<String> classpath =
        ImmutableList.of(
            "a.jar", "b.jar", "c.jar", "d.jar", "e.jar", "f.jar", "g.jar", "h.jar", "i.jar",
            "j.jar");
    ImmutableMap<String, String> directJarsToTargets =
        ImmutableMap.of(
            "c.jar", "//a",
            "d.jar", "//d",
            "g.jar", "//e");
    ImmutableList<String> depsArtifacts =
        ImmutableList.of(cdeps.toString(), ddeps.toString(), gdeps.toString());
    writeDeps(
        cdeps,
        ImmutableMap.of(
            "b.jar", DepsProto.Dependency.Kind.EXPLICIT,
            "e.jar", DepsProto.Dependency.Kind.EXPLICIT));
    writeDeps(
        ddeps,
        ImmutableMap.of(
            "f.jar", DepsProto.Dependency.Kind.UNUSED,
            "j.jar", DepsProto.Dependency.Kind.UNUSED));
    writeDeps(gdeps, ImmutableMap.of("i.jar", DepsProto.Dependency.Kind.IMPLICIT));
    assertThat(Dependencies.reduceClasspath(classpath, directJarsToTargets, depsArtifacts))
        .containsExactly("b.jar", "c.jar", "d.jar", "e.jar", "g.jar", "i.jar")
        .inOrder();
  }

  @Test
  public void packageInfo() throws Exception {
    Path libpackageInfo =
        new LibraryBuilder()
            .addSourceLines(
                "p/Anno.java",
                "package p;",
                "import java.lang.annotation.Retention;",
                "import static java.lang.annotation.RetentionPolicy.RUNTIME;",
                "@Retention(RUNTIME)",
                "@interface Anno {}")
            .addSourceLines(
                "p/package-info.java", //
                "@Anno",
                "package p;")
            .compileToJar("libpackage-info.jar");
    Path libp =
        new LibraryBuilder()
            .setClasspath(libpackageInfo)
            .addSourceLines(
                "p/P.java", //
                "package p;",
                "public class P {}")
            .compileToJar("libp.jar");
    {
      DepsProto.Dependencies deps =
          new DepsBuilder()
              .setClasspath(libp, libpackageInfo)
              .addSourceLines(
                  "Test.java", //
                  "import p.P;",
                  "class Test {",
                  "  P p;",
                  "}")
              .run();
      assertThat(depsMap(deps))
          .containsExactly(
              libpackageInfo,
              DepsProto.Dependency.Kind.EXPLICIT,
              libp,
              DepsProto.Dependency.Kind.EXPLICIT);
    }
  }

  void writeDeps(Path path, ImmutableMap<String, DepsProto.Dependency.Kind> deps)
      throws IOException {
    DepsProto.Dependencies.Builder builder =
        DepsProto.Dependencies.newBuilder().setSuccess(true).setRuleLabel("//test");
    for (Map.Entry<String, DepsProto.Dependency.Kind> e : deps.entrySet()) {
      builder.addDependency(
          DepsProto.Dependency.newBuilder().setPath(e.getKey()).setKind(e.getValue()));
    }
    try (OutputStream os = new BufferedOutputStream(Files.newOutputStream(path))) {
      builder.build().writeTo(os);
    }
  }
}
