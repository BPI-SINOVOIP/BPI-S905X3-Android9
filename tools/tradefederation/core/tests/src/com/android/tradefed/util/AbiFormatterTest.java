/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.util;

import com.android.tradefed.device.ITestDevice;
import junit.framework.TestCase;
import org.easymock.EasyMock;

/**
 * Unit tests for the {@link AbiFormatter} utility class
 */
public class AbiFormatterTest extends TestCase {
    /**
     * Verify that {@link AbiFormatter#formatCmdForAbi} works as expected.
     */
    public void testFormatCmdForAbi() throws Exception {
        String a = "test foo|#ABI#| bar|#ABI32#| foobar|#ABI64#|";
        // if abi is null, remove all place holders.
        assertEquals("test foo bar foobar", AbiFormatter.formatCmdForAbi(a, null));
        // if abi is "", remove all place holders.
        assertEquals("test foo bar foobar", AbiFormatter.formatCmdForAbi(a, ""));
        // if abi is 32
        assertEquals("test foo32 bar foobar32", AbiFormatter.formatCmdForAbi(a, "32"));
        // if abi is 64
        assertEquals("test foo64 bar64 foobar", AbiFormatter.formatCmdForAbi(a, "64"));
        // test null input string
        assertNull(AbiFormatter.formatCmdForAbi(null, "32"));
    }

    /**
     * Verify that {@link AbiFormatter#getDefaultAbi} works as expected.
     */
    public void testGetDefaultAbi() throws Exception {
        ITestDevice device = EasyMock.createMock(ITestDevice.class);

        EasyMock.expect(device.getProperty("ro.product.cpu.abilist32")).andReturn(null);
        EasyMock.expect(device.getProperty("ro.product.cpu.abi")).andReturn(null);
        EasyMock.replay(device);
        assertEquals(null, AbiFormatter.getDefaultAbi(device, "32"));

        EasyMock.reset(device);
        EasyMock.expect(device.getProperty(EasyMock.eq("ro.product.cpu.abilist32")))
                .andReturn("abi,abi2,abi3");
        EasyMock.replay(device);
        assertEquals("abi", AbiFormatter.getDefaultAbi(device, "32"));

        EasyMock.reset(device);
        EasyMock.expect(device.getProperty(EasyMock.eq("ro.product.cpu.abilist64"))).andReturn("");
        EasyMock.expect(device.getProperty("ro.product.cpu.abi")).andReturn(null);
        EasyMock.replay(device);
        assertEquals(null, AbiFormatter.getDefaultAbi(device, "64"));
    }

    /**
     * Verify that {@link AbiFormatter#getSupportedAbis} works as expected.
     */
    public void testGetSupportedAbis() throws Exception {
        ITestDevice device = EasyMock.createMock(ITestDevice.class);

        EasyMock.expect(device.getProperty("ro.product.cpu.abilist32")).andReturn("abi1,abi2");
        EasyMock.replay(device);
        String[] supportedAbiArray = AbiFormatter.getSupportedAbis(device, "32");
        assertEquals("abi1", supportedAbiArray[0]);
        assertEquals("abi2", supportedAbiArray[1]);

        EasyMock.reset(device);
        EasyMock.expect(device.getProperty("ro.product.cpu.abilist32")).andReturn(null);
        EasyMock.expect(device.getProperty("ro.product.cpu.abi")).andReturn("abi");
        EasyMock.replay(device);
        supportedAbiArray = AbiFormatter.getSupportedAbis(device, "32");
        assertEquals("abi", supportedAbiArray[0]);

        EasyMock.reset(device);
        EasyMock.expect(device.getProperty("ro.product.cpu.abilist")).andReturn("");
        EasyMock.expect(device.getProperty("ro.product.cpu.abi")).andReturn("abi");
        EasyMock.replay(device);
        supportedAbiArray = AbiFormatter.getSupportedAbis(device, "");
        assertEquals("abi", supportedAbiArray[0]);
    }
}
