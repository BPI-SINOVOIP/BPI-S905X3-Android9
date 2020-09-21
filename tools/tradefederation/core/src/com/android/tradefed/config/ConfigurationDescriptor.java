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
package com.android.tradefed.config;

import com.android.tradefed.build.BuildSerializedVersion;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.util.MultiMap;

import com.google.common.annotations.VisibleForTesting;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * Configuration Object that describes some aspect of the configuration itself. Like a membership
 * test-suite-tag. This class cannot receive option values via command line. Only directly in the
 * xml.
 */
@OptionClass(alias = "config-descriptor")
public class ConfigurationDescriptor implements Serializable {
    private static final long serialVersionUID = BuildSerializedVersion.VERSION;

    @Option(name = "test-suite-tag", description = "A membership tag to suite. Can be repeated.")
    private List<String> mSuiteTags = new ArrayList<>();

    @Option(name = "metadata", description = "Metadata associated with this configuration, can be "
            + "free formed key value pairs, and a key may be associated with multiple values.")
    private MultiMap<String, String> mMetaData = new MultiMap<>();

    @Option(
        name = "not-shardable",
        description =
                "A metadata that allows a suite configuration to specify that it cannot be "
                        + "sharded. Not because it doesn't support it but because it doesn't make "
                        + "sense."
    )
    private boolean mNotShardable = false;

    @Option(
        name = "not-strict-shardable",
        description =
                "A metadata to allows a suite configuration to specify that it cannot be "
                        + "sharded in a strict context (independent shards). If a config is already "
                        + "not-shardable, it will be not-strict-shardable."
    )
    private boolean mNotStrictShardable = false;

    @Option(
        name = "use-sandboxing",
        description = "Option used to notify an invocation that it is running in a sandbox."
    )
    private boolean mUseSandboxing = false;

    /** Optional Abi information the configuration will be run against. */
    private IAbi mAbi = null;
    /** Optional for a module configuration, the original name of the module. */
    private String mModuleName = null;

    /** Returns the list of suite tags the test is part of. */
    public List<String> getSuiteTags() {
        return mSuiteTags;
    }

    /** Sets the list of suite tags the test is part of. */
    public void setSuiteTags(List<String> suiteTags) {
        mSuiteTags = suiteTags;
    }

    /** Retrieves all configured metadata and return a copy of the map. */
    public MultiMap<String, String> getAllMetaData() {
        MultiMap<String, String> copy = new MultiMap<>();
        copy.putAll(mMetaData);
        return copy;
    }

    /** Get the named metadata entries */
    public List<String> getMetaData(String name) {
        return mMetaData.get(name);
    }

    @VisibleForTesting
    public void setMetaData(MultiMap<String, String> metadata) {
        mMetaData = metadata;
    }

    /** Returns if the configuration is shardable or not as part of a suite */
    public boolean isNotShardable() {
        return mNotShardable;
    }

    /** Returns if the configuration is strict shardable or not as part of a suite */
    public boolean isNotStrictShardable() {
        return mNotStrictShardable;
    }

    /** Sets the abi the configuration is going to run against. */
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /** Returns the abi the configuration is running against if known, null otherwise. */
    public IAbi getAbi() {
        return mAbi;
    }

    /** If this configuration represents a module, we can set the module name associated with it. */
    public void setModuleName(String name) {
        mModuleName = name;
    }

    /** Returns the module name of the module configuration. */
    public String getModuleName() {
        return mModuleName;
    }

    /** Returns true if the invocation should run in sandboxed mode. False otherwise. */
    public boolean shouldUseSandbox() {
        return mUseSandboxing;
    }

    /** Sets whether or not a config will run in sandboxed mode or not. */
    public void setSandboxed(boolean useSandboxed) {
        mUseSandboxing = useSandboxed;
    }
}
