/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package com.squareup.okhttp;

import org.junit.Test;

import libcore.net.event.NetworkEventDispatcher;

import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;

/**
 * Tests for {@link ConfigAwareConnectionPool}.
 */
public class ConfigAwareConnectionPoolTest {

  @Test
  public void getInstance() {
    assertSame(ConfigAwareConnectionPool.getInstance(), ConfigAwareConnectionPool.getInstance());
  }

  @Test
  public void get() throws Exception {
    NetworkEventDispatcher networkEventDispatcher = new NetworkEventDispatcher() {};
    ConfigAwareConnectionPool instance = new ConfigAwareConnectionPool(networkEventDispatcher) {};
    assertSame(instance.get(), instance.get());

    ConnectionPool beforeEventInstance = instance.get();
    networkEventDispatcher.onNetworkConfigurationChanged();

    assertNotSame(beforeEventInstance, instance.get());
  }
}
