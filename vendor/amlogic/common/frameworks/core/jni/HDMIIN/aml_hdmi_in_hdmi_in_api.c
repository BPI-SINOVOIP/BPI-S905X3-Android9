/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC aml_hdmi_in_hdmi_in_api
 */

#include "aml_hdmi_in_hdmi_in_api.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#include <dirent.h>
#include <android/log.h>
#include "audio_global_cfg.h"
#include "audiodsp_control.h"
#include "audio_utils_ctl.h"
#include "mAlsa.h"
//#define LOG  printf

static unsigned char audio_state = 0;
static unsigned char audio_enable = 1;
static unsigned char video_enable = 0;

#define OUTPUT_MODE_NUM     7
typedef struct output_mode_info_{
    char name[16];
    int width;
    int height;
}output_mode_info_t;
output_mode_info_t output_mode_info[OUTPUT_MODE_NUM] =
{
    {"1080p", 1920, 1080},
    {"1080i", 1920, 1080},
    {"720p", 1280, 720},
    {"480p", 720, 480},
    {"480i", 720, 480},
    {"576p", 720, 576},
    {"576i", 720, 576},
};

static int send_command(char* cmd_file, char* cmd_string)
{
    int fd = open(cmd_file, O_RDWR);
    int len = strlen(cmd_string);
    int ret = -1;
	  if(fd >= 0){
        write(fd, cmd_string, len);
        close(fd);

         ret = 0;
	  }
	  if(ret<0){
        __android_log_print(ANDROID_LOG_INFO, "<HDMI IN>","fail to %s file %s\n", ret==-1?"open":"write", cmd_file);
	  }
    return ret;
}

static int read_value(char* prop_file)
{
    int fd = open(prop_file, O_RDONLY);
    int len = 0;
    char tmp_buf[32];
    int ret = 0;
	  if(fd >= 0){
        len = read(fd, tmp_buf, 32 );
        close(fd);
        if(len > 0){
            ret = atoi(tmp_buf);
           // __android_log_print(ANDROID_LOG_INFO, "<HDMI IN>","read %s => %d\n", prop_file, ret);
        }
	  }
	  else{
        __android_log_print(ANDROID_LOG_INFO, "<HDMI IN>","fail to open file %s\n", prop_file);
	  }
    return ret;

}

static int read_output_display_mode(void)
{
    int fd = open("/sys/class/display/mode", O_RDONLY);
    int len = 0;
    int i;
    char tmp_buf[32];
    int ret = -1;
	  if(fd >= 0){
        len = read(fd, tmp_buf, 32 );
        close(fd);
        if(len > 0){
            for(i=0; i<OUTPUT_MODE_NUM; i++){
                if(strncmp(tmp_buf, output_mode_info[i].name, strlen(output_mode_info[i].name))==0){
                    ret = i;
                    break;
                }
            }
            if(i==OUTPUT_MODE_NUM){
                __android_log_print(ANDROID_LOG_INFO, "<HDMI IN>","output mode not supported: %s\n", tmp_buf);
            }
        }
	  }
	  else{
        __android_log_print(ANDROID_LOG_INFO, "<HDMI IN>","fail to open file %s\n", "/sys/class/display/mode");
	  }
    return ret;

}


static void disp_android(void)
{
    send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "0"); // disable "hdmi in"
    //send_command("/sys/class/ppmgr/ppscaler" , "0"); // disable pscaler  for "hdmi in"

    send_command("/sys/class/vfm/map", "rm default_ext" );
    send_command("/sys/class/vfm/map", "add default_ext vdin vm amvideo" );
    send_command("/sys/module/di/parameters/bypass_prog", "0" );

     /* set and enable freescale */
    send_command("/sys/class/graphics/fb0/free_scale", "0");
    send_command("/sys/class/graphics/fb0/free_scale_axis", "0 0 1279 719 ");
    send_command("/sys/class/graphics/fb0/free_scale", "1");

    send_command("/sys/class/graphics/fb1/free_scale", "0");
    send_command("/sys/class/graphics/fb1/free_scale_axis", "0 0 1279 719 ");
    send_command("/sys/class/graphics/fb1/free_scale", "1");
    /**/

     /* turn on OSD layer */
    send_command("/sys/class/graphics/fb0/blank", "0");
    send_command("/sys/class/graphics/fb1/blank", "0");
     /**/

}

static void disp_hdmi(void)
{
    send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "0"); // disable "hdmi in"
    send_command("/sys/class/video/disable_video"           , "2"); // disable video layer, video layer will be enabled after "hdmi in" is enabled

    send_command("/sys/class/vfm/map", "rm default_ext" );
    send_command("/sys/class/vfm/map", "add default_ext vdin deinterlace amvideo" );
    send_command("/sys/module/di/parameters/bypass_prog", "1" );

     /* disable OSD layer */
    send_command("/sys/class/graphics/fb0/blank"            , "1");
    send_command("/sys/class/graphics/fb1/blank"            , "1");
     /**/

    //send_command("/sys/class/ppmgr/ppscaler"                , "0"); // disable pscaler  for "hdmi in"

     /* disable freescale */
    send_command("/sys/class/graphics/fb0/free_scale"       , "0");
    send_command("/sys/class/graphics/fb1/free_scale"       , "0");
     /**/

    send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "1"); // enable "hdmi in"

//    system("stop media");
//    system("alsa_aplay -C -Dhw:0,0 -r 48000 -f S16_LE -t raw -c 2 | alsa_aplay -Dhw:0,0 -r 48000 -f S16_LE -t raw -c 2");

}


static void disp_pip(int x, int y, int width, int height)
{
    char buf[32];
    int idx = 0;
    send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "0"); // disable "hdmi in"
    send_command("/sys/class/video/disable_video"           , "1"); // disable video layer
     /* disable OSD layer */
    //send_command("/sys/class/graphics/fb0/blank"            , "1");
    //send_command("/sys/class/graphics/fb1/blank"            , "1");
     /**/

    send_command("/sys/class/vfm/map", "rm default_ext" );
    send_command("/sys/class/vfm/map", "add default_ext vdin ppmgr amvideo" );
    send_command("/sys/module/di/parameters/bypass_prog", "0" );

     /* set and enable freescale */
    send_command("/sys/class/graphics/fb0/free_scale", "0");
    send_command("/sys/class/graphics/fb0/free_scale_axis", "0 0 1279 719");
    send_command("/sys/class/graphics/fb0/free_scale", "1");

    send_command("/sys/class/graphics/fb1/free_scale", "0");
    send_command("/sys/class/graphics/fb1/free_scale_axis", "0 0 1279 719 ");
    send_command("/sys/class/graphics/fb1/free_scale", "1");

 /**/

    send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "1"); // enable "hdmi in"

    /* set and enable pscaler */
    idx = read_output_display_mode();
    if(idx < 0){
        idx = 0;
    }
    snprintf(buf, 31, "%d %d %d %d", 0, 0, output_mode_info[idx].width-1, output_mode_info[idx].height-1);
    send_command("/sys/class/video/axis", buf);
    send_command("/sys/class/ppmgr/ppscaler", "1");
    snprintf(buf, 31, "%d %d %d %d 1", x, y, x+width-1, y+height-1);
    send_command("/sys/class/ppmgr/ppscaler_rect", buf);
     /**/

    send_command("/sys/class/video/disable_video"           , "0"); // enable video layer
     /* turn on OSD layer */
    send_command("/sys/class/graphics/fb0/blank", "0");
    send_command("/sys/class/graphics/fb1/blank", "0");
     /**/
}

static void init(void)
{
    __android_log_print(ANDROID_LOG_INFO, "<HDMI IN>","Version 0.1\n");
    //send_command("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "performance");

    send_command("/sys/class/it660x/it660x_hdmirx0/poweron", "1");
}

static void uninit(void)
{

    send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "0"); // disable "hdmi in"
    send_command("/sys/class/it660x/it660x_hdmirx0/poweron", "0"); //power off "hdmi in"

    send_command("/sys/class/vfm/map", "rm default_ext" );

}

static void audio_start(void)
{
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","audio passthrough test code start\n");
	AudioUtilsInit();
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","init audioutil finished\n");
	mAlsaInit(0, CC_FLAG_CREATE_TRACK | CC_FLAG_START_TRACK|CC_FLAG_CREATE_RECORD|/*CC_FLAG_START_RECORD|*/CC_FLAG_SOP_RECORD);
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","start audio track finished\n");
	audiodsp_start();
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","audiodsp start finished\n");
	//AudioUtilsStartLineIn();
	//__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","start line in finished\n");

}

static void audio_stop(void)
{
	AudioUtilsStopLineIn();
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","stop line in finished\n");
	AudioUtilsUninit();
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","uninit audioutil finished\n");
	mAlsaUninit(0);
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","stop audio track finished\n");
	audiodsp_stop();
	__android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","audiodsp stop finished\n");

}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_init
  (JNIEnv *env, jobject jobj){
    init();
}


/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_uninit
  (JNIEnv *env, jobject jobj){
    video_enable = 0;
    uninit();

}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    set_hdmi
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_display_1hdmi
  (JNIEnv *env, jobject jobj){

    disp_hdmi();
    video_enable = 1;
    return 0;
}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    set_android
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_display_1android
  (JNIEnv *env, jobject jobj){

    video_enable = 0;
    disp_android();
    return 0;
}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    set_pip
 * Signature: (IIII)I
 */
JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_display_1pip
  (JNIEnv *env, jobject jobj, jint x, jint y, jint width, jint height){

    disp_pip( x, y, width, height);
    video_enable = 1;
    return 0;
}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    get_h_active
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_get_1h_1active
  (JNIEnv *env, jobject jobj){

    return read_value("/sys/module/tvin_it660x/parameters/horz_active");
}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    get_v_active
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_get_1v_1active
  (JNIEnv *env, jobject jobj){

    return read_value("/sys/module/tvin_it660x/parameters/vert_active");
}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    is_dvi
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_is_1dvi
  (JNIEnv *env, jobject jobj){

    return (read_value("/sys/module/tvin_it660x/parameters/is_hdmi_mode")==0);

}

/*
 * Class:     aml_hdmi_in_hdmi_in_api
 * Method:    is_interlace
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_is_1interlace
  (JNIEnv *env, jobject jobj){

    return (read_value("/sys/module/tvin_it660x/parameters/is_interlace")==1);
}


JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_enable_1audio
  (JNIEnv *env, jobject jobj, jint flag){

    if(flag == 1){
        audio_enable = 1;
    }
    else{
        audio_enable = 0;
    }
    return 0;
}

JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1api_handle_1audio
  (JNIEnv *env, jobject jobj){

    if(video_enable&&audio_enable){
        if(audio_state == 0){
            audio_start();
            audio_state = 1;
        }

        if((read_value("/sys/module/tvin_it660x/parameters/vert_active")==0)
            ||(read_value("/sys/module/tvin_it660x/parameters/is_hdmi_mode")==0)){
            if(audio_state == 2){
                AudioUtilsStopLineIn();
                audio_state = 1;
    	          __android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","%s: stop line in finished\n", __func__);
    	      }
        }
        else{
            if(audio_state == 1){
    	        AudioUtilsStartLineIn();
              send_command("/sys/class/it660x/it660x_hdmirx0/enable"  , "257"); //0x101: bit16, event of "mailbox_send_audiodsp"
    	        audio_state = 2;
    	        __android_log_print(ANDROID_LOG_INFO, "<HDMI IN AUD>","%s: start line in finished\n", __func__);	
            }
        }
    }
    else{
        if(audio_state!=0){
            audio_stop();
            audio_state = 0;
        }
    }
    return 0;
}


