/*
 * Copyright (C) 2014 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package dagger.internal.codegen;

import com.google.common.collect.ImmutableList;
import com.google.testing.compile.JavaFileObjects;
import javax.tools.JavaFileObject;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static com.google.common.truth.Truth.assertAbout;
import static com.google.testing.compile.JavaSourceSubjectFactory.javaSource;
import static com.google.testing.compile.JavaSourcesSubjectFactory.javaSources;
import static dagger.internal.codegen.ErrorMessages.ABSTRACT_INJECT_METHOD;
import static dagger.internal.codegen.ErrorMessages.FINAL_INJECT_FIELD;
import static dagger.internal.codegen.ErrorMessages.GENERIC_INJECT_METHOD;
import static dagger.internal.codegen.ErrorMessages.INJECT_CONSTRUCTOR_ON_ABSTRACT_CLASS;
import static dagger.internal.codegen.ErrorMessages.INJECT_CONSTRUCTOR_ON_INNER_CLASS;
import static dagger.internal.codegen.ErrorMessages.INJECT_ON_PRIVATE_CONSTRUCTOR;
import static dagger.internal.codegen.ErrorMessages.MULTIPLE_INJECT_CONSTRUCTORS;
import static dagger.internal.codegen.ErrorMessages.MULTIPLE_QUALIFIERS;
import static dagger.internal.codegen.ErrorMessages.MULTIPLE_SCOPES;
import static dagger.internal.codegen.ErrorMessages.PRIVATE_INJECT_FIELD;
import static dagger.internal.codegen.ErrorMessages.PRIVATE_INJECT_METHOD;
import static dagger.internal.codegen.ErrorMessages.QUALIFIER_ON_INJECT_CONSTRUCTOR;
import static dagger.internal.codegen.ErrorMessages.STATIC_INJECT_FIELD;
import static dagger.internal.codegen.ErrorMessages.STATIC_INJECT_METHOD;

@RunWith(JUnit4.class)
// TODO(gak): add tests for generation in the default package.
public final class InjectConstructorFactoryGeneratorTest {
  private static final JavaFileObject QUALIFIER_A =
      JavaFileObjects.forSourceLines("test.QualifierA",
          "package test;",
          "",
          "import javax.inject.Qualifier;",
          "",
          "@Qualifier @interface QualifierA {}");
  private static final JavaFileObject QUALIFIER_B =
      JavaFileObjects.forSourceLines("test.QualifierB",
          "package test;",
          "",
          "import javax.inject.Qualifier;",
          "",
          "@Qualifier @interface QualifierB {}");
  private static final JavaFileObject SCOPE_A =
      JavaFileObjects.forSourceLines("test.ScopeA",
          "package test;",
          "",
          "import javax.inject.Scope;",
          "",
          "@Scope @interface ScopeA {}");
  private static final JavaFileObject SCOPE_B =
      JavaFileObjects.forSourceLines("test.ScopeB",
          "package test;",
          "",
          "import javax.inject.Scope;",
          "",
          "@Scope @interface ScopeB {}");

  @Test public void injectOnPrivateConstructor() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.PrivateConstructor",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class PrivateConstructor {",
        "  @Inject private PrivateConstructor() {}",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(INJECT_ON_PRIVATE_CONSTRUCTOR).in(file).onLine(6);
  }

  @Test public void injectConstructorOnInnerClass() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.OuterClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class OuterClass {",
        "  class InnerClass {",
        "    @Inject InnerClass() {}",
        "  }",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(INJECT_CONSTRUCTOR_ON_INNER_CLASS).in(file).onLine(7);
  }

  @Test public void injectConstructorOnAbstractClass() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.AbstractClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "abstract class AbstractClass {",
        "  @Inject AbstractClass() {}",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(INJECT_CONSTRUCTOR_ON_ABSTRACT_CLASS).in(file).onLine(6);
  }

  @Test public void injectConstructorOnGenericClass() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class GenericClass<T> {",
        "  @Inject GenericClass(T t) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines("test.GenericClass_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class GenericClass_Factory<T> implements Factory<GenericClass<T>> {",
        "  private final Provider<T> tProvider;",
        "",
        "  public GenericClass_Factory(Provider<T> tProvider) {",
        "    assert tProvider != null;",
        "    this.tProvider = tProvider;",
        "  }",
        "",
        "  @Override",
        "  public GenericClass<T> get() {",
        "    return new GenericClass<T>(tProvider.get());",
        "  }",
        "",
        "  public static <T> Factory<GenericClass<T>> create(Provider<T> tProvider) {",
        "    return new GenericClass_Factory<T>(tProvider);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test public void fieldAndMethodGenerics() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class GenericClass<A, B> {",
        "  @Inject A a;",
        "",
        "  @Inject GenericClass() {}",
        "",
        " @Inject void register(B b) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines("test.GenericClass_Factory",
        "package test;",
        "",
        "import dagger.MembersInjector;",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class GenericClass_Factory<A, B> implements Factory<GenericClass<A, B>> {",
        "  private final MembersInjector<GenericClass<A, B>> membersInjector;",
        "",
        "  public GenericClass_Factory(MembersInjector<GenericClass<A, B>> membersInjector) {",
        "    assert membersInjector != null;",
        "    this.membersInjector = membersInjector;",
        "  }",
        "",
        "  @Override",
        "  public GenericClass<A, B> get() {",
        "    GenericClass<A, B> instance = new GenericClass<A, B>();",
        "    membersInjector.injectMembers(instance);",
        "    return instance;",
        "  }",
        "",
        "  public static <A, B> Factory<GenericClass<A, B>> create(",
        "      MembersInjector<GenericClass<A, B>> membersInjector) {",
        "    return new GenericClass_Factory<A, B>(membersInjector);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test public void genericClassWithNoDependencies() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class GenericClass<T> {",
        "  @Inject GenericClass() {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines("test.GenericClass_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "",
        "@SuppressWarnings(\"rawtypes\")",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public enum GenericClass_Factory implements Factory<GenericClass> {",
        "  INSTANCE;",
        "",
        "  @Override",
        "  public GenericClass get() {",
        "    return new GenericClass();",
        "  }",
        "",
        "  @SuppressWarnings(\"unchecked\")",
        "  public static <T> Factory<GenericClass<T>> create() {",
        "    return (Factory) INSTANCE;",
        "  }",
        "",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test public void twoGenericTypes() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class GenericClass<A, B> {",
        "  @Inject GenericClass(A a, B b) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines("test.GenericClass_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class GenericClass_Factory<A, B> implements Factory<GenericClass<A, B>> {",
        "  private final Provider<A> aProvider;",
        "  private final Provider<B> bProvider;",
        "",
        "  public GenericClass_Factory(Provider<A> aProvider, Provider<B> bProvider) {",
        "    assert aProvider != null;",
        "    this.aProvider = aProvider;",
        "    assert bProvider != null;",
        "    this.bProvider = bProvider;",
        "  }",
        "",
        "  @Override",
        "  public GenericClass<A, B> get() {",
        "    return new GenericClass<A, B>(aProvider.get(), bProvider.get());",
        "  }",
        "",
        "  public static <A, B> Factory<GenericClass<A, B>> create(",
        "      Provider<A> aProvider, Provider<B> bProvider) {",
        "    return new GenericClass_Factory<A, B>(aProvider, bProvider);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }
  
  @Test public void boundedGenerics() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "import java.util.List;",
        "",
        "class GenericClass<A extends Number & Comparable<A>,",
        "    B extends List<? extends String>,",
        "    C extends List<? super String>> {",
        "  @Inject GenericClass(A a, B b, C c) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines("test.GenericClass_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import java.util.List;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class GenericClass_Factory<A extends Number & Comparable<A>,",
        "        B extends List<? extends String>,",
        "        C extends List<? super String>>",
        "    implements Factory<GenericClass<A, B, C>> {",
        "  private final Provider<A> aProvider;",
        "  private final Provider<B> bProvider;",
        "  private final Provider<C> cProvider;",
        "",
        "  public GenericClass_Factory(Provider<A> aProvider,",
        "      Provider<B> bProvider,",
        "      Provider<C> cProvider) {",
        "    assert aProvider != null;",
        "    this.aProvider = aProvider;",
        "    assert bProvider != null;",
        "    this.bProvider = bProvider;",
        "    assert cProvider != null;",
        "    this.cProvider = cProvider;",
        "  }",
        "",
        "  @Override",
        "  public GenericClass<A, B, C> get() {",
        "    return new GenericClass<A, B, C>(aProvider.get(), bProvider.get(), cProvider.get());",
        "  }",
        "",
        "  public static <A extends Number & Comparable<A>,",
        "      B extends List<? extends String>,",
        "      C extends List<? super String>> Factory<GenericClass<A, B, C>> create(",
        "          Provider<A> aProvider, Provider<B> bProvider, Provider<C> cProvider) {",
        "    return new GenericClass_Factory<A, B, C>(aProvider, bProvider, cProvider);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test public void multipleSameTypesWithGenericsAndQualifiersAndLazies() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "import javax.inject.Provider;",
        "import dagger.Lazy;",
        "",
        "class GenericClass<A, B> {",
        "  @Inject GenericClass(A a, A a2, Provider<A> pa, @QualifierA A qa, Lazy<A> la, ",
        "                       String s, String s2, Provider<String> ps, ",
        "                       @QualifierA String qs, Lazy<String> ls,",
        "                       B b, B b2, Provider<B> pb, @QualifierA B qb, Lazy<B> lb) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines("test.GenericClass_Factory",
        "package test;",
        "",
        "import dagger.internal.DoubleCheckLazy;",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class GenericClass_Factory<A, B> implements Factory<GenericClass<A, B>> {",
        "  private final Provider<A> aAndA2AndPaAndLaProvider;",
        "  private final Provider<A> qaProvider;",
        "  private final Provider<String> sAndS2AndPsAndLsProvider;",
        "  private final Provider<String> qsProvider;",
        "  private final Provider<B> bAndB2AndPbAndLbProvider;",
        "  private final Provider<B> qbProvider;",
        "",
        "  public GenericClass_Factory(Provider<A> aAndA2AndPaAndLaProvider,",
        "      Provider<A> qaProvider,", 
        "      Provider<String> sAndS2AndPsAndLsProvider,",
        "      Provider<String> qsProvider,",
        "      Provider<B> bAndB2AndPbAndLbProvider,",
        "      Provider<B> qbProvider) {",
        "    assert aAndA2AndPaAndLaProvider != null;",
        "    this.aAndA2AndPaAndLaProvider = aAndA2AndPaAndLaProvider;",
        "    assert qaProvider != null;",
        "    this.qaProvider = qaProvider;",
        "    assert sAndS2AndPsAndLsProvider != null;",
        "    this.sAndS2AndPsAndLsProvider = sAndS2AndPsAndLsProvider;",
        "    assert qsProvider != null;",
        "    this.qsProvider = qsProvider;",
        "    assert bAndB2AndPbAndLbProvider != null;",
        "    this.bAndB2AndPbAndLbProvider = bAndB2AndPbAndLbProvider;",
        "    assert qbProvider != null;",
        "    this.qbProvider = qbProvider;",
        "  }",
        "",
        "  @Override",
        "  public GenericClass<A, B> get() {",
        "    return new GenericClass<A, B>(",
        "      aAndA2AndPaAndLaProvider.get(),",
        "      aAndA2AndPaAndLaProvider.get(),",
        "      aAndA2AndPaAndLaProvider,",
        "      qaProvider.get(),",
        "      DoubleCheckLazy.create(aAndA2AndPaAndLaProvider),",
        "      sAndS2AndPsAndLsProvider.get(),",
        "      sAndS2AndPsAndLsProvider.get(),",
        "      sAndS2AndPsAndLsProvider,",
        "      qsProvider.get(),",
        "      DoubleCheckLazy.create(sAndS2AndPsAndLsProvider),",
        "      bAndB2AndPbAndLbProvider.get(),",
        "      bAndB2AndPbAndLbProvider.get(),", 
        "      bAndB2AndPbAndLbProvider,",
        "      qbProvider.get(),",
        "      DoubleCheckLazy.create(bAndB2AndPbAndLbProvider));",
        "  }",
        "",
        "  public static <A, B> Factory<GenericClass<A, B>> create(",
        "      Provider<A> aAndA2AndPaAndLaProvider,",
        "      Provider<A> qaProvider,", 
        "      Provider<String> sAndS2AndPsAndLsProvider,",
        "      Provider<String> qsProvider,",
        "      Provider<B> bAndB2AndPbAndLbProvider,",
        "      Provider<B> qbProvider) {",
        "    return new GenericClass_Factory<A, B>(",
        "        aAndA2AndPaAndLaProvider,",
        "        qaProvider,",
        "        sAndS2AndPsAndLsProvider,",
        "        qsProvider,",
        "        bAndB2AndPbAndLbProvider,",
        "        qbProvider);",
        "  }",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(file, QUALIFIER_A))
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test public void multipleInjectConstructors() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.TooManyInjectConstructors",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class TooManyInjectConstructors {",
        "  @Inject TooManyInjectConstructors() {}",
        "  TooManyInjectConstructors(int i) {}",
        "  @Inject TooManyInjectConstructors(String s) {}",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(MULTIPLE_INJECT_CONSTRUCTORS).in(file).onLine(6)
        .and().withErrorContaining(MULTIPLE_INJECT_CONSTRUCTORS).in(file).onLine(8);
  }

  @Test public void multipleQualifiersOnInjectConstructorParameter() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.MultipleQualifierConstructorParam",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class MultipleQualifierConstructorParam {",
        "  @Inject MultipleQualifierConstructorParam(@QualifierA @QualifierB String s) {}",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(file, QUALIFIER_A, QUALIFIER_B))
        .processedWith(new ComponentProcessor()).failsToCompile()
        // for whatever reason, javac only reports the error once on the constructor
        .withErrorContaining(MULTIPLE_QUALIFIERS).in(file).onLine(6);
  }

  @Test public void injectConstructorOnClassWithMultipleScopes() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.MultipleScopeClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "@ScopeA @ScopeB class MultipleScopeClass {",
        "  @Inject MultipleScopeClass() {}",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(file, SCOPE_A, SCOPE_B))
        .processedWith(new ComponentProcessor()).failsToCompile()
        .withErrorContaining(MULTIPLE_SCOPES).in(file).onLine(5).atColumn(1)
        .and().withErrorContaining(MULTIPLE_SCOPES).in(file).onLine(5).atColumn(9);
  }

  @Test public void injectConstructorWithQualifier() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.MultipleScopeClass",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class MultipleScopeClass {",
        "  @Inject",
        "  @QualifierA",
        "  @QualifierB",
        "  MultipleScopeClass() {}",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(file, QUALIFIER_A, QUALIFIER_B))
        .processedWith(new ComponentProcessor()).failsToCompile()
        .withErrorContaining(QUALIFIER_ON_INJECT_CONSTRUCTOR).in(file).onLine(7)
        .and().withErrorContaining(QUALIFIER_ON_INJECT_CONSTRUCTOR).in(file).onLine(8);
  }

  @Test public void finalInjectField() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.FinalInjectField",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class FinalInjectField {",
        "  @Inject final String s;",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(FINAL_INJECT_FIELD).in(file).onLine(6);
  }

  @Test public void privateInjectFieldError() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.PrivateInjectField",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class PrivateInjectField {",
        "  @Inject private String s;",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(PRIVATE_INJECT_FIELD).in(file).onLine(6);
  }
  
  @Test public void privateInjectFieldWarning() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.PrivateInjectField",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class PrivateInjectField {",
        "  @Inject private String s;",
        "}");
    assertAbout(javaSource()).that(file)
        .withCompilerOptions("-Adagger.privateMemberValidation=WARNING")
        .processedWith(new ComponentProcessor())
        .compilesWithoutError(); // TODO: Verify warning message when supported
  }
  
  @Test public void staticInjectFieldError() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.StaticInjectField",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class StaticInjectField {",
        "  @Inject static String s;",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(STATIC_INJECT_FIELD).in(file).onLine(6);
  }
  
  @Test public void staticInjectFieldWarning() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.StaticInjectField",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class StaticInjectField {",
        "  @Inject static String s;",
        "}");
    assertAbout(javaSource()).that(file)
        .withCompilerOptions("-Adagger.staticMemberValidation=WARNING")
        .processedWith(new ComponentProcessor())
        .compilesWithoutError(); // TODO: Verify warning message when supported
  }

  @Test public void multipleQualifiersOnField() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.MultipleQualifierInjectField",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class MultipleQualifierInjectField {",
        "  @Inject @QualifierA @QualifierB String s;",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(file, QUALIFIER_A, QUALIFIER_B))
        .processedWith(new ComponentProcessor()).failsToCompile()
        .withErrorContaining(MULTIPLE_QUALIFIERS).in(file).onLine(6).atColumn(11)
        .and().withErrorContaining(MULTIPLE_QUALIFIERS).in(file).onLine(6).atColumn(23);
  }

  @Test public void abstractInjectMethod() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.AbstractInjectMethod",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "abstract class AbstractInjectMethod {",
        "  @Inject abstract void method();",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(ABSTRACT_INJECT_METHOD).in(file).onLine(6);
  }

  @Test public void privateInjectMethodError() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.PrivateInjectMethod",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class PrivateInjectMethod {",
        "  @Inject private void method(){}",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(PRIVATE_INJECT_METHOD).in(file).onLine(6);
  }
  
  @Test public void privateInjectMethodWarning() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.PrivateInjectMethod",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class PrivateInjectMethod {",
        "  @Inject private void method(){}",
        "}");
    assertAbout(javaSource()).that(file)
        .withCompilerOptions("-Adagger.privateMemberValidation=WARNING")
        .processedWith(new ComponentProcessor())
        .compilesWithoutError(); // TODO: Verify warning message when supported
  }
  
  @Test public void staticInjectMethodError() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.StaticInjectMethod",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class StaticInjectMethod {",
        "  @Inject static void method(){}",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(STATIC_INJECT_METHOD).in(file).onLine(6);
  }
  
  @Test public void staticInjectMethodWarning() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.StaticInjectMethod",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class StaticInjectMethod {",
        "  @Inject static void method(){}",
        "}");
    assertAbout(javaSource()).that(file)
        .withCompilerOptions("-Adagger.staticMemberValidation=WARNING")
        .processedWith(new ComponentProcessor())
        .compilesWithoutError(); // TODO: Verify warning message when supported
  }

  @Test public void genericInjectMethod() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.GenericInjectMethod",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class AbstractInjectMethod {",
        "  @Inject <T> void method();",
        "}");
    assertAbout(javaSource()).that(file)
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        .withErrorContaining(GENERIC_INJECT_METHOD).in(file).onLine(6);
  }

  @Test public void multipleQualifiersOnInjectMethodParameter() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.MultipleQualifierMethodParam",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class MultipleQualifierMethodParam {",
        "  @Inject void method(@QualifierA @QualifierB String s) {}",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(file, QUALIFIER_A, QUALIFIER_B))
        .processedWith(new ComponentProcessor())
        .failsToCompile()
        // for whatever reason, javac only reports the error once on the method
        .withErrorContaining(MULTIPLE_QUALIFIERS).in(file).onLine(6);
  }

  @Test public void injectConstructor() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.InjectConstructor",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class InjectConstructor {",
        "  @Inject InjectConstructor(String s) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines(
        "test.InjectConstructor_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class InjectConstructor_Factory ",
        "    implements Factory<InjectConstructor> {",
        "",
        "  private final Provider<String> sProvider;",
        "",
        "  public InjectConstructor_Factory(Provider<String> sProvider) {",
        "    assert sProvider != null;",
        "    this.sProvider = sProvider;",
        "  }",
        "",
        "  @Override public InjectConstructor get() {",
        "    return new InjectConstructor(sProvider.get());",
        "  }",
        "",
        "  public static Factory<InjectConstructor> create(Provider<String> sProvider) {",
        "    return new InjectConstructor_Factory(sProvider);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file).processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test public void injectConstructorAndMembersInjection() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.AllInjections",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class AllInjections {",
        "  @Inject String s;",
        "  @Inject AllInjections(String s) {}",
        "  @Inject void s(String s) {}",
        "}");
    JavaFileObject expectedFactory = JavaFileObjects.forSourceLines(
        "test.AllInjections_Factory",
        "package test;",
        "",
        "import dagger.MembersInjector;",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class AllInjections_Factory ",
        "    implements Factory<AllInjections> {",
        "",
        "  private final MembersInjector<AllInjections> membersInjector;",
        "  private final Provider<String> sProvider;",
        "",
        "  public AllInjections_Factory(MembersInjector<AllInjections> membersInjector, ",
        "      Provider<String> sProvider) {",
        "    assert membersInjector != null;",
        "    this.membersInjector = membersInjector;",
        "    assert sProvider != null;",
        "    this.sProvider = sProvider;",
        "  }",
        "",
        "  @Override public AllInjections get() {",
        "    AllInjections instance = new AllInjections(sProvider.get());",
        "    membersInjector.injectMembers(instance);",
        "    return instance;",
        "  }",
        "",
        "  public static Factory<AllInjections> create(",
        "      MembersInjector<AllInjections> membersInjector, ",
        "      Provider<String> sProvider) {",
        "    return new AllInjections_Factory(membersInjector, sProvider);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file).processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and()
        .generatesSources(expectedFactory);
  }

  @Test public void supertypeRequiresMemberInjection() {
    JavaFileObject aFile = JavaFileObjects.forSourceLines("test.A",
        "package test;",
        "",
        "class A {}");
    JavaFileObject bFile = JavaFileObjects.forSourceLines("test.B",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class B extends A {",
        "  @Inject B() {}",
        "}");
    JavaFileObject expectedFactory = JavaFileObjects.forSourceLines(
        "test.B_Factory",
        "package test;",
        "",
        "import dagger.MembersInjector;",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class B_Factory implements Factory<B> {",
        "",
        "  private final MembersInjector<B> membersInjector;",
        "",
        "  public B_Factory(MembersInjector<B> membersInjector) {",
        "    assert membersInjector != null;",
        "    this.membersInjector = membersInjector;",
        "  }",
        "",
        "  @Override public B get() {",
        "    B instance = new B();",
        "    membersInjector.injectMembers(instance);",
        "    return instance;",
        "  }",
        "",
        "  public static Factory<B> create(MembersInjector<B> membersInjector) {",
        "    return new B_Factory(membersInjector);",
        "  }",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(aFile, bFile))
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expectedFactory);
  }

  @Test
  public void wildcardDependency() {
    JavaFileObject file = JavaFileObjects.forSourceLines("test.InjectConstructor",
        "package test;",
        "",
        "import java.util.List;",
        "import javax.inject.Inject;",
        "",
        "class InjectConstructor {",
        "  @Inject InjectConstructor(List<? extends Object> objects) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines(
        "test.InjectConstructor_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import java.util.List;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class InjectConstructor_Factory ",
        "    implements Factory<InjectConstructor> {",
        "",
        "  private final Provider<List<? extends Object>> objectsProvider;",
        "",
        "  public InjectConstructor_Factory(Provider<List<? extends Object>> objectsProvider) {",
        "    assert objectsProvider != null;",
        "    this.objectsProvider = objectsProvider;",
        "  }",
        "",
        "  @Override public InjectConstructor get() {",
        "    return new InjectConstructor(objectsProvider.get());",
        "  }",
        "",
        "  public static Factory<InjectConstructor> create(",
        "      Provider<List<? extends Object>> objectsProvider) {",
        "    return new InjectConstructor_Factory(objectsProvider);",
        "  }",
        "}");
    assertAbout(javaSource()).that(file).processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test
  public void basicNameCollision() {
    JavaFileObject factoryFile = JavaFileObjects.forSourceLines("other.pkg.Factory",
        "package other.pkg;",
        "",
        "public class Factory {}");
    JavaFileObject file = JavaFileObjects.forSourceLines("test.InjectConstructor",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "import other.pkg.Factory;",
        "",
        "class InjectConstructor {",
        "  @Inject InjectConstructor(Factory factory) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines(
        "test.InjectConstructor_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class InjectConstructor_Factory ",
        "    implements Factory<InjectConstructor> {",
        "",
        "  private final Provider<other.pkg.Factory> factoryProvider;",
        "",
        "  public InjectConstructor_Factory(Provider<other.pkg.Factory> factoryProvider) {",
        "    assert factoryProvider != null;",
        "    this.factoryProvider = factoryProvider;",
        "  }",
        "",
        "  @Override public InjectConstructor get() {",
        "    return new InjectConstructor(factoryProvider.get());",
        "  }",
        "",
        "  public static Factory<InjectConstructor> create(",
        "      Provider<other.pkg.Factory> factoryProvider) {",
        "    return new InjectConstructor_Factory(factoryProvider);",
        "  }",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(factoryFile, file))
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test
  public void nestedNameCollision() {
    JavaFileObject factoryFile = JavaFileObjects.forSourceLines("other.pkg.Outer",
        "package other.pkg;",
        "",
        "public class Outer {",
        "  public class Factory {}",
        "}");
    JavaFileObject file = JavaFileObjects.forSourceLines("test.InjectConstructor",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "import other.pkg.Outer;",
        "",
        "class InjectConstructor {",
        "  @Inject InjectConstructor(Outer.Factory factory) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines(
        "test.InjectConstructor_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "import other.pkg.Outer;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class InjectConstructor_Factory ",
        "    implements Factory<InjectConstructor> {",
        "",
        "  private final Provider<Outer.Factory> factoryProvider;",
        "",
        "  public InjectConstructor_Factory(Provider<Outer.Factory> factoryProvider) {",
        "    assert factoryProvider != null;",
        "    this.factoryProvider = factoryProvider;",
        "  }",
        "",
        "  @Override public InjectConstructor get() {",
        "    return new InjectConstructor(factoryProvider.get());",
        "  }",
        "",
        "  public static Factory<InjectConstructor> create(",
        "      Provider<Outer.Factory> factoryProvider) {",
        "    return new InjectConstructor_Factory(factoryProvider);",
        "  }",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(factoryFile, file))
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test
  public void samePackageNameCollision() {
    JavaFileObject samePackageInterface = JavaFileObjects.forSourceLines("test.CommonName",
        "package test;",
        "",
        "public interface CommonName {}");
    JavaFileObject differentPackageInterface = JavaFileObjects.forSourceLines(
        "other.pkg.CommonName",
        "package other.pkg;",
        "",
        "public interface CommonName {}");
    JavaFileObject file = JavaFileObjects.forSourceLines("test.InjectConstructor",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "class InjectConstructor implements CommonName {",
        "  @Inject InjectConstructor(other.pkg.CommonName otherPackage, CommonName samePackage) {}",
        "}");
    JavaFileObject expected = JavaFileObjects.forSourceLines(
        "test.InjectConstructor_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import javax.inject.Provider;",
        "import other.pkg.CommonName;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public final class InjectConstructor_Factory ",
        "    implements Factory<InjectConstructor> {",
        "",
        "  private final Provider<CommonName> otherPackageProvider;",
        "  private final Provider<test.CommonName> samePackageProvider;",
        "",
        "  public InjectConstructor_Factory(Provider<CommonName> otherPackageProvider,",
        "      Provider<test.CommonName> samePackageProvider) {",
        "    assert otherPackageProvider != null;",
        "    this.otherPackageProvider = otherPackageProvider;",
        "    assert samePackageProvider != null;",
        "    this.samePackageProvider = samePackageProvider;",
        "  }",
        "",
        "  @Override public InjectConstructor get() {",
        "    return new InjectConstructor(otherPackageProvider.get(), samePackageProvider.get());",
        "  }",
        "",
        "  public static Factory<InjectConstructor> create(",
        "      Provider<CommonName> otherPackageProvider,",
        "      Provider<test.CommonName> samePackageProvider) {",
        "    return new InjectConstructor_Factory(otherPackageProvider, samePackageProvider);",
        "  }",
        "}");
    assertAbout(javaSources())
        .that(ImmutableList.of(samePackageInterface, differentPackageInterface, file))
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(expected);
  }

  @Test
  public void noDeps() {
    JavaFileObject simpleType = JavaFileObjects.forSourceLines("test.SimpleType",
        "package test;",
        "",
        "import javax.inject.Inject;",
        "",
        "final class SimpleType {",
        "  @Inject SimpleType() {}",
        "}");
    JavaFileObject factory = JavaFileObjects.forSourceLines("test.SimpleType_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public enum SimpleType_Factory implements Factory<SimpleType> {",
        "  INSTANCE;",
        "",
        "  @Override public SimpleType get() {",
        "    return new SimpleType();",
        "  }",
        "",
        "  public static Factory<SimpleType> create() {",
        "    return INSTANCE;",
        "  }",
        "}");
    assertAbout(javaSource())
        .that(simpleType)
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(factory);
  }

  @Test public void simpleComponentWithNesting() {
    JavaFileObject nestedTypesFile = JavaFileObjects.forSourceLines("test.OuterType",
        "package test;",
        "",
        "import dagger.Component;",
        "import javax.inject.Inject;",
        "",
        "final class OuterType {",
        "  static class A {",
        "    @Inject A() {}",
        "  }",
        "  static class B {",
        "    @Inject A a;",
        "  }",
        "  @Component interface SimpleComponent {",
        "    A a();",
        "    void inject(B b);",
        "  }",
        "}");
    JavaFileObject aFactory = JavaFileObjects.forSourceLines(
        "test.OuterType$A_Factory",
        "package test;",
        "",
        "import dagger.internal.Factory;",
        "import javax.annotation.Generated;",
        "import test.OuterType.A;",
        "@Generated(\"dagger.internal.codegen.ComponentProcessor\")",
        "public enum OuterType$A_Factory implements Factory<A> {",
        "  INSTANCE;",
        "",
        "  @Override public A get() {",
        "    return new A();",
        "  }",
        "",
        "  public static Factory<A> create() {",
        "    return INSTANCE;",
        "  }",
        "}");
    assertAbout(javaSources()).that(ImmutableList.of(nestedTypesFile))
        .processedWith(new ComponentProcessor())
        .compilesWithoutError()
        .and().generatesSources(aFactory);
  }
}
