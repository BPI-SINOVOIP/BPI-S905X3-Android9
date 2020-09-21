/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.tradefed.invoker.shard;

import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.IRescheduler;

/** Interface of an object that describes the sharding strategy to adopt for a configuration. */
public interface IShardHelper {

    /**
     * Shard and reschedule the shard if possible.
     *
     * @param config a {@link IConfiguration} to be run.
     * @param context the {@link IInvocationContext} of the current invocation.
     * @param rescheduler the {@link IRescheduler} where to reschedule the shards.
     * @return True if the configuration was sharded. false otherwise.
     */
    public boolean shardConfig(
            IConfiguration config, IInvocationContext context, IRescheduler rescheduler);
}
