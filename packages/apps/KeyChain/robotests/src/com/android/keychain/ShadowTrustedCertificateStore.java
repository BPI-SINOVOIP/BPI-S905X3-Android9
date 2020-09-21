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

package com.android.keychain;

import com.android.org.conscrypt.TrustedCertificateStore;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

import java.io.IOException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

/**
 * Delegating shadow for TrustedCertificateStore.
 */
@Implements(TrustedCertificateStore.class)
public class ShadowTrustedCertificateStore {
    public static TrustedCertificateStore sDelegate;

    @Implementation
    public void installCertificate(X509Certificate cert) throws IOException, CertificateException {
        sDelegate.installCertificate(cert);
    }

    @Implementation
    public String getCertificateAlias(Certificate cert) {
        return sDelegate.getCertificateAlias(cert);
    }

    @Implementation
    public Certificate getCertificate(String alias) {
        return sDelegate.getCertificate(alias);
    }

    @Implementation
    public void deleteCertificateEntry(String alias)
            throws IOException, CertificateException {
        sDelegate.deleteCertificateEntry(alias);
    }
}

