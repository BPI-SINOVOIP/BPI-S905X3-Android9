/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DROID_LOGIC_SERVER_HDMIIN
 */

#define LOG_NDEBUG 0
#define LOG_TAG "hdmiin-jni"

#include <jni.h>
#include <JNIHelp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include <utils/Log.h>
#include "HDMIIN/audio_global_cfg.h"
#include "HDMIIN/audiodsp_control.h"
#include "HDMIIN/audio_utils_ctl.h"
#include "HDMIIN/mAlsa.h"
#include "cutils/properties.h"

#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <android_runtime/android_view_Surface.h>
#include <linux/videodev2.h>
#include <hardware/hardware.h>
#include <hardware/aml_screen.h>

#include <droid_logic_server_HDMIIN.h>

namespace android
{

static unsigned char audioState = 0;
static unsigned char audioEnable = 1;
static unsigned char audioReady = 0;
static unsigned char videoEnable = 0;
static char audioRate[32];
static int rate = 0;

static char classPath[PATH_MAX] = {0,};
static char paramPath[PATH_MAX] = {0,};
static bool sysfsChecked = false;
static bool useSii9293 = false;
static bool useSii9233a = false;

static char vfmTvpath[PATH_MAX] = {0,};
static char vfmDefaultExt[PATH_MAX] = {0,};
static char vfmDefaultAmlvideo2[PATH_MAX] = {0,};
static char vfmHdmiin[PATH_MAX] = {0,};
static bool rmPathFlag = false;
static int inputSource = 0;
static sp<ANativeWindow> window = NULL;
static bool useVideoLayer = true;
static bool isDisplayFullscreen = false;

//check use ppmgr
static bool usePpmgr = false;

enum State{
    START,
    PAUSE,
    STOPING,
    STOP,
};
aml_screen_module_t* screenModule;
aml_screen_device_t* screenDev;
int displayWidth = 0;
int displayHeight = 0;
int displayState = STOP;

typedef struct output_mode_info_ {
    char name[16];
    int width;
    int height;
    int scale_width;
    int scale_height;
    bool iMode;
} output_mode_info_t;

output_mode_info_t output_mode_info[] = {
    {"1080p60hz", 1920, 1080, 720, 405, false},
    {"1080p24hz", 1920, 1080, 720, 405, false},
    {"1080p50hz", 1920, 1080, 720, 405, false},
    {"1080i60hz", 1920, 1080, 720, 405, true},
    {"1080i50hz", 1920, 1080, 720, 405, true},
    {"720p60hz", 1280, 720, 480, 270, false},
    {"720p50hz", 1280, 720, 480, 270, false},
    {"480p60hz", 720, 480, 360, 240, false},
    {"480i60hz", 720, 480, 360, 240, true},
    {"576p50hz", 720, 576, 360, 288, false},
    {"576i50hz", 720, 576, 360, 288, true},
};

#define OUTPUT_MODE_NUM (sizeof(output_mode_info) / sizeof(output_mode_info[0]))

static int sendCommand(const char* cmdFile, const char* cmdString) {
    int fd = open(cmdFile, O_RDWR);
    int len = strlen(cmdString);
    int ret = -1;

    if(fd >= 0) {
        write(fd, cmdString, len);
        close(fd);
        ret = 0;
    } if(ret < 0) {
        ALOGE("fail to %s file %s", ret==-1?"open":"write", cmdFile);
    }

    return ret;
}

static int readValue(const char* propFile) {
    int fd = open(propFile, O_RDONLY);
    int len = 0;
    char tmpBuf[32] = {0,};
    int ret = 0;

    if(fd >= 0) {
        len = read(fd, tmpBuf, 32);
        close(fd);
        if(len > 0)
            ret = atoi(tmpBuf);
    } else {
        ALOGE("fail to open file %s", propFile);
    }

    return ret;
}

static int readValue(const char* propFile, const int pos) {
    int fd = open(propFile, O_RDONLY);
    int len = 0;
    char tmpBuf[128] = {0,};
    int ret = 0;

    memset(tmpBuf, 0, sizeof(tmpBuf));
    if(fd >= 0) {
        len = read(fd, tmpBuf, 128);
        close(fd);
        if(len > 0)
            ret = (tmpBuf[pos] == '0') ? 0 : 1;
    } else {
        ALOGE("fail to open file %s", propFile);
    }

    return ret;
}

static int readValue(const char* propFile, char* buf) {
    int fd = open(propFile, O_RDONLY);
    int len = 0;
    char tmpBuf[128] = {0,};
    int ret = -1;

    memset(tmpBuf, 0, sizeof(tmpBuf));
    if (fd >= 0) {
        len = read(fd, tmpBuf, 128);
        close(fd);
        if (len > 0) {
            strncpy(buf, tmpBuf, strlen(tmpBuf) - 1);
            buf[strlen(tmpBuf)] = '\0';
            ret = 0;
        }
    }

    return ret;
}

enum {
    HDMI_TYPE,
    DISPLAY_TYPE
};

static int getInputInfo(char* mode, const int type, char* buf) {
    int ret = -1;
    char* context = NULL;
    char* ptr = NULL;

    ptr = strtok_r(mode, ":", &context);
    if (ptr != NULL) {
        if (type == HDMI_TYPE) {
            strcpy(buf, ptr);
            ret = 0;
        } else if (type == DISPLAY_TYPE) {
            ptr = strtok_r(NULL, ":", &context);
            if (ptr != NULL) {
                strcpy(buf, ptr);
                ret = 0;
            }
        }
    }

    return ret;
}

static int readOutputDisplayMode() {
    int fd = open(DISPLAY_MODE_PATH, O_RDONLY);
    int len = 0;
    unsigned int i;
    char tmpBuf[32] = {0,};
    int ret = -1;
    if (fd >= 0) {
        memset(tmpBuf, 0, sizeof(tmpBuf));
        len = read(fd, tmpBuf, 32);
        close(fd);
        if (len > 0) {
            for (i = 0; i < OUTPUT_MODE_NUM; i++) {
                if (strncmp(tmpBuf, output_mode_info[i].name, strlen(output_mode_info[i].name)) == 0) {
                    ret = i;
                    break;
                }
            }
            if (i == OUTPUT_MODE_NUM) {
                ALOGE("output mode not supported: %s", tmpBuf);
            }
        }
    } else
        ALOGE("fail to open file /sys/class/display/mode");

    return ret;
}

static char* getFs(const char* path, const char* key, char *buf) {
    strncpy(buf, path, strlen(path));
    strcat(buf, key);
    return buf;
}

static void dispAndroid() {
    char fsBuf[PATH_MAX] = {0,};
    memset(fsBuf, 0, sizeof(fsBuf));
    sendCommand(getFs(classPath, "enable", fsBuf) , "0"); // disable "hdmi in"
    //sendCommand(PPSCALER_PATH, "0"); // disable pscaler  for "hdmi in"

    sendCommand(VFM_MAP_PATH, "rm default_ext" );
    sendCommand(VFM_MAP_PATH, "add default_ext vdin0 vm amvideo" );
    sendCommand(BYPASS_PROG_PATH, "0" );

     /* set and enable freescale */
    sendCommand(FREESCALE_PATH, "0");
    sendCommand(FREESCALE_AXIS_PATH, "0 0 1279 719 ");
    sendCommand(FREESCALE_PATH, "1");

    sendCommand(FREESCALE_1_PATH, "0");
    sendCommand(FREESCALE_1_AXIS_PATH, "0 0 1279 719 ");
    sendCommand(FREESCALE_1_PATH, "1");

     /* turn on OSD layer */
    sendCommand(OSD_PATH, "0");
    //sendCommand(OSD_1_PATH, "0");
}

static void dispHdmi() {
    char fsBuf[PATH_MAX] = {0,};
    memset(fsBuf, 0, sizeof(fsBuf));
    sendCommand(getFs(classPath, "enable", fsBuf) , "0"); // disable "hdmi in"
    sendCommand(DISABLE_VIDEO_PATH, "2"); // disable video layer, video layer will be enabled after "hdmi in" is enabled

    sendCommand(VFM_MAP_PATH, "rm default_ext" );
    sendCommand(VFM_MAP_PATH, "add default_ext vdin0 deinterlace amvideo" );
    sendCommand(BYPASS_PROG_PATH, "1" );

     /* disable OSD layer */
    sendCommand(OSD_PATH, "1");
    //sendCommand(OSD_1_PATH, "1");
    //sendCommand(PPSCALER_PATH, "0"); // disable pscaler  for "hdmi in"

     /* disable freescale */
    sendCommand(FREESCALE_PATH, "0");
    sendCommand(FREESCALE_1_PATH, "0");

    sendCommand(getFs(classPath, "enable", fsBuf) , "1"); // enable "hdmi in"

//    system("stop media");
//    system("alsa_aplay -C -Dhw:0,0 -r 48000 -f S16_LE -t raw -c 2 | alsa_aplay -Dhw:0,0 -r 48000 -f S16_LE -t raw -c 2");
}


static void dispPip(int x, int y, int width, int height) {
    char buf[32];
    int idx = 0;
    displayWidth = width;
    displayHeight = height;

    if (usePpmgr) {
        ALOGV("disp_pip(), usePpmgr");
        /* set and enable pscaler */
        idx = readOutputDisplayMode();
        if (idx < 0) {
            idx = 0;
        }
        snprintf(buf, 31, "%d %d %d %d", 0, 0, output_mode_info[idx].width-1, output_mode_info[idx].height-1);
        sendCommand(VIDEO_AXIS_PATH, buf);
        sendCommand(PPSCALER_PATH, "1");
        snprintf(buf, 31, "%d %d %d %d 1", x, y, x+width-1, y+height-1);
        sendCommand(PPSCALER_RECT_PATH, buf);
    } else {
        /* sendCommand(PPSCALER_PATH, "0");
        sendCommand(FREESCALE_PATH, "0");
        sendCommand(FREESCALE_1_PATH, "0");
        sendCommand(SCREEN_MODE_PATH, "1"); // 1:full stretch
        snprintf(buf, 31, "%d %d %d %d 1", x, y, x+width-1, y+height-1);
        sendCommand(VIDEO_AXIS_PATH, buf); */
    }
}

static int checkExistFile(const char* path, const char* name) {
    struct stat buf;
    char sysfsPath[PATH_MAX] = {0,};

    strncpy(sysfsPath, path, strlen(path));
    if (stat(strcat(sysfsPath, name), &buf) == 0)
        return 0;

    return -1;
}

static int checkExist(const char* path) {
    return checkExistFile(path, "poweron");
}

static void checkSysfs() {
    if (sysfsChecked)
        return;

    sysfsChecked = true;
    if (checkExist(IT660X_CLASS_PATH) == 0) {
        strncpy(classPath, IT660X_CLASS_PATH, strlen(IT660X_CLASS_PATH));
        strncpy(paramPath, IT660X_PARAM_PATH, strlen(IT660X_PARAM_PATH));
    } else if (checkExist(MTBOX_CLASS_PATH) == 0) {
        strncpy(classPath, MTBOX_CLASS_PATH, strlen(MTBOX_CLASS_PATH));
        strncpy(paramPath, MTBOX_PARAM_PATH, strlen(MTBOX_PARAM_PATH));
    } else if (checkExistFile(SII9233A_CLASS_PATH, "port") == 0) {
        useSii9233a = true;
        strncpy(classPath, SII9233A_CLASS_PATH, strlen(SII9233A_CLASS_PATH));
        strncpy(paramPath, SII9233A_PARAM_PATH, strlen(SII9233A_PARAM_PATH));
    } else if (checkExistFile(SII9293_CLASS_PATH, "enable") == 0) {
        useSii9293 = true;
        strncpy(classPath, SII9293_CLASS_PATH, strlen(SII9293_CLASS_PATH));
        strncpy(paramPath, SII9293_PARAM_PATH, strlen(SII9293_PARAM_PATH));
    } else {
        // TODO: add more
    }
    ALOGV("classPath %s", classPath);
    ALOGV("paramPath %s", paramPath);
}

static void getPath(const char *name, char *vfmPath) {
    int len = 0;
    int offset = 0;
    char* ptr = NULL;
    char* starptr = NULL;
    char buf[1024*4]={0,};
    int fd = open(VFM_MAP_PATH, O_RDWR);
    if(fd >= 0) {
        len = read(fd, buf, sizeof(buf));
        close(fd);
        if(len > 0) {
            ptr = strstr(buf, name);
            if(ptr) {
                starptr = ptr;
                ptr = strchr(starptr, '}');
                if(ptr) {
                    offset = ptr - starptr + 1;
                    strncpy(vfmPath, starptr, offset);
                }
            }
        }
    }
}

static void resumePath(const char *name, char *vfmPath)
{
    char* ptr = NULL;
    char* endptr = NULL;
    int offset = 0;
    char path[512]={0,};
    char buf[512]={0,};
    char *bufptr = NULL;
    char *context = NULL;
    char cmd[32] = {0,};

    if(strlen(vfmPath) > 0) {
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "rm %s\n", name);
        sendCommand(VFM_MAP_PATH, cmd);

        endptr = strchr(vfmPath, '}');
        ptr = strchr(vfmPath, '{');
        strncpy(buf, ptr + 1, endptr - ptr - 1);
        bufptr = buf;
        snprintf(path, sizeof(path), "add %s ", name);
        while ((ptr = strtok_r(bufptr, " ", &context)) != NULL) {
            if (strlen(ptr) > 0) {
                endptr = strchr(ptr, '(');
                if (endptr != NULL) {
                    offset = endptr - ptr;
                    strncat(path, ptr, offset);
                    strcat(path, " ");
                } else
                    strcat(path, ptr);
            }
            bufptr = NULL;
        }
        sendCommand(VFM_MAP_PATH, path);
    }
}

static bool searchPath(const char *vfmPath, const char *module) {
    char *ptr = NULL;
    char *endptr = NULL;
    char buf[512] = {0,};
    char *bufptr = NULL;
    char *context = NULL;
    char node[128] = {0,};

    if (strlen(vfmPath) > 0) {
        endptr = strchr(vfmPath, '}');
        ptr = strchr(vfmPath, '{');
        strncpy(buf, ptr + 1, endptr - ptr - 1);
        bufptr = buf;
        while ((ptr = strtok_r(bufptr, " ", &context)) != NULL) {
            if (strlen(ptr) > 0) {
                endptr = strchr(ptr, '(');
                memset(node, 0, sizeof(node));
                if (endptr != NULL) {
                    strncpy(node, ptr, endptr - ptr);
                } else
                    strcpy(node, ptr);
                if (!strcmp(node, module))
                    return true;
            }
            bufptr = NULL;
        }
    }

    return false;
}

static void safeRmPath(const char* path) {
    int retry = 20; //retry 20 times
    int len = 0;
    int readRlt = 0;
    char buf[512]={0,};
    char cmd[512]={0,};

    if (!strcmp(path, "hdmiin") && strlen(vfmHdmiin) > 0) {
        if (!searchPath(vfmHdmiin, "amvideo")) {
            sendCommand(VFM_MAP_PATH, "rm hdmiin");
            return;
        }
    }

    strcpy(cmd, "rm ");
    while(retry > 0) {
        int fd = open(VIDEO_FRAME_COUNT_PATH, O_RDONLY);
        if (fd >= 0) {
            memset(buf, 0, sizeof(buf));
            len = read(fd, buf, sizeof(buf));
            close(fd);
            if (len > 0) {
                readRlt = atoi(buf);
                if(readRlt > 0)
                    sleep(1);  // sleep 10 ms
                else if(readRlt == 0) {
                    strcat(cmd, path);
                    sendCommand(VFM_MAP_PATH, cmd);
                    break;
                } else
                    break;
            }
        }
        retry--;

        if(retry == 0) {
            strcat(cmd, path);
            sendCommand(VFM_MAP_PATH, cmd);
        }
    }
}

static bool checkBoolProp(const char *name, const char *def) {
    char prop[PROPERTY_VALUE_MAX] = {0,};

    property_get(name, prop, def);
    if (!strcmp(prop, "true"))
        return true;

    return false;
}

static void getScreenDev() {
    if (screenDev)
        return;

    if (!screenModule)
        hw_get_module(AML_SCREEN_HARDWARE_MODULE_ID, (const hw_module_t **)&screenModule);

    if (screenModule)
        screenModule->common.methods->open((const hw_module_t *)screenModule, AML_SCREEN_SOURCE,
                (struct hw_device_t **)&screenDev);
}

static void init(JNIEnv *env, jobject obj, int source, bool isFullscreen) {
    char fsBuf[PATH_MAX] = {0,};
    checkSysfs();
    usePpmgr = checkBoolProp(PROP_HDMIIN_PPMGR, "false");
    useVideoLayer = checkBoolProp(PROP_HDMIIN_VIDEOLAYER, "true");
    isDisplayFullscreen = isFullscreen;
    inputSource = source;

    int retry = 5;
    if (useSii9293) {
        while (retry > 0) {
            memset(fsBuf, 0, sizeof(fsBuf));
            if (readValue(getFs(classPath, "drv_init_flag", fsBuf), 16) == 1)
                break;
            retry--;
            ALOGV("retry: %d, sleep 100ms", retry);
            usleep(100000);
        }
    }

    // sendCommand("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "performance");
    if (useSii9233a || useSii9293) {
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "0\n");
    } else {
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "0"); // disable "hdmi in"
    }

    if (isDisplayFullscreen && useVideoLayer)
        sendCommand(DISABLE_VIDEO_PATH, "1"); // disable video layer
    else {
        getScreenDev();
        if (screenDev != NULL) {
            screenDev->ops.set_source_type(screenDev, HDMI_IN);
        } else
            ALOGE("mScreenDev == NULL");
    }

    //rm tvpath for conflict
    memset(vfmTvpath, 0, sizeof(vfmTvpath));
    memset(vfmDefaultExt, 0, sizeof(vfmDefaultExt));
    memset(vfmDefaultAmlvideo2, 0, sizeof(vfmDefaultAmlvideo2));
    memset(vfmHdmiin, 0, sizeof(vfmHdmiin));
    getPath("tvpath ", vfmTvpath);
    sendCommand(VFM_MAP_PATH, "rm tvpath");
    getPath("default_ext ", vfmDefaultExt);
    sendCommand(VFM_MAP_PATH, "rm default_ext");
    if (!isDisplayFullscreen || !useVideoLayer) {
        getPath("default_amlvideo2 ", vfmDefaultAmlvideo2);
        sendCommand(VFM_MAP_PATH, "rm default_amlvideo2");
    }

    safeRmPath("hdmiin");
    if (usePpmgr) {
        ALOGV("usePpmgr\n");
        // sendCommand(VFM_MAP_PATH, "rm default_amlvideo2" );
        sendCommand(VFM_MAP_PATH, "add hdmiin vdin0 amlvideo2 ppmgr amvideo" );

        /* set and enable freescale */
        sendCommand(FREESCALE_PATH, "0");
        sendCommand(FREESCALE_AXIS_PATH, "0 0 1279 719");
        sendCommand(FREESCALE_PATH, "1");
        sendCommand(FREESCALE_1_PATH, "0");
        sendCommand(FREESCALE_1_AXIS_PATH, "0 0 1279 719 ");
        sendCommand(FREESCALE_1_PATH, "1");
    } else {
        if (isDisplayFullscreen && useVideoLayer)
            sendCommand(VFM_MAP_PATH, "add hdmiin vdin0 amvideo");
        else
            sendCommand(VFM_MAP_PATH, "add hdmiin vdin0 amlvideo2");
    }

    if (useSii9233a) {
        char port[8] = {0,};
        memset(port, 0, sizeof(port));
        sprintf(port, "%d\n", inputSource);
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "port", fsBuf), port);
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "1\n");
    } else if (useSii9293) {
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "1\n");
    } else {
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "poweron", fsBuf), "1");
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "1");
    }
    if (isDisplayFullscreen && useVideoLayer)
        sendCommand(HDMIIN_ON_VIDEO_PATH, "1");
}

static void setMwFull();

static void deinit(JNIEnv *env, jobject obj) {
    char fsBuf[PATH_MAX] = {0,};
    videoEnable = 0;

    if (useSii9233a || useSii9293) {
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "0\n"); // disable "hdmi in"
    } else {
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "enable", fsBuf), "0"); // disable "hdmi in"
        memset(fsBuf, 0, sizeof(fsBuf));
        sendCommand(getFs(classPath, "poweron", fsBuf), "0"); //power off "hdmi in"
    }

    if (isDisplayFullscreen && useVideoLayer)
        sendCommand(DISABLE_VIDEO_PATH, "2");

    memset(vfmHdmiin, 0, sizeof(vfmHdmiin));
    getPath("hdmiin ", vfmHdmiin);
    safeRmPath("hdmiin");
    resumePath("tvpath", vfmTvpath);
    resumePath("default_ext", vfmDefaultExt);
    if (!isDisplayFullscreen || !useVideoLayer)
        resumePath("default_amlvideo2", vfmDefaultAmlvideo2);
    else
        sendCommand(HDMIIN_ON_VIDEO_PATH, "0");
    sysfsChecked = false;
}

static void setMwFull() {
    char value[32] = {0,};
    int ret = 0;
    unsigned int i = 0;
    int width = 0;
    int height = 0;

    memset(value, 0, sizeof(value));
    ret = readValue(DISPLAY_MODE_PATH, value);
    if (ret == -1)
        return;

    for (i = 0; i < OUTPUT_MODE_NUM; i++) {
        if (strstr(value, output_mode_info[i].name) != NULL) {
            width = output_mode_info[i].width;
            height = output_mode_info[i].height;
            break;
        }
    }
    if (width == 0 || height == 0)
        return;

    memset(value, 0, sizeof(value));
    sprintf(value, "0 0 %d %d", width - 1, height - 1);
    sendCommand(WINDOW_AXIS_PATH, value);
    sendCommand(FREESCALE_PATH, "0x10001");
}

static void setMainWindowFull(JNIEnv *env, jobject obj) {
    setMwFull();
}

static void setMainWindowPosition(JNIEnv *env, jobject obj, jint x, jint y) {
    char value[32] = {0,};
    int ret = 0;
    unsigned int i = 0;
    int width = 0;
    int height = 0;

    memset(value, 0, sizeof(value));
    ret = readValue(DISPLAY_MODE_PATH, value);
    if (ret == -1)
        return;

    for (i = 0; i < OUTPUT_MODE_NUM; i++) {
        if (strstr(value, output_mode_info[i].name) != NULL) {
            width = output_mode_info[i].scale_width;
            height = output_mode_info[i].scale_height;
            break;
        }
    }
    if (width == 0 || height == 0)
        return;

    memset(value, 0, sizeof(value));
    sprintf(value, "%d %d %d %d", x, y, x + width - 1, y + height - 1);
    sendCommand(WINDOW_AXIS_PATH, value);
    sendCommand(FREESCALE_PATH, "0x10001");
}

static void startVideo(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer) {
        int ret = readValue(DISABLE_VIDEO_PATH);
        if (ret != 2 && ret != 0) {
            sendCommand(DISABLE_VIDEO_PATH, "2");
        }
        videoEnable = 1;
    }
}

static void stopVideo(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer)
        sendCommand(DISABLE_VIDEO_PATH, "1");
}

static void audioStart(int trackRate) {
    sendCommand(RECORD_TYPE_PATH, "2");
    AudioUtilsInit();
    ALOGV("init audioutil finished, track_rate: %d", trackRate);
    mAlsaInit(0, CC_FLAG_CREATE_TRACK | CC_FLAG_START_TRACK|CC_FLAG_CREATE_RECORD|CC_FLAG_START_RECORD, trackRate);
    ALOGV("start audio track finished");
    // audiodsp_start();
    ALOGV("audiodsp start finished");
    //AudioUtilsStartLineIn();
}

static void audioStop() {
    AudioUtilsStopLineIn();
    sendCommand(RECORD_TYPE_PATH, "0");
    ALOGV("stop line in finished");
    AudioUtilsUninit();
    ALOGV("uninit audioutil finished");
    mAlsaUninit(0);
    ALOGV("stop audio track finished");
    // audiodsp_stop();
    ALOGV("audiodsp stop finished, sys.hdmiin.mute false");
    property_set(PROP_MUTE, "false");
}

static void setStateCB(int state) {
    displayState = state;
}

static int getState() {
    return displayState;
}

static jint displayHdmi(JNIEnv *env, jobject obj) {
    dispHdmi();
    videoEnable = 1;
    return 0;
}

static jint displayAndroid(JNIEnv *env, jobject obj) {
    videoEnable = 0;
    dispAndroid();
    return 0;
}

static void displayPip(JNIEnv *env, jobject obj, jint x, jint y, jint width, jint height) {
    dispPip( x, y, width, height);
    videoEnable = 1;
}

static jint getHActive(JNIEnv *env, jobject obj) {
    char fsBuf[PATH_MAX] = {0,};
    char value[128] = {0,};
    char buf[16] = {0,};
    int ret = 0;
    unsigned int i = 0;

    if (useSii9233a || useSii9293) {
        memset(value, 0, sizeof(value));
        memset(fsBuf, 0, sizeof(fsBuf));
        ret = readValue(getFs(paramPath, "input_mode", fsBuf), value);
        if (ret == -1)
            return ret;

        if (!strcmp(value, "invalid\n"))
            return -1;

        memset(buf, 0, sizeof(buf));
        ret = getInputInfo(value, DISPLAY_TYPE, buf);
        if (ret == -1)
            return ret;

        for (i = 0; i < OUTPUT_MODE_NUM; i++) {
            if (!strcmp(buf, output_mode_info[i].name)) {
                return output_mode_info[i].width;
            }
        }
    }

    memset(fsBuf, 0, sizeof(fsBuf));
    return readValue(getFs(paramPath, "horz_active", fsBuf));
}

static jstring getHdmiInSize(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    int ret = 0;

    checkSysfs();
    if (useSii9233a || useSii9293) {
        memset(value, 0, sizeof(value));
        memset(fsBuf, 0, sizeof(fsBuf));
        ret = readValue(getFs(paramPath, "input_mode", fsBuf), value);
        if (ret == -1)
            return NULL;

        if (!strcmp(value, "invalid\n"))
            return NULL;

        return env->NewStringUTF(value);
    }

    return NULL;
}

static jint getVActive(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    char buf[16] = {0,};
    int ret = 0;
    unsigned int i = 0;

    if (useSii9233a || useSii9293) {
        memset(value, 0, sizeof(value));
        memset(fsBuf, 0, sizeof(fsBuf));
        ret = readValue(getFs(paramPath, "input_mode", fsBuf), value);
        if (ret == -1)
            return ret;

        if (!strcmp(value, "invalid\n"))
            return -1;

        memset(buf, 0, sizeof(buf));
        ret = getInputInfo(value, DISPLAY_TYPE, buf);
        if (ret == -1)
            return ret;

        for (i = 0; i < OUTPUT_MODE_NUM; i++) {
            if (!strcmp(buf, output_mode_info[i].name)) {
                return output_mode_info[i].height;
            }
        }
    }

    memset(fsBuf, 0, sizeof(fsBuf));
    return readValue(getFs(paramPath, "vert_active", fsBuf));
}

static jboolean isDvi(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    char buf[16] = {0,};
    int ret = 0;

    if (useSii9233a || useSii9293) {
        memset(value, 0, sizeof(value));
        memset(fsBuf, 0, sizeof(fsBuf));
        ret = readValue(getFs(paramPath, "input_mode", fsBuf), value);
        if (ret == -1)
            return JNI_FALSE;

        if (!strcmp(value, "invalid\n"))
            return JNI_FALSE;

        memset(buf, 0, sizeof(buf));
        ret = getInputInfo(value, HDMI_TYPE, buf);
        if (ret == -1)
            return JNI_FALSE;

        return (strcmp(buf, "DVI") == 0);
    }

    memset(fsBuf, 0, sizeof(fsBuf));
    return (readValue(getFs(paramPath, "is_hdmi_mode", fsBuf)) == 0);
}

static jboolean isPowerOn(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    int ret = 0;

    checkSysfs();
    if (useSii9293) {
        memset(fsBuf, 0, sizeof(fsBuf));
        if (readValue(getFs(classPath, "enable", fsBuf), 22) == 1)
            return JNI_TRUE;

        return JNI_FALSE;
    } else if (useSii9233a) {
        memset(fsBuf, 0, sizeof(fsBuf));
        return (readValue(getFs(classPath, "port", fsBuf), 30) == 1);
    }

    memset(fsBuf, 0, sizeof(fsBuf));
    return (readValue(getFs(classPath, "poweron", fsBuf)) == 1);
}

static jboolean isEnable(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    int ret = 0;

    checkSysfs();
    if (useSii9233a || useSii9293) {
        if (useSii9293) {
            memset(fsBuf, 0, sizeof(fsBuf));
            if (readValue(getFs(classPath, "enable", fsBuf), 22) == 1)
                return JNI_TRUE;
        } else if (useSii9233a) {
            memset(fsBuf, 0, sizeof(fsBuf));
            if (readValue(getFs(classPath, "enable", fsBuf), 23) == 1)
                return JNI_TRUE;
        }

        memset(fsBuf, 0, sizeof(fsBuf));
        memset(value, 0, sizeof(value));
        ret = readValue(getFs(paramPath, "input_mode", fsBuf), value);
        if (ret == -1)
            return JNI_FALSE;

        return JNI_TRUE;
    }

    memset(fsBuf, 0, sizeof(fsBuf));
    return (readValue(getFs(classPath, "enable", fsBuf))==1);
}


static jboolean isInterlace(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    char buf[16] = {0,};
    int ret = 0;
    unsigned int i = 0;

    if (useSii9233a || useSii9293) {
        memset(fsBuf, 0, sizeof(fsBuf));
        memset(value, 0, sizeof(value));
        ret = readValue(getFs(paramPath, "input_mode", fsBuf), value);
        if (ret == -1)
            return JNI_FALSE;

        if (!strcmp(value, "invalid\n"))
            return JNI_FALSE;

        memset(buf, 0, sizeof(buf));
        ret = getInputInfo(value, DISPLAY_TYPE, buf);
        if (ret == -1)
            return JNI_FALSE;

        for (i = 0; i < OUTPUT_MODE_NUM; i++) {
            if (!strcmp(buf, output_mode_info[i].name)) {
                return output_mode_info[i].iMode;
            }
        }
    }

    memset(buf, 0, sizeof(buf));
    return (readValue(getFs(paramPath, "is_interlace", fsBuf)) == 1);
}

static jboolean hdmiPlugged(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    int ret = 0;

    checkSysfs();
    if (useSii9233a || useSii9293) {
        memset(value, 0, sizeof(value));
        memset(fsBuf, 0, sizeof(fsBuf));
        ret = readValue(getFs(paramPath, "cable_status", fsBuf), value);
        if (ret == -1)
            return JNI_FALSE;
        if ('1' == value[0])
            return JNI_TRUE;
        return JNI_FALSE;
    }

    return JNI_FALSE;
}

static jboolean hdmiSignal(JNIEnv *env, jobject obj) {
    char value[128] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    int ret = 0;

    checkSysfs();
    if (useSii9233a || useSii9293) {
        memset(value, 0, sizeof(value));
        memset(fsBuf, 0, sizeof(fsBuf));
        ret = readValue(getFs(paramPath, "signal_status", fsBuf), value);
        if (ret == -1)
            return JNI_FALSE;
        if ('1' == value[0])
            return JNI_TRUE;
        return JNI_FALSE;
    }

    return JNI_FALSE;
}

static void enableAudio(JNIEnv *env, jobject obj, jint flag) {
    memset(audioRate, 0, sizeof(audioRate));
    if (flag == 1) {
        audioEnable = 1;
    } else {
        audioEnable = 0;
    }
}

static jint handleAudio(JNIEnv *env, jobject obj) {
    char value[32] = {0,};
    char fsBuf[PATH_MAX] = {0,};
    int ret = 0;
    bool rateChanged = false;

    audioReady = 0;
    memset(value, 0, sizeof(value));
    memset(fsBuf, 0, sizeof(fsBuf));
    ret = readValue(getFs(paramPath, "audio_sample_rate", fsBuf), value);
    if (ret == -1)
        audioReady = 0;
    else if (strstr(value, "kHz") != NULL)
        audioReady = 1;
    if (ret != -1 && strcmp(audioRate, value)) {
        ALOGV("audio_sample_rate: %s", value);
        rateChanged = true;
        strcpy(audioRate, value);
        double val = atof(audioRate);
        val *= 1000;
        rate = (int)val;
        if (rate == 0) {
            ALOGV("sys.hdmiin.mute true");
            property_set("sys.hdmiin.mute", "true");
        } else {
            ALOGV("sys.hdmiin.mute false");
            property_set("sys.hdmiin.mute", "false");
        }
    }

    if (audioReady != 0 && videoEnable && audioEnable) {
        if (audioState == 0) {
            audioStart(rate);
            audioState = 1;
        } else if (audioState == 1 && rateChanged) {
            audioStart(rate);
        }

        if (audioState == 1) {
            AudioUtilsStartLineIn();
            if (!useSii9233a && !useSii9293) {
                memset(fsBuf, 0, sizeof(fsBuf));
                sendCommand(getFs(classPath, "enable", fsBuf) , "257"); //0x101: bit16, event of "mailbox_send_audiodsp"
            }
            audioState = 2;
        }
    } else if (!audioEnable) {
        if (audioState != 0) {
            audioStop();
            audioState = 0;
        }
    }
    return audioReady;
}

static void setEnable(JNIEnv *env, jobject obj, jboolean enable) {
    char fsBuf[PATH_MAX] = {0,};
    if (enable) {
        if (!isDisplayFullscreen || !useVideoLayer) {
            getScreenDev();
            if (screenDev != NULL) {
                screenDev->ops.set_source_type(screenDev, HDMI_IN);
            } else
                ALOGE("screenDev == NULL");
        }

        if (useSii9233a) {
            char port[8] = {0,};
            memset(port, 0, sizeof(port));
            sprintf(port, "%d\n", inputSource);
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "port", fsBuf), port);
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "enable", fsBuf), "1\n");
        } else if (useSii9293) {
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "enable", fsBuf) , "1\n");
        } else {
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "poweron", fsBuf) , "1");
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "enable", fsBuf)  , "1"); // disable "hdmi in"
        }
    } else {
        if (useSii9233a || useSii9293) {
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "enable", fsBuf)  , "0\n"); // disable "hdmi in"
        } else {
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "enable", fsBuf)  , "0"); // disable "hdmi in"
            memset(fsBuf, 0, sizeof(fsBuf));
            sendCommand(getFs(classPath, "poweron", fsBuf), "0"); //power off "hdmi in"
        }
    }
}

static jint setSourceType(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer)
        return -1;

    if (screenDev != NULL)
        screenDev->ops.set_source_type(screenDev, HDMI_IN);
    return 0;
}

static jboolean isSurfaceAvailable(JNIEnv *env, jobject obj, jobject jsurface) {
    if (jsurface) {
        sp<Surface> surface(android_view_Surface_getSurface(env, jsurface));
        if (surface == NULL) {
            ALOGE("isSurfaceAvailable, surface == NULL");
            return JNI_FALSE;
        }
        return JNI_TRUE;
    }

    ALOGE("isSurfaceAvailable, surface == NULL");
    return JNI_FALSE;
}

static void displayOSD(JNIEnv *env, jobject obj, jint width, jint height) {
    displayWidth = width;
    displayHeight = height;
    videoEnable = 1;
}

static jboolean setPreviewWindow(JNIEnv *env, jobject obj, jobject jsurface) {
    if (isDisplayFullscreen && useVideoLayer)
        return JNI_FALSE;

    sp<IGraphicBufferProducer> gbp;
    if(jsurface) {
        sp<Surface> surface(android_view_Surface_getSurface(env, jsurface));
        if (surface == NULL) {
            ALOGE("setPreviewWindow, surface == NULL");
            return JNI_FALSE;
        }
        gbp = surface->getIGraphicBufferProducer();
        if(gbp != NULL)
            window = new Surface(gbp);
    }

    if(window != NULL) {
        window ->incStrong((void*)ANativeWindow_acquire);
        getScreenDev();
        if(screenDev != NULL) {
            screenDev->ops.setPreviewWindow(screenDev, window.get());
            ALOGV("setPreviewWindow, width: %d, height: %d\n", displayWidth, displayHeight);
            screenDev->ops.set_format(screenDev, displayWidth, displayHeight, V4L2_PIX_FMT_NV21);
            screenDev->ops.setStateCallBack(screenDev, setStateCB);
        } else {
            ALOGE("AML_SCREEN_HARDWARE_MODULE is busy!!");
            return JNI_FALSE;
        }
    }

    return JNI_TRUE;
}

static jint setCrop(JNIEnv *env, jobject obj, jint x, jint y, jint width, jint height) {
    if (isDisplayFullscreen && useVideoLayer)
        return -1;

    ALOGV("setCrop, x: %d, y: %d, width: %d, height: %d", x, y, width, height);
    if (screenDev != NULL)
        screenDev->ops.set_crop(screenDev, x, y, width, height);
    return 0;
}

static void startMov(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer)
        return;

    if(screenDev != NULL)
        screenDev->ops.start(screenDev);
}

static void stopMov(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer)
        return;

    int state = getState();
    ALOGV("stopMov, state:%d", state);
    if(screenDev != NULL && state != STOP) {
        screenDev->ops.stop(screenDev);
        screenDev->common.close((struct hw_device_t *)screenDev);
        screenDev = NULL;
        screenModule = NULL;

        if (window != NULL)
            window ->decStrong((void*)ANativeWindow_acquire);
    }
}

static void pauseMov(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer)
        return;

    if(screenDev != NULL) {
        // screenDev->ops.pause(screenDev);
    }
}

static void resumeMov(JNIEnv *env, jobject obj) {
    if (isDisplayFullscreen && useVideoLayer)
        return;

    if(screenDev != NULL) {
        // screenDev->ops.resume(screenDev);
    }
}

static JNINativeMethod sMethods[] =
{
    {"_init", "(IZ)V", (void*)init},
    {"_deinit", "()V", (void*)deinit},
    {"_setMainWindowFull", "()V", (void*)setMainWindowFull},
    {"_setMainWindowPosition", "(II)V", (void*)setMainWindowPosition},
    {"_startVideo", "()V", (void*)startVideo},
    {"_stopVideo", "()V", (void*)stopVideo},
    {"_displayHdmi", "()I", (void*)displayHdmi},
    {"_displayAndroid", "()I", (void*)displayAndroid},
    {"_displayPip", "(IIII)V", (void*)displayPip},
    {"_getHActive", "()I", (void*)getHActive},
    {"_getHdmiInSize", "()Ljava/lang/String;", (void*)getHdmiInSize},
    {"_getVActive", "()I", (void*)getVActive},
    {"_isDvi", "()Z", (void*)isDvi},
    {"_isPowerOn", "()Z", (void*)isPowerOn},
    {"_isEnable", "()Z", (void*)isEnable},
    {"_isInterlace", "()Z", (void*)isInterlace},
    {"_hdmiPlugged", "()Z", (void*)hdmiPlugged},
    {"_hdmiSignal", "()Z", (void*)hdmiSignal},
    {"_enableAudio", "(I)V", (void*)enableAudio},
    {"_handleAudio", "()I", (void*)handleAudio},
    {"_setEnable", "(Z)V", (void*)setEnable},
    {"_setSourceType", "()I", (void*)setSourceType},
    {"_isSurfaceAvailable", "(Landroid/view/Surface;)Z", (void*)isSurfaceAvailable},
    {"_displayOSD", "(II)V", (void*)displayOSD},
    {"_setPreviewWindow", "(Landroid/view/Surface;)Z", (void*)setPreviewWindow},
    {"_setCrop", "(IIII)I", (void*)setCrop},
    {"_startMov", "()V", (void*)startMov},
    {"_stopMov", "()V", (void*)stopMov},
    {"_pauseMov", "()V", (void*)pauseMov},
    {"_resumeMov", "()V", (void*)resumeMov},
};

int register_android_server_HDMIIN(JNIEnv* env)
{
    jclass clazz;
    const char *kClassPathName = "com/droidlogic/app/HdmiInManager";

    clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'",
                kClassPathName);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, sMethods, NELEM(sMethods)) < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

}  // end namespace android
