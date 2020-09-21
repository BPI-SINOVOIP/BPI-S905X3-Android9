/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
package com.droidlogic.videoplayer;

public class MediaKey {
        public static final int KEY_PARAMETER_AML_VIDEO_POSITION_INFO = 2000;
        public static final int KEY_PARAMETER_AML_PLAYER_TYPE_STR  = 2001;
        public static final int KEY_PARAMETER_AML_PLAYER_VIDEO_OUT_TYPE = 2002;
        public static final int KEY_PARAMETER_AML_PLAYER_SWITCH_SOUND_TRACK = 2003;         //refer to lmono,rmono,stereo,set only
        public static final int KEY_PARAMETER_AML_PLAYER_SWITCH_AUDIO_TRACK = 2004;         //refer to audio track index,set only
        public static final int KEY_PARAMETER_AML_PLAYER_TRICKPLAY_FORWARD = 2005;          //refer to forward:speed
        public static final int KEY_PARAMETER_AML_PLAYER_TRICKPLAY_BACKWARD = 2006;         //refer to backward:speed
        public static final int KEY_PARAMETER_AML_PLAYER_FORCE_HARD_DECODE = 2007;          //refer to mp3,etc.
        public static final int KEY_PARAMETER_AML_PLAYER_FORCE_SOFT_DECODE = 2008;          //refer to mp3,etc.
        public static final int KEY_PARAMETER_AML_PLAYER_GET_MEDIA_INFO = 2009;             //get media info
        public static final int KEY_PARAMETER_AML_PLAYER_FORCE_SCREEN_MODE = 2010;          //set screen mode
        public static final int KEY_PARAMETER_AML_PLAYER_SET_DISPLAY_MODE = 2011;           //set display mode
        public static final int KEY_PARAMETER_AML_PLAYER_GET_DTS_ASSET_TOTAL = 2012;        //get dts asset total number
        public static final int KEY_PARAMETER_AML_PLAYER_SET_DTS_ASSET = 2013;              //set dts asset

        public static final int KEY_PARAMETER_AML_PLAYER_HWBUFFER_STATE = 3001;             //refer to stream buffer info, hardware decoder buffer infos,get only

        public static final int KEY_PARAMETER_AML_PLAYER_RESET_BUFFER = 8000;               //top level seek..player need to reset & clearbuffers
        public static final int KEY_PARAMETER_AML_PLAYER_FREERUN_MODE = 8002;               //play ASAP...
        public static final int KEY_PARAMETER_AML_PLAYER_ENABLE_OSDVIDEO = 8003;            //play enable osd video for this player....
        public static final int KEY_PARAMETER_AML_PLAYER_DIS_AUTO_BUFFER = 8004;            //play ASAP...
        public static final int KEY_PARAMETER_AML_PLAYER_ENA_AUTO_BUFFER = 8005;            //play ASAP...
        public static final int KEY_PARAMETER_AML_PLAYER_USE_SOFT_DEMUX = 8006;             //play use soft demux
        public static final int KEY_PARAMETER_AML_PLAYER_PR_CUSTOM_DATA = 9001;             //string, playready, set only
}
