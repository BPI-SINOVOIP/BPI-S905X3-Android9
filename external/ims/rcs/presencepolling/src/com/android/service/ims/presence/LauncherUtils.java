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
import android.content.Intent;

import com.android.ims.internal.Logger;

/**
 * Launcher utility functions
 *
 */
public class LauncherUtils {
    private static Logger logger = Logger.getLogger(LauncherUtils.class.getName());

    /**
     * Launch the Presence service.
     *
     * @param context application context
     * @param boot Boot flag
     */
    public static boolean launchPollingService(Context context) {
        logger.debug("Launch polling service");

        Intent intent = new Intent(context, PollingService.class);
        return (context.startService(intent) != null);
    }

    /**
     * Stop the Presence service.
     *
     * @param context application context
     */
    public static boolean stopPollingService(Context context) {
        logger.debug("Stop polling service");

        Intent intent = new Intent(context, PollingService.class);
        return context.stopService(intent);
    }

    public static boolean launchEabService(Context context) {
        logger.debug("Launch Eab Service");

        Intent intent = new Intent(context, EABService.class);
        return (context.startService(intent) != null);
    }

    public static boolean stopEabService(Context context) {
        logger.debug("Stop EAB service");

        Intent intent = new Intent(context, EABService.class);
        return context.stopService(intent);
    }
}

