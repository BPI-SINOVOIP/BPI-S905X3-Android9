#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>
//get android  media stream volume
#define CODE_CALC_VOLUME
#ifdef CODE_CALC_VOLUME
#include <media/AudioSystem.h>
#define EXTERN_TAG  extern "C"
namespace android
{
#else
#define EXTERN_TAG
#endif
//#define  LOG_TAG    "wfd-output"
#define adec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

// default using tinyalsa
#include <tinyalsa/asoundlib.h>



//"/sys/class/switch/hdmi/state"
#define  HDMI_SWITCH_STATE_PATH  "/sys/class/amhdmitx/amhdmitx0/hpd_state"

// audio PCM output configuration
#define WFD_PERIOD_SIZE  1024
#define WFD_PERIOD_NUM   4
static struct pcm_config wfd_config_out;
static struct pcm *wfd_pcm;
static char cache_buffer_bytes[64];
static int  cached_len = 0;
static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
static int tv_mode = 0;
static int getprop_bool(const char * path)
{
    char buf[PROPERTY_VALUE_MAX];
    int ret = -1;
    ret = property_get(path, buf, NULL);
    if (ret > 0) {
        if (strcasecmp(buf, "true") == 0 || strcmp(buf, "1") == 0) {
            return 1;
        }
    }
    return 0;
}
static int get_hdmi_switch_state()
{
    return 0;
#if 0
    int state = 0;
    int fd = -1;
    char  bcmd[16] = {0};
    fd = open(HDMI_SWITCH_STATE_PATH, O_RDONLY);
    if (fd >= 0) {
        read(fd, bcmd, sizeof(bcmd));
        state = strtol(bcmd, NULL, 10);
        close(fd);
    } else {
        adec_print("unable to open file %s,err: %s", HDMI_SWITCH_STATE_PATH, strerror(errno));
    }
    return state;
#endif
}
#ifdef CODE_CALC_VOLUME
// save the last volume  in case get vol failure, which cause the volume value large gap
static float last_vol = 1.0;
static float get_android_stream_volume()
{
    float vol = last_vol;
#if defined(ANDROID_VERSION_JBMR2_UP)
    unsigned int sr = 0;
#else
    int sr = 0;
#endif
#if ANDROID_PLATFORM_SDK_VERSION >= 21
    audio_stream_type_t media_type = AUDIO_STREAM_SYSTEM;
#else
    audio_stream_type_t media_type = AUDIO_STREAM_MUSIC;
#endif

    AudioSystem::getOutputSamplingRate(&sr, AUDIO_STREAM_MUSIC);
    if (sr > 0) {
        audio_io_handle_t handle = -1;
        /*handle =  AudioSystem::getOutput(AUDIO_STREAM_MUSIC,
                                    48000,
                                    AUDIO_FORMAT_PCM_16_BIT,
                                    AUDIO_CHANNEL_OUT_STEREO,
        #if defined(_VERSION_ICS)
                            AUDIO_POLICY_OUTPUT_FLAG_INDIRECT
        #else   //JB...
                            AUDIO_OUTPUT_FLAG_PRIMARY
        #endif
        );*/
        if (handle > 0) {
            if (AudioSystem::getStreamVolume(media_type, &vol, handle) ==  NO_ERROR) {
                last_vol = vol;
                //  adec_print("stream volume %f \n",vol);
            } else {
                adec_print("get stream volume failed\n");
            }
        } else {
            adec_print("get output handle failed\n");
        }
    }
    return vol;

}
static void apply_stream_volume(float vol, char *buf, int size)
{
    int i;
    short *sample = (short*)buf;
    for (i = 0; i < (int)(size / sizeof(short)); i++) {
        sample[i] = vol * sample[i];
    }
}
#endif
static int  get_aml_card()
{
    int card = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        adec_print("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }

    read_buf = (char *)malloc(fileSize);
    if (!read_buf) {
        adec_print("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        adec_print("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        free(read_buf);
        close(fd);
        return -EINVAL;
    }
    pd = strstr(read_buf, "AML");
    card = *(pd - 3) - '0';

    //OUT:
    free(read_buf);
    close(fd);
    return card;
}
static int get_spdif_port()
{
    return 0;
}

EXTERN_TAG int  pcm_output_init(int sr, int ch)
{
    int card = 0;
    int device = 2;
    cached_len = 0;
    tv_mode = getprop_bool("ro.platform.has.tvuimode");
    wfd_config_out.channels = 2;
    wfd_config_out.rate = 48000;
    wfd_config_out.period_size = WFD_PERIOD_SIZE;
    wfd_config_out.period_count = WFD_PERIOD_NUM;
    wfd_config_out.format = PCM_FORMAT_S16_LE;
    wfd_config_out.start_threshold = WFD_PERIOD_SIZE;
    wfd_config_out.avail_min = 0;//SHORT_PERIOD_SIZE;
    card = get_aml_card();
    if (card  < 0) {
        card = 0;
        adec_print("get aml card fail, use default \n");
    }
    // if hdmi state on
    if (get_hdmi_switch_state()) {
        device = get_spdif_port();
    } else {
        device = 0;    //i2s output for analog output
    }
    if (device < 0) {
        device = 0;
        adec_print("get aml card device fail, use default \n");
    }
    device = 0;
    adec_print("open output device card %d, device %d \n", card, device);
    if (sr < 32000 || sr > 48000 || ch != 2) {
        adec_print("wfd output: not right parameter sr %d,ch %d \n", sr, ch);
        return -1;
    }
    wfd_config_out.rate = sr;
    wfd_config_out.channels = ch;
    if (tv_mode) {
        wfd_config_out.channels = 8;
        wfd_config_out.format = PCM_FORMAT_S32_LE;
        wfd_config_out.period_size = WFD_PERIOD_SIZE;
        wfd_config_out.period_count = WFD_PERIOD_NUM;
        wfd_config_out.start_threshold = WFD_PERIOD_SIZE;
    }
    wfd_pcm = pcm_open(card, device, PCM_OUT /*| PCM_MMAP | PCM_NOIRQ*/, &wfd_config_out);
    if (!pcm_is_ready(wfd_pcm)) {
        adec_print("wfd cannot open pcm_out driver: %s", pcm_get_error(wfd_pcm));
        pcm_close(wfd_pcm);
        return -1;
    }
    adec_print("pcm_output_init done  wfd : %p,\n", wfd_pcm);
    return 0;

}

EXTERN_TAG int  pcm_output_write(char *buf, unsigned size)
{

    int ret = 0;
    char *data,  *data_dst;
    char *data_src;
    char outbuf[8192];
    int total_len, ouput_len;
#ifdef CODE_CALC_VOLUME
    float vol = get_android_stream_volume();
    apply_stream_volume(vol, buf, size);
#endif
    if (size < 64) {
        return 0;
    }
    if (size > sizeof(outbuf)) {
        adec_print("write size tooo big %d \n", size);
    }
    total_len = size + cached_len;

    //adec_print("total_len(%d) =  + cached_len111(%d)", size, cached_len);

    data_src = (char *)cache_buffer_bytes;
    data_dst = (char *)outbuf;



    /*write_back data from cached_buffer*/
    if (cached_len) {
        memcpy((void *)data_dst, (void *)data_src, cached_len);
        data_dst += cached_len;
    }
    ouput_len = total_len & (~0x3f);
    data = (char*)buf;

    memcpy((void *)data_dst, (void *)data, ouput_len - cached_len);
    data += (ouput_len - cached_len);
    cached_len = total_len & 0x3f;
    data_src = (char *)cache_buffer_bytes;

    /*save data to cached_buffer*/
    if (cached_len) {
        memcpy((void *)data_src, (void *)data, cached_len);
    }
    char *write_buf = outbuf;
    int *tmp_buffer = NULL;
    if (tv_mode) {
        tmp_buffer = (int*)malloc(ouput_len * 8);
        if (tmp_buffer == NULL) {
            ALOGE("malloc tmp_buffer failed\n");
            return -1;
        }
        int i;
        int out_frames = ouput_len / 4;
        short  *in_buffer = (short*)outbuf;
        for (i = 0; i < out_frames; i ++) {
            tmp_buffer[8 * i] = ((int)(in_buffer[2 * i])) << 16;
            tmp_buffer[8 * i + 1] = ((int)(in_buffer[2 * i + 1])) << 16;
            tmp_buffer[8 * i + 2] = ((int)(in_buffer[2 * i])) << 16;
            tmp_buffer[8 * i + 3] = ((int)(in_buffer[2 * i + 1])) << 16;
            tmp_buffer[8 * i + 4] = 0;
            tmp_buffer[8 * i + 5] = 0;
            tmp_buffer[8 * i + 6] = 0;
            tmp_buffer[8 * i + 7] = 0;
        }
        write_buf = (char*)tmp_buffer;
        ouput_len = ouput_len * 8;
    }
    ret = pcm_write(wfd_pcm, write_buf, ouput_len);
    if (ret < 0) {
        adec_print("pcm_output_write failed ? \n");
    }
    if (tmp_buffer) {
        free(tmp_buffer);
    }
    //adec_print("write size %d ,ret %d \n",size,ret);
    return ret;
}
EXTERN_TAG int  pcm_output_uninit()
{
    if (wfd_pcm) {
        pcm_close(wfd_pcm);
    }
    wfd_pcm = NULL;
    adec_print("pcm_output_uninit done \n");
    return 0;
}
EXTERN_TAG int  pcm_output_latency()
{
#if 1
    struct timespec tstamp;
    unsigned int  avail = 0;
    int ret;
    ret = pcm_get_htimestamp(wfd_pcm, &avail, &tstamp);
    //adec_print("pcm_get_latency ret %d,latency %d \n",ret,avail*1000/48000);
    if (ret) {
        return ret;
    } else {
        return avail * 1000 / wfd_config_out.rate;
    }
#else
    return pcm_hw_lantency(wfd_pcm);
#endif
}
#ifdef CODE_CALC_VOLUME
}
#endif
