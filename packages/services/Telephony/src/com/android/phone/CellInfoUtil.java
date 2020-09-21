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

package com.android.phone;

import android.telephony.CellIdentity;
import android.telephony.CellIdentityCdma;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.TelephonyManager;
import android.text.BidiFormatter;
import android.text.TextDirectionHeuristics;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.OperatorInfo;

import java.util.List;

/**
 * Add static Utility functions to get information from the CellInfo object.
 * TODO: Modify {@link CellInfo} for simplify those functions
 */
public final class CellInfoUtil {
    private static final String TAG = "NetworkSelectSetting";

    private CellInfoUtil() {
    }

    /**
     * Get the network type from a CellInfo. Network types include
     * {@link TelephonyManager#NETWORK_TYPE_LTE}, {@link TelephonyManager#NETWORK_TYPE_UMTS},
     * {@link TelephonyManager#NETWORK_TYPE_GSM}, {@link TelephonyManager#NETWORK_TYPE_CDMA} and
     * {@link TelephonyManager#NETWORK_TYPE_UNKNOWN}
     * @return network types
     */
    public static int getNetworkType(CellInfo cellInfo) {
        if (cellInfo instanceof CellInfoLte) {
            return TelephonyManager.NETWORK_TYPE_LTE;
        } else if (cellInfo instanceof CellInfoWcdma) {
            return TelephonyManager.NETWORK_TYPE_UMTS;
        } else if (cellInfo instanceof CellInfoGsm) {
            return TelephonyManager.NETWORK_TYPE_GSM;
        } else if (cellInfo instanceof CellInfoCdma) {
            return TelephonyManager.NETWORK_TYPE_CDMA;
        } else {
            Log.e(TAG, "Invalid CellInfo type");
            return TelephonyManager.NETWORK_TYPE_UNKNOWN;
        }
    }

    /**
     * Get signal level as an int from 0..4.
     * @return Signal strength level
     */
    public static int getLevel(CellInfo cellInfo) {
        if (cellInfo instanceof CellInfoLte) {
            return ((CellInfoLte) cellInfo).getCellSignalStrength().getLevel();
        } else if (cellInfo instanceof CellInfoWcdma) {
            return ((CellInfoWcdma) cellInfo).getCellSignalStrength().getLevel();
        } else if (cellInfo instanceof CellInfoGsm) {
            return ((CellInfoGsm) cellInfo).getCellSignalStrength().getLevel();
        } else if (cellInfo instanceof CellInfoCdma) {
            return ((CellInfoCdma) cellInfo).getCellSignalStrength().getLevel();
        } else {
            Log.e(TAG, "Invalid CellInfo type");
            return 0;
        }
    }

    /**
     * Wrap a CellIdentity into a CellInfo.
     */
    public static CellInfo wrapCellInfoWithCellIdentity(CellIdentity cellIdentity) {
        if (cellIdentity instanceof CellIdentityLte) {
            CellInfoLte cellInfo = new CellInfoLte();
            cellInfo.setCellIdentity((CellIdentityLte) cellIdentity);
            return cellInfo;
        } else if (cellIdentity instanceof CellIdentityCdma) {
            CellInfoCdma cellInfo = new CellInfoCdma();
            cellInfo.setCellIdentity((CellIdentityCdma) cellIdentity);
            return cellInfo;
        }  else if (cellIdentity instanceof CellIdentityWcdma) {
            CellInfoWcdma cellInfo = new CellInfoWcdma();
            cellInfo.setCellIdentity((CellIdentityWcdma) cellIdentity);
            return cellInfo;
        } else if (cellIdentity instanceof CellIdentityGsm) {
            CellInfoGsm cellInfo = new CellInfoGsm();
            cellInfo.setCellIdentity((CellIdentityGsm) cellIdentity);
            return cellInfo;
        } else {
            Log.e(TAG, "Invalid CellInfo type");
            return null;
        }
    }

    /**
     * Returns the title of the network obtained in the manual search.
     *
     * @param cellInfo contains the information of the network.
     * @return Long Name if not null/empty, otherwise Short Name if not null/empty,
     * else MCCMNC string.
     */
    public static String getNetworkTitle(CellInfo cellInfo) {
        OperatorInfo oi = getOperatorInfoFromCellInfo(cellInfo);

        if (!TextUtils.isEmpty(oi.getOperatorAlphaLong())) {
            return oi.getOperatorAlphaLong();
        } else if (!TextUtils.isEmpty(oi.getOperatorAlphaShort())) {
            return oi.getOperatorAlphaShort();
        } else {
            BidiFormatter bidiFormatter = BidiFormatter.getInstance();
            return bidiFormatter.unicodeWrap(oi.getOperatorNumeric(), TextDirectionHeuristics.LTR);
        }
    }

    /**
     * Wrap a cell info into an operator info.
     */
    public static OperatorInfo getOperatorInfoFromCellInfo(CellInfo cellInfo) {
        OperatorInfo oi;
        if (cellInfo instanceof CellInfoLte) {
            CellInfoLte lte = (CellInfoLte) cellInfo;
            oi = new OperatorInfo(
                    (String) lte.getCellIdentity().getOperatorAlphaLong(),
                    (String) lte.getCellIdentity().getOperatorAlphaShort(),
                    lte.getCellIdentity().getMobileNetworkOperator());
        } else if (cellInfo instanceof CellInfoWcdma) {
            CellInfoWcdma wcdma = (CellInfoWcdma) cellInfo;
            oi = new OperatorInfo(
                    (String) wcdma.getCellIdentity().getOperatorAlphaLong(),
                    (String) wcdma.getCellIdentity().getOperatorAlphaShort(),
                    wcdma.getCellIdentity().getMobileNetworkOperator());
        } else if (cellInfo instanceof CellInfoGsm) {
            CellInfoGsm gsm = (CellInfoGsm) cellInfo;
            oi = new OperatorInfo(
                    (String) gsm.getCellIdentity().getOperatorAlphaLong(),
                    (String) gsm.getCellIdentity().getOperatorAlphaShort(),
                    gsm.getCellIdentity().getMobileNetworkOperator());
        } else if (cellInfo instanceof CellInfoCdma) {
            CellInfoCdma cdma = (CellInfoCdma) cellInfo;
            oi = new OperatorInfo(
                    (String) cdma.getCellIdentity().getOperatorAlphaLong(),
                    (String) cdma.getCellIdentity().getOperatorAlphaShort(),
                    "" /* operator numeric */);
        } else {
            Log.e(TAG, "Invalid CellInfo type");
            oi = new OperatorInfo("", "", "");
        }
        return oi;
    }

    /** Checks whether the network operator is forbidden. */
    public static boolean isForbidden(CellInfo cellInfo, List<String> forbiddenPlmns) {
        String plmn = CellInfoUtil.getOperatorInfoFromCellInfo(cellInfo).getOperatorNumeric();
        return forbiddenPlmns != null && forbiddenPlmns.contains(plmn);
    }
}
