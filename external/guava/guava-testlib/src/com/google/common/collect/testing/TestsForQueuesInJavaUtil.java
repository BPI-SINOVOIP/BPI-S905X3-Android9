/*
 * Copyright (C) 2009 The Guava Authors
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

package com.google.common.collect.testing;

import com.google.common.collect.testing.features.CollectionFeature;
import com.google.common.collect.testing.features.CollectionSize;

import junit.framework.Test;
import junit.framework.TestSuite;

import java.lang.reflect.Method;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedList;
import java.util.PriorityQueue;
import java.util.Queue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.PriorityBlockingQueue;

/**
 * Generates a test suite covering the {@link Queue} implementations in the
 * {@link java.util} package. Can be subclassed to specify tests that should
 * be suppressed.
 *
 * @author Jared Levy
 */
public class TestsForQueuesInJavaUtil {
  public static Test suite() {
    return new TestsForQueuesInJavaUtil().allTests();
  }

  public Test allTests() {
    TestSuite suite = new TestSuite();
    suite.addTest(testsForLinkedList());
    suite.addTest(testsForArrayBlockingQueue());
    suite.addTest(testsForConcurrentLinkedQueue());
    suite.addTest(testsForLinkedBlockingQueue());
    suite.addTest(testsForPriorityBlockingQueue());
    suite.addTest(testsForPriorityQueue());
    return suite;
  }

  protected Collection<Method> suppressForLinkedList() {
    return Collections.emptySet();
  }
  protected Collection<Method> suppressForArrayBlockingQueue() {
    return Collections.emptySet();
  }
  protected Collection<Method> suppressForConcurrentLinkedQueue() {
    return Collections.emptySet();
  }
  protected Collection<Method> suppressForLinkedBlockingQueue() {
    return Collections.emptySet();
  }
  protected Collection<Method> suppressForPriorityBlockingQueue() {
    return Collections.emptySet();
  }
  protected Collection<Method> suppressForPriorityQueue() {
    return Collections.emptySet();
  }

  public Test testsForLinkedList() {
    return QueueTestSuiteBuilder
        .using(new TestStringQueueGenerator() {
            @Override public Queue<String> create(String[] elements) {
              return new LinkedList<String>(MinimalCollection.of(elements));
            }
          })
        .named("LinkedList")
        .withFeatures(
            CollectionFeature.GENERAL_PURPOSE,
            CollectionFeature.ALLOWS_NULL_VALUES,
            CollectionFeature.KNOWN_ORDER,
            CollectionSize.ANY)
        .skipCollectionTests() // already covered in TestsForListsInJavaUtil
        .suppressing(suppressForLinkedList())
        .createTestSuite();
  }

  public Test testsForArrayBlockingQueue() {
    return QueueTestSuiteBuilder
        .using(new TestStringQueueGenerator() {
            @Override public Queue<String> create(String[] elements) {
              return new ArrayBlockingQueue<String>(
                  100, false, MinimalCollection.of(elements));
            }
          })
        .named("ArrayBlockingQueue")
        .withFeatures(
            CollectionFeature.GENERAL_PURPOSE,
            CollectionFeature.KNOWN_ORDER,
            CollectionSize.ANY)
        .suppressing(suppressForArrayBlockingQueue())
        .createTestSuite();
  }

  public Test testsForConcurrentLinkedQueue() {
    return QueueTestSuiteBuilder
        .using(new TestStringQueueGenerator() {
            @Override public Queue<String> create(String[] elements) {
              return new ConcurrentLinkedQueue<String>(
                  MinimalCollection.of(elements));
            }
          })
        .named("ConcurrentLinkedQueue")
        .withFeatures(
            CollectionFeature.GENERAL_PURPOSE,
            CollectionFeature.KNOWN_ORDER,
            CollectionSize.ANY)
        .suppressing(suppressForConcurrentLinkedQueue())
        .createTestSuite();
  }

  public Test testsForLinkedBlockingQueue() {
    return QueueTestSuiteBuilder
        .using(new TestStringQueueGenerator() {
            @Override public Queue<String> create(String[] elements) {
              return new LinkedBlockingQueue<String>(
                  MinimalCollection.of(elements));
            }
          })
        .named("LinkedBlockingQueue")
        .withFeatures(
            CollectionFeature.GENERAL_PURPOSE,
            CollectionFeature.KNOWN_ORDER,
            CollectionSize.ANY)
        .suppressing(suppressForLinkedBlockingQueue())
        .createTestSuite();
  }

  // Not specifying KNOWN_ORDER for PriorityQueue and PriorityBlockingQueue
  // even though they do have it, because our tests interpret KNOWN_ORDER to
  // also mean that the iterator returns the head element first, which those
  // don't.

  public Test testsForPriorityBlockingQueue() {
    return QueueTestSuiteBuilder
        .using(new TestStringQueueGenerator() {
            @Override public Queue<String> create(String[] elements) {
              return new PriorityBlockingQueue<String>(
                  MinimalCollection.of(elements));
            }
          })
        .named("PriorityBlockingQueue")
        .withFeatures(
            CollectionFeature.GENERAL_PURPOSE,
            CollectionSize.ANY)
        .suppressing(suppressForPriorityBlockingQueue())
        .createTestSuite();
  }

  public Test testsForPriorityQueue() {
    return QueueTestSuiteBuilder
        .using(new TestStringQueueGenerator() {
            @Override public Queue<String> create(String[] elements) {
              return new PriorityQueue<String>(MinimalCollection.of(elements));
            }
          })
        .named("PriorityQueue")
        .withFeatures(
            CollectionFeature.GENERAL_PURPOSE,
            CollectionSize.ANY)
        .suppressing(suppressForPriorityQueue())
        .createTestSuite();
  }
}
