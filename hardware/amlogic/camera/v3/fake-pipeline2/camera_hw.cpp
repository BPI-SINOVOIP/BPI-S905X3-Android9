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
 *
 * Description:
 */

#ifndef __CAMERA_HW__
#define __CAMERA_HW__

//#define LOG_NDEBUG 0
#define LOG_TAG "Camera_hw"

#include <errno.h>
#include <cutils/properties.h>
#include "camera_hw.h"
#include "ispaaalib.h"

#ifdef __cplusplus
//extern "C" {
#endif
static int set_rotate_value(int camera_fd, int value)
{
    int ret = 0;
    struct v4l2_control ctl;
    if(camera_fd<0)
        return -1;
    if((value!=0)&&(value!=90)&&(value!=180)&&(value!=270)){
        CAMHAL_LOGDB("Set rotate value invalid: %d.", value);
        return -1;
    }
    memset( &ctl, 0, sizeof(ctl));
    ctl.value=value;
    ctl.id = V4L2_CID_ROTATE;
    ALOGD("set_rotate_value:: id =%x , value=%d",ctl.id,ctl.value);
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if(ret<0){
        CAMHAL_LOGDB("Set rotate value fail: %s,errno=%d. ret=%d", strerror(errno),errno,ret);
    }
    return ret ;
}

void set_device_status(struct VideoInfo *vinfo)
{
    vinfo->dev_status = -1;
}

int get_device_status(struct VideoInfo *vinfo)
{
    return vinfo->dev_status;
}

int camera_open(struct VideoInfo *cam_dev)
{
        char dev_name[128];
        int ret;
        char property[PROPERTY_VALUE_MAX];

        property_get("ro.vendor.platform.board_camera", property, "false");
        if (strstr(property, "true")) {
                cam_dev->fd = open("/dev/video50", O_RDWR | O_NONBLOCK);
        } else {
                sprintf(dev_name, "%s%d", "/dev/video", cam_dev->idx);
                cam_dev->fd = open(dev_name, O_RDWR | O_NONBLOCK);
        }

        //cam_dev->fd = open("/dev/video0", O_RDWR | O_NONBLOCK);
        if (cam_dev->fd < 0){
                DBG_LOGB("open %s failed, errno=%d\n", dev_name, errno);
                return -ENOTTY;
        }

        ret = ioctl(cam_dev->fd, VIDIOC_QUERYCAP, &cam_dev->cap);
        if (ret < 0) {
                DBG_LOGB("VIDIOC_QUERYCAP, errno=%d", errno);
        }

        if (!(cam_dev->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                DBG_LOGB( "%s is not video capture device\n",
                                dev_name);
        }

        if (!(cam_dev->cap.capabilities & V4L2_CAP_STREAMING)) {
                DBG_LOGB( "video%d does not support streaming i/o\n",
                                cam_dev->idx);
        }

        if (strstr((const char *)cam_dev->cap.driver, "ARM-camera-isp")) {
                isp_lib_enable();
        }

        return ret;
}

int setBuffersFormat(struct VideoInfo *cam_dev)
{
        int ret = 0;
        if ((cam_dev->preview.format.fmt.pix.width != 0) && (cam_dev->preview.format.fmt.pix.height != 0)) {
        int pixelformat = cam_dev->preview.format.fmt.pix.pixelformat;

        ret = ioctl(cam_dev->fd, VIDIOC_S_FMT, &cam_dev->preview.format);
        if (ret < 0) {
                DBG_LOGB("Open: VIDIOC_S_FMT Failed: %s, ret=%d\n", strerror(errno), ret);
        }

        CAMHAL_LOGIB("Width * Height %d x %d expect pixelfmt:%.4s, get:%.4s\n",
                        cam_dev->preview.format.fmt.pix.width,
                        cam_dev->preview.format.fmt.pix.height,
                        (char*)&pixelformat,
                        (char*)&cam_dev->preview.format.fmt.pix.pixelformat);
        }
        return ret;
}

int start_capturing(struct VideoInfo *vinfo)
{
        int ret = 0;
        int i;
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;

        if (vinfo->isStreaming) {
                DBG_LOGA("already stream on\n");
        }
        CLEAR(vinfo->preview.rb);

        vinfo->preview.rb.count = 6;
        vinfo->preview.rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //TODO DMABUF & ION
        vinfo->preview.rb.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(vinfo->fd, VIDIOC_REQBUFS, &vinfo->preview.rb);
        if (ret < 0) {
                DBG_LOGB("camera idx:%d does not support "
                                "memory mapping, errno=%d\n", vinfo->idx, errno);
        }

        if (vinfo->preview.rb.count < 2) {
                DBG_LOGB( "Insufficient buffer memory on /dev/video%d, errno=%d\n",
                                vinfo->idx, errno);
                return -EINVAL;
        }

        for (i = 0; i < (int)vinfo->preview.rb.count; ++i) {

                CLEAR(vinfo->preview.buf);

                vinfo->preview.buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                vinfo->preview.buf.memory      = V4L2_MEMORY_MMAP;
                vinfo->preview.buf.index       = i;

                if (ioctl(vinfo->fd, VIDIOC_QUERYBUF, &vinfo->preview.buf) < 0) {
                        DBG_LOGB("VIDIOC_QUERYBUF, errno=%d", errno);
                }
            /*pluge usb camera when preview, vinfo->preview.buf.length value will equal to 0, so save this value*/
            vinfo->tempbuflen = vinfo->preview.buf.length;
            vinfo->mem[i] = mmap(NULL /* start anywhere */,
                                    vinfo->preview.buf.length,
                                        PROT_READ | PROT_WRITE /* required */,
                                        MAP_SHARED /* recommended */,
                                        vinfo->fd,
                                    vinfo->preview.buf.m.offset);

                if (MAP_FAILED == vinfo->mem[i]) {
                        DBG_LOGB("mmap failed, errno=%d\n", errno);
                }
        }
        ////////////////////////////////
        for (i = 0; i < (int)vinfo->preview.rb.count; ++i) {

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (ioctl(vinfo->fd, VIDIOC_QBUF, &buf) < 0)
                        DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if ((vinfo->preview.format.fmt.pix.width != 0) &&
               (vinfo->preview.format.fmt.pix.height != 0)) {
             if (ioctl(vinfo->fd, VIDIOC_STREAMON, &type) < 0)
                  DBG_LOGB("VIDIOC_STREAMON, errno=%d\n", errno);
        }

        vinfo->isStreaming = true;
        return 0;
}

int stop_capturing(struct VideoInfo *vinfo)
{
        enum v4l2_buf_type type;
        int res = 0;
        int i;

        if (!vinfo->isStreaming)
                return -1;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(vinfo->fd, VIDIOC_STREAMOFF, &type) < 0) {
                DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);
                res = -1;
        }

        if (!vinfo->preview.buf.length) {
            vinfo->preview.buf.length = vinfo->tempbuflen;
        }

        for (i = 0; i < (int)vinfo->preview.rb.count; ++i) {
                if (munmap(vinfo->mem[i], vinfo->preview.buf.length) < 0) {
                        DBG_LOGB("munmap failed errno=%d", errno);
                        res = -1;
                }
        }

        if (strstr((const char *)vinfo->cap.driver, "ARM-camera-isp")) {
            vinfo->preview.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            vinfo->preview.rb.memory = V4L2_MEMORY_MMAP;
            vinfo->preview.rb.count = 0;

            res = ioctl(vinfo->fd, VIDIOC_REQBUFS, &vinfo->preview.rb);
            if (res < 0) {
                DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            }else{
                DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
            }
        }

        vinfo->isStreaming = false;
        return res;
}

int releasebuf_and_stop_capturing(struct VideoInfo *vinfo)
{
        enum v4l2_buf_type type;
        int res = 0 ,ret;
        int i;

        if (!vinfo->isStreaming)
                return -1;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if ((vinfo->preview.format.fmt.pix.width != 0) &&
               (vinfo->preview.format.fmt.pix.height != 0)) {
            if (ioctl(vinfo->fd, VIDIOC_STREAMOFF, &type) < 0) {
                 DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);
                 res = -1;
            }
        }
        if (!vinfo->preview.buf.length) {
            vinfo->preview.buf.length = vinfo->tempbuflen;
        }
        for (i = 0; i < (int)vinfo->preview.rb.count; ++i) {
                if (munmap(vinfo->mem[i], vinfo->preview.buf.length) < 0) {
                        DBG_LOGB("munmap failed errno=%d", errno);
                        res = -1;
                }
        }
        vinfo->isStreaming = false;

        vinfo->preview.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->preview.rb.memory = V4L2_MEMORY_MMAP;
        vinfo->preview.rb.count = 0;

        ret = ioctl(vinfo->fd, VIDIOC_REQBUFS, &vinfo->preview.rb);
        if (ret < 0) {
           DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
           //return ret;
        }else{
           DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
        }
        return res;
}


uintptr_t get_frame_phys(struct VideoInfo *vinfo)
{
        CLEAR(vinfo->preview.buf);

        vinfo->preview.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->preview.buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(vinfo->fd, VIDIOC_DQBUF, &vinfo->preview.buf) < 0) {
                switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                DBG_LOGB("VIDIOC_DQBUF failed, errno=%d\n", errno);
                                exit(1);
                }
        DBG_LOGB("VIDIOC_DQBUF failed, errno=%d\n", errno);
        }

        return (uintptr_t)vinfo->preview.buf.m.userptr;
}

void *get_frame(struct VideoInfo *vinfo)
{
        CLEAR(vinfo->preview.buf);

        vinfo->preview.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->preview.buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(vinfo->fd, VIDIOC_DQBUF, &vinfo->preview.buf) < 0) {
                switch (errno) {
                        case EAGAIN:
                                return NULL;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                CAMHAL_LOGDB("VIDIOC_DQBUF failed, errno=%d\n", errno); //CAMHAL_LOGDB
                                //exit(1); /*here will generate crash, so delete.  when ocour error, should break while() loop*/
                                set_device_status(vinfo);
                                return NULL;
                }
        }
        //DBG_LOGA("get frame\n");
        return vinfo->mem[vinfo->preview.buf.index];
}

int putback_frame(struct VideoInfo *vinfo)
{
        if (vinfo->dev_status == -1)
            return 0;

        if (!vinfo->preview.buf.length) {
            vinfo->preview.buf.length = vinfo->tempbuflen;
        }

        if (ioctl(vinfo->fd, VIDIOC_QBUF, &vinfo->preview.buf) < 0) {
            DBG_LOGB("QBUF failed error=%d\n", errno);
            if (errno == ENODEV) {
                set_device_status(vinfo);
            }
        }

        return 0;
}

int putback_picture_frame(struct VideoInfo *vinfo)
{

        if (ioctl(vinfo->fd, VIDIOC_QBUF, &vinfo->picture.buf) < 0)
                DBG_LOGB("QBUF failed error=%d\n", errno);

        return 0;
}

int start_picture(struct VideoInfo *vinfo, int rotate)
{
        int ret = 0;
        int i;
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;
        bool usbcamera = false;

        CLEAR(vinfo->picture.rb);

        //step 1 : ioctl  VIDIOC_S_FMT
        ret = ioctl(vinfo->fd, VIDIOC_S_FMT, &vinfo->picture.format);
        if (ret < 0) {
                DBG_LOGB("Open: VIDIOC_S_FMT Failed: %s, ret=%d\n", strerror(errno), ret);
        }

        //step 2 : request buffer
        vinfo->picture.rb.count = 3;
        vinfo->picture.rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //TODO DMABUF & ION
        vinfo->picture.rb.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(vinfo->fd, VIDIOC_REQBUFS, &vinfo->picture.rb);
        if (ret < 0) {
                DBG_LOGB("camera idx:%d does not support "
                                "memory mapping, errno=%d\n", vinfo->idx, errno);
        }

        if (vinfo->picture.rb.count < 1) {
                DBG_LOGB( "Insufficient buffer memory on /dev/video%d, errno=%d\n",
                                vinfo->idx, errno);
                return -EINVAL;
        }

        //step 3: mmap buffer
        for (i = 0; i < (int)vinfo->picture.rb.count; ++i) {

                CLEAR(vinfo->picture.buf);

                vinfo->picture.buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                vinfo->picture.buf.memory      = V4L2_MEMORY_MMAP;
                vinfo->picture.buf.index       = i;

                if (ioctl(vinfo->fd, VIDIOC_QUERYBUF, &vinfo->picture.buf) < 0) {
                        DBG_LOGB("VIDIOC_QUERYBUF, errno=%d", errno);
                }
                vinfo->mem_pic[i] = mmap(NULL /* start anywhere */,
                                    vinfo->picture.buf.length,
                                        PROT_READ | PROT_WRITE /* required */,
                                        MAP_SHARED /* recommended */,
                                        vinfo->fd,
                                    vinfo->picture.buf.m.offset);

                if (MAP_FAILED == vinfo->mem_pic[i]) {
                        DBG_LOGB("mmap failed, errno=%d\n", errno);
                }
        }

        //step 4 : QBUF
                ////////////////////////////////
        for (i = 0; i < (int)vinfo->picture.rb.count; ++i) {

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (ioctl(vinfo->fd, VIDIOC_QBUF, &buf) < 0)
                        DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);
        }

        if (vinfo->isPicture) {
                DBG_LOGA("already stream on\n");
        }

        if (strstr((const char *)vinfo->cap.driver, "uvcvideo")) {
            usbcamera = true;
        }
        if (!usbcamera) {
            set_rotate_value(vinfo->fd,rotate);
        }
        //step 5: Stream ON
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(vinfo->fd, VIDIOC_STREAMON, &type) < 0)
                DBG_LOGB("VIDIOC_STREAMON, errno=%d\n", errno);
        vinfo->isPicture = true;

        return 0;
}

void *get_picture(struct VideoInfo *vinfo)
{
        CLEAR(vinfo->picture.buf);

        vinfo->picture.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->picture.buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(vinfo->fd, VIDIOC_DQBUF, &vinfo->picture.buf) < 0) {
             switch (errno) {
             case EAGAIN:
                return NULL;
             case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
             default:
                DBG_LOGB("VIDIOC_DQBUF failed, errno=%d\n", errno);
                set_device_status(vinfo);
                return NULL;
            }
        }
        DBG_LOGA("get picture\n");
        return vinfo->mem_pic[vinfo->picture.buf.index];
}

void stop_picture(struct VideoInfo *vinfo)
{
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;
        int i;
        int ret;

        if (!vinfo->isPicture)
                return ;

        //QBUF
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = vinfo->picture.buf.index;
        if (ioctl(vinfo->fd, VIDIOC_QBUF, &buf) < 0)
            DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);

        //stream off and unmap buffer
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(vinfo->fd, VIDIOC_STREAMOFF, &type) < 0)
                DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);

        for (i = 0; i < (int)vinfo->picture.rb.count; i++)
        {
            if (munmap(vinfo->mem_pic[i], vinfo->picture.buf.length) < 0)
                DBG_LOGB("munmap failed errno=%d", errno);
        }

        if (strstr((const char *)vinfo->cap.driver, "ARM-camera-isp")) {
            vinfo->picture.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            vinfo->picture.rb.memory = V4L2_MEMORY_MMAP;
            vinfo->picture.rb.count = 0;

            ret = ioctl(vinfo->fd, VIDIOC_REQBUFS, &vinfo->picture.rb);
            if (ret < 0) {
                DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            } else {
                DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
            }
        }

        set_rotate_value(vinfo->fd,0);
        vinfo->isPicture = false;
        setBuffersFormat(vinfo);
        start_capturing(vinfo);
}

void releasebuf_and_stop_picture(struct VideoInfo *vinfo)
{
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;
        int i,ret;

        if (!vinfo->isPicture)
                return ;

        //QBUF
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = vinfo->picture.buf.index;
        if (ioctl(vinfo->fd, VIDIOC_QBUF, &buf) < 0)
            DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);

        //stream off and unmap buffer
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(vinfo->fd, VIDIOC_STREAMOFF, &type) < 0)
                DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);

        for (i = 0; i < (int)vinfo->picture.rb.count; i++)
        {
            if (munmap(vinfo->mem_pic[i], vinfo->picture.buf.length) < 0)
                DBG_LOGB("munmap failed errno=%d", errno);
        }

        vinfo->isPicture = false;

        vinfo->picture.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vinfo->picture.rb.memory = V4L2_MEMORY_MMAP;
        vinfo->picture.rb.count = 0;
        ret = ioctl(vinfo->fd, VIDIOC_REQBUFS, &vinfo->picture.rb);
        if (ret < 0) {
          DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
          //return ret;
        }else{
          DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
        }
        setBuffersFormat(vinfo);
        start_capturing(vinfo);
}

void camera_close(struct VideoInfo *vinfo)
{
    if (NULL == vinfo) {
        DBG_LOGA("vinfo is null\n");
        return ;
    }

    if (close(vinfo->fd) != 0)
        DBG_LOGB("close failed, errno=%d\n", errno);

    vinfo->fd = -1;

    if (strstr((const char *)vinfo->cap.driver, "ARM-camera-isp")) {
        isp_lib_disable();
    }
}
#ifdef __cplusplus
//}
#endif
#endif
