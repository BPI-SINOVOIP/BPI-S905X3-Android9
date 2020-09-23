#define LOG_TAG "vadwake"

#include <tinyalsa/asoundlib.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <string.h>
#include <log/log.h>
#include <unistd.h>
#include <pthread.h>


#define VAD_ENABLE "VAD enable"
#define VAD_SWITCH "VAD Switch"
#define VALUE_ENABLE "1"
#define VALUE_DISABLE "0"
#define VAD_CARD 0
#define VAD_DEVICE 3

static int vad_capturing = 0;
static int vad_support = 0;
static pthread_t vad_thread;

static bool tinymix_get_bool(struct mixer *mixer, const char *control)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    //unsigned int num_values;
    int ret;

    ctl = mixer_get_ctl_by_name(mixer, control);
    if (!ctl) {
        ALOGE("Invalid mixer control: %s\n", control);
        return -1;
    }

    type = mixer_ctl_get_type(ctl);
    //num_values = mixer_ctl_get_num_values(ctl);
    if (type != MIXER_CTL_TYPE_BOOL) {
        ALOGE("mixer control type is not bool: %d\n", type);
        return -1;
    }

    return mixer_ctl_get_value(ctl, 0);
}

static int tinymix_set_value(struct mixer *mixer, const char *control, const char *values)
{
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    unsigned int i;

    ctl = mixer_get_ctl_by_name(mixer, control);

    if (!ctl) {
        ALOGE("Invalid mixer control: %s\n", control);
        return -1;
    }

    type = mixer_ctl_get_type(ctl);
    num_ctl_values = mixer_ctl_get_num_values(ctl);

    if (type == MIXER_CTL_TYPE_BYTE) {
        ALOGE("mixer control type: MIXER_CTL_TYPE_BYTE\n");
        //tinymix_set_byte_ctl(ctl, values, num_values);
        return -1;
    }

    int value = atoi(values);

    for (i = 0; i < num_ctl_values; i++) {
        if (mixer_ctl_set_value(ctl, i, value)) {
            ALOGE("Error: invalid value\n");
            return -1;
        }
    }
    return 0;
}

static void* vadwake_func(void* arg __attribute__((unused))) {
    struct pcm_config config;
    struct mixer *mixer;
    struct pcm *pcm;
    char *buffer;
    unsigned int size;
    bool vadswitchflag = true;

    memset(&config, 0, sizeof(config));
    config.channels = 1;
    config.rate = 16000;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(VAD_CARD, VAD_DEVICE, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return NULL;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        ALOGE("Unable to allocate %u bytes\n", size);
        free(buffer);
        pcm_close(pcm);
        return NULL;
    }

    ALOGD("Capturing sample: %u ch, %u hz, %u bit\n", config.channels, config.rate,
           pcm_format_to_bits(config.format));
    while (true) {
        pcm_read(pcm, buffer, size);
        //usleep(50000);
        if (!vad_capturing)
            break;
        if (vadswitchflag) {
            vadswitchflag = false;
            mixer = mixer_open(VAD_CARD);
            if (!mixer)
                ALOGE("Failed to open mixer\n");
            else {
                tinymix_set_value(mixer, VAD_SWITCH, VALUE_ENABLE);
                mixer_close(mixer);
            }
        }
    }
    ALOGD("Capturing sample: finish: %d", vad_capturing);

    free(buffer);
    pcm_close(pcm);
    return NULL;
}

bool vadwake_support() {
    char val[PROPERTY_VALUE_MAX];
    int ret = property_get("ro.vendor.platform.vadwake", val, "0");
    if (!strncmp(val, "1", 1) || !strncmp(val, "true", 4))
        return true;

    ALOGD("vadwake_support: false\n");
    return false;
}

int vadwake_enable(int enable) {
    int ret = 0;
    struct mixer *mixer = mixer_open(VAD_CARD);

    ALOGD("vadwake_enable: %d\n", enable);
    if (!mixer) {
        ALOGE("Failed to open mixer\n");
        return -1;
    }

    if (enable) {
        if (!tinymix_get_bool(mixer, VAD_ENABLE))
			tinymix_set_value(mixer, VAD_ENABLE, VALUE_ENABLE);
        if (vad_capturing)
            return 0;
        vad_capturing = 1;
        ret = pthread_create(&vad_thread, NULL, vadwake_func, NULL);
        if (ret) {
            ALOGE("vadwake error creating thread: %s\n", strerror(ret));
            vad_capturing = 0;
            return -1;
        }
    } else {
        tinymix_set_value(mixer, VAD_SWITCH, VALUE_DISABLE);
        usleep(50*1000);
        vad_capturing = 0;
    }
    mixer_close(mixer);
    return 0;
}

