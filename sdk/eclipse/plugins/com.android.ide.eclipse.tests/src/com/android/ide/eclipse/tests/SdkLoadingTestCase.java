/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License"); you
 * may not use this file except in compliance with the License. You may obtain a
 * copy of the License at
 *
 * http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.ide.eclipse.tests;

import com.android.ide.common.sdk.LoadStatus;
import com.android.ide.eclipse.adt.AdtPlugin;
import com.android.ide.eclipse.adt.internal.preferences.AdtPrefs;
import com.android.ide.eclipse.adt.internal.sdk.AndroidTargetParser;
import com.android.ide.eclipse.adt.internal.sdk.Sdk;
import com.android.sdklib.IAndroidTarget;
import com.android.testutils.SdkTestCase;

import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.NullProgressMonitor;

/**
 * A test case which uses the SDK loaded by the ADT plugin.
 */
public abstract class SdkLoadingTestCase extends SdkTestCase {

    private Sdk mSdk;

    protected SdkLoadingTestCase() {
    }

    /**
     * Retrieve the {@link Sdk} under test.
     */
    protected Sdk getSdk() {
        if (mSdk == null) {
            mSdk = loadSdk();
            assertNotNull(mSdk);
            validateSdk(mSdk);
        }
        return mSdk;
    }

    /**
     * Gets the current SDK from ADT, waiting if necessary.
     */
    private Sdk loadSdk() {
        AdtPlugin adt = AdtPlugin.getDefault();

        // We'll never get an AdtPlugin object when running this with the
        // non-Eclipse jUnit test runner.
        if (adt == null) {
            return null;
        }

        // We'll never break out of the SDK load-wait-loop if the AdtPlugin doesn't
        // actually have a valid SDK location because it won't have started an async load:
        String sdkLocation = AdtPrefs.getPrefs().getOsSdkFolder();
        assertTrue("No valid SDK installation is set; for tests you typically need to set the"
                + " environment variable ADT_TEST_SDK_PATH to point to an SDK folder",
                sdkLocation != null && sdkLocation.length() > 0);

        Object sdkLock = Sdk.getLock();
        LoadStatus loadStatus = LoadStatus.LOADING;
        // wait for ADT to load the SDK on a separate thread
        // loop max of 600 times * 200 ms =  2 minutes
        final int maxWait = 600;
        for (int i=0; i < maxWait && loadStatus == LoadStatus.LOADING; i++) {
            try {
                Thread.sleep(200);
            }
            catch (InterruptedException e) {
                // ignore
            }
            synchronized (sdkLock) {
                loadStatus = adt.getSdkLoadStatus();
            }
        }
        Sdk sdk = null;
        synchronized (sdkLock) {
            assertEquals(LoadStatus.LOADED, loadStatus);
            sdk = Sdk.getCurrent();
        }
        assertNotNull(sdk);
        return sdk;
    }

    protected boolean validateSdk(IAndroidTarget target) {
        return true;
    }

    /**
     * Checks that the provided sdk contains one or more valid targets.
     * @param sdk the {@link Sdk} to validate.
     */
    private void validateSdk(Sdk sdk) {
        assertTrue("sdk has no targets", sdk.getTargets().length > 0);
        for (IAndroidTarget target : sdk.getTargets()) {
            if (!validateSdk(target)) {
                continue;
            }
            if (false) { // This takes forEVER
            IStatus status = new AndroidTargetParser(target).run(new NullProgressMonitor());
            if (status.getCode() != IStatus.OK) {
                fail("Failed to parse targets data");
            }
            }
        }
    }
}
