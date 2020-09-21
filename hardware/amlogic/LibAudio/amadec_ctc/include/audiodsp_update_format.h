#ifndef __AUDIODSP_UPDATE_FORMAT_H__
#define __AUDIODSP_UPDATE_FORMAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <audio-dec.h>
#include <audiodsp.h>
#include <log-print.h>

void adec_reset_track_enable(int enable_flag);
void adec_reset_track(aml_audio_dec_t *audec);
int audiodsp_format_update(aml_audio_dec_t *audec);
void audiodsp_set_format_changed_flag(int val);

int audiodsp_get_pcm_left_len();

#endif