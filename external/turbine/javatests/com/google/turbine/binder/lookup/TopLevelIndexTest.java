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

package com.google.turbine.binder.lookup;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;

import com.google.common.collect.ImmutableList;
import com.google.turbine.binder.sym.ClassSymbol;
import java.util.NoSuchElementException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class TopLevelIndexTest {

  private static final TopLevelIndex index = buildIndex();

  private static TopLevelIndex buildIndex() {
    TopLevelIndex.Builder builder = TopLevelIndex.builder();
    builder.insert(new ClassSymbol("java/util/Map"));
    builder.insert(new ClassSymbol("java/util/List"));
    builder.insert(new ClassSymbol("com/google/common/base/Optional"));
    return builder.build();
  }

  @Test
  public void simple() {
    LookupResult result = index.lookup(new LookupKey(ImmutableList.of("java", "util", "Map")));
    assertThat(result.sym()).isEqualTo(new ClassSymbol("java/util/Map"));
    assertThat(result.remaining()).isEmpty();
  }

  @Test
  public void nested() {
    LookupResult result =
        index.lookup(new LookupKey(ImmutableList.of("java", "util", "Map", "Entry")));
    assertThat(result.sym()).isEqualTo(new ClassSymbol("java/util/Map"));
    assertThat(result.remaining()).containsExactly("Entry");
  }

  @Test
  public void empty() {
    assertThat(index.lookup(new LookupKey(ImmutableList.of("java", "NoSuch", "Entry")))).isNull();
    assertThat(index.lookupPackage(ImmutableList.of("java", "math"))).isNull();
    assertThat(index.lookupPackage(ImmutableList.of("java", "util", "Map"))).isNull();
  }

  @Test
  public void packageScope() {
    Scope scope = index.lookupPackage(ImmutableList.of("java", "util"));

    assertThat(scope.lookup(new LookupKey(ImmutableList.of("Map"))).sym())
        .isEqualTo(new ClassSymbol("java/util/Map"));
    assertThat(scope.lookup(new LookupKey(ImmutableList.of("List"))).sym())
        .isEqualTo(new ClassSymbol("java/util/List"));
    assertThat(scope.lookup(new LookupKey(ImmutableList.of("NoSuch")))).isNull();
  }

  @Test
  public void overrideClass() {
    {
      // the use of Foo as a class name in the package java is "sticky"
      TopLevelIndex.Builder builder = TopLevelIndex.builder();
      builder.insert(new ClassSymbol("java/Foo"));
      assertThat(builder.insert(new ClassSymbol("java/Foo/Bar"))).isFalse();
      TopLevelIndex index = builder.build();

      LookupResult result = index.lookup(new LookupKey(ImmutableList.of("java", "Foo")));
      assertThat(result.sym()).isEqualTo(new ClassSymbol("java/Foo"));
      assertThat(result.remaining()).isEmpty();
    }
    {
      // the use of Foo as a package name under java is "sticky"
      TopLevelIndex.Builder builder = TopLevelIndex.builder();
      builder.insert(new ClassSymbol("java/Foo/Bar"));
      assertThat(builder.insert(new ClassSymbol("java/Foo"))).isFalse();
      TopLevelIndex index = builder.build();

      assertThat(index.lookup(new LookupKey(ImmutableList.of("java", "Foo")))).isNull();
      LookupResult packageResult =
          index
              .lookupPackage(ImmutableList.of("java", "Foo"))
              .lookup(new LookupKey(ImmutableList.of("Bar")));
      assertThat(packageResult.sym()).isEqualTo(new ClassSymbol("java/Foo/Bar"));
      assertThat(packageResult.remaining()).isEmpty();
    }
  }

  @Test
  public void emptyLookup() {
    LookupKey key = new LookupKey(ImmutableList.of("java", "util", "List"));
    key = key.rest();
    key = key.rest();
    try {
      key.rest();
      fail("expected exception");
    } catch (NoSuchElementException e) {
      // expected
    }
  }
}
