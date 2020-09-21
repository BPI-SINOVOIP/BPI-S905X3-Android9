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

package vogar.target;

import java.util.Arrays;
import org.junit.runner.Description;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;
import vogar.Result;
import vogar.monitor.TargetMonitor;
import vogar.target.junit.JUnitUtils;

/**
 * A {@link RunListener} that will notify the vogar main process of the results of the tests.
 */
public class TargetMonitorRunListener extends RunListener {

    private final TargetMonitor monitor;
    private Failure failure;

    public TargetMonitorRunListener(TargetMonitor monitor) {
        this.monitor = monitor;
    }

    @Override
    public void testStarted(Description description) throws Exception {
        failure = null;
        monitor.outcomeStarted(JUnitUtils.getTestName(description));
    }

    @Override
    public void testFailure(Failure failure) throws Exception {
        this.failure = failure;
    }

    @Override
    public void testFinished(Description description) throws Exception {
        if (failure == null) {
            monitor.outcomeFinished(Result.SUCCESS);
        } else {
            @SuppressWarnings("ThrowableResultOfMethodCallIgnored")
            Throwable thrown = failure.getException();
            prepareForDisplay(thrown);
            thrown.printStackTrace(System.out);
            monitor.outcomeFinished(Result.EXEC_FAILED);
        }
    }

    /**
     * Strip vogar's lines from the stack trace. For example, we'd strip the
     * first two Assert lines and everything after the testFoo() line in this
     * stack trace:
     *
     *   at junit.framework.Assert.fail(Assert.java:198)
     *   at junit.framework.Assert.assertEquals(Assert.java:56)
     *   at junit.framework.Assert.assertEquals(Assert.java:61)
     *   at libcore.java.net.FooTest.testFoo(FooTest.java:124)
     *   at java.lang.reflect.Method.invokeNative(Native Method)
     *   at java.lang.reflect.Method.invoke(Method.java:491)
     *   at vogar.target.junit.Junit$JUnitTest.run(Junit.java:214)
     *   at vogar.target.junit.JUnitRunner$1.call(JUnitRunner.java:112)
     *   at vogar.target.junit.JUnitRunner$1.call(JUnitRunner.java:105)
     *   at java.util.concurrent.FutureTask$Sync.innerRun(FutureTask.java:305)
     *   at java.util.concurrent.FutureTask.run(FutureTask.java:137)
     *   at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1076)
     *   at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:569)
     *   at java.lang.Thread.run(Thread.java:863)
     */
    private static void prepareForDisplay(Throwable t) {
        StackTraceElement[] stackTraceElements = t.getStackTrace();
        boolean foundVogar = false;

        int last = stackTraceElements.length - 1;
        for (; last >= 0; last--) {
            String className = stackTraceElements[last].getClassName();
            if (className.startsWith("vogar.target")) {
                foundVogar = true;
            } else if (foundVogar
                    && !className.startsWith("java.lang.reflect")
                    && !className.startsWith("sun.reflect")
                    && !className.startsWith("junit.framework")) {
                if (last < stackTraceElements.length) {
                    last++;
                }
                break;
            }
        }

        int first = 0;
        for (; first < last; first++) {
            String className = stackTraceElements[first].getClassName();
            if (!className.startsWith("junit.framework")) {
                break;
            }
        }

        if (first > 0) {
            first--; // retain one assertSomething() line in the trace
        }

        if (first < last) {
            t.setStackTrace(Arrays.copyOfRange(stackTraceElements, first, last));
        }
    }
}
