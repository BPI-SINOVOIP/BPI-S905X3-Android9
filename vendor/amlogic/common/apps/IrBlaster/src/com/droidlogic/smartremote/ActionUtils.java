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

public class ActionUtils {
	
	//you can add action but should never modify the defined actions.
	public static final int INVALID_ACTION = -1;
	//power 
	public static final int ACTION_POWER = 0;
	public static final int ACTION_POWER_ON = 1;
	public static final int ACTION_POWER_OFF = 2;
	//navigation 
	public static final int ACTION_NAViGATION_OK = 3;
	public static final int ACTION_NAViGATION_UP = 4;
	public static final int ACTION_NAViGATION_LEFT = 5;
	public static final int ACTION_NAViGATION_DOWN = 6;
	public static final int ACTION_NAViGATION_RIGHT = 7;
	public static final int ACTION_NAViGATION_BACK = 8;
	public static final int ACTION_NAViGATION_HOME = 9;
	//numbers 
	public static final int ACTION_NUM_0 = 10;
	public static final int ACTION_NUM_1 = 11;
	public static final int ACTION_NUM_2 = 12;
	public static final int ACTION_NUM_3 = 13;
	public static final int ACTION_NUM_4 = 14;
	public static final int ACTION_NUM_5 = 15;
	public static final int ACTION_NUM_6 = 16;
	public static final int ACTION_NUM_7 = 17;
	public static final int ACTION_NUM_8 = 18;
	public static final int ACTION_NUM_9 = 19;
	//functions 
	public static final int ACTION_SET_BIT = 20;
	public static final int ACTION_BACK_LOOK = 21;
	public static final int ACTION_SET_SOURCE = 22;
	public static final int ACTION_FUNC_MENU = 23;
	public static final int ACTION_FUNC_VOICE = 24;
	public static final int ACTION_FUNC_VIDEO = 25;
	public static final int ACTION_VOLUME_ADD = 26;
	public static final int ACTION_VOLUME_DEC = 27;
	public static final int ACTION_MUTE = 28;
	public static final int ACTION_CH_ADD = 29;
	public static final int ACTION_CH_DEC = 30;
	public static final int ACTION_SHOW_CH = 31;
	public static final int ACTION_VOICE_CNTL = 32;
	public static final int ACTION_SEARCH_DEV = 33;
	public static final int ACTION_DEF_1 = 34;
	public static final int ACTION_DEF_2 = 35;
	public static final int ACTION_DEF_3 = 36;
	public static final int ACTION_DEF_4 = 37;
	
	public static final int CURR_MAX_ACTION = 37;
	
	private static HashMap<Integer , Integer> sActionMap = new HashMap<Integer , Integer>() {
		{
			put(R.id.cntl_power , ACTION_POWER);
			put(-1 , ACTION_POWER_ON);
			put(-1 , ACTION_POWER_OFF );
	
			put(R.id.navagation_ok , ACTION_NAViGATION_OK );
			put(R.id.navagation_up , ACTION_NAViGATION_UP );
			put(R.id.navagation_left , ACTION_NAViGATION_LEFT );
			put(R.id.navagation_down, ACTION_NAViGATION_DOWN );
			put(R.id.navagation_right , ACTION_NAViGATION_RIGHT );
			put(R.id.navigation_back  , ACTION_NAViGATION_BACK );
			put(R.id.navigation_home , ACTION_NAViGATION_HOME );
	
			put(R.id.func_num_0 , ACTION_NUM_0 );
			put(R.id.func_num_1 , ACTION_NUM_1 );
			put(R.id.func_num_2 , ACTION_NUM_2 );
			put(R.id.func_num_3 , ACTION_NUM_3 );
			put(R.id.func_num_4 , ACTION_NUM_4 );
			put(R.id.func_num_5 , ACTION_NUM_5 );
			put(R.id.func_num_6 , ACTION_NUM_6 );
			put(R.id.func_num_7 , ACTION_NUM_7 );
			put(R.id.func_num_8 , ACTION_NUM_8 );
			put(R.id.func_num_9 , ACTION_NUM_9 );
		
			put(R.id.func_bit_set , ACTION_SET_BIT );
			put(R.id.func_back_look , ACTION_BACK_LOOK );
			put(R.id.func_source , ACTION_SET_SOURCE );     
			put(R.id.func_menu , ACTION_FUNC_MENU );
			put(R.id.func_voice , ACTION_FUNC_VOICE );
			put(R.id.func_video , ACTION_FUNC_VIDEO );
			put(R.id.cntl_voice_add , ACTION_VOLUME_ADD );
			put(R.id.cntl_voice_dec , ACTION_VOLUME_DEC );
			put(R.id.cntl_mute , ACTION_MUTE );
			put(R.id.cntl_ch_add , ACTION_CH_ADD );
			put(R.id.cntl_ch_dec , ACTION_CH_DEC );
			put(R.id.cntl_show_ch , ACTION_SHOW_CH );
			put(R.id.cntl_voice , ACTION_VOICE_CNTL );
			put(R.id.cntl_search , ACTION_SEARCH_DEV );
			put(R.id.func_def_red , ACTION_DEF_1 );
			put(R.id.func_def_green , ACTION_DEF_2 );
			put(R.id.func_def_blue , ACTION_DEF_3 );
			put(R.id.func_def_yellow , ACTION_DEF_4 );
		}

	};
	
	public static int getAction(int id) 
	{
		if(sActionMap.containsKey(id)) {
			return sActionMap.get(id);
		} else {
			return INVALID_ACTION;
		}
	}

}
