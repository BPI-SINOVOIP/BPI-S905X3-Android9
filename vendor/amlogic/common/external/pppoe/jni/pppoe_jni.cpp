/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

#define LOG_NDEBUG 0
#define LOCAL_TAG "PPPOE_JNI"


/*
dalvik/libnativehelper/include/nativehelper/jni.h
*/
#include <jni.h>

#include <JNIHelp.h>

#include <android_runtime/AndroidRuntime.h>

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/un.h>
#include "netwrapper.h"
#include "pppoe_status.h"
#include <android/log.h>

#include "cutils/properties.h"

#define PPPOE_PLUGIN_CMD_LEN_MAX 256
#define PPPOE_CONNECT_CMD_LEN_MAX 512

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define TRACE() LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)
using namespace android;


struct fields_t {
    JavaVM      *gJavaVM ;
    JNIEnv* env;
    jmethodID   post_event;
};

static struct fields_t fields;

extern int get_eth0_updown();
extern int get_pppoe_status( const char *phy_if_name);

static char pppoe_connect_cmd[PPPOE_CONNECT_CMD_LEN_MAX];

static char pppoe_disconnect_cmd[512] = "ppp-stop";
static char pppoe_terminate_cmd[512] = "ppp-terminate";

static char pppoe_plugin_cmd[PPPOE_PLUGIN_CMD_LEN_MAX];

#define PPPD_OPTIONS_LEN 512
static char pppd_options[PPPD_OPTIONS_LEN + 1] = {"debug logfd 1 noipdefault noauth default-asyncmap defaultroute show-password nodetach mtu 1492 mru 1492 noaccomp nodeflate nopcomp novj usepeerdns novjccomp lcp-echo-interval 20 lcp-echo-failure 3"};



static char* create_pppoe_plugin_cmd(char *pid_file, char *phy_if)
{
    int len;
    len = snprintf(pppoe_plugin_cmd, PPPOE_PLUGIN_CMD_LEN_MAX, 
                   "'pppoe -p %s -I %s -T 80 -U -m 1412'", 
                   pid_file, phy_if);
    if ( len < 0 || len >= PPPOE_PLUGIN_CMD_LEN_MAX ) {
        return NULL;
    }

    return pppoe_plugin_cmd;
}


static char* create_pppoe_connect_cmd
(char *plugin_cmd, char *options, char *username, char *pwd)
{
    int len;
    len = snprintf(pppoe_connect_cmd, PPPOE_CONNECT_CMD_LEN_MAX, 
                   "pppd pty %s %s user %s password %s &", 
                   plugin_cmd, options, username, pwd);
    if ( len < 0 || len >= PPPOE_CONNECT_CMD_LEN_MAX ) {
        return NULL;
    }

    return pppoe_connect_cmd;
}

//#ifndef _ROOT_PATH
#define _ROOT_PATH "/data/misc"
#define CONFIG_FILE  _ROOT_PATH "/etc/ppp/pppd_options.conf"



/* Handy routine to read very long lines in text files.
 * This means we read the whole line and avoid any nasty buffer overflows. */
static ssize_t get_line(char **line, size_t *len, FILE *fp)
{
	char *p;
	size_t last = 0;

	while(!feof(fp)) {
		if (*line == NULL || last != 0) {
			*len += BUFSIZ;
			*line = (char*)realloc(*line, *len);
		}
		p = *line + last;
		memset(p, 0, BUFSIZ);
		if (fgets(p, BUFSIZ, fp) == NULL)
			break;
		last += strlen(p);
		if (last && (*line)[last - 1] == '\n') {
			(*line)[last - 1] = '\0';
			break;
		}
	}
	return last;
}

static const char PPPOE_PHY_IF_PROP_NAME[]    = "net.pppoe.phyif";

static jboolean com_amlogic_PppoeOperation_connect
(JNIEnv *env, jobject obj, jstring jstr_if_name, jstring jstr_account, jstring jstr_passwd)
{
    char *p_ifname;
    char *p_user; 
    char *p_passwd;
    struct netwrapper_ctrl * ctrl;
    char *p;

	FILE *f;
	f = fopen(CONFIG_FILE, "r");
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,"Try Open %s", CONFIG_FILE);

	if (f) {
    	char *line, *option, *p, *buffer = NULL;
    	size_t len = 0;

        get_line(&buffer, &len, f);
        __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,"get_line: [%s]", buffer);
        if (buffer){
            strncpy(pppd_options, buffer, sizeof(pppd_options) - 1);
            free(buffer);
        }
        fclose(f);
    }
    
    p_ifname = (char *)env->GetStringUTFChars(jstr_if_name, NULL);
    p_user = (char *)env->GetStringUTFChars(jstr_account, NULL);
    p_passwd = (char *)env->GetStringUTFChars(jstr_passwd, NULL);

    property_set(PPPOE_PHY_IF_PROP_NAME, p_ifname);

    p = create_pppoe_plugin_cmd((char*)PPPOE_PIDFILE, p_ifname);
    if (!p) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"failed to create plug_in command\n");
        return -1;
    }
    
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,"plug_in command: %s\n", p);

    p = create_pppoe_connect_cmd(p, pppd_options, p_user, p_passwd);
    if (!p) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"failed to create connect command\n");
        return -1;
    }
    
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,"connect command: %s\n", p);


    __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"ppp.connect\n");

    ctrl = netwrapper_ctrl_open(PPPOE_WRAPPER_CLIENT_PATH, PPPOE_WRAPPER_SERVER_PATH);
    if (ctrl == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG, "Failed to connect to pppd\n");
        return -1;
    }

    netwrapper_ctrl_request(ctrl, pppoe_connect_cmd, strlen(pppoe_connect_cmd));

    netwrapper_ctrl_close(ctrl);

    env->ReleaseStringUTFChars(jstr_if_name, p_ifname);
    env->ReleaseStringUTFChars(jstr_account, p_user);
    env->ReleaseStringUTFChars(jstr_passwd, p_passwd);

    return 1;
}



jboolean com_amlogic_PppoeOperation_disconnect
(JNIEnv *env, jobject obj)
{
    struct netwrapper_ctrl * ctrl;

    __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"ppp.disconnect\n");

    ctrl = netwrapper_ctrl_open(PPPOE_WRAPPER_CLIENT_PATH, PPPOE_WRAPPER_SERVER_PATH);
    if (ctrl == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG, "Failed to connect to pppd\n");
        return -1;
    }

    netwrapper_ctrl_request(ctrl, pppoe_disconnect_cmd, strlen(pppoe_disconnect_cmd));

    netwrapper_ctrl_close(ctrl);

    return 1;
}

jboolean com_amlogic_PppoeOperation_terminate
(JNIEnv *env, jobject obj)
{
    struct netwrapper_ctrl * ctrl;

    __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"ppp.terminate\n");

    ctrl = netwrapper_ctrl_open(PPPOE_WRAPPER_CLIENT_PATH, PPPOE_WRAPPER_SERVER_PATH);
    if (ctrl == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG, "Failed to connect to pppd\n");
        return -1;
    }

    netwrapper_ctrl_request(ctrl, pppoe_terminate_cmd, strlen(pppoe_terminate_cmd));

    netwrapper_ctrl_close(ctrl);

    return 1;
}

jint com_amlogic_PppoeOperation_isNetAdded
(JNIEnv *env, jobject obj, jstring jstr_if_name)
{
    char *p_ifname;
    int status;

    __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"ppp.isNetAdded\n");
    
    p_ifname = (char *)env->GetStringUTFChars(jstr_if_name, NULL);
    status = get_net_updown(p_ifname);

    env->ReleaseStringUTFChars(jstr_if_name, p_ifname);

    return status;
}

jint com_amlogic_PppoeOperation_status
(JNIEnv *env, jobject obj, jstring jstr_if_name)
{
    char *p_ifname;
    int status;

    __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,"ppp.status\n");

    p_ifname = (char *)env->GetStringUTFChars(jstr_if_name, NULL);
    status = get_pppoe_status(p_ifname);

    env->ReleaseStringUTFChars(jstr_if_name, p_ifname);

    return status;
}


static JNINativeMethod gPppoeJNIMethods[] = {
    /* name, signature, funcPtr */
    { "_connect",       "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z", (void*) com_amlogic_PppoeOperation_connect },
    { "_disconnect",    "()Z", (void*) com_amlogic_PppoeOperation_disconnect },
    { "_terminate",     "()Z", (void*) com_amlogic_PppoeOperation_terminate },
    { "isNetAdded",     "(Ljava/lang/String;)I", (void*) com_amlogic_PppoeOperation_isNetAdded },
    { "status",       "(Ljava/lang/String;)I", (void*) com_amlogic_PppoeOperation_status },
};


#define PPPOE_CLASS_NAME "com/amlogic/pppoe/PppoeOperation"


int register_pppoe_jni(JNIEnv* env)
{
    int ret;
    jclass pppoe = env->FindClass(PPPOE_CLASS_NAME);
    if (NULL == pppoe) {
        LOGI("%s:Unable to find class %s", __FUNCTION__, PPPOE_CLASS_NAME);
        return -1;
    }
    else {
        LOGI("%s:class %s FOUND", __FUNCTION__, PPPOE_CLASS_NAME);
    }
    
    TRACE();
    ret = android::AndroidRuntime::registerNativeMethods(env,
            PPPOE_CLASS_NAME, gPppoeJNIMethods, NELEM(gPppoeJNIMethods));
    return ret;
}


JNIEXPORT jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    jint ret;
    JNIEnv* env = NULL;
    TRACE();
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK || NULL == env) {
        LOGE("GetEnv failed!");
        return -1;
    }
    fields.gJavaVM = vm;
    fields.env = env;
    TRACE();
    ret = register_pppoe_jni(env);
    LOGI("register_pppoe_jni=%d\n", ret);
    if (ret == 0) {
        return JNI_VERSION_1_4;
    } else {
        return -1;
    }
}


JNIEXPORT void
JNI_OnUnload(JavaVM* vm, void* reserved)
{
    return;
}


