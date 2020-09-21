/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.mediacenter;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

/**
 * @ClassName DMRBroadcastReceiver
 * @Description TODO
 * @Date 2013-9-4
 * @Email
 * @Author
 * @Version V1.0
 */
public class DMRBroadcastReceiver extends BroadcastReceiver {

        /* (non-Javadoc)
         * @see android.content.BroadcastReceiver#onReceive(android.content.Context, android.content.Intent)
         */
        @Override
        public void onReceive ( Context cxt, Intent intent ) {
            cxt.startService(new Intent(cxt,WeakRefService.class));
        }

}
