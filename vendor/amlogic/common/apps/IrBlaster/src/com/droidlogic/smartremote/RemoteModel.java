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
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.Reader;
import java.util.ArrayList;

import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.ConsumerIrManager;
import android.hardware.ConsumerIrManager.CarrierFrequencyRange;

public class RemoteModel implements Settings.SettingsChangedListener , 
                                    Controller.OnStateChangedListener {
	
	private String mBasePath = null;
	Context mContext = null;
	File mTempFile = null;
	SharedPreferences mPref = null;
	ConsumerIrManager mConsumerIrManager = null;
	
	String SEPARATOR = ",";
	int DATA_MAX_LENGTH = 4800;
	//{{{{{{{{{
   	//private native int checkAndInit();
    //private native int sendAction(int data[]);
    //private native int[] receiveAction();
    //private native int checkAction(String path);
    //private native int redefAction();
    //private native void cancelRecv();
    //private native void appExit();  //do nothing
	//}}}}}}}}}
  
	public RemoteModel(Context c)
	{
		this.mContext = c;
		mConsumerIrManager = (ConsumerIrManager)c.getSystemService(Context.CONSUMER_IR_SERVICE);
		mBasePath = mContext.getFilesDir().getAbsolutePath();
	}
	
    public void init()
    {
		mPref = mContext.getSharedPreferences("RemoteModel", Context.MODE_PRIVATE);
		boolean needFirstInit = mPref.getBoolean("FirstInit", true);
		if(needFirstInit) {
			doFirstInit();
		}
		Controller.get().registerStateChangedListener(this);
		Settings.registerListener(this);
    }
    
    private void doFirstInit()
    {
    	mPref.edit().putBoolean("RemoteModel", false).commit();
    }
    
    private InputStream generateInput(int action)
    {
    	InputStream input;
    	String path = generateDataFilePath(action);
    	
    	File file = new File(path);
    	if(file.exists()) {
    		try {
				input = new FileInputStream(file);
				return input;
			} catch (FileNotFoundException e) {}
    	}
    	
    	path = generateDataFilePath("",action);
    	try {
			input = mContext.getAssets().open(path);
	    	if(input != null && input.available() > 0) {
	    		return input;
	    	}
		} catch (IOException e) {}
    	Log.w("action:"+action+" not define");
    	return null;
    }   
    
    private String generateDataFilePath(int action) {
    	return generateDataFilePath(mBasePath , action);
    }
    
    private String generateDataFilePath(String basePath , int action) {
    	return generateDataFilePath(basePath , Settings.getCurrModel().mId,action);
    }
    
    private String generateDataFilePath(String base,int model , int action)
    {
    	String b = null;
    	if(base != null && !base.equals("")) {
    		b = base+"/";
    	} else {
    		b = "";
    	}
    	String path = b +
    			"model"+model+"/"+
                "model"+model+"_"+
 			   "action"+action+".data";
    	Log.v("generateDestinationPath:"+path);
    	return path;
    }
    /*
    private boolean saveKeyData(String path, int data[])
    {
    	boolean ret = true;
    	File outFile = new File(path);
    	try {
		    if(!outFile.exists()) outFile.createNewFile();
		} catch (IOException e) {
			e.printStackTrace();
			Log.e("create save file:"+path+" failed");
			return false;
		}
    	
		Writer out = null;
		String str = "";
    	
    	try {
			out = new FileWriter(outFile);
			for(int i=0;i<data.length;i++) {
				if(data[i] == CalibratePolicy.BREAK_POINT) {
					break;
				}
				
				str += data[i];
				
				if(i != (data.length-1) && 
						data[i+1] != CalibratePolicy.BREAK_POINT) {
					str += SEPARATOR;
				}
			}
			out.write(str);
			Log.v("write :"+str);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			ret = false;
			Log.e("write file:"+path+" failed");
			e.printStackTrace();
		}finally {
			try {
				if(out != null) out.close();
			}catch(Exception e){}
		}
    	return ret;
    }
    */
    //{{{{{{{{{
    private int sendAction(int data[])
    {
    	if(mConsumerIrManager == null)
    		return -1;
    	CarrierFrequencyRange[] freq = mConsumerIrManager.getCarrierFrequencies();
    	if(freq == null || freq.length == 0)
    		return -1;
    	//for(CarrierFrequencyRange f:freq)
    	//{
    		//Log.v("f.getMinFrequency():"+f.getMinFrequency());
    		mConsumerIrManager.transmit(38000, data);
    	//}
    	return 0;
    }
	//}}}}}}}}
    
    private int[] getKeyData(InputStream input)
    {
    	int data[] = null;
    	char temp[] = new char[DATA_MAX_LENGTH];
    	try {
    		if(input == null || input.available() == 0) {
    			return null;
    		}
    	} catch(Exception e){}
    	
    	Reader in = new InputStreamReader(input);
    	
    	try {
    		int count = in.read(temp);
    		String str = new String(temp,0,count);
    		
    		String strs[] = str.trim().split(SEPARATOR);
    		data = new int[strs.length];
    		//Log.v("data length: "+strs.length);
    		for(int i=0;i<strs.length;i++) {
    			data[i] = Integer.parseInt(strs[i])*10;
    			//Log.v("read data["+i+"]="+data[i]);
    		}			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			data = null;
			Log.e("read data failed");
			e.printStackTrace();
		}finally {
			try {
				if(in != null) in.close();
			}catch(Exception e){}
		}
    	
    	return data;
    }
    
    public int checkEnvAndInit()
    {
    	if(mConsumerIrManager == null)
    		return -1;
    	return 0;
        //return checkAndInit();
    }
    public synchronized void sendRepeat(int action)
    {
    	int policy_id = 0;
    	//InputStream input = generateInput(action);
    	int data[] = {9000,2250,560,560};
    	if(true) {
    		//data = getKeyData(input);
    		//policy_id = CalibratePolicy.detect(data);
    		policy_id = CalibratePolicy.POLICY_ID_NEC;
    		switch(policy_id) {
    		case CalibratePolicy.POLICY_ID_NONE :
    		{
    			sendAction(data);
    		}
    		break;
    		case CalibratePolicy.POLICY_ID_NEC:
    		{
    			//int repeat[] = new int[]{908,216,66,1398};
    			sendAction(data);
    		}
    		break;
    		}
    		
    	} 
    }
    public synchronized void send(int action)
    {
    	boolean ret = false;
    	Log.v("send start");
    	InputStream input = generateInput(action);
    	int data[];
    	if(input != null) {
    		data = getKeyData(input);
    		if(0 == sendAction(data)) {
    			ret = true;
    		}
    	}
    	if(input == null) {
    		Controller.get().sendMessage(Controller.MSG_SEND_ACTION_UNDEF);
    	}else if(ret) {
    		Controller.get().sendMessage(Controller.MSG_SEND_ACTION_SUCCESS);
    	} else {
    		Controller.get().sendMessage(Controller.MSG_SEND_ACTION_FAILED);
    	}
    	
    	try {
			if(input != null) input.close();
		} catch (IOException e) {}
		Log.v("send end");
    }
    /*
    public synchronized void redef(int action)
    {
        int ret = 0 , notify = 2; 
        String path = generateDataFilePath(action);
        Log.v("redef start");
        ret = redefAction();
                        
        if(ret == 0) {
        	int data[] = receiveAction();
        	int policy_id = Settings.getCurrModel().mPolicyId;
        	CalibratePolicy.Result rlt = null;
        	
        	if(policy_id != CalibratePolicy.POLICY_ID_NONE) {
            	 rlt = CalibratePolicy.apply(policy_id, data);
        
            	 if(rlt.mResult != CalibratePolicy.RESULT_OK) {
            		 policy_id = CalibratePolicy.detect(data);
            		 rlt = CalibratePolicy.apply(policy_id, data);
            		 if(rlt.mResult == CalibratePolicy.RESULT_OK) {
            			 if(policy_id != CalibratePolicy.POLICY_ID_NONE) {
            				 Settings.updateModelProtocol(
            						 Settings.getCurrModel().mId, policy_id);
            			 }
            		 } else {
            			 rlt = CalibratePolicy.apply(CalibratePolicy.POLICY_ID_NONE, data);
            		 }
            	 }
        	} else {
       		 	policy_id = CalibratePolicy.detect(data);
       		 	rlt = CalibratePolicy.apply(policy_id, data);
       		 	if(rlt.mResult == CalibratePolicy.RESULT_OK) {
       		 		if(policy_id != CalibratePolicy.POLICY_ID_NONE) {
       		 			Settings.updateModelProtocol(
       						 Settings.getCurrModel().mId, policy_id);
       		 		}
       		 	} else {
       		 		rlt = CalibratePolicy.apply(CalibratePolicy.POLICY_ID_NONE, data);
       		 	}        		
        	}
        	
    		if(saveKeyData(path , rlt.mData)) {
    			notify = 0;
    		}

        } else if(ret == -8) {
        	notify = 1;
        }
        
        if(notify == 0) {
        	Controller.get().sendMessage(Controller.MSG_REDEF_ACTION_SUCCESS);
        } else if(notify == 1){
        	Controller.get().sendMessage(Controller.MSG_REDEF_ACTION_CANCEL);
        } else {
        	Controller.get().sendMessage(Controller.MSG_REDEF_ACTION_FAILED);
        }
        
        Log.v("redef end");
    }
    */
    private InputStream getSource(int model ,  int action)
    {
    	InputStream input;
    	String path = generateDataFilePath(mBasePath,model,action);
    	
    	File file = new File(path);
    	if(file.exists()) {
    		try {
				input = new FileInputStream(file);
				return input;
			} catch (FileNotFoundException e) {}
    	}
    	
    	path = generateDataFilePath("",model,action);
    	try {
			input = mContext.getAssets().open(path);
	    	if(input != null && input.available() > 0) {
	    		return input;
	    	}
		} catch (IOException e) {}
    	Log.w("action:"+action+" not define");
    	return null;
    }
    
    private boolean copyFile(String dst , InputStream src)
    {
    	boolean ret = true;
    	File outFile = new File(dst);
    	
    	try {
		    if(!outFile.exists()) outFile.createNewFile();
		} catch (IOException e) {
			e.printStackTrace();
			Log.e("create copy file:"+dst+" failed");
			try {
				src.close();
			} catch (IOException e1) {}
			return false;
		}
    	OutputStream out = null;
    	byte bytes[] = new byte[DATA_MAX_LENGTH*4];
    	try {
    		int count = src.read(bytes);
    		String str = new String(bytes,0,count);
    		
    		out = new FileOutputStream(outFile);
    		out.write(str.getBytes());
 		
		} catch (Exception e) {
			// TODO Auto-generated catch block
			ret = false;
			Log.e("copy file failed");
			e.printStackTrace();
		}finally {
			try {
				if(src != null) src.close();
				if(out != null) out.close();
			}catch(Exception e){}
		}
    	
    	return ret;
    }
    public boolean deriveData(String basePath)
    {
    	boolean ret = true;
    	for(Settings.Model model : Settings.getModeslList()) {
        	File file = new File(basePath+"/model"+model.mId);
        	if(!file.exists()) 	file.mkdirs();
    		for(int action = 0;action <= ActionUtils.CURR_MAX_ACTION;action++) {
    			InputStream src = getSource(model.mId,action);
    			if(src != null) {
    				ret &= copyFile(generateDataFilePath(basePath,model.mId,action),src);
    			}
    		}
    	}
    	
    	return ret;
    }
    
    public void exit()
    {
    	Controller.get().unregisterStateChangedListener(this);
    	Settings.unregisterListener(this);
        //appExit();
    }

	@Override
	public void onStateChanged(int state) {
		// TODO Auto-generated method stub
		//if(state == Controller.STATE_NORMAL) {
			//cancelRecv();
		//}
	}
	@Override
	public void onCurrModelChanged(Settings.Model m) {
		// TODO Auto-generated method stub
		File init = new File(mBasePath+"/model"+m.mId);
		if(!init.exists()) {
			init.mkdirs();
		}
	}
	@Override
	public void onModelsChanged(ArrayList<Settings.Model> list) {
		// TODO Auto-generated method stub
	}
	
}
