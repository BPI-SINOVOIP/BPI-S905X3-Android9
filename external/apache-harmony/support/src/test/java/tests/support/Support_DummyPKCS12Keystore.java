/* 
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package tests.support;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.security.Key;
import java.security.KeyStoreException;
import java.security.KeyStoreSpi;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.util.Date;
import java.util.Enumeration;

/**
 * Empty implementation used to enable unit tests to run.
 */
public class Support_DummyPKCS12Keystore extends KeyStoreSpi {

    public Support_DummyPKCS12Keystore() {
        super();
    }

    @Override
    public Key engineGetKey(String arg0, char[] arg1)
            throws NoSuchAlgorithmException, UnrecoverableKeyException {
        return null;
    }

    @Override
    public Certificate[] engineGetCertificateChain(String arg0) {
        return null;
    }

    @Override
    public Certificate engineGetCertificate(String arg0) {
        return null;
    }

    @Override
    public Date engineGetCreationDate(String arg0) {
        return null;
    }

    @Override
    public void engineSetKeyEntry(String arg0, Key arg1, char[] arg2,
            Certificate[] arg3) throws KeyStoreException {
    }

    @Override
    public void engineSetKeyEntry(String arg0, byte[] arg1, Certificate[] arg2)
            throws KeyStoreException {
    }

    @Override
    public void engineSetCertificateEntry(String arg0, Certificate arg1)
            throws KeyStoreException {
    }

    @Override
    public void engineDeleteEntry(String arg0) throws KeyStoreException {
    }

    @Override
    public Enumeration<String> engineAliases() {
        return null;
    }

    @Override
    public boolean engineContainsAlias(String arg0) {
        return false;
    }

    @Override
    public int engineSize() {
        return 0;
    }

    @Override
    public boolean engineIsKeyEntry(String arg0) {
        return false;
    }

    @Override
    public boolean engineIsCertificateEntry(String arg0) {
        return false;
    }

    @Override
    public String engineGetCertificateAlias(Certificate arg0) {
        return null;
    }

    @Override
    public void engineStore(OutputStream arg0, char[] arg1) throws IOException,
            NoSuchAlgorithmException, CertificateException {
    }

    @Override
    public void engineLoad(InputStream arg0, char[] arg1) throws IOException,
            NoSuchAlgorithmException, CertificateException {
    }
}
