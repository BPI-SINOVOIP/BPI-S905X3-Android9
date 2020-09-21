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

package com.android.server.wifi;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.test.MockAnswerUtil.AnswerWithArguments;
import android.net.IpConfiguration;
import android.net.wifi.WifiConfiguration;
import android.support.test.filters.SmallTest;
import android.text.TextUtils;
import android.util.SparseArray;

import com.android.server.wifi.WifiConfigStoreLegacy.IpConfigStoreWrapper;
import com.android.server.wifi.hotspot2.LegacyPasspointConfig;
import com.android.server.wifi.hotspot2.LegacyPasspointConfigParser;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link com.android.server.wifi.WifiConfigStoreLegacy}.
 */
@SmallTest
public class WifiConfigStoreLegacyTest {
    private static final String MASKED_FIELD_VALUE = "*";

    // Test mocks
    @Mock private WifiNative mWifiNative;
    @Mock private WifiNetworkHistory mWifiNetworkHistory;
    @Mock private IpConfigStoreWrapper mIpconfigStoreWrapper;
    @Mock private LegacyPasspointConfigParser mPasspointConfigParser;

    /**
     * Test instance of WifiConfigStore.
     */
    private WifiConfigStoreLegacy mWifiConfigStore;


    /**
     * Setup the test environment.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mWifiConfigStore = new WifiConfigStoreLegacy(mWifiNetworkHistory, mWifiNative,
                mIpconfigStoreWrapper, mPasspointConfigParser);
    }

    /**
     * Called after each test
     */
    @After
    public void cleanup() {
        validateMockitoUsage();
    }

    /**
     * Verify loading of network configurations from legacy stores. This is verifying the population
     * of the masked wpa_supplicant fields using wpa_supplicant.conf file.
     */
    @Test
    public void testLoadFromStores() throws Exception {
        WifiConfiguration pskNetwork = WifiConfigurationTestUtil.createPskNetwork();
        WifiConfiguration wepNetwork = WifiConfigurationTestUtil.createWepNetwork();
        WifiConfiguration eapNetwork = WifiConfigurationTestUtil.createEapNetwork();
        WifiConfiguration passpointNetwork = WifiConfigurationTestUtil.createPasspointNetwork();
        eapNetwork.enterpriseConfig.setPassword("EapPassword");

        // Initialize Passpoint configuration data.
        int passpointNetworkId = 1234;
        String fqdn = passpointNetwork.FQDN;
        String providerFriendlyName = passpointNetwork.providerFriendlyName;
        long[] roamingConsortiumIds = new long[] {0x1234, 0x5678};
        String realm = "test.com";
        String imsi = "214321";

        // Update Passpoint network.
        // Network ID is used for lookup network extras, so use an unique ID for passpoint network.
        passpointNetwork.networkId = passpointNetworkId;
        passpointNetwork.enterpriseConfig.setPassword("PaspointPassword");
        // Reset FQDN and provider friendly name so that the derived network from #read will
        // obtained these information from networkExtras and {@link LegacyPasspointConfigParser}.
        passpointNetwork.FQDN = null;
        passpointNetwork.providerFriendlyName = null;

        final List<WifiConfiguration> networks = new ArrayList<>();
        networks.add(pskNetwork);
        networks.add(wepNetwork);
        networks.add(eapNetwork);
        networks.add(passpointNetwork);

        // Setup legacy Passpoint configuration data.
        Map<String, LegacyPasspointConfig> passpointConfigs = new HashMap<>();
        LegacyPasspointConfig passpointConfig = new LegacyPasspointConfig();
        passpointConfig.mFqdn = fqdn;
        passpointConfig.mFriendlyName = providerFriendlyName;
        passpointConfig.mRoamingConsortiumOis = roamingConsortiumIds;
        passpointConfig.mRealm = realm;
        passpointConfig.mImsi = imsi;
        passpointConfigs.put(fqdn, passpointConfig);

        // Return the config data with passwords masked from wpa_supplicant control interface.
        doAnswer(new AnswerWithArguments() {
            public boolean answer(String ifaceName, Map<String, WifiConfiguration> configs,
                    SparseArray<Map<String, String>> networkExtras) {
                for (Map.Entry<String, WifiConfiguration> entry:
                        createWpaSupplicantLoadData(networks).entrySet()) {
                    configs.put(entry.getKey(), entry.getValue());
                }
                // Setup networkExtras for Passpoint configuration.
                networkExtras.put(passpointNetworkId, createNetworkExtrasForPasspointConfig(fqdn));
                return true;
            }
        }).when(mWifiNative).migrateNetworksFromSupplicant(
                any(), any(Map.class), any(SparseArray.class));

        when(mPasspointConfigParser.parseConfig(anyString())).thenReturn(passpointConfigs);
        when(mIpconfigStoreWrapper.readIpAndProxyConfigurations(anyString()))
                .thenReturn(createIpConfigStoreLoadData(networks));
        WifiConfigStoreLegacy.WifiConfigStoreDataLegacy storeData = mWifiConfigStore.read();

        // Update the expected configuration for Passpoint network.
        passpointNetwork.isLegacyPasspointConfig = true;
        passpointNetwork.FQDN = fqdn;
        passpointNetwork.providerFriendlyName = providerFriendlyName;
        passpointNetwork.roamingConsortiumIds = roamingConsortiumIds;
        passpointNetwork.enterpriseConfig.setRealm(realm);
        passpointNetwork.enterpriseConfig.setPlmn(imsi);

        WifiConfigurationTestUtil.assertConfigurationsEqualForConfigStore(
                networks, storeData.getConfigurations());
    }

    /**
     * Verify loading of network configurations from legacy stores with a null configKey.
     * This happens if the 'ID_STR' field in wpa_supplicant.conf is not populated for some reason.
     */
    @Test
    public void testLoadFromStoresWithNullConfigKey() throws Exception {
        WifiConfiguration pskNetwork = WifiConfigurationTestUtil.createPskNetwork();
        WifiConfiguration wepNetwork = WifiConfigurationTestUtil.createWepNetwork();
        WifiConfiguration eapNetwork = WifiConfigurationTestUtil.createEapNetwork();
        eapNetwork.enterpriseConfig.setPassword("EapPassword");

        final List<WifiConfiguration> networks = new ArrayList<>();
        networks.add(pskNetwork);
        networks.add(wepNetwork);
        networks.add(eapNetwork);

        // Return the config data with null configKey.
        doAnswer(new AnswerWithArguments() {
            public boolean answer(String ifaceName, Map<String, WifiConfiguration> configs,
                                  SparseArray<Map<String, String>> networkExtras) {
                for (Map.Entry<String, WifiConfiguration> entry:
                        createWpaSupplicantLoadData(networks).entrySet()) {
                    configs.put(null, entry.getValue());
                }
                return true;
            }
        }).when(mWifiNative).migrateNetworksFromSupplicant(
                any(), any(Map.class), any(SparseArray.class));

        when(mIpconfigStoreWrapper.readIpAndProxyConfigurations(anyString()))
                .thenReturn(createIpConfigStoreLoadData(networks));
        assertNotNull(mWifiConfigStore.read());
    }

    private SparseArray<IpConfiguration> createIpConfigStoreLoadData(
            List<WifiConfiguration> configurations) {
        SparseArray<IpConfiguration> newIpConfigurations = new SparseArray<>();
        for (WifiConfiguration config : configurations) {
            newIpConfigurations.put(
                    config.configKey().hashCode(),
                    new IpConfiguration(config.getIpConfiguration()));
        }
        return newIpConfigurations;
    }

    private Map<String, String> createPskMap(List<WifiConfiguration> configurations) {
        Map<String, String> pskMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            if (!TextUtils.isEmpty(config.preSharedKey)) {
                pskMap.put(config.configKey(), config.preSharedKey);
            }
        }
        return pskMap;
    }

    private Map<String, String> createWepKey0Map(List<WifiConfiguration> configurations) {
        Map<String, String> wepKeyMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            if (!TextUtils.isEmpty(config.wepKeys[0])) {
                wepKeyMap.put(config.configKey(), config.wepKeys[0]);
            }
        }
        return wepKeyMap;
    }

    private Map<String, String> createWepKey1Map(List<WifiConfiguration> configurations) {
        Map<String, String> wepKeyMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            if (!TextUtils.isEmpty(config.wepKeys[1])) {
                wepKeyMap.put(config.configKey(), config.wepKeys[1]);
            }
        }
        return wepKeyMap;
    }

    private Map<String, String> createWepKey2Map(List<WifiConfiguration> configurations) {
        Map<String, String> wepKeyMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            if (!TextUtils.isEmpty(config.wepKeys[2])) {
                wepKeyMap.put(config.configKey(), config.wepKeys[2]);
            }
        }
        return wepKeyMap;
    }

    private Map<String, String> createWepKey3Map(List<WifiConfiguration> configurations) {
        Map<String, String> wepKeyMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            if (!TextUtils.isEmpty(config.wepKeys[3])) {
                wepKeyMap.put(config.configKey(), config.wepKeys[3]);
            }
        }
        return wepKeyMap;
    }

    private Map<String, String> createEapPasswordMap(List<WifiConfiguration> configurations) {
        Map<String, String> eapPasswordMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            if (!TextUtils.isEmpty(config.enterpriseConfig.getPassword())) {
                eapPasswordMap.put(config.configKey(), config.enterpriseConfig.getPassword());
            }
        }
        return eapPasswordMap;
    }

    private Map<String, WifiConfiguration> createWpaSupplicantLoadData(
            List<WifiConfiguration> configurations) {
        Map<String, WifiConfiguration> configurationMap = new HashMap<>();
        for (WifiConfiguration config : configurations) {
            configurationMap.put(config.configKey(true), config);
        }
        return configurationMap;
    }

    private List<WifiConfiguration> createMaskedWifiConfigurations(
            List<WifiConfiguration> configurations) {
        List<WifiConfiguration> newConfigurations = new ArrayList<>();
        for (WifiConfiguration config : configurations) {
            newConfigurations.add(createMaskedWifiConfiguration(config));
        }
        return newConfigurations;
    }

    private WifiConfiguration createMaskedWifiConfiguration(WifiConfiguration configuration) {
        WifiConfiguration newConfig = new WifiConfiguration(configuration);
        if (!TextUtils.isEmpty(configuration.preSharedKey)) {
            newConfig.preSharedKey = MASKED_FIELD_VALUE;
        }
        if (!TextUtils.isEmpty(configuration.wepKeys[0])) {
            newConfig.wepKeys[0] = MASKED_FIELD_VALUE;
        }
        if (!TextUtils.isEmpty(configuration.wepKeys[1])) {
            newConfig.wepKeys[1] = MASKED_FIELD_VALUE;
        }
        if (!TextUtils.isEmpty(configuration.wepKeys[2])) {
            newConfig.wepKeys[2] = MASKED_FIELD_VALUE;
        }
        if (!TextUtils.isEmpty(configuration.wepKeys[3])) {
            newConfig.wepKeys[3] = MASKED_FIELD_VALUE;
        }
        if (!TextUtils.isEmpty(configuration.enterpriseConfig.getPassword())) {
            newConfig.enterpriseConfig.setPassword(MASKED_FIELD_VALUE);
        }
        return newConfig;
    }

    private Map<String, String> createNetworkExtrasForPasspointConfig(String fqdn) {
        Map<String, String> extras = new HashMap<>();
        extras.put(SupplicantStaNetworkHal.ID_STRING_KEY_FQDN, fqdn);
        return extras;
    }
}
