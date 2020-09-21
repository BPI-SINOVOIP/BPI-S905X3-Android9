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
import com.android.tradefed.command.CommandOptions;
import com.android.tradefed.command.ICommandOptions;
import com.android.tradefed.config.ConfigurationDef.OptionDef;
import com.android.tradefed.config.OptionSetter.FieldDef;
import com.android.tradefed.device.IDeviceRecovery;
import com.android.tradefed.device.IDeviceSelection;
import com.android.tradefed.device.TestDeviceOptions;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.log.ILeveledLogOutput;
import com.android.tradefed.log.StdoutLogger;
import com.android.tradefed.result.FileSystemLogSaver;
import com.android.tradefed.result.ILogSaver;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.TextResultReporter;
import com.android.tradefed.sandbox.SandboxOptions;
import com.android.tradefed.suite.checker.ISystemStatusChecker;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.StubTest;
import com.android.tradefed.util.MultiMap;
import com.android.tradefed.util.QuotationAwareTokenizer;
import com.android.tradefed.util.keystore.IKeyStoreClient;

import com.google.common.base.Joiner;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.kxml2.io.KXmlSerializer;

import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.regex.Pattern;

/**
 * A concrete {@link IConfiguration} implementation that stores the loaded config objects in a map.
 */
public class Configuration implements IConfiguration {

    // type names for built in configuration objects
    public static final String BUILD_PROVIDER_TYPE_NAME = "build_provider";
    public static final String TARGET_PREPARER_TYPE_NAME = "target_preparer";
    // Variation of Multi_target_preparer that runs BEFORE each device target_preparer.
    public static final String MULTI_PRE_TARGET_PREPARER_TYPE_NAME = "multi_pre_target_preparer";
    public static final String MULTI_PREPARER_TYPE_NAME = "multi_target_preparer";
    public static final String TEST_TYPE_NAME = "test";
    public static final String DEVICE_RECOVERY_TYPE_NAME = "device_recovery";
    public static final String LOGGER_TYPE_NAME = "logger";
    public static final String LOG_SAVER_TYPE_NAME = "log_saver";
    public static final String RESULT_REPORTER_TYPE_NAME = "result_reporter";
    public static final String CMD_OPTIONS_TYPE_NAME = "cmd_options";
    public static final String DEVICE_REQUIREMENTS_TYPE_NAME = "device_requirements";
    public static final String DEVICE_OPTIONS_TYPE_NAME = "device_options";
    public static final String SYSTEM_STATUS_CHECKER_TYPE_NAME = "system_checker";
    public static final String CONFIGURATION_DESCRIPTION_TYPE_NAME = "config_desc";
    public static final String DEVICE_NAME = "device";
    public static final String DEVICE_METRICS_COLLECTOR_TYPE_NAME = "metrics_collector";
    public static final String SANDBOX_TYPE_NAME = "sandbox";
    public static final String SANBOX_OPTIONS_TYPE_NAME = "sandbox_options";

    private static Map<String, ObjTypeInfo> sObjTypeMap = null;
    private static Set<String> sMultiDeviceSupportedTag = null;

    // regexp pattern used to parse map option values
    private static final Pattern OPTION_KEY_VALUE_PATTERN = Pattern.compile("(?<!\\\\)=");

    private static final String CONFIG_EXCEPTION_PATTERN = "Could not find option with name ";

    /** Mapping of config object type name to config objects. */
    private Map<String, List<Object>> mConfigMap;
    private final String mName;
    private final String mDescription;
    // original command line used to create this given configuration.
    private String[] mCommandLine;

    // Used to track config names that were used to set field values
    private MultiMap<FieldDef, String> mFieldSources = new MultiMap<>();

    /**
     * Container struct for built-in config object type
     */
    private static class ObjTypeInfo {
        final Class<?> mExpectedType;
        /**
         * true if a list (ie many objects in a single config) are supported for this type
         */
        final boolean mIsListSupported;

        ObjTypeInfo(Class<?> expectedType, boolean isList) {
            mExpectedType = expectedType;
            mIsListSupported = isList;
        }
    }

    /**
     * Determine if given config object type name is a built in object
     *
     * @param typeName the config object type name
     * @return <code>true</code> if name is a built in object type
     */
    static boolean isBuiltInObjType(String typeName) {
        return getObjTypeMap().containsKey(typeName);
    }

    private static synchronized Map<String, ObjTypeInfo> getObjTypeMap() {
        if (sObjTypeMap == null) {
            sObjTypeMap = new HashMap<String, ObjTypeInfo>();
            sObjTypeMap.put(BUILD_PROVIDER_TYPE_NAME, new ObjTypeInfo(IBuildProvider.class, false));
            sObjTypeMap.put(TARGET_PREPARER_TYPE_NAME,
                    new ObjTypeInfo(ITargetPreparer.class, true));
            sObjTypeMap.put(
                    MULTI_PRE_TARGET_PREPARER_TYPE_NAME,
                    new ObjTypeInfo(IMultiTargetPreparer.class, true));
            sObjTypeMap.put(MULTI_PREPARER_TYPE_NAME,
                    new ObjTypeInfo(IMultiTargetPreparer.class, true));
            sObjTypeMap.put(TEST_TYPE_NAME, new ObjTypeInfo(IRemoteTest.class, true));
            sObjTypeMap.put(DEVICE_RECOVERY_TYPE_NAME,
                    new ObjTypeInfo(IDeviceRecovery.class, false));
            sObjTypeMap.put(LOGGER_TYPE_NAME, new ObjTypeInfo(ILeveledLogOutput.class, false));
            sObjTypeMap.put(LOG_SAVER_TYPE_NAME, new ObjTypeInfo(ILogSaver.class, false));
            sObjTypeMap.put(RESULT_REPORTER_TYPE_NAME,
                    new ObjTypeInfo(ITestInvocationListener.class, true));
            sObjTypeMap.put(CMD_OPTIONS_TYPE_NAME, new ObjTypeInfo(ICommandOptions.class,
                    false));
            sObjTypeMap.put(DEVICE_REQUIREMENTS_TYPE_NAME, new ObjTypeInfo(IDeviceSelection.class,
                    false));
            sObjTypeMap.put(DEVICE_OPTIONS_TYPE_NAME, new ObjTypeInfo(TestDeviceOptions.class,
                    false));
            sObjTypeMap.put(DEVICE_NAME, new ObjTypeInfo(IDeviceConfiguration.class, true));
            sObjTypeMap.put(SYSTEM_STATUS_CHECKER_TYPE_NAME,
                    new ObjTypeInfo(ISystemStatusChecker.class, true));
            sObjTypeMap.put(
                    CONFIGURATION_DESCRIPTION_TYPE_NAME,
                    new ObjTypeInfo(ConfigurationDescriptor.class, false));
            sObjTypeMap.put(
                    DEVICE_METRICS_COLLECTOR_TYPE_NAME,
                    new ObjTypeInfo(IMetricCollector.class, true));
            sObjTypeMap.put(SANBOX_OPTIONS_TYPE_NAME, new ObjTypeInfo(SandboxOptions.class, false));
        }
        return sObjTypeMap;
    }

    /**
     * Determine if a given config object type is allowed to exists inside a device tag
     * configuration.
     * Authorized type are: build_provider, target_preparer, device_recovery, device_requirements,
     * device_options
     *
     * @param typeName the config object type name
     * @return True if name is allowed to exists inside the device tag
     */
    static boolean doesBuiltInObjSupportMultiDevice(String typeName) {
        return getMultiDeviceSupportedTag().contains(typeName);
    }

    /**
     * Return the {@link Set} of tags that are supported in a device tag for multi device
     * configuration.
     */
    private static synchronized Set<String> getMultiDeviceSupportedTag() {
        if (sMultiDeviceSupportedTag == null) {
            sMultiDeviceSupportedTag = new HashSet<String>();
            sMultiDeviceSupportedTag.add(BUILD_PROVIDER_TYPE_NAME);
            sMultiDeviceSupportedTag.add(TARGET_PREPARER_TYPE_NAME);
            sMultiDeviceSupportedTag.add(DEVICE_RECOVERY_TYPE_NAME);
            sMultiDeviceSupportedTag.add(DEVICE_REQUIREMENTS_TYPE_NAME);
            sMultiDeviceSupportedTag.add(DEVICE_OPTIONS_TYPE_NAME);
        }
        return sMultiDeviceSupportedTag;
    }

    /**
     * Creates an {@link Configuration} with default config objects.
     */
    public Configuration(String name, String description) {
        mName = name;
        mDescription = description;
        mConfigMap = new LinkedHashMap<String, List<Object>>();
        setDeviceConfig(new DeviceConfigurationHolder(ConfigurationDef.DEFAULT_DEVICE_NAME));
        setCommandOptions(new CommandOptions());
        setTest(new StubTest());
        setLogOutput(new StdoutLogger());
        setLogSaver(new FileSystemLogSaver()); // FileSystemLogSaver saves to tmp by default.
        setTestInvocationListener(new TextResultReporter());
        // Init an empty list of target_preparers
        setConfigurationObjectListNoThrow(TARGET_PREPARER_TYPE_NAME, new ArrayList<>());
        setMultiPreTargetPreparers(new ArrayList<>());
        setMultiTargetPreparers(new ArrayList<>());
        setSystemStatusCheckers(new ArrayList<ISystemStatusChecker>());
        setConfigurationDescriptor(new ConfigurationDescriptor());
        setDeviceMetricCollectors(new ArrayList<>());
        setConfigurationObjectNoThrow(SANBOX_OPTIONS_TYPE_NAME, new SandboxOptions());
    }

    /**
     * If we are in multi device mode, we cannot allow fetching the regular references because
     * they are most likely wrong.
     */
    private void notAllowedInMultiMode(String function) {
        if (getConfigurationObjectList(DEVICE_NAME).size() > 1) {
            throw new UnsupportedOperationException(String.format("Calling %s is not allowed "
                    + "in multi device mode", function));
        }
        if (getConfigurationObjectList(DEVICE_NAME).size() == 0) {
            throw new UnsupportedOperationException(
                    "We should always have at least 1 Device config");
        }
    }

    /** {@inheritDoc} */
    @Override
    public String getName() {
        return mName;
    }

    /**
     * @return a short user readable description this {@link Configuration}
     */
    public String getDescription() {
        return mDescription;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCommandLine(String[] arrayArgs) {
        mCommandLine = arrayArgs;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getCommandLine() {
        // FIXME: obfuscated passwords from command line.
        if (mCommandLine != null && mCommandLine.length != 0) {
            return QuotationAwareTokenizer.combineTokens(mCommandLine);
        }
        // If no args were available return null.
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public IBuildProvider getBuildProvider() {
        notAllowedInMultiMode("getBuildProvider");
        return ((List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME))
                .get(0).getBuildProvider();
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<ITargetPreparer> getTargetPreparers() {
        notAllowedInMultiMode("getTargetPreparers");
        return ((List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME))
                .get(0).getTargetPreparers();
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<IRemoteTest> getTests() {
        return (List<IRemoteTest>) getConfigurationObjectList(TEST_TYPE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public IDeviceRecovery getDeviceRecovery() {
        notAllowedInMultiMode("getDeviceRecovery");
        return ((List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME))
                .get(0).getDeviceRecovery();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ILeveledLogOutput getLogOutput() {
        return (ILeveledLogOutput) getConfigurationObject(LOGGER_TYPE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ILogSaver getLogSaver() {
        return (ILogSaver) getConfigurationObject(LOG_SAVER_TYPE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<IMultiTargetPreparer> getMultiTargetPreparers() {
        return (List<IMultiTargetPreparer>) getConfigurationObjectList(MULTI_PREPARER_TYPE_NAME);
    }

    /** {@inheritDoc} */
    @SuppressWarnings("unchecked")
    @Override
    public List<IMultiTargetPreparer> getMultiPreTargetPreparers() {
        return (List<IMultiTargetPreparer>)
                getConfigurationObjectList(MULTI_PRE_TARGET_PREPARER_TYPE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<ISystemStatusChecker> getSystemStatusCheckers() {
        return (List<ISystemStatusChecker>)
                getConfigurationObjectList(SYSTEM_STATUS_CHECKER_TYPE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<ITestInvocationListener> getTestInvocationListeners() {
        return (List<ITestInvocationListener>) getConfigurationObjectList(
                RESULT_REPORTER_TYPE_NAME);
    }

    @SuppressWarnings("unchecked")
    @Override
    public List<IMetricCollector> getMetricCollectors() {
        return (List<IMetricCollector>)
                getConfigurationObjectList(DEVICE_METRICS_COLLECTOR_TYPE_NAME);
    }

    /** {@inheritDoc} */
    @Override
    public ICommandOptions getCommandOptions() {
        return (ICommandOptions) getConfigurationObject(CMD_OPTIONS_TYPE_NAME);
    }

    /** {@inheritDoc} */
    @Override
    public ConfigurationDescriptor getConfigurationDescription() {
        return (ConfigurationDescriptor)
                getConfigurationObject(CONFIGURATION_DESCRIPTION_TYPE_NAME);
    }

    /** {@inheritDoc} */
    @SuppressWarnings("unchecked")
    @Override
    public IDeviceSelection getDeviceRequirements() {
        notAllowedInMultiMode("getDeviceRequirements");
        return ((List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME))
                .get(0).getDeviceRequirements();
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public TestDeviceOptions getDeviceOptions() {
        notAllowedInMultiMode("getDeviceOptions");
        return ((List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME))
                .get(0).getDeviceOptions();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<?> getConfigurationObjectList(String typeName) {
        return mConfigMap.get(typeName);
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public IDeviceConfiguration getDeviceConfigByName(String nameDevice) {
        for (IDeviceConfiguration deviceHolder :
                (List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME)) {
            if (deviceHolder.getDeviceName().equals(nameDevice)) {
                return deviceHolder;
            }
        }
        return null;
    }

    /**
     * {@inheritDoc}
     */
    @SuppressWarnings("unchecked")
    @Override
    public List<IDeviceConfiguration> getDeviceConfig() {
        return (List<IDeviceConfiguration>)getConfigurationObjectList(DEVICE_NAME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Object getConfigurationObject(String typeName) {
        List<?> configObjects = getConfigurationObjectList(typeName);
        if (configObjects == null) {
            return null;
        }
        ObjTypeInfo typeInfo = getObjTypeMap().get(typeName);
        if (typeInfo != null && typeInfo.mIsListSupported) {
            throw new IllegalStateException(
                    String.format(
                            "Wrong method call for type %s. Used getConfigurationObject() for a "
                                    + "config object that is stored as a list",
                            typeName));
        }
        if (configObjects.size() != 1) {
            throw new IllegalStateException(String.format(
                    "Attempted to retrieve single object for %s, but %d are present",
                    typeName, configObjects.size()));
        }
        return configObjects.get(0);
    }

    /**
     * Return a copy of all config objects
     */
    private Collection<Object> getAllConfigurationObjects() {
        return getAllConfigurationObjects(null);
    }

    /**
     * Return a copy of all config objects, minus the object configuration of the type specified.
     * Returns all the config objects if param is null.
     */
    private Collection<Object> getAllConfigurationObjects(String excludedConfigName) {
        Collection<Object> objectsCopy = new ArrayList<Object>();
        for (Entry<String, List<Object>> entryList : mConfigMap.entrySet()) {
            if (excludedConfigName != null) {
                // Only add if not a descriptor config object type.
                if (!excludedConfigName.equals(entryList.getKey())) {
                    objectsCopy.addAll(entryList.getValue());
                }
            } else {
                objectsCopy.addAll(entryList.getValue());
            }
        }
        return objectsCopy;
    }

    /**
     * Creates an OptionSetter which is appropriate for setting options on all objects which
     * will be returned by {@link #getAllConfigurationObjects}.
     */
    private OptionSetter createOptionSetter() throws ConfigurationException {
        return new OptionSetter(getAllConfigurationObjects());
    }

    /**
     * Injects an option value into the set of configuration objects.
     *
     * Uses provided arguments as is and fails if arguments have invalid format or
     * provided ambiguously, e.g. {@code optionKey} argument is provided for non-map option,
     * or the value for an option of integer type cannot be parsed as an integer number.
     *
     * @param optionSetter setter to use for the injection
     * @param optionName name of the option
     * @param optionKey map key, if the option is of map type
     * @param optionValue value of the option or map value, if the option is of map type
     * @param source source of the option
     * @throws ConfigurationException if option value cannot be injected
     */
    private void internalInjectOptionValue(OptionSetter optionSetter, String optionName,
            String optionKey, String optionValue, String source) throws ConfigurationException {
        if (optionSetter == null) {
            throw new IllegalArgumentException("optionSetter cannot be null");
        }

        // Set all fields that match this option name / key
        List<FieldDef> affectedFields = optionSetter.setOptionValue(
                optionName, optionKey, optionValue);

        if (source != null) {
            // Update the source for each affected field
            for (FieldDef field : affectedFields) {
                // Unless the field is a Collection or MultiMap entry, it can only have one source
                if (!Collection.class.isAssignableFrom(field.field.getType()) &&
                        !MultiMap.class.isAssignableFrom(field.field.getType())) {
                    mFieldSources.remove(field);
                }
                mFieldSources.put(field, source);
            }
        }
    }

    /**
     * Injects an option value into the set of configuration objects.
     *
     * If the option to be set is of map type, an attempt to parse {@code optionValue} argument
     * into key-value pair is made. In this case {@code optionValue} must have an equal sign
     * separating a key and a value (e.g. my_key=my_value).
     * In case a key or a value themselves contain an equal sign, this equal sign in them
     * must be escaped using a backslash (e.g. a\=b=y\=z).
     *
     * @param optionSetter setter to use for the injection
     * @param optionName name of the option
     * @param optionValue value of the option
     * @throws ConfigurationException if option value cannot be injected
     */
    private void internalInjectOptionValue(OptionSetter optionSetter, String optionName,
            String optionValue) throws ConfigurationException {
        // Cannot continue without optionSetter
        if (optionSetter == null) {
            throw new IllegalArgumentException("optionSetter cannot be null");
        }

        // If the option is not a map, then the key is null...
        if (!optionSetter.isMapOption(optionName)) {
            internalInjectOptionValue(optionSetter, optionName, null, optionValue, null);
            return;
        }

        // ..., otherwise try to parse the value to retrieve the key
        String[] parts = OPTION_KEY_VALUE_PATTERN.split(optionValue);
        if (parts.length != 2) {
            throw new ConfigurationException(String.format(
                    "option '%s' has an invalid format for value %s:w",
                    optionName, optionValue));
        }
        internalInjectOptionValue(optionSetter, optionName,
                parts[0].replace("\\\\=", "="), parts[1].replace("\\\\=", "="), null);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void injectOptionValue(String optionName, String optionValue)
            throws ConfigurationException {
        internalInjectOptionValue(createOptionSetter(), optionName, optionValue);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void injectOptionValue(String optionName, String optionKey, String optionValue)
            throws ConfigurationException {
        internalInjectOptionValue(createOptionSetter(), optionName, optionKey, optionValue, null);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void injectOptionValueWithSource(String optionName, String optionKey, String optionValue,
            String source) throws ConfigurationException {
        internalInjectOptionValue(createOptionSetter(), optionName, optionKey, optionValue, source);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void injectOptionValues(List<OptionDef> optionDefs) throws ConfigurationException {
        OptionSetter optionSetter = createOptionSetter();
        for (OptionDef optionDef : optionDefs) {
            internalInjectOptionValue(optionSetter, optionDef.name, optionDef.key, optionDef.value,
                    optionDef.source);
        }
    }

    /**
     * Creates a shallow copy of this object.
     */
    @Override
    public Configuration clone() {
        Configuration clone = new Configuration(getName(), getDescription());
        for (Map.Entry<String, List<Object>> entry : mConfigMap.entrySet()) {
            if (DEVICE_NAME.equals(entry.getKey())) {
                List<Object> newDeviceConfigList = new ArrayList<Object>();
                for (Object deviceConfig : entry.getValue()) {
                    IDeviceConfiguration config = ((IDeviceConfiguration)deviceConfig);
                    IDeviceConfiguration newDeviceConfig = config.clone();
                    newDeviceConfigList.add(newDeviceConfig);
                }
                clone.setConfigurationObjectListNoThrow(entry.getKey(), newDeviceConfigList);
            } else {
                clone.setConfigurationObjectListNoThrow(entry.getKey(), entry.getValue());
            }
        }
        return clone;
    }

    private void addToDefaultDeviceConfig(Object obj) {
        try {
            getDeviceConfigByName(ConfigurationDef.DEFAULT_DEVICE_NAME).addSpecificConfig(obj);
        } catch (ConfigurationException e) {
            // should never happen
            throw new IllegalArgumentException(e);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuildProvider(IBuildProvider provider) {
        notAllowedInMultiMode("setBuildProvider");
        addToDefaultDeviceConfig(provider);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestInvocationListeners(List<ITestInvocationListener> listeners) {
        setConfigurationObjectListNoThrow(RESULT_REPORTER_TYPE_NAME, listeners);
    }

    @Override
    public void setDeviceMetricCollectors(List<IMetricCollector> collectors) {
        setConfigurationObjectListNoThrow(DEVICE_METRICS_COLLECTOR_TYPE_NAME, collectors);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTestInvocationListener(ITestInvocationListener listener) {
        setConfigurationObjectNoThrow(RESULT_REPORTER_TYPE_NAME, listener);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceConfig(IDeviceConfiguration deviceConfig) {
        setConfigurationObjectNoThrow(DEVICE_NAME, deviceConfig);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceConfigList(List<IDeviceConfiguration> deviceConfigs) {
        setConfigurationObjectListNoThrow(DEVICE_NAME, deviceConfigs);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTest(IRemoteTest test) {
        setConfigurationObjectNoThrow(TEST_TYPE_NAME, test);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTests(List<IRemoteTest> tests) {
        setConfigurationObjectListNoThrow(TEST_TYPE_NAME, tests);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMultiTargetPreparers(List<IMultiTargetPreparer> multiTargPreps) {
        setConfigurationObjectListNoThrow(MULTI_PREPARER_TYPE_NAME, multiTargPreps);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setMultiTargetPreparer(IMultiTargetPreparer multiTargPrep) {
        setConfigurationObjectNoThrow(MULTI_PREPARER_TYPE_NAME, multiTargPrep);
    }

    /** {@inheritDoc} */
    @Override
    public void setMultiPreTargetPreparers(List<IMultiTargetPreparer> multiPreTargPreps) {
        setConfigurationObjectListNoThrow(MULTI_PRE_TARGET_PREPARER_TYPE_NAME, multiPreTargPreps);
    }

    /** {@inheritDoc} */
    @Override
    public void setMultiPreTargetPreparer(IMultiTargetPreparer multiPreTargPrep) {
        setConfigurationObjectNoThrow(MULTI_PRE_TARGET_PREPARER_TYPE_NAME, multiPreTargPrep);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSystemStatusCheckers(List<ISystemStatusChecker> systemCheckers) {
        setConfigurationObjectListNoThrow(SYSTEM_STATUS_CHECKER_TYPE_NAME, systemCheckers);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setSystemStatusChecker(ISystemStatusChecker systemChecker) {
        setConfigurationObjectNoThrow(SYSTEM_STATUS_CHECKER_TYPE_NAME, systemChecker);
    }

    /** {@inheritDoc} */
    @Override
    public void setLogOutput(ILeveledLogOutput logger) {
        setConfigurationObjectNoThrow(LOGGER_TYPE_NAME, logger);
    }

    /** {@inheritDoc} */
    @Override
    public void setLogSaver(ILogSaver logSaver) {
        setConfigurationObjectNoThrow(LOG_SAVER_TYPE_NAME, logSaver);
    }

    /** Sets the {@link ConfigurationDescriptor} to be used in the configuration. */
    private void setConfigurationDescriptor(ConfigurationDescriptor configDescriptor) {
        setConfigurationObjectNoThrow(CONFIGURATION_DESCRIPTION_TYPE_NAME, configDescriptor);
    }

    /** {@inheritDoc} */
    @Override
    public void setDeviceRecovery(IDeviceRecovery recovery) {
        notAllowedInMultiMode("setDeviceRecovery");
        addToDefaultDeviceConfig(recovery);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setTargetPreparer(ITargetPreparer preparer) {
        notAllowedInMultiMode("setTargetPreparer");
        addToDefaultDeviceConfig(preparer);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setCommandOptions(ICommandOptions cmdOptions) {
        setConfigurationObjectNoThrow(CMD_OPTIONS_TYPE_NAME, cmdOptions);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceRequirements(IDeviceSelection devRequirements) {
        notAllowedInMultiMode("setDeviceRequirements");
        addToDefaultDeviceConfig(devRequirements);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDeviceOptions(TestDeviceOptions devOptions) {
        notAllowedInMultiMode("setDeviceOptions");
        addToDefaultDeviceConfig(devOptions);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void setConfigurationObject(String typeName, Object configObject)
            throws ConfigurationException {
        if (configObject == null) {
            throw new IllegalArgumentException("configObject cannot be null");
        }
        mConfigMap.remove(typeName);
        addObject(typeName, configObject);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized void setConfigurationObjectList(String typeName, List<?> configList)
            throws ConfigurationException {
        if (configList == null) {
            throw new IllegalArgumentException("configList cannot be null");
        }
        mConfigMap.remove(typeName);
        mConfigMap.put(typeName, new ArrayList<Object>(1));
        for (Object configObject : configList) {
            addObject(typeName, configObject);
        }
    }

    /** {@inheritDoc} */
    @Override
    public boolean isDeviceConfiguredFake(String deviceName) {
        IDeviceConfiguration deviceConfig = getDeviceConfigByName(deviceName);
        if (deviceConfig == null) {
            return false;
        }
        return deviceConfig.isFake();
    }

    /**
     * Adds a loaded object to this configuration.
     *
     * @param typeName the unique object type name of the configuration object
     * @param configObject the configuration object
     * @throws ConfigurationException if object was not the correct type
     */
    private synchronized void addObject(String typeName, Object configObject)
            throws ConfigurationException {
        List<Object> objList = mConfigMap.get(typeName);
        if (objList == null) {
            objList = new ArrayList<Object>(1);
            mConfigMap.put(typeName, objList);
        }
        ObjTypeInfo typeInfo = getObjTypeMap().get(typeName);
        if (typeInfo != null && !typeInfo.mExpectedType.isInstance(configObject)) {
            throw new ConfigurationException(String.format(
                    "The config object %s is not the correct type. Expected %s, received %s",
                    typeName, typeInfo.mExpectedType.getCanonicalName(),
                    configObject.getClass().getCanonicalName()));
        }
        if (typeInfo != null && !typeInfo.mIsListSupported && objList.size() > 0) {
            throw new ConfigurationException(String.format(
                    "Only one config object allowed for %s, but multiple were specified.",
                    typeName));
        }
        objList.add(configObject);
        if (configObject instanceof IConfigurationReceiver) {
            ((IConfigurationReceiver) configObject).setConfiguration(this);
        }
        // Inject to object inside device holder too.
        if (configObject instanceof IDeviceConfiguration) {
            for (Object obj : ((IDeviceConfiguration) configObject).getAllObjects()) {
                if (obj instanceof IConfigurationReceiver) {
                    ((IConfigurationReceiver) obj).setConfiguration(this);
                }
            }
        }
    }

    /**
     * A wrapper around {@link #setConfigurationObject(String, Object)} that
     * will not throw {@link ConfigurationException}.
     * <p/>
     * Intended to be used in cases where its guaranteed that
     * <var>configObject</var> is the correct type.
     *
     * @param typeName
     * @param configObject
     */
    private void setConfigurationObjectNoThrow(String typeName, Object configObject) {
        try {
            setConfigurationObject(typeName, configObject);
        } catch (ConfigurationException e) {
            // should never happen
            throw new IllegalArgumentException(e);
        }
    }

    /**
     * A wrapper around {@link #setConfigurationObjectList(String, List)} that
     * will not throw {@link ConfigurationException}.
     * <p/>
     * Intended to be used in cases where its guaranteed that
     * <var>configObject</var> is the correct type
     *
     * @param typeName
     * @param configList
     */
    private void setConfigurationObjectListNoThrow(String typeName, List<?> configList) {
        try {
            setConfigurationObjectList(typeName, configList);
        } catch (ConfigurationException e) {
            // should never happen
            throw new IllegalArgumentException(e);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> setOptionsFromCommandLineArgs(List<String> listArgs)
            throws ConfigurationException {
        return setOptionsFromCommandLineArgs(listArgs, null);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<String> setOptionsFromCommandLineArgs(List<String> listArgs,
            IKeyStoreClient keyStoreClient)
            throws ConfigurationException {
        // We get all the objects except the one describing the Configuration itself which does not
        // allow passing its option via command line.
        ArgsOptionParser parser =
                new ArgsOptionParser(
                        getAllConfigurationObjects(CONFIGURATION_DESCRIPTION_TYPE_NAME));
        if (keyStoreClient != null) {
            parser.setKeyStore(keyStoreClient);
        }
        try {
            return parser.parse(listArgs);
        } catch (ConfigurationException e) {
            if (!e.getMessage().contains(CONFIG_EXCEPTION_PATTERN)) {
                throw e;
            }
            String optionName = e.getMessage().split(CONFIG_EXCEPTION_PATTERN)[1];
            try {
                // In case the option exists in the config descriptor, we change the error message
                // to be more specific about why the option is rejected.
                OptionSetter setter = new OptionSetter(getConfigurationDescription());
                setter.getTypeForOption(optionName);
            } catch (ConfigurationException stillThrowing) {
                // Throw the original exception since it cannot be found at all.
                throw e;
            }
            throw new OptionNotAllowedException(
                    String.format(
                            "Option %s cannot be specified via "
                                    + "command line. Only in the configuration xml.",
                            optionName));
        }
    }

    /**
     * Outputs a command line usage help text for this configuration to given
     * printStream.
     *
     * @param out the {@link PrintStream} to use.
     * @throws ConfigurationException
     */
    @Override
    public void printCommandUsage(boolean importantOnly, PrintStream out)
            throws ConfigurationException {
        out.println(String.format("'%s' configuration: %s", getName(), getDescription()));
        out.println();
        if (importantOnly) {
            out.println("Printing help for only the important options. " +
                    "To see help for all options, use the --help-all flag");
            out.println();
        }
        for (Map.Entry<String, List<Object>> configObjectsEntry : mConfigMap.entrySet()) {
            for (Object configObject : configObjectsEntry.getValue()) {
                if (configObject instanceof IDeviceConfiguration) {
                    // We expand the Device Config Object.
                    for (Object subconfigObject : ((IDeviceConfiguration)configObject)
                            .getAllObjects()) {
                        printCommandUsageForObject(importantOnly, out, configObjectsEntry.getKey(),
                                subconfigObject);
                    }
                } else {
                    printCommandUsageForObject(importantOnly, out, configObjectsEntry.getKey(),
                            configObject);
                }
            }
        }
    }

    private void printCommandUsageForObject(boolean importantOnly, PrintStream out, String key,
            Object obj) throws ConfigurationException {
        String optionHelp = printOptionsForObject(importantOnly, key, obj);
        // only print help for object if optionHelp is non zero length
        if (optionHelp.length() > 0) {
            String classAlias = "";
            if (obj.getClass().isAnnotationPresent(OptionClass.class)) {
                final OptionClass classAnnotation = obj.getClass().getAnnotation(
                        OptionClass.class);
                classAlias = String.format("'%s' ", classAnnotation.alias());
            }
            out.printf("  %s%s options:", classAlias, key);
            out.println();
            out.print(optionHelp);
            out.println();
        }
    }

    /**
     * Get the JSON representation of a single {@link Option} field.
     */
    @SuppressWarnings({
            "unchecked", "rawtypes"
    })
    private JSONObject getOptionJson(Object optionObject, Field field) throws JSONException {
        // Build a JSON representation of the option
        JSONObject jsonOption = new JSONObject();

        // Store values from the @Option annotation
        Option option = field.getAnnotation(Option.class);
        jsonOption.put("name", option.name());
        if (option.shortName() != Option.NO_SHORT_NAME) {
            jsonOption.put("shortName", option.shortName());
        }
        jsonOption.put("description", option.description());
        jsonOption.put("importance", option.importance());
        jsonOption.put("mandatory", option.mandatory());
        jsonOption.put("isTimeVal", option.isTimeVal());
        jsonOption.put("updateRule", option.updateRule().name());

        // Store the field's class
        Type fieldType = field.getGenericType();
        if (fieldType instanceof ParameterizedType) {
            // Resolve paramaterized type arguments
            Type[] paramTypes = ((ParameterizedType) fieldType).getActualTypeArguments();
            String[] paramStrings = new String[paramTypes.length];
            for (int i = 0; i < paramTypes.length; i++) {
                paramStrings[i] = ((Class<?>) paramTypes[i]).getName();
            }

            jsonOption.put("javaClass", String.format("%s<%s>",
                    field.getType().getName(), Joiner.on(", ").join(paramStrings)));
        } else {
            jsonOption.put("javaClass", field.getType().getName());
        }

        // Store the field's value
        Object value = null;
        try {
            field.setAccessible(true);
            value = field.get(optionObject);

            // Convert nulls to JSONObject.NULL
            if (value == null) {
                jsonOption.put("value", JSONObject.NULL);
                // Convert MuliMap values to a JSON representation
            } else if (value instanceof MultiMap) {
                MultiMap multimap = (MultiMap) value;
                JSONObject jsonValue = new JSONObject();
                for (Object keyObj : multimap.keySet()) {
                    jsonValue.put(keyObj.toString(), multimap.get(keyObj));
                }
                jsonOption.put("value", jsonValue);
                // Convert Map values to JSON
            } else if (value instanceof Map) {
                jsonOption.put("value", new JSONObject((Map) value));
                // For everything else, just use the default representation
            } else {
                jsonOption.put("value", value);
            }
        } catch (IllegalAccessException e) {
            // Shouldn't happen
            throw new RuntimeException(e);
        }

        // Store the field's source
        // Maps and MultiMaps track sources per key, so use a JSONObject to
        // represent their sources
        if (Map.class.isAssignableFrom(field.getType())) {
            JSONObject jsonSourcesMap = new JSONObject();
            if (value != null) {
                // For each entry in the map, store the source as a JSONArray
                for (Object key : ((Map) value).keySet()) {
                    List<String> source = mFieldSources.get(new FieldDef(optionObject, field, key));
                    jsonSourcesMap.put(key.toString(), source == null ? new JSONArray() : source);
                }
            }
            jsonOption.put("source", jsonSourcesMap);

        } else if (MultiMap.class.isAssignableFrom(field.getType())) {
            JSONObject jsonSourcesMap = new JSONObject();
            if (value != null) {
                // For each entry in the map, store the sources as a JSONArray
                for (Object key : ((MultiMap) value).keySet()) {
                    List<String> source = mFieldSources.get(new FieldDef(optionObject, field, key));
                    jsonSourcesMap.put(key.toString(), source == null ? new JSONArray() : source);
                }
            }
            jsonOption.put("source", jsonSourcesMap);

            // Collections and regular objects only have one set of sources for
            // the whole field, so use
            // a JSONArray
        } else {
            List<String> source = mFieldSources.get(new FieldDef(optionObject, field, null));
            jsonOption.put("source", source == null ? new JSONArray() : source);
        }

        return jsonOption;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONArray getJsonCommandUsage() throws JSONException {
        JSONArray ret = new JSONArray();
        for (Map.Entry<String, List<Object>> configObjectsEntry : mConfigMap.entrySet()) {
            for (Object optionObject : configObjectsEntry.getValue()) {

                // Build a JSON representation of the current class
                JSONObject jsonClass = new JSONObject();
                jsonClass.put("name", configObjectsEntry.getKey());
                String alias = null;
                if (optionObject.getClass().isAnnotationPresent(OptionClass.class)) {
                    OptionClass optionClass = optionObject.getClass()
                            .getAnnotation(OptionClass.class);
                    alias = optionClass.alias();
                }
                jsonClass.put("alias", alias == null ? JSONObject.NULL : alias);
                jsonClass.put("class", optionObject.getClass().getName());

                // For each of the @Option annotated fields
                Collection<Field> optionFields = OptionSetter
                        .getOptionFieldsForClass(optionObject.getClass());
                JSONArray jsonOptions = new JSONArray();
                for (Field field : optionFields) {
                    // Add the JSON field representation to the JSON class
                    // representation
                    jsonOptions.put(getOptionJson(optionObject, field));
                }
                jsonClass.put("options", jsonOptions);

                // Add the JSON class representation to the list
                ret.put(jsonClass);
            }
        }

        return ret;
    }

    /**
     * Prints out the available config options for given configuration object.
     *
     * @param importantOnly print only the important options
     * @param objectTypeName the config object type name. Used to generate more
     *            descriptive error messages
     * @param configObject the config object
     * @return a {@link String} of option help text
     * @throws ConfigurationException
     */
    private String printOptionsForObject(boolean importantOnly, String objectTypeName,
            Object configObject) throws ConfigurationException {
        return ArgsOptionParser.getOptionHelp(importantOnly, configObject);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void validateOptions() throws ConfigurationException {
        new ArgsOptionParser(getAllConfigurationObjects()).validateMandatoryOptions();
        ICommandOptions options = getCommandOptions();
        if (options.getShardCount() != null && options.getShardCount() < 1) {
            throw new ConfigurationException("a shard count must be a positive number");
        }
        if (options.getShardIndex() != null
                && (options.getShardCount() == null || options.getShardIndex() < 0
                        || options.getShardIndex() >= options.getShardCount())) {
            throw new ConfigurationException("a shard index must be in range [0, shard count)");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dumpXml(PrintWriter output) throws IOException {
        dumpXml(output, new ArrayList<String>());
    }

    /** {@inheritDoc} */
    @Override
    public void dumpXml(PrintWriter output, List<String> excludeFilters) throws IOException {
        KXmlSerializer serializer = new KXmlSerializer();
        serializer.setOutput(output);
        serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
        serializer.startDocument("UTF-8", null);
        serializer.startTag(null, ConfigurationUtil.CONFIGURATION_NAME);

        for (IMultiTargetPreparer multiPreTargerPrep : getMultiPreTargetPreparers()) {
            ConfigurationUtil.dumpClassToXml(
                    serializer,
                    MULTI_PRE_TARGET_PREPARER_TYPE_NAME,
                    multiPreTargerPrep,
                    excludeFilters);
        }

        for (IMultiTargetPreparer multipreparer : getMultiTargetPreparers()) {
            ConfigurationUtil.dumpClassToXml(
                    serializer, MULTI_PREPARER_TYPE_NAME, multipreparer, excludeFilters);
        }

        if (getDeviceConfig().size() > 1) {
            // Handle multi device.
            for (IDeviceConfiguration deviceConfig : getDeviceConfig()) {
                serializer.startTag(null, Configuration.DEVICE_NAME);
                serializer.attribute(null, "name", deviceConfig.getDeviceName());
                ConfigurationUtil.dumpClassToXml(
                        serializer,
                        BUILD_PROVIDER_TYPE_NAME,
                        deviceConfig.getBuildProvider(),
                        excludeFilters);
                for (ITargetPreparer preparer : deviceConfig.getTargetPreparers()) {
                    ConfigurationUtil.dumpClassToXml(
                            serializer, TARGET_PREPARER_TYPE_NAME, preparer, excludeFilters);
                }
                ConfigurationUtil.dumpClassToXml(
                        serializer,
                        DEVICE_RECOVERY_TYPE_NAME,
                        deviceConfig.getDeviceRecovery(),
                        excludeFilters);
                ConfigurationUtil.dumpClassToXml(
                        serializer,
                        DEVICE_REQUIREMENTS_TYPE_NAME,
                        deviceConfig.getDeviceRequirements(),
                        excludeFilters);
                ConfigurationUtil.dumpClassToXml(
                        serializer,
                        DEVICE_OPTIONS_TYPE_NAME,
                        deviceConfig.getDeviceOptions(),
                        excludeFilters);
                serializer.endTag(null, Configuration.DEVICE_NAME);
            }
        } else {
            // Put single device tags
            ConfigurationUtil.dumpClassToXml(
                    serializer, BUILD_PROVIDER_TYPE_NAME, getBuildProvider(), excludeFilters);
            for (ITargetPreparer preparer : getTargetPreparers()) {
                ConfigurationUtil.dumpClassToXml(
                        serializer, TARGET_PREPARER_TYPE_NAME, preparer, excludeFilters);
            }
            ConfigurationUtil.dumpClassToXml(
                    serializer, DEVICE_RECOVERY_TYPE_NAME, getDeviceRecovery(), excludeFilters);
            ConfigurationUtil.dumpClassToXml(
                    serializer,
                    DEVICE_REQUIREMENTS_TYPE_NAME,
                    getDeviceRequirements(),
                    excludeFilters);
            ConfigurationUtil.dumpClassToXml(
                    serializer, DEVICE_OPTIONS_TYPE_NAME, getDeviceOptions(), excludeFilters);
        }
        for (IRemoteTest test : getTests()) {
            ConfigurationUtil.dumpClassToXml(serializer, TEST_TYPE_NAME, test, excludeFilters);
        }
        ConfigurationUtil.dumpClassToXml(
                serializer,
                CONFIGURATION_DESCRIPTION_TYPE_NAME,
                getConfigurationDescription(),
                excludeFilters);
        ConfigurationUtil.dumpClassToXml(
                serializer, LOGGER_TYPE_NAME, getLogOutput(), excludeFilters);
        ConfigurationUtil.dumpClassToXml(
                serializer, LOG_SAVER_TYPE_NAME, getLogSaver(), excludeFilters);
        for (ITestInvocationListener listener : getTestInvocationListeners()) {
            ConfigurationUtil.dumpClassToXml(
                    serializer, RESULT_REPORTER_TYPE_NAME, listener, excludeFilters);
        }
        ConfigurationUtil.dumpClassToXml(
                serializer, CMD_OPTIONS_TYPE_NAME, getCommandOptions(), excludeFilters);

        for (IMetricCollector collector : getMetricCollectors()) {
            ConfigurationUtil.dumpClassToXml(
                    serializer, DEVICE_METRICS_COLLECTOR_TYPE_NAME, collector, excludeFilters);
        }

        for (ISystemStatusChecker checker : getSystemStatusCheckers()) {
            ConfigurationUtil.dumpClassToXml(
                    serializer, SYSTEM_STATUS_CHECKER_TYPE_NAME, checker, excludeFilters);
        }

        ConfigurationUtil.dumpClassToXml(
                serializer,
                SANBOX_OPTIONS_TYPE_NAME,
                getConfigurationObject(SANBOX_OPTIONS_TYPE_NAME),
                excludeFilters);

        serializer.endTag(null, ConfigurationUtil.CONFIGURATION_NAME);
        serializer.endDocument();
    }
}
