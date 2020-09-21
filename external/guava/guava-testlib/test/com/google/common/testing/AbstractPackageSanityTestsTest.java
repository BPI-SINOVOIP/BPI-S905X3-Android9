/*
 * Copyright (C) 2012 The Guava Authors
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

package com.google.common.testing;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.base.Predicates;
import com.google.common.collect.ImmutableList;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link AbstractPackageSanityTests}.
 *
 * @author Ben Yu
 */
public class AbstractPackageSanityTestsTest extends TestCase {

  private final AbstractPackageSanityTests sanityTests = new AbstractPackageSanityTests() {};

  public void testFindClassesToTest_testClass() {
    assertThat(findClassesToTest(ImmutableList.of(EmptyTest.class)))
        .isEmpty();
    assertThat(findClassesToTest(ImmutableList.of(EmptyTests.class)))
        .isEmpty();
    assertThat(findClassesToTest(ImmutableList.of(EmptyTestCase.class)))
        .isEmpty();
    assertThat(findClassesToTest(ImmutableList.of(EmptyTestSuite.class)))
        .isEmpty();
  }

  public void testFindClassesToTest_noCorrespondingTestClass() {
    assertThat(findClassesToTest(ImmutableList.of(Foo.class)))
        .has().exactly(Foo.class).inOrder();
    assertThat(findClassesToTest(ImmutableList.of(Foo.class, Foo2Test.class)))
        .has().exactly(Foo.class).inOrder();
  }

  public void testFindClassesToTest_publicApiOnly() {
    sanityTests.publicApiOnly();
    assertThat(findClassesToTest(ImmutableList.of(Foo.class)))
        .isEmpty();
    assertThat(findClassesToTest(ImmutableList.of(PublicFoo.class))).has().item(PublicFoo.class);
  }

  public void testFindClassesToTest_ignoreClasses() {
    sanityTests.ignoreClasses(Predicates.<Object>equalTo(PublicFoo.class));
    assertThat(findClassesToTest(ImmutableList.of(PublicFoo.class)))
        .isEmpty();
    assertThat(findClassesToTest(ImmutableList.of(Foo.class))).has().item(Foo.class);
  }

  public void testFindClassesToTest_withCorrespondingTestClassButNotExplicitlyTested() {
    assertThat(findClassesToTest(ImmutableList.of(Foo.class, FooTest.class), "testNotThere"))
        .has().exactly(Foo.class).inOrder();
    assertThat(findClassesToTest(ImmutableList.of(Foo.class, FooTest.class), "testNotPublic"))
        .has().exactly(Foo.class).inOrder();
  }

  public void testFindClassesToTest_withCorrespondingTestClassAndExplicitlyTested() {
    ImmutableList<Class<? extends Object>> classes = ImmutableList.of(Foo.class, FooTest.class);
    assertThat(findClassesToTest(classes, "testPublic"))
        .isEmpty();
    assertThat(findClassesToTest(classes, "testNotThere", "testPublic"))
        .isEmpty();
  }

  public void testFindClassesToTest_withCorrespondingTestClass_noTestName() {
    assertThat(findClassesToTest(ImmutableList.of(Foo.class, FooTest.class)))
        .has().exactly(Foo.class).inOrder();
  }

  static class EmptyTestCase {}

  static class EmptyTest {}

  static class EmptyTests {}

  static class EmptyTestSuite {}

  static class Foo {}

  public static class PublicFoo {}

  static class FooTest {
    @SuppressWarnings("unused") // accessed reflectively
    public void testPublic() {}
    @SuppressWarnings("unused") // accessed reflectively
    void testNotPublic() {}
  }

  // Shouldn't be mistaken as Foo's test
  static class Foo2Test {
    @SuppressWarnings("unused") // accessed reflectively
    public void testPublic() {}
  }

  private List<Class<?>> findClassesToTest(
      Iterable<? extends Class<?>> classes, String... explicitTestNames) {
    return sanityTests.findClassesToTest(classes, Arrays.asList(explicitTestNames));
  }
}
