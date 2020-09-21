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

import android.content.Context;

import com.android.ims.ImsConfig;
import com.android.ims.ImsException;
import com.android.ims.ImsManager;

import com.android.ims.internal.Logger;

public class PresenceSetting {
    private static Logger logger = Logger.getLogger("PresenceSetting");
    private static Context sContext = null;

    public static void init(Context context) {
        sContext = context;
    }

    public static long getCapabilityPollInterval() {
        long value = -1;
        if (sContext != null) {
            ImsManager imsManager = ImsManager.getInstance(sContext, 0);
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        value = imsConfig.getProvisionedValue(
                                ImsConfig.ConfigConstants.CAPABILITIES_POLL_INTERVAL);
                        logger.debug("Read ImsConfig_CapabilityPollInterval: " + value);
                    }
                } catch (ImsException ex) {
                }
            }
        }
        if (value <= 0) {
            value = 604800L;
            logger.error("Failed to get CapabilityPollInterval, the default: " + value);
        }
        return value;
    }

    public static long getCapabilityCacheExpiration() {
        long value = -1;
        if (sContext != null) {
            ImsManager imsManager = ImsManager.getInstance(sContext, 0);
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        value = imsConfig.getProvisionedValue(
                                ImsConfig.ConfigConstants.CAPABILITIES_CACHE_EXPIRATION);
                        logger.debug("Read ImsConfig_CapabilityCacheExpiration: " + value);
                    }
                } catch (ImsException ex) {
                }
            }
        }
        if (value <= 0) {
            value = 7776000L;
            logger.error("Failed to get CapabilityCacheExpiration, the default: " + value);
        }
        return value;
    }

    public static int getPublishTimer() {
        int value = -1;
        if (sContext != null) {
            ImsManager imsManager = ImsManager.getInstance(sContext, 0);
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        value = imsConfig.getProvisionedValue(
                                ImsConfig.ConfigConstants.PUBLISH_TIMER);
                        logger.debug("Read ImsConfig_PublishTimer: " + value);
                    }
                } catch (ImsException ex) {
                }
            }
        }
        if (value <= 0) {
            value = (int)1200;
            logger.error("Failed to get PublishTimer, the default: " + value);
        }
        return value;
    }

    public static int getPublishTimerExtended() {
        int value = -1;
        if (sContext != null) {
            ImsManager imsManager = ImsManager.getInstance(sContext, 0);
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        value = imsConfig.getProvisionedValue(
                                ImsConfig.ConfigConstants.PUBLISH_TIMER_EXTENDED);
                        logger.debug("Read ImsConfig_PublishTimerExtended: " + value);
                    }
                } catch (ImsException ex) {
                }
            }
        }
        if (value <= 0) {
            value = (int)86400;
            logger.error("Failed to get PublishTimerExtended, the default: " + value);
        }
        return value;
    }

    public static int getMaxNumberOfEntriesInRequestContainedList() {
        int value = -1;
        if (sContext != null) {
            ImsManager imsManager = ImsManager.getInstance(sContext, 0);
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        value = imsConfig.getProvisionedValue(
                                ImsConfig.ConfigConstants.MAX_NUMENTRIES_IN_RCL);
                        logger.debug("Read ImsConfig_MaxNumEntriesInRCL: " + value);
                    }
                } catch (ImsException ex) {
                }
            }
        }
        if (value <= 0) {
            value = (int)100;
            logger.error("Failed to get MaxNumEntriesInRCL, the default: " + value);
        }
        return value;
    }

    public static int getCapabilityPollListSubscriptionExpiration() {
        int value = -1;
        if (sContext != null) {
            ImsManager imsManager = ImsManager.getInstance(sContext, 0);
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        value = imsConfig.getProvisionedValue(
                                ImsConfig.ConfigConstants.CAPAB_POLL_LIST_SUB_EXP);
                        logger.debug("Read ImsConfig_CapabPollListSubExp: " + value);
                    }
                } catch (ImsException ex) {
                }
            }
        }
        if (value <= 0) {
            value = (int)30;
            logger.error("Failed to get CapabPollListSubExp, the default: " + value);
        }
        return value;
    }
}

