/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Contains implementation of a class EmulatedCameraFactory that manages cameras
 * available for emulation.
 */

//#define LOG_NDEBUG 0
//#define LOG_NDDEBUG 0
//#define LOG_NIDEBUG 0
#define LOG_TAG "EmulatedCamera_Factory"
#include <android/log.h>
#include <cutils/properties.h>
#include "EmulatedQemuCamera.h"
#include "EmulatedFakeCamera.h"
#include "EmulatedFakeCamera2.h"
#include "EmulatedFakeCamera3.h"
#include "EmulatedCameraHotplugThread.h"
#include "EmulatedCameraFactory.h"

extern camera_module_t HAL_MODULE_INFO_SYM;
volatile int32_t gCamHal_LogLevel = 4;

/* A global instance of EmulatedCameraFactory is statically instantiated and
 * initialized when camera emulation HAL is loaded.
 */
android::EmulatedCameraFactory  gEmulatedCameraFactory;
default_camera_hal::VendorTags gVendorTags;

static const char *USB_SENSOR_PATH[]={
    "/dev/video0",
    "/dev/video1",
    "/dev/video2",
    "/dev/video3",
    "/dev/video4",
    "/dev/video5",
};

static const char *BOARD_SENSOR_PATH[]={
    "/dev/video50",
};

int updateLogLevels()
{
    char levels_value[92];
    int tmp = 4;
    if (property_get("camera.log_levels", levels_value, NULL) > 0)
        sscanf(levels_value, "%d", &tmp);
    else
        ALOGD("Can not read property camera.log_levels, using defalut value\n");
    gCamHal_LogLevel = tmp;
    return tmp;
}

static  int getCameraNum() {
    int iCamerasNum = 0;
    char property[PROPERTY_VALUE_MAX];
    property_get("ro.vendor.platform.board_camera", property, "false");
    if (strstr(property, "true")) {
        for (int i = 0; i < (int)ARRAY_SIZE(BOARD_SENSOR_PATH); i++ ) {
            //int camera_fd;
            CAMHAL_LOGDB("try access %s\n", BOARD_SENSOR_PATH[i]);
            if (0 == access(BOARD_SENSOR_PATH[i], F_OK | R_OK | W_OK)) {
                CAMHAL_LOGDB("access %s success\n", BOARD_SENSOR_PATH[i]);
                iCamerasNum++;
            }
        }
    } else {
        for (int i = 0; i < (int)ARRAY_SIZE(USB_SENSOR_PATH); i++ ) {
            //int camera_fd;
            CAMHAL_LOGDB("try access %s\n", USB_SENSOR_PATH[i]);
            if (0 == access(USB_SENSOR_PATH[i], F_OK | R_OK | W_OK)) {
                CAMHAL_LOGDB("access %s success\n", USB_SENSOR_PATH[i]);
                iCamerasNum++;
            }
        }
    }

    return iCamerasNum;
}
namespace android {

EmulatedCameraFactory::EmulatedCameraFactory()
        : mQemuClient(),
          mEmulatedCameraNum(0),
          mConstructedOK(false),
          mCallbacks(NULL)
{
    status_t res;
    /* Connect to the factory service in the emulator, and create Qemu cameras. */
    int cameraId = 0;

    memset(mEmulatedCameras, 0,(MAX_CAMERA_NUM) * sizeof(EmulatedBaseCamera*));
    mEmulatedCameraNum = getCameraNum();
    CAMHAL_LOGDB("Camera num = %d", mEmulatedCameraNum);

    for( int i = 0; i < mEmulatedCameraNum; i++ ) {
        cameraId = i;
        mEmulatedCameras[i] = new EmulatedFakeCamera3(cameraId, &HAL_MODULE_INFO_SYM.common);
        if (mEmulatedCameras[i] != NULL) {
            ALOGV("%s: camera device version is %d", __FUNCTION__,
                    getFakeCameraHalVersion(cameraId));
            res = mEmulatedCameras[i]->Initialize();
            if (res != NO_ERROR) {
                ALOGE("%s: Unable to intialize camera %d: %s (%d)",
                    __FUNCTION__, i, strerror(-res), res);
                delete mEmulatedCameras[i];
            }
        }
    }

    CAMHAL_LOGDB("%d cameras are being created",
          mEmulatedCameraNum);

    /* Create hotplug thread */
    {
        Vector<int> cameraIdVector;
        for (int i = 0; i < mEmulatedCameraNum; ++i) {
            cameraIdVector.push_back(i);
        }
        mHotplugThread = new EmulatedCameraHotplugThread(&cameraIdVector[0],
                                                         mEmulatedCameraNum);
        mHotplugThread->run("");
    }

    mConstructedOK = true;
}

EmulatedCameraFactory::~EmulatedCameraFactory()
{
    CAMHAL_LOGDA("Camera Factory deconstruct the BaseCamera\n");
    for (int n = 0; n < mEmulatedCameraNum; n++) {
        if (mEmulatedCameras[n] != NULL) {
            delete mEmulatedCameras[n];
        }
    }

    if (mHotplugThread != NULL) {
        mHotplugThread->requestExit();
        mHotplugThread->join();
    }
}

int EmulatedCameraFactory::getValidCameraId() {
    int iValidId = 0;
    char property[PROPERTY_VALUE_MAX];
    property_get("ro.vendor.platform.board_camera", property, "false");

    for (int i = 0; i < MAX_CAMERA_NUM; i++ ) {
        if (strstr(property, "true")) {
            if (0 == access(BOARD_SENSOR_PATH[i], F_OK | R_OK | W_OK)) {
                iValidId = i;
                break;
            }
        } else {
            if (0 == access(USB_SENSOR_PATH[i], F_OK | R_OK | W_OK)) {
                iValidId = i;
                break;
            }
        }
    }
    return iValidId;
}

/****************************************************************************
 * Camera HAL API handlers.
 *
 * Each handler simply verifies existence of an appropriate EmulatedBaseCamera
 * instance, and dispatches the call to that instance.
 *
 ***************************************************************************/

int EmulatedCameraFactory::cameraDeviceOpen(int camera_id, hw_device_t** device)
{
    ALOGV("%s: id = %d", __FUNCTION__, camera_id);
    int valid_id;
    *device = NULL;

    updateLogLevels();

    if (!isConstructedOK()) {
        ALOGE("%s: EmulatedCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getEmulatedCameraNum()) {
        ALOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getEmulatedCameraNum());
        return -ENODEV;
    }
    valid_id = getValidCameraId();
    //return mEmulatedCameras[camera_id]->connectCamera(device);
    return mEmulatedCameras[valid_id]->connectCamera(device);
}

int EmulatedCameraFactory::getCameraInfo(int camera_id, struct camera_info* info)
{
    ALOGV("%s: id = %d", __FUNCTION__, camera_id);
    int valid_id;
    if (!isConstructedOK()) {
        ALOGE("%s: EmulatedCameraFactory has failed to initialize", __FUNCTION__);
        return -EINVAL;
    }

    if (camera_id < 0 || camera_id >= getEmulatedCameraNum()) {
        ALOGE("%s: Camera id %d is out of bounds (%d)",
             __FUNCTION__, camera_id, getEmulatedCameraNum());
        return -ENODEV;
    }
    valid_id = getValidCameraId();
    //return mEmulatedCameras[camera_id]->getCameraInfo(info);
    return mEmulatedCameras[valid_id]->getCameraInfo(info);
}

int EmulatedCameraFactory::setCallbacks(
        const camera_module_callbacks_t *callbacks)
{
    ALOGV("%s: callbacks = %p", __FUNCTION__, callbacks);

    mCallbacks = callbacks;

    return OK;
}

static int get_tag_count(const vendor_tag_ops_t* ops)
{
    return gVendorTags.getTagCount(ops);
}
static void get_all_tags(const vendor_tag_ops_t* ops, uint32_t* tag_array)
{
    gVendorTags.getAllTags(ops, tag_array);
}
static const char* get_section_name(const vendor_tag_ops_t* ops, uint32_t tag)
{
    return gVendorTags.getSectionName(ops, tag);
}
static const char* get_tag_name(const vendor_tag_ops_t* ops, uint32_t tag)
{
    return gVendorTags.getTagName(ops, tag);
}
static int get_tag_type(const vendor_tag_ops_t* ops, uint32_t tag)
{
    return gVendorTags.getTagType(ops, tag);
}
void EmulatedCameraFactory::getvendortagops(vendor_tag_ops_t* ops)
{
    ALOGV("%s : ops=%p", __func__, ops);
    ops->get_tag_count      = get_tag_count;
    ops->get_all_tags       = get_all_tags;
    ops->get_section_name   = get_section_name;
    ops->get_tag_name       = get_tag_name;
    ops->get_tag_type       = get_tag_type;
}
/****************************************************************************
 * Camera HAL API callbacks.
 ***************************************************************************/

EmulatedBaseCamera* EmulatedCameraFactory::getValidCameraOject()
{
    EmulatedBaseCamera* cam = NULL;
    for (int i = 0; i < MAX_CAMERA_NUM; i++) {
        if (mEmulatedCameras[i] != NULL) {
            cam =  mEmulatedCameras[i];
            break;
        }
    }
    return cam;
}

int EmulatedCameraFactory::getValidCameraOjectId()
{
    int j =0;
    for (int i = 0; i < MAX_CAMERA_NUM; i++) {
        if (mEmulatedCameras[i] != NULL) {
            j = i;
            break;
        }
    }
    return j;
}

int EmulatedCameraFactory::device_open(const hw_module_t* module,
                                       const char* name,
                                       hw_device_t** device)
{
    /*
     * Simply verify the parameters, and dispatch the call inside the
     * EmulatedCameraFactory instance.
     */

    if (module != &HAL_MODULE_INFO_SYM.common) {
        ALOGE("%s: Invalid module %p expected %p",
             __FUNCTION__, module, &HAL_MODULE_INFO_SYM.common);
        return -EINVAL;
    }
    if (name == NULL) {
        ALOGE("%s: NULL name is not expected here", __FUNCTION__);
        return -EINVAL;
    }

    return gEmulatedCameraFactory.cameraDeviceOpen(atoi(name), device);
}

int EmulatedCameraFactory::get_number_of_cameras(void)
{
    int i = 0;
    EmulatedBaseCamera* cam = gEmulatedCameraFactory.getValidCameraOject();
    while (i < 6) {
        if (cam != NULL) {
            if (!cam->getHotplugStatus()) {
                DBG_LOGA("here we wait usb camera plug");
                usleep(50000);
                i++;
            } else {
                break;
            }
        } else {
            DBG_LOGB("%s : cam is NULL", __FUNCTION__);
            break;
        }
    }
    return gEmulatedCameraFactory.getEmulatedCameraNum();
}

int EmulatedCameraFactory::get_camera_info(int camera_id,
                                           struct camera_info* info)
{
    return gEmulatedCameraFactory.getCameraInfo(camera_id, info);
}

int EmulatedCameraFactory::set_callbacks(
        const camera_module_callbacks_t *callbacks)
{
    return gEmulatedCameraFactory.setCallbacks(callbacks);
}

void EmulatedCameraFactory::get_vendor_tag_ops(vendor_tag_ops_t* ops)
{
	 gEmulatedCameraFactory.getvendortagops(ops);
}
/********************************************************************************
 * Internal API
 *******************************************************************************/

/*
 * Camera information tokens passed in response to the "list" factory query.
 */

/* Device name token. */
//static const char lListNameToken[]    = "name=";
/* Frame dimensions token. */
//static const char lListDimsToken[]    = "framedims=";
/* Facing direction token. */
//static const char lListDirToken[]     = "dir=";

void EmulatedCameraFactory::createQemuCameras()
{
#if 0
    /* Obtain camera list. */
    char* camera_list = NULL;
    status_t res = mQemuClient.listCameras(&camera_list);
    /* Empty list, or list containing just an EOL means that there were no
     * connected cameras found. */
    if (res != NO_ERROR || camera_list == NULL || *camera_list == '\0' ||
        *camera_list == '\n') {
        if (camera_list != NULL) {
            free(camera_list);
        }
        return;
    }

    /*
     * Calculate number of connected cameras. Number of EOLs in the camera list
     * is the number of the connected cameras.
     */

    int num = 0;
    const char* eol = strchr(camera_list, '\n');
    while (eol != NULL) {
        num++;
        eol = strchr(eol + 1, '\n');
    }

    /* Allocate the array for emulated camera instances. Note that we allocate
     * two more entries for back and front fake camera emulation. */
    mEmulatedCameras = new EmulatedBaseCamera*[num + 2];
    if (mEmulatedCameras == NULL) {
        ALOGE("%s: Unable to allocate emulated camera array for %d entries",
             __FUNCTION__, num + 1);
        free(camera_list);
        return;
    }
    memset(mEmulatedCameras, 0, sizeof(EmulatedBaseCamera*) * (num + 1));

    /*
     * Iterate the list, creating, and initializin emulated qemu cameras for each
     * entry (line) in the list.
     */

    int index = 0;
    char* cur_entry = camera_list;
    while (cur_entry != NULL && *cur_entry != '\0' && index < num) {
        /* Find the end of the current camera entry, and terminate it with zero
         * for simpler string manipulation. */
        char* next_entry = strchr(cur_entry, '\n');
        if (next_entry != NULL) {
            *next_entry = '\0';
            next_entry++;   // Start of the next entry.
        }

        /* Find 'name', 'framedims', and 'dir' tokens that are required here. */
        char* name_start = strstr(cur_entry, lListNameToken);
        char* dim_start = strstr(cur_entry, lListDimsToken);
        char* dir_start = strstr(cur_entry, lListDirToken);
        if (name_start != NULL && dim_start != NULL && dir_start != NULL) {
            /* Advance to the token values. */
            name_start += strlen(lListNameToken);
            dim_start += strlen(lListDimsToken);
            dir_start += strlen(lListDirToken);

            /* Terminate token values with zero. */
            char* s = strchr(name_start, ' ');
            if (s != NULL) {
                *s = '\0';
            }
            s = strchr(dim_start, ' ');
            if (s != NULL) {
                *s = '\0';
            }
            s = strchr(dir_start, ' ');
            if (s != NULL) {
                *s = '\0';
            }

            /* Create and initialize qemu camera. */
            EmulatedQemuCamera* qemu_cam =
                new EmulatedQemuCamera(index, &HAL_MODULE_INFO_SYM.common);
            if (NULL != qemu_cam) {
                res = qemu_cam->Initialize(name_start, dim_start, dir_start);
                if (res == NO_ERROR) {
                    mEmulatedCameras[index] = qemu_cam;
                    index++;
                } else {
                    delete qemu_cam;
                }
            } else {
                ALOGE("%s: Unable to instantiate EmulatedQemuCamera",
                     __FUNCTION__);
            }
        } else {
            ALOGW("%s: Bad camera information: %s", __FUNCTION__, cur_entry);
        }

        cur_entry = next_entry;
    }

    mEmulatedCameraNum = index;
#else
    CAMHAL_LOGDA("delete this function");
#endif
}

bool EmulatedCameraFactory::isFakeCameraFacingBack(int cameraId)
{
    if (cameraId%mEmulatedCameraNum == 1)
        return false;

    return true;
}

int EmulatedCameraFactory::getFakeCameraHalVersion(int cameraId __unused)
{
    /* Defined by 'qemu.sf.back_camera_hal_version' boot property: if the
     * property doesn't exist, it is assumed to be 1. */
#if 0
    char prop[PROPERTY_VALUE_MAX];
    if (property_get("qemu.sf.back_camera_hal", prop, NULL) > 0) {
        char *prop_end = prop;
        int val = strtol(prop, &prop_end, 10);
        if (*prop_end == '\0') {
            return val;
        }
        // Badly formatted property, should just be a number
        ALOGE("qemu.sf.back_camera_hal is not a number: %s", prop);
    }
    return 1;
#else
    return 3;
#endif
}

void EmulatedCameraFactory::onStatusChanged(int cameraId, int newStatus)
{
    status_t res;
    char dev_name[128];
    int i = 0 , j = 0;
    int m = 0, n = 0;
    int k = 0;
    //EmulatedBaseCamera *cam = mEmulatedCameras[cameraId];
    const camera_module_callbacks_t* cb = mCallbacks;
    sprintf(dev_name, "%s%d", "/dev/video", cameraId);

    /* ignore cameraid >= MAX_CAMERA_NUM to avoid overflow, we now have
     * ion device with device like /dev/video13
     */
    if (cameraId >= MAX_CAMERA_NUM)
        return;

    CAMHAL_LOGDB("mEmulatedCameraNum =%d\n", mEmulatedCameraNum);
    n = getValidCameraOjectId();
    if ((n != cameraId) && (mEmulatedCameras[n] != NULL)) {
        DBG_LOGA("device node changed");
        mEmulatedCameras[n]->unplugCamera();
        delete mEmulatedCameras[n];
        mEmulatedCameras[n] = NULL;
    }

    if (mEmulatedCameras[cameraId] != NULL && (!mEmulatedCameras[cameraId]->getHotplugStatus())) {
        DBG_LOGA("close EmulatedFakeCamera3 object for the last time");
        while (k < 150) {
            if (!(mEmulatedCameras[cameraId]->getCameraStatus())) {
                usleep(10000);
                k++;
            } else {
                break;
            }
        }
        if (k == 150) {
            DBG_LOGA("wait 1s, but camera still not closed , it's abnormal status.\n");
            return;
        }
        delete mEmulatedCameras[cameraId];
        mEmulatedCameras[cameraId] = NULL;
    }

    EmulatedBaseCamera *cam = mEmulatedCameras[cameraId];

    if ((!cam) && (newStatus == CAMERA_DEVICE_STATUS_PRESENT)) {
        /*suppose only usb camera produce uevent, and it is facing back*/
        cam = new EmulatedFakeCamera3(cameraId, &HAL_MODULE_INFO_SYM.common);
        if (cam != NULL) {
            CAMHAL_LOGDB("%s: new camera device version is %d", __FUNCTION__,
                    getFakeCameraHalVersion(cameraId));
            //sleep 10ms for /dev/video* create
            usleep(50000);
            while (i < 20) {
                if (0 == access(dev_name, F_OK | R_OK | W_OK)) {
                    DBG_LOGB("access %s success\n", dev_name);
                    break;
                } else {
                    CAMHAL_LOGDB("access %s fail , i = %d .\n", dev_name,i);
                    usleep(50000);
                    i++;
                }
            }
            res = cam->Initialize();
            if (res != NO_ERROR) {
                ALOGE("%s: Unable to intialize camera %d: %s (%d)",
                    __FUNCTION__, cameraId, strerror(-res), res);
                delete cam;
                return ;
            }

            /* Open the camera. then send the callback to framework*/
            mEmulatedCameras[cameraId] = cam;
            mEmulatedCameraNum ++;
            cam->plugCamera();
            if (cb != NULL && cb->camera_device_status_change != NULL) {
                cb->camera_device_status_change(cb, cameraId, newStatus);
            }
        }
        return ;
    }

    CAMHAL_LOGDB("mEmulatedCameraNum =%d\n", mEmulatedCameraNum);

    /**
     * (Order is important)
     * Send the callback first to framework, THEN close the camera.
     */

    if (newStatus == cam->getHotplugStatus()) {
        CAMHAL_LOGDB("%s: Ignoring transition to the same status", __FUNCTION__);
        return;
    }

/*here we don't notify cameraservice close camera, let app to close camera, or will generate crash*/
#if 0
    CAMHAL_LOGDB("mEmulatedCameraNum =%d\n", mEmulatedCameraNum);
    if (cb != NULL && cb->camera_device_status_change != NULL) {
        cb->camera_device_status_change(cb, cameraId, newStatus);
    }
#endif

    CAMHAL_LOGDB("mEmulatedCameraNum =%d\n", mEmulatedCameraNum);

    if (newStatus == CAMERA_DEVICE_STATUS_NOT_PRESENT) {
        mEmulatedCameraNum --;
        j = getValidCameraOjectId();
        while (m < 200) {
            if (mEmulatedCameras[j] != NULL) {
                if (mEmulatedCameras[j]->getCameraStatus()) {
                    DBG_LOGA("start to delete EmulatedFakeCamera3 object");
                    cam->unplugCamera();
                    delete mEmulatedCameras[j];
                    mEmulatedCameras[j] = NULL;
                } else {
                    usleep(5000);
                    m++;
                }
            } else {
                break;
            }
        }
        if (m == 200) {
            cam->unplugCamera();
        }

        if (cb != NULL && cb->camera_device_status_change != NULL) {
            DBG_LOGA("callback unplug status to framework.\n");
            cb->camera_device_status_change(cb, cameraId, newStatus);
        }
    } else if (newStatus == CAMERA_DEVICE_STATUS_PRESENT) {
        CAMHAL_LOGDA("camera plugged again?\n");
        cam->plugCamera();
    }
    CAMHAL_LOGDB("mEmulatedCameraNum =%d\n", mEmulatedCameraNum);

}

/********************************************************************************
 * Initializer for the static member structure.
 *******************************************************************************/

/* Entry point for camera HAL API. */
struct hw_module_methods_t EmulatedCameraFactory::mCameraModuleMethods = {
    .open = EmulatedCameraFactory::device_open
};

}; /* namespace android */
