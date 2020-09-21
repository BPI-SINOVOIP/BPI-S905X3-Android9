/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.android.tradefed.device.ITestDevice;

import java.util.Collection;

/**
 * A {@link IRemoteTest} that can be split into separately executable sub-tests. The splitting into
 * sub-tests is expected to be deterministic and each sub-test should be independent in order to
 * allow for execution of different shards on different hosts.
 */
public interface IShardableTest extends IRemoteTest {

    /**
     * Shard the test into separately runnable chunks.
     *
     * <p>This must be deterministic and always return the same list of {@link IRemoteTest}s for the
     * same input.
     *
     * <p>This will be called before test execution, so injected dependencies (such as the {@link
     * ITestDevice} for {@link IDeviceTest}s) may be null.
     *
     * @return a collection of subtests to be executed separately or <code>null</code> if test is
     *     not currently shardable
     */
    public default Collection<IRemoteTest> split() {
        return null;
    }

    /**
     * Alternative version of {@link #split()} which also provides the shardCount that is attempted
     * to be run. This is useful for some test runner that cannot arbitrarly decide sometimes.
     *
     * @param shardCountHint the attempted shard count.
     * @return a collection of subtests to be executed separately or <code>null</code> if test is
     *     not currently shardable
     */
    public default Collection<IRemoteTest> split(int shardCountHint) {
        return split();
    }
}
