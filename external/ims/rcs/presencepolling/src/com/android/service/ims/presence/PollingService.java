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

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.telephony.CarrierConfigManager;
import android.os.IBinder;
import android.os.PersistableBundle;
import android.os.ServiceManager;
import android.os.SystemProperties;

import com.android.ims.internal.Logger;
import com.android.internal.annotations.VisibleForTesting;

/**
 * Manages the CapabilityPolling class. Starts capability polling when a SIM card is inserted that
 * supports RCS Presence Capability Polling and stops the service otherwise.
 */
public class PollingService extends Service {

    private Logger logger = Logger.getLogger(this.getClass().getName());

    private CapabilityPolling mCapabilityPolling = null;

    // not final for testing
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            switch (intent.getAction()) {
                case CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED: {
                    checkAndUpdateCapabilityPollStatus();
                    break;
                }
            }
        }
    };

    /**
     * Constructor
     */
    public PollingService() {
        logger.debug("PollingService()");
    }

    @Override
    public void onCreate() {
        logger.debug("onCreate()");

        registerBroadcastReceiver();
        // Check when the service starts in case the SIM info has already been loaded.
        checkAndUpdateCapabilityPollStatus();
    }

    /**
      * Cleans up when the service is destroyed
      */
    @Override
    public void onDestroy() {
        logger.debug("onDestroy()");

        if (mCapabilityPolling != null) {
            mCapabilityPolling.stop();
            mCapabilityPolling = null;
        }

        unregisterReceiver(mReceiver);

        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        logger.debug("onBind(), intent: " + intent);

        if (!isRcsSupported(this)) {
            return null;
        }

        logger.debug("onBind add services here");
        return null;
    }

    @VisibleForTesting
    public void setBroadcastReceiver(BroadcastReceiver r) {
        mReceiver = r;
    }

    private void registerBroadcastReceiver() {
        IntentFilter filter = new IntentFilter(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        registerReceiver(mReceiver, filter);
    }

    private void checkAndUpdateCapabilityPollStatus() {
        // If the carrier doesn't support RCS Presence, stop polling.
        if (!isRcsSupportedByCarrier(this)) {
            logger.info("RCS not supported by carrier. Stopping CapabilityPolling");
            if (mCapabilityPolling != null) {
                mCapabilityPolling.stop();
                mCapabilityPolling = null;
            }
            return;
        }
        // Carrier supports, so start the service if it hasn't already been started.
        if (mCapabilityPolling == null) {
            logger.info("Starting CapabilityPolling...");
            mCapabilityPolling = CapabilityPolling.getInstance(this);
            mCapabilityPolling.start();
        }
    }

    static boolean isRcsSupported(Context context) {
        return isRcsSupportedByDevice() && isRcsSupportedByCarrier(context);
    }

    private static boolean isRcsSupportedByCarrier(Context context) {
        CarrierConfigManager configManager = context.getSystemService(CarrierConfigManager.class);
        if (configManager != null) {
            PersistableBundle b = configManager.getConfig();
            if (b != null) {
                return b.getBoolean(CarrierConfigManager.KEY_USE_RCS_PRESENCE_BOOL, false);
            }
        }
        return true;
    }

    public static boolean isRcsSupportedByDevice() {
        String rcsSupported = SystemProperties.get("persist.rcs.supported");
        return "1".equals(rcsSupported);
    }
}
