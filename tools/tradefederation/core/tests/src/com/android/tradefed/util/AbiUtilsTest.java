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
 * limitations under the License
 */

package com.android.tradefed.util;

import static org.junit.Assert.assertEquals;

import org.junit.Assert;
import org.junit.Test;

import java.util.HashSet;
import java.util.Set;

/**
 * Unit tests for {@link AbiUtils}
 */
public class AbiUtilsTest {

    private static final String MODULE_NAME = "ModuleName";
    private static final String ABI_NAME = "mips64";
    private static final String ABI_FLAG = "--abi mips64 ";
    private static final String ABI_ID = "mips64 ModuleName";

    @Test
    public void testCreateAbiFlag() {
        String flag = AbiUtils.createAbiFlag(ABI_NAME);
        assertEquals("Incorrect flag created", ABI_FLAG, flag);
    }

    @Test
    public void testCreateAbiFlag_invalid() {
        assertEquals("", AbiUtils.createAbiFlag(null));
        assertEquals("", AbiUtils.createAbiFlag(""));
        assertEquals("", AbiUtils.createAbiFlag("NOT SUPPORTED"));
    }

    @Test
    public void testCreateId() {
        String id = AbiUtils.createId(ABI_NAME, MODULE_NAME);
        assertEquals("Incorrect id created", ABI_ID, id);
    }

    @Test
    public void testParseId() {
        String[] parts = AbiUtils.parseId(ABI_ID);
        assertEquals("Wrong size array", 2, parts.length);
        assertEquals("Wrong abi name", ABI_NAME, parts[0]);
        assertEquals("Wrong module name", MODULE_NAME, parts[1]);
    }

    @Test
    public void testParseId_invalid() {
        String[] parts = AbiUtils.parseId(null);
        assertEquals(2, parts.length);
        assertEquals("", parts[0]);
        assertEquals("", parts[1]);
        parts = AbiUtils.parseId("");
        assertEquals(2, parts.length);
        assertEquals("", parts[0]);
        assertEquals("", parts[1]);
    }

    @Test
    public void testParseName() {
        String name = AbiUtils.parseTestName(ABI_ID);
        assertEquals("Incorrect module name", MODULE_NAME, name);
    }

    @Test
    public void testParseAbi() {
        String abi = AbiUtils.parseAbi(ABI_ID);
        assertEquals("Incorrect abi name", ABI_NAME, abi);
    }

    @Test
    public void getAbiForArch() {
        assertEquals(AbiUtils.ARM_ABIS, AbiUtils.getAbisForArch(AbiUtils.BASE_ARCH_ARM));
        assertEquals(AbiUtils.ARM_ABIS, AbiUtils.getAbisForArch(AbiUtils.ARCH_ARM64));
    }

    @Test
    public void getAbiForArch_nullArch() {
        assertEquals(AbiUtils.ABIS_SUPPORTED_BY_COMPATIBILITY, AbiUtils.getAbisForArch(null));
        assertEquals(AbiUtils.ABIS_SUPPORTED_BY_COMPATIBILITY, AbiUtils.getAbisForArch(""));
        assertEquals(AbiUtils.ABIS_SUPPORTED_BY_COMPATIBILITY,
                AbiUtils.getAbisForArch("NOT EXIST"));
    }

    @Test
    public void getArchForAbi_emptyNull() {
        try {
            AbiUtils.getArchForAbi(null);
            Assert.fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            Assert.assertEquals("Abi cannot be null or empty", expected.getMessage());
        }
        try {
            AbiUtils.getArchForAbi("");
            Assert.fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            Assert.assertEquals("Abi cannot be null or empty", expected.getMessage());
        }
    }

    @Test
    public void getArchForAbi() {
        assertEquals(AbiUtils.BASE_ARCH_ARM, AbiUtils.getArchForAbi(AbiUtils.ABI_ARM_V7A));
        assertEquals(AbiUtils.ARCH_ARM64, AbiUtils.getArchForAbi(AbiUtils.ABI_ARM_64_V8A));
    }

    @Test
    public void getBaseArchForAbi_emptyNull() {
        try {
            AbiUtils.getBaseArchForAbi(null);
            Assert.fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            Assert.assertEquals("Abi cannot be null or empty", expected.getMessage());
        }
        try {
            AbiUtils.getBaseArchForAbi("");
            Assert.fail("Should have thrown an exception");
        } catch (IllegalArgumentException expected) {
            Assert.assertEquals("Abi cannot be null or empty", expected.getMessage());
        }
    }

    @Test
    public void getBaseArchForAbi() {
        assertEquals(AbiUtils.BASE_ARCH_ARM, AbiUtils.getBaseArchForAbi(AbiUtils.ABI_ARM_V7A));
        assertEquals(AbiUtils.BASE_ARCH_ARM, AbiUtils.getBaseArchForAbi(AbiUtils.ABI_ARM_64_V8A));
        assertEquals(AbiUtils.BASE_ARCH_X86, AbiUtils.getBaseArchForAbi(AbiUtils.ABI_X86));
        assertEquals(AbiUtils.BASE_ARCH_X86, AbiUtils.getBaseArchForAbi(AbiUtils.ABI_X86_64));
        assertEquals(AbiUtils.BASE_ARCH_MIPS, AbiUtils.getBaseArchForAbi(AbiUtils.ABI_MIPS));
        assertEquals(AbiUtils.BASE_ARCH_MIPS, AbiUtils.getBaseArchForAbi(AbiUtils.ABI_MIPS64));
    }


    @Test
    public void testParseFromProperty() {
        Set<String> abiSet = new HashSet<>();
        abiSet.add("arm64-v8a");
        abiSet.add("armeabi-v7a");
        // armeabi will not appear as it is not supported by compatibility.
        assertEquals(abiSet, AbiUtils.parseAbiListFromProperty("arm64-v8a,armeabi-v7a,armeabi"));
    }

    @Test
    public void testGetBitness() {
        assertEquals("32", AbiUtils.getBitness(AbiUtils.ABI_ARM_V7A));
        assertEquals("64", AbiUtils.getBitness(AbiUtils.ABI_ARM_64_V8A));
    }

    @Test
    public void testParseAbiList() {
        Set<String> abiSet = new HashSet<>();
        abiSet.add("arm64-v8a");
        abiSet.add("armeabi-v7a");
        // armeabi will not appear as it is not supported by compatibility.
        assertEquals(abiSet, AbiUtils.parseAbiList("list1:arm64-v8a,armeabi-v7a,armeabi"));
    }

    @Test
    public void testParseAbiList_invalidArg() {
        Set<String> abiSet = new HashSet<>();
        assertEquals(abiSet, AbiUtils.parseAbiList("BAD FORMAT"));
    }
}