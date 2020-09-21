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
 * limitations under the License
 */

package com.android.server.pm.dex;

import com.android.server.pm.PackageDexOptimizer;

import static com.android.server.pm.PackageDexOptimizer.SKIP_SHARED_LIBRARY_CHECK;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.pm.ApplicationInfo;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.SparseArray;

import dalvik.system.DelegateLastClassLoader;
import dalvik.system.DexClassLoader;
import dalvik.system.PathClassLoader;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class DexoptUtilsTest {
    private static final String DEX_CLASS_LOADER_NAME = DexClassLoader.class.getName();
    private static final String PATH_CLASS_LOADER_NAME = PathClassLoader.class.getName();
    private static final String DELEGATE_LAST_CLASS_LOADER_NAME =
            DelegateLastClassLoader.class.getName();

    private static class TestData {
        ApplicationInfo info;
        boolean[] pathsWithCode;
    }

    private TestData createMockApplicationInfo(String baseClassLoader, boolean addSplits,
            boolean addSplitDependencies) {
        ApplicationInfo ai = new ApplicationInfo();
        String codeDir = "/data/app/mock.android.com";
        ai.setBaseCodePath(codeDir + "/base.dex");
        ai.classLoaderName = baseClassLoader;
        ai.privateFlags = ai.privateFlags | ApplicationInfo.PRIVATE_FLAG_ISOLATED_SPLIT_LOADING;
        boolean[] pathsWithCode;
        if (!addSplits) {
            pathsWithCode = new boolean[] {true};
        } else {
            pathsWithCode = new boolean[9];
            Arrays.fill(pathsWithCode, true);
            pathsWithCode[7] = false;  // config split

            ai.setSplitCodePaths(new String[]{
                    codeDir + "/base-1.dex",
                    codeDir + "/base-2.dex",
                    codeDir + "/base-3.dex",
                    codeDir + "/base-4.dex",
                    codeDir + "/base-5.dex",
                    codeDir + "/base-6.dex",
                    codeDir + "/config-split-7.dex",
                    codeDir + "/feature-no-deps.dex"});

            ai.splitClassLoaderNames = new String[]{
                    DELEGATE_LAST_CLASS_LOADER_NAME,
                    DELEGATE_LAST_CLASS_LOADER_NAME,
                    PATH_CLASS_LOADER_NAME,
                    DEX_CLASS_LOADER_NAME,
                    PATH_CLASS_LOADER_NAME,
                    null,   // A null class loader name should default to PathClassLoader.
                    null,   // The config split gets a null class loader.
                    null};  // The feature split with no dependency and no specified class loader.
            if (addSplitDependencies) {
                ai.splitDependencies = new SparseArray<>(ai.splitClassLoaderNames.length + 1);
                ai.splitDependencies.put(0, new int[] {-1}); // base has no dependency
                ai.splitDependencies.put(1, new int[] {2}); // split 1 depends on 2
                ai.splitDependencies.put(2, new int[] {4}); // split 2 depends on 4
                ai.splitDependencies.put(3, new int[] {4}); // split 3 depends on 4
                ai.splitDependencies.put(4, new int[] {0}); // split 4 depends on base
                ai.splitDependencies.put(5, new int[] {0}); // split 5 depends on base
                ai.splitDependencies.put(6, new int[] {5}); // split 6 depends on 5
                // Do not add the config split to the dependency list.
                // Do not add the feature split with no dependency to the dependency list.
            }
        }
        TestData data = new TestData();
        data.info = ai;
        data.pathsWithCode = pathsWithCode;
        return data;
    }

    @Test
    public void testSplitChain() {
        TestData data = createMockApplicationInfo(PATH_CLASS_LOADER_NAME, true, true);
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals("PCL[a.dex:b.dex]", contexts[0]);
        assertEquals("DLC[];DLC[base-2.dex];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]",
                contexts[1]);
        assertEquals("DLC[];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[2]);
        assertEquals("PCL[];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[3]);
        assertEquals("PCL[];PCL[a.dex:b.dex:base.dex]", contexts[4]);
        assertEquals("PCL[];PCL[a.dex:b.dex:base.dex]", contexts[5]);
        assertEquals("PCL[];PCL[base-5.dex];PCL[a.dex:b.dex:base.dex]", contexts[6]);
        assertEquals(null, contexts[7]);  // config split
        assertEquals("PCL[]", contexts[8]);  // feature split with no dependency
    }

    @Test
    public void testSplitChainNoSplitDependencies() {
        TestData data = createMockApplicationInfo(PATH_CLASS_LOADER_NAME, true, false);
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals("PCL[a.dex:b.dex]", contexts[0]);
        assertEquals("PCL[a.dex:b.dex:base.dex]", contexts[1]);
        assertEquals("PCL[a.dex:b.dex:base.dex:base-1.dex]", contexts[2]);
        assertEquals("PCL[a.dex:b.dex:base.dex:base-1.dex:base-2.dex]", contexts[3]);
        assertEquals("PCL[a.dex:b.dex:base.dex:base-1.dex:base-2.dex:base-3.dex]", contexts[4]);
        assertEquals(
                "PCL[a.dex:b.dex:base.dex:base-1.dex:base-2.dex:base-3.dex:base-4.dex]",
                contexts[5]);
        assertEquals(
                "PCL[a.dex:b.dex:base.dex:base-1.dex:base-2.dex:base-3.dex:base-4.dex:base-5.dex]",
                contexts[6]);
        assertEquals(null, contexts[7]);  // config split
        assertEquals(
                "PCL[a.dex:b.dex:base.dex:base-1.dex:base-2.dex:base-3.dex:base-4.dex:base-5.dex:base-6.dex:config-split-7.dex]",
                contexts[8]);  // feature split with no dependency
    }

    @Test
    public void testSplitChainNoIsolationNoSharedLibrary() {
        TestData data = createMockApplicationInfo(PATH_CLASS_LOADER_NAME, true, true);
        data.info.privateFlags = data.info.privateFlags
                & (~ApplicationInfo.PRIVATE_FLAG_ISOLATED_SPLIT_LOADING);
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, null, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals("PCL[]", contexts[0]);
        assertEquals("PCL[base.dex]", contexts[1]);
        assertEquals("PCL[base.dex:base-1.dex]", contexts[2]);
        assertEquals("PCL[base.dex:base-1.dex:base-2.dex]", contexts[3]);
        assertEquals("PCL[base.dex:base-1.dex:base-2.dex:base-3.dex]", contexts[4]);
        assertEquals("PCL[base.dex:base-1.dex:base-2.dex:base-3.dex:base-4.dex]", contexts[5]);
        assertEquals(
                "PCL[base.dex:base-1.dex:base-2.dex:base-3.dex:base-4.dex:base-5.dex]",
                contexts[6]);
        assertEquals(null, contexts[7]);  // config split
        assertEquals(
                "PCL[base.dex:base-1.dex:base-2.dex:base-3.dex:base-4.dex:base-5.dex:base-6.dex:config-split-7.dex]",
                contexts[8]);  // feature split with no dependency
    }

    @Test
    public void testSplitChainNoSharedLibraries() {
        TestData data = createMockApplicationInfo(
                DELEGATE_LAST_CLASS_LOADER_NAME, true, true);
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, null, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals("DLC[]", contexts[0]);
        assertEquals("DLC[];DLC[base-2.dex];PCL[base-4.dex];DLC[base.dex]", contexts[1]);
        assertEquals("DLC[];PCL[base-4.dex];DLC[base.dex]", contexts[2]);
        assertEquals("PCL[];PCL[base-4.dex];DLC[base.dex]", contexts[3]);
        assertEquals("PCL[];DLC[base.dex]", contexts[4]);
        assertEquals("PCL[];DLC[base.dex]", contexts[5]);
        assertEquals("PCL[];PCL[base-5.dex];DLC[base.dex]", contexts[6]);
        assertEquals(null, contexts[7]);  // config split
        assertEquals("PCL[]", contexts[8]);  // feature split with no dependency
    }

    @Test
    public void testSplitChainWithNullPrimaryClassLoader() {
        // A null classLoaderName should mean PathClassLoader.
        TestData data = createMockApplicationInfo(null, true, true);
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals("PCL[a.dex:b.dex]", contexts[0]);
        assertEquals("DLC[];DLC[base-2.dex];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[1]);
        assertEquals("DLC[];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[2]);
        assertEquals("PCL[];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[3]);
        assertEquals("PCL[];PCL[a.dex:b.dex:base.dex]", contexts[4]);
        assertEquals("PCL[];PCL[a.dex:b.dex:base.dex]", contexts[5]);
        assertEquals("PCL[];PCL[base-5.dex];PCL[a.dex:b.dex:base.dex]", contexts[6]);
        assertEquals(null, contexts[7]);  // config split
        assertEquals("PCL[]", contexts[8]);  // feature split with no dependency
    }

    @Test
    public void tesNoSplits() {
        TestData data = createMockApplicationInfo(PATH_CLASS_LOADER_NAME, false, false);
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(1, contexts.length);
        assertEquals("PCL[a.dex:b.dex]", contexts[0]);
    }

    @Test
    public void tesNoSplitsNullClassLoaderName() {
        TestData data = createMockApplicationInfo(null, false, false);
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(1, contexts.length);
        assertEquals("PCL[a.dex:b.dex]", contexts[0]);
    }

    @Test
    public void tesNoSplitDelegateLast() {
        TestData data = createMockApplicationInfo(
                DELEGATE_LAST_CLASS_LOADER_NAME, false, false);
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(1, contexts.length);
        assertEquals("DLC[a.dex:b.dex]", contexts[0]);
    }

    @Test
    public void tesNoSplitsNoSharedLibraries() {
        TestData data = createMockApplicationInfo(PATH_CLASS_LOADER_NAME, false, false);
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, null, data.pathsWithCode);

        assertEquals(1, contexts.length);
        assertEquals("PCL[]", contexts[0]);
    }

    @Test
    public void tesNoSplitDelegateLastNoSharedLibraries() {
        TestData data = createMockApplicationInfo(
                DELEGATE_LAST_CLASS_LOADER_NAME, false, false);
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, null, data.pathsWithCode);

        assertEquals(1, contexts.length);
        assertEquals("DLC[]", contexts[0]);
    }

    @Test
    public void testContextWithNoCode() {
        TestData data = createMockApplicationInfo(null, true, false);
        Arrays.fill(data.pathsWithCode, false);

        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals(null, contexts[0]);
        assertEquals(null, contexts[1]);
        assertEquals(null, contexts[2]);
        assertEquals(null, contexts[3]);
        assertEquals(null, contexts[4]);
        assertEquals(null, contexts[5]);
        assertEquals(null, contexts[6]);
        assertEquals(null, contexts[7]);
    }

    @Test
    public void testContextBaseNoCode() {
        TestData data = createMockApplicationInfo(null, true, true);
        data.pathsWithCode[0] = false;
        String[] sharedLibrary = new String[] {"a.dex", "b.dex"};
        String[] contexts = DexoptUtils.getClassLoaderContexts(
                data.info, sharedLibrary, data.pathsWithCode);

        assertEquals(9, contexts.length);
        assertEquals(null, contexts[0]);
        assertEquals("DLC[];DLC[base-2.dex];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[1]);
        assertEquals("DLC[];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[2]);
        assertEquals("PCL[];PCL[base-4.dex];PCL[a.dex:b.dex:base.dex]", contexts[3]);
        assertEquals("PCL[];PCL[a.dex:b.dex:base.dex]", contexts[4]);
        assertEquals("PCL[];PCL[a.dex:b.dex:base.dex]", contexts[5]);
        assertEquals("PCL[];PCL[base-5.dex];PCL[a.dex:b.dex:base.dex]", contexts[6]);
        assertEquals(null, contexts[7]);
    }

    @Test
    public void testProcessContextForDexLoad() {
        List<String> classLoaders = Arrays.asList(
                DELEGATE_LAST_CLASS_LOADER_NAME,
                PATH_CLASS_LOADER_NAME,
                PATH_CLASS_LOADER_NAME);
        List<String> classPaths = Arrays.asList(
                String.join(File.pathSeparator, "foo.dex", "bar.dex"),
                String.join(File.pathSeparator, "parent1.dex"),
                String.join(File.pathSeparator, "parent2.dex", "parent3.dex"));
        String[] context = DexoptUtils.processContextForDexLoad(classLoaders, classPaths);
        assertNotNull(context);
        assertEquals(2, context.length);
        assertEquals("DLC[];PCL[parent1.dex];PCL[parent2.dex:parent3.dex]", context[0]);
        assertEquals("DLC[foo.dex];PCL[parent1.dex];PCL[parent2.dex:parent3.dex]", context[1]);
    }

    @Test
    public void testProcessContextForDexLoadSingleElement() {
        List<String> classLoaders = Arrays.asList(PATH_CLASS_LOADER_NAME);
        List<String> classPaths = Arrays.asList(
                String.join(File.pathSeparator, "foo.dex", "bar.dex", "zoo.dex"));
        String[] context = DexoptUtils.processContextForDexLoad(classLoaders, classPaths);
        assertNotNull(context);
        assertEquals(3, context.length);
        assertEquals("PCL[]", context[0]);
        assertEquals("PCL[foo.dex]", context[1]);
        assertEquals("PCL[foo.dex:bar.dex]", context[2]);
    }

    @Test
    public void testProcessContextForDexLoadUnsupported() {
        List<String> classLoaders = Arrays.asList(
                DELEGATE_LAST_CLASS_LOADER_NAME,
                "unsupported.class.loader");
        List<String> classPaths = Arrays.asList(
                String.join(File.pathSeparator, "foo.dex", "bar.dex"),
                String.join(File.pathSeparator, "parent1.dex"));
        String[] context = DexoptUtils.processContextForDexLoad(classLoaders, classPaths);
        assertNull(context);
    }

    @Test
    public void testProcessContextForDexLoadIllegalCallEmptyList() {
        boolean gotException = false;
        try {
            DexoptUtils.processContextForDexLoad(Collections.emptyList(), Collections.emptyList());
        } catch (IllegalArgumentException ignore) {
            gotException = true;
        }
        assertTrue(gotException);
    }

    @Test
    public void testProcessContextForDexLoadIllegalCallDifferentSize() {
        boolean gotException = false;
        try {
            DexoptUtils.processContextForDexLoad(Collections.emptyList(), Arrays.asList("a"));
        } catch (IllegalArgumentException ignore) {
            gotException = true;
        }
        assertTrue(gotException);
    }

    @Test
    public void testEncodeClassLoader() {
        assertEquals(SKIP_SHARED_LIBRARY_CHECK, DexoptUtils.encodeClassLoader(
                SKIP_SHARED_LIBRARY_CHECK, "dalvik.system.PathClassLoader"));
        assertEquals(SKIP_SHARED_LIBRARY_CHECK, DexoptUtils.encodeClassLoader(
                SKIP_SHARED_LIBRARY_CHECK, "dalvik.system.DexClassLoader"));
        assertEquals(SKIP_SHARED_LIBRARY_CHECK, DexoptUtils.encodeClassLoader(
                SKIP_SHARED_LIBRARY_CHECK, "dalvik.system.DelegateLastClassLoader"));
        assertEquals("PCL[xyz]", DexoptUtils.encodeClassLoader("xyz",
                "dalvik.system.PathClassLoader"));
        assertEquals("PCL[xyz]", DexoptUtils.encodeClassLoader("xyz",
                "dalvik.system.DexClassLoader"));
        assertEquals("DLC[xyz]", DexoptUtils.encodeClassLoader("xyz",
                "dalvik.system.DelegateLastClassLoader"));
        assertEquals("PCL[xyz]", DexoptUtils.encodeClassLoader("xyz", null));
        assertEquals("abc[xyz]", DexoptUtils.encodeClassLoader("xyz", "abc"));

        try {
            DexoptUtils.encodeClassLoader(null, "abc");
            fail(); // Exception should be caught.
        } catch (NullPointerException expected) {}
    }

    @Test
    public void testEncodeClassLoaderChain() {
        assertEquals(SKIP_SHARED_LIBRARY_CHECK, DexoptUtils.encodeClassLoaderChain(
                SKIP_SHARED_LIBRARY_CHECK, "PCL[a]"));
        assertEquals(SKIP_SHARED_LIBRARY_CHECK, DexoptUtils.encodeClassLoaderChain("PCL[a]",
                SKIP_SHARED_LIBRARY_CHECK));
        assertEquals("PCL[a];DLC[b]", DexoptUtils.encodeClassLoaderChain("PCL[a]",
                "DLC[b]"));
        assertEquals(SKIP_SHARED_LIBRARY_CHECK, DexoptUtils.encodeClassLoaderChain("PCL[a]",
                SKIP_SHARED_LIBRARY_CHECK));

        try {
            DexoptUtils.encodeClassLoaderChain("a", null);
            fail(); // exception is expected
        } catch (NullPointerException expected) {}

        try {
            DexoptUtils.encodeClassLoaderChain(null, "b");
            fail(); // exception is expected
        } catch (NullPointerException expected) {}
    }
}
