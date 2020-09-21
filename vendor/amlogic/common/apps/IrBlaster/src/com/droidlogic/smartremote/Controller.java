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

import java.io.File;
import java.util.ArrayList;

import com.droidlogic.smartremote.R;
import com.droidlogic.smartremote.Settings.Model;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;

public class Controller implements Settings.SettingsChangedListener{
	
	private  static Controller sInstance = null;
	private  ControllerHandler mHandler = null;
	private  HandlerThread mControllerThread = new HandlerThread("ControllerThread");
	
	private RemoteModel mModel = null;
	private RemoteActivity mActivity = null;
	
	public static final int SEND_REPEAT_KEY_DISTANCE = 97;
	
	public static final int MSG_UI_READY = 0;
	public static final int MSG_CHECK_AND_INIT = 1;
	public static final int MSG_SEND_ACTION = 2;
	public static final int MSG_ON_EXIT = 3;
	public static final int MSG_SEND_ACTION_SUCCESS = 4;
	public static final int MSG_SEND_ACTION_FAILED = 5;
	public static final int MSG_SEND_ACTION_UNDEF = 6;
	public static final int MSG_REDEF_ACTION_FAILED = 7;
	public static final int MSG_REDEF_ACTION_SUCCESS = 8;
	public static final int MSG_REDEF_ACTION_CANCEL = 9;
	public static final int MSG_SEND_REPEAT = 10;

    public static final int STATE_UNKNOW = 0;
    public static final int STATE_INIT = 1;
    public static final int STATE_NORMAL = 2;
    public static final int STATE_GO_TO_LEARN = 3;
    public static final int STATE_LEARNING = 4;
	
	private int mState = STATE_UNKNOW;
	private ArrayList<OnStateChangedListener> mListeners = null;
	private boolean envOk = true; 
	
	private Controller()
	{
		Log.v("Controller create instance");
		mControllerThread.start();
		mHandler = new ControllerHandler(mControllerThread.getLooper());
		mListeners = new ArrayList<OnStateChangedListener>();
	}
	
	public static void instance()
	{
		if(sInstance == null) {
			sInstance = new Controller();
		}
	}
	
	public static Controller get()
	{
		if(sInstance == null) {
			sInstance = new Controller();
		}
		return sInstance;
	}
	
	public interface OnStateChangedListener {
		public void onStateChanged(int state);
	}
	
	class ControllerHandler extends Handler
	{
		ControllerHandler(Looper lp)
		{
			super(lp);
		}
		
		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			onMessage(msg.what ,msg);
			super.handleMessage(msg);
		}
		
	}
	
	public void setActivity(RemoteActivity activity) 
	{
		this.mActivity = activity;
	}
	
	public boolean checkEnvOk()
	{
		return envOk;
	}
	
	private String getString(int resId)
	{
		return mActivity.getString(resId);
	}
	
	private void onMessage(int what , Message msg)
	{
		Log.d("getMsg Key="+what+" args="+msg);
		
		switch(what)
		{
		case MSG_CHECK_AND_INIT :
		{
			Log.v("doCheckEnvAndInit ...");
			mActivity.showTextInfo(getString(R.string.msg_initing),false);
			mModel = new RemoteModel(mActivity);
			if(mModel.checkEnvAndInit() != 0) {
				envOk = false;
			}
			mListeners.clear();
			Settings.init();
			mModel.init();
			updateState(STATE_INIT);
			Settings.registerListener(this);
		}
		break;
		case MSG_SEND_ACTION :
		{
			if(mState == STATE_GO_TO_LEARN) {
				updateState(STATE_LEARNING);
				mActivity.showTextInfo(getString(R.string.msg_learning),false);
				//mModel.redef(msg.arg1);
			} else if(mState == STATE_NORMAL) {
				mModel.send(msg.arg1);
			}
		}
		break;
		case MSG_UI_READY :
		{
			if(envOk) {
				updateState(STATE_NORMAL);
				mActivity.showTextInfo(getString(R.string.msg_normal_use),false);
			}else{
				mActivity.showTextInfo(getString(R.string.msg_not_support),false);
			}
		}
		break;
		case MSG_SEND_ACTION_SUCCESS :
		{
			mActivity.showTextInfo(getString(R.string.msg_send_success));
		}
		break;
		case MSG_SEND_ACTION_FAILED :
		{
			mActivity.showTextInfo(getString(R.string.msg_send_failed));
		}
		break;
		case MSG_SEND_ACTION_UNDEF :
		{
			mActivity.showTextInfo(getString(R.string.msg_send_undef));
		}
		break;
		case MSG_REDEF_ACTION_SUCCESS :
		{
			updateState(STATE_NORMAL);
			mActivity.onRedefCompleted();
			mActivity.showTextInfo(getString(R.string.msg_redef_success));
		}
		break;
		case MSG_REDEF_ACTION_FAILED :
		{
			updateState(STATE_NORMAL);
			mActivity.onRedefCompleted();
			mActivity.showTextInfo(getString(R.string.msg_redef_failed));
		}
		break;
		case MSG_REDEF_ACTION_CANCEL :
		{
			updateState(STATE_NORMAL);
			mActivity.onRedefCompleted();
			mActivity.showTextInfo(getString(R.string.msg_redef_cancel));
		}
		break;
		case MSG_SEND_REPEAT :
		{
			if(mState == STATE_NORMAL) {
				mModel.sendRepeat(msg.arg1);
			}
		}
		break;
		
		default:
		}
	}
	
	public void registerStateChangedListener(OnStateChangedListener  listener) 
	{
		mListeners.add(listener);
	}
	public void unregisterStateChangedListener(OnStateChangedListener  listener) 
	{
		mListeners.remove(listener);
	}
	
	public int getCurrState()
	{
		return mState;
	}
	
	private void updateState(int state)
	{
		Log.d("updateState state="+state);
		this.mState = state;
		for(OnStateChangedListener listener:mListeners) {
			listener.onStateChanged(state);
		}
	}
	
	public void sendMessage(int msg)
	{
		mHandler.sendEmptyMessage(msg);
	}
	public void sendAction(int action)
	{
		Log.d("send action="+action);
		Message msg = new Message();
		msg.what = MSG_SEND_ACTION;
		msg.arg1 = action;
		mHandler.sendMessage(msg);
		
	}
	public void sendRepeat(int action)
	{
		Log.d("send repeat="+action);
		Message msg = new Message();
		msg.what = MSG_SEND_REPEAT;
		msg.arg1 = action;
		mHandler.sendMessage(msg);
	}
	public void onRedefKeyClicked(boolean isLight)
	{
		final boolean iconLightState = isLight; 
		mHandler.post(new Runnable(){
			
			@Override
			public void run() {
				// TODO Auto-generated method stub
				if(iconLightState) {
					if(mState != STATE_GO_TO_LEARN
					   && mState != STATE_LEARNING) {
						mActivity.showTextInfo(getString(R.string.msg_go_learn),false);
						updateState(STATE_GO_TO_LEARN);
					}
				} else {
					if(mState == STATE_GO_TO_LEARN) {
						mActivity.showTextInfo(getString(R.string.msg_redef_cancel));
						updateState(STATE_NORMAL);
					} else if(mState == STATE_LEARNING) {
						updateState(STATE_NORMAL);
					}
				}
			}
			
		});
	}
	
	public boolean deriveData()
	{
		boolean ret = true;
    	String basePath = "/storage/external_storage/sdcard1/SmartRemoteData";
        //String basePath = android.os.Environment.getExternalStorage2Directory()
				                 //.getAbsolutePath()+"/SmartRemoteData";

    	File file = new File(basePath);
    	if(!file.exists()) {
    		file.mkdirs();
    	}
    	ret &= Settings.deriveData(basePath);
		ret &= mModel.deriveData(basePath);
		return ret;
	}
	
	public void onExit()
	{
		updateState(STATE_NORMAL);
		mActivity.onRedefCompleted(); 
		Settings.unregisterListener(this);
		mModel.exit();
	}

	@Override
	public void onCurrModelChanged(Model currModel) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onModelsChanged(ArrayList<Model> list) {
		// TODO Auto-generated method stub
		
	}

}
