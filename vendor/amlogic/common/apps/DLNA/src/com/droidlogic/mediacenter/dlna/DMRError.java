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
package com.droidlogic.mediacenter.dlna;

import com.droidlogic.mediacenter.R;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
/**
 * @ClassName DMRError
 * @Description TODO
 * @Date 2013-9-5
 * @Email
 * @Author
 * @Version V1.0
 */
public class DMRError extends Activity implements OnClickListener {
        private Button btn;
        @Override
        protected void onCreate ( Bundle arg0 ) {
            super.onCreate ( arg0 );
            setContentView ( R.layout.dmr_error_dialog );
            btn = ( Button ) findViewById ( R.id.btn_ok );
            btn.setOnClickListener ( this );
        }

        @Override
        protected void onDestroy() {
            super.onDestroy();
        }

        @Override
        protected void onResume() {
            super.onResume();
        }

        @Override
        protected void onStop() {
            super.onStop();
            exitApp();
        }

        /* (non-Javadoc)
         * @see android.view.View.OnClickListener#onClick(android.view.View)
         */
        @Override
        public void onClick ( View arg0 ) {
            exitApp();
        }

        private void exitApp() {
            ActivityManager activityMgr = ( ActivityManager ) getSystemService ( ACTIVITY_SERVICE );
            try {
                Method method = ActivityManager.class.getMethod("forceStopPackage",String.class);
                method.invoke(activityMgr,getPackageName());
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                this.stopService ( new Intent ( DMRError.this, MediaCenterService.class ) );
            }
        }
}
