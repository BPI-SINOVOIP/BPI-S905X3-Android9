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

package com.android.tradefed.command;

import com.android.tradefed.util.UniqueMultiMap;

/**
 *  Container for execution options for commands.
 */
public interface ICommandOptions {

    /**
     * Returns <code>true</code> if abbreviated help mode has been requested
     */
    public boolean isHelpMode();

    /**
     * Returns <code>true</code> if full detailed help mode has been requested
     */
    public boolean isFullHelpMode();

    /**
     * Returns <code>true</code> if full json help mode has been requested
     */
    public boolean isJsonHelpMode();

    /**
     * Return <code>true</code> if we should <emph>skip</emph> adding this command to the queue.
     */
    public boolean isDryRunMode();

    /**
     * Return <code>true</code> if we should print the command out to the console before we
     * <emph>skip</emph> adding it to the queue.
     */
    public boolean isNoisyDryRunMode();

    /**
     * Return the loop mode for the config.
     */
    public boolean isLoopMode();

    /**
     * Get the min loop time for the config.
     *
     * @deprecated use {@link #getLoopTime()} instead
     */
    @Deprecated
    public long getMinLoopTime();

    /**
     * Get the time to wait before re-scheduling this command.
     * @return time in ms
     */
    public long getLoopTime();

    /**
     * Sets the loop mode for the command
     *
     * @param loopMode
     */
    public void setLoopMode(boolean loopMode);

    /**
     * Return the test-tag for the invocation. Default is 'stub' if unspecified.
     */
    public String getTestTag();

    /**
     * Sets the test-tag for the invocation.
     *
     * @param testTag
     */
    public void setTestTag(String testTag);

    /**
     * Return the test-tag suffix, appended to test-tag to represents some variants of one test.
     */
    public String getTestTagSuffix();

    /**
     * Creates a copy of the {@link ICommandOptions} object.
     */
    public ICommandOptions clone();

    /**
     * Return true if command should run on all devices.
     */
    public boolean runOnAllDevices();

    /**
     * Return true if a bugreport should be taken when the test invocation has ended.
     */
    public boolean takeBugreportOnInvocationEnded();

    /** Sets whether or not to capture a bugreport at the end of the invocation. */
    public void setBugreportOnInvocationEnded(boolean takeBugreport);

    /**
     * Return true if a bugreportz should be taken instead of bugreport during the test invocation
     * final bugreport.
     */
    public boolean takeBugreportzOnInvocationEnded();

    /** Sets whether or not to capture a bugreportz at the end of the invocation. */
    public void setBugreportzOnInvocationEnded(boolean takeBugreportz);

    /**
     * Return the invocation timeout specified. 0 if no timeout to be used.
     */
    public long getInvocationTimeout();

    /**
     * Set the invocation timeout. 0 if no timeout to be used.
     */
    public void setInvocationTimeout(Long mInvocationTimeout);

    /**
     * Return the total shard count for the command.
     */
    public Integer getShardCount();

    /**
     * Sets the shard count for the command.
     */
    public void setShardCount(Integer shardCount);

    /**
     * Return the shard index for the command.
     */
    public Integer getShardIndex();

    /**
     * Sets the shard index for the command.
     */
    public void setShardIndex(Integer shardIndex);

    /** Return true if the test should skip device setup during TestInvocation setup. */
    public boolean shouldSkipPreDeviceSetup();

    /** Returns if we should use dynamic sharding or not */
    public boolean shouldUseDynamicSharding();

    /** Returns the data passed to the invocation to describe it */
    public UniqueMultiMap<String, String> getInvocationData();

    /** Returns true if we should use Tf new sharding logic */
    public boolean shouldUseTfSharding();

    /** Returns true if we should use Tf containers to run the invocation */
    public boolean shouldUseSandboxing();

    /** Sets whether or not we should use TF containers */
    public void setShouldUseSandboxing(boolean use);
}
