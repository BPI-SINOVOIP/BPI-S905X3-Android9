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
package com.android.tradefed.invoker;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ILogSaverListener;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.result.LogFile;
import com.android.tradefed.result.LogSaverResultForwarder;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.util.TimeUtil;

import java.util.ArrayList;
import java.util.List;
import java.util.Map.Entry;

/**
 * A {@link ResultForwarder} that combines the results of a sharded test invocations. It only
 * reports completion of the invocation to the listeners once all sharded invocations are complete.
 *
 * <p>This class is not thread safe. It is expected that clients will lock on this class when
 * sending test results, to prevent invocation callbacks from being called out of order.
 */
public class ShardMasterResultForwarder extends ResultForwarder implements ILogSaverListener {

    private final int mInitCount;
    private int mShardsRemaining;
    private long mTotalElapsed = 0L;
    private boolean mStartReported = false;

    private long mFirstShardEndTime = 0L;
    private IInvocationContext mOriginalContext;
    private List<IInvocationContext> mShardContextList;
    private int shardIndex = 0;

    /**
     * Create a {@link ShardMasterResultForwarder}.
     *
     * @param listeners the list of {@link ITestInvocationListener} to forward results to when all
     *     shards are completed
     * @param expectedShards the number of shards
     */
    public ShardMasterResultForwarder(List<ITestInvocationListener> listeners, int expectedShards) {
        super(listeners);
        mShardsRemaining = expectedShards;
        mInitCount = expectedShards;
        mShardContextList = new ArrayList<>();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationStarted(IInvocationContext context) {
        if (!mStartReported) {
            mOriginalContext = context;
            super.invocationStarted(context);
            mStartReported = true;
        } else {
            // Track serials used in each shard.
            mOriginalContext.addSerialsFromShard(shardIndex, context.getSerials());
            mShardContextList.add(context);
            shardIndex++;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationFailed(Throwable cause) {
        // one of the shards failed. Fail the whole invocation
        // TODO: does any extra logging need to be done ?
        super.invocationFailed(cause);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void invocationEnded(long elapsedTime) {
        mTotalElapsed += elapsedTime;
        if (mInitCount == mShardsRemaining) {
            mFirstShardEndTime = System.currentTimeMillis();
        }
        mShardsRemaining--;
        if (mShardsRemaining <= 0) {
            // TODO: consider logging all shard final times.
            CLog.i(
                    "There was %s between the first and last shard ended.",
                    TimeUtil.formatElapsedTime(System.currentTimeMillis() - mFirstShardEndTime));
            copyShardBuildInfoToMain(mOriginalContext, mShardContextList);
            super.invocationEnded(mTotalElapsed);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void testLogSaved(
            String dataName, LogDataType dataType, InputStreamSource dataStream, LogFile logFile) {
        for (ITestInvocationListener listener : getListeners()) {
            try {
                // Forward the testLogSaved event to ILogSaverListener
                if (listener instanceof ILogSaverListener) {
                    ((ILogSaverListener) listener)
                            .testLogSaved(dataName, dataType, dataStream, logFile);
                }
            } catch (Exception e) {
                CLog.e("Exception while invoking %s#testLogSaved", listener.getClass().getName());
                CLog.e(e);
            }
        }
    }

    /** Only forward the testLog instead of saving the log first. */
    public void testLogForward(
            String dataName, LogDataType dataType, InputStreamSource dataStream) {
        for (ITestInvocationListener listener : getListeners()) {
            if (listener instanceof LogSaverResultForwarder) {
                // If the listener is a log saver, we should simply forward the testLog not save
                // again.
                ((LogSaverResultForwarder) listener).testLogForward(dataName, dataType, dataStream);
            } else {
                try {
                    listener.testLog(dataName, dataType, dataStream);
                } catch (RuntimeException e) {
                    CLog.e(
                            "RuntimeException while invoking %s#testLog",
                            listener.getClass().getName());
                    CLog.e(e);
                }
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void logAssociation(String dataName, LogFile logFile) {
        for (ITestInvocationListener listener : getListeners()) {
            try {
                // Forward the logAssociation call
                if (listener instanceof ILogSaverListener) {
                    ((ILogSaverListener) listener).logAssociation(dataName, logFile);
                }
            } catch (RuntimeException e) {
                CLog.e("Failed to provide the log association");
                CLog.e(e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        // Shard master does not need log saver.
    }

    /**
     * Copy the build info from the shard builds to the main build in the original invocation
     * context.
     *
     * @param main the original {@link IInvocationContext} from the main invocation.
     * @param shardContexts the list of {@link IInvocationContext}s, one for each shard invocation.
     */
    private void copyShardBuildInfoToMain(
            IInvocationContext main, List<IInvocationContext> shardContexts) {
        for (IInvocationContext shard : shardContexts) {
            for (String deviceName : shard.getDeviceConfigNames()) {
                IBuildInfo shardBuild = shard.getBuildInfo(deviceName);
                IBuildInfo mainBuild = main.getBuildInfo(deviceName);
                if (mainBuild != null) {
                    for (Entry<String, String> entry : shardBuild.getBuildAttributes().entrySet()) {
                        mainBuild.addBuildAttribute(entry.getKey(), entry.getValue());
                    }
                } else {
                    // Should not happen
                    CLog.e(
                            "Found a device '%s' in shard configuration but not in parent configuration.",
                            deviceName);
                }
            }
        }
    }
}
