/*
 * Copyright (C) 2009 The Android Open Source Project
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

import com.google.common.annotations.VisibleForTesting;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import javax.annotation.Nullable;
import vogar.Result;
import vogar.RunnerType;
import vogar.TestProperties;
import vogar.monitor.TargetMonitor;
import vogar.target.junit.JUnitRunnerFactory;

/**
 * Runs an action, in process on the target.
 */
public final class TestRunner {

    private final String qualifiedClassOrPackageName;

    /** the monitor port if a monitor is expected, or null for no monitor */
    @VisibleForTesting final Integer monitorPort;

    /** use an atomic reference so the runner can null it out when it is encountered. */
    private final AtomicReference<String> skipPastReference;
    private final int timeoutSeconds;

    private final RunnerFactory runnerFactory;
    private final String[] args;
    private boolean useSocketMonitor;

    public TestRunner(Properties properties, List<String> argsList) {
        qualifiedClassOrPackageName = properties.getProperty(TestProperties.TEST_CLASS_OR_PACKAGE);
        timeoutSeconds = Integer.parseInt(properties.getProperty(TestProperties.TIMEOUT));

        int monitorPort = Integer.parseInt(properties.getProperty(TestProperties.MONITOR_PORT));
        String skipPast = null;

        for (Iterator<String> i = argsList.iterator(); i.hasNext(); ) {
            String arg = i.next();
            if (arg.equals("--monitorPort")) {
                i.remove();
                monitorPort = Integer.parseInt(i.next());
                i.remove();
            }
            if (arg.equals("--skipPast")) {
                i.remove();
                skipPast = i.next();
                i.remove();
            }
        }

        // Select the RunnerFactory instances to use based on the selected runner type.
        RunnerType runnerType =
                RunnerType.valueOf(properties.getProperty(TestProperties.RUNNER_TYPE));
        List<RunnerFactory> runnerFactories = new ArrayList<>();
        if (runnerType.supportsCaliper()) {
            runnerFactories.add(new CaliperRunnerFactory(argsList));
        }
        if (runnerType.supportsJUnit()) {
            runnerFactories.add(new JUnitRunnerFactory());
        }
        if (runnerType.supportsMain()) {
            runnerFactories.add(new MainRunnerFactory());
        }
        runnerFactory = new CompositeRunnerFactory(runnerFactories);

        this.monitorPort = monitorPort;
        this.skipPastReference = new AtomicReference<>(skipPast);
        this.args = argsList.toArray(new String[argsList.size()]);
    }

    /**
     * Load the properties that were either encapsulated in the APK (if using
     * {@link vogar.android.ActivityMode}), or encapsulated in the JAR compiled by Vogar (in other
     * modes).
     *
     * @return The {@link Properties} that were loaded.
     */
    public static Properties loadProperties() {
        try {
            InputStream in = getPropertiesStream();
            Properties properties = new Properties();
            properties.load(in);
            in.close();
            return properties;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Configure this test runner to await an incoming socket connection when
     * writing test results. Otherwise all communication happens over
     * System.out.
     */
    public void useSocketMonitor() {
        this.useSocketMonitor = true;
    }

    /**
     * Attempt to load the test properties file from both the application and system classloader.
     * This is necessary because sometimes we run tests from the boot classpath.
     */
    private static InputStream getPropertiesStream() throws IOException {
        for (Class<?> classToLoadFrom : new Class<?>[] { TestRunner.class, Object.class }) {
            InputStream propertiesStream = classToLoadFrom.getResourceAsStream(
                    "/" + TestProperties.FILE);
            if (propertiesStream != null) {
                return propertiesStream;
            }
        }
        throw new IOException(TestProperties.FILE + " missing!");
    }

    public void run() throws IOException {
        final TargetMonitor monitor = useSocketMonitor
                ? TargetMonitor.await(monitorPort)
                : TargetMonitor.forPrintStream(System.out);

        PrintStream monitorPrintStream = new PrintStreamDecorator(System.out) {
            @Override public void print(String str) {
                monitor.output(str != null ? str : "null");
            }
        };
        System.setOut(monitorPrintStream);
        System.setErr(monitorPrintStream);

        try {
            run(monitor);
        } catch (Throwable internalError) {
            internalError.printStackTrace(monitorPrintStream);
        } finally {
            monitor.close();
        }
    }

    private void run(final TargetMonitor monitor) {
        TestEnvironment testEnvironment = new TestEnvironment();
        testEnvironment.reset();

        String classOrPackageName;
        String qualification;

        // Check whether the class or package is qualified and, if so, strip it off and pass it
        // separately to the runners. For instance, may qualify a junit class by appending
        // #method_name, where method_name is the name of a single test of the class to run.
        int hash_position = qualifiedClassOrPackageName.indexOf("#");
        if (hash_position != -1) {
            classOrPackageName = qualifiedClassOrPackageName.substring(0, hash_position);
            qualification = qualifiedClassOrPackageName.substring(hash_position + 1);
        } else {
            classOrPackageName = qualifiedClassOrPackageName;
            qualification = null;
        }

        Set<Class<?>> classes = new ClassFinder().find(classOrPackageName);

        // if there is more than one class in the set, this must be a package. Since we're
        // running everything in the package already, remove any class called AllTests.
        if (classes.size() > 1) {
            Set<Class<?>> toRemove = new HashSet<>();
            for (Class<?> klass : classes) {
                if (klass.getName().endsWith(".AllTests")) {
                    toRemove.add(klass);
                }
            }
            classes.removeAll(toRemove);
        }


        for (Class<?> klass : classes) {
            TargetRunner targetRunner;
            try {
                targetRunner = runnerFactory.newRunner(monitor, qualification, klass,
                        skipPastReference, testEnvironment, timeoutSeconds, args);
            } catch (RuntimeException e) {
                monitor.outcomeStarted(klass.getName());
                e.printStackTrace();
                monitor.outcomeFinished(Result.ERROR);
                return;
            }

            if (targetRunner == null) {
                monitor.outcomeStarted(klass.getName());
                System.out.println("Skipping " + klass.getName()
                        + ": no associated runner class");
                monitor.outcomeFinished(Result.UNSUPPORTED);
                continue;
            }

            boolean completedNormally = targetRunner.run();
            if (!completedNormally) {
                return; // let the caller start another process
            }
        }

        monitor.completedNormally(true);
    }

    public static void main(String[] args) throws IOException {
        new TestRunner(loadProperties(), new ArrayList<>(Arrays.asList(args))).run();
        System.exit(0);
    }

    /**
     * A {@link RunnerFactory} that will traverse a list of {@link RunnerFactory} instances to find
     * one that can be used to run the code.
     */
    private static class CompositeRunnerFactory implements RunnerFactory {

        private final List<? extends RunnerFactory> runnerFactories;

        private CompositeRunnerFactory(List<RunnerFactory> factories) {
            this.runnerFactories = factories;
        }

        @Override @Nullable
        public TargetRunner newRunner(TargetMonitor monitor, String qualification,
                Class<?> klass, AtomicReference<String> skipPastReference,
                TestEnvironment testEnvironment, int timeoutSeconds, String[] args) {
            for (RunnerFactory runnerFactory : runnerFactories) {
                TargetRunner targetRunner = runnerFactory.newRunner(monitor, qualification, klass,
                        skipPastReference, testEnvironment, timeoutSeconds, args);
                if (targetRunner != null) {
                    return targetRunner;
                }
            }

            return null;
        }
    }
}
