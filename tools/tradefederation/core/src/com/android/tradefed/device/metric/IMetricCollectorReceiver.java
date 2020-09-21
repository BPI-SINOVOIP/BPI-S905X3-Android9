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
package com.android.tradefed.device.metric;

import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.suite.ITestSuite;

import java.util.List;

/**
 * Interface for {@link IRemoteTest}s to implement if they need to get the list of {@link
 * IMetricCollector}s for the test run.
 *
 * <p>Tests implementing this interface will not have their default {@link ITestInvocationListener}
 * instrumented with the collectors, they will have to do it themselves via {@link
 * IMetricCollector#init(IInvocationContext, ITestInvocationListener)}.
 *
 * <p>Some tests mechanisms involved buffering Tradefed callbacks and replaying it at the end (like
 * in {@link ITestSuite}), such mechanism would results in the collectors being called during the
 * replay and not during the actual execution. By letting tests runner handle when to use the
 * collectors we can ensure the callbacks being handled at the proper time.
 *
 * <pre>In order to use the collectors, the following pattern can be used:
 * for (IMetricCollector collector : config.getMetricCollectors()) {
 *     originalCollector = collector.init(mModuleInvocationContext, originalCollector);
 * }
 * </pre>
 *
 * The originalCollector will have all the metric collector wrapped around it to be called in
 * sequence.
 *
 * @see IMetricCollector#init(IInvocationContext, ITestInvocationListener)
 */
public interface IMetricCollectorReceiver {

    /** Sets the list of {@link IMetricCollector}s defined for the test run. */
    public void setMetricCollectors(List<IMetricCollector> collectors);
}
