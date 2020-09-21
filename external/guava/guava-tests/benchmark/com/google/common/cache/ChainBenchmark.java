/*
 * Copyright (C) 2011 The Guava Authors
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

package com.google.common.cache;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Benchmark;
import com.google.caliper.Param;
import com.google.common.cache.LocalCache.ReferenceEntry;
import com.google.common.cache.LocalCache.Segment;

/**
 * Benchmark for {@code LocalCache.Segment.removeEntryFromChain}.
 *
 * @author Charles Fry
 */
public class ChainBenchmark {

  @Param({"1", "2", "3", "4", "5", "6"}) int length;

  private Segment<Object, Object> segment;
  private ReferenceEntry<Object, Object> head;
  private ReferenceEntry<Object, Object> chain;

  @BeforeExperiment
  void setUp() {
    LocalCache<Object, Object> cache = new LocalCache<Object, Object>(
        CacheBuilder.newBuilder()
            .concurrencyLevel(1), null);
    segment = cache.segments[0];
    chain = null;
    for (int i = 0; i < length; i++) {
      Object key = new Object();
      chain = segment.newEntry(key, cache.hash(key), chain);
      if (i == 0) {
        head = chain;
      }
    }
  }

  @Benchmark int time(int reps) {
    int dummy = 0;
    for (int i = 0; i < reps; i++) {
      segment.removeEntryFromChain(chain, head);
      dummy += segment.count;
    }
    return dummy;
  }
}
