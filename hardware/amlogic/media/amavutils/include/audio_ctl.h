/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef AUDIO_CTL_H
#define AUDIO_CTL_H

#ifdef  __cplusplus
extern "C" {
#endif
int media_set_adec_format(int format);
int media_get_adec_format();
int media_set_adec_samplerate(int rate);
int media_get_adec_samplerate();
int media_set_adec_channum(int num);
int media_get_adec_channum();
int media_set_adec_pts(int pts);
int media_get_adec_pts();
int media_set_adec_datawidth(int width);
int media_get_adec_datawidth();
int media_get_audio_digital_output_mode();

#ifdef  __cplusplus
}
#endif

#endif