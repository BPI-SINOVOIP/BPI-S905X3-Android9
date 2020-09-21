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
package android.device.collectors;

import android.device.collectors.annotations.MetricOption;
import android.device.collectors.annotations.OptionClass;
import android.device.collectors.util.SendToInstrumentation;
import android.os.Bundle;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.support.annotation.VisibleForTesting;
import android.support.test.InstrumentationRegistry;
import android.support.test.internal.runner.listener.InstrumentationRunListener;
import android.util.Log;

import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Base implementation of a device metric listener that will capture and output metrics for each
 * test run or test cases. Collectors will have access to {@link DataRecord} objects where they
 * can put results and the base class ensure these results will be send to the instrumentation.
 *
 * Any subclass that calls {@link #createAndEmptyDirectory(String)} needs external storage
 * permission. So to use this class at runtime, your test need to
 * <a href="{@docRoot}training/basics/data-storage/files.html#GetWritePermission">have storage
 * permission enabled</a>, and preferably granted at install time (to avoid interrupting the test).
 * For testing at desk, run adb install -r -g testpackage.apk
 * "-g" grants all required permission at install time.
 *
 * Filtering:
 * You can annotate any test method (@Test) with {@link MetricOption} and specify an arbitrary
 * group name that the test will be part of. It is possible to trigger the collection only against
 * test part of a group using '--include-filter-group [group name]' or to exclude a particular
 * group using '--exclude-filter-group [group name]'.
 * Several group name can be passed using a comma separated argument.
 *
 */
public class BaseMetricListener extends InstrumentationRunListener {

    public static final int BUFFER_SIZE = 1024;

    /** Options keys that the collector can receive. */
    // Filter groups, comma separated list of group name to be included or excluded
    public static final String INCLUDE_FILTER_GROUP_KEY = "include-filter-group";
    public static final String EXCLUDE_FILTER_GROUP_KEY = "exclude-filter-group";

    private static final String NAMESPACE_SEPARATOR = ":";

    private DataRecord mRunData;
    private DataRecord mTestData;

    private Bundle mArgsBundle = null;
    private final List<String> mIncludeFilters;
    private final List<String> mExcludeFilters;

    public BaseMetricListener() {
        mIncludeFilters = new ArrayList<>();
        mExcludeFilters = new ArrayList<>();
    }

    /**
     * Constructor to simulate receiving the instrumentation arguments. Should not be used except
     * for testing.
     */
    @VisibleForTesting
    BaseMetricListener(Bundle argsBundle) {
        this();
        mArgsBundle = argsBundle;
    }

    @Override
    public final void testRunStarted(Description description) throws Exception {
        parseArguments();
        try {
            mRunData = createDataRecord();
            onTestRunStart(mRunData, description);
        } catch (RuntimeException e) {
            // Prevent exception from reporting events.
            Log.e(getTag(), "Exception during onTestRunStart.", e);
        }
        super.testRunStarted(description);
    }

    @Override
    public final void testRunFinished(Result result) throws Exception {
        try {
            onTestRunEnd(mRunData, result);
        } catch (RuntimeException e) {
            // Prevent exception from reporting events.
            Log.e(getTag(), "Exception during onTestRunEnd.", e);
        }
        super.testRunFinished(result);
    }

    @Override
    public final void testStarted(Description description) throws Exception {
        if (shouldRun(description)) {
            try {
                mTestData = createDataRecord();
                onTestStart(mTestData, description);
            } catch (RuntimeException e) {
                // Prevent exception from reporting events.
                Log.e(getTag(), "Exception during onTestStart.", e);
            }
        }
        super.testStarted(description);
    }

    @Override
    public final void testFailure(Failure failure) throws Exception {
        Description description = failure.getDescription();
        if (shouldRun(description)) {
            try {
                onTestFail(mTestData, description, failure);
            } catch (RuntimeException e) {
                // Prevent exception from reporting events.
                Log.e(getTag(), "Exception during onTestFail.", e);
            }
        }
        super.testFailure(failure);
    }

    @Override
    public final void testFinished(Description description) throws Exception {
        if (shouldRun(description)) {
            try {
                onTestEnd(mTestData, description);
            } catch (RuntimeException e) {
                // Prevent exception from reporting events.
                Log.e(getTag(), "Exception during onTestEnd.", e);
            }
            if (mTestData.hasMetrics()) {
                // Only send the status progress if there are metrics
                SendToInstrumentation.sendBundle(getInstrumentation(),
                        mTestData.createBundleFromMetrics());
            }
        }
        super.testFinished(description);
    }

    @Override
    public void instrumentationRunFinished(
            PrintStream streamResult, Bundle resultBundle, Result junitResults) {
        // Test Run data goes into the INSTRUMENTATION_RESULT
        if (mRunData != null) {
            resultBundle.putAll(mRunData.createBundleFromMetrics());
        }
    }

    /**
     * Create a {@link DataRecord}. Exposed for testing.
     */
    @VisibleForTesting
    DataRecord createDataRecord() {
        return new DataRecord();
    }

    // ---------- Interfaces that can be implemented to take action on each test state.

    /**
     * Called when {@link #testRunStarted(Description)} is called.
     *
     * @param runData structure where metrics can be put.
     * @param description the {@link Description} for the run about to start.
     */
    public void onTestRunStart(DataRecord runData, Description description) {
        // Does nothing
    }

    /**
     * Called when {@link #testRunFinished(Result result)} is called.
     *
     * @param runData structure where metrics can be put.
     * @param result the {@link Result} for the run coming from the runner.
     */
    public void onTestRunEnd(DataRecord runData, Result result) {
        // Does nothing
    }

    /**
     * Called when {@link #testStarted(Description)} is called.
     *
     * @param testData structure where metrics can be put.
     * @param description the {@link Description} for the test case about to start.
     */
    public void onTestStart(DataRecord testData, Description description) {
        // Does nothing
    }

    /**
     * Called when {@link #testFailure(Failure)} is called.
     *
     * @param testData structure where metrics can be put.
     * @param description the {@link Description} for the test case that just failed.
     * @param failure the {@link Failure} describing the failure.
     */
    public void onTestFail(DataRecord testData, Description description, Failure failure) {
        // Does nothing
    }

    /**
     * Called when {@link #testFinished(Description)} is called.
     *
     * @param testData structure where metrics can be put.
     * @param description the {@link Description} of the test coming from the runner.
     */
    public void onTestEnd(DataRecord testData, Description description) {
        // Does nothing
    }

    /**
     * Turn executeShellCommand into a blocking operation.
     *
     * @param command shell command to be executed.
     * @return byte array of execution result
     */
    public byte[] executeCommandBlocking(String command) {
        try (
                InputStream is = new ParcelFileDescriptor.AutoCloseInputStream(
                        getInstrumentation().getUiAutomation().executeShellCommand(command));
                ByteArrayOutputStream out = new ByteArrayOutputStream()
        ) {
            byte[] buf = new byte[BUFFER_SIZE];
            int length;
            while ((length = is.read(buf)) >= 0) {
                out.write(buf, 0, length);
            }
            return out.toByteArray();
        } catch (IOException e) {
            Log.e(getTag(), "Error executing: " + command, e);
            return null;
        }
    }

    /**
     * Create a directory inside external storage, and empty it.
     *
     * @param dir full path to the dir to be created.
     * @return directory file created
     */
    public File createAndEmptyDirectory(String dir) {
        File rootDir = Environment.getExternalStorageDirectory();
        File destDir = new File(rootDir, dir);
        executeCommandBlocking("rm -rf " + destDir.getAbsolutePath());
        if (!destDir.exists() && !destDir.mkdirs()) {
            Log.e(getTag(), "Unable to create dir: " + destDir.getAbsolutePath());
            return null;
        }
        return destDir;
    }

    /**
     * Returns the name of the current class to be used as a logging tag.
     */
    String getTag() {
        return this.getClass().getName();
    }

    /**
     * Returns the bundle containing the instrumentation arguments.
     */
    protected final Bundle getArgsBundle() {
        if (mArgsBundle == null) {
            mArgsBundle = InstrumentationRegistry.getArguments();
        }
        return mArgsBundle;
    }

    private void parseArguments() {
        Bundle args = getArgsBundle();
        // First filter the arguments with the alias
        filterAlias(args);
        // Handle filtering
        String includeGroup = args.getString(INCLUDE_FILTER_GROUP_KEY);
        String excludeGroup = args.getString(EXCLUDE_FILTER_GROUP_KEY);
        if (includeGroup != null) {
            mIncludeFilters.addAll(Arrays.asList(includeGroup.split(",")));
        }
        if (excludeGroup != null) {
            mExcludeFilters.addAll(Arrays.asList(excludeGroup.split(",")));
        }
    }

    /**
     * Filter the alias-ed options from the bundle, each implementation of BaseMetricListener will
     * have its own list of arguments.
     * TODO: Split the filtering logic outside the collector class in a utility/helper.
     */
    private void filterAlias(Bundle bundle) {
        Set<String> keySet = new HashSet<>(bundle.keySet());
        OptionClass optionClass = this.getClass().getAnnotation(OptionClass.class);
        if (optionClass == null) {
            // No @OptionClass was specified, remove all alias-ed options.
            for (String key : keySet) {
                if (key.indexOf(NAMESPACE_SEPARATOR) != -1) {
                    bundle.remove(key);
                }
            }
            return;
        }
        // Alias is a required field so if OptionClass is set, alias is set.
        String alias = optionClass.alias();
        for (String key : keySet) {
            if (key.indexOf(NAMESPACE_SEPARATOR) == -1) {
                continue;
            }
            String optionAlias = key.split(NAMESPACE_SEPARATOR)[0];
            if (alias.equals(optionAlias)) {
                // Place the option again, without alias.
                String optionName = key.split(NAMESPACE_SEPARATOR)[1];
                bundle.putString(optionName, bundle.getString(key));
                bundle.remove(key);
            } else {
                // Remove other aliases.
                bundle.remove(key);
            }
        }
    }

    /**
     * Helper to decide whether the collector should run or not against the test case.
     *
     * @param desc The {@link Description} of the method.
     * @return True if the collector should run.
     */
    private boolean shouldRun(Description desc) {
        MetricOption annotation = desc.getAnnotation(MetricOption.class);
        List<String> groups = new ArrayList<>();
        if (annotation != null) {
            String group = annotation.group();
            groups.addAll(Arrays.asList(group.split(",")));
        }
        if (!mExcludeFilters.isEmpty()) {
            for (String group : groups) {
                // Exclude filters has priority, if any of the group is excluded, exclude the method
                if (mExcludeFilters.contains(group)) {
                    return false;
                }
            }
        }
        // If we have include filters, we can only run what's part of them.
        if (!mIncludeFilters.isEmpty()) {
            for (String group : groups) {
                if (mIncludeFilters.contains(group)) {
                    return true;
                }
            }
            // We have include filter and did not match them.
            return false;
        }
        return true;
    }
}
