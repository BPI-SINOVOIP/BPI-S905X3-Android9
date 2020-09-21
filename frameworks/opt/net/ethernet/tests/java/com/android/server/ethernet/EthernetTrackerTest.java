/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.server.ethernet;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.LinkAddress;
import android.net.StaticIpConfiguration;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.net.InetAddress;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class EthernetTrackerTest {

    @Test
    public void createStaticIpConfiguration() {
        assertStaticConfiguration("", new StaticIpConfiguration());

        assertStaticConfiguration(
                "ip=192.0.2.10/24",
                new StaticIpConfigBuilder().setIp("192.0.2.10/24").build());

        assertStaticConfiguration(
                "ip=192.0.2.10/24 dns=4.4.4.4,8.8.8.8 gateway=192.0.2.1 domains=android",
                new StaticIpConfigBuilder()
                        .setIp("192.0.2.10/24")
                        .setDns(new String[] {"4.4.4.4", "8.8.8.8"})
                        .setGateway("192.0.2.1")
                        .setDomains("android")
                        .build());

        // Verify order doesn't matter
        assertStaticConfiguration(
                "domains=android ip=192.0.2.10/24 gateway=192.0.2.1 dns=4.4.4.4,8.8.8.8 ",
                new StaticIpConfigBuilder()
                        .setIp("192.0.2.10/24")
                        .setDns(new String[] {"4.4.4.4", "8.8.8.8"})
                        .setGateway("192.0.2.1")
                        .setDomains("android")
                        .build());
    }

    @Test
    public void createStaticIpConfiguration_Bad() {
        assertFails("ip=192.0.2.1/24 gateway= blah=20.20.20.20");  // Unknown key
        assertFails("ip=192.0.2.1");  // mask is missing
        assertFails("ip=a.b.c");  // not a valid ip address
        assertFails("dns=4.4.4.4,1.2.3.A");  // not valid ip address in dns
        assertFails("=");  // Key and value is empty
        assertFails("ip=");  // Value is empty
        assertFails("ip=192.0.2.1/24 gateway=");  // Gateway is empty
    }

    private void assertFails(String config) {
        try {
            EthernetTracker.parseStaticIpConfiguration(config);
            fail("Expected to fail: " + config);
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    private void assertStaticConfiguration(String configAsString,
            StaticIpConfiguration expectedStaticIpConfig) {
        IpConfiguration expectedIpConfiguration = new IpConfiguration(IpAssignment.STATIC,
                ProxySettings.NONE, expectedStaticIpConfig, null);

        assertEquals(expectedIpConfiguration,
                EthernetTracker.parseStaticIpConfiguration(configAsString));
    }

    private static class StaticIpConfigBuilder {
        private final StaticIpConfiguration config = new StaticIpConfiguration();

        StaticIpConfigBuilder setIp(String address) {
            config.ipAddress = new LinkAddress(address);
            return this;
        }

        StaticIpConfigBuilder setDns(String[] dnsArray) {
            for (String dns : dnsArray) {
                config.dnsServers.add(InetAddress.parseNumericAddress(dns));
            }
            return this;
        }

        StaticIpConfigBuilder setGateway(String gateway) {
            config.gateway = InetAddress.parseNumericAddress(gateway);
            return this;
        }

        StaticIpConfigBuilder setDomains(String domains) {
            config.domains = domains;
            return this;
        }


        StaticIpConfiguration build() {
            return new StaticIpConfiguration(config);
        }
    }
}
