/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _PLAYER_H_
#define _PLAYER_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define MSG_SIZE                    64
#define MAX_VIDEO_STREAMS           8
#define MAX_AUDIO_STREAMS           8
#define MAX_SUB_INTERNAL            8
#define MAX_SUB_EXTERNAL            24
#define MAX_SUB_STREAMS             (MAX_SUB_INTERNAL + MAX_SUB_EXTERNAL)
#define MAX_PLAYER_THREADS          32

#define CALLBACK_INTERVAL			(300)
 #define DEBUG_VARIABLE_DUR

typedef enum
{      
	/******************************
	* 0x1000x: 
	* player do parse file
	* decoder not running
	******************************/
	PLAYER_INITING  	= 0x10001,
	PLAYER_TYPE_REDY  = 0x10002,
	PLAYER_INITOK   	= 0x10003,	

	/******************************
	* 0x2000x: 
	* playback status
	* decoder is running
	******************************/
	PLAYER_RUNNING  	= 0x20001,
	PLAYER_BUFFERING 	= 0x20002,
	PLAYER_PAUSE    	= 0x20003,
	PLAYER_SEARCHING	= 0x20004,
	
	PLAYER_SEARCHOK 	= 0x20005,
	PLAYER_START    	= 0x20006,	
	PLAYER_FF_END   	= 0x20007,
	PLAYER_FB_END   	= 0x20008,

	PLAYER_PLAY_NEXT	= 0x20009,	

	/******************************
	* 0x3000x: 
	* player will exit	
	******************************/
	PLAYER_ERROR		= 0x30001,
	PLAYER_PLAYEND  	= 0x30002,	
	PLAYER_STOPED   	= 0x30003,  
	PLAYER_EXIT   		= 0x30004, 
	
}player_status;

typedef enum 
{
    UNKNOWN_FILE = 0,
    AVI_FILE 		,
    MPEG_FILE		,
    WAV_FILE		,
    MP3_FILE		,
    AAC_FILE		,
    AC3_FILE		,
    RM_FILE			,
    DTS_FILE		,        
    MKV_FILE		,    
    MOV_FILE       ,
    MP4_FILE		,		
    FLAC_FILE		,
    H264_FILE		,
    M2V_FILE        ,
    FLV_FILE		,
    P2P_FILE        ,
    ASF_FILE        ,
    FILE_MAX		,        
}pfile_type;

typedef struct
{
	char id;
	char internal_external; //0:internal_sub 1:external_sub       
	unsigned short width;
	unsigned short height;
	unsigned int sub_type;
	char resolution;
	long long subtitle_size;  
	char *sub_language;   
}msub_info_t;

typedef struct
{	
	char *filename;
	int  duration;  
	long long  file_size;
	pfile_type type;
	int bitrate;
	int has_video;
	int has_audio;
	int has_sub;
	int nb_streams;
	int total_video_num;
	int cur_video_index;
	int total_audio_num;
	int cur_audio_index;
	int total_sub_num;      
	int cur_sub_index;	
	int seekable;
}mstream_info_t;

typedef struct
{	
	mstream_info_t stream_info;
	void *video_info[MAX_VIDEO_STREAMS];
	void *audio_info[MAX_AUDIO_STREAMS];
    	msub_info_t *sub_info[MAX_SUB_STREAMS];
}media_info_t;

typedef struct pid_info
{
	int num;
	int pid[MAX_PLAYER_THREADS];
}pid_info_t;

typedef struct player_file_type
{
	const char *fmt_string;
	int video_tracks;
	int audio_tracks;
	int subtitle_tracks;
	/**/
}player_file_type_t;


#define state_pre(sta) (sta>>16)
#define player_thread_init(sta)	(state_pre(sta)==0x1)
#define player_thread_run(sta)	(state_pre(sta)==0x2)
#define player_thread_stop(sta)	(state_pre(sta)==0x3)

typedef int (*update_state_fun_t)(int pid,player_info_t *) ;
typedef int (*notify_callback)(int pid,int msg,unsigned long ext1,unsigned long ext2);
typedef enum
{      
	PLAYER_EVENTS_PLAYER_INFO=1,			///<ext1=player_info*,ext2=0,same as update_statue_callback 
	PLAYER_EVENTS_STATE_CHANGED,			///<ext1=new_state,ext2=0,
	PLAYER_EVENTS_ERROR,					///<ext1=error_code,ext2=message char *
	PLAYER_EVENTS_BUFFERING,				///<ext1=buffered=d,d={0-100},ext2=0,
	PLAYER_EVENTS_FILE_TYPE,				///<ext1=player_file_type_t*,ext2=0
}player_events;

typedef struct
{
    update_state_fun_t update_statue_callback;
    int update_interval;
    long callback_old_time;
	notify_callback	   notify_fn;
}callback_t;

typedef struct
 {
	char  *file_name;	
    char  *headers;
	//List  *play_list;
	int	video_index;
	int	audio_index;
	int sub_index;
	int t_pos;	
	int	read_max_cnt;
	union
	{     
		struct{
			unsigned int loop_mode:1;
			unsigned int nosound:1;	
			unsigned int novideo:1;	
			unsigned int hassub:1;
			unsigned int need_start:1;
			#ifdef DEBUG_VARIABLE_DUR
			unsigned int is_variable:1;
			#endif
		};
		int mode;
	};  
	callback_t callback_fn;
	int byteiobufsize;
	int loopbufsize;
	int enable_rw_on_pause;
	/*
	data%<min && data% <max  enter buffering;
	data% >middle exit buffering;
	*/
	int auto_buffing_enable;
	float buffing_min;
	float buffing_middle;
	float buffing_max;
	int is_playlist;
 }play_control_t; 
 
int 	player_init();
int     player_start(play_control_t *p,unsigned long  priv);
int 	player_stop(int pid);
int 	player_stop_async(int pid);
int     player_exit(int pid);
int 	player_pause(int pid);
int	 	player_resume(int pid);
int 	player_timesearch(int pid,int s_time);
int     player_forward(int pid,int speed);
int     player_backward(int pid,int speed);
int     player_aid(int pid,int audio_id);
int     player_sid(int pid,int sub_id);
int 	player_progress_exit(void);
int     player_list_allpid(pid_info_t *pid);
int     check_pid_valid(int pid);
int 	player_get_play_info(int pid,player_info_t *info);
int 	player_get_media_info(int pid,media_info_t *minfo);
int 	player_video_overlay_en(unsigned enable);
int 	player_start_play(int pid);
//int 	player_send_message(int pid, player_cmd_t *cmd);
player_status 	player_get_state(int pid);
unsigned int 	player_get_extern_priv(int pid);


int 	audio_set_mute(int pid,int mute);
int 	audio_get_volume_range(int pid,int *min,int *max);
int 	audio_set_volume(int pid,float val);
int 	audio_get_volume(int pid);
int 	audio_set_volume_balance(int pid,int balance);
int 	audio_swap_left_right(int pid);
int 	audio_left_mono(int pid);
int 	audio_right_mono(int pid);
int 	audio_stereo(int pid);
int 	audio_set_spectrum_switch(int pid,int isStart,int interval);
int 	player_register_update_callback(callback_t *cb,update_state_fun_t up_fn,int interval_s);
char 	*player_status2str(player_status status);

//control interface
int     player_loop(int pid);
int     player_noloop(int pid);

int 	check_url_type(char *filename);
int 	play_list_player(play_control_t *pctrl,unsigned long priv);

//freescale
int 	enable_freescale(int cfg);
int 	disable_freescale(int cfg);

#ifdef  __cplusplus
}
#endif

#endif

