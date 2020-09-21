/*
 * Copyright (C) 2008 The Guava Authors
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
public class FakeTickerTest_gwt extends com.google.gwt.junit.client.GWTTestCase {
@Override public String getModuleName() {
  return "com.google.common.testing.testModule";
}
public void testAdvance() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAdvance();
}

public void testAutoIncrementStep_millis() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAutoIncrementStep_millis();
}

public void testAutoIncrementStep_nanos() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAutoIncrementStep_nanos();
}

public void testAutoIncrementStep_resetToZero() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAutoIncrementStep_resetToZero();
}

public void testAutoIncrementStep_returnsSameInstance() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAutoIncrementStep_returnsSameInstance();
}

public void testAutoIncrementStep_seconds() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAutoIncrementStep_seconds();
}

public void testAutoIncrement_negative() throws Exception {
  com.google.common.testing.FakeTickerTest testCase = new com.google.common.testing.FakeTickerTest();
  testCase.testAutoIncrement_negative();
}
}
