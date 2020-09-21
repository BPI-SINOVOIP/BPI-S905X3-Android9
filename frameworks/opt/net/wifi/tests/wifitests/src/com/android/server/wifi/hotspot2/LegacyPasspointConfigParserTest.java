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

package com.android.server.wifi.hotspot2;

import static org.junit.Assert.*;

import android.os.FileUtils;
import android.support.test.filters.SmallTest;

import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * Unit tests for {@link com.android.server.wifi.hotspot2.LegacyPasspointConfigParser}.
 */
@SmallTest
public class LegacyPasspointConfigParserTest {
    private static final String TEST_CONFIG =
            "tree 3:1.2(urn:wfa:mo:hotspot2dot0-perprovidersubscription:1.0)\n"
                    + "8:MgmtTree+\n"
                    + "17:PerProviderSubscription+\n"
                    + "4:r1i1+\n"
                    + "6:HomeSP+\n"
                    + "c:FriendlyName=12:Test Provider 1™\n"
                    + "4:FQDN=9:test1.net\n"
                    + "13:RoamingConsortiumOI=9:1234,5678\n"
                    + ".\n"
                    + "a:Credential+\n"
                    + "10:UsernamePassword+\n"
                    + "8:Username=5:user1\n"
                    + "8:Password=5:pass1\n"
                    + "\n"
                    + "9:EAPMethod+\n"
                    + "7:EAPType=2:21\n"
                    + "b:InnerMethod=3:PAP\n"
                    + ".\n"
                    + ".\n"
                    + "5:Realm=9:test1.com\n"
                    + ".\n"
                    + ".\n"
                    + ".\n"
                    + "17:PerProviderSubscription+\n"
                    + "4:r1i2+\n"
                    + "6:HomeSP+\n"
                    + "c:FriendlyName=f:Test Provider 2\n"
                    + "4:FQDN=9:test2.net\n"
                    + ".\n"
                    + "a:Credential+\n"
                    + "3:SIM+\n"
                    + "4:IMSI=4:1234\n"
                    + "7:EAPType=2:18\n"
                    + ".\n"
                    + "5:Realm=9:test2.com\n"
                    + ".\n"
                    + ".\n"
                    + ".\n"
                    + ".\n";

    /**
     * Helper function for generating {@link LegacyPasspointConfig} objects based on the predefined
     * test configuration string {@link #TEST_CONFIG}
     *
     * @return Map of FQDN to {@link LegacyPasspointConfig}
     */
    private Map<String, LegacyPasspointConfig> generateTestConfig() {
        Map<String, LegacyPasspointConfig> configs = new HashMap<>();

        LegacyPasspointConfig config1 = new LegacyPasspointConfig();
        config1.mFqdn = "test1.net";
        config1.mFriendlyName = "Test Provider 1™";
        config1.mRoamingConsortiumOis = new long[] {0x1234, 0x5678};
        config1.mRealm = "test1.com";
        configs.put("test1.net", config1);

        LegacyPasspointConfig config2 = new LegacyPasspointConfig();
        config2.mFqdn = "test2.net";
        config2.mFriendlyName = "Test Provider 2";
        config2.mRealm = "test2.com";
        config2.mImsi = "1234";
        configs.put("test2.net", config2);

        return configs;
    }

    /**
     * Helper function for parsing configuration data.
     *
     * @param data The configuration data to parse
     * @return Map of FQDN to {@link LegacyPasspointConfig}
     * @throws Exception
     */
    private Map<String, LegacyPasspointConfig> parseConfig(String data) throws Exception {
        // Write configuration data to file.
        File configFile = File.createTempFile("LegacyPasspointConfig", "");
        FileUtils.stringToFile(configFile, data);

        // Parse the configuration file.
        LegacyPasspointConfigParser parser = new LegacyPasspointConfigParser();
        Map<String, LegacyPasspointConfig> configMap =
                parser.parseConfig(configFile.getAbsolutePath());

        configFile.delete();
        return configMap;
    }

    /**
     * Verify that the expected {@link LegacyPasspointConfig} objects are return when parsing
     * predefined test configuration data {@link #TEST_CONFIG}.
     *
     * @throws Exception
     */
    @Test
    public void parseTestConfig() throws Exception {
        Map<String, LegacyPasspointConfig> parsedConfig = parseConfig(TEST_CONFIG);
        assertEquals(generateTestConfig(), parsedConfig);
    }

    /**
     * Verify that an empty map is return when parsing a configuration containing an empty
     * configuration (MgmtTree).
     *
     * @throws Exception
     */
    @Test
    public void parseEmptyConfig() throws Exception {
        String emptyConfig = "tree 3:1.2(urn:wfa:mo:hotspot2dot0-perprovidersubscription:1.0)\n"
                + "8:MgmtTree+\n"
                + ".\n";
        Map<String, LegacyPasspointConfig> parsedConfig = parseConfig(emptyConfig);
        assertTrue(parsedConfig.isEmpty());
    }

    /**
     * Verify that an empty map is return when parsing an empty configuration data.
     *
     * @throws Exception
     */
    @Test
    public void parseEmptyData() throws Exception {
        Map<String, LegacyPasspointConfig> parsedConfig = parseConfig("");
        assertTrue(parsedConfig.isEmpty());
    }

    /**
     * Verify that an IOException is thrown when parsing a configuration containing an unknown
     * root name.  The expected root name is "MgmtTree".
     *
     * @throws Exception
     */
    @Test(expected = IOException.class)
    public void parseConfigWithUnknownRootName() throws Exception {
        String config = "tree 3:1.2(urn:wfa:mo:hotspot2dot0-perprovidersubscription:1.0)\n"
                + "8:TestTest+\n"
                + ".\n";
        parseConfig(config);
    }

    /**
     * Verify that an IOException is thrown when parsing a configuration containing a line with
     * mismatched string length for the name.
     *
     * @throws Exception
     */
    @Test(expected = IOException.class)
    public void parseConfigWithMismatchedStringLengthInName() throws Exception {
        String config = "tree 3:1.2(urn:wfa:mo:hotspot2dot0-perprovidersubscription:1.0)\n"
                + "9:MgmtTree+\n"
                + ".\n";
        parseConfig(config);
    }

    /**
     * Verify that an IOException is thrown when parsing a configuration containing a line with
     * mismatched string length for the value.
     *
     * @throws Exception
     */
    @Test(expected = IOException.class)
    public void parseConfigWithMismatchedStringLengthInValue() throws Exception {
        String config = "tree 3:1.2(urn:wfa:mo:hotspot2dot0-perprovidersubscription:1.0)\n"
                + "8:MgmtTree+\n"
                + "4:test=5:test\n"
                + ".\n";
        parseConfig(config);
    }
}
