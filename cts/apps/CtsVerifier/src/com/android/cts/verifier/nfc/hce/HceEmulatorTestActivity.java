/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cts.verifier.nfc.hce;

import com.android.cts.verifier.ArrayTestListAdapter;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestListAdapter.TestListItem;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.nfc.NfcAdapter;
import android.nfc.cardemulation.CardEmulation;
import android.os.Build;
import android.os.Bundle;

/** Activity that lists all the NFC HCE emulator tests. */
public class HceEmulatorTestActivity extends PassFailButtons.TestListActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pass_fail_list);
        setInfoResources(R.string.nfc_test, R.string.nfc_hce_emulator_test_info, 0);
        setPassFailButtonClickListeners();

        ArrayTestListAdapter adapter = new ArrayTestListAdapter(this);

        NfcAdapter nfcAdapter = NfcAdapter.getDefaultAdapter(this);
        CardEmulation cardEmulation = CardEmulation.getInstance(nfcAdapter);
        if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_NFC_HOST_CARD_EMULATION)) {
            adapter.add(TestListItem.newCategory(this, R.string.nfc_hce_emulator_tests));

            /*
             * Only add this test when supported in platform
            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_default_route_emulator,
                    DefaultRouteEmulatorActivity.class.getName(),
                    new Intent(this, DefaultRouteEmulatorActivity.class), null));
            */
            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_protocol_params_emulator,
                    ProtocolParamsEmulatorActivity.class.getName(),
                    new Intent(this, ProtocolParamsEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_single_payment_emulator,
                    SinglePaymentEmulatorActivity.class.getName(),
                    new Intent(this, SinglePaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_dual_payment_emulator,
                    DualPaymentEmulatorActivity.class.getName(),
                    new Intent(this, DualPaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_change_default_emulator,
                    ChangeDefaultEmulatorActivity.class.getName(),
                    new Intent(this, ChangeDefaultEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_foreground_payment_emulator,
                    ForegroundPaymentEmulatorActivity.class.getName(),
                    new Intent(this, ForegroundPaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_single_non_payment_emulator,
                    SingleNonPaymentEmulatorActivity.class.getName(),
                    new Intent(this, SingleNonPaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_dual_non_payment_emulator,
                    DualNonPaymentEmulatorActivity.class.getName(),
                    new Intent(this, DualNonPaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_conflicting_non_payment_emulator,
                    ConflictingNonPaymentEmulatorActivity.class.getName(),
                    new Intent(this, ConflictingNonPaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_foreground_non_payment_emulator,
                    ForegroundNonPaymentEmulatorActivity.class.getName(),
                    new Intent(this, ForegroundNonPaymentEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_throughput_emulator,
                    ThroughputEmulatorActivity.class.getName(),
                    new Intent(this, ThroughputEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_tap_test_emulator,
                    TapTestEmulatorActivity.class.getName(),
                    new Intent(this, TapTestEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_offhost_service_emulator,
                    OffHostEmulatorActivity.class.getName(),
                    new Intent(this, OffHostEmulatorActivity.class), null));

            adapter.add(TestListItem.newTest(this, R.string.nfc_hce_on_and_offhost_service_emulator,
                    OnAndOffHostEmulatorActivity.class.getName(),
                    new Intent(this, OnAndOffHostEmulatorActivity.class), null));

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                adapter.add(TestListItem.newTest(this, R.string.nfc_hce_large_num_aids_emulator,
                        LargeNumAidsEmulatorActivity.class.getName(),
                        new Intent(this, LargeNumAidsEmulatorActivity.class), null));

                adapter.add(TestListItem.newTest(this, R.string.nfc_hce_payment_dynamic_aids_emulator,
                        DynamicAidEmulatorActivity.class.getName(),
                        new Intent(this, DynamicAidEmulatorActivity.class), null));

                if (cardEmulation.supportsAidPrefixRegistration()) {
                    adapter.add(TestListItem.newTest(this, R.string.nfc_hce_payment_prefix_aids_emulator,
                            PrefixPaymentEmulatorActivity.class.getName(),
                            new Intent(this, PrefixPaymentEmulatorActivity.class), null));

                    adapter.add(TestListItem.newTest(this, R.string.nfc_hce_payment_prefix_aids_emulator_2,
                            PrefixPaymentEmulator2Activity.class.getName(),
                            new Intent(this, PrefixPaymentEmulator2Activity.class), null));

                    adapter.add(TestListItem.newTest(this, R.string.nfc_hce_other_prefix_aids_emulator,
                            DualNonPaymentPrefixEmulatorActivity.class.getName(),
                            new Intent(this, DualNonPaymentPrefixEmulatorActivity.class), null));

                    adapter.add(TestListItem.newTest(this, R.string.nfc_hce_other_conflicting_prefix_aids_emulator,
                            ConflictingNonPaymentPrefixEmulatorActivity.class.getName(),
                            new Intent(this, ConflictingNonPaymentPrefixEmulatorActivity.class), null));
                }
            }
        }

        setTestListAdapter(adapter);
    }
}
