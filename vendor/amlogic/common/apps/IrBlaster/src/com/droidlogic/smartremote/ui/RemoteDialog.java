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

import com.droidlogic.smartremote.R;
import com.droidlogic.smartremote.Controller;
import com.droidlogic.smartremote.Settings;
import com.droidlogic.smartremote.SmartRemote;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

public class RemoteDialog extends Activity{

	Context mContext = null;
	int mCurrType = -1; 
	TextView mPrograssText = null;
	LinearLayout mPrograssZone = null;
	LinearLayout mResultZone = null;
	
	boolean mIsInOptEdit = false;
	EditText mOptsEdit = null;
	int mRenameId = -1;
	EditOptsAdapter mAdapter = null;
	LayoutInflater mInflater =null;

	
	Handler mHandler = null;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		Intent it = this.getIntent();
		mHandler = new Handler();
		mContext = this;
		mInflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		setViewByIntent(it);
		super.onCreate(savedInstanceState);
	}

	private void setViewByIntent(Intent it)
	{
	    mCurrType = it.getIntExtra(SmartRemote.DIALOG_TYPE, -1);
		switch(mCurrType)
		{
		/*
		case SmartRemote.DIALOG_ADD_MODEL :
		{
			this.setContentView(R.layout.dialog_add_model);
			this.setTitle(R.string.menu_add_model);
			final EditText mEt = (EditText)this.findViewById(R.id.add_model_et);
			Button cancel = (Button)this.findViewById(R.id.add_model_bt_cancel);
			Button add = (Button)this.findViewById(R.id.add_model_bt_add);

			OnClickListener listener = new OnClickListener()
			{

				@Override
				public void onClick(View v) {
					// TODO Auto-generated method stub
					if(v.getId() == R.id.add_model_bt_cancel) {
						RemoteDialog.this.finish();
					}
					if(v.getId() == R.id.add_model_bt_add) {
						String name = mEt.getText().toString();
						if(name != null && !name.equals("")) {
							Settings.addModel(name);
						}
						RemoteDialog.this.finish();
					}
					
				}
				
			};
			
			cancel.setOnClickListener(listener);
			add.setOnClickListener(listener);
		}
		break;
		*/
		case SmartRemote.DIALOG_DERIVE_DATA:
		{
			this.setTitle(R.string.menu_derive);
			this.setContentView(R.layout.dialog_derive);
			mPrograssText = (TextView)this.findViewById(R.id.prograss_text);
			mPrograssZone = (LinearLayout)this.findViewById(R.id.prograss_zone);
			mResultZone = (LinearLayout)this.findViewById(R.id.result_zone);
			Button bt = (Button)this.findViewById(R.id.prograss_bt);
			bt.setOnClickListener(new OnClickListener(){

				@Override
				public void onClick(View v) {
					// TODO Auto-generated method stub
					RemoteDialog.this.finish();
				}
				
			});
		}
		break;
		/*
		case SmartRemote.DIALOG_EDIT_OPTS:
		
		{
			this.setTitle(R.string.menu_edit_ops);
			this.setContentView(R.layout.dialog_edit_opts);
			mOptsEdit = (EditText)this.findViewById(R.id.model_rename_et);
			ListView list = (ListView)this.findViewById(R.id.ops_list);
			mAdapter = new EditOptsAdapter();
			list.setAdapter(mAdapter);
			Button cancel = (Button)this.findViewById(R.id.model_opt_bt_cancel);
			Button ok = (Button)this.findViewById(R.id.model_opt_bt_ok);
			OnClickListener listener = new OnClickListener()
			{

				@Override
				public void onClick(View v) {
					// TODO Auto-generated method stub
					if(v.getId() == R.id.model_opt_bt_cancel) {
						if(mIsInOptEdit) {
							mOptsEdit.setVisibility(View.GONE);
							mIsInOptEdit = false;
							mRenameId = -1;
						} else {
							RemoteDialog.this.finish();
						}
					}
					if(v.getId() == R.id.model_opt_bt_ok) {
						if(mIsInOptEdit) {
							String name = mOptsEdit.getText().toString();
							if(name != null && !name.equals("")){
								Settings.renameModel(mRenameId, name);
								mAdapter.notifyDataSetChanged();
							}
							mOptsEdit.setVisibility(View.GONE);
							mIsInOptEdit = false;
							mRenameId = -1;
						} else {
							RemoteDialog.this.finish();
						}
					}
					
				}
				
			};
			
			cancel.setOnClickListener(listener);
			ok.setOnClickListener(listener);
		}
		break;
		*/
		default:
		{
			this.finish();
		}
		}
	}
	
	class EditOptsAdapter extends BaseAdapter
	{
		@Override
		public int getCount() {
			// TODO Auto-generated method stub
			return Settings.getModeslList().size();
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
		public View getView(int position, View convertView, ViewGroup parent) {
			// TODO Auto-generated method stub
			
			View view = mInflater.inflate(R.layout.edit_opts_item, parent, false);
			TextView text = (TextView)view.findViewById(R.id.edit_opts_text);
			text.setText(Settings.getModeslList().get(position).name);
			Button del = (Button)view.findViewById(R.id.bt_del);
			Button rename = (Button)view.findViewById(R.id.bt_rename);
			del.setTag(position);
			rename.setTag(position);
			OnClickListener listener = new OnClickListener()
			{

				@Override
				public void onClick(View v) {
					// TODO Auto-generated method stub
					if(v.getId() == R.id.bt_del) {
						Settings.delModel(Settings.getModeslList().get((Integer)v.getTag()).mId);
						EditOptsAdapter.this.notifyDataSetChanged();
					}
					if(v.getId() == R.id.bt_rename) {
						mRenameId = Settings.getModeslList().get((Integer)v.getTag()).mId;
						mOptsEdit.setVisibility(View.VISIBLE);
						mOptsEdit.requestFocus();
						mIsInOptEdit = true;
					}
				}
				
			};
			
			del.setOnClickListener(listener);
			rename.setOnClickListener(listener);
			convertView = view;
			return convertView;
		}
		
	}
	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		return true;
		//return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onNavigateUp() {
		// TODO Auto-generated method stub
		return super.onNavigateUp();
	}

	@Override
	protected void onPause() {
		// TODO Auto-generated method stub
		super.onPause();
	}

	@Override
	protected void onRestart() {
		// TODO Auto-generated method stub
		super.onRestart();
	}

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		if(mCurrType == SmartRemote.DIALOG_DERIVE_DATA) {
			mHandler.post(new Runnable()
			{

				@Override
				public void run() {
					// TODO Auto-generated method stub
					boolean ret = Controller.get().deriveData();
					if(ret) {
						mPrograssText.setText(R.string.derive_success);
					}else{
						mPrograssText.setText(R.string.derive_failed);
					}
					
					mPrograssZone.setVisibility(View.GONE);
					mResultZone.setVisibility(View.VISIBLE);
				}
				
			});
		}
		super.onResume();
	}

	@Override
	protected void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
	}
	
}
