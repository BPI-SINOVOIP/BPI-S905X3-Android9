/*
* AMLOGIC Audio/Video streaming port driver.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the named License,
* or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*
* Author:  Tim Yao <timyao@amlogic.com>
*
*/

#ifndef AMSTREAM_H
#define AMSTREAM_H

#define PORT_FLAG_IN_USE    0x0001
#define PORT_FLAG_VFORMAT   0x0002
#define PORT_FLAG_AFORMAT   0x0004
#define PORT_FLAG_FORMAT    (PORT_FLAG_VFORMAT | PORT_FLAG_AFORMAT)
#define PORT_FLAG_VID       0x0008
#define PORT_FLAG_AID       0x0010
#define PORT_FLAG_ID        (PORT_FLAG_VID | PORT_FLAG_AID)
#define PORT_FLAG_INITED  0x100

#define PORT_TYPE_VIDEO     0x01
#define PORT_TYPE_AUDIO     0x02
#define PORT_TYPE_MPTS      0x04
#define PORT_TYPE_MPPS      0x08
#define PORT_TYPE_ES        0x10
#define PORT_TYPE_RM        0x20

#define AMSTREAM_IOC_MAGIC  'S'

#define AMSTREAM_IOC_VB_START   _IOW(AMSTREAM_IOC_MAGIC, 0x00, int)
#define AMSTREAM_IOC_VB_SIZE    _IOW(AMSTREAM_IOC_MAGIC, 0x01, int)
#define AMSTREAM_IOC_AB_START   _IOW(AMSTREAM_IOC_MAGIC, 0x02, int)
#define AMSTREAM_IOC_AB_SIZE    _IOW(AMSTREAM_IOC_MAGIC, 0x03, int)
#define AMSTREAM_IOC_VFORMAT    _IOW(AMSTREAM_IOC_MAGIC, 0x04, int)
#define AMSTREAM_IOC_AFORMAT    _IOW(AMSTREAM_IOC_MAGIC, 0x05, int)
#define AMSTREAM_IOC_VID        _IOW(AMSTREAM_IOC_MAGIC, 0x06, int)
#define AMSTREAM_IOC_AID        _IOW(AMSTREAM_IOC_MAGIC, 0x07, int)
#define AMSTREAM_IOC_VB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x08, unsigned long)
#define AMSTREAM_IOC_AB_STATUS  _IOR(AMSTREAM_IOC_MAGIC, 0x09, unsigned long)
#define AMSTREAM_IOC_SYSINFO    _IOW(AMSTREAM_IOC_MAGIC, 0x0a, int)
#define AMSTREAM_IOC_ACHANNEL   _IOW(AMSTREAM_IOC_MAGIC, 0x0b, int)
#define AMSTREAM_IOC_SAMPLERATE _IOW(AMSTREAM_IOC_MAGIC, 0x0c, int)
#define AMSTREAM_IOC_DATAWIDTH  _IOW(AMSTREAM_IOC_MAGIC, 0x0d, int)
#define AMSTREAM_IOC_TSTAMP     _IOW(AMSTREAM_IOC_MAGIC, 0x0e, unsigned long)
#define AMSTREAM_IOC_VDECSTAT   _IOR(AMSTREAM_IOC_MAGIC, 0x0f, unsigned long)
#define AMSTREAM_IOC_ADECSTAT   _IOR(AMSTREAM_IOC_MAGIC, 0x10, unsigned long)
#define AMSTREAM_IOC_PORT_INIT   _IO(AMSTREAM_IOC_MAGIC, 0x11)
#define AMSTREAM_IOC_TRICKMODE  _IOW(AMSTREAM_IOC_MAGIC, 0x12, unsigned long)
#define AMSTREAM_IOC_AUDIO_INFO  _IOW(AMSTREAM_IOC_MAGIC, 0x13, unsigned long)
#define AMSTREAM_IOC_TRICK_STAT  _IOR(AMSTREAM_IOC_MAGIC, 0x14, unsigned long)
#define AMSTREAM_IOC_AUDIO_RESET _IO(AMSTREAM_IOC_MAGIC, 0x15)
#define AMSTREAM_IOC_SID         _IOW(AMSTREAM_IOC_MAGIC, 0x16, int)
#define AMSTREAM_IOC_VPAUSE      _IOW(AMSTREAM_IOC_MAGIC, 0x17, int)
#define AMSTREAM_IOC_AVTHRESH   _IOW(AMSTREAM_IOC_MAGIC, 0x18, int)
#define AMSTREAM_IOC_SYNCTHRESH _IOW(AMSTREAM_IOC_MAGIC, 0x19, int)
#define AMSTREAM_IOC_SUB_RESET   _IOW(AMSTREAM_IOC_MAGIC, 0x1a, int)
#define AMSTREAM_IOC_SUB_LENGTH  _IOR(AMSTREAM_IOC_MAGIC, 0x1b, int)
#define AMSTREAM_IOC_SET_DEC_RESET _IOW(AMSTREAM_IOC_MAGIC, 0x1c, int)
#define AMSTREAM_IOC_TS_SKIPBYTE _IOW(AMSTREAM_IOC_MAGIC, 0x1d, int)
#define AMSTREAM_IOC_SUB_TYPE    _IOW(AMSTREAM_IOC_MAGIC, 0x1e, int)

#define AMSTREAM_IOC_GET_SUBTITLE_INFO       _IOR(AMSTREAM_IOC_MAGIC, 0xad, unsigned long)
#define AMSTREAM_IOC_SET_SUBTITLE_INFO       _IOW(AMSTREAM_IOC_MAGIC, 0xae, unsigned long)

struct buf_status
{
    int size;
    int data_len;
    int free_len;
    unsigned int read_pointer;
    unsigned int write_pointer;
};

struct vdec_status
{
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    unsigned int error_count;
    unsigned int status;
};

struct adec_status
{
    unsigned int channels;
    unsigned int sample_rate;
    unsigned int resolution;
    unsigned int error_count;
    unsigned int status;
};

struct am_io_param
{
    union
    {
        int data;
        int id;     //get bufstatus? //or others
    };

    int len;        //buffer size;

    union
    {
        char buf[1];
        struct buf_status status;
        struct vdec_status vstatus;
        struct adec_status astatus;
    };
};
void set_vdec_func(int (*vdec_func)(struct vdec_status *));
void set_adec_func(int (*adec_func)(struct adec_status *));

#endif              /* AMSTREAM_H */
