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

package com.android.tradefed.config;

import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.command.ICommandOptions;
import com.android.tradefed.config.ConfigurationDef.OptionDef;
import com.android.tradefed.device.IDeviceRecovery;
import com.android.tradefed.device.IDeviceSelection;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.log.ILeveledLogOutput;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.keystore.IKeyStoreClient;

import org.json.JSONArray;
import org.json.JSONException;

import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.List;

/**
 * Configuration information for a TradeFederation invocation.
 *
 * Each TradeFederation invocation has a single {@link IConfiguration}. An {@link IConfiguration}
 * stores all the delegate objects that should be used during the invocation, and their associated
 * {@link Option}'s
 */
public interface IConfiguration {

    /** Returns the name of the configuration. */
    public String getName();

    /**
     * Gets the {@link IBuildProvider} from the configuration.
     *
     * @return the {@link IBuildProvider} provided in the configuration
     */
    public IBuildProvider getBuildProvider();

    /**
     * Gets the {@link ITargetPreparer}s from the configuration.
     *
     * @return the {@link ITargetPreparer}s provided in order in the configuration
     */
    public List<ITargetPreparer> getTargetPreparers();

    /**
     * Gets the {@link IRemoteTest}s to run from the configuration.
     *
     * @return the tests provided in the configuration
     */
    public List<IRemoteTest> getTests();

    /**
     * Gets the {@link ITestInvocationListener}s to use from the configuration.
     *
     * @return the {@link ITestInvocationListener}s provided in the configuration.
     */
    public List<ITestInvocationListener> getTestInvocationListeners();

    /**
     * Gets the {@link IDeviceRecovery} to use from the configuration.
     *
     * @return the {@link IDeviceRecovery} provided in the configuration.
     */
    public IDeviceRecovery getDeviceRecovery();

    /**
     * Gets the {@link TestDeviceOptions} to use from the configuration.
     *
     * @return the {@link TestDeviceOptions} provided in the configuration.
     */
    public TestDeviceOptions getDeviceOptions();

    /**
     * Gets the {@link ILeveledLogOutput} to use from the configuration.
     *
     * @return the {@link ILeveledLogOutput} provided in the configuration.
     */
    public ILeveledLogOutput getLogOutput();

    /**
     * Gets the {@link ILogSaver} to use from the configuration.
     *
     * @return the {@link ILogSaver} provided in the configuration.
     */
    public ILogSaver getLogSaver();

    /**
     * Gets the {@link IMultiTargetPreparer}s from the configuration.
     *
     * @return the {@link IMultiTargetPreparer}s provided in order in the configuration
     */
    public List<IMultiTargetPreparer> getMultiTargetPreparers();

    /**
     * Gets the {@link IMultiTargetPreparer}s from the configuration that should be executed before
     * any of the devices target_preparers.
     *
     * @return the {@link IMultiTargetPreparer}s provided in order in the configuration
     */
    public List<IMultiTargetPreparer> getMultiPreTargetPreparers();

    /**
     * Gets the {@link ISystemStatusChecker}s from the configuration.
     *
     * @return the {@link ISystemStatusChecker}s provided in order in the configuration
     */
    public List<ISystemStatusChecker> getSystemStatusCheckers();

    /** Gets the {@link IMetricCollector}s from the configuration. */
    public List<IMetricCollector> getMetricCollectors();

    /**
     * Gets the {@link ICommandOptions} to use from the configuration.
     *
     * @return the {@link ICommandOptions} provided in the configuration.
     */
    public ICommandOptions getCommandOptions();

    /** Returns the {@link ConfigurationDescriptor} provided in the configuration. */
    public ConfigurationDescriptor getConfigurationDescription();

    /**
     * Gets the {@link IDeviceSelection} to use from the configuration.
     *
     * @return the {@link IDeviceSelection} provided in the configuration.
     */
    public IDeviceSelection getDeviceRequirements();

    /**
     * Generic interface to get the configuration object with the given type name.
     *
     * @param typeName the unique type of the configuration object
     *
     * @return the configuration object or <code>null</code> if the object type with given name
     * does not exist.
     */
    public Object getConfigurationObject(String typeName);

    /**
     * Similar to {@link #getConfigurationObject(String)}, but for configuration
     * object types that support multiple objects.
     *
     * @param typeName the unique type name of the configuration object
     *
     * @return the list of configuration objects or <code>null</code> if the object type with
     * given name does not exist.
     */
    public List<?> getConfigurationObjectList(String typeName);

    /**
     * Gets the {@link IDeviceConfiguration}s from the configuration.
     *
     * @return the {@link IDeviceConfiguration}s provided in order in the configuration
     */
    public List<IDeviceConfiguration> getDeviceConfig();

    /**
     * Return the {@link IDeviceConfiguration} associated to the name provided, null if not found.
     */
    public IDeviceConfiguration getDeviceConfigByName(String nameDevice);

    /**
     * Inject a option value into the set of configuration objects.
     * <p/>
     * Useful to provide values for options that are generated dynamically.
     *
     * @param optionName the option name
     * @param optionValue the option value
     * @throws ConfigurationException if failed to set the option's value
     */
    public void injectOptionValue(String optionName, String optionValue)
            throws ConfigurationException;

    /**
     * Inject a option value into the set of configuration objects.
     * <p/>
     * Useful to provide values for options that are generated dynamically.
     *
     * @param optionName the option name
     * @param optionKey the optional key for map options, or null
     * @param optionValue the map option value
     * @throws ConfigurationException if failed to set the option's value
     */
    public void injectOptionValue(String optionName, String optionKey, String optionValue)
            throws ConfigurationException;

    /**
     * Inject a option value into the set of configuration objects.
     * <p/>
     * Useful to provide values for options that are generated dynamically.
     *
     * @param optionName the option name
     * @param optionKey the optional key for map options, or null
     * @param optionValue the map option value
     * @param optionSource the source config that provided this option value
     * @throws ConfigurationException if failed to set the option's value
     */
    public void injectOptionValueWithSource(String optionName, String optionKey, String optionValue,
            String optionSource) throws ConfigurationException;

    /**
     * Inject multiple option values into the set of configuration objects.
     * <p/>
     * Useful to inject many option values at once after creating a new object.
     *
     * @param optionDefs a list of option defs to inject
     * @throws ConfigurationException if failed to set option values
     */
    public void injectOptionValues(List<OptionDef> optionDefs) throws ConfigurationException;

    /**
     * Create a copy of this object.
     *
     * @return a {link IConfiguration} copy
     */
    public IConfiguration clone();

    /**
     * Replace the current {@link IBuildProvider} in the configuration.
     *
     * @param provider the new {@link IBuildProvider}
     */
    public void setBuildProvider(IBuildProvider provider);

    /**
     * Set the {@link ILeveledLogOutput}, replacing any existing value.
     *
     * @param logger
     */
    public void setLogOutput(ILeveledLogOutput logger);

    /**
     * Set the {@link ILogSaver}, replacing any existing value.
     *
     * @param logSaver
     */
    public void setLogSaver(ILogSaver logSaver);

    /**
     * Set the {@link IDeviceRecovery}, replacing any existing value.
     *
     * @param recovery
     */
    public void setDeviceRecovery(IDeviceRecovery recovery);

    /**
     * Set the {@link ITargetPreparer}, replacing any existing value.
     *
     * @param preparer
     */
    public void setTargetPreparer(ITargetPreparer preparer);

    /**
     * Set a {@link IDeviceConfiguration}, replacing any existing value.
     *
     * @param deviceConfig
     */
    public void setDeviceConfig(IDeviceConfiguration deviceConfig);

    /**
     * Set the {@link IDeviceConfiguration}s, replacing any existing value.
     *
     * @param deviceConfigs
     */
    public void setDeviceConfigList(List<IDeviceConfiguration> deviceConfigs);

    /**
     * Convenience method to set a single {@link IRemoteTest} in this configuration, replacing any
     * existing values
     *
     * @param test
     */
    public void setTest(IRemoteTest test);

    /**
     * Set the list of {@link IRemoteTest}s in this configuration, replacing any
     * existing values
     *
     * @param tests
     */
    public void setTests(List<IRemoteTest> tests);

    /**
     * Set the list of {@link IMultiTargetPreparer}s in this configuration, replacing any
     * existing values
     *
     * @param multiTargPreps
     */
    public void setMultiTargetPreparers(List<IMultiTargetPreparer> multiTargPreps);

    /**
     * Convenience method to set a single {@link IMultiTargetPreparer} in this configuration,
     * replacing any existing values
     *
     * @param multiTargPrep
     */
    public void setMultiTargetPreparer(IMultiTargetPreparer multiTargPrep);

    /**
     * Set the list of {@link IMultiTargetPreparer}s in this configuration that should be executed
     * before any of the devices target_preparers, replacing any existing values
     *
     * @param multiPreTargPreps
     */
    public void setMultiPreTargetPreparers(List<IMultiTargetPreparer> multiPreTargPreps);

    /**
     * Convenience method to set a single {@link IMultiTargetPreparer} in this configuration that
     * should be executed before any of the devices target_preparers, replacing any existing values
     *
     * @param multiPreTargPreps
     */
    public void setMultiPreTargetPreparer(IMultiTargetPreparer multiPreTargPreps);

    /**
     * Set the list of {@link ISystemStatusChecker}s in this configuration, replacing any
     * existing values
     *
     * @param systemCheckers
     */
    public void setSystemStatusCheckers(List<ISystemStatusChecker> systemCheckers);

    /**
     * Convenience method to set a single {@link ISystemStatusChecker} in this configuration,
     * replacing any existing values
     *
     * @param systemChecker
     */
    public void setSystemStatusChecker(ISystemStatusChecker systemChecker);

    /**
     * Set the list of {@link ITestInvocationListener}s, replacing any existing values
     *
     * @param listeners
     */
    public void setTestInvocationListeners(List<ITestInvocationListener> listeners);

    /**
     * Convenience method to set a single {@link ITestInvocationListener}
     *
     * @param listener
     */
    public void setTestInvocationListener(ITestInvocationListener listener);

    /** Set the list of {@link IMetricCollector}s, replacing any existing values. */
    public void setDeviceMetricCollectors(List<IMetricCollector> collectors);

    /**
     * Set the {@link ICommandOptions}, replacing any existing values
     *
     * @param cmdOptions
     */
    public void setCommandOptions(ICommandOptions cmdOptions);

    /**
     * Set the {@link IDeviceSelection}, replacing any existing values
     *
     * @param deviceSelection
     */
    public void setDeviceRequirements(IDeviceSelection deviceSelection);

    /**
     * Set the {@link TestDeviceOptions}, replacing any existing values
     */
    public void setDeviceOptions(TestDeviceOptions deviceOptions);

    /**
     * Generic method to set the config object with the given name, replacing any existing value.
     *
     * @param name the unique name of the config object type.
     * @param configObject the config object
     * @throws ConfigurationException if the configObject was not the correct type
     */
    public void setConfigurationObject(String name, Object configObject)
            throws ConfigurationException;

    /**
     * Generic method to set the config object list for the given name, replacing any existing
     * value.
     *
     * @param name the unique name of the config object type.
     * @param configList the config object list
     * @throws ConfigurationException if any objects in the list are not the correct type
     */
    public void setConfigurationObjectList(String name, List<?> configList)
            throws ConfigurationException;

    /** Returns whether or not a configured device is tagged isFake=true or not. */
    public boolean isDeviceConfiguredFake(String deviceName);

    /**
     * Set the config {@link Option} fields with given set of command line arguments
     * <p/>
     * {@link ArgsOptionParser} for expected format
     *
     * @param listArgs the command line arguments
     * @return the unconsumed arguments
     */
    public List<String> setOptionsFromCommandLineArgs(List<String> listArgs)
            throws ConfigurationException;

    /**
     * Set the config {@link Option} fields with given set of command line arguments
     * <p/>
     * See {@link ArgsOptionParser} for expected format
     *
     * @param listArgs the command line arguments
     * @param keyStoreClient {@link IKeyStoreClient} to use.
     * @return the unconsumed arguments
     */
    public List<String> setOptionsFromCommandLineArgs(List<String> listArgs,
            IKeyStoreClient keyStoreClient)
            throws ConfigurationException;

    /**
     * Outputs a command line usage help text for this configuration to given printStream.
     *
     * @param importantOnly if <code>true</code> only print help for the important options
     * @param out the {@link PrintStream} to use.
     * @throws ConfigurationException
     */
    public void printCommandUsage(boolean importantOnly, PrintStream out)
            throws ConfigurationException;

    /**
     * Returns a JSON representation of this configuration.
     * <p/>
     * The return value is a JSONArray containing JSONObjects to represent each configuration
     * object. Each configuration object entry has the following structure:
     * <pre>
     * {@code
     *   &#123;
     *     "alias": "device-unavail-email",
     *     "name": "result_reporter",
     *     "class": "com.android.tradefed.result.DeviceUnavailEmailResultReporter",
     *     "options": [ ... ]
     *   &#125;
     * }
     * </pre>
     * The "options" entry is a JSONArray containing JSONObjects to represent each @Option annotated
     * field. Each option entry has the following structure:
     * <pre>
     * {@code
     *   &#123;
     *     "updateRule": "LAST",
     *     "isTimeVal": false,
     *     "source": "google\/template\/reporters\/asit",
     *     "importance": "IF_UNSET",
     *     "description": "The envelope-sender address to use for the messages.",
     *     "mandatory": false,
     *     "name": "sender",
     *     "javaClass": "java.lang.String",
     *     "value": "tffail@google.com"
     *   &#125;
     * }
     * </pre>
     * Most of the values come from the @Option annotation. 'javaClass' is the name of the
     * underlying java class for this option. 'value' is a JSON representation of the field's
     * current value. 'source' is the set of config names which set the field's value. For regular
     * objects or Collections, 'source' is a JSONArray containing each contributing config's name.
     * For map fields, sources for each key are tracked individually and stored in a JSONObject.
     * Each key / value pair in the JSONObject corresponds to a key in the map and an array of its
     * source configurations.
     *
     * @throws JSONException
     */
    public JSONArray getJsonCommandUsage() throws JSONException;

    /**
     * Validate option values.
     * <p/>
     * Currently this will just validate that all mandatory options have been set
     *
     * @throws ConfigurationException if config is not valid
     */
    public void validateOptions() throws ConfigurationException;

    /**
     * Sets the command line used to create this {@link IConfiguration}.
     * This stores the whole command line, including the configuration name,
     * unlike setOptionsFromCommandLineArgs.
     *
     * @param arrayArgs the command line
     */
    public void setCommandLine(String[] arrayArgs);

    /**
     * Gets the command line used to create this {@link IConfiguration}.
     *
     * @return the command line used to create this {@link IConfiguration}.
     */
    public String getCommandLine();

    /**
     * Gets the expanded XML file for the config with all options shown for this
     * {@link IConfiguration} as a {@link String}.
     *
     * @param output the writer to print the xml to.
     * @throws IOException
     */
    public void dumpXml(PrintWriter output) throws IOException;

    /**
     * Gets the expanded XML file for the config with all options shown for this {@link
     * IConfiguration} minus the objects filters by their key name.
     *
     * <p>Filter example: {@link Configuration#TARGET_PREPARER_TYPE_NAME}.
     *
     * @param output the writer to print the xml to.
     * @param excludeFilters the list of object type that should not be dumped.
     * @throws IOException
     */
    public void dumpXml(PrintWriter output, List<String> excludeFilters) throws IOException;
}
