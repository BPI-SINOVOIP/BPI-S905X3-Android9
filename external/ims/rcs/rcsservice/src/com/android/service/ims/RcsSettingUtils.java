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

package com.android.service.ims;

import java.lang.String;

import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.TelephonyManager;
import android.content.Context;
import com.android.ims.ImsConfig;
import com.android.ims.ImsManager;
import com.android.ims.ImsException;
import android.os.SystemProperties;

import com.android.ims.RcsManager.ResultCode;

import com.android.ims.internal.Logger;

public class RcsSettingUtils{
    /*
     * The logger
     */
    static private Logger logger = Logger.getLogger("RcsSettingUtils");

    public RcsSettingUtils() {
    }

    public static boolean isFeatureProvisioned(Context context,
            int featureId, boolean defaultValue) {
        CarrierConfigManager configManager = (CarrierConfigManager)
                context.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        // Don't need provision.
        if (configManager != null) {
            PersistableBundle config = configManager.getConfig();
            if (config != null && !config.getBoolean(
                    CarrierConfigManager.KEY_CARRIER_VOLTE_PROVISIONED_BOOL)) {
                return true;
            }
        }

        boolean provisioned = defaultValue;
        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    provisioned = imsConfig.getProvisionedValue(featureId)
                            == ImsConfig.FeatureValueConstants.ON;
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("featureId=" + featureId + " provisioned=" + provisioned);
        return provisioned;
    }

    public static boolean isVowifiProvisioned(Context context) {
        return isFeatureProvisioned(context,
                ImsConfig.ConfigConstants.VOICE_OVER_WIFI_SETTING_ENABLED, false);
    }

    public static boolean isLvcProvisioned(Context context) {
        return isFeatureProvisioned(context,
                ImsConfig.ConfigConstants.LVC_SETTING_ENABLED, false);
    }

    public static boolean isEabProvisioned(Context context) {
        return isFeatureProvisioned(context,
                ImsConfig.ConfigConstants.EAB_SETTING_ENABLED, false);
    }

    public static int getSIPT1Timer(Context context) {
        int sipT1Timer = 0;

        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    sipT1Timer = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.SIP_T1_TIMER);
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("sipT1Timer=" + sipT1Timer);
        return sipT1Timer;
    }

    /**
     * Capability discovery status of Enabled (1), or Disabled (0).
     */
    public static boolean getCapabilityDiscoveryEnabled(Context context) {
        boolean capabilityDiscoveryEnabled = false;

        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    capabilityDiscoveryEnabled = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.CAPABILITY_DISCOVERY_ENABLED)
                            == ImsConfig.FeatureValueConstants.ON;
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("capabilityDiscoveryEnabled=" + capabilityDiscoveryEnabled);
        return capabilityDiscoveryEnabled;
    }

    /**
     * The Maximum number of MDNs contained in one Request Contained List.
     */
    public static int getMaxNumbersInRCL(Context context) {
        int maxNumbersInRCL = 100;

        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    maxNumbersInRCL = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.MAX_NUMENTRIES_IN_RCL);
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("maxNumbersInRCL=" + maxNumbersInRCL);
        return maxNumbersInRCL;
    }

    /**
     * Expiration timer for subscription of a Request Contained List, used in capability polling.
     */
    public static int getCapabPollListSubExp(Context context) {
        int capabPollListSubExp = 30;

        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    capabPollListSubExp = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.CAPAB_POLL_LIST_SUB_EXP);
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("capabPollListSubExp=" + capabPollListSubExp);
        return capabPollListSubExp;
    }

    /**
     * Peiod of time the availability information of a contact is cached on device.
     */
    public static int getAvailabilityCacheExpiration(Context context) {
        int availabilityCacheExpiration = 30;

        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    availabilityCacheExpiration = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.AVAILABILITY_CACHE_EXPIRATION);
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("availabilityCacheExpiration=" + availabilityCacheExpiration);
        return availabilityCacheExpiration;
    }

    public static boolean isMobileDataEnabled(Context context) {
        boolean mobileDataEnabled = false;
        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    mobileDataEnabled = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.MOBILE_DATA_ENABLED)
                            == ImsConfig.FeatureValueConstants.ON;
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("mobileDataEnabled=" + mobileDataEnabled);
        return mobileDataEnabled;
    }

    public static void setMobileDataEnabled(Context context, boolean mobileDataEnabled) {
        logger.debug("mobileDataEnabled=" + mobileDataEnabled);
        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    imsConfig.setProvisionedValue(
                            ImsConfig.ConfigConstants.MOBILE_DATA_ENABLED, mobileDataEnabled?
                            ImsConfig.FeatureValueConstants.ON:ImsConfig.FeatureValueConstants.OFF);
                }
            } catch (ImsException ex) {
                logger.debug("ImsException", ex);
            }
        }
    }

    public static int getPublishThrottle(Context context) {
        int publishThrottle = 60000;

        ImsManager imsManager = ImsManager.getInstance(context, 0);
        if (imsManager != null) {
            try {
                ImsConfig imsConfig = imsManager.getConfigInterface();
                if (imsConfig != null) {
                    publishThrottle = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.SOURCE_THROTTLE_PUBLISH);
                }
            } catch (ImsException ex) {
            }
        }

        logger.debug("publishThrottle=" + publishThrottle);
        return publishThrottle;
    }
}

