/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import java.lang.String;
import java.util.ArrayList;
import java.util.List;
import android.text.TextUtils;

import com.android.ims.internal.uce.presence.PresTupleInfo;
import com.android.ims.internal.uce.presence.PresRlmiInfo;
import com.android.ims.internal.uce.presence.PresResInfo;
import com.android.ims.internal.uce.presence.PresResInstanceInfo;

import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.RcsPresenceInfo.ServiceType;
import com.android.ims.RcsPresenceInfo.ServiceState;

import com.android.ims.internal.Logger;

public class PresenceInfoParser{
    /*
     * The logger
     */
    static private Logger logger = Logger.getLogger("PresenceInfoParser");

    public PresenceInfoParser() {
    }

    static public RcsPresenceInfo getPresenceInfoFromTuple(String pPresentityURI,
            PresTupleInfo[] pTupleInfo){
        logger.debug("getPresenceInfoFromTuple: pPresentityURI=" + pPresentityURI +
                " pTupleInfo=" + pTupleInfo);

        if(pPresentityURI == null){
            logger.error("pPresentityURI=" + pPresentityURI);
            return null;
        }

        String contactNumber = getPhoneFromUri(pPresentityURI);
        // Now that we got notification if the content didn't indicate it is not Volte
        // then it should be Volte enabled. So default to Volte enabled.
        int volteStatus = RcsPresenceInfo.VolteStatus.VOLTE_ENABLED;

        int ipVoiceCallState = RcsPresenceInfo.ServiceState.UNKNOWN;
        String ipVoiceCallServiceNumber = null;
         // We use the timestamp which when we received it instead of the one in PDU
        long ipVoiceCallTimestamp = System.currentTimeMillis();

        int ipVideoCallState = RcsPresenceInfo.ServiceState.UNKNOWN;
        String ipVideoCallServiceNumber = null;
        long ipVideoCallTimestamp = System.currentTimeMillis();

        if( pTupleInfo == null){
            logger.debug("pTupleInfo=null");
            return (new RcsPresenceInfo(contactNumber, volteStatus,
                ipVoiceCallState, ipVoiceCallServiceNumber, ipVoiceCallTimestamp,
                ipVideoCallState, ipVideoCallServiceNumber, ipVideoCallTimestamp));
        }

        for(int i = 0; i < pTupleInfo.length; i++){
            // Video call has high priority. If it supports video call it will support voice call.
            String featureTag = pTupleInfo[i].getFeatureTag();
            logger.debug("getFeatureTag " + i + ": " + featureTag);
            if(featureTag.equals(
                "+g.3gpp.icsi-ref=\"urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel\";video") ||
                featureTag.equals("+g.gsma.rcs.telephony=\"cs,volte\";video")) {
                logger.debug("Got Volte and VT enabled tuple");
                ipVoiceCallState = RcsPresenceInfo.ServiceState.ONLINE;
                ipVideoCallState = RcsPresenceInfo.ServiceState.ONLINE;

                if(pTupleInfo[i].getContactUri() != null){
                    ipVideoCallServiceNumber = getPhoneFromUri(
                            pTupleInfo[i].getContactUri().toString());
                }
            } else if(featureTag.equals("+g.gsma.rcs.telephony=\"cs\";video")) {
                logger.debug("Got Volte disabled but VT enabled tuple");
                ipVoiceCallState = RcsPresenceInfo.ServiceState.OFFLINE;
                ipVideoCallState = RcsPresenceInfo.ServiceState.ONLINE;

                if(pTupleInfo[i].getContactUri() != null){
                    ipVideoCallServiceNumber = getPhoneFromUri(
                            pTupleInfo[i].getContactUri().toString());
                }
            }else{
                if(featureTag.equals(
                    "+g.3gpp.icsi-ref=\"urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel\"") ||
                    featureTag.equals("+g.gsma.rcs.telephony=\"cs,volte\"")) {

                    logger.debug("Got Volte only tuple");
                    ipVoiceCallState = RcsPresenceInfo.ServiceState.ONLINE;
                    // "OR" for multiple tuples.
                    if(RcsPresenceInfo.ServiceState.UNKNOWN == ipVideoCallState) {
                        ipVideoCallState = RcsPresenceInfo.ServiceState.OFFLINE;
                    }

                    if(pTupleInfo[i].getContactUri() != null){
                        ipVoiceCallServiceNumber = getPhoneFromUri(
                                pTupleInfo[i].getContactUri().toString());
                    }
                }else{
                    logger.debug("Ignoring feature tag: " + pTupleInfo[i].getFeatureTag());
                }
           }
        }

        RcsPresenceInfo retPresenceInfo = new RcsPresenceInfo(contactNumber,volteStatus,
                ipVoiceCallState, ipVoiceCallServiceNumber, ipVoiceCallTimestamp,
                ipVideoCallState, ipVideoCallServiceNumber, ipVideoCallTimestamp);

        logger.debug("getPresenceInfoFromTuple: " + retPresenceInfo);

        return retPresenceInfo;
    }

    static private void addPresenceInfo(ArrayList<RcsPresenceInfo> presenceInfoList,
            RcsPresenceInfo presenceInfo){
        logger.debug("addPresenceInfo presenceInfoList=" + presenceInfoList +
                    " presenceInfo=" + presenceInfo);

        if(presenceInfoList == null || presenceInfo == null){
            logger.debug("addPresenceInfo presenceInfoList=" + presenceInfoList +
                    " presenceInfo=" + presenceInfo);
            return;
        }

        for(int i=0; i< presenceInfoList.size(); i++){
            RcsPresenceInfo presenceInfoTmp = presenceInfoList.get(i);
            if((presenceInfoTmp != null) && (presenceInfoTmp.getContactNumber() != null) &&
                    presenceInfoTmp.getContactNumber().equalsIgnoreCase(
                    presenceInfo.getContactNumber())){
                // merge the information
                presenceInfoList.set(i, new RcsPresenceInfo(presenceInfoTmp.getContactNumber(),
                        presenceInfoTmp.getVolteStatus(),
                        ((ServiceState.ONLINE == presenceInfo.getServiceState(
                                ServiceType.VOLTE_CALL)) ||
                                (ServiceState.ONLINE == presenceInfoTmp.getServiceState(
                                ServiceType.VOLTE_CALL)))?ServiceState.ONLINE:
                                    presenceInfoTmp.getServiceState(ServiceType.VOLTE_CALL),
                        presenceInfoTmp.getServiceContact(ServiceType.VOLTE_CALL),
                        presenceInfoTmp.getTimeStamp(ServiceType.VOLTE_CALL),
                        ((ServiceState.ONLINE == presenceInfo.getServiceState(
                                ServiceType.VT_CALL)) ||
                                (ServiceState.ONLINE == presenceInfoTmp.getServiceState(
                                ServiceType.VT_CALL)))?ServiceState.ONLINE:
                                    presenceInfoTmp.getServiceState(ServiceType.VT_CALL),
                        presenceInfoTmp.getServiceContact(ServiceType.VOLTE_CALL),
                        presenceInfoTmp.getTimeStamp(ServiceType.VOLTE_CALL)));
                return;
            }
        }

        // didn't merge, so add the new one.
        presenceInfoList.add(presenceInfo);
    }

    static public RcsPresenceInfo[] getPresenceInfosFromPresenceRes(
            PresRlmiInfo pRlmiInfo, PresResInfo[] pRcsPresenceInfo) {
        if(pRcsPresenceInfo == null){
            logger.debug("getPresenceInfosFromPresenceRes pRcsPresenceInfo=null");
            return null;
        }

        ArrayList<RcsPresenceInfo> presenceInfoList = new ArrayList<RcsPresenceInfo>();
        for(int i=0; i < pRcsPresenceInfo.length; i++ ) {
            if(pRcsPresenceInfo[i].getInstanceInfo() == null){
                logger.error("invalid data getInstanceInfo = null");
                continue;
            }

            String resUri = pRcsPresenceInfo[i].getResUri();
            if(resUri == null){
                logger.error("invalid data getResUri = null");
                continue;
            }

            String contactNumber = getPhoneFromUri(resUri);
            if(contactNumber == null){
                logger.error("invalid data contactNumber=null");
                continue;
            }

            if(pRcsPresenceInfo[i].getInstanceInfo().getResInstanceState() ==
                    PresResInstanceInfo.UCE_PRES_RES_INSTANCE_STATE_TERMINATED){
                logger.debug("instatance state is terminated");
                String reason = pRcsPresenceInfo[i].getInstanceInfo().getReason();
                if(reason != null){
                    reason = reason.toLowerCase();
                    // Noresource: 404. Device shall consider the resource as non-EAB capable
                    // Rejected: 403. Device shall consider the resource as non-EAB  capable
                    // Deactivated: 408, 481, 603, other 4xx, 5xx 6xx. Ignore.
                    // Give Up: 480. Ignore.
                    // Probation: 503. Ignore.
                    if(reason.equals("rejected") || reason.equals("noresource")){
                        RcsPresenceInfo presenceInfo = new RcsPresenceInfo(contactNumber,
                                RcsPresenceInfo.VolteStatus.VOLTE_DISABLED,
                                RcsPresenceInfo.ServiceState.OFFLINE, null,
                                System.currentTimeMillis(),
                                RcsPresenceInfo.ServiceState.OFFLINE, null,
                                System.currentTimeMillis());
                        addPresenceInfo(presenceInfoList, presenceInfo);
                        logger.debug("reason=" + reason + " presenceInfo=" + presenceInfo);
                        continue;
                    }
                }
            }

            RcsPresenceInfo presenceInfo = getPresenceInfoFromTuple(resUri,
                    pRcsPresenceInfo[i].getInstanceInfo().getTupleInfo());
            if(presenceInfo != null){
                addPresenceInfo(presenceInfoList, presenceInfo);
            }else{
                logger.debug("presenceInfo["+ i + "] = null");
                addPresenceInfo(presenceInfoList, getPresenceInfoFromTuple(resUri, null));
            }
        }

        logger.debug("getPresenceInfoFromPresenceRes, presenceInfos:" + presenceInfoList);
        if(presenceInfoList.size() == 0){
            return null;
        }

        RcsPresenceInfo[] theArray = new RcsPresenceInfo[presenceInfoList.size()];
        return (RcsPresenceInfo[])presenceInfoList.toArray(theArray);
    }

    static public String getPhoneFromUri(String uriValue) {
        if(uriValue == null){
            return null;
        }

        int startIndex = uriValue.indexOf(":", 0);
        int endIndex = uriValue.indexOf("@", startIndex);

        logger.debug("getPhoneFromUri uriValue=" + uriValue +
            " startIndex=" + startIndex + " endIndex=" + endIndex);

        String number = uriValue;
        if(endIndex != -1){
            number = uriValue.substring(0, endIndex);
        }

        if (startIndex != -1) {
            return number = number.substring(startIndex + 1);
        }

        return number;
    }
}
