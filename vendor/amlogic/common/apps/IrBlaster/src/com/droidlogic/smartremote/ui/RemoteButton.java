/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

package com.droidlogic.smartremote.ui;

import java.util.ArrayList;

import android.content.Context;
import android.graphics.Color;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.Button;

import com.droidlogic.smartremote.*;
import com.droidlogic.smartremote.Settings.Model;

public class RemoteButton extends Button implements Settings.SettingsChangedListener,
                                                        Controller.OnStateChangedListener{
	
	Context mContext = null;
	Handler mHandler = null;
	boolean mStateDowm = false;

	public RemoteButton(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
		init(context);
	}
	public RemoteButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		init(context);
	}
	public RemoteButton(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		// TODO Auto-generated constructor stub
		init(context);
	}
	
	void init(Context context)
	{
		this.mContext = context;
		mHandler = new Handler();

		this.setTextColor(Color.WHITE);
		Settings.registerListener(this);
		Controller.get().registerStateChangedListener(this);
		if(Controller.get().getCurrState() != Controller.STATE_NORMAL) {
			this.setEnabled(false);
		}
	}
	@Override
	public void onStateChanged(int state) {
		// TODO Auto-generated method stub
		Log.d("RemoteButton onStateChannged state="+state);
		switch(state)
		{
		case Controller.STATE_NORMAL :
		{
			mHandler.post(new Runnable(){

				@Override
				public void run() {
					// TODO Auto-generated method stub
					RemoteButton.this.setEnabled(true);
				}
				
			});
		}
		break;
		case Controller.STATE_GO_TO_LEARN :
		{
			
		}
		break;
		case Controller.STATE_LEARNING :
		{
			mHandler.post(new Runnable(){

				@Override
				public void run() {
					// TODO Auto-generated method stub
					RemoteButton.this.setEnabled(false);
				}
				
			});
		}
		break;
		default:
		}
	}
	@Override
	public void onCurrModelChanged(Model currModel) {
		// TODO Auto-generated method stub
		
	}
	@Override
	public void onModelsChanged(ArrayList<Model> list) {
		// TODO Auto-generated method stub
		
	}
	
	private void sendAction()
	{
		int action = ActionUtils.getAction(this.getId());
		if(action != ActionUtils.INVALID_ACTION) {
			Controller.get().sendAction(action);
		}
	}
	
	private void sendRepeat()
	{
		int action = ActionUtils.getAction(this.getId());
		if(action != ActionUtils.INVALID_ACTION) {
			Controller.get().sendRepeat(action);
		}
	}
	
	class DownExec extends Thread 
	{
		DownExec()
		{
			sendAction();
		}
		@Override
		public void run() {
			// TODO Auto-generated method stub
			while(true) {
				try {
					Thread.sleep(Controller.SEND_REPEAT_KEY_DISTANCE);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				if(!mStateDowm) {
					break;
				}
				
				sendRepeat();
			}
			super.run();
		}
		
	}
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		// TODO Auto-generated method stub
		switch(event.getAction())
		{
		case MotionEvent.ACTION_DOWN :
		{
			if(!mStateDowm)
			{
				new DownExec().start();
				mStateDowm = true;
			}
		}
		break;
		case MotionEvent.ACTION_UP :
		{
			mStateDowm = false;
		}
		break;
		case MotionEvent.ACTION_MOVE :
		{
			
		}
		break;
		}
		return super.onTouchEvent(event);
	}

}
