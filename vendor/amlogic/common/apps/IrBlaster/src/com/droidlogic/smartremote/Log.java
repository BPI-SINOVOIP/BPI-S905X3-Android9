/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

package com.droidlogic.smartremote;

	public class Log {
		private static final boolean sIsDebug = true;
		private static  int sLogLevel = 5;
		private static final String TAG = "SmartRemote" ;
		
		public static void setLogLevel(int level)
		{
			sLogLevel = level;
		}
		
		public static void v(String tag , String log) 
		{
			if(sIsDebug && sLogLevel > 4) {
				android.util.Log.v(tag , log);
			}
		}
		
		public static void v(String log) 
		{
			if(sIsDebug && sLogLevel > 4) {
				android.util.Log.v(TAG ,log);
			}
		}
		
		public static void d(String tag , String log) 
		{
			if(sIsDebug && sLogLevel > 3) {
				android.util.Log.v(tag , log);
			}
		}
		
		public static void d(String log) 
		{
			if(sIsDebug && sLogLevel > 3) {
				android.util.Log.v(TAG ,log);
			}
		}
		
		public static void w(String tag , String log) 
		{
			if(sIsDebug && sLogLevel > 2) {
				android.util.Log.v(tag , log);
			}
		}
		
		public static void w(String log) 
		{
			if(sIsDebug && sLogLevel > 2) {
				android.util.Log.v(TAG ,log);
			}
		}
		
		public static void e(String tag , String log) 
		{
			if(sIsDebug && sLogLevel > 1) {
				android.util.Log.v(tag , log);
			}
		}
		
		public static void e(String log) 
		{
			if(sIsDebug && sLogLevel > 1) {
				android.util.Log.v(TAG ,log);
			}
		}
	}

