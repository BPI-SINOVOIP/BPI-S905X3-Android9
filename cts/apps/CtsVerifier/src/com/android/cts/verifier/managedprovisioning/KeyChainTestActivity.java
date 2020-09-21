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

package com.android.cts.verifier.managedprovisioning;

import static android.keystore.cts.CertificateUtils.createCertificate;

import android.app.admin.DevicePolicyManager;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.security.AttestedKeyPair;
import android.security.KeyChain;
import android.security.KeyChainAliasCallback;
import android.security.KeyChainException;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import java.security.GeneralSecurityException;
import java.security.Principal;
import java.security.PrivateKey;
import java.security.Signature;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import javax.security.auth.x500.X500Principal;

/**
 * Activity to test KeyChain key generation. The following flows are tested: * Generating a key. *
 * Installing a (self-signed) certificate associated with the key, visible to users. * Setting
 * visibility of the certificate to not be visible to user.
 *
 * <p>After the key generation and certificate installation, it should be possible for a user to
 * select the key from the certificate selection prompt when {@code KeyChain.choosePrivateKeyAlias}
 * is called. The test then tests that the key is indeed usable for signing.
 *
 * <p>After the visibility is set to not-user-visible, the prompt is shown again, this time the
 * testes is asked to verify no keys are selectable and cancel the dialog.
 */
public class KeyChainTestActivity extends PassFailButtons.Activity {
    private static final String TAG = "ByodKeyChainActivity";

    public static final String ACTION_KEYCHAIN =
            "com.android.cts.verifier.managedprovisioning.KEYCHAIN";

    public static final String ALIAS = "cts-verifier-gen-rsa-1";
    public static final String KEY_ALGORITHM = "RSA";

    private DevicePolicyManager mDevicePolicyManager;
    private AttestedKeyPair mAttestedKeyPair;
    private X509Certificate mCert;
    private TextView mLogView;
    private TextView mInstructionsView;
    private Button mSetupButton;
    private Button mGoButton;

    // Callback interface for when a key is generated.
    static interface KeyGenerationListener {
        void onKeyPairGenerated(AttestedKeyPair keyPair);
    }

    // Task for generating a key pair using {@code DevicePolicyManager.generateKeyPair}.
    // The listener, if provided, will be invoked after the key has been generated successfully.
    class GenerateKeyTask extends AsyncTask<KeyGenParameterSpec, Integer, AttestedKeyPair> {
        KeyGenerationListener mListener;

        public GenerateKeyTask(KeyGenerationListener listener) {
            mListener = listener;
        }

        @Override
        protected AttestedKeyPair doInBackground(KeyGenParameterSpec... specs) {
            Log.i(TAG, "Generating key pair.");
            try {
                AttestedKeyPair kp =
                        mDevicePolicyManager.generateKeyPair(
                                DeviceAdminTestReceiver.getReceiverComponentName(),
                                KEY_ALGORITHM,
                                specs[0],
                                0);
                if (kp != null) {
                    mLogView.setText("Key generated successfully.");
                } else {
                    mLogView.setText("Failed generating key.");
                }
                return kp;
            } catch (SecurityException e) {
                mLogView.setText("Security exception while generating key.");
                Log.w(TAG, "Security exception", e);
            }

            return null;
        }

        @Override
        protected void onPostExecute(AttestedKeyPair kp) {
            super.onPostExecute(kp);
            if (mListener != null && kp != null) {
                mListener.onKeyPairGenerated(kp);
            }
        }
    }

    // Helper for generating and installing a self-signed certificate.
    class CertificateInstaller implements KeyGenerationListener {
        @Override
        public void onKeyPairGenerated(AttestedKeyPair keyPair) {
            mAttestedKeyPair = keyPair;
            X500Principal issuer = new X500Principal("CN=SelfSigned, O=Android, C=US");
            X500Principal subject = new X500Principal("CN=Subject, O=Android, C=US");
            try {
                mCert = createCertificate(mAttestedKeyPair.getKeyPair(), subject, issuer);
                boolean installResult = installCertificate(mCert, true);
                // called from onPostExecute so safe to interact with the UI here.
                if (installResult) {
                    mLogView.setText("Test ready");
                    mInstructionsView.setText(R.string.provisioning_byod_keychain_info_first_test);
                    mGoButton.setEnabled(true);
                } else {
                    mLogView.setText("FAILED certificate installation.");
                }
            } catch (Exception e) {
                Log.w(TAG, "Failed installing certificate", e);
                mLogView.setText("Error generating a certificate.");
            }
        }
    }

    // Helper for calling {@code DevicePolicyManager.setKeyPairCertificate} with the user-visibility
    // specified in the constructor. Returns true if the call was successful (and no exceptions
    // were thrown).
    protected boolean installCertificate(X509Certificate cert, boolean isUserVisible) {
        try {
            return mDevicePolicyManager.setKeyPairCertificate(
                    DeviceAdminTestReceiver.getReceiverComponentName(),
                    ALIAS,
                    Arrays.asList(new X509Certificate[] {cert}),
                    isUserVisible);
        } catch (SecurityException e) {
            logStatus("Security exception while installing cert.");
            Log.w(TAG, "Security exception", e);
        }
        return false;
    }

    // Invokes choosePrivateKeyAlias.
    void selectCertificate(KeyChainAliasCallback callback) {
        String[] keyTypes = new String[] {KEY_ALGORITHM};
        Principal[] issuers = new Principal[0];
        KeyChain.choosePrivateKeyAlias(
                KeyChainTestActivity.this, callback, keyTypes, issuers, null, null);
    }

    class TestPreparator implements View.OnClickListener {
        @Override
        public void onClick(View v) {
            mLogView.setText("Starting key generation");
            KeyGenParameterSpec spec =
                    new KeyGenParameterSpec.Builder(
                                    ALIAS,
                                    KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)
                            .setKeySize(2048)
                            .setDigests(KeyProperties.DIGEST_SHA256)
                            .setSignaturePaddings(
                                    KeyProperties.SIGNATURE_PADDING_RSA_PSS,
                                    KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
                            .build();
            new GenerateKeyTask(new CertificateInstaller()).execute(spec);
        }
    }

    class SelectCertificate implements View.OnClickListener, KeyChainAliasCallback {
        @Override
        public void onClick(View v) {
            Log.i(TAG, "Selecting certificate");
            mLogView.setText("Waiting for prompt");
            selectCertificate(this);
        }

        @Override
        public void alias(String alias) {
            Log.i(TAG, "Got alias: " + alias);
            if (alias == null) {
                logStatus("FAILED (no alias)");
                return;
            } else if (!alias.equals(ALIAS)) {
                logStatus("FAILED (wrong alias)");
                return;
            }
            logStatus("Got right alias.");
            try {
                PrivateKey privateKey = KeyChain.getPrivateKey(KeyChainTestActivity.this, alias);
                byte[] data = new String("hello").getBytes();
                Signature sign = Signature.getInstance("SHA256withRSA");
                sign.initSign(privateKey);
                sign.update(data);
                if (sign.sign() != null) {
                    prepareSecondTest();
                } else {
                    logStatus("FAILED (cannot sign)");
                }
            } catch (GeneralSecurityException | KeyChainException | InterruptedException e) {
                Log.w(TAG, "Failed using the key", e);
                logStatus("FAILED (key unusable)");
            }
        }
    }

    class SelectCertificateExpectingNone implements View.OnClickListener, KeyChainAliasCallback {
        @Override
        public void onClick(View v) {
            Log.i(TAG, "Selecting certificate");
            mLogView.setText("Waiting for prompt");
            selectCertificate(this);
        }

        @Override
        public void alias(String alias) {
            Log.i(TAG, "Got alias: " + alias);
            if (alias != null) {
                logStatus("FAILED: Should have no certificate.");
            } else {
                logStatus("PASSED (2/2)");
                runOnUiThread(
                        () -> {
                            getPassButton().setEnabled(true);
                        });
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.keychain_test);
        setPassFailButtonClickListeners();
        mDevicePolicyManager =
                (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);

        mLogView = (TextView) findViewById(R.id.provisioning_byod_keychain_test_log);
        mLogView.setMovementMethod(new ScrollingMovementMethod());

        mInstructionsView = (TextView) findViewById(R.id.provisioning_byod_keychain_instructions);

        mSetupButton = (Button) findViewById(R.id.prepare_test_button);
        mSetupButton.setOnClickListener(new TestPreparator());

        mGoButton = (Button) findViewById(R.id.run_test_button);
        mGoButton.setOnClickListener(new SelectCertificate());
        mGoButton.setEnabled(false);

        // Disable the pass button here, only enable it when the 2nd test passes.
        getPassButton().setEnabled(false);
    }

    protected void prepareSecondTest() {
        Runnable uiChanges;
        if (installCertificate(mCert, false)) {
            uiChanges =
                    () -> {
                        mLogView.setText("Second test ready.");
                        mInstructionsView.setText(
                                R.string.provisioning_byod_keychain_info_second_test);
                        mGoButton.setText("Run 2nd test");
                        mGoButton.setOnClickListener(new SelectCertificateExpectingNone());
                    };
        } else {
            uiChanges =
                    () -> {
                        mLogView.setText("FAILED second test setup.");
                        mGoButton.setEnabled(false);
                    };
        }

        runOnUiThread(uiChanges);
    }

    @Override
    public void finish() {
        super.finish();
        try {
            mDevicePolicyManager.removeKeyPair(
                    DeviceAdminTestReceiver.getReceiverComponentName(), ALIAS);
            Log.i(TAG, "Deleted alias " + ALIAS);
        } catch (SecurityException e) {
            Log.w(TAG, "Failed deleting alias", e);
        }
    }

    private void logStatus(String status) {
        runOnUiThread(
                () -> {
                    mLogView.setText(status);
                });
    }
}
