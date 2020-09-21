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

package com.android.keychain;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.security.Credentials;
import android.security.KeyStore;
import com.android.keychain.internal.KeyInfoProvider;
import java.util.concurrent.CancellationException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowApplication;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = TestConfig.MANIFEST_PATH, sdk = TestConfig.SDK_VERSION)
public final class AliasLoaderTest {
    private KeyInfoProvider mDummyInfoProvider;

    @Before
    public void setUp() {
        mDummyInfoProvider =
                new KeyInfoProvider() {
                    public boolean isUserSelectable(String alias) {
                        return true;
                    }
                };
    }

    @Test
    public void testAliasLoader_loadsAllAliases()
            throws InterruptedException, ExecutionException, CancellationException,
                    TimeoutException {
        KeyStore keyStore = mock(KeyStore.class);
        when(keyStore.list(Credentials.USER_PRIVATE_KEY)).thenReturn(new String[] {"b", "c", "a"});

        KeyChainActivity.AliasLoader loader =
                new KeyChainActivity.AliasLoader(
                        keyStore, RuntimeEnvironment.application, mDummyInfoProvider);
        loader.execute();

        ShadowApplication.runBackgroundTasks();
        KeyChainActivity.CertificateAdapter result = loader.get(5, TimeUnit.SECONDS);
        Assert.assertNotNull(result);
        Assert.assertEquals(3, result.getCount());
        Assert.assertEquals("a", result.getItem(0));
        Assert.assertEquals("b", result.getItem(1));
        Assert.assertEquals("c", result.getItem(2));
    }

    @Test
    public void testAliasLoader_copesWithNoAliases()
            throws InterruptedException, ExecutionException, CancellationException,
                    TimeoutException {
        KeyStore keyStore = mock(KeyStore.class);
        when(keyStore.list(Credentials.USER_PRIVATE_KEY)).thenReturn(null);

        KeyChainActivity.AliasLoader loader =
                new KeyChainActivity.AliasLoader(
                        keyStore, RuntimeEnvironment.application, mDummyInfoProvider);
        loader.execute();

        ShadowApplication.runBackgroundTasks();
        KeyChainActivity.CertificateAdapter result = loader.get(5, TimeUnit.SECONDS);
        Assert.assertNotNull(result);
        Assert.assertEquals(0, result.getCount());
    }

    @Test
    public void testAliasLoader_filtersNonUserSelectableAliases()
            throws InterruptedException, ExecutionException, CancellationException,
                    TimeoutException {
        KeyStore keyStore = mock(KeyStore.class);
        when(keyStore.list(Credentials.USER_PRIVATE_KEY)).thenReturn(new String[] {"a", "b", "c"});
        KeyInfoProvider infoProvider = mock(KeyInfoProvider.class);
        when(infoProvider.isUserSelectable("a")).thenReturn(false);
        when(infoProvider.isUserSelectable("b")).thenReturn(true);
        when(infoProvider.isUserSelectable("c")).thenReturn(false);

        KeyChainActivity.AliasLoader loader =
                new KeyChainActivity.AliasLoader(
                        keyStore, RuntimeEnvironment.application, infoProvider);
        loader.execute();

        ShadowApplication.runBackgroundTasks();
        KeyChainActivity.CertificateAdapter result = loader.get(5, TimeUnit.SECONDS);
        Assert.assertNotNull(result);
        Assert.assertEquals(1, result.getCount());
        Assert.assertEquals("b", result.getItem(0));
    }
}
