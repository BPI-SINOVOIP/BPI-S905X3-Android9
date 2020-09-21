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

import android.app.Application;

public class SmartRemote extends Application{
	
	public static final String ACTION_SHOW_DIALOG = "com.droidlogic.smartremote.SHOW_DIALOG";
	public static final String DIALOG_TYPE = "dialog_type";
	public static final int DIALOG_ADD_MODEL = 1;
	public static final int DIALOG_DERIVE_DATA = 2;
	public static final int DIALOG_EDIT_OPTS = 3;
	
	public static int SCREEN_HEIGHT = -1;
	public static int SCREEN_WIDTH = -1;
	
	public static int NAVI_BUTTON_WIDTH = -1;
	public static int NAVI_BUTTON_HEIGHT = -1;
	
	public static int FUNC_NUB_BUTTON_WIDTH = -1;
	public static int FUNC_NUB_BUTTON_HEIGHT = -1;
	
	public static int CNTL_BUTTON_WIDTH = -1;
	public static int CNTL_BUTTON_HEIGHT = -1;
	/*
	static {
		System.loadLibrary("smart_remote");
	}
	*/
	@Override
	public void onCreate() {
		// TODO Auto-generated method stub
		super.onCreate();
		
		Controller.instance();
		Settings.instance(this);
		SCREEN_HEIGHT = this.getResources().getConfiguration().screenHeightDp;
		SCREEN_WIDTH = this.getResources().getConfiguration().screenWidthDp;
		Log.d("SCREEN_WIDTH:"+SCREEN_WIDTH+" SCREEN_HEIGHT:"+SCREEN_HEIGHT);
	}

	@Override
	public void onLowMemory() {
		// TODO Auto-generated method stub
		super.onLowMemory();
	}

	@Override
	public void onTerminate() {
		// TODO Auto-generated method stub
		super.onTerminate();
	}

}
