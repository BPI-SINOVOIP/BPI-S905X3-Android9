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

package com.googlecode.android_scripting.facade.wifi;

import static android.net.wifi.ScanResult.FLAG_80211mc_RESPONDER;
import static android.net.wifi.ScanResult.FLAG_PASSPOINT_NETWORK;

import android.net.wifi.ScanResult;

import org.apache.commons.codec.binary.Base64Codec;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Set of utility methods to parse JSON constructs back into classes. Usually counterparts to
 * utility methods which automatically do the conversion from classes to JSON constructs and
 * are in JsonBuilder.java.
 */
public class WifiJsonParser {
    /**
     * Converts a JSON representation of a ScanResult to an actual ScanResult object. Mirror of
     * the code in
     * {@link com.googlecode.android_scripting.jsonrpc.JsonBuilder#buildJsonScanResult(ScanResult)}.
     *
     * @param j JSON object representing a ScanResult.
     * @return a ScanResult object
     * @throws JSONException on any JSON errors
     */
    public static ScanResult getScanResult(JSONObject j) throws JSONException {
        if (j == null) {
            return null;
        }

        ScanResult scanResult = new ScanResult();

        if (j.has("BSSID")) {
            scanResult.BSSID = j.getString("BSSID");
        }
        if (j.has("SSID")) {
            scanResult.SSID = j.getString("SSID");
        }
        if (j.has("frequency")) {
            scanResult.frequency = j.getInt("frequency");
        }
        if (j.has("level")) {
            scanResult.level = j.getInt("level");
        }
        if (j.has("capabilities")) {
            scanResult.capabilities = j.getString("capabilities");
        }
        if (j.has("timestamp")) {
            scanResult.timestamp = j.getLong("timestamp");
        }
        if (j.has("centerFreq0")) {
            scanResult.centerFreq0 = j.getInt("centerFreq0");
        }
        if (j.has("centerFreq1")) {
            scanResult.centerFreq1 = j.getInt("centerFreq1");
        }
        if (j.has("channelWidth")) {
            scanResult.channelWidth = j.getInt("channelWidth");
        }
        if (j.has("distanceCm")) {
            scanResult.distanceCm = j.getInt("distanceCm");
        }
        if (j.has("distanceSdCm")) {
            scanResult.distanceSdCm = j.getInt("distanceSdCm");
        }
        if (j.has("is80211McRTTResponder") && j.getBoolean("is80211McRTTResponder")) {
            scanResult.setFlag(FLAG_80211mc_RESPONDER);
        }
        if (j.has("passpointNetwork") && j.getBoolean("passpointNetwork")) {
            scanResult.setFlag(FLAG_PASSPOINT_NETWORK);
        }
        if (j.has("numUsage")) {
            scanResult.numUsage = j.getInt("numUsage");
        }
        if (j.has("seen")) {
            scanResult.seen = j.getLong("seen");
        }
        if (j.has("untrusted")) {
            scanResult.untrusted = j.getBoolean("untrusted");
        }
        if (j.has("operatorFriendlyName")) {
            scanResult.operatorFriendlyName = j.getString("operatorFriendlyName");
        }
        if (j.has("venueName")) {
            scanResult.venueName = j.getString("venueName");
        }
        if (j.has("InformationElements")) {
            JSONArray jIes = j.getJSONArray("InformationElements");
            if (jIes.length() > 0) {
                scanResult.informationElements = new ScanResult.InformationElement[jIes.length()];
                for (int i = 0; i < jIes.length(); ++i) {
                    ScanResult.InformationElement scanIe = new ScanResult.InformationElement();
                    JSONObject jIe = jIes.getJSONObject(i);
                    scanIe.id = jIe.getInt("id");
                    scanIe.bytes = Base64Codec.decodeBase64(jIe.getString("bytes"));
                    scanResult.informationElements[i] = scanIe;
                }
            }
        }
        if (j.has("radioChainInfos")) {
            JSONArray jRinfos = j.getJSONArray("radioChainInfos");
            if (jRinfos.length() > 0) {
                scanResult.radioChainInfos = new ScanResult.RadioChainInfo[jRinfos.length()];
                for (int i = 0; i < jRinfos.length(); i++) {
                    ScanResult.RadioChainInfo item = new ScanResult.RadioChainInfo();
                    JSONObject jRinfo = jRinfos.getJSONObject(i);
                    item.id = jRinfo.getInt("id");
                    item.level = jRinfo.getInt("level");
                    scanResult.radioChainInfos[i] = item;
                }
            }
        }

        return scanResult;
    }

    /**
     * Converts a JSONArray toa a list of ScanResult.
     *
     * @param j JSONArray representing a collection of ScanResult objects
     * @return a list of ScanResult objects
     * @throws JSONException on any JSON error
     */
    public static List<ScanResult> getScanResults(JSONArray j) throws JSONException {
        if (j == null || j.length() == 0) {
            return null;
        }

        ArrayList<ScanResult> scanResults = new ArrayList<>(j.length());
        for (int i = 0; i < j.length(); ++i) {
            scanResults.add(getScanResult(j.getJSONObject(i)));
        }

        return scanResults;
    }
}
