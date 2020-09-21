/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.jpda.tests.share;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.util.ArrayList;

/**
 * A class that allows one to observe GCs and finalization.
 */
public class GcMarker {

  private final ReferenceQueue mQueue;
  private final ArrayList<PhantomReference> mList;

  public GcMarker() {
    mQueue = new ReferenceQueue();
    mList = new ArrayList<PhantomReference>(3);
  }

  public void add(Object referent) {
    mList.add(new PhantomReference(referent, mQueue));
  }

  public void waitForGc(int numberOfExpectedFinalizations) {
    if (numberOfExpectedFinalizations > mList.size()) {
      throw new IllegalArgumentException("wait condition will never be met");
    }

    // Request finalization of objects, and subsequent reference enqueueing.
    // Repeat until reference queue reaches expected size.
    do {
        System.runFinalization();
        Runtime.getRuntime().gc();
        try { Thread.sleep(10); } catch (Exception e) {}
    } while (isLive(numberOfExpectedFinalizations));
  }

  private boolean isLive(int numberOfExpectedFinalizations) {
    int numberFinalized = 0;
    for (int i = 0, n = mList.size(); i < n; i++) {
      if (mList.get(i).isEnqueued()) {
        numberFinalized++;
      }
    }
    return numberFinalized < numberOfExpectedFinalizations;
  }
}
