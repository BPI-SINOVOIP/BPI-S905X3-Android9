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

import android.content.Context;
import android.content.Intent;
import android.content.ComponentName;

import com.android.ims.internal.Logger;
import com.android.service.ims.RcsService;

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
    public static void launchRcsService(Context context) {
        logger.debug("Launch RCS service");

        ComponentName comp = new ComponentName(context.getPackageName(),
                RcsService.class.getName());
        ComponentName service = context.startService(new Intent().setComponent(comp));
        if (service == null) {
            logger.error("Could Not Start Service " + comp.toString());
        } else {
            logger.debug("ImsService Auto Boot Started Successfully");
        }
    }

    /**
     * Stop the Presence service.
     *
     * @param context application context
     */
    public static void stopRcsService(Context context) {
        logger.debug("Stop RCS service");

        context.stopService(new Intent(context, RcsService.class));
    }
}

