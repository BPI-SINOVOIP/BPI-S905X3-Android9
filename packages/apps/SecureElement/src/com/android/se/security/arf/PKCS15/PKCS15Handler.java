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
/*
 * Copyright (c) 2014-2017, The Linux Foundation.
 */

/*
 * Copyright (C) 2011 Deutsche Telekom, A.G.
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

/*
 * Contributed by: Giesecke & Devrient GmbH.
 */

package com.android.se.security.arf.pkcs15;

import android.util.Log;

import com.android.se.Channel;
import com.android.se.security.arf.SecureElement;
import com.android.se.security.arf.SecureElementException;

import java.io.IOException;
import java.security.AccessControlException;
import java.security.cert.CertificateException;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

/** Handles PKCS#15 topology */
public class PKCS15Handler {

    // AID of the GPAC Applet/ADF
    public static final byte[] GPAC_ARF_AID = {
            (byte) 0xA0, 0x00, 0x00, 0x00, 0x18, 0x47, 0x50, 0x41, 0x43, 0x2D, 0x31, 0x35
    };
    // AID of the PKCS#15 ADF
    public static final byte[] PKCS15_AID = {
            (byte) 0xA0, 0x00, 0x00, 0x00, 0x63, 0x50, 0x4B, 0x43, 0x53, 0x2D, 0x31, 0x35
    };
    // AIDs of "Access Control Rules" containers
    public static final byte[][] CONTAINER_AIDS = {PKCS15_AID, GPAC_ARF_AID, null};
    public final String mTag = "SecureElement-PKCS15Handler";
    // Handle to "Secure Element"
    private SecureElement mSEHandle;
    // "Secure Element" label
    private String mSELabel = null;
    // Handle to "Logical Channel" allocated by the SE
    private Channel mArfChannel = null;
    // "EF Access Control Main" object
    private EFACMain mACMainObject = null;
    // EF AC Rules object
    private EFACRules mACRulesObject = null;
    private byte[] mPkcs15Path = null;
    private byte[] mACMainPath = null;
    private boolean mACMFfound = true;

    /**
     * Constructor
     *
     * @param handle Handle to "Secure Element"
     */
    public PKCS15Handler(SecureElement handle) {
        mSEHandle = handle;
    }

    /** Updates "Access Control Rules" */
    private boolean updateACRules() throws CertificateException, IOException,
            MissingResourceException, NoSuchElementException, PKCS15Exception,
            SecureElementException {
        byte[] ACRulesPath = null;
        if (!mACMFfound) {
            mSEHandle.resetAccessRules();
            mACMainPath = null;
            if (mArfChannel != null) mSEHandle.closeArfChannel();
            this.initACEntryPoint();
        }
        try {
            ACRulesPath = mACMainObject.analyseFile();
            mACMFfound = true;
        } catch (IOException e) {
            // IOException must be propagated to the access control enforcer.
            throw e;
        } catch (Exception e) {
            Log.i(mTag, "ACMF Not found !");
            mACMainObject = null;
            mSEHandle.resetAccessRules();
            mACMFfound = false;
            throw e;
        }
        // Check if rules must be updated
        if (ACRulesPath != null) {
            Log.i(mTag, "Access Rules needs to be updated...");
            if (mACRulesObject == null) {
                mACRulesObject = new EFACRules(mSEHandle);
            }
            mSEHandle.clearAccessRuleCache();
            mACMainPath = null;
            if (mArfChannel != null) mSEHandle.closeArfChannel();

            this.initACEntryPoint();

            try {
                mACRulesObject.analyseFile(ACRulesPath);
            } catch (IOException e) {
                // IOException must be propagated to the access control enforcer.
                throw e;
            } catch (Exception e) {
                Log.i(mTag, "Exception: clear access rule cache and refresh tag");
                mSEHandle.resetAccessRules();
                throw e;
            }
            return true;
        } else {
            Log.i(mTag, "Refresh Tag has not been changed...");
            return false;
        }
    }

    /** Initializes "Access Control" entry point [ACMain] */
    private void initACEntryPoint() throws CertificateException, IOException,
            MissingResourceException, NoSuchElementException, PKCS15Exception,
            SecureElementException {

        byte[] DODFPath = null;
        boolean absent = true;

        for (int ind = 0; ind < CONTAINER_AIDS.length; ind++) {
            try {
                boolean result = selectACRulesContainer(CONTAINER_AIDS[ind]);
                // NoSuchElementException was not thrown by the terminal.
                // The terminal confirmed that the specified applet or PKCS#15 ADF exists
                // or could not determine that it does not exists on the secure element.
                absent = false;
                if (!result) {
                    continue;
                }

                byte[] acMainPath = null;
                if (mACMainPath == null) {
                    EFODF ODFObject = new EFODF(mSEHandle);
                    DODFPath = ODFObject.analyseFile(mPkcs15Path);
                    EFDODF DODFObject = new EFDODF(mSEHandle);
                    acMainPath = DODFObject.analyseFile(DODFPath);

                    mACMainPath = acMainPath;
                } else {
                    if (mPkcs15Path != null) {
                        acMainPath = new byte[mPkcs15Path.length + mACMainPath.length];
                        System.arraycopy(mPkcs15Path, 0, acMainPath, 0, mPkcs15Path.length);
                        System.arraycopy(mACMainPath, 0, acMainPath, mPkcs15Path.length,
                                mACMainPath.length);

                    } else {
                        acMainPath = mACMainPath;
                    }
                }
                mACMainObject = new EFACMain(mSEHandle, acMainPath);
                break;
            } catch (NoSuchElementException e) {
                // The specified applet or PKCS#15 ADF does not exist.
                // Let us check the next candidate.
            }
        }

        if (absent) {
            // All the candidate applet and/or PKCS#15 ADF cannot be found on the secure element.
            throw new NoSuchElementException("No ARF exists");
        }
    }

    /**
     * Selects "Access Control Rules" container
     *
     * @param AID Identification of the GPAC Applet/PKCS#15 ADF; <code>null</code> for EF_DIR file
     * @return <code>true</code> when container is active; <code>false</code> otherwise
     */
    private boolean selectACRulesContainer(byte[] aid) throws IOException, MissingResourceException,
            NoSuchElementException, PKCS15Exception, SecureElementException {
        if (aid == null) {
            mArfChannel = mSEHandle.openLogicalArfChannel(new byte[]{});
            if (mArfChannel != null) {
                Log.i(mTag, "Logical channels are used to access to PKC15");
            } else {
                return false;
            }
            // estimate PKCS15 path only if it is not known already.
            if (mPkcs15Path == null) {
                mACMainPath = null;
                // EF_DIR parsing
                EFDIR DIRObject = new EFDIR(mSEHandle);
                mPkcs15Path = DIRObject.lookupAID(PKCS15_AID);
                if (mPkcs15Path == null) {
                    Log.i(mTag, "Cannot use ARF: cannot select PKCS#15 directory via EF Dir");
                    // TODO: Here it might be possible to set a default path
                    // so that SIMs without EF-Dir could be supported.
                    throw new NoSuchElementException("Cannot select PKCS#15 directory via EF Dir");
                }
            }
        } else {
            // if an AID is given use logical channel.
            // Selection of Applet/ADF via AID is done via SCAPI and logical Channels
            mArfChannel = mSEHandle.openLogicalArfChannel(aid);
            if (mArfChannel == null) {
                Log.w(mTag, "GPAC/PKCS#15 ADF not found!!");
                return false;
            }
            // ARF is selected via AID.
            // if there is a change from path selection to AID
            // selection, then reset AC Main path.
            if (mPkcs15Path != null) {
                mACMainPath = null;
            }
            mPkcs15Path = null; // selection is done via AID
        }
        return true;
    }

    /**
     * Loads "Access Control Rules" from container
     *
     * @return false if access rules where not read due to constant refresh tag.
     */
    public synchronized boolean loadAccessControlRules(String secureElement) throws IOException,
            MissingResourceException, NoSuchElementException {
        mSELabel = secureElement;
        Log.i(mTag, "- Loading " + mSELabel + " rules...");
        try {
            initACEntryPoint();
            return updateACRules();
        } catch (IOException | MissingResourceException | NoSuchElementException e) {
            throw e;
        } catch (Exception e) {
            Log.e(mTag, mSELabel + " rules not correctly initialized! " + e.getLocalizedMessage());
            throw new AccessControlException(e.getLocalizedMessage());
        } finally {
            // Close previously opened channel
            if (mArfChannel != null) mSEHandle.closeArfChannel();
        }
    }
}
