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

package com.google.common.eventbus;

import com.google.common.testing.AbstractPackageSanityTests;

import java.lang.reflect.Method;

import javax.annotation.Nullable;

/**
 * Basic sanity tests for the entire package.
 *
 * @author Ben Yu
 */

public class PackageSanityTests extends AbstractPackageSanityTests {

  public PackageSanityTests() throws Exception {
    setDefault(EventSubscriber.class, new DummySubscriber().toEventSubscriber());
    setDefault(Method.class, DummySubscriber.subscriberMethod());
  }

  private static class DummySubscriber {

    @SuppressWarnings("unused") // Used by reflection
    public void handle(@Nullable Object anything) {}

    EventSubscriber toEventSubscriber() throws Exception {
      return new EventSubscriber(this, subscriberMethod());
    }

    private static Method subscriberMethod() throws NoSuchMethodException {
      return DummySubscriber.class.getMethod("handle", Object.class);
    }
  }
}
