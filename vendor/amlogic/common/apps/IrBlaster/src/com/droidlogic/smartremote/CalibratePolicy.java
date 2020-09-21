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

import java.util.HashMap;

import com.droidlogic.smartremote.R;

public class CalibratePolicy {
	public static final int TOLERANCE = 15;
	
	public static final int RESULT_OK = 0;
	public static final int RESULT_WARRING = 1;
	public static final int RESULT_ERROR = 2;
	public static final int RESULT_UNKNOW = 3;
	public static final int BREAK_POINT = -1;
	
	public static final int POLICY_ID_NONE = 0;
	public static final int POLICY_ID_NEC = 1;
	public static final int POLICY_ID_SONY_SIRC = 2;
	public static final int POLICY_ID_RCA = 3;
	
	public static HashMap<Integer , Integer> sProtocols = new HashMap<Integer , Integer>()
		{
			{
				put(POLICY_ID_NONE , R.string.protocol_unknow);
				put(POLICY_ID_NEC , R.string.protocol_nec);
			}
		};
	
	public static class Result {
		public int mResult = RESULT_UNKNOW;
		public int mData[] = null;
	}
	
	private static boolean NEAR(int val , int std)
	{
		if(val >= (std-TOLERANCE) && val <= (std+TOLERANCE)) {
			return true;
		}else{
			return false;
		}
	}
	
	public static Result apply(int id,int data[]) {
		Result rlt = null;
		int temp[] = new int[data.length];
		
		for(int i=0;i<data.length;i++) {
			temp[i] = data[i];
		}
		
		switch(id) {
		case POLICY_ID_NONE :
		{
			Log.d("Calibrate with NONE");
			rlt = new Result();
			rlt.mResult = RESULT_OK;
			rlt.mData = temp;	
		}
		break;
		case POLICY_ID_NEC :
		{
			Log.d("Calibrate with NEC");
			rlt = NEC(temp);
		}
		break;
		case POLICY_ID_SONY_SIRC :
		{
			Log.d("Calibrate with SONY_SIRC");
			rlt = SONY_SIRC(temp);
		}
		break;
		case POLICY_ID_RCA :
		{
			Log.d("Calibrate with RCA");
			rlt = RCA(temp);
		}
		break;
		default:
		{
			rlt = new Result();
			rlt.mResult = RESULT_OK;
			rlt.mData = temp;
		}
		}
		
		return rlt;
	}
	
	private static Result NEC(int data[]) 
	{
		Result rlt = new Result();
		int length = data.length;
		int klen = 0;
		boolean isAfterKey = false;
		
		rlt.mData = data;
		for(int i=0;i<length;i++) {
			int key = data[i];
			if(key > 1500) {
				//data[i] = BREAK_POINT;
				klen = i;
				break;
			} else if(key > 700) {
				data[i] = 900;
			} else if(key > 300) {
				data[i] = 450;
			} else if(key > 100) {
				data[i] = 168;
			} else if(key > 0) {
				data[i] = 56;
			} else {
				Log.e("do nec calibrete failed , invalid data item");
				rlt.mResult = RESULT_ERROR;
				return rlt;
			}
		}
		
		if(data[0] != 900 || data[1] != 450) {
			Log.e("do nec calibrete failed , error type");
			rlt.mResult = RESULT_ERROR;
			return rlt;
		}
		
		if(klen < 64) {
			Log.w("do nec calibrete failed , warring length");
			rlt.mResult = RESULT_WARRING;
			return rlt;
		}
				
		rlt.mData = data;
		rlt.mResult = RESULT_OK;

		return rlt;
	}
	
	private static Result SONY_SIRC(int data[])
	{
		Result rlt = new Result();
		int length = data.length;
		int klen = 0;
		boolean isAfterKey = false;
		
		rlt.mData = data;
		for(int i=0;i<length;i++) {
			int key = data[i];
			if(key > 900) {
				//data[i] = BREAK_POINT;
				klen = i;
				break;
			} else if(key > 200) {
				data[i] = 240;
			} else if(key > 90) {
				data[i] = 120;
			} else if(key > 0) {
				data[i] = 60;
			} else {
				Log.e("do SONY_SIRC calibrete failed , invalid data item");
				rlt.mResult = RESULT_ERROR;
				return rlt;
			}
		}
		
		if(data[0] != 240 || data[1] != 60) {
			Log.e("do SONY_SIRC calibrete failed , error type");
			rlt.mResult = RESULT_ERROR;
			return rlt;
		}
		
		if(klen < 26) {
			Log.w("do SONY_SIRC calibrete failed , warring length");
			rlt.mResult = RESULT_WARRING;
			return rlt;
		}
				
		rlt.mData = data;
		rlt.mResult = RESULT_OK;

		return rlt;
	}
	
	private static Result RCA(int data[])
	{
		Result rlt = new Result();
		int length = data.length;
		int klen = 0;
		boolean isAfterKey = false;
		
		rlt.mData = data;
		for(int i=0;i<length;i++) {
			int key = data[i];
			if(key > 900) {
				//data[i] = BREAK_POINT;
				klen = i;
				break;
			} else if(key > 375) {
				data[i] = 400;
			} else if(key > 160) {
				data[i] = 200;
			} else if(key > 75) {
				data[i] = 100;
			} else if(key > 0) {
				data[i] = 50;
			} else {
				Log.e("do RCA calibrete failed , invalid data item");
				rlt.mResult = RESULT_ERROR;
				return rlt;
			}
		}
		
		if(data[0] != 400 || data[1] != 400) {
			Log.e("do RCA calibrete failed , error type");
			rlt.mResult = RESULT_ERROR;
			return rlt;
		}
		
		if(klen < 26) {
			Log.w("do RCA calibrete failed , warring length");
			rlt.mResult = RESULT_WARRING;
			return rlt;
		}
				
		rlt.mData = data;
		rlt.mResult = RESULT_OK;

		return rlt;
	}
	
	public static int detect(int data[]) 
	{
		Log.d("do detect");
		int result = POLICY_ID_NONE;
		switch(POLICY_ID_NONE) {
		case POLICY_ID_NONE:
		case POLICY_ID_NEC :
		{
			if(data.length >= 64 && NEAR(data[0],900) && NEAR(data[1],450)){
				boolean get_it = true;
				for(int i=2;i<62;i++) {
					if(!NEAR(data[i],56) && !NEAR(data[i],168)) {
						get_it = false;
						break;
					}
				}
				if(get_it) {
					result = POLICY_ID_NEC;
					Log.d("get policy NEC");
					break;
				}
			}
		}
		case POLICY_ID_SONY_SIRC :
		{
			if(data.length >= 26 && NEAR(data[0],240)){
				boolean get_it = true;
				for(int i=1;i<26;i++) {
					if(!NEAR(data[i],60) && !NEAR(data[i],120)) {
						get_it = false;
						break;
					}
				}
				if(get_it) {
					result = POLICY_ID_SONY_SIRC;
					break;
				}
			}
		}
		case POLICY_ID_RCA :
		{
			if(data.length >= 26 && NEAR(data[0],400) && NEAR(data[1],400)){
				boolean get_it = true;
				for(int i=2;i<26;i++) {
					if(!NEAR(data[i],50) && !NEAR(data[i],200) && NEAR(data[i],100)) {
						get_it = false;
						break;
					}
				}
				if(get_it) {
					result = POLICY_ID_RCA;
					break;
				}
			}
		}
		}
		return result;
	}
}
