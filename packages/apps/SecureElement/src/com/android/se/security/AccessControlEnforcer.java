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
 * Copyright 2012 Giesecke & Devrient GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.android.se.security;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.Signature;
import android.os.Build;
import android.os.SystemProperties;
import android.util.Log;

import com.android.se.Channel;
import com.android.se.SecureElementService;
import com.android.se.Terminal;
import com.android.se.security.ChannelAccess.ACCESS;
import com.android.se.security.ara.AraController;
import com.android.se.security.arf.ArfController;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.security.AccessControlException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.cert.Certificate;
import java.security.cert.CertificateEncodingException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.MissingResourceException;
import java.util.NoSuchElementException;

/** Reads and Maintains the ARF and ARA access control for a particular Secure Element */
public class AccessControlEnforcer {

    private final String mTag = "SecureElement-AccessControlEnforcer";
    private PackageManager mPackageManager = null;
    private AraController mAraController = null;
    private boolean mUseAra = true;
    private ArfController mArfController = null;
    private boolean mUseArf = false;
    private AccessRuleCache mAccessRuleCache = null;
    private boolean mRulesRead = false;
    private Terminal mTerminal = null;
    private ChannelAccess mInitialChannelAccess = new ChannelAccess();
    private boolean mFullAccess = false;

    public AccessControlEnforcer(Terminal terminal) {

        mTerminal = terminal;
        mAccessRuleCache = new AccessRuleCache();
    }

    public static byte[] getDefaultAccessControlAid() {
        return AraController.getAraMAid();
    }

    private static Certificate decodeCertificate(byte[] certData) throws CertificateException {
        CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        X509Certificate cert =
                (X509Certificate) certFactory.generateCertificate(
                        new ByteArrayInputStream(certData));
        return cert;
    }

    /** Returns the Hash of the Application */
    public static byte[] getAppCertHash(Certificate appCert) throws CertificateEncodingException {
        MessageDigest md = null;
        try {
            md = MessageDigest.getInstance("SHA");
        } catch (NoSuchAlgorithmException e) {
            throw new AccessControlException("Exception getting SHA for the signature");
        }
        if (md == null) {
            throw new AccessControlException("Hash can not be computed");
        }
        return md.digest(appCert.getEncoded());
    }

    public PackageManager getPackageManager() {
        return mPackageManager;
    }

    public void setPackageManager(PackageManager packageManager) {
        mPackageManager = packageManager;
    }

    public Terminal getTerminal() {
        return mTerminal;
    }

    public AccessRuleCache getAccessRuleCache() {
        return mAccessRuleCache;
    }

    /** Resets the Access Control for the Secure Element */
    public synchronized void reset() {
        // Destroy any previous Controler
        // in order to reset the ACE
        Log.i(mTag, "Reset the ACE for terminal:" + mTerminal.getName());
        mAccessRuleCache.reset();
        mAraController = null;
        mArfController = null;
    }

    /** Initializes the Access Control for the Secure Element */
    public synchronized void initialize() throws IOException, MissingResourceException {
        boolean status = true;
        String denyMsg = "";
        // allow access to set up access control for a channel
        mInitialChannelAccess.setApduAccess(ChannelAccess.ACCESS.ALLOWED);
        mInitialChannelAccess.setNFCEventAccess(ChannelAccess.ACCESS.ALLOWED);
        mInitialChannelAccess.setAccess(ChannelAccess.ACCESS.ALLOWED, "");

        readSecurityProfile();

        if (!mTerminal.getName().startsWith(SecureElementService.UICC_TERMINAL)) {
            // When SE is not the UICC then it's allowed to grant full access if no
            // rules can be retreived.
            mFullAccess = true;
        }

        // 1 - Let's try to use ARA
        if (mUseAra && mAraController == null) {
            mAraController = new AraController(mAccessRuleCache, mTerminal);
        }

        if (mUseAra && mAraController != null) {
            try {
                mAraController.initialize();
                Log.i(mTag, "ARA applet is used for:" + mTerminal.getName());
                // disable other access methods
                mUseArf = false;
                mFullAccess = false;
            } catch (IOException | MissingResourceException e) {
                throw e;
            } catch (Exception e) {
                // ARA cannot be used since we got an exception during initialization
                mUseAra = false;
                denyMsg = e.getLocalizedMessage();
                if (e instanceof NoSuchElementException) {
                    Log.i(mTag, "No ARA applet found in: " + mTerminal.getName());
                } else if (mTerminal.getName().startsWith(SecureElementService.UICC_TERMINAL)) {
                    // A possible explanation could simply be due to the fact that the UICC is old
                    // and does not support logical channel (and is not compliant with GP spec).
                    // We should simply act as if no ARA was available in this case.
                    if (!mUseArf) {
                        // Only ARA was the candidate to retrieve access rules,
                        // but it is not 100% sure if the expected ARA really does not exist.
                        // Full access should not be granted in this case.
                        mFullAccess = false;
                        status = false;
                    }
                } else {
                    // ARA is available but doesn't work properly.
                    // We are going to disable everything per security req.
                    mUseArf = false;
                    mFullAccess = false;
                    status = false;
                    Log.i(mTag, "Problem accessing ARA, Access DENIED "
                            + e.getLocalizedMessage());
                }
            }
        }

        // 2 - Let's try to use ARF since ARA cannot be used
        if (mUseArf && !mTerminal.getName().startsWith(SecureElementService.UICC_TERMINAL)) {
            Log.i(mTag, "Disable ARF for terminal: " + mTerminal.getName()
                    + " (ARF is only available for UICC)");
            mUseArf = false; // Arf is only supproted on UICC
        }

        if (mUseArf && mArfController == null) {
            mArfController = new ArfController(mAccessRuleCache, mTerminal);
        }

        if (mUseArf && mArfController != null) {
            try {
                mArfController.initialize();
                // disable other access methods
                Log.i(mTag, "ARF rules are used for:" + mTerminal.getName());
                mFullAccess = false;
            } catch (IOException | MissingResourceException e) {
                throw e;
            } catch (Exception e) {
                // ARF cannot be used since we got an exception
                mUseArf = false;
                denyMsg = e.getLocalizedMessage();
                Log.e(mTag, e.getMessage());
                if (mFullAccess) {
                    if (!(e instanceof NoSuchElementException)) {
                        // It is not 100% sure if the expected ARF really does not exist.
                        // No ARF might be due to a kind of temporary problem like missing resource,
                        // so full access should not be granted in this case.
                        mFullAccess = false;
                        status = false;
                    }
                } else {
                    status = false;
                }
            }
        }

        /* 4 - Let's block everything since neither ARA, ARF or fullaccess can be used */
        if (!mUseArf && !mUseAra && !mFullAccess) {
            mInitialChannelAccess.setApduAccess(ChannelAccess.ACCESS.DENIED);
            mInitialChannelAccess.setNFCEventAccess(ChannelAccess.ACCESS.DENIED);
            mInitialChannelAccess.setAccess(ChannelAccess.ACCESS.DENIED, denyMsg);
            Log.i(mTag, "Deny any access to:" + mTerminal.getName());
        }

        mRulesRead = status;
    }

    /** Check if the Channel has permission for the given APDU */
    public synchronized void checkCommand(Channel channel, byte[] command) {
        ChannelAccess ca = channel.getChannelAccess();
        if (ca == null) {
            throw new AccessControlException(mTag + "Channel access not set");
        }
        String reason = ca.getReason();
        if (reason.length() == 0) {
            reason = "Command not allowed!";
        }
        if (ca.getAccess() != ACCESS.ALLOWED) {
            throw new AccessControlException(mTag + reason);
        }
        if (ca.isUseApduFilter()) {
            ApduFilter[] accessConditions = ca.getApduFilter();
            if (accessConditions == null || accessConditions.length == 0) {
                throw new AccessControlException(mTag + "Access Rule not available:"
                        + reason);
            }
            for (ApduFilter ac : accessConditions) {
                if (CommandApdu.compareHeaders(command, ac.getMask(), ac.getApdu())) {
                    return;
                }
            }
            throw new AccessControlException(mTag + "Access Rule does not match: "
                    + reason);
        }
        if (ca.getApduAccess() == ChannelAccess.ACCESS.ALLOWED) {
            return;
        } else {
            throw new AccessControlException(mTag + "APDU access NOT allowed");
        }
    }

    /** Sets up the Channel Access for the given Package */
    public ChannelAccess setUpChannelAccess(byte[] aid, String packageName, boolean checkRefreshTag)
            throws IOException, MissingResourceException {
        ChannelAccess channelAccess = null;
        // check result of channel access during initialization procedure
        if (mInitialChannelAccess.getAccess() == ChannelAccess.ACCESS.DENIED) {
            throw new AccessControlException(
                    mTag + "access denied: " + mInitialChannelAccess.getReason());
        }
        // this is the new GP Access Control Enforcer implementation
        if (mUseAra || mUseArf) {
            channelAccess = internal_setUpChannelAccess(aid, packageName,
                    checkRefreshTag);
        }
        if (channelAccess == null || (channelAccess.getApduAccess() != ChannelAccess.ACCESS.ALLOWED
                && !channelAccess.isUseApduFilter())) {
            if (mFullAccess) {
                // if full access is set then we reuse the initial channel access,
                // since we got so far it allows everything with a descriptive reason.
                channelAccess = mInitialChannelAccess;
            } else {
                throw new AccessControlException(mTag + "no APDU access allowed!");
            }
        }
        channelAccess.setPackageName(packageName);
        return channelAccess.clone();
    }

    private synchronized ChannelAccess internal_setUpChannelAccess(byte[] aid,
            String packageName, boolean checkRefreshTag) throws IOException,
            MissingResourceException {
        if (packageName == null || packageName.isEmpty()) {
            throw new AccessControlException("package names must be specified");
        }
        try {
            // estimate SHA-1 hash value of the device application's certificate.
            Certificate[] appCerts = getAPPCerts(packageName);
            // APP certificates must be available => otherwise Exception
            if (appCerts == null || appCerts.length == 0) {
                throw new AccessControlException(
                        "Application certificates are invalid or do not exist.");
            }
            if (checkRefreshTag) {
                updateAccessRuleIfNeed();
            }
            return getAccessRule(aid, appCerts);
        } catch (IOException | MissingResourceException e) {
            throw e;
        } catch (Throwable exp) {
            throw new AccessControlException(exp.getMessage());
        }
    }

    /** Fetches the Access Rules for the given application and AID pair */
    public ChannelAccess getAccessRule(
            byte[] aid, Certificate[] appCerts)
            throws AccessControlException, CertificateEncodingException {
        ChannelAccess channelAccess = null;
        // if read all is true get rule from cache.
        if (mRulesRead) {
            // get rules from internal storage
            channelAccess = mAccessRuleCache.findAccessRule(aid, appCerts);
        }
        // if no rule was found return an empty access rule
        // with all access denied.
        if (channelAccess == null) {
            channelAccess = new ChannelAccess();
            channelAccess.setAccess(ChannelAccess.ACCESS.DENIED, "no access rule found!");
            channelAccess.setApduAccess(ChannelAccess.ACCESS.DENIED);
            channelAccess.setNFCEventAccess(ChannelAccess.ACCESS.DENIED);
        }
        return channelAccess;
    }

    /**
     * Returns Certificate chain for one package.
     */
    private Certificate[] getAPPCerts(String packageName)
            throws CertificateException, NoSuchAlgorithmException, AccessControlException {
        if (packageName == null || packageName.length() == 0) {
            throw new AccessControlException("Package Name not defined");
        }
        PackageInfo foundPkgInfo;
        try {
            foundPkgInfo = mPackageManager.getPackageInfo(packageName,
                    PackageManager.GET_SIGNATURES);
        } catch (NameNotFoundException ne) {
            throw new AccessControlException("Package does not exist");
        }
        if (foundPkgInfo == null) {
            throw new AccessControlException("Package does not exist");
        }
        ArrayList<Certificate> appCerts = new ArrayList<Certificate>();
        for (Signature signature : foundPkgInfo.signatures) {
            appCerts.add(decodeCertificate(signature.toByteArray()));
        }
        return appCerts.toArray(new Certificate[appCerts.size()]);
    }

    /** Returns true if the given application is allowed to recieve NFC Events */
    public synchronized boolean[] isNfcEventAllowed(byte[] aid,
            String[] packageNames, boolean checkRefreshTag) {
        if (mUseAra || mUseArf) {
            return internal_isNfcEventAllowed(aid, packageNames, checkRefreshTag);
        } else {
            // if ARA and ARF is not available and
            // - terminal DOES NOT belong to a UICC -> mFullAccess is true
            // - terminal belongs to a UICC -> mFullAccess is false
            boolean[] ret = new boolean[packageNames.length];
            for (int i = 0; i < ret.length; i++) {
                ret[i] = mFullAccess;
            }
            return ret;
        }
    }

    private synchronized boolean[] internal_isNfcEventAllowed(byte[] aid,
            String[] packageNames, boolean checkRefreshTag) {
        if (checkRefreshTag) {
            try {
                updateAccessRuleIfNeed();
            } catch (IOException | MissingResourceException e) {
                throw new AccessControlException("Access-Control not found in "
                        + mTerminal.getName());
            }
        }

        int i = 0;
        boolean[] nfcEventFlags = new boolean[packageNames.length];
        for (String packageName : packageNames) {
            // estimate SHA-1 hash value of the device application's certificate.
            try {
                Certificate[] appCerts = getAPPCerts(packageName);
                // APP certificates must be available => otherwise Exception
                if (appCerts == null || appCerts.length == 0) {
                    nfcEventFlags[i] = false;
                } else {
                    ChannelAccess channelAccess = getAccessRule(aid, appCerts);
                    nfcEventFlags[i] =
                            (channelAccess.getNFCEventAccess() == ChannelAccess.ACCESS.ALLOWED);
                }
            } catch (Exception e) {
                Log.w(mTag, " Access Rules for NFC: " + e.getLocalizedMessage());
                nfcEventFlags[i] = false;
            }
            i++;
        }
        return nfcEventFlags;
    }

    private void updateAccessRuleIfNeed() throws IOException {
        if (mUseAra && mAraController != null) {
            try {
                mAraController.initialize();
                mUseArf = false;
                mFullAccess = false;
            } catch (IOException | MissingResourceException e) {
                // There was a communication error between the terminal and the secure element
                // or failure in retrieving rules due to the lack of a new logical channel.
                // These errors must be distinguished from other ones.
                throw e;
            } catch (Exception e) {
                throw new AccessControlException("No ARA applet found in " + mTerminal.getName());
            }
        } else if (mUseArf && mArfController != null) {
            try {
                mArfController.initialize();
            } catch (IOException | MissingResourceException e) {
                // There was a communication error between the terminal and the secure element
                // or failure in retrieving rules due to the lack of a new logical channel.
                // These errors must be distinguished from other ones.
                throw e;
            } catch (Exception e) {
                Log.e(mTag, e.getMessage());
            }
        }
    }

    /** Debug information to be used by dumpsys */
    public void dump(PrintWriter writer) {
        writer.println(mTag + ":");

        writer.println("mUseArf: " + mUseArf);
        writer.println("mUseAra: " + mUseAra);
        writer.println("mInitialChannelAccess:");
        writer.println(mInitialChannelAccess.toString());
        writer.println();

        /* Dump the access rule cache */
        if (mAccessRuleCache != null) mAccessRuleCache.dump(writer);
    }

    private void readSecurityProfile() {
        if (!Build.IS_DEBUGGABLE) {
            mUseArf = true;
            mUseAra = true;
            mFullAccess = false; // Per default we don't grant full access.
        } else {
            String level = SystemProperties.get("service.seek", "useara usearf");
            level = SystemProperties.get("persist.service.seek", level);

            if (level.contains("usearf")) {
                mUseArf = true;
            } else {
                mUseArf = false;
            }
            if (level.contains("useara")) {
                mUseAra = true;
            } else {
                mUseAra = false;
            }
            if (level.contains("fullaccess")) {
                mFullAccess = true;
            } else {
                mFullAccess = false;
            }
        }
        Log.i(
                mTag,
                "Allowed ACE mode: ara=" + mUseAra + " arf=" + mUseArf + " fullaccess="
                        + mFullAccess);
    }
}
