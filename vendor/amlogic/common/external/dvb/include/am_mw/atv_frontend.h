/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef __ATV_FRONTEND_H__
#define __ATV_FRONTEND_H__

#include <linux/types.h>
#include <linux/videodev2.h>


#define V4L2_FE_DEV    "/dev/v4l2_frontend"

struct v4l2_tune_status {
	unsigned char lock; /* unlocked:0, locked 1 */
	v4l2_std_id std;
	unsigned int audmode;
	int snr;
	int afc;
	union {
		void *resrvred;
		__u64 reserved1;
	};
};

enum v4l2_status {
	V4L2_HAS_SIGNAL  = 0x01, /* found something above the noise level */
	V4L2_HAS_CARRIER = 0x02, /* found a DVB signal */
	V4L2_HAS_VITERBI = 0x04, /* FEC is stable  */
	V4L2_HAS_SYNC    = 0x08, /* found sync bytes  */
	V4L2_HAS_LOCK    = 0x10, /* everything's working... */
	V4L2_TIMEDOUT    = 0x20, /* no lock within the last ~2 seconds */
	V4L2_REINIT      = 0x40, /* frontend was reinitialized, */
};							 /* application is recommended to reset */
							 /* DiSEqC, tone and parameters */

struct v4l2_analog_parameters {
	unsigned int frequency;
	unsigned int audmode;	/* audio mode standard */
	unsigned int soundsys;	/*A2,BTSC,EIAJ,NICAM */
	v4l2_std_id std;	/* v4l2 analog video standard */
	unsigned int flag;
	unsigned int afc_range;
	unsigned int reserved;
};

struct v4l2_property {
	unsigned int cmd;
	unsigned int data;
	int result;
};

struct v4l2_properties {
	unsigned int num;
	union {
		struct v4l2_property *props;
		__u64 reserved;
	};
};

struct v4l2_frontend_event {
	enum v4l2_status status;
	struct v4l2_analog_parameters parameters;
};

#define V4L2_SET_FRONTEND    _IOW('V', 105, struct v4l2_analog_parameters)
#define V4L2_GET_FRONTEND    _IOR('V', 106, struct v4l2_analog_parameters)
#define V4L2_GET_EVENT       _IOR('V', 107, struct v4l2_frontend_event)
#define V4L2_SET_MODE        _IOW('V', 108, int) /* 1 : entry atv, 0 : leave atv */
#define V4L2_READ_STATUS     _IOR('V', 109, enum v4l2_status)
#define V4L2_SET_PROPERTY    _IOWR('V', 110, struct v4l2_properties)
#define V4L2_GET_PROPERTY    _IOWR('V', 111, struct v4l2_properties)
#define V4L2_DETECT_TUNE     _IOR('V', 112, struct v4l2_tune_status)
#define V4L2_DETECT_STANDARD _IO('V', 113)

#define V4L2_TUNE                1
#define V4L2_SOUND_SYS           2
#define V4L2_SLOW_SEARCH_MODE    3
#define V4L2_FREQUENCY           4
#define V4L2_STD                 5
#define V4L2_FINE_TUNE           6
#define V4L2_SIF_OVER_MODULATION 7
#define V4L2_TUNER_TYPE          8
#define V4L2_TUNER_IF_FREQ       9
#define V4L2_AFC                 10

/* audmode */


#endif /* __V4L2_FRONTEND_H__ */
