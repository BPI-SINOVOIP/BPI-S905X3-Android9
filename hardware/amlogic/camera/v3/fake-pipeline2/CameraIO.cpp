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

#ifndef __CAMERA_IO__
#define __CAMERA_IO__

//#define LOG_NDEBUG 0
#define LOG_TAG "Camera_IO"

#include <errno.h>
#include <cutils/properties.h>
#include "CameraIO.h"
namespace android {
CVideoInfo::CVideoInfo(){
    memset(&cap, 0, sizeof(struct v4l2_capability));
    memset(&preview,0,sizeof(FrameV4L2Info));
    memset(&picture,0,sizeof(FrameV4L2Info));
    memset(mem,0,sizeof(mem));
    memset(mem_pic,0,sizeof(mem_pic));
    //memset(canvas,0,sizeof(canvas));
    isStreaming = false;
    isPicture = false;
    //canvas_mode = false;
    width = 0;
    height = 0;
    formatIn = 0;
    framesizeIn = 0;
    idVendor = 0;
    idProduct = 0;
    idx = 0;
    fd = -1;
    tempbuflen = 0;
    dev_status = 0;
}

int CVideoInfo::EnumerateFormat(uint32_t pixelformat){
    struct v4l2_fmtdesc fmt;
    int ret;
    if (fd < 0) {
        ALOGE("camera not be init!");
        return -1;
    }
    memset(&fmt,0,sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.index = 0;
    while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt)) == 0) {
        if (fmt.pixelformat == pixelformat)
            return pixelformat;
        fmt.index++;
    }
    return 0;
}

bool CVideoInfo::IsSupportRotation() {
    struct v4l2_queryctrl qc;
    int ret = 0;
    bool Support = false;
    memset(&qc, 0, sizeof(struct v4l2_queryctrl));
    qc.id = V4L2_ROTATE_ID;
    ret = ioctl (fd, VIDIOC_QUERYCTRL, &qc);
    if ((qc.flags == V4L2_CTRL_FLAG_DISABLED) ||( ret < 0)|| (qc.type != V4L2_CTRL_TYPE_INTEGER)) {
            Support = false;
    }else{
            Support = true;
    }
    return Support;
}

int CVideoInfo::set_rotate(int camera_fd, int value)
{
    int ret = 0;
    struct v4l2_control ctl;
    if (camera_fd < 0)
        return -1;
    if ((value != 0) && (value != 90) && (value != 180) && (value != 270)) {
        CAMHAL_LOGDB("Set rotate value invalid: %d.", value);
        return -1;
    }
    memset( &ctl, 0, sizeof(ctl));
    ctl.value=value;
    ctl.id = V4L2_CID_ROTATE;
    ALOGD("set_rotate:: id =%x , value=%d",ctl.id,ctl.value);
    ret = ioctl(camera_fd, VIDIOC_S_CTRL, &ctl);
    if (ret<0) {
        CAMHAL_LOGDB("Set rotate value fail: %s,errno=%d. ret=%d", strerror(errno),errno,ret);
    }
    return ret ;
}

void CVideoInfo::set_device_status(void)
{
    dev_status = -1;
}

int CVideoInfo::get_device_status(void)
{
    return dev_status;
}

int CVideoInfo::camera_init(void)
{
    ALOGV("%s: E", __FUNCTION__);
    int ret =0 ;
    if (fd < 0) {
          ALOGE("open /dev/video%d failed, errno=%s\n", idx, strerror(errno));
          return -ENOTTY;
    }

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        ALOGE("VIDIOC_QUERYCAP, errno=%s", strerror(errno));
        return ret;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        ALOGV( "/dev/video%d is not video capture device\n",idx);


    if (!(cap.capabilities & V4L2_CAP_STREAMING))
        ALOGV( "video%d does not support streaming i/o\n",idx);

    if (strstr((const char*)cap.driver,"ARM-camera-isp"))
        sprintf(sensor_type,"%s","mipi");
    else
        sprintf(sensor_type,"%s","usb");

    return ret;
}


int CVideoInfo::setBuffersFormat(void)
{
        int ret = 0;
    if (fd < 0) {
        ALOGE("camera not be init!");
        return -1;
    }
        if ((preview.format.fmt.pix.width != 0) && (preview.format.fmt.pix.height != 0)) {
        int pixelformat = preview.format.fmt.pix.pixelformat;

        ret = ioctl(fd, VIDIOC_S_FMT, &preview.format);
        if (ret < 0) {
                DBG_LOGB("Open: VIDIOC_S_FMT Failed: %s, ret=%d\n", strerror(errno), ret);
        }

        CAMHAL_LOGIB("Width * Height %d x %d expect pixelfmt:%.4s, get:%.4s\n",
                        preview.format.fmt.pix.width,
                        preview.format.fmt.pix.height,
                        (char*)&pixelformat,
                        (char*)&preview.format.fmt.pix.pixelformat);
        }
        return ret;
}

int CVideoInfo::start_capturing(void)
{
        int ret = 0;
        int i;
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;

        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }

        if (isStreaming) {
                DBG_LOGA("already stream on\n");
        }
        CLEAR(preview.rb);

        preview.rb.count = IO_PREVIEW_BUFFER;
        preview.rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //TODO DMABUF & ION
        preview.rb.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(fd, VIDIOC_REQBUFS, &preview.rb);
        if (ret < 0) {
                DBG_LOGB("camera idx:%d does not support "
                                "memory mapping, errno=%d\n", idx, errno);
        }

        if (preview.rb.count < 2) {
                DBG_LOGB( "Insufficient buffer memory on /dev/video%d, errno=%d\n",
                                idx, errno);
                return -EINVAL;
        }

        for (i = 0; i < (int)preview.rb.count; ++i) {

                CLEAR(preview.buf);

                preview.buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                preview.buf.memory      = V4L2_MEMORY_MMAP;
                preview.buf.index       = i;

                if (ioctl(fd, VIDIOC_QUERYBUF, &preview.buf) < 0) {
                        DBG_LOGB("VIDIOC_QUERYBUF, errno=%d", errno);
                }
            /*pluge usb camera when preview, vinfo->preview.buf.length value will equal to 0, so save this value*/
            mem[i].size = preview.buf.length;
            mem[i].addr = mmap(NULL /* start anywhere */,
                                    mem[i].size,
                                    PROT_READ | PROT_WRITE /* required */,
                                    MAP_SHARED /* recommended */,
                                    fd,
                                    preview.buf.m.offset);

                if (MAP_FAILED == mem[i].addr) {
                        DBG_LOGB("mmap failed, errno=%d\n", errno);
                }
        }
        ////////////////////////////////
        for (i = 0; i < (int)preview.rb.count; ++i) {

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
                        DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if ((preview.format.fmt.pix.width != 0) &&
               (preview.format.fmt.pix.height != 0)) {
             if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
                  DBG_LOGB("VIDIOC_STREAMON, errno=%d\n", errno);
        }

        isStreaming = true;
        return 0;
}

int CVideoInfo::stop_capturing(void)
{
        enum v4l2_buf_type type;
        int res = 0;
        int i;
        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }

        if (!isStreaming)
                return -1;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
                DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);
                res = -1;
        }

        if (!preview.buf.length) {
            preview.buf.length = tempbuflen;
        }

        for (i = 0; i < (int)preview.rb.count; ++i) {
                if (munmap(mem[i].addr, mem[i].size) < 0) {
                        DBG_LOGB("munmap failed errno=%d", errno);
                        res = -1;
                }
        }

        if (strstr((const char *)cap.driver, "ARM-camera-isp")) {
            preview.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            preview.rb.memory = V4L2_MEMORY_MMAP;
            preview.rb.count = 0;

            res = ioctl(fd, VIDIOC_REQBUFS, &preview.rb);
            if (res < 0) {
                DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            }else{
                DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
            }
        }

        isStreaming = false;
        return res;
}

int CVideoInfo::releasebuf_and_stop_capturing(void)
{
        enum v4l2_buf_type type;
        int res = 0 ,ret;
        int i;

        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }
        if (!isStreaming)
                return -1;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if ((preview.format.fmt.pix.width != 0) &&
               (preview.format.fmt.pix.height != 0)) {
            if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
                 DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);
                 res = -1;
            }
        }
        if (!preview.buf.length) {
            preview.buf.length = tempbuflen;
        }
        for (i = 0; i < (int)preview.rb.count; ++i) {
                if (munmap(mem[i].addr, mem[i].size) < 0) {
                        DBG_LOGB("munmap failed errno=%d", errno);
                        res = -1;
                }
        }
        isStreaming = false;

        preview.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        preview.rb.memory = V4L2_MEMORY_MMAP;
        preview.rb.count = 0;

        ret = ioctl(fd, VIDIOC_REQBUFS, &preview.rb);
        if (ret < 0) {
           DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
           //return ret;
        }else{
           DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
        }
        return res;
}


uintptr_t CVideoInfo::get_frame_phys(void)
{
        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }
        CLEAR(preview.buf);

        preview.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        preview.buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &preview.buf) < 0) {
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

        return (uintptr_t)preview.buf.m.userptr;
}

int CVideoInfo::get_frame_index(FrameV4L2Info& info) {
        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }
        CLEAR(info.buf);
        info.buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        info.buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_DQBUF, &info.buf) < 0) {
            switch (errno) {
                case EAGAIN:
                    return -1;

                case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

                default:
                    CAMHAL_LOGDB("VIDIOC_DQBUF failed, errno=%d\n", errno); //CAMHAL_LOGDB
                    //exit(1); /*here will generate crash, so delete.  when ocour error, should break while() loop*/
                    if (errno == ENODEV) {
                        set_device_status();
                    }
                    return -1;
            }
        }
        return info.buf.index;
}

void* CVideoInfo::get_frame()
{
    //DBG_LOGA("get frame\n");
    int index = get_frame_index(preview);
    if (index < 0)
        return nullptr;
    return mem[index].addr;
}

int CVideoInfo::putback_frame()
{
        if (dev_status == -1)
            return 0;

        if (!preview.buf.length) {
            preview.buf.length = tempbuflen;
        }
        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }

        if (ioctl(fd, VIDIOC_QBUF, &preview.buf) < 0) {
            DBG_LOGB("QBUF failed :%s\n", strerror(errno));
            if (errno == ENODEV) {
                set_device_status();
            }
        }

        return 0;
}

int CVideoInfo::putback_picture_frame()
{
        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }
        if (ioctl(fd, VIDIOC_QBUF, &picture.buf) < 0)
                DBG_LOGB("QBUF failed error=%d\n", errno);

        return 0;
}

int CVideoInfo::start_picture(int rotate)
{
        int ret = 0;
        int i;
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;
        bool usbcamera = false;

        CLEAR(picture.rb);
        if (fd < 0) {
            ALOGE("camera not be init!");
            return -1;
        }

        //step 1 : ioctl  VIDIOC_S_FMT
        for (int i = 0; i < 3; i++) {
            ret = ioctl(fd, VIDIOC_S_FMT, &picture.format);
            if (ret < 0 ) {
             switch (errno) {
                 case  -EBUSY:
                 case  0:
                    usleep(3000); //3ms
                    continue;
                    default:
                    DBG_LOGB("Open: VIDIOC_S_FMT Failed: %s, errno=%d\n", strerror(errno), ret);
                 return ret;
             }
            }else
            break;
        }
        //step 2 : request buffer
        picture.rb.count = IO_PICTURE_BUFFER;
        picture.rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //TODO DMABUF & ION
        picture.rb.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(fd, VIDIOC_REQBUFS, &picture.rb);
        if (ret < 0 ) {
                DBG_LOGB("camera idx:%d does not support "
                                "memory mapping, errno=%d\n", idx, errno);
        }

        if (picture.rb.count < 1) {
                DBG_LOGB( "Insufficient buffer memory on /dev/video%d, errno=%d\n",
                                idx, errno);
                return -EINVAL;
        }

        //step 3: mmap buffer
        for (i = 0; i < (int)picture.rb.count; ++i) {

                CLEAR(picture.buf);

                picture.buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                picture.buf.memory      = V4L2_MEMORY_MMAP;
                picture.buf.index       = i;

                if (ioctl(fd, VIDIOC_QUERYBUF, &picture.buf) < 0) {
                        DBG_LOGB("VIDIOC_QUERYBUF, errno=%d", errno);
                }
                mem_pic[i].size = picture.buf.length;
                mem_pic[i].addr = mmap(NULL /* start anywhere */,
                                        mem_pic[i].size,
                                        PROT_READ | PROT_WRITE /* required */,
                                        MAP_SHARED /* recommended */,
                                        fd,
                                        picture.buf.m.offset);

                if (MAP_FAILED == mem_pic[i].addr) {
                        DBG_LOGB("mmap failed, errno=%d\n", errno);
                }
        }

        //step 4 : QBUF
                ////////////////////////////////
        for (i = 0; i < (int)picture.rb.count; ++i) {

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
                        DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);
        }

        if (isPicture) {
                DBG_LOGA("already stream on\n");
        }

        if (strstr((const char *)cap.driver, "uvcvideo")) {
            usbcamera = true;
        }
        if (!usbcamera) {
            set_rotate(fd,rotate);
        }
        //step 5: Stream ON
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
                DBG_LOGB("VIDIOC_STREAMON, errno=%d\n", errno);
        isPicture = true;

        return 0;
}

void* CVideoInfo::get_picture()
{
    DBG_LOGA("get picture\n");
    int index = get_frame_index(picture);
    if (index < 0)
        return nullptr;
    return mem_pic[picture.buf.index].addr;
}

void CVideoInfo::stop_picture()
{
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;
        int i;
        int ret;
        if (fd < 0) {
                ALOGE("camera not be init!");
                return ;
            }

        if (!isPicture)
                return ;

        //QBUF
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = picture.buf.index;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
            DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);

        //stream off and unmap buffer
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
                DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);

        for (i = 0; i < (int)picture.rb.count; i++)
        {
            if (munmap(mem_pic[i].addr, mem_pic[i].size) < 0)
                DBG_LOGB("munmap failed errno=%d", errno);
        }

        if (strstr((const char *)cap.driver, "ARM-camera-isp")) {
            picture.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            picture.rb.memory = V4L2_MEMORY_MMAP;
            picture.rb.count = 0;

            ret = ioctl(fd, VIDIOC_REQBUFS, &picture.rb);
            if (ret < 0) {
                DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
            } else {
                DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
            }
        }

        set_rotate(fd,0);
        isPicture = false;
        setBuffersFormat();
        start_capturing();
}

void CVideoInfo::releasebuf_and_stop_picture()
{
        enum v4l2_buf_type type;
        struct  v4l2_buffer buf;
        int i,ret;
        if (fd < 0) {
            ALOGE("camera not be init!");
            return;
        }

        if (!isPicture)
                return ;

        //QBUF
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = picture.buf.index;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
            DBG_LOGB("VIDIOC_QBUF failed, errno=%d\n", errno);

        //stream off and unmap buffer
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
                DBG_LOGB("VIDIOC_STREAMOFF, errno=%d", errno);

        for (i = 0; i < (int)picture.rb.count; i++)
        {
            if (munmap(mem_pic[i].addr, mem_pic[i].size) < 0)
                DBG_LOGB("munmap failed errno=%d", errno);
        }

        isPicture = false;

        picture.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        picture.rb.memory = V4L2_MEMORY_MMAP;
        picture.rb.count = 0;
        ret = ioctl(fd, VIDIOC_REQBUFS, &picture.rb);
        if (ret < 0) {
          DBG_LOGB("VIDIOC_REQBUFS failed: %s", strerror(errno));
          //return ret;
        }else{
          DBG_LOGA("VIDIOC_REQBUFS delete buffer success\n");
        }
        setBuffersFormat();
        start_capturing();
}
}
#endif

