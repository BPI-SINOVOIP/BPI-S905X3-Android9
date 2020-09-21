/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.nfc.NdefMessage;
import android.nfc.NdefRecord;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.os.UserManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Toast;

import com.android.cts.verifier.R;

import java.nio.charset.Charset;

public class NfcTestActivity extends Activity {
    private static final String TAG = "NfcTestActivity";

    /* package */ static final String EXTRA_DISALLOW_BY_POLICY = "disallowByPolicy";

    private static final String NFC_BEAM_PACKAGE = "com.android.nfc";
    private static final String NFC_BEAM_ACTIVITY = "com.android.nfc.BeamShareActivity";
    private static final String SAMPLE_TEXT = "sample text";

    private ComponentName mAdminReceiverComponent;
    private DevicePolicyManager mDevicePolicyManager;
    private UserManager mUserMangaer;
    private NfcAdapter mNfcAdapter;
    private boolean mDisallowByPolicy;
    private boolean mAddUserRestrictionOnFinish;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.byod_nfc_test_activity);

        mAdminReceiverComponent = new ComponentName(this, DeviceAdminTestReceiver.class.getName());
        mDevicePolicyManager = (DevicePolicyManager) getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        mUserMangaer = (UserManager) getSystemService(Context.USER_SERVICE);
        mAddUserRestrictionOnFinish = mUserMangaer.hasUserRestriction(
                UserManager.DISALLOW_OUTGOING_BEAM);
        mDisallowByPolicy = getIntent().getBooleanExtra(EXTRA_DISALLOW_BY_POLICY, false);
        if (mDisallowByPolicy) {
            mDevicePolicyManager.addUserRestriction(mAdminReceiverComponent,
                    UserManager.DISALLOW_OUTGOING_BEAM);
        } else {
            mDevicePolicyManager.clearUserRestriction(mAdminReceiverComponent,
                    UserManager.DISALLOW_OUTGOING_BEAM);
        }

        mNfcAdapter = NfcAdapter.getDefaultAdapter(this);
        mNfcAdapter.setNdefPushMessage(getTestMessage(), this);

        final Intent shareIntent = new Intent(Intent.ACTION_SEND);
        shareIntent.putExtra(Intent.EXTRA_TEXT, SAMPLE_TEXT);

        findViewById(R.id.manual_beam_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                mNfcAdapter.invokeBeam(NfcTestActivity.this);
            }
        });
        findViewById(R.id.intent_share_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                // Specify the package name of NfcBeamActivity so that the tester don't need to
                // select the activity manually.
                shareIntent.setClassName(NFC_BEAM_PACKAGE, NFC_BEAM_ACTIVITY);
                try {
                    startActivity(shareIntent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(NfcTestActivity.this,
                            R.string.provisioning_byod_cannot_resolve_beam_activity,
                            Toast.LENGTH_SHORT).show();
                    Log.e(TAG, "Nfc beam activity not found", e);
                }
            }
        });
    }

    @Override
    public void finish() {
        if (mAddUserRestrictionOnFinish) {
            mDevicePolicyManager.addUserRestriction(mAdminReceiverComponent,
                    UserManager.DISALLOW_OUTGOING_BEAM);
        } else {
            mDevicePolicyManager.clearUserRestriction(mAdminReceiverComponent,
                    UserManager.DISALLOW_OUTGOING_BEAM);
        }
        super.finish();
    }

    private NdefMessage getTestMessage() {
        byte[] mimeBytes = "application/com.android.cts.verifier.managedprovisioning"
                .getBytes(Charset.forName("US-ASCII"));
        byte[] id = new byte[] {1, 3, 3, 7};
        byte[] payload = SAMPLE_TEXT.getBytes(Charset.forName("US-ASCII"));
        return new NdefMessage(new NdefRecord[] {
                new NdefRecord(NdefRecord.TNF_MIME_MEDIA, mimeBytes, id, payload)
        });
    }
}
