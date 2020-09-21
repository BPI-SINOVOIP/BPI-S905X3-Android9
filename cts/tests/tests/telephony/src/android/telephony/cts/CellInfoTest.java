/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.telephony.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.os.Parcel;
import android.telephony.CellIdentityCdma;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.CellSignalStrengthCdma;
import android.telephony.CellSignalStrengthGsm;
import android.telephony.CellSignalStrengthLte;
import android.telephony.CellSignalStrengthWcdma;
import android.telephony.TelephonyManager;
import android.test.AndroidTestCase;
import android.util.Log;


import java.util.List;
import java.util.Objects;

/**
 * Test TelephonyManager.getAllCellInfo()
 * <p>
 * TODO(chesnutt): test onCellInfoChanged() once the implementation
 * of async callbacks is complete (see http://b/13788638)
 */
public class CellInfoTest extends AndroidTestCase{
    private final Object mLock = new Object();
    private TelephonyManager mTelephonyManager;
    private static ConnectivityManager mCm;
    private static final String TAG = "android.telephony.cts.CellInfoTest";
    // Maximum and minimum possible RSSI values(in dbm).
    private static final int MAX_RSSI = -10;
    private static final int MIN_RSSI = -150;
    // Maximum and minimum possible RSSP values(in dbm).
    private static final int MAX_RSRP = -44;
    private static final int MIN_RSRP = -140;
    // Maximum and minimum possible RSSQ values.
    private static final int MAX_RSRQ = -3;
    private static final int MIN_RSRQ = -35;
    // Maximum and minimum possible RSSNR values.
    private static final int MAX_RSSNR = 50;
    private static final int MIN_RSSNR = 0;
    // Maximum and minimum possible CQI values.
    private static final int MAX_CQI = 30;
    private static final int MIN_CQI = 0;

    // The followings are parameters for testing CellIdentityCdma
    // Network Id ranges from 0 to 65535.
    private static final int NETWORK_ID  = 65535;
    // CDMA System Id ranges from 0 to 32767
    private static final int SYSTEM_ID = 32767;
    // Base Station Id ranges from 0 to 65535
    private static final int BASESTATION_ID = 65535;
    // Longitude ranges from -2592000 to 2592000.
    private static final int LONGITUDE = 2592000;
    // Latitude ranges from -1296000 to 1296000.
    private static final int LATITUDE = 1296000;
    // Cell identity ranges from 0 to 268435456.

    // The followings are parameters for testing CellIdentityLte
    private static final int CI = 268435456;
    // Physical cell id ranges from 0 to 503.
    private static final int PCI = 503;
    // Tracking area code ranges from 0 to 65535.
    private static final int TAC = 65535;
    // Absolute RF Channel Number ranges from 0 to 262140.
    private static final int EARFCN = 47000;
    private static final int BANDWIDTH_LOW = 1400;  // kHz
    private static final int BANDWIDTH_HIGH = 20000;  // kHz

    // The followings are parameters for testing CellIdentityWcdma
    // Location Area Code ranges from 0 to 65535.
    private static final int LAC = 65535;
    // UMTS Cell Identity ranges from 0 to 268435455.
    private static final int CID_UMTS = 268435455;
    // Primary Scrambling Coderanges from 0 to 511.
    private static final int PSC = 511;

    // The followings are parameters for testing CellIdentityGsm
    // GSM Cell Identity ranges from 0 to 65535.
    private static final int CID_GSM = 65535;
    // GSM Absolute RF Channel Number ranges from 0 to 65535.
    private static final int ARFCN = 1024;

    // 3gpp 36.101 Sec 5.7.2
    private static final int CHANNEL_RASTER_EUTRAN = 100; //kHz

    private PackageManager mPm;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTelephonyManager =
                (TelephonyManager)getContext().getSystemService(Context.TELEPHONY_SERVICE);
        mCm = (ConnectivityManager)getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        mPm = getContext().getPackageManager();
    }

    public void testCellInfo() throws Throwable {

        if(! (mPm.hasSystemFeature(PackageManager.FEATURE_TELEPHONY))) {
            Log.d(TAG, "Skipping test that requires FEATURE_TELEPHONY");
            return;
        }

        if (mCm.getNetworkInfo(ConnectivityManager.TYPE_MOBILE) == null) {
            Log.d(TAG, "Skipping test that requires ConnectivityManager.TYPE_MOBILE");
            return;
        }

        // getAllCellInfo should never return null, and there should
        // be at least one entry.
        List<CellInfo> allCellInfo = mTelephonyManager.getAllCellInfo();
        assertNotNull("TelephonyManager.getAllCellInfo() returned NULL!", allCellInfo);
        assertTrue("TelephonyManager.getAllCellInfo() returned zero-length list!",
            allCellInfo.size() > 0);

        int numRegisteredCells = 0;
        for (CellInfo cellInfo : allCellInfo) {
            if (cellInfo.isRegistered()) {
                ++numRegisteredCells;
            }
            if (cellInfo instanceof CellInfoLte) {
                verifyLteInfo((CellInfoLte) cellInfo);
            } else if (cellInfo instanceof CellInfoWcdma) {
                verifyWcdmaInfo((CellInfoWcdma) cellInfo);
            } else if (cellInfo instanceof CellInfoGsm) {
                verifyGsmInfo((CellInfoGsm) cellInfo);
            } else if (cellInfo instanceof CellInfoCdma) {
                verifyCdmaInfo((CellInfoCdma) cellInfo);
            }
        }

        //FIXME: The maximum needs to be calculated based on the number of
        //       radios and the technologies used (ex SRLTE); however, we have
        //       not hit any of these cases yet.
        assertTrue("None or too many registered cells : " + numRegisteredCells,
                numRegisteredCells > 0 && numRegisteredCells <= 2);
    }

    private void verifyCdmaInfo(CellInfoCdma cdma) {
        verifyCellConnectionStatus(cdma.getCellConnectionStatus());
        verifyCellInfoCdmaParcelandHashcode(cdma);
        verifyCellIdentityCdma(cdma.getCellIdentity());
        verifyCellIdentityCdmaParcel(cdma.getCellIdentity());
        verifyCellSignalStrengthCdma(cdma.getCellSignalStrength());
        verifyCellSignalStrengthCdmaParcel(cdma.getCellSignalStrength());
    }

    private void verifyCellInfoCdmaParcelandHashcode(CellInfoCdma cdma) {
        Parcel p = Parcel.obtain();
        cdma.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellInfoCdma newCi = CellInfoCdma.CREATOR.createFromParcel(p);
        assertTrue(cdma.equals(newCi));
        assertEquals("hashCode() did not get right hasdCode", cdma.hashCode(), newCi.hashCode());
    }

    private void verifyCellIdentityCdma(CellIdentityCdma cdma) {
        String alphaLong = (String) cdma.getOperatorAlphaLong();
        assertNotNull("getOperatorAlphaLong() returns NULL!", alphaLong);

        String alphaShort = (String) cdma.getOperatorAlphaShort();
        assertNotNull("getOperatorAlphaShort() returns NULL!", alphaShort);

        int networkId = cdma.getNetworkId();
        assertTrue("getNetworkId() out of range [0,65535], networkId=" + networkId,
                networkId == Integer.MAX_VALUE || (networkId >= 0 && networkId <= NETWORK_ID));

        int systemId = cdma.getSystemId();
        assertTrue("getSystemId() out of range [0,32767], systemId=" + systemId,
                systemId == Integer.MAX_VALUE || (systemId >= 0 && systemId <= SYSTEM_ID));

        int basestationId = cdma.getBasestationId();
        assertTrue("getBasestationId() out of range [0,65535], basestationId=" + basestationId,
                basestationId == Integer.MAX_VALUE || (basestationId >= 0 && basestationId <= BASESTATION_ID));

        int longitude = cdma.getLongitude();
        assertTrue("getLongitude() out of range [-2592000,2592000], longitude=" + longitude,
                longitude == Integer.MAX_VALUE || (longitude >= -LONGITUDE && longitude <= LONGITUDE));

        int latitude = cdma.getLatitude();
        assertTrue("getLatitude() out of range [-1296000,1296000], latitude=" + latitude,
                latitude == Integer.MAX_VALUE || (latitude >= -LATITUDE && latitude <= LATITUDE));
    }

    private void verifyCellIdentityCdmaParcel(CellIdentityCdma cdma) {
        Parcel p = Parcel.obtain();
        cdma.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellIdentityCdma newCi = CellIdentityCdma.CREATOR.createFromParcel(p);
        assertTrue(cdma.equals(newCi));
    }

    private void verifyCellSignalStrengthCdma(CellSignalStrengthCdma cdma) {
        int level = cdma.getLevel();
        assertTrue("getLevel() out of range [0,4], level=" + level,
                level >= 0 && level <= 4);

        int asuLevel = cdma.getAsuLevel();
        assertTrue("getAsuLevel() out of range [0,97] (or 99 is unknown), asuLevel=" + asuLevel,
                asuLevel == 99 || (asuLevel >= 0 && asuLevel <= 97));

        int cdmaLevel = cdma.getCdmaLevel();
        assertTrue("getCdmaLevel() out of range [0,4], cdmaLevel=" + cdmaLevel,
                cdmaLevel >= 0 && cdmaLevel <= 4);

        int evdoLevel = cdma.getEvdoLevel();
        assertTrue("getEvdoLevel() out of range [0,4], evdoLevel=" + evdoLevel,
                evdoLevel >= 0 && evdoLevel <= 4);

        // The following four fields do not have specific limits. So just calling to verify that
        // they don't crash the phone.
        int cdmaDbm = cdma.getCdmaDbm();
        int evdoDbm = cdma.getEvdoDbm();
        cdma.getCdmaEcio();
        cdma.getEvdoEcio();

        int dbm = (cdmaDbm < evdoDbm) ? cdmaDbm : evdoDbm;
        assertEquals("getDbm() did not get correct value", dbm, cdma.getDbm());

        int evdoSnr = cdma.getEvdoSnr();
        assertTrue("getEvdoSnr() out of range [0,8], evdoSnr=" + evdoSnr,
                (evdoSnr == Integer.MAX_VALUE) || (evdoSnr >= 0 && evdoSnr <= 8));
    }

    private void verifyCellSignalStrengthCdmaParcel(CellSignalStrengthCdma cdma) {
        Parcel p = Parcel.obtain();
        cdma.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellSignalStrengthCdma newCss = CellSignalStrengthCdma.CREATOR.createFromParcel(p);
        assertEquals(cdma, newCss);
    }

    // Verify lte cell information is within correct range.
    private void verifyLteInfo(CellInfoLte lte) {
        verifyCellConnectionStatus(lte.getCellConnectionStatus());
        verifyCellInfoLteParcelandHashcode(lte);
        verifyCellIdentityLte(lte.getCellIdentity());
        verifyCellIdentityLteParcel(lte.getCellIdentity());
        verifyCellSignalStrengthLte(lte.getCellSignalStrength());
        verifyCellSignalStrengthLteParcel(lte.getCellSignalStrength());
    }

    private void verifyCellInfoLteParcelandHashcode(CellInfoLte lte) {
        Parcel p = Parcel.obtain();
        lte.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellInfoLte newCi = CellInfoLte.CREATOR.createFromParcel(p);
        assertTrue(lte.equals(newCi));
        assertEquals("hashCode() did not get right hasdCode", lte.hashCode(), newCi.hashCode());
    }


    private void verifyCellIdentityLte(CellIdentityLte lte) {
        int mcc = lte.getMcc();
        // getMcc() returns Integer.MAX_VALUE if mccStr is null.
        assertTrue("getMcc() out of range [0, 999], mcc=" + mcc,
                (mcc >= 0 && mcc <= 999) || mcc == Integer.MAX_VALUE);

        int mnc = lte.getMnc();
        // getMnc() returns Integer.MAX_VALUE if mccStr is null.
        assertTrue("getMnc() out of range [0, 999], mnc=" + mnc,
                (mnc >= 0 && mnc <= 999) || mnc == Integer.MAX_VALUE);

        // Cell identity ranges from 0 to 268435456.
        int ci = lte.getCi();
        assertTrue("getCi() out of range [0,268435456], ci=" + ci,
                (ci == Integer.MAX_VALUE) || (ci >= 0 && ci <= CI));

        // Verify LTE physical cell id information.
        // Only physical cell id is available for LTE neighbor.
        int pci = lte.getPci();
        // Physical cell id should be within [0, 503].
        assertTrue("getPci() out of range [0, 503], pci=" + pci,
                (pci== Integer.MAX_VALUE) || (pci >= 0 && pci <= PCI));

        // Tracking area code ranges from 0 to 65535.
        int tac = lte.getTac();
        assertTrue("getTac() out of range [0,65535], tac=" + tac,
                (tac == Integer.MAX_VALUE) || (tac >= 0 && tac <= TAC));

        int bw = lte.getBandwidth();
        assertTrue("getBandwidth out of range [1400, 20000] | Integer.Max_Value, bw=",
                bw == Integer.MAX_VALUE || bw >= BANDWIDTH_LOW && bw <= BANDWIDTH_HIGH);

        int earfcn = lte.getEarfcn();
        // Reference 3GPP 36.101 Table 5.7.3-1
        // As per NOTE 1 in the table, although 0-6 are valid channel numbers for
        // LTE, the reported EARFCN is the center frequency, rendering these channels
        // out of the range of the narrowest 1.4Mhz deployment.
        int minEarfcn = 7;
        if (bw != Integer.MAX_VALUE) {
            // The number of channels used by a cell is equal to the cell bandwidth divided
            // by the channel raster (bandwidth of a channel). The center channel is the channel
            // the n/2-th channel where n is the number of channels, and since it is the center
            // channel that is reported as the channel number for a cell, we can exclude any channel
            // numbers within a band that would place the bottom of a cell's bandwidth below the
            // edge of the band. For channel numbers in Band 1, the EARFCN numbering starts from
            // channel 0, which means that we can exclude from the valid range channels starting
            // from 0 and numbered less than half the total number of channels occupied by a cell.
            minEarfcn = bw / CHANNEL_RASTER_EUTRAN / 2;
        }
        assertTrue("getEarfcn() out of range [7,47000], earfcn=" + earfcn,
                earfcn == Integer.MAX_VALUE || (earfcn >= minEarfcn && earfcn <= EARFCN));

        String mccStr = lte.getMccString();
        // mccStr is set as NULL if empty, unknown or invalid.
        assertTrue("getMccString() out of range [0, 999], mcc=" + mccStr,
                mccStr == null || mccStr.matches("^[0-9]{3}$"));

        String mncStr = lte.getMncString();
        // mncStr is set as NULL if empty, unknown or invalid.
        assertTrue("getMncString() out of range [0, 999], mnc=" + mncStr,
                mncStr == null || mncStr.matches("^[0-9]{2,3}$"));

        String mobileNetworkOperator = lte.getMobileNetworkOperator();
        assertTrue("getMobileNetworkOperator() out of range [0, 999999], mobileNetworkOperator="
                        + mobileNetworkOperator,
                mobileNetworkOperator == null
                        || mobileNetworkOperator.matches("^[0-9]{5,6}$"));

        String alphaLong = (String) lte.getOperatorAlphaLong();
        assertNotNull("getOperatorAlphaLong() returns NULL!", alphaLong);

        String alphaShort = (String) lte.getOperatorAlphaShort();
        assertNotNull("getOperatorAlphaShort() returns NULL!", alphaShort);
    }

    private void verifyCellIdentityLteParcel(CellIdentityLte lte) {
        Parcel p = Parcel.obtain();
        lte.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellIdentityLte newci = CellIdentityLte.CREATOR.createFromParcel(p);
        assertEquals(lte, newci);
    }

    private void verifyCellSignalStrengthLte(CellSignalStrengthLte cellSignalStrengthLte) {
        verifyRssiDbm(cellSignalStrengthLte.getDbm());

        //Integer.MAX_VALUE indicates an unavailable field
        int rsrp = cellSignalStrengthLte.getRsrp();
        // RSRP is being treated as RSSI in LTE (they are similar but not quite right)
        // so reusing the constants here.
        assertTrue("getRsrp() out of range, rsrp=" + rsrp, rsrp >= MIN_RSRP && rsrp <= MAX_RSRP);

        int rsrq = cellSignalStrengthLte.getRsrq();
        assertTrue("getRsrq() out of range | Integer.MAX_VALUE, rsrq=" + rsrq,
            rsrq == Integer.MAX_VALUE || (rsrq >= MIN_RSRQ && rsrq <= MAX_RSRQ));

        int rssnr = cellSignalStrengthLte.getRssnr();
        assertTrue("getRssnr() out of range | Integer.MAX_VALUE, rssnr=" + rssnr,
            rssnr == Integer.MAX_VALUE || (rssnr >= MIN_RSSNR && rssnr <= MAX_RSSNR));

        int cqi = cellSignalStrengthLte.getCqi();
        assertTrue("getCqi() out of range | Integer.MAX_VALUE, cqi=" + cqi,
            cqi == Integer.MAX_VALUE || (cqi >= MIN_CQI && cqi <= MAX_CQI));

        int ta = cellSignalStrengthLte.getTimingAdvance();
        assertTrue("getTimingAdvance() invalid [0-1282] | Integer.MAX_VALUE, ta=" + ta,
                ta == Integer.MAX_VALUE || (ta >= 0 && ta <=1282));

        int level = cellSignalStrengthLte.getLevel();
        assertTrue("getLevel() out of range [0,4], level=" + level, level >= 0 && level <= 4);

        int asuLevel = cellSignalStrengthLte.getAsuLevel();
        assertTrue("getAsuLevel() out of range [0,97] (or 99 is unknown), asuLevel=" + asuLevel,
                (asuLevel == 99) || (asuLevel >= 0 && asuLevel <= 97));

        int timingAdvance = cellSignalStrengthLte.getTimingAdvance();
        assertTrue("getTimingAdvance() out of range [0,1282], timingAdvance=" + timingAdvance,
                timingAdvance == Integer.MAX_VALUE || (timingAdvance >= 0 && timingAdvance <= 1282));
    }

    private void verifyCellSignalStrengthLteParcel(CellSignalStrengthLte cellSignalStrengthLte) {
        Parcel p = Parcel.obtain();
        cellSignalStrengthLte.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellSignalStrengthLte newCss = CellSignalStrengthLte.CREATOR.createFromParcel(p);
        assertEquals(cellSignalStrengthLte, newCss);
    }

    // Verify wcdma cell information is within correct range.
    private void verifyWcdmaInfo(CellInfoWcdma wcdma) {
        verifyCellConnectionStatus(wcdma.getCellConnectionStatus());
        verifyCellInfoWcdmaParcelandHashcode(wcdma);
        verifyCellIdentityWcdma(wcdma.getCellIdentity());
        verifyCellIdentityWcdmaParcel(wcdma.getCellIdentity());
        verifyCellSignalStrengthWcdma(wcdma.getCellSignalStrength());
        verifyCellSignalStrengthWcdmaParcel(wcdma.getCellSignalStrength());
    }

    private void verifyCellInfoWcdmaParcelandHashcode(CellInfoWcdma wcdma) {
        Parcel p = Parcel.obtain();
        wcdma.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellInfoWcdma newCi = CellInfoWcdma.CREATOR.createFromParcel(p);
        assertTrue(wcdma.equals(newCi));
        assertEquals("hashCode() did not get right hasdCode", wcdma.hashCode(), newCi.hashCode());
    }

    private void verifyCellIdentityWcdma(CellIdentityWcdma wcdma) {
        int mcc = wcdma.getMcc();
        // getMcc() returns Integer.MAX_VALUE if mccStr is null.
        assertTrue("getMcc() out of range [0, 999], mcc=" + mcc,
                (mcc >= 0 && mcc <= 999) || mcc == Integer.MAX_VALUE);

        int mnc = wcdma.getMnc();
        // getMnc() returns Integer.MAX_VALUE if mccStr is null.
        assertTrue("getMnc() out of range [0, 999], mnc=" + mnc,
                (mnc >= 0 && mnc <= 999) || mnc == Integer.MAX_VALUE);

        int lac = wcdma.getLac();
        assertTrue("getLac() out of range [0, 65535], lac=" + lac,
                (lac >= 0 && lac <= LAC) || lac == Integer.MAX_VALUE);

        int cid = wcdma.getCid();
        assertTrue("getCid() out of range [0, 268435455], cid=" + cid,
                (cid >= 0 && cid <= CID_UMTS) || cid == Integer.MAX_VALUE);

        // Verify wcdma primary scrambling code information.
        // Primary scrambling code should be within [0, 511].
        int psc = wcdma.getPsc();
        assertTrue("getPsc() out of range [0, 511], psc=" + psc,
                (psc >= 0 && psc <= PSC) || psc == Integer.MAX_VALUE);

        String mccStr = wcdma.getMccString();
        // mccStr is set as NULL if empty, unknown or invalid.
        assertTrue("getMccString() out of range [0, 999], mcc=" + mccStr,
                mccStr == null || mccStr.matches("^[0-9]{3}$"));

        String mncStr = wcdma.getMncString();
        // mncStr is set as NULL if empty, unknown or invalid.
        assertTrue("getMncString() out of range [0, 999], mnc=" + mncStr,
                mncStr == null || mncStr.matches("^[0-9]{2,3}$"));

        String mobileNetworkOperator = wcdma.getMobileNetworkOperator();
        assertTrue("getMobileNetworkOperator() out of range [0, 999999], mobileNetworkOperator="
                        + mobileNetworkOperator,
                mobileNetworkOperator == null
                        || mobileNetworkOperator.matches("^[0-9]{5,6}$"));

        int uarfcn = wcdma.getUarfcn();
        // Reference 3GPP 25.101 Table 5.2
        // From Appendix E.1, even though UARFCN is numbered from 400, the minumum
        // usable channel is 412 due to the fixed bandwidth of 5Mhz
        assertTrue("getUarfcn() out of range [412,11000], uarfcn=" + uarfcn,
                uarfcn >= 412 && uarfcn <= 11000);

        String alphaLong = (String) wcdma.getOperatorAlphaLong();
        assertNotNull("getOperatorAlphaLong() returns NULL!", alphaLong);

        String alphaShort = (String) wcdma.getOperatorAlphaShort();
        assertNotNull("getOperatorAlphaShort() returns NULL!", alphaShort);
    }

    private void verifyCellIdentityWcdmaParcel(CellIdentityWcdma wcdma) {
        Parcel p = Parcel.obtain();
        wcdma.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellIdentityWcdma newci = CellIdentityWcdma.CREATOR.createFromParcel(p);
        assertEquals(wcdma, newci);
    }

    private void verifyCellSignalStrengthWcdma(CellSignalStrengthWcdma wcdma) {
        verifyRssiDbm(wcdma.getDbm());

        // Dbm here does not have specific limits. So just calling to verify that it does not crash
        // the phone
        wcdma.getDbm();

        int asuLevel = wcdma.getAsuLevel();
        assertTrue("getLevel() out of range [0,31] (or 99 is unknown), level=" + asuLevel,
                asuLevel == 99 || (asuLevel >= 0 && asuLevel <= 31));

        int level = wcdma.getLevel();
        assertTrue("getLevel() out of range [0,4], level=" + level, level >= 0 && level <= 4);
    }

    private void verifyCellSignalStrengthWcdmaParcel(CellSignalStrengthWcdma wcdma) {
        Parcel p = Parcel.obtain();
        wcdma.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellSignalStrengthWcdma newCss = CellSignalStrengthWcdma.CREATOR.createFromParcel(p);
        assertEquals(wcdma, newCss);
    }

    // Verify gsm cell information is within correct range.
    private void verifyGsmInfo(CellInfoGsm gsm) {
        verifyCellConnectionStatus(gsm.getCellConnectionStatus());
        verifyCellInfoWcdmaParcelandHashcode(gsm);
        verifyCellIdentityGsm(gsm.getCellIdentity());
        verifyCellIdentityGsmParcel(gsm.getCellIdentity());
        verifyCellSignalStrengthGsm(gsm.getCellSignalStrength());
        verifyCellSignalStrengthGsmParcel(gsm.getCellSignalStrength());
    }

    private void verifyCellInfoWcdmaParcelandHashcode(CellInfoGsm gsm) {
        Parcel p = Parcel.obtain();
        gsm.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellInfoGsm newCi = CellInfoGsm.CREATOR.createFromParcel(p);
        assertTrue(gsm.equals(newCi));
        assertEquals("hashCode() did not get right hasdCode", gsm.hashCode(), newCi.hashCode());
    }

    private void verifyCellIdentityGsm(CellIdentityGsm gsm) {
        // Local area code and cellid should be with [0, 65535].
        int lac = gsm.getLac();
        assertTrue("getLac() out of range [0, 65535], lac=" + lac,
                lac == Integer.MAX_VALUE || (lac >= 0 && lac <= LAC));
        int cid = gsm.getCid();
        assertTrue("getCid() out range [0, 65535], cid=" + cid,
                cid== Integer.MAX_VALUE || (cid >= 0 && cid <= CID_GSM));

        int arfcn = gsm.getArfcn();
        // Reference 3GPP 45.005 Table 2-2
        assertTrue("getArfcn() out of range [0,1024], arfcn=" + arfcn,
                arfcn == Integer.MAX_VALUE || (arfcn >= 0 && arfcn <= ARFCN));

        String alphaLong = (String) gsm.getOperatorAlphaLong();
        assertNotNull("getOperatorAlphaLong() returns NULL!", alphaLong);

        String alphaShort = (String) gsm.getOperatorAlphaShort();
        assertNotNull("getOperatorAlphaShort() returns NULL!", alphaShort);

        String mccStr = gsm.getMccString();
        // mccStr is set as NULL if empty, unknown or invalid.
        assertTrue("getMccString() out of range [0, 999], mcc=" + mccStr,
                mccStr == null || mccStr.matches("^[0-9]{3}$"));
        String mncStr = gsm.getMncString();
        // mncStr is set as NULL if empty, unknown or invalid.
        assertTrue("getMncString() out of range [0, 999], mnc=" + mncStr,
                mncStr == null || mncStr.matches("^[0-9]{2,3}$"));

        String mobileNetworkOperator = gsm.getMobileNetworkOperator();
        assertTrue("getMobileNetworkOperator() out of range [0, 999999], mobileNetworkOperator="
                        + mobileNetworkOperator,
                mobileNetworkOperator == null
                        || mobileNetworkOperator.matches("^[0-9]{5,6}$"));

        int mcc = gsm.getMcc();
        // getMcc() returns Integer.MAX_VALUE if mccStr is null.
        assertTrue("getMcc() out of range [0, 999], mcc=" + mcc,
                (mcc >= 0 && mcc <= 999) || mcc == Integer.MAX_VALUE);
        int mnc = gsm.getMnc();
        // getMnc() returns Integer.MAX_VALUE if mccStr is null.
        assertTrue("getMnc() out of range [0, 999], mnc=" + mnc,
                (mnc >= 0 && mnc <= 999) || mnc == Integer.MAX_VALUE);

        int bsic = gsm.getBsic();
        // TODO(b/32774471) - Bsic should always be valid
        //assertTrue("getBsic() out of range [0,63]", bsic >= 0 && bsic <=63);
    }

    private void verifyCellIdentityGsmParcel(CellIdentityGsm gsm) {
        Parcel p = Parcel.obtain();
        gsm.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellIdentityGsm newci = CellIdentityGsm.CREATOR.createFromParcel(p);
        assertEquals(gsm, newci);
    }

    private void verifyCellSignalStrengthGsm(CellSignalStrengthGsm gsm) {
        verifyRssiDbm(gsm.getDbm());

        int level = gsm.getLevel();
        assertTrue("getLevel() out of range [0,4], level=" + level, level >= 0 && level <= 4);

        int ta = gsm.getTimingAdvance();
        assertTrue("getTimingAdvance() out of range [0,219] | Integer.MAX_VALUE, ta=" + ta,
                ta == Integer.MAX_VALUE || (ta >= 0 && ta <= 219));

        // Dbm here does not have specific limits. So just calling to verify that it does not
        // crash the phone
        gsm.getDbm();

        int asuLevel = gsm.getAsuLevel();
        assertTrue("getLevel() out of range [0,31] (or 99 is unknown), level=" + asuLevel,
                asuLevel == 99 || (asuLevel >=0 && asuLevel <= 31));
    }

    private void verifyCellSignalStrengthGsmParcel(CellSignalStrengthGsm gsm) {
        Parcel p = Parcel.obtain();
        gsm.writeToParcel(p, 0);
        p.setDataPosition(0);

        CellSignalStrengthGsm newCss = CellSignalStrengthGsm.CREATOR.createFromParcel(p);
        assertEquals(gsm, newCss);
    }

    // Rssi(in dbm) should be within [MIN_RSSI, MAX_RSSI].
    private void verifyRssiDbm(int dbm) {
        assertTrue("getCellSignalStrength().getDbm() out of range, dbm=" + dbm,
                dbm >= MIN_RSSI && dbm <= MAX_RSSI);
    }

    private void verifyCellConnectionStatus(int status) {
        assertTrue("getCellConnectionStatus() invalid [0,2] | Integer.MAX_VALUE, status=",
            status == CellInfo.CONNECTION_NONE
                || status == CellInfo.CONNECTION_PRIMARY_SERVING
                || status == CellInfo.CONNECTION_SECONDARY_SERVING
                || status == CellInfo.CONNECTION_UNKNOWN);
    }
}
