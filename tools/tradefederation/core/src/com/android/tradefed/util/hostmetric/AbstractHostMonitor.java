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
package com.android.tradefed.util.hostmetric;

import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VersionParser;

import java.net.InetAddress;
import java.util.HashMap;
import java.util.Map;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Typical class for Host Health Monitoring. implementing dispatch() with specifics of the agent.
 */
public abstract class AbstractHostMonitor extends Thread implements IHostMonitor {
    @Option(
        name = "dispatch-interval",
        description = "the time interval between dispatches in ms",
        isTimeVal = true
    )
    private long mDispatchInterval = 15 * 1000;

    @Option(name = "agent-name", description = "the name of the agent object to be used")
    private String mAgentName = "host_metric_agent";

    @Option(name = "event-tag", description = "Event Tag that will be accepted by the Monitor.")
    private HostMetricType mTag = HostMetricType.NONE;

    protected Queue<HostDataPoint> mHostEvents = new LinkedBlockingQueue<HostDataPoint>();

    protected Map<String, String> mHostData = new HashMap<>();

    private boolean mIsCanceled = false;

    public AbstractHostMonitor() {
        super("AbstractHostMonitor");
        this.setDaemon(true);
    }

    /**
     * Collect and Emits the current host data values. Should emits the Events of the Queue if any.
     */
    public abstract void dispatch();

    /** Return the tag identifying which 'class' of {@link IHostMonitor} to reach. */
    public HostMetricType getTag() {
        return mTag;
    }

    @Override
    public void run() {
        try {
            mHostData.put("hostname", InetAddress.getLocalHost().getHostName());
            mHostData.put("tradefed_version", VersionParser.fetchVersion());

            while (!mIsCanceled) {
                dispatch();
                getRunUtil().sleep(mDispatchInterval);
            }
        } catch (Exception e) {
            CLog.e(e);
        }
    }

    /** {@inheritDoc} */
    @Override
    public synchronized void addHostEvent(HostMetricType tag, HostDataPoint event) {
        if (getTag().equals(tag)) {
            mHostEvents.add(event);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void terminate() {
        mIsCanceled = true;
    }

    IHostHealthAgent getMetricAgent() {
        IHostHealthAgent agent =
                (IHostHealthAgent)
                        GlobalConfiguration.getInstance().getConfigurationObject(mAgentName);
        return agent;
    }

    IRunUtil getRunUtil() {
        return RunUtil.getDefault();
    }

    int getQueueSize() {
        return mHostEvents.size();
    }
}
