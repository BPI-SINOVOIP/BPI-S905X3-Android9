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
import com.google.common.collect.Iterables;
import com.google.testing.compile.CompilationRule;
import dagger.internal.codegen.MethodSignatureFormatterTest.OuterClass.InnerClass;
import javax.inject.Singleton;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.TypeElement;
import javax.lang.model.util.Elements;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import static com.google.common.truth.Truth.assertThat;
import static javax.lang.model.util.ElementFilter.methodsIn;

@RunWith(JUnit4.class)
public class MethodSignatureFormatterTest {
  @Rule public CompilationRule compilationRule = new CompilationRule();

  static class OuterClass {
    @interface Foo {
       Class<?> bar();
    }

    static class InnerClass {
      @Foo(bar = String.class)
      @Singleton
      String foo(@SuppressWarnings("unused") int a, ImmutableList<Boolean> blah) { return "foo"; }
    }
  }

  @Test public void methodSignatureTest() {
    Elements elements = compilationRule.getElements();
    TypeElement inner = elements.getTypeElement(InnerClass.class.getCanonicalName());
    ExecutableElement method = Iterables.getOnlyElement(methodsIn(inner.getEnclosedElements()));
    String formatted = new MethodSignatureFormatter(compilationRule.getTypes()).format(method);
    // This is gross, but it turns out that annotation order is not guaranteed when getting
    // all the AnnotationMirrors from an Element, so I have to test this chopped-up to make it
    // less brittle.
    assertThat(formatted).contains("@Singleton");
    assertThat(formatted).doesNotContain("@javax.inject.Singleton"); // maybe more importantly
    assertThat(formatted)
        .contains("@dagger.internal.codegen.MethodSignatureFormatterTest.OuterClass.Foo"
            + "(bar=String.class)");
    assertThat(formatted).contains(" String "); // return type compressed
    assertThat(formatted).contains("int, ImmutableList<Boolean>)"); // parameters compressed.
  }
}
