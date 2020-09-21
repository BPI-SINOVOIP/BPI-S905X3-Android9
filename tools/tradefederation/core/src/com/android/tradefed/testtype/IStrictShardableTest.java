/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.tradefed.testtype;

/**
 * A {@link IRemoteTest} that can be sharded into separately executable sub-tests. The splitting
 * into sub-tests is expected to be deterministic and each sub-test should be independent in order
 * to allow for execution of different shards on different hosts.
 */
public interface IStrictShardableTest extends IRemoteTest {

    /**
     * Returns a {@link IRemoteTest} for a single shard. This must be deterministic and always
     * return the same {@link IRemoteTest} for the same input.
     *
     * @param shardCount the number of total shards
     * @param shardIndex the index of a test shard to return. The value is in range [0, shardCount).
     * @return a {@link IRemoteTest}
     */
    IRemoteTest getTestShard(int shardCount, int shardIndex);

}
