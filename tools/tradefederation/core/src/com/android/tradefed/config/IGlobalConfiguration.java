/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.tradefed.command.ICommandScheduler;
import com.android.tradefed.device.DeviceManager;
import com.android.tradefed.device.IDeviceManager;
import com.android.tradefed.device.IDeviceMonitor;
import com.android.tradefed.device.IDeviceSelection;
import com.android.tradefed.device.IMultiDeviceRecovery;
import com.android.tradefed.host.IHostOptions;
import com.android.tradefed.invoker.shard.IShardHelper;
import com.android.tradefed.log.ITerribleFailureHandler;
import com.android.tradefed.util.hostmetric.IHostMonitor;
import com.android.tradefed.util.keystore.IKeyStoreFactory;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * A class to encompass global configuration information for a single Trade Federation instance
 * (encompassing any number of invocations of actual configurations).
 */
public interface IGlobalConfiguration {
    /**
     * Gets the {@link IHostOptions} to use from the configuration.
     *
     * @return the {@link IDeviceManager} provided in the configuration.
     */
    public IHostOptions getHostOptions();

    /**
     * Gets the list of {@link IDeviceMonitor} from the global config.
     *
     * @return the list of {@link IDeviceMonitor} from the global config, or <code>null</code> if none
     *         was specified.
     */
    public List<IDeviceMonitor> getDeviceMonitors();

    /**
     * Gets the list of {@link IHostMonitor} from the global config.
     *
     * @return the list of {@link IHostMonitor} from the global config, or <code>null</code> if none
     *         was specified.
     */
    public List<IHostMonitor> getHostMonitors();

    /**
     * Set the {@link IDeviceMonitor}.
     *
     * @param deviceMonitor The monitor
     * @throws ConfigurationException if an {@link IDeviceMonitor} has already been set.
     */
    public void setDeviceMonitor(IDeviceMonitor deviceMonitor) throws ConfigurationException;

    /**
     * Set the {@link IHostMonitor} list.
     *
     * @param hostMonitors The list of monitors
     * @throws ConfigurationException if an {@link IHostMonitor} has already been set.
     */
    public void setHostMonitors(List<IHostMonitor> hostMonitors) throws ConfigurationException;

    /**
     * Set the {@link ITerribleFailureHandler}.
     *
     * @param wtfHandler the WTF handler
     * @throws ConfigurationException if an {@link ITerribleFailureHandler} has
     *             already been set.
     */
    public void setWtfHandler(ITerribleFailureHandler wtfHandler) throws ConfigurationException;

    /**
     * Generic method to set the config object list for the given name, replacing any existing
     * value.
     *
     * @param typeName the unique name of the config object type.
     * @param configList the config object list
     * @throws ConfigurationException if any objects in the list are not the correct type
     */
    public void setConfigurationObjectList(String typeName, List<?> configList)
            throws ConfigurationException;

    /**
     * Inject a option value into the set of configuration objects.
     * <p/>
     * Useful to provide values for options that are generated dynamically.
     *
     * @param optionName the option name
     * @param optionValue the option value(s)
     * @throws ConfigurationException if failed to set the option's value
     */
    public void injectOptionValue(String optionName, String optionValue)
            throws ConfigurationException;

    /**
     * Inject a option value into the set of configuration objects.
     * <p/>
     * Useful to provide values for options that are generated dynamically.
     *
     * @param optionName the map option name
     * @param optionKey the map option key
     * @param optionValue the map option value
     * @throws ConfigurationException if failed to set the option's value
     */
    public void injectOptionValue(String optionName, String optionKey, String optionValue)
            throws ConfigurationException;

    /**
     * Get a list of option's values.
     *
     * @param optionName the map option name
     * @return a list of the given option's values. <code>null</code> if the option name does not
     *          exist.
     */
    public List<String> getOptionValues(String optionName);

    /**
     * Set the global config {@link Option} fields with given set of command line arguments
     * <p/>
     * See {@link ArgsOptionParser} for expected format
     *
     * @param listArgs the command line arguments
     * @return the unconsumed arguments
     */
    public List<String> setOptionsFromCommandLineArgs(List<String> listArgs)
            throws ConfigurationException;

    /**
     * Set the {@link IDeviceSelection}, replacing any existing values.  This sets a global device
     * filter on which devices the {@link DeviceManager} can see.
     *
     * @param deviceSelection
     */
    public void setDeviceRequirements(IDeviceSelection deviceSelection);

    /**
     * Gets the {@link IDeviceSelection} to use from the configuration.  Represents a global filter
     * on which devices the {@link DeviceManager} can see.
     *
     * @return the {@link IDeviceSelection} provided in the configuration.
     */
    public IDeviceSelection getDeviceRequirements();

    /**
     * Gets the {@link IDeviceManager} to use from the configuration. Manages the set of available
     * devices for testing
     *
     * @return the {@link IDeviceManager} provided in the configuration.
     */
    public IDeviceManager getDeviceManager();

    /**
     * Gets the {@link ITerribleFailureHandler} to use from the configuration.
     * Handles what to do in the event that a WTF (What a Terrible Failure)
     * occurs.
     *
     * @return the {@link ITerribleFailureHandler} provided in the
     *         configuration, or null if no handler is set
     */
    public ITerribleFailureHandler getWtfHandler();

    /**
     * Gets the {@link ICommandScheduler} to use from the configuration.
     *
     * @return the {@link ICommandScheduler}. Will never return null.
     */
    public ICommandScheduler getCommandScheduler();

    /**
     * Gets the list of {@link IMultiDeviceRecovery} to use from the configuration.
     *
     * @return the list of {@link IMultiDeviceRecovery}, or <code>null</code> if not set.
     */
    public List<IMultiDeviceRecovery> getMultiDeviceRecoveryHandlers();


    /**
     * Gets the {@link IKeyStoreFactory} to use from the configuration.
     *
     * @return the {@link IKeyStoreFactory} or null if no key store factory is set.
     */
    public IKeyStoreFactory getKeyStoreFactory();

    /** Returns the {@link IShardHelper} that defines the way to shard a configuration. */
    public IShardHelper getShardingStrategy();

    /**
     * Set the {@link IHostOptions}, replacing any existing values.
     *
     * @param hostOptions
     */
    public void setHostOptions(IHostOptions hostOptions);

    /**
     * Set the {@link IDeviceManager}, replacing any existing values. This sets the manager
     * for the test devices
     *
     * @param deviceManager
     */
    public void setDeviceManager(IDeviceManager deviceManager);

    /**
     * Set the {@link ICommandScheduler}, replacing any existing values.
     *
     * @param scheduler
     */
    public void setCommandScheduler(ICommandScheduler scheduler);


    /**
     * Set the {@link IKeyStoreFactory}, replacing any existing values.
     *
     * @param factory
     */
    public void setKeyStoreFactory(IKeyStoreFactory factory);

    /** Sets the {@link IShardHelper} to be used when sharding a configuration. */
    public void setShardingStrategy(IShardHelper sharding);

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
     * Gets the custom configuration object with given name.
     *
     * @param typeName the unique type of the configuration object
     * @return the object or null if object with that name is not found
     */
    public Object getConfigurationObject(String typeName);

    /**
     * Gets global config server. Global config server is used to get host configs from a server
     * instead of getting it from local files.
     */
    public IConfigurationServer getGlobalConfigServer();

    /**
     * Validate option values.
     * <p/>
     * Currently this will just validate that all mandatory options have been set
     *
     * @throws ConfigurationException if configuration is missing mandatory fields
     */
    public void validateOptions() throws ConfigurationException;

    /**
     * Filter the GlobalConfiguration based on a white list and output to an XML file.
     *
     * <p>For example, for following configuration:
     * {@code
     * <xml>
     *     <configuration>
     *         <device_monitor class="com.android.tradefed.device.DeviceMonitorMultiplexer" />
     *         <wtf_handler class="com.android.tradefed.log.TerribleFailureEmailHandler" />
     *         <key_store class="com.android.tradefed.util.keystore.JSONFileKeyStoreFactory" />
     *     </configuration>
     * </xml>
     * }
     *
     * <p>all config except "key_store" will be filtered out, and result a config file with
     * following content:
     * {@code
     * <xml>
     *     <configuration>
     *         <key_store class="com.android.tradefed.util.keystore.JSONFileKeyStoreFactory" />
     *     </configuration>
     * </xml>
     * }
     *
     * @param outputXml the XML file to write to
     * @param whitelistConfigs a {@link String} array of configs to be included in the new XML file.
     *     If it's set to <code>null<code/>, a default list should be used.
     * @throws IOException
     */
    public void cloneConfigWithFilter(File outputXml, String[] whitelistConfigs) throws IOException;
}
