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

import com.droidlogic.mediacenter.airplay.util.Utils;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.util.Log;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
/**
 * @ClassName PrefUtils
 * @Description TODO
 * @Date 2013-8-26
 * @Email
 * @Author
 * @Version V1.0
 */
public class PrefUtils {
        private Context             mContent;
        private SharedPreferences          mPrefs;
        public static final Boolean DEBUG              = false;
        public static final String  TAG                = "DLNA";
        /*
        public static final String  AUTOENABLEWHENBOOT = "boot_cfg_dmp";
        public static final String  AUTOENABLEWHENAPK  = "start_service_dmp";*/
        public static final String FISAT_START = "first_start";
        public static final String SERVICE_NAME = "saved_device_name";
        public PrefUtils ( Context cxt ) {
            mContent = cxt;
            mPrefs = PreferenceManager.getDefaultSharedPreferences(cxt);
        }
        public static Object getResource(String cname,String fieldname) {
            Field field = null;
            Object ret = null;
            try{
                Class rclass = Class.forName(cname);
                field = rclass.getField(fieldname);
                ret = field.get(null);
            }catch (Exception ex) {
                ex.printStackTrace();
                return 0;
            }
            if (field != null) {
                Log.d(TAG,"getResource:"+cname+" Val:"+ret);
                return ret;
            } else {
                return 0;
            }

        }
        public void setString ( String key, String Str ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putString ( key, Str );
            mEditor.commit();
        }

        public void setBoolean ( String key, boolean defVal ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putBoolean ( key, defVal );
            mEditor.commit();
        }
        public boolean getBooleanVal ( String key, boolean defVal ) {
            return mPrefs.getBoolean ( key, defVal );
        }
        public String getString ( String key, String defVal ) {
            return mPrefs.getString ( key, defVal );
        }
        public static Object getProperties(String key, String def) {
            String defVal = def;
            try {
                Class properClass = Class.forName("android.os.SystemProperties");
                Method getMethod = properClass.getMethod("get",String.class,String.class);
                defVal = (String)getMethod.invoke(null,key,def);
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                Log.d(TAG,"getProperty:"+key+" defVal:"+defVal);
                return defVal;
            }

        }
        public static void setProperties(String key, String def) {
            String defVal = def;
            try {
                Class properClass = Class.forName("android.os.SystemProperties");
                Method getMethod = properClass.getMethod("set",String.class,String.class);
                defVal = (String)getMethod.invoke(null,key,def);
            } catch (Exception ex) {
                ex.printStackTrace();
            }

        }
}
