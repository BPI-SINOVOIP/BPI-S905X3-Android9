#ifndef DTV_PATCH_OUT_H
#define DTV_PATCH_OUT_H

#define OUTPUT_BUFFER_SIZE (8 * 1024)

typedef int (*out_pcm_write)(unsigned char *pcm_data, int size, int symbolrate, int channel, void *args);
typedef int (*out_raw_wirte)(unsigned char *raw_data, int size,
                             void *args);

typedef int (*out_get_wirte_space)(void *args);

int dtv_patch_input_open(unsigned int *handle, out_pcm_write pcmcb,
                         out_get_wirte_space spacecb, void *args);
int dtv_patch_input_start(unsigned int handle, int aformat, int has_video);
int dtv_patch_input_stop(unsigned int handle);
int dtv_patch_input_pause(unsigned int handle);
int dtv_patch_input_resume(unsigned int handle);
unsigned long dtv_patch_get_pts(void);

#endif
