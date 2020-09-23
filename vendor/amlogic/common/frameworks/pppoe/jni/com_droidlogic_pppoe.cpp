/*
 * Copyright 2009, The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Yi Sun <beyounn@gmail.com>
 */

#define LOG_TAG "pppoe"

#include "jni.h"
#include <cutils/properties.h>
#include "com_droidlogic_common.h"
#include "com_droidlogic_common.cpp"

#include <jni.h>
#include <JNIHelp.h>

#include <utils/Log.h>
#include <gui/SurfaceComposerClient.h>


#define PPPOE_PKG_NAME "com/droidlogic/pppoe/PppoeNative"
#define MAX_FGETS_LEN 4

namespace android {
static struct fieldIds {
    jclass dhcpInfoClass;
    jmethodID constructorId;
    jfieldID ipaddress;
    jfieldID gateway;
    jfieldID netmask;
    jfieldID dns1;
    jfieldID dns2;
    jfieldID serverAddress;
    jfieldID leaseDuration;
} dhcpInfoFieldIds;


interface_info_t pppoe_if_list[NR_PPP_INTERFACES];

static int nr_pppoe_if = 0;
static int nl_socket_netlink_route = -1;
static int nl_socket_kobj_uevent = -1;

static int netlink_init_pppoe_list(void);


static int is_interested_event(char *ifname)
{
    char phy_if_name[PROP_VALUE_MAX] = {0};
    property_get("net.pppoe.phyif", phy_if_name, "UNKNOWN_PHYIF");
    return  (0 == strncmp("ppp", ifname, 3) || 0 == strcmp(phy_if_name, ifname));
}


static jstring com_droidlogic_pppoe_PppoeNative_waitForEvent
(JNIEnv *env, jobject clazz)
{
        char rbuf[RET_STR_SZ] = {0};
        int guard = 0;

        waitForNetInterfaceEvent(nl_socket_netlink_route, nl_socket_kobj_uevent,
        rbuf, &guard, netlink_init_pppoe_list, is_interested_event);

        return env->NewStringUTF(guard > 0 ? rbuf : NULL);
}


static int netlink_init_pppoe_list(void)
{
    int ret = -1;
    DIR  *netdir;
    struct dirent *de;
    char path[SYSFS_PATH_MAX];
    interface_info_t *intfinfo;
    int index;
    FILE *ifidx;
    char idx[MAX_FGETS_LEN+1];
    int i;

    for ( i = 0; i < NR_ETHER_INTERFACES; i++) {
        pppoe_if_list[i].if_index = -1;
        if (pppoe_if_list[i].name != NULL)
            free(pppoe_if_list[i].name);
        pppoe_if_list[i].name = NULL;
        nr_pppoe_if = 0;
    }

    if ((netdir = opendir(SYSFS_CLASS_NET)) != NULL) {
         while ((de = readdir(netdir)) != NULL) {
            if (strcmp(de->d_name,"ppp0"))
                continue;

            snprintf(path, SYSFS_PATH_MAX, "%s/%s/type", SYSFS_CLASS_NET, de->d_name);
            FILE *typefd;
            if ((typefd = fopen(path, "r")) != NULL) {
                char typestr[MAX_FGETS_LEN + 1];
                int type = 0;
                memset(typestr, 0, MAX_FGETS_LEN + 1);
                if (fgets(typestr, MAX_FGETS_LEN, typefd) != NULL) {
                    type = strtoimax(typestr, NULL, 10);
                }
                fclose(typefd);
                if (type >= ARPHRD_TUNNEL && type < ARPHRD_IEEE802_TR)
                    continue;
            } else {
                ALOGE("Can not open %s for read",path);
                continue;
            }

            snprintf(path, SYSFS_PATH_MAX,"%s/%s/ifindex",SYSFS_CLASS_NET,de->d_name);
            if ((ifidx = fopen(path,"r")) != NULL ) {
                memset(idx,0,MAX_FGETS_LEN+1);
                if (fgets(idx,MAX_FGETS_LEN,ifidx) != NULL) {
                    index = strtoimax(idx,NULL,10);
                } else {
                    ALOGE("Can not read %s",path);
                    fclose(ifidx);
                    continue;
                }
                fclose(ifidx);
            } else {
                ALOGE("Can not open %s for read",path);
                continue;
            }

            if (0 == add_interface_node_to_list(pppoe_if_list,NR_PPP_INTERFACES,index, (char *) de->d_name)) {
                nr_pppoe_if++;
            }
        }
        closedir(netdir);
        ret = 0;
    }
    return ret;
}


static jint com_droidlogic_pppoe_PppoeNative_initPppoeNative
(JNIEnv *env, jobject clazz)
{
    int ret = -1;

    ALOGI("==>%s",__FUNCTION__);

    nl_socket_netlink_route = open_NETLINK_socket(NETLINK_ROUTE, RTMGRP_LINK | RTMGRP_IPV4_IFADDR);
    if (nl_socket_netlink_route < 0) {
        ALOGE("failed to create NETLINK socket");
        goto error;
    }

    nl_socket_kobj_uevent = open_NETLINK_socket(NETLINK_KOBJECT_UEVENT, 0xFFFFFFFF);
    if (nl_socket_kobj_uevent < 0) {
        ALOGE("failed to create NETLINK socket");
        goto error;
    }

    if ((ret = netlink_init_pppoe_list()) < 0) {
        ALOGE("Can not collect the interface list");
        goto error;
    }
    return ret;
error:
    ALOGE("%s exited with error",__FUNCTION__);
    if (nl_socket_netlink_route >= 0)
        close(nl_socket_netlink_route);
    if (nl_socket_kobj_uevent >= 0)
        close(nl_socket_kobj_uevent);
    return ret;
}


static jstring com_droidlogic_pppoe_PppoeNative_getInterfaceName
(JNIEnv *env, jobject clazz, jint index)
{
    int ret;
    char ifname[IFNAMSIZ+1] = {0};
    ALOGI("User ask for device name on %d, total:%d",index, nr_pppoe_if);

    if (nr_pppoe_if == 0 || index >= nr_pppoe_if) {
        ALOGI("Invalid parameters");
        return env->NewStringUTF(NULL);
    }

    ret = getInterfaceName(pppoe_if_list, NR_PPP_INTERFACES, index, ifname);
    if (ret != 0) {
        ALOGI("No device name found");
    }

    return env->NewStringUTF(ret == 0 ? ifname:NULL);
}


static jint com_droidlogic_pppoe_PppoeNative_getInterfaceCnt() {
    return nr_pppoe_if;
}

static jint com_droidlogic_pppoe_PppoeNative_isInterfaceAdded
(JNIEnv *env, jobject clazz, jstring ifname)
{
    int retval = 0;
    const char * ppp_name = env->GetStringUTFChars(ifname, NULL);
    if (ppp_name == NULL) {
        ALOGE("Device name NULL!");
        return 0;
    }
    while (true) {
        ALOGE("android_net_pppoe_isInterfaceAdded undefined!");
    }

    env->ReleaseStringUTFChars(ifname, ppp_name);
    return retval;
}

static JNINativeMethod gPppoeMethods[] = {
    {"waitForEvent", "()Ljava/lang/String;",
     (void *)com_droidlogic_pppoe_PppoeNative_waitForEvent},
    {"getInterfaceName", "(I)Ljava/lang/String;",
     (void *)com_droidlogic_pppoe_PppoeNative_getInterfaceName},
    {"initPppoeNative", "()I",
     (void *)com_droidlogic_pppoe_PppoeNative_initPppoeNative},
    {"getInterfaceCnt","()I",
     (void *)com_droidlogic_pppoe_PppoeNative_getInterfaceCnt},
    {"isInterfaceAdded","(Ljava/lang/String;)I",
     (void *)com_droidlogic_pppoe_PppoeNative_isInterfaceAdded}
};




int register_com_droidlogic_pppoe_PppoeManager(JNIEnv* env)
{
    jclass pppoe = env->FindClass(PPPOE_PKG_NAME);
    ALOGI("Loading pppoe jni class");
    LOG_FATAL_IF( pppoe == NULL, "Unable to find class " PPPOE_PKG_NAME);

    dhcpInfoFieldIds.dhcpInfoClass =
        env->FindClass("android/net/DhcpInfo");
    if (dhcpInfoFieldIds.dhcpInfoClass != NULL) {
        dhcpInfoFieldIds.constructorId =
            env->GetMethodID(dhcpInfoFieldIds.dhcpInfoClass,
                             "<init>", "()V");
        dhcpInfoFieldIds.ipaddress =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                            "ipAddress", "I");
        dhcpInfoFieldIds.gateway =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                            "gateway", "I");
        dhcpInfoFieldIds.netmask =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                            "netmask", "I");
        dhcpInfoFieldIds.dns1 =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass, "dns1", "I");
        dhcpInfoFieldIds.dns2 =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass, "dns2", "I");
        dhcpInfoFieldIds.serverAddress =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                            "serverAddress", "I");
        dhcpInfoFieldIds.leaseDuration =
            env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                            "leaseDuration", "I");
    }

  //  return AndroidRuntime::registerNativeMethods(env,
  //                                               PPPOE_PKG_NAME,
  //                                               gPppoeMethods,
  //                                               NELEM(gPppoeMethods));
	return jniRegisterNativeMethods(env, PPPOE_PKG_NAME,
                                gPppoeMethods, NELEM(gPppoeMethods));
}

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved __unused)
{

    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_com_droidlogic_pppoe_PppoeManager(env) < 0) {
        ALOGE("ERROR: PppoeManager native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

};
