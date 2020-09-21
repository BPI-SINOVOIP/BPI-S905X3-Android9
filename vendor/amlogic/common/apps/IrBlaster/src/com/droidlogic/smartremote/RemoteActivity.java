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

import java.util.ArrayList;

import com.droidlogic.smartremote.R;
import com.droidlogic.smartremote.Settings.Model;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.database.DataSetObserver;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;

public class RemoteActivity extends Activity implements Controller.OnStateChangedListener ,
														Settings.SettingsChangedListener ,
														OnItemSelectedListener{

	ActivityHandler mHandler = null;
	Context mContext = null;
	TextView mInfoShow = null; 
	boolean mRedefIconLight = false;
	MenuItem mRedfKey = null;
	
    ActionBar mBar = null;
    View mActionBarView = null;
    Spinner mSp = null;
    LayoutInflater mInflater = null;
    
    public static final int MSG_SHOW_TEXT__BACK_NORMAL = 0;
    public static final int MSG_SHOW_TEXT_INFO = 1;
    public static final int MSG_REDEF_COMPLETED = 2;
    
    boolean mBackNormalRunning = false;
    boolean mNeedStopBackNormal = false;
        
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		
		this.setTitle("");
		
		mHandler = new ActivityHandler();
		mContext = this;
		Controller.get().setActivity(this);
		Controller.get().sendMessage(Controller.MSG_CHECK_AND_INIT);
		
		super.onCreate(savedInstanceState);
		
		this.setContentView(R.layout.activity_remote);

		/*
		FrameLayout infoZone = (FrameLayout)findViewById(R.id.info_zone);
		infoZone.setMinimumHeight((int)(SmartRemote.SCREEN_HEIGHT*0.18));
		*/
    mInfoShow = (TextView)findViewById(R.id.info_text);
        
    Controller.get().registerStateChangedListener(this);
		Settings.registerListener(this);
        
	}
	
	class MyAdapter implements SpinnerAdapter
	{   
		private ArrayList<Model> mModelsList = null;
		ArrayList<View> views = new ArrayList<View>();
		public MyAdapter(ArrayList<Model> list) {
			mModelsList = list;
		}
		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return  mModelsList.size();
		}

		@Override
		public Object getItem(int position) {
			// TODO Auto-generated method stub
			return position;
		}

		@Override
		public long getItemId(int position) {
			// TODO Auto-generated method stub
			return position;
		}

		@Override
		public int getItemViewType(int position) {
			// TODO Auto-generated method stub
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			// TODO Auto-generated method stub
			View view = mInflater.inflate(R.layout.spinner_list, null, false);
			TextView tv = (TextView)view.findViewById(R.id.settings_item);
			tv.setText(mModelsList.get(position).name);
			tv.setTextColor(Color.WHITE);
			convertView = view;
		    return convertView;
		}

		@Override
		public int getViewTypeCount() {
			// TODO Auto-generated method stub
			return 0;
		}

		@Override
		public boolean hasStableIds() {
			// TODO Auto-generated method stub
			return false;
		}

		@Override
		public boolean isEmpty() {
			// TODO Auto-generated method stub
			return false;
		}

		@Override
		public void registerDataSetObserver(DataSetObserver observer) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public void unregisterDataSetObserver(DataSetObserver observer) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public View getDropDownView(int position, View convertView,
				ViewGroup parent) {
			// TODO Auto-generated method stub

				View view = mInflater.inflate(R.layout.spinner_list, null, false);
				TextView tv = (TextView)view.findViewById(R.id.settings_item);
				tv.setText(mModelsList.get(position).name);
				tv.setTextColor(Color.WHITE);
				convertView = view;
				convertView.setBackgroundResource(R.drawable.common_bg);
				view.setMinimumHeight(86);
			    return convertView;
		}
		
	};
	
	private void setActionBar(ArrayList<Model> list)
	{
		Log.v("setActionBar");
	    mInflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	    mActionBarView = mInflater.inflate(R.layout.action_bar, null, false);
	    mSp =  (Spinner)mActionBarView.findViewById(R.id.settings_sp);
	    MyAdapter adapter = new MyAdapter(list);
	    mSp.setAdapter(adapter);
	    mSp.setSelection(Settings.getModeslList().indexOf(Settings.getCurrModel()));
	    mSp.setOnItemSelectedListener(this);
	    
	    mBar = this.getActionBar();
	    mBar.setDisplayShowCustomEnabled(true);
	    mBar.setCustomView(mActionBarView);
	    mBar.setDisplayHomeAsUpEnabled(false);
	    mBar.setTitle("");
	}

	class ActivityHandler extends Handler
	{

		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			onMessage(msg.what , msg);
			super.handleMessage(msg);
		}
		
	}
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.activity_remote, menu);
		return true;
	}
	
	private void onMessage(int what , Message msg)
	{
		switch(what)
		{
		case MSG_SHOW_TEXT__BACK_NORMAL :
		{
				if(mInfoShow != null) {
						mInfoShow.setText(mContext.getString(R.string.msg_normal_use));
				}
		}
		break;
		case MSG_SHOW_TEXT_INFO :
		{
			String info = msg.getData().getString("INFO", mContext.getString(R.string.msg_normal_use));
			if(mInfoShow != null) {
					mInfoShow.setText(info);
			}
		}
		break;
		case MSG_REDEF_COMPLETED :
		{
			if(mRedfKey != null) {
				Log.d("onRedefCompleted run");
				mRedfKey.setIcon(R.drawable.redef);
				mRedefIconLight = false;
			}
		}
		break;
		default:
		}
	}
	public void showTextInfo(final String info)
	{
		showTextInfo(info , true);
	}
	public synchronized void showTextInfo(final String info, boolean backNormal)
	{
		if(mBackNormalRunning) {
			mNeedStopBackNormal = true;
		}
		mHandler.removeMessages(MSG_SHOW_TEXT__BACK_NORMAL);
		
		if(mInfoShow == null) return;
		Bundle b = new Bundle();
		b.putString("INFO", info);
		Message msg = new Message();
		msg.what = MSG_SHOW_TEXT_INFO;
		msg.setData(b);
		mHandler.sendMessage(msg);
		
		if(backNormal) {
			while(mBackNormalRunning);
			new BackNormal().start();
		}
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// TODO Auto-generated method stub
		
		switch(item.getItemId())
		{
		case R.id.menu_redef :
		{
			mRedfKey = item;
			if(!Controller.get().checkEnvOk()) 
				break;
			//{{{{{{
			/*
			if(!mRedefIconLight) {
				item.setIcon(R.drawable.redefing);
				mRedefIconLight = true;
			} else {
				item.setIcon(R.drawable.redef);
				mRedefIconLight = false;
			}
			
			Controller.get().onRedefKeyClicked(mRedefIconLight);
			*/
			//}}}}}}
		}
		break;
		case R.id.menu_add_model :
		{
			Intent it = new Intent(SmartRemote.ACTION_SHOW_DIALOG);
			it.putExtra(SmartRemote.DIALOG_TYPE, SmartRemote.DIALOG_ADD_MODEL);
			this.startActivity(it);
		}
		break;
		case R.id.model_edit_ops :
		{
			Intent it = new Intent(SmartRemote.ACTION_SHOW_DIALOG);
			it.putExtra(SmartRemote.DIALOG_TYPE, SmartRemote.DIALOG_EDIT_OPTS);
			this.startActivity(it);
		}
		break;
		case R.id.menu_derive:
		{
			Intent it = new Intent(SmartRemote.ACTION_SHOW_DIALOG);
			it.putExtra(SmartRemote.DIALOG_TYPE, SmartRemote.DIALOG_DERIVE_DATA);
			this.startActivity(it);
		}
		break;
		case R.id.menu_settings :
		{
			
		}
		break;
		default:
		}

		return super.onOptionsItemSelected(item);
	}

	@Override
	public void onStateChanged(int state) {
		// TODO Auto-generated method stub
		if(state == Controller.STATE_NORMAL) {
			
		}
	}
	
	public void onRedefCompleted()
	{
		Log.d("onRedefCompleted");
		mHandler.sendEmptyMessage(MSG_REDEF_COMPLETED);
	}

		private void clearState()
		{
				Controller.get().unregisterStateChangedListener(this);
				Settings.unregisterListener(this);
				Controller.get().onExit();		
		}
	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
	}

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		Settings.notifySettingsInited();
		Controller.get().sendMessage(Controller.MSG_UI_READY);
		super.onResume();
	}

	@Override
	protected void onStop() {
		// TODO Auto-generated method stub
			clearState();
		super.onStop();
	}
	
    class BackNormal extends Thread
    {
    
		int mLoopCount = 0;
		public void run() {
			// TODO Auto-generated method stub
			mBackNormalRunning = true;
			while(mLoopCount++ < 120) {

				try {
					Thread.sleep(10);
				} catch (InterruptedException e) {}
				if(mNeedStopBackNormal) {
					mNeedStopBackNormal = false;
					break;
				}
				
			}
			if(mLoopCount >= 120) {
				mHandler.sendEmptyMessage(MSG_SHOW_TEXT__BACK_NORMAL);
			}
			mBackNormalRunning = false;
		}
    };

	@Override
	public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
			long arg3) {
		// TODO Auto-generated method stub
		Settings.setCurrModel(Settings.getModeslList().get(arg2).mId);
	}

	@Override
	public void onNothingSelected(AdapterView<?> arg0) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onCurrModelChanged(Model m) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onModelsChanged(ArrayList<Model> list) {
		// TODO Auto-generated method stub
		setActionBar(list);
	}
		
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		// TODO Auto-generated method stub
			Log.d("onConfigurationChanged");
		super.onConfigurationChanged(newConfig);
	}
}
