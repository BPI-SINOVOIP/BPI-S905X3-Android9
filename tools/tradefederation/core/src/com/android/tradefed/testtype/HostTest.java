/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.metrics.proto.MetricMeasurement.Metric;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.JUnit4ResultForwarder;
import com.android.tradefed.result.ResultForwarder;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.testtype.host.PrettyTestEventLogger;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.JUnit4TestFilter;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.SystemUtil.EnvVariable;
import com.android.tradefed.util.TestFilterHelper;

import com.google.common.annotations.VisibleForTesting;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import org.junit.internal.runners.ErrorReportingRunner;
import org.junit.runner.Description;
import org.junit.runner.JUnitCore;
import org.junit.runner.Request;
import org.junit.runner.RunWith;
import org.junit.runner.Runner;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.Suite.SuiteClasses;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Deque;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/**
 * A test runner for JUnit host based tests. If the test to be run implements {@link IDeviceTest}
 * this runner will pass a reference to the device.
 */
@OptionClass(alias = "host")
public class HostTest
        implements IDeviceTest,
                ITestFilterReceiver,
                ITestAnnotationFilterReceiver,
                IRemoteTest,
                ITestCollector,
                IBuildReceiver,
                IAbiReceiver,
                IShardableTest,
                IStrictShardableTest,
                IRuntimeHintProvider,
                IMultiDeviceTest,
                IInvocationContextReceiver {


    @Option(name = "class", description = "The JUnit test classes to run, in the format "
            + "<package>.<class>. eg. \"com.android.foo.Bar\". This field can be repeated.",
            importance = Importance.IF_UNSET)
    private Set<String> mClasses = new LinkedHashSet<>();

    @Option(name = "method", description = "The name of the method in the JUnit TestCase to run. "
            + "eg. \"testFooBar\"",
            importance = Importance.IF_UNSET)
    private String mMethodName;

    @Option(
        name = "jar",
        description = "The jars containing the JUnit test class to run.",
        importance = Importance.IF_UNSET
    )
    private Set<String> mJars = new HashSet<>();

    public static final String SET_OPTION_NAME = "set-option";
    public static final String SET_OPTION_DESC =
            "Options to be passed down to the class under test, key and value should be "
                    + "separated by colon \":\"; for example, if class under test supports "
                    + "\"--iteration 1\" from a command line, it should be passed in as"
                    + " \"--set-option iteration:1\" or \"--set-option iteration:key=value\" for "
                    + "passing options to map; escaping of \":\" \"=\" is currently not supported";

    @Option(name = SET_OPTION_NAME, description = SET_OPTION_DESC)
    private List<String> mKeyValueOptions = new ArrayList<>();

    @Option(name = "include-annotation",
            description = "The set of annotations a test must have to be run.")
    private Set<String> mIncludeAnnotations = new HashSet<>();

    @Option(name = "exclude-annotation",
            description = "The set of annotations to exclude tests from running. A test must have "
                    + "none of the annotations in this list to run.")
    private Set<String> mExcludeAnnotations = new HashSet<>();

    @Option(name = "collect-tests-only",
            description = "Only invoke the instrumentation to collect list of applicable test "
                    + "cases. All test run callbacks will be triggered, but test execution will "
                    + "not be actually carried out.")
    private boolean mCollectTestsOnly = false;

    @Option(
        name = "runtime-hint",
        isTimeVal = true,
        description = "The hint about the test's runtime."
    )
    private long mRuntimeHint = 60000; // 1 minute

    enum ShardUnit {
        CLASS, METHOD;
    }

    @Option(name = "shard-unit",
            description = "Shard by class or method")
    private ShardUnit mShardUnit = ShardUnit.CLASS;

    @Option(
        name = "enable-pretty-logs",
        description =
                "whether or not to enable a logging for each test start and end on both host and "
                        + "device side."
    )
    private boolean mEnableHostDeviceLogs = true;

    private ITestDevice mDevice;
    private IBuildInfo mBuildInfo;
    private IAbi mAbi;
    private Map<ITestDevice, IBuildInfo> mDeviceInfos;
    private IInvocationContext mContext;
    private TestFilterHelper mFilterHelper;
    private boolean mSkipTestClassCheck = false;

    private List<Object> mTestMethods;
    private int mNumTestCases = -1;

    private static final String EXCLUDE_NO_TEST_FAILURE = "org.junit.runner.manipulation.Filter";
    private static final String TEST_FULL_NAME_FORMAT = "%s#%s";
    private static final String ROOT_DIR = "ROOT_DIR";

    public HostTest() {
        mFilterHelper = new TestFilterHelper(new ArrayList<String>(), new ArrayList<String>(),
                mIncludeAnnotations, mExcludeAnnotations);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /** {@inheritDoc} */
    @Override
    public long getRuntimeHint() {
        return mRuntimeHint;
    }

    /** {@inheritDoc} */
    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    /** {@inheritDoc} */
    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    /**
     * Get the build info received by HostTest.
     *
     * @return the {@link IBuildInfo}
     */
    protected IBuildInfo getBuild() {
        return mBuildInfo;
    }

    @Override
    public void setDeviceInfos(Map<ITestDevice, IBuildInfo> deviceInfos) {
        mDeviceInfos = deviceInfos;
    }

    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mContext = invocationContext;
    }

    /**
     * @return true if shard-unit is method; false otherwise
     */
    private boolean shardUnitIsMethod() {
        return ShardUnit.METHOD.equals(mShardUnit);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addIncludeFilter(String filter) {
        mFilterHelper.addIncludeFilter(filter);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllIncludeFilters(Set<String> filters) {
        mFilterHelper.addAllIncludeFilters(filters);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludeFilter(String filter) {
        mFilterHelper.addExcludeFilter(filter);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllExcludeFilters(Set<String> filters) {
        mFilterHelper.addAllExcludeFilters(filters);
    }

    /**
     * Return the number of test cases across all classes part of the tests
     */
    public int countTestCases() {
        if (mTestMethods != null) {
            return mTestMethods.size();
        } else if (mNumTestCases >= 0) {
            return mNumTestCases;
        }
        // Ensure filters are set in the helper
        mFilterHelper.addAllIncludeAnnotation(mIncludeAnnotations);
        mFilterHelper.addAllExcludeAnnotation(mExcludeAnnotations);

        int count = 0;
        for (Class<?> classObj : getClasses()) {
            if (IRemoteTest.class.isAssignableFrom(classObj)
                    || Test.class.isAssignableFrom(classObj)) {
                TestSuite suite = collectTests(collectClasses(classObj));
                int suiteCount = suite.countTestCases();
                if (suiteCount == 0
                        && IRemoteTest.class.isAssignableFrom(classObj)
                        && !Test.class.isAssignableFrom(classObj)) {
                    // If it's a pure IRemoteTest we count the run() as one test.
                    count++;
                } else {
                    count += suiteCount;
                }
            } else if (hasJUnit4Annotation(classObj)) {
                Request req = Request.aClass(classObj);
                req = req.filterWith(new JUnit4TestFilter(mFilterHelper));
                Runner checkRunner = req.getRunner();
                // If no tests are remaining after filtering, checkRunner is ErrorReportingRunner.
                // testCount() for ErrorReportingRunner returns 1, skip this classObj in this case.
                if (checkRunner instanceof ErrorReportingRunner) {
                    if (!EXCLUDE_NO_TEST_FAILURE.equals(
                            checkRunner.getDescription().getClassName())) {
                        // If after filtering we have remaining tests that are malformed, we still
                        // count them toward the total number of tests. (each malformed class will
                        // count as 1 in the testCount()).
                        count += checkRunner.testCount();
                    }
                } else {
                    count += checkRunner.testCount();
                }
            } else {
                count++;
            }
        }
        return mNumTestCases = count;
    }

    /**
     * Clear then set a class name to be run.
     */
    protected void setClassName(String className) {
        mClasses.clear();
        mClasses.add(className);
    }

    @VisibleForTesting
    public Set<String> getClassNames() {
        return mClasses;
    }

    void setMethodName(String methodName) {
        mMethodName = methodName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addIncludeAnnotation(String annotation) {
        mIncludeAnnotations.add(annotation);
        mFilterHelper.addIncludeAnnotation(annotation);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllIncludeAnnotation(Set<String> annotations) {
        mIncludeAnnotations.addAll(annotations);
        mFilterHelper.addAllIncludeAnnotation(annotations);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addExcludeAnnotation(String notAnnotation) {
        mExcludeAnnotations.add(notAnnotation);
        mFilterHelper.addExcludeAnnotation(notAnnotation);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void addAllExcludeAnnotation(Set<String> notAnnotations) {
        mExcludeAnnotations.addAll(notAnnotations);
        mFilterHelper.addAllExcludeAnnotation(notAnnotations);
    }

    /**
     * Helper to set the information of an object based on some of its type.
     */
    private void setTestObjectInformation(Object testObj) {
        if (testObj instanceof IBuildReceiver) {
            if (mBuildInfo == null) {
                throw new IllegalArgumentException("Missing build information");
            }
            ((IBuildReceiver)testObj).setBuild(mBuildInfo);
        }
        if (testObj instanceof IDeviceTest) {
            if (mDevice == null) {
                throw new IllegalArgumentException("Missing device");
            }
            ((IDeviceTest)testObj).setDevice(mDevice);
        }
        // We are more flexible about abi info since not always available.
        if (testObj instanceof IAbiReceiver) {
            ((IAbiReceiver)testObj).setAbi(mAbi);
        }
        if (testObj instanceof IMultiDeviceTest) {
            ((IMultiDeviceTest) testObj).setDeviceInfos(mDeviceInfos);
        }
        if (testObj instanceof IInvocationContextReceiver) {
            ((IInvocationContextReceiver) testObj).setInvocationContext(mContext);
        }
        // managed runner should have the same set-option to pass option too.
        if (testObj instanceof ISetOptionReceiver) {
            try {
                OptionSetter setter = new OptionSetter(testObj);
                for (String item : mKeyValueOptions) {
                    setter.setOptionValue(SET_OPTION_NAME, item);
                }
            } catch (ConfigurationException e) {
                throw new RuntimeException(e);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        // Ensure filters are set in the helper
        mFilterHelper.addAllIncludeAnnotation(mIncludeAnnotations);
        mFilterHelper.addAllExcludeAnnotation(mExcludeAnnotations);

        List<Class<?>> classes = getClasses();
        if (!mSkipTestClassCheck) {
            if (classes.isEmpty()) {
                throw new IllegalArgumentException("Missing Test class name");
            }
        }
        if (mMethodName != null && classes.size() > 1) {
            throw new IllegalArgumentException("Method name given with multiple test classes");
        }
        // Add a pretty logger to the events to mark clearly start/end of test cases.
        if (mEnableHostDeviceLogs) {
            PrettyTestEventLogger logger = new PrettyTestEventLogger(mContext.getDevices());
            listener = new ResultForwarder(logger, listener);
        }
        if (mTestMethods != null) {
            runTestCases(listener);
        } else {
            runTestClasses(listener);
        }
    }

    private void runTestClasses(ITestInvocationListener listener)
            throws DeviceNotAvailableException {
        for (Class<?> classObj : getClasses()) {
            if (IRemoteTest.class.isAssignableFrom(classObj)) {
                IRemoteTest test = (IRemoteTest) loadObject(classObj);
                applyFilters(classObj, test);
                runRemoteTest(listener, test);
            } else if (Test.class.isAssignableFrom(classObj)) {
                TestSuite junitTest = collectTests(collectClasses(classObj));
                runJUnit3Tests(listener, junitTest, classObj.getName());
            } else if (hasJUnit4Annotation(classObj)) {
                // Include the method name filtering
                Set<String> includes = mFilterHelper.getIncludeFilters();
                if (mMethodName != null) {
                    includes.add(String.format(TEST_FULL_NAME_FORMAT, classObj.getName(),
                            mMethodName));
                }

                // Running in a full JUnit4 manner, no downgrade to JUnit3 {@link Test}
                Request req = Request.aClass(classObj);
                req = req.filterWith(new JUnit4TestFilter(mFilterHelper));
                Runner checkRunner = req.getRunner();
                runJUnit4Tests(listener, checkRunner, classObj.getName());
            } else {
                throw new IllegalArgumentException(
                        String.format("%s is not a supported test", classObj.getName()));
            }
        }
    }

    private void runTestCases(ITestInvocationListener listener) throws DeviceNotAvailableException {
        for (Object obj : getTestMethods()) {
            if (IRemoteTest.class.isInstance(obj)) {
                IRemoteTest test = (IRemoteTest) obj;
                runRemoteTest(listener, test);
            } else if (TestSuite.class.isInstance(obj)) {
                TestSuite junitTest = (TestSuite) obj;
                runJUnit3Tests(listener, junitTest, junitTest.getName());
            } else if (Description.class.isInstance(obj)) {
                // Running in a full JUnit4 manner, no downgrade to JUnit3 {@link Test}
                Description desc = (Description) obj;
                Request req = Request.aClass(desc.getTestClass());
                Runner checkRunner = req.filterWith(desc).getRunner();
                runJUnit4Tests(listener, checkRunner, desc.getClassName());
            } else {
                throw new IllegalArgumentException(
                        String.format("%s is not a supported test", obj));
            }
        }
    }

    private void runRemoteTest(ITestInvocationListener listener, IRemoteTest test)
            throws DeviceNotAvailableException {
        if (mCollectTestsOnly) {
            // Collect only mode is propagated to the test.
            if (test instanceof ITestCollector) {
                ((ITestCollector) test).setCollectTestsOnly(true);
            } else {
                throw new IllegalArgumentException(
                        String.format(
                                "%s does not implement ITestCollector", test.getClass()));
            }
        }
        test.run(listener);
    }

    private void runJUnit3Tests(
            ITestInvocationListener listener, TestSuite junitTest, String className)
            throws DeviceNotAvailableException {
        if (mCollectTestsOnly) {
            // Collect only mode, fake the junit test execution.
            listener.testRunStarted(className, junitTest.countTestCases());
            HashMap<String, Metric> empty = new HashMap<>();
            for (int i = 0; i < junitTest.countTestCases(); i++) {
                Test t = junitTest.testAt(i);
                // Test does not have a getName method.
                // using the toString format instead: <testName>(className)
                String testName = t.toString().split("\\(")[0];
                TestDescription testId = new TestDescription(t.getClass().getName(), testName);
                listener.testStarted(testId);
                listener.testEnded(testId, empty);
            }
            HashMap<String, Metric> emptyMap = new HashMap<>();
            listener.testRunEnded(0, emptyMap);
        } else {
            JUnitRunUtil.runTest(listener, junitTest, className);
        }
    }

    private void runJUnit4Tests(
            ITestInvocationListener listener, Runner checkRunner, String className) {
        JUnitCore runnerCore = new JUnitCore();
        JUnit4ResultForwarder list = new JUnit4ResultForwarder(listener);
        runnerCore.addListener(list);

        // If no tests are remaining after filtering, it returns an Error Runner.
        if (!(checkRunner instanceof ErrorReportingRunner)) {
            long startTime = System.currentTimeMillis();
            listener.testRunStarted(className, checkRunner.testCount());
            if (mCollectTestsOnly) {
                fakeDescriptionExecution(checkRunner.getDescription(), list);
            } else {
                setTestObjectInformation(checkRunner);
                runnerCore.run(checkRunner);
            }
            listener.testRunEnded(
                    System.currentTimeMillis() - startTime, new HashMap<String, Metric>());
        } else {
            // Special case where filtering leaves no tests to run, we report no failure
            // in this case.
            if (EXCLUDE_NO_TEST_FAILURE.equals(
                    checkRunner.getDescription().getClassName())) {
                listener.testRunStarted(className, 0);
                listener.testRunEnded(0, new HashMap<String, Metric>());
            } else {
                // Run the Error runner to get the failures from test classes.
                listener.testRunStarted(className, checkRunner.testCount());
                RunNotifier failureNotifier = new RunNotifier();
                failureNotifier.addListener(list);
                checkRunner.run(failureNotifier);
                listener.testRunEnded(0, new HashMap<String, Metric>());
            }
        }
    }

    /**
     * Helper to fake the execution of JUnit4 Tests, using the {@link Description}
     */
    private void fakeDescriptionExecution(Description desc, JUnit4ResultForwarder listener) {
        if (desc.getMethodName() == null || !desc.getChildren().isEmpty()) {
            for (Description child : desc.getChildren()) {
                fakeDescriptionExecution(child, listener);
            }
        } else {
            listener.testStarted(desc);
            listener.testFinished(desc);
        }
    }

    private Set<Class<?>> collectClasses(Class<?> classObj) {
        Set<Class<?>> classes = new HashSet<>();
        if (TestSuite.class.isAssignableFrom(classObj)) {
            TestSuite testObj = (TestSuite) loadObject(classObj);
            classes.addAll(getClassesFromSuite(testObj));
        } else {
            classes.add(classObj);
        }
        return classes;
    }

    private Set<Class<?>> getClassesFromSuite(TestSuite suite) {
        Set<Class<?>> classes = new HashSet<>();
        Enumeration<Test> tests = suite.tests();
        while (tests.hasMoreElements()) {
            Test test = tests.nextElement();
            if (test instanceof TestSuite) {
                classes.addAll(getClassesFromSuite((TestSuite) test));
            } else {
                classes.addAll(collectClasses(test.getClass()));
            }
        }
        return classes;
    }

    private TestSuite collectTests(Set<Class<?>> classes) {
        TestSuite suite = new TestSuite();
        for (Class<?> classObj : classes) {
            String packageName = classObj.getPackage().getName();
            String className = classObj.getName();
            Method[] methods = null;
            if (mMethodName == null) {
                methods = classObj.getMethods();
            } else {
                try {
                    methods = new Method[] {
                            classObj.getMethod(mMethodName, (Class[]) null)
                    };
                } catch (NoSuchMethodException e) {
                    throw new IllegalArgumentException(
                            String.format("Cannot find %s#%s", className, mMethodName), e);
                }
            }

            for (Method method : methods) {
                if (!Modifier.isPublic(method.getModifiers())
                        || !method.getReturnType().equals(Void.TYPE)
                        || method.getParameterTypes().length > 0
                        || !method.getName().startsWith("test")
                        || !mFilterHelper.shouldRun(packageName, classObj, method)) {
                    continue;
                }
                Test testObj = (Test) loadObject(classObj, false);
                if (testObj instanceof TestCase) {
                    ((TestCase)testObj).setName(method.getName());
                }

                suite.addTest(testObj);
            }
        }
        return suite;
    }

    private List<Object> getTestMethods() throws IllegalArgumentException  {
        if (mTestMethods != null) {
            return mTestMethods;
        }
        mTestMethods = new ArrayList<>();
        mFilterHelper.addAllIncludeAnnotation(mIncludeAnnotations);
        mFilterHelper.addAllExcludeAnnotation(mExcludeAnnotations);
        List<Class<?>> classes = getClasses();
        for (Class<?> classObj : classes) {
            if (Test.class.isAssignableFrom(classObj)) {
                TestSuite suite = collectTests(collectClasses(classObj));
                for (int i = 0; i < suite.testCount(); i++) {
                    TestSuite singletonSuite = new TestSuite();
                    singletonSuite.setName(classObj.getName());
                    Test testObj = suite.testAt(i);
                    singletonSuite.addTest(testObj);
                    if (IRemoteTest.class.isInstance(testObj)) {
                        setTestObjectInformation(testObj);
                    }
                    mTestMethods.add(singletonSuite);
                }
            } else if (IRemoteTest.class.isAssignableFrom(classObj)) {
                // a pure IRemoteTest is considered a test method itself
                IRemoteTest test = (IRemoteTest) loadObject(classObj);
                applyFilters(classObj, test);
                mTestMethods.add(test);
            } else if (hasJUnit4Annotation(classObj)) {
                // Running in a full JUnit4 manner, no downgrade to JUnit3 {@link Test}
                Request req = Request.aClass(classObj);
                // Include the method name filtering
                Set<String> includes = mFilterHelper.getIncludeFilters();
                if (mMethodName != null) {
                    includes.add(String.format(TEST_FULL_NAME_FORMAT, classObj.getName(),
                            mMethodName));
                }

                req = req.filterWith(new JUnit4TestFilter(mFilterHelper));
                Runner checkRunner = req.getRunner();
                Deque<Description> descriptions = new ArrayDeque<>();
                descriptions.push(checkRunner.getDescription());
                while (!descriptions.isEmpty()) {
                    Description desc = descriptions.pop();
                    if (desc.isTest()) {
                        mTestMethods.add(desc);
                    }
                    List<Description> children = desc.getChildren();
                    Collections.reverse(children);
                    for (Description child : children) {
                        descriptions.push(child);
                    }
                }
            } else {
                throw new IllegalArgumentException(
                        String.format("%s is not a supported test", classObj.getName()));
            }
        }
        return mTestMethods;
    }

    protected final List<Class<?>> getClasses() throws IllegalArgumentException {
        List<Class<?>> classes = new ArrayList<>();
        for (String className : mClasses) {
            try {
                classes.add(Class.forName(className, true, getClassLoader()));
            } catch (ClassNotFoundException e) {
                throw new IllegalArgumentException(String.format("Could not load Test class %s",
                        className), e);
            }
        }
        // Inspect for the jar files
        for (String jarName : mJars) {
            JarFile jarFile = null;
            try {
                File file = getJarFile(jarName, getBuild());
                jarFile = new JarFile(file);
                Enumeration<JarEntry> e = jarFile.entries();
                URL[] urls = {new URL(String.format("jar:file:%s!/", file.getAbsolutePath()))};
                URLClassLoader cl = URLClassLoader.newInstance(urls);

                while (e.hasMoreElements()) {
                    JarEntry je = e.nextElement();
                    if (je.isDirectory()
                            || !je.getName().endsWith(".class")
                            || je.getName().contains("$")) {
                        continue;
                    }
                    String className = getClassName(je.getName());
                    try {
                        Class<?> cls = cl.loadClass(className);
                        int modifiers = cls.getModifiers();
                        if ((IRemoteTest.class.isAssignableFrom(cls)
                                        || Test.class.isAssignableFrom(cls)
                                        || hasJUnit4Annotation(cls))
                                && !Modifier.isStatic(modifiers)
                                && !Modifier.isPrivate(modifiers)
                                && !Modifier.isProtected(modifiers)
                                && !Modifier.isInterface(modifiers)
                                && !Modifier.isAbstract(modifiers)) {
                            classes.add(cls);
                        }
                    } catch (ClassNotFoundException cnfe) {
                        throw new IllegalArgumentException(
                                String.format("Cannot find test class %s", className));
                    } catch (IllegalAccessError | NoClassDefFoundError err) {
                        // IllegalAccessError can happen when the class or one of its super
                        // class/interfaces are package-private. We can't load such class from
                        // here (= outside of the package). Since our intention is not to load
                        // all classes in the jar, but to find our the main test classes, this
                        // can be safely skipped.
                        // NoClassDefFoundErrror is also okay because certain CTS test cases
                        // might statically link to a jar library (e.g. tools.jar from JDK)
                        // where certain internal classes in the library are referencing
                        // classes that are not available in the jar. Again, since our goal here
                        // is to find test classes, this can be safely skipped.
                        continue;
                    }
                }
            } catch (IOException e) {
                CLog.e(e);
                throw new IllegalArgumentException(e);
            } finally {
                StreamUtil.close(jarFile);
            }
        }
        return classes;
    }

    /** Returns the default classloader. */
    @VisibleForTesting
    protected ClassLoader getClassLoader() {
        return this.getClass().getClassLoader();
    }

    /** load the class object and set the test info (device, build). */
    protected Object loadObject(Class<?> classObj) {
        return loadObject(classObj, true);
    }

    /**
     * Load the class object and set the test info if requested.
     *
     * @param classObj the class object to be loaded.
     * @param setInfo True the test infos need to be set.
     * @return The loaded object from the class.
     */
    private Object loadObject(Class<?> classObj, boolean setInfo) throws IllegalArgumentException {
        final String className = classObj.getName();
        try {
            Object testObj = classObj.newInstance();
            // set options
            setOptionToLoadedObject(testObj, mKeyValueOptions);
            // Set the test information if needed.
            if (setInfo) {
                setTestObjectInformation(testObj);
            }
            return testObj;
        } catch (InstantiationException e) {
            throw new IllegalArgumentException(String.format("Could not load Test class %s",
                    className), e);
        } catch (IllegalAccessException e) {
            throw new IllegalArgumentException(String.format("Could not load Test class %s",
                    className), e);
        }
    }

    /**
     * Helper for Device Runners to use to set options the same way as HostTest, from set-option.
     *
     * @param testObj the object that will receive the options.
     * @param keyValueOptions the list of options formatted as HostTest set-option requires.
     */
    public static void setOptionToLoadedObject(Object testObj, List<String> keyValueOptions) {
        if (!keyValueOptions.isEmpty()) {
            try {
                OptionSetter setter = new OptionSetter(testObj);
                for (String item : keyValueOptions) {
                    String[] fields = item.split(":");
                    if (fields.length == 2) {
                        if (fields[1].contains("=")) {
                            String[] values = fields[1].split("=");
                            if (values.length != 2) {
                                throw new RuntimeException(
                                        String.format(
                                                "set-option provided '%s' format is invalid. Only one "
                                                        + "'=' is allowed",
                                                item));
                            }
                            setter.setOptionValue(fields[0], values[0], values[1]);
                        } else {
                            setter.setOptionValue(fields[0], fields[1]);
                        }
                    } else {
                        throw new RuntimeException(
                                String.format("invalid option spec \"%s\"", item));
                    }
                }
            } catch (ConfigurationException ce) {
                throw new RuntimeException("error passing options down to test class", ce);
            }
        }
    }

    /**
     * Check if an elements that has annotation pass the filter. Exposed for unit testing.
     * @param annotatedElement
     * @return false if the test should not run.
     */
    protected boolean shouldTestRun(AnnotatedElement annotatedElement) {
        return mFilterHelper.shouldTestRun(annotatedElement);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCollectTestsOnly(boolean shouldCollectTest) {
        mCollectTestsOnly = shouldCollectTest;
    }

    /**
     * Helper to determine if we are dealing with a Test class with Junit4 annotations.
     */
    protected boolean hasJUnit4Annotation(Class<?> classObj) {
        if (classObj.isAnnotationPresent(SuiteClasses.class)) {
            return true;
        }
        if (classObj.isAnnotationPresent(RunWith.class)) {
            return true;
        }
        for (Method m : classObj.getMethods()) {
            if (m.isAnnotationPresent(org.junit.Test.class)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Helper method to apply all the filters to an IRemoteTest.
     */
    private void applyFilters(Class<?> classObj, IRemoteTest test) {
        Set<String> includes = mFilterHelper.getIncludeFilters();
        if (mMethodName != null) {
            includes.add(String.format(TEST_FULL_NAME_FORMAT, classObj.getName(), mMethodName));
        }
        Set<String> excludes = mFilterHelper.getExcludeFilters();
        if (test instanceof ITestFilterReceiver) {
            ((ITestFilterReceiver) test).addAllIncludeFilters(includes);
            ((ITestFilterReceiver) test).addAllExcludeFilters(excludes);
        } else if (!includes.isEmpty() || !excludes.isEmpty()) {
            throw new IllegalArgumentException(String.format(
                    "%s does not implement ITestFilterReceiver", classObj.getName()));
        }
        if (test instanceof ITestAnnotationFilterReceiver) {
            ((ITestAnnotationFilterReceiver) test).addAllIncludeAnnotation(
                    mIncludeAnnotations);
            ((ITestAnnotationFilterReceiver) test).addAllExcludeAnnotation(
                    mExcludeAnnotations);
        }
    }

    /**
     * We split by individual by either test class or method.
     */
    @Override
    public Collection<IRemoteTest> split(int shardCount) {
        if (shardCount < 1) {
            throw new IllegalArgumentException("Must have at least 1 shard");
        }
        List<IRemoteTest> listTests = new ArrayList<>();
        List<Class<?>> classes = getClasses();
        if (classes.isEmpty()) {
            throw new IllegalArgumentException("Missing Test class name");
        }
        if (mMethodName != null && classes.size() > 1) {
            throw new IllegalArgumentException("Method name given with multiple test classes");
        }
        List<? extends Object> testObjects;
        if (shardUnitIsMethod()) {
            testObjects = getTestMethods();
        } else {
            testObjects = classes;
            // ignore shardCount when shard unit is class;
            // simply shard by the number of classes
            shardCount = testObjects.size();
        }
        if (testObjects.size() == 1) {
            return null;
        }
        int i = 0;
        int numTotalTestCases = countTestCases();
        for (Object testObj : testObjects) {
            Class<?> classObj = Class.class.isInstance(testObj) ? (Class<?>)testObj : null;
            HostTest test;
            if (i >= listTests.size()) {
                test = createHostTest(classObj);
                test.mRuntimeHint = 0;
                // Carry over non-annotation filters to shards.
                test.addAllExcludeFilters(mFilterHelper.getExcludeFilters());
                test.addAllIncludeFilters(mFilterHelper.getIncludeFilters());
                listTests.add(test);
            }
            test = (HostTest) listTests.get(i);
            Collection<? extends Object> subTests;
            if (classObj != null) {
                test.addClassName(classObj.getName());
                subTests = test.mClasses;
            } else {
                test.addTestMethod(testObj);
                subTests = test.mTestMethods;
            }
            if (numTotalTestCases == 0) {
                // In case there is no tests left
                test.mRuntimeHint = 0L;
            } else {
                test.mRuntimeHint = mRuntimeHint * subTests.size() / numTotalTestCases;
            }
            i = (i + 1) % shardCount;
        }

        return listTests;
    }

    private void addTestMethod(Object testObject) {
        if (mTestMethods == null) {
            mTestMethods = new ArrayList<>();
            mClasses.clear();
        }
        mTestMethods.add(testObject);
        if (IRemoteTest.class.isInstance(testObject)) {
            addClassName(testObject.getClass().getName());
        } else if (TestSuite.class.isInstance(testObject)) {
            addClassName(((TestSuite)testObject).getName());
        } else if (Description.class.isInstance(testObject)) {
            addClassName(((Description)testObject).getTestClass().getName());
        }
    }

    /**
     * Add a class to be ran by HostTest.
     */
    private void addClassName(String className) {
        mClasses.add(className);
    }

    /**
     * Helper to create a HostTest instance when sharding. Override to return any child from
     * HostTest.
     */
    protected HostTest createHostTest(Class<?> classObj) {
        HostTest test;
        try {
            test = this.getClass().newInstance();
        } catch (InstantiationException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
        OptionCopier.copyOptionsNoThrow(this, test);
        if (classObj != null) {
            test.setClassName(classObj.getName());
        }
        // clean the jar option since we are loading directly from classes after.
        test.mJars = new HashSet<>();
        // Copy the abi if available
        test.setAbi(mAbi);
        return test;
    }

    @Override
    public IRemoteTest getTestShard(int shardCount, int shardIndex) {
        List<Class<?>> classes = getClasses();
        if (classes.isEmpty()) {
            throw new IllegalArgumentException("Missing Test class name");
        }
        if (mMethodName != null && classes.size() > 1) {
            throw new IllegalArgumentException("Method name given with multiple test classes");
        }
        HostTest test = createTestShard(shardCount, shardIndex);
        // In case we don't have enough classes to shard, we return a Stub.
        if (test == null) {
            test = createHostTest(null);
            test.mSkipTestClassCheck = true;
            test.mClasses.clear();
            test.mRuntimeHint = 0l;
        } else {
            int newCount = test.countTestCases();
            int numTotalTestCases = countTestCases();
            // In case of counting inconsistency we raise the issue. Should not happen if we are
            // counting properly. Here as a security.
            if (newCount > numTotalTestCases) {
                throw new RuntimeException(
                        "Tests count number after sharding is higher than initial count.");
            }
            // update the runtime hint on pro-rate of number of tests.
            if (newCount == 0) {
                // In case there is not tests left.
                test.mRuntimeHint = 0L;
            } else {
                test.mRuntimeHint = (mRuntimeHint * newCount) / numTotalTestCases;
            }
        }
        return test;
    }

    private HostTest createTestShard(int shardCount, int shardIndex) {
        int i = 0;
        HostTest test = null;
        List<? extends Object> tests = shardUnitIsMethod() ? getTestMethods() : getClasses();
        for (Object testObj : tests) {
            Class<?> classObj = Class.class.isInstance(testObj) ? (Class<?>)testObj : null;
            if (i % shardCount == shardIndex) {
                if (test == null) {
                    test = createHostTest(classObj);
                }
                if (classObj != null) {
                    test.addClassName(classObj.getName());
                } else {
                    test.addTestMethod(testObj);
                }
                // Carry over non-annotation filters to shards.
                test.addAllExcludeFilters(mFilterHelper.getExcludeFilters());
                test.addAllIncludeFilters(mFilterHelper.getIncludeFilters());
            }
            i++;
        }
        return test;
    }

    private String getClassName(String name) {
        // -6 because of .class
        return name.substring(0, name.length() - 6).replace('/', '.');
    }

    /**
     * Inspect several location where the artifact are usually located for different use cases to
     * find our jar.
     */
    @VisibleForTesting
    protected File getJarFile(String jarName, IBuildInfo buildInfo) throws FileNotFoundException {
        File jarFile = null;
        // Check env variable
        String testcasesPath = System.getenv(EnvVariable.ANDROID_HOST_OUT_TESTCASES.toString());
        if (testcasesPath != null) {
            File testCasesFile = new File(testcasesPath);
            jarFile = searchJarFile(testCasesFile, jarName);
        }
        if (jarFile != null) {
            return jarFile;
        }

        // Check tests dir
        if (buildInfo instanceof IDeviceBuildInfo) {
            IDeviceBuildInfo deviceBuildInfo = (IDeviceBuildInfo) buildInfo;
            File testDir = deviceBuildInfo.getTestsDir();
            jarFile = searchJarFile(testDir, jarName);
        }
        if (jarFile != null) {
            return jarFile;
        }

        // Check ROOT_DIR
        if (buildInfo.getBuildAttributes().get(ROOT_DIR) != null) {
            jarFile =
                    searchJarFile(new File(buildInfo.getBuildAttributes().get(ROOT_DIR)), jarName);
        }
        if (jarFile != null) {
            return jarFile;
        }
        throw new FileNotFoundException(String.format("Could not find jar: %s", jarName));
    }

    private File searchJarFile(File baseSearchFile, String jarName) {
        if (baseSearchFile != null && baseSearchFile.isDirectory()) {
            File jarFile = FileUtil.findFile(baseSearchFile, jarName);
            if (jarFile != null && jarFile.isFile()) {
                return jarFile;
            }
        }
        return null;
    }
}
