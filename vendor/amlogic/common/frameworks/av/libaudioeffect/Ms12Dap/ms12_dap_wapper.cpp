/*
 * Copyright (C) 2018 Amlogic Corporation.
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
 *  DESCRIPTION:
 *      This file implements a special EQ  from Amlogic.
 *
 */

#define LOG_TAG "MS12DAP_Effect"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <hardware/audio_effect.h>
#include <cutils/properties.h>
#include <unistd.h>

#include "IniParser.h"
#include "dolby_audio_processing_control.h"
#include "ms12_dap_wapper.h"
#include <utils/CallStack.h>

extern "C" {
#include "../Utility/AudioFade.h"

#define LOG_NDEBUG_FUNCTION
#ifdef LOG_NDEBUG_FUNCTION
#define LOGFUNC(...) ((void)0)
#else
#define LOGFUNC(...) (ALOGD(__VA_ARGS__))
#endif

    //#define DEFAULT_INI_FILE_PATH "/tvconfig/audio/amlogic_audio_effect_default.ini"
#define MODEL_SUM_DEFAULT_PATH "/vendor/etc/tvconfig/model/model_sum.ini"
#define AUDIO_EFFECT_DEFAULT_PATH "/vendor/etc/tvconfig/audio/AMLOGIC_AUDIO_EFFECT_DEFAULT.ini"

#if defined(__LP64__)
#define LIBDAP_PATH_A "/system/lib64/soundfx/libms12dap.so"
#define LIBDAP_PATH_B "/vendor/lib64/libms12dap.so"
#else
#define LIBDAP_PATH_B "/system/lib/soundfx/libms12dap.so"
#define LIBDAP_PATH_A "/vendor/lib/libms12dap.so"
#endif

    extern const struct effect_interface_s DAPInterface;

    // DAP effect TYPE: 3337b21d-c8e6-4bbd-8f24-698ade8491b9
    // DAP effect UUID: 86cafba6-3ff3-485d-b8df-0de96b34b272
    const effect_descriptor_t DAPDescriptor = {
        {0x3337b21d, 0xc8e6, 0x4bbd, 0x8f24, {0x69, 0x8a, 0xde, 0x84, 0x91, 0xb9}}, // type
        {0x86cafba6, 0x3ff3, 0x485d, 0xb8df, {0x0d, 0xe9, 0x6b, 0x34, 0xb2, 0x72}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        EFFECT_FLAG_TYPE_POST_PROC | EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_NO_PROCESS | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        //EFFECT_FLAG_DEVICE_IND | EFFECT_FLAG_OFFLOAD_SUPPORTED,
        DAP_CUP_LOAD_ARM9E,
        DAP_MEM_USAGE,
        "MS12 DAP",
        "Dobly Labs",
    };

#define DAP_RET_SUCESS 0
#define DAP_RET_FAIL -1
#define DAP_GEQ_NB_BANDS_DEFAULT 5
#define DAP_INPUT_STORE_SIZE 8192 // 2048 samples * 2ch * 2 bytes
#define DEFAULT_POSTGAIN     0

    enum DAP_state_e {
        DAP_STATE_UNINITIALIZED,
        DAP_STATE_INITIALIZED,
        DAP_STATE_ACTIVE,
    };

    typedef enum {
        DAP_MODE_DISABLE = 0,
        DAP_MODE_MOVIE,
        DAP_MODE_MUSIC,
        DAP_MODE_NIGHT,
        DAP_MODE_CUSTOM,
        DAP_MODE_MAX,
    } DAPmode;

    typedef enum {
        GEQ_MODE_DISABLE = 0,
        GEQ_MODE_DEFAULT,
        GEQ_MODE_OPEN,
        GEQ_MODE_RICH,
        GEQ_MODE_FOCUSED,
        GEQ_MODE_USER,
        GEQ_MODE_MAX,
    } geq_mode;

    typedef enum {
        DAP_PARAM_ENABLE = 0, // must equal to 0, because this must match with JAVA define
        DAP_PARAM_EFFECT_MODE, // must equal to 1, because this must match with JAVA define
        DAP_PARAM_GEQ_GAINS, // must equal to 2, because this must match with JAVA define
        DAP_PARAM_GEQ_ENABLE,
        DAP_PARAM_POST_GAIN,
        DAP_PARAM_VOL_LEVELER_ENABLE,
        DAP_PARAM_VOL_LEVELER_AMOUNT,
        DAP_PARAM_DE_ENABLE,
        DAP_PARAM_DE_AMOUNT,
        DAP_PARAM_SUROUND_ENABLE,
        DAP_PARAM_SURROUND_BOOST,
        DAP_PARAM_VIRTUALIZER_ENABLE
    } DAPparams;

    typedef struct DAPapi_s {
        int (*DAP_get_chip_ms12_license)(void);
        unsigned(*DAP_cpdp_get_latency)(void *);
        int (*DAP_cpdp_pvt_device_processing_supported)(int);
        int (*DAP_cpdp_pvt_dual_virtualizer_supported)(int);
        int (*DAP_cpdp_pvt_dual_stereo_enabled)(unsigned , int);
        unsigned(*DAP_cpdp_pvt_post_upmixer_channels)(const void *);
        unsigned(*DAP_cpdp_pvt_content_sidechain_channels)(const void *);
        int (*DAP_cpdp_pvt_surround_compressor_enabled)(const void *);
        int (*DAP_cpdp_pvt_content_sidechain_enabled)(const void *);
        int (*DAP_cpdp_pvt_ngcs_enabled)(const void *);
        int (*DAP_cpdp_pvt_audio_optimizer_enabled)(const void *);
        size_t (*DAP_cpdp_query_memory)(const dap_cpdp_init_info *);
        size_t (*DAP_cpdp_query_scratch)(const dap_cpdp_init_info *);
        void    *(*DAP_cpdp_init)(const dap_cpdp_init_info *, void *);
        void (*DAP_cpdp_shutdown)(void *);
        /* This function will be called from the same thread as the process function.
         * It must never block. */
        unsigned(*DAP_cpdp_prepare)(void *, const dlb_buffer *, const dap_cpdp_metadata *, const dap_cpdp_mix_data *);

        dap_cpdp_metadata(*dap_cpdp_process_api)(void *, const dlb_buffer *, void *, int *);
        /* Output channel count */
        void (*DAP_cpdp_output_mode_set)(void *, int);
        /*Dialog Enhancer enable setting */
        void (*DAP_cpdp_de_enable_set)(void *, int);
        /* Dialog Enhancer amount setting */
        void (*DAP_cpdp_de_amount_set)(void *, int);
        /* Dialog Enhancement ducking setting */
        void (*DAP_cpdp_de_ducking_set)(void *, int);
        /* Surround boost setting */
        void (*DAP_cpdp_surround_boost_set)(void *, int);
        /* Surround Decoder enable setting */
        void (*DAP_cpdp_surround_decoder_enable_set)(void *, int);
        /* Graphic Equalizer enable setting */
        void (*DAP_cpdp_graphic_equalizer_enable_set)(void *, int);
        /* Graphic Equalizer bands setting */
        void (*DAP_cpdp_graphic_equalizer_bands_set)(void *, unsigned int, const unsigned int *, const int *);
        /* Audio optimizer enable setting */
        void (*DAP_cpdp_audio_optimizer_enable_set)(void *, int);
        /* Audio optimizer bands setting */
        void (*DAP_cpdp_audio_optimizer_bands_set)(void *, unsigned int, const unsigned int *, int **);
        /* Bass enhancer enable setting */
        void (*DAP_cpdp_bass_enhancer_enable_set)(void *, int);
        /* Bass enhancer boost setting */
        void (*DAP_cpdp_bass_enhancer_boost_set)(void *, int);
        /* Bass enhancer cutoff frequency setting */
        void (*DAP_cpdp_bass_enhancer_cutoff_frequency_set)(void *, unsigned);
        /* Bass enhancer width setting */
        void (*DAP_cpdp_bass_enhancer_width_set)(void *, int);
        /* Visualizer bands get */
        void (*DAP_cpdp_vis_bands_get)(void *, unsigned int *, unsigned int *, int *, int *);
        /* Visualizer custom bands get */
        void (*DAP_cpdp_vis_custom_bands_get)(void *, unsigned int, const unsigned int *, int *, int *);
        /* Audio Regulator Overdrive setting */
        void (*DAP_cpdp_regulator_overdrive_set)(void *, int);
        /* Audio Regulator Timbre Preservation setting */
        void (*DAP_cpdp_regulator_timbre_preservation_set)(void *, int);
        /* Audio Regulator Distortion Relaxation Amount setting */
        void (*DAP_cpdp_regulator_relaxation_amount_set)(void *, int);
        /* Audio Regulator Distortion Operating Mode setting */
        void (*DAP_cpdp_regulator_speaker_distortion_enable_set)(void *, int);
        /* Audio Regulator Enable setting */
        void (*DAP_cpdp_regulator_enable_set)(void *, int);
        /* Audio Regulator tuning setting */
        void (*DAP_cpdp_regulator_tuning_set)(void *, unsigned, const unsigned *, const int *, const int *, const int *);
        /* Audio Regulator tuning info get */
        void (*DAP_cpdp_regulator_tuning_info_get)(void *, unsigned int *, int *, int *);
        /* Virtual Bass mode setting */
        void (*DAP_cpdp_virtual_bass_mode_set)(void *, int);

        /* Virtual Bass source frequency boundaries setting */
        void (*DAP_cpdp_virtual_bass_src_freqs_set)(void *, unsigned, unsigned);
        void (*DAP_cpdp_virtual_bass_overall_gain_set)(void *, int);
        void (*DAP_cpdp_virtual_bass_slope_gain_set)(void *, int);
        void (*DAP_cpdp_virtual_bass_subgains_set)(void *, unsigned, int *);
        void (*DAP_cpdp_virtual_bass_mix_freqs_set)(void *, unsigned, unsigned);

        int (*DAP_cpdp_ieq_bands_set)(void *, unsigned int, const unsigned int *, const int *);
        int (*DAP_cpdp_ieq_enable_set)(void *, int);
        int (*DAP_cpdp_volume_leveler_enable_set)(void *, int);
        int (*DAP_cpdp_volume_modeler_enable_set)(void *, int);
        int (*DAP_cpdp_virtualizer_headphone_reverb_gain_set)(void *, int);
        /* system gain setting */
        void (*DAP_cpdp_system_gain_set)(void *, int);
        /*---------------------------------------------------------------------------------*/
        /*DAP_CPDP_DV_PARAMS*/
        /*---------------------------------------------------------------------------------*/
        size_t (*DAP_cpdp_pvt_dv_params_query_memory)(void);
        void (*DAP_cpdp_pvt_dv_params_init)(void *, const unsigned*, unsigned, void *);
        void (*DAP_cpdp_pvt_volume_and_ieq_update_control)(void *, unsigned);
        int (*DAP_cpdp_pvt_dv_enabled)(const void *);
        int (*DAP_cpdp_pvt_async_dv_enabled)(const void *);
        int (*DAP_cpdp_pvt_async_leveler_or_modeler_enabled)(const void *);
        void (*DAP_cpdp_volume_leveler_amount_set)(void *, int);
        void (*DAP_cpdp_ieq_amount_set)(void *, int);

        void (*DAP_cpdp_virtualizer_speaker_angle_set)(void *, unsigned int);
        void (*DAP_cpdp_virtualizer_speaker_start_freq_set)(void *, unsigned int);
        void (*DAP_cpdp_mi2ieq_steering_enable_set)(void *, int);
        void (*DAP_cpdp_mi2dv_leveler_steering_enable_set)(void *, int);
        void (*DAP_cpdp_mi2dialog_enhancer_steering_enable_set)(void *, int);
        void (*DAP_cpdp_mi2surround_compressor_steering_enable_set)(void *, int);
        void (*DAP_cpdp_calibration_boost_set)(void *, int);
        void (*DAP_cpdp_volume_leveler_in_target_set)(void *, int);
        void (*DAP_cpdp_volume_leveler_out_target_set)(void *, int);
        void (*DAP_cpdp_volume_modeler_calibration_set)(void *, int);
        void (*DAP_cpdp_pregain_set)(void *, int);
        void (*DAP_cpdp_postgain_set)(void *, int);
        void (*DAP_cpdp_volmax_boost_set)(void   *, int);
    } DAPapi;

    typedef struct dolby_base_t {
        int pregain;
        int postgain;
        int systemgain;

        int headphone_reverb; // dap_cpdp_virtualizer_headphone_reverb_gain_set
        int speaker_angle; // dap_cpdp_virtualizer_speaker_angle_set
        int speaker_start; // dap_cpdp_virtualizer_speaker_start_freq_set
        int mi_ieq_enable; // dap_cpdp_mi2ieq_steering_enable_set
        int mi_dv_enable; // dap_cpdp_mi2dv_leveler_steering_enable_set
        int mi_de_enable; // dap_cpdp_mi2dialog_enhancer_steering_enable_set
        int mi_surround_enable; // dap_cpdp_mi2surround_compressor_steering_enable_set

        int calibration_boost; // dap_cpdp_calibration_boost_set
        int leveler_input; // dap_cpdp_volume_leveler_in_target_set
        int leveler_output; // dap_cpdp_volume_leveler_out_target_set
        int modeler_enable; // dap_cpdp_volume_modeler_enable_set
        int modeler_calibration; // dap_cpdp_volume_modeler_calibration_set

        int ieq_enable; // dap_cpdp_ieq_enable_set
        int ieq_amount; // dap_cpdp_ieq_amount_set
        int ieq_nb_bands; // dap_cpdp_ieq_bands_set
        int a_ieq_band_center[DAP_IEQ_MAX_BANDS];
        int a_ieq_band_target[DAP_IEQ_MAX_BANDS];
        int de_ducking; // dap_cpdp_de_ducking_set
        int volmax_boost; // dap_cpdp_volmax_boost_set

        int optimizer_enable; // dap_cpdp_audio_optimizer_enable_set
        int ao_bands; // dap_cpdp_audio_optimizer_bands_set
        int ao_band_center_freq[DAP_OPT_MAX_BANDS];
        int ao_band_gains[DAP_MAX_CHANNELS][DAP_OPT_MAX_BANDS];
        int bass_enable; // dap_cpdp_bass_enhancer_enable_set
        int bass_boost; // dap_cpdp_bass_enhancer_boost_set
        int bass_cutoff; // dap_cpdp_bass_enhancer_cutoff_frequency_set
        int bass_width; // dap_cpdp_bass_enhancer_width_set

        int ar_bands; // dap_cpdp_regulator_tuning_set
        int ar_band_center_freq[DAP_REG_MAX_BANDS];
        int ar_low_thresholds[DAP_REG_MAX_BANDS];
        int ar_high_thresholds[DAP_REG_MAX_BANDS];
        int ar_isolated_bands[DAP_REG_MAX_BANDS];
        int regulator_overdrive; // dap_cpdp_regulator_overdrive_set
        int regulator_timbre; // dap_cpdp_regulator_timbre_preservation_set
        int regulator_distortion; // dap_cpdp_regulator_relaxation_amount_set
        int regulator_mode; // dap_cpdp_regulator_speaker_distortion_enable_set
        int regulator_enable; // dap_cpdp_regulator_enable_set

        int virtual_bass_mode;
        int virtual_bass_low_src_freq;
        int virtual_bass_high_src_freq;
        int virtual_bass_overall_gain;
        int virtual_bass_slope_gain;
        int virtual_bass_subgain[3];
        int virtual_bass_mix_low_freq;
        int virtual_bass_mix_high_freq;
    } dolby_base;

    typedef struct dolby_virtual_enable {
        int surround_decoder_enable;
        int virtualizer_enable;
    } dolby_virtual_enable_t;

    typedef struct dolby_virtual_surround {
        dolby_virtual_enable_t enable;
        int surround_boost;
    } dolby_virtual_surround_t;

    typedef struct dolby_dialog_enhance {
        int de_enable;
        int de_amount;
    } dolby_dialog_enhance_t;

    typedef struct dolby_vol_leveler {
        int vl_enable;
        int vl_amount;
    } dolby_vol_leveler_t;


    typedef struct dolby_eq_enable {
        int geq_enable;
    } dolby_eq_enable_t;

    typedef struct dolby_eq_params {
        int geq_nb_bands;      //5
        int a_geq_band_center[5];//0-20000
        int a_geq_band_target[5];//-10~10dB
    } dolby_eq_params_t;

    typedef struct dolby_eq {
        dolby_eq_enable_t eq_enable;
        dolby_eq_params_t eq_params;
    } dolby_eq_t;

    typedef struct DAPdata_s {

        /* DAP API needed data */
        dap_cpdp_init_info init_info;
        dolby_base dapBase;
        void *pPersistMem;
        size_t uPersistMemSize;
        void *pScratchMem;
        size_t uScratchSize;

        void *dap_cpdp;
        // audioInTmp16 for DLB buffer usage for dap process
        short audioInTmp16[DAP_CPDP_MAX_NUM_CHANNELS][DAP_CPDP_PCM_SAMPLES_PER_BLOCK];
        // should copy input data into "inStorgeBuf" for legacy reason
        void *inStorgeBuf;
        unsigned int inStorgeBufSize;
        unsigned int curInfrmCnts;
        unsigned long totalFrmCnts;
        unsigned int islicenseOK;
        unsigned int is_passthrough;

        /* DAP configuration */
        unsigned int bDapCPDPInited;
        unsigned int bDapEnabled;
        unsigned int dapCPDPOutputMode; // DAP_CPDP_OUTPUT_2_SPEAKER
        unsigned int dapPostGain;
        unsigned int bNeedReset;
        //unsigned int bVolLevelerEnabled;
        DAPmode eDapEffectMode;
        dolby_virtual_surround_t dapVirtualSrnd;
        dolby_dialog_enhance_t   dapDialogEnhance;
        dolby_vol_leveler_t      dapVolLeveler;
        dolby_eq_t               dapEQ;
        dolby_base               dapBaseSetting;

    } DAPdata;


    typedef struct DAPContext_s {
        const struct effect_interface_s *itfe;
        effect_config_t                 config;
        DAP_state_e                     state;
        void                            *gDAPLibHandler;
        DAPapi                          gDAPapi;
        DAPdata                         gDAPdata;

        int                             bUseFade; // when recieve setting change from app, "fade audio out->do setting->fade audio In"
        AudioFade_t                     gAudFade;
        int32_t                         modeValue;
    } DAPContext;


    typedef struct DAPcfg_8bit_s {
        signed char band1;
        signed char band2;
        signed char band3;
        signed char band4;
        signed char band5;
    } DAPcfg_8bit;

    const char *DapEnableStr[] = {"Disable", "Enable"};
    const char *DapEffectModeStr[] = {"Disable", "Movie", "Music", "Night", "Custom"};

    const int default_surr_param[DAP_MODE_MAX][3] = {{0, DAP_CPDP_OUTPUT_2_SPEAKER, 0},
                                                     {1, DAP_CPDP_OUTPUT_2_SPEAKER, 2},
                                                     {0, DAP_CPDP_OUTPUT_2_SPEAKER, 0},
                                                     {0, DAP_CPDP_OUTPUT_2_SPEAKER, 0},
                                                     {1, DAP_CPDP_OUTPUT_2_SPEAKER, 0}};

    const int default_de_param[DAP_MODE_MAX][2] = {{0, 0},
                                                   {1, 5},
                                                   {1, 0},
                                                   {1, 8},
                                                   {0, 0}};

    const int default_vl_param[DAP_MODE_MAX][2] = {{0, 0},
                                                   {1, 4},
                                                   {1, 0},
                                                   {1, 8},
                                                   {1, 0}};

    const int default_geq_enable[DAP_MODE_MAX] = {GEQ_MODE_DISABLE,
                                                  GEQ_MODE_DISABLE,
                                                  GEQ_MODE_DEFAULT,
                                                  GEQ_MODE_DEFAULT,
                                                  GEQ_MODE_USER};

    const int default_geq_center[DAP_GEQ_NB_BANDS_DEFAULT] = { 100, 300, 1000, 3000, 10000};
    const int default_geq_gains[GEQ_MODE_MAX][5] = {{0, 0, 0, 0, 0},
                                                    {0, 0, 0, 0, 0},
                                                    {4, -2, 0, 0, 4},
                                                    {6, 2, 2, 1, 1},
                                                    {-5, -2, 2, 1, 3},
                                                    {0, 0, 0, 0, 0}};

    const dolby_base dap_dolby_base_movie = {
        .pregain = 0,
        .postgain = 0,
        .systemgain = 0,
        .headphone_reverb = 0,
        .speaker_angle = 5,
        .speaker_start = 200,
        .mi_ieq_enable = 0,
        .mi_dv_enable = 0,
        .mi_de_enable = 0,
        .mi_surround_enable = 0,
        .calibration_boost = 0,
        .leveler_input = -384,
        .leveler_output = -384,
        .modeler_enable = 0,
        .modeler_calibration = 0,
        .ieq_enable = 0,
        .ieq_amount = 10,
        .ieq_nb_bands = 20,
        .a_ieq_band_center = {
            65, 136, 223, 332, 467, 634, 841, 1098, 1416, 1812,
            2302, 2909, 3663, 4598, 5756, 7194, 8976, 11186, 13927, 17326
        },
        .a_ieq_band_target = {
            117, 133, 188, 176, 141, 149, 175, 185, 185, 200,
            236, 242, 228, 213, 182, 132, 110, 68, -27, -240
        },
        .de_ducking = 0,
        .volmax_boost = 0,
        .optimizer_enable = 1,
        .ao_bands = 20,
        .ao_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ao_band_gains = {
            {
                0, 0, 70, -50, 75, 87, 37, 12, -30, -33,
                109, 175, -9, -43, 20, 72, 80, 112, 78, 81
            },
            {
                0, 0, 59, -50, 75, 87, 23, 12, -30, -33,
                109, 175, -9, -64, -4, 37, 80, 112, 78, 81
            },
            {
                0, 0, 0, -50, 75, 87, 0, 12, -30, -33,
                109, 175, -9, 0, 0, 0, 80, 112, 78, 81
            },
            {
                0, 0, 0, -50, 75, 87, 0, 12, -30, -33,
                109, 175, -9, 0, 0, 0, 80, 112, 78, 81
            },
            {
                0, 0, 0, -50, 75, 87, 0, 12, -30, -33,
                109, 175, -9, 0, 0, 0, 80, 112, 78, 81
            },
            {
                0, 0, 0, -50, 75, 87, 0, 12, -30, -33,
                109, 175, -9, 0, 0, 0, 80, 112, 78, 81
            },
        },
        .bass_enable = 0,
        .bass_boost = 111,
        .bass_cutoff = 209,
        .bass_width = 5,
        .ar_bands = 20,
        .ar_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ar_low_thresholds = {
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192,
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192
        },
        .ar_high_thresholds = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .ar_isolated_bands = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .regulator_overdrive = 0,
        .regulator_timbre = 12,
        .regulator_distortion = 96,
        .regulator_mode = 0,
        .regulator_enable = 0,
        .virtual_bass_mode = 1,
        .virtual_bass_low_src_freq = 90,
        .virtual_bass_high_src_freq = 200,
        .virtual_bass_overall_gain = 0,
        .virtual_bass_slope_gain = 0,
        .virtual_bass_subgain = { -160, -144, -192},
        .virtual_bass_mix_low_freq = 181,
        .virtual_bass_mix_high_freq = 399,
    };

    const dolby_base dap_dolby_base_music = {
        .pregain = 0,
        .postgain = 0,
        .systemgain = 0,
        .headphone_reverb = 0,
        .speaker_angle = 5,
        .speaker_start = 200,
        .mi_ieq_enable = 0,
        .mi_dv_enable = 0,
        .mi_de_enable = 0,
        .mi_surround_enable = 0,
        .calibration_boost = 0,
        .leveler_input = -384,
        .leveler_output = -384,
        .modeler_enable = 0,
        .modeler_calibration = 0,
        .ieq_enable = 1,
        .ieq_amount = 8,
        .ieq_nb_bands = 20,
        .a_ieq_band_center = {
            65, 136, 223, 332, 467, 634, 841, 1098, 1416, 1812,
            2302, 2909, 3663, 4598, 5756, 7194, 8976, 11186, 13927, 17326
        },
        .a_ieq_band_target = {
            117, 133, 188, 176, 141, 149, 175, 185, 185, 200,
            236, 242, 228, 213, 182, 132, 110, 68, -27, -240
        },
        .de_ducking = 0,
        .volmax_boost = 0,
        .optimizer_enable = 1,
        .ao_bands = 20,
        .ao_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ao_band_gains = {
            {
                2, -1, 27, -38, -43, -5, 35, 24, -18, -70,
                //-35, 26, 54, -1, -9, 60, 158, 141, 120, 173
                -35, 26, 54, -1, -9, 97, 110, 96, 97, 93
            },
            {
                2, -1, 10, -43, -12, 25, 12, 10, -16, -60,
                //-27, 28, 53, 21, 70, 187, 192, 192, 189, 173
                -27, 28, 53, 21, 70, 97, 110, 96, 97, 93
            },
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            },
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            },
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            },
        },
        .bass_enable = 0,
        .bass_boost = 87,
        .bass_cutoff = 234,
        .bass_width = 8,
        .ar_bands = 20,
        .ar_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ar_low_thresholds = {
            -769, -365, -278, -208, -208, -192, -192, -283, -192, -192,
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192
        },
        .ar_high_thresholds = {
            -577, -173, -86, -16, -16, 0, 0, -91, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .ar_isolated_bands = {
            1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .regulator_overdrive = 0,
        .regulator_timbre = 12,
        .regulator_distortion = 96,
        .regulator_mode = 1,
        .regulator_enable = 1,
        .virtual_bass_mode = 2,
        .virtual_bass_low_src_freq = 90,
        .virtual_bass_high_src_freq = 234,
        .virtual_bass_overall_gain = -213,
        .virtual_bass_slope_gain = 0,
        .virtual_bass_subgain = { -32, -156, -192},
        .virtual_bass_mix_low_freq = 234,
        .virtual_bass_mix_high_freq = 400,
    };

    dolby_base dap_dolby_base_disable = {
        .pregain = 0,
        .postgain = 0,
        .systemgain = 0,
        .headphone_reverb = 0,
        .speaker_angle = 5,
        .speaker_start = 200,
        .mi_ieq_enable = 0,
        .mi_dv_enable = 0,
        .mi_de_enable = 0,
        .mi_surround_enable = 0,
        .calibration_boost = 0,
        .leveler_input = -384,
        .leveler_output = -384,
        .modeler_enable = 0,
        .modeler_calibration = 0,
        .ieq_enable = 0,
        .ieq_amount = 8,
        .ieq_nb_bands = 20,
        .a_ieq_band_center = {
            65, 136, 223, 332, 467, 634, 841, 1098, 1416, 1812,
            2302, 2909, 3663, 4598, 5756, 7194, 8976, 11186, 13927, 17326
        },
        .a_ieq_band_target = {
            117, 133, 188, 176, 141, 149, 175, 185, 185, 200,
            236, 242, 228, 213, 182, 132, 110, 68, -27, -240
        },
        .de_ducking = 0,
        .volmax_boost = 0,
        .optimizer_enable = 0,
        .ao_bands = 20,
        .ao_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ao_band_gains = {
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
        },
        .bass_enable = 0,
        .bass_boost = 0,
        .bass_cutoff = 0,
        .bass_width = 0,
        .ar_bands = 20,
        .ar_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ar_low_thresholds = {
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192,
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192
        },
        .ar_high_thresholds = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .ar_isolated_bands = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .regulator_overdrive = 0,
        .regulator_timbre = 12,
        .regulator_distortion = 96,
        .regulator_mode = 0,
        .regulator_enable = 0,
        .virtual_bass_mode = 0,
        .virtual_bass_low_src_freq = 35,
        .virtual_bass_high_src_freq = 160,
        .virtual_bass_overall_gain = 0,
        .virtual_bass_slope_gain = 0,
        .virtual_bass_subgain = {0, 0, 0},
        .virtual_bass_mix_low_freq = 0,
        .virtual_bass_mix_high_freq = 0,
    };

    dolby_base dap_dolby_base_night = {
        .pregain = 0,
        .postgain = 0,
        .systemgain = 0,
        .headphone_reverb = 0,
        .speaker_angle = 10,
        .speaker_start = 200,
        .mi_ieq_enable = 1,
        .mi_dv_enable = 1,
        .mi_de_enable = 1,
        .mi_surround_enable = 1,
        .calibration_boost = 0,
        .leveler_input = -384,
        .leveler_output = -384,
        .modeler_enable = 0,
        .modeler_calibration = 0,
        .ieq_enable = 0,
        .ieq_amount = 10,
        .ieq_nb_bands = 20,
        .a_ieq_band_center = {
            65, 136, 223, 332, 467, 634, 841, 1098, 1416, 1812,
            2302, 2909, 3663, 4598, 5756, 7194, 8976, 11186, 13927, 17326
        },
        .a_ieq_band_target = {
            117, 133, 188, 176, 141, 149, 175, 185, 185, 200,
            236, 242, 228, 213, 182, 132, 110, 68, -27, -240
        },
        .de_ducking = 0,
        .volmax_boost = 0,
        .optimizer_enable = 0,
        .ao_bands = 20,
        .ao_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ao_band_gains = {
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
        },
        .bass_enable = 0,
        .bass_boost = 0,
        .bass_cutoff = 0,
        .bass_width = 0,
        .ar_bands = 20,
        .ar_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ar_low_thresholds = {
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192,
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192
        },
        .ar_high_thresholds = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .ar_isolated_bands = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .regulator_overdrive = 0,
        .regulator_timbre = 12,
        .regulator_distortion = 96,
        .regulator_mode = 0,
        .regulator_enable = 0,
        .virtual_bass_mode = 0,
        .virtual_bass_low_src_freq = 35,
        .virtual_bass_high_src_freq = 160,
        .virtual_bass_overall_gain = 0,
        .virtual_bass_slope_gain = 0,
        .virtual_bass_subgain = {0, 0, 0},
        .virtual_bass_mix_low_freq = 0,
        .virtual_bass_mix_high_freq = 0,
    };//done

    // custom data can be modified, no "const" needed. or system will crash.
    dolby_base dap_dolby_base_custom = {
        .pregain = 0,
        .postgain = 0,
        .systemgain = 0,
        .headphone_reverb = 0,
        .speaker_angle = 5,
        .speaker_start = 200,
        .mi_ieq_enable = 0,
        .mi_dv_enable = 0,
        .mi_de_enable = 0,
        .mi_surround_enable = 0,
        .calibration_boost = 0,
        .leveler_input = -384,
        .leveler_output = -384,
        .modeler_enable = 0,
        .modeler_calibration = 0,
        .ieq_enable = 1,
        .ieq_amount = 8,
        .ieq_nb_bands = 20,
        .a_ieq_band_center = {
            65, 136, 223, 332, 467, 634, 841, 1098, 1416, 1812,
            2302, 2909, 3663, 4598, 5756, 7194, 8976, 11186, 13927, 17326
        },
        .a_ieq_band_target = {
            117, 133, 188, 176, 141, 149, 175, 185, 185, 200,
            236, 242, 228, 213, 182, 132, 110, 68, -27, -240
        },
        .de_ducking = 0,
        .volmax_boost = 0,
        .optimizer_enable = 0,
        .ao_bands = 20,
        .ao_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ao_band_gains = {
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
            {
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,  0, 0, 0, 0, 0, 0
            },
        },
        .bass_enable = 0,
        .bass_boost = 0,
        .bass_cutoff = 0,
        .bass_width = 0,
        .ar_bands = 20,
        .ar_band_center_freq = {
            47, 141, 234, 328, 469, 656, 844, 1031, 1313, 1688,
            2250, 3000, 3750, 4688, 5813, 7125, 9000, 11250, 13875, 19688
        },
        .ar_low_thresholds = {
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192,
            -192, -192, -192, -192, -192, -192, -192, -192, -192, -192
        },
        .ar_high_thresholds = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .ar_isolated_bands = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        .regulator_overdrive = 0,
        .regulator_timbre = 12,
        .regulator_distortion = 96,
        .regulator_mode = 0,
        .regulator_enable = 0,
        .virtual_bass_mode = 0,
        .virtual_bass_low_src_freq = 35,
        .virtual_bass_high_src_freq = 160,
        .virtual_bass_overall_gain = 0,
        .virtual_bass_slope_gain = 0,
        .virtual_bass_subgain = {0, 0, 0},
        .virtual_bass_mix_low_freq = 0,
        .virtual_bass_mix_high_freq = 0,
    };//done

    //  int (*DAP_get_chip_ms12_license)(void);
    int aml_get_chip_ms12_license(DAPapi *pDAPapi)
    {
        int ret = 0;

        if (pDAPapi->DAP_get_chip_ms12_license == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_get_chip_ms12_license)();
        return ret;
    }


    //unsigned (*DAP_cpdp_get_latency)(void *);
    int aml_dap_cpdp_get_latency(DAPapi *pDAPapi, void *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_get_latency == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_get_latency)(p_dap_cpdp);
        return ret;
    }

    //int (*DAP_cpdp_pvt_device_processing_supported)(int);
    int aml_dap_cpdp_pvt_device_processing_supported(DAPapi *pDAPapi, int mode)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_device_processing_supported == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_device_processing_supported)(mode);

        return ret;
    }

    //int (*DAP_cpdp_pvt_dual_virtualizer_supported)(int);
    int aml_dap_cpdp_pvt_dual_virtualizer_supported(DAPapi *pDAPapi, int mode)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_dual_virtualizer_supported == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_dual_virtualizer_supported)(mode);

        return ret;
    }

    //int (*DAP_cpdp_pvt_dual_stereo_enabled)(unsigned , int);
    int aml_dap_cpdp_pvt_dual_stereo_enabled(DAPapi *pDAPapi, unsigned output_channel_count, int downmixing_mode)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_dual_stereo_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_dual_stereo_enabled)(output_channel_count, downmixing_mode);

        return ret;
    }

    //unsigned (*DAP_cpdp_pvt_post_upmixer_channels)(const void *);
    int aml_dap_cpdp_pvt_post_upmixer_channels(DAPapi *pDAPapi, const void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_post_upmixer_channels == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_pvt_post_upmixer_channels)(p_dap_cpdp);

        return ret;
    }


    //unsigned (*DAP_cpdp_pvt_content_sidechain_channels)(const void *);
    int aml_dap_cpdp_pvt_content_sidechain_channels(DAPapi *pDAPapi, const void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_content_sidechain_channels == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_pvt_content_sidechain_channels)(p_dap_cpdp);

        return ret;
    }

    //int (*DAP_cpdp_pvt_surround_compressor_enabled)(const void *);
    int aml_dap_cpdp_pvt_surround_compressor_enabled(DAPapi *pDAPapi, const void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_surround_compressor_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_pvt_surround_compressor_enabled)(p_dap_cpdp);

        return ret;
    }

    //int (*DAP_cpdp_pvt_content_sidechain_enabled)(const void *);
    int aml_dap_cpdp_pvt_content_sidechain_enabled(DAPapi *pDAPapi, const void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_content_sidechain_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_pvt_content_sidechain_enabled)(p_dap_cpdp);

        return ret;
    }

    //int (*DAP_cpdp_pvt_ngcs_enabled)(const void *);
    int aml_dap_cpdp_pvt_ngcs_enabled(DAPapi *pDAPapi, const void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_ngcs_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_pvt_ngcs_enabled)(p_dap_cpdp);

        return ret;
    }

    //int (*DAP_cpdp_pvt_audio_optimizer_enabled)(const void *);
    int aml_dap_cpdp_pvt_audio_optimizer_enabled(DAPapi *pDAPapi, const void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_pvt_audio_optimizer_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_pvt_audio_optimizer_enabled)(p_dap_cpdp);

        return ret;
    }

    //size_t (*DAP_cpdp_query_memory)(const dap_cpdp_init_info *);
    size_t aml_dap_cpdp_query_memory(DAPapi *pDAPapi, const dap_cpdp_init_info  *p_info)
    {
        size_t ret = 0;

        if (pDAPapi->DAP_cpdp_query_memory == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_info == NULL) {
            ALOGE("%s, p_info == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_query_memory)(p_info);

        return ret;
    }

    //size_t (*DAP_cpdp_query_scratch)(const dap_cpdp_init_info *);
    size_t aml_dap_cpdp_query_scratch(DAPapi *pDAPapi, const dap_cpdp_init_info  *p_info)
    {
        size_t ret = 0;

        if (pDAPapi->DAP_cpdp_query_scratch == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return 0;
        }
        if (p_info == NULL) {
            ALOGE("%s, p_info == NULL.\n", __FUNCTION__);
            return 0;
        }
        ret = (*pDAPapi->DAP_cpdp_query_scratch)(p_info);

        return ret;
    }

    //void *(*DAP_cpdp_init)(const dap_cpdp_init_info *, void *);
    void *aml_dap_cpdp_init(DAPapi *pDAPapi, const dap_cpdp_init_info *p_info , void *p_mem)
    {
        void *ret = NULL;

        if (pDAPapi->DAP_cpdp_init == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return NULL;
        }
        if (p_info == NULL) {
            ALOGE("%s, p_info == NULL.\n", __FUNCTION__);
            return NULL;
        }
        if (p_mem == NULL) {
            ALOGE("%s, p_mem == NULL.\n", __FUNCTION__);
            return NULL;
        }
        ret = (*pDAPapi->DAP_cpdp_init)(p_info, p_mem);

        return ret;
    }

    //void (*DAP_cpdp_shutdown)(void *);
    int aml_dap_cpdp_shutdown(DAPapi *pDAPapi, void  *p_dap_cpdp)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_shutdown == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        (*pDAPapi->DAP_cpdp_shutdown)(p_dap_cpdp);

        return ret;
    }

    /* This function will be called from the same thread as the process function.
     * It must never block. */
    //unsigned (*DAP_cpdp_prepare)(void *, const dlb_buffer *, const dap_cpdp_metadata *, const dap_cpdp_mix_data *);
    int aml_dap_cpdp_prepare(DAPapi *pDAPapi, void *p_dap_cpdp, const dlb_buffer *p_input, const dap_cpdp_metadata *p_metadata_in, const dap_cpdp_mix_data *p_downmix_data)
    {
        int ret = 0;

        if (pDAPapi->DAP_cpdp_prepare == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        if (p_input == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        ret = (*pDAPapi->DAP_cpdp_prepare)(p_dap_cpdp, p_input, p_metadata_in, p_downmix_data);

        return ret;
    }


    dap_cpdp_metadata aml_dap_cpdp_process_api
    (DAPapi *pDAPapi
     , void *p_dap_cpdp
     , const dlb_buffer *p_output
     , void *scratch
     , int *err
    )
    {
        dap_cpdp_metadata ret;

        if (pDAPapi->dap_cpdp_process_api == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return ret;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return ret;
        }
        if (p_output == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return ret;
        }
        ret = (*pDAPapi->dap_cpdp_process_api)(p_dap_cpdp, p_output, scratch, err);

        return ret;
    }



    /* Output channel count */
    //void (*DAP_cpdp_output_mode_set)(void *, int);
    int aml_dap_cpdp_output_mode_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_output_mode_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        (*pDAPapi->DAP_cpdp_output_mode_set)(p_dap_cpdp, mode);

        return ret;
    }


    /*Dialog Enhancer enable setting */
    //void (*DAP_cpdp_de_enable_set)(void *, int);
    int aml_dap_cpdp_de_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_de_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        (*pDAPapi->DAP_cpdp_de_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Dialog Enhancer amount setting */
    //void (*DAP_cpdp_de_amount_set)(void *, int);
    int aml_dap_cpdp_de_amount_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_de_amount_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        (*pDAPapi->DAP_cpdp_de_amount_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Dialog Enhancement ducking setting */
    //void (*DAP_cpdp_de_ducking_set)(void *, int);
    int aml_dap_cpdp_de_ducking_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_de_ducking_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }
        if (p_dap_cpdp == NULL) {
            ALOGE("%s, p_dap_cpdp == NULL.\n", __FUNCTION__);
            return -1;
        }
        (*pDAPapi->DAP_cpdp_de_ducking_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Surround boost setting */
    //void (*DAP_cpdp_surround_boost_set)(void *, int);
    int aml_dap_cpdp_surround_boost_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_surround_boost_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_surround_boost_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Surround Decoder enable setting */
    //void (*DAP_cpdp_surround_decoder_enable_set)(void *, int);
    int aml_dap_cpdp_surround_decoder_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_surround_decoder_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_surround_decoder_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Graphic Equalizer enable setting */
    //void (*DAP_cpdp_graphic_equalizer_enable_set)(void *, int);
    int aml_dap_cpdp_graphic_equalizer_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_graphic_equalizer_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_graphic_equalizer_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Graphic Equalizer bands setting */
    //void (*DAP_cpdp_graphic_equalizer_bands_set)(void *, unsigned int, const unsigned int *, const int *);
    int aml_dap_cpdp_graphic_equalizer_bands_set(DAPapi *pDAPapi, void *p_dap_cpdp, unsigned int nb_bands, const unsigned int *p_freq, const int *p_gains)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_graphic_equalizer_bands_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_graphic_equalizer_bands_set)(p_dap_cpdp, nb_bands, p_freq, p_gains);

        return ret;
    }


    /* Audio optimizer enable setting */
    //void (*DAP_cpdp_audio_optimizer_enable_set)(void *, int);
    int aml_dap_cpdp_audio_optimizer_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_audio_optimizer_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_audio_optimizer_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Audio optimizer bands setting */
    //void (*DAP_cpdp_audio_optimizer_bands_set)(void *, unsigned int, const unsigned int *, int **);
    int aml_dap_cpdp_audio_optimizer_bands_set(DAPapi *pDAPapi, void *p_dap_cpdp, unsigned int nb_bands, const unsigned int *p_freq, int *const   ap_gains[DAP_CPDP_MAX_NUM_CHANNELS])
    //int aml_dap_cpdp_audio_optimizer_bands_set(DAPapi *pDAPapi,void *p_dap_cpdp, unsigned int nb_bands, const unsigned int *p_freq, int **     ap_gains)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_audio_optimizer_bands_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_audio_optimizer_bands_set)(p_dap_cpdp, nb_bands, p_freq, (int **)ap_gains);

        return ret;
    }

    /* Bass enhancer enable setting */
    //void (*DAP_cpdp_bass_enhancer_enable_set)(void *, int);
    int aml_dap_cpdp_bass_enhancer_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_bass_enhancer_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_bass_enhancer_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Bass enhancer boost setting */
    //void (*DAP_cpdp_bass_enhancer_boost_set)(void *, int);
    int aml_dap_cpdp_bass_enhancer_boost_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_bass_enhancer_boost_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_bass_enhancer_boost_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Bass enhancer cutoff frequency setting */
    //void (*DAP_cpdp_bass_enhancer_cutoff_frequency_set)(void *, unsigned);
    int aml_dap_cpdp_bass_enhancer_cutoff_frequency_set(DAPapi *pDAPapi, void  *p_dap_cpdp, unsigned value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_bass_enhancer_cutoff_frequency_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_bass_enhancer_cutoff_frequency_set)(p_dap_cpdp, value);

        return ret;
    }


    /* Bass enhancer width setting */
    //void (*DAP_cpdp_bass_enhancer_width_set)(void *, int);
    int aml_dap_cpdp_bass_enhancer_width_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_bass_enhancer_width_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_bass_enhancer_width_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Visualizer bands get */
    //void (*DAP_cpdp_vis_bands_get)(void *, unsigned int *, unsigned int *, int *, int *);
    int aml_dap_cpdp_vis_bands_get
    (
        DAPapi *pDAPapi,
        void *p_dap_cpdp,
        unsigned int *p_nb_bands,
        unsigned int *p_band_centers,
        int *p_band_gains,
        int *p_band_excitations
    )
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_vis_bands_get == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_vis_bands_get)(p_dap_cpdp, p_nb_bands, p_band_centers, p_band_gains, p_band_excitations);

        return ret;
    }

    /* Visualizer custom bands get */
    //void (*DAP_cpdp_vis_custom_bands_get)(void *, unsigned int, const unsigned int *, int *, int *);
    int aml_dap_cpdp_vis_custom_bands_get
    (
        DAPapi *pDAPapi,
        void *p_dap_cpdp,
        unsigned int nb_bands,
        const unsigned int *p_band_centers,
        int *p_band_gains,
        int *p_band_excitation
    )
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_vis_custom_bands_get == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_vis_custom_bands_get)(p_dap_cpdp, nb_bands, p_band_centers, p_band_gains, p_band_excitation);

        return ret;
    }

    /* Audio Regulator Overdrive setting */
    //void (*DAP_cpdp_regulator_overdrive_set)(void *, int);
    int aml_dap_cpdp_regulator_overdrive_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_overdrive_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_overdrive_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Audio Regulator Timbre Preservation setting */
    //void (*DAP_cpdp_regulator_timbre_preservation_set)(void *, int);
    int aml_dap_cpdp_regulator_timbre_preservation_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_timbre_preservation_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_timbre_preservation_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Audio Regulator Distortion Relaxation Amount setting */
    //void (*DAP_cpdp_regulator_relaxation_amount_set)(void *, int);
    int aml_dap_cpdp_regulator_relaxation_amount_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_relaxation_amount_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_relaxation_amount_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Audio Regulator Distortion Operating Mode setting */
    //void (*DAP_cpdp_regulator_speaker_distortion_enable_set)(void *, int);
    int aml_dap_cpdp_regulator_speaker_distortion_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_speaker_distortion_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_speaker_distortion_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Audio Regulator Enable setting */
    //void (*DAP_cpdp_regulator_enable_set)(void *, int);
    int aml_dap_cpdp_regulator_enable_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_enable_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Audio Regulator tuning setting */
    //void (*DAP_cpdp_regulator_tuning_set)(void *, unsigned, const unsigned *, const int *, const int *, const int *);
    int aml_dap_cpdp_regulator_tuning_set
    (
        DAPapi *pDAPapi,
        void *p_dap_cpdp,
        unsigned nb_bands,
        const  unsigned *p_band_centers,
        const  int *p_low_thresholds,
        const  int *p_high_thresholds,
        const  int *p_isolated_bands
    )
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_tuning_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_tuning_set)(p_dap_cpdp, nb_bands, p_band_centers, p_low_thresholds, p_high_thresholds, p_isolated_bands);

        return ret;
    }



    /* Audio Regulator tuning info get */
    //void (*DAP_cpdp_regulator_tuning_info_get)(void *, unsigned int *, int *, int *);
    int aml_dap_cpdp_regulator_tuning_info_get(DAPapi *pDAPapi, void *p_dap_cpdp, unsigned int *p_nb_bands, int *p_regulator_gains, int *p_regulator_excitations)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_regulator_tuning_info_get == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_regulator_tuning_info_get)(p_dap_cpdp, p_nb_bands, p_regulator_gains, p_regulator_excitations);

        return ret;
    }


    /* Virtual Bass mode setting */
    //void (*DAP_cpdp_virtual_bass_mode_set)(void *, int);
    int aml_dap_cpdp_virtual_bass_mode_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtual_bass_mode_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtual_bass_mode_set)(p_dap_cpdp, mode);

        return ret;
    }

    /* Virtual Bass source frequency boundaries setting */
    //void (*DAP_cpdp_virtual_bass_src_freqs_set)(void *, unsigned, unsigned);
    int aml_dap_cpdp_virtual_bass_src_freqs_set(DAPapi *pDAPapi, void *p_dap_cpdp, unsigned low_src_freq , unsigned high_src_freq)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtual_bass_src_freqs_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtual_bass_src_freqs_set)(p_dap_cpdp, low_src_freq, high_src_freq);

        return ret;
    }

    //void (*DAP_cpdp_virtual_bass_overall_gain_set)(void *, int);
    int aml_dap_cpdp_virtual_bass_overall_gain_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtual_bass_overall_gain_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtual_bass_overall_gain_set)(p_dap_cpdp, mode);

        return ret;
    }

    //void (*DAP_cpdp_virtual_bass_slope_gain_set)(void *, int);
    int aml_dap_cpdp_virtual_bass_slope_gain_set(DAPapi *pDAPapi, void  *p_dap_cpdp, int mode)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtual_bass_slope_gain_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtual_bass_slope_gain_set)(p_dap_cpdp, mode);

        return ret;
    }

    //void (*DAP_cpdp_virtual_bass_subgains_set)(void *, unsigned, int *);
    int aml_dap_cpdp_virtual_bass_subgains_set(DAPapi *pDAPapi, void  *p_dap_cpdp, unsigned size, int *p_subgains)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtual_bass_subgains_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtual_bass_subgains_set)(p_dap_cpdp, size, p_subgains);

        return ret;
    }


    //void (*DAP_cpdp_virtual_bass_mix_freqs_set)(void *, unsigned, unsigned);
    int aml_dap_cpdp_virtual_bass_mix_freqs_set(DAPapi *pDAPapi, void *p_dap_cpdp, unsigned low_mix_freq, unsigned high_mix_freq)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtual_bass_mix_freqs_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtual_bass_mix_freqs_set)(p_dap_cpdp, low_mix_freq, high_mix_freq);

        return ret;
    }

    int aml_dap_cpdp_ieq_enable_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_ieq_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_ieq_enable_set)(p_dap_cpdp, value);

        return ret;
    }


    int aml_dap_cpdp_ieq_bands_set
    (
        DAPapi *pDAPapi
        , void *p_dap_cpdp
        , unsigned int nb_bands
        , const unsigned int *p_band_centers
        , const int *p_band_targets
    )
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_ieq_bands_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_ieq_bands_set)(p_dap_cpdp, nb_bands, p_band_centers, p_band_targets);

        return ret;
    }

    int aml_dap_cpdp_volume_leveler_enable_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volume_leveler_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_volume_leveler_enable_set)(p_dap_cpdp, value);

        return ret;
    }


    int aml_dap_cpdp_volume_modeler_enable_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volume_modeler_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_volume_modeler_enable_set)(p_dap_cpdp, value);

        return ret;
    }

    /*---------------------------------------------------------------------------------*/
    /*DAP_CPDP_DV_PARAMS*/
    /*---------------------------------------------------------------------------------*/

    //size_t dap_cpdp_pvt_dv_params_query_memory(void);
    size_t aml_dap_cpdp_pvt_dv_params_query_memory(DAPapi *pDAPapi)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pvt_dv_params_query_memory == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_dv_params_query_memory)();

        return ret;
    }

    /*void
    dap_cpdp_pvt_dv_params_init
        (       void    *p_params
        ,const  unsigned                   *p_fixed_band_frequencies
        ,       unsigned                    nb_fixed_bands
        ,       void                       *p_mem
        );*/
    int aml_dap_cpdp_pvt_dv_params_init
    (
        DAPapi *pDAPapi
        , void    *p_params
        , const  unsigned                   *p_fixed_band_frequencies
        , unsigned                    nb_fixed_bands
        , void                       *p_mem
    )
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pvt_dv_params_init == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_pvt_dv_params_init)(p_params, p_fixed_band_frequencies, nb_fixed_bands, p_mem);

        return ret;
    }

    //void dap_cpdp_pvt_volume_and_ieq_update_control(void *p_params, unsigned nb_fixed_bands);
    int aml_dap_cpdp_pvt_volume_and_ieq_update_control(DAPapi *pDAPapi, void *p_params, unsigned nb_fixed_bands)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pvt_volume_and_ieq_update_control == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_pvt_volume_and_ieq_update_control)(p_params, nb_fixed_bands);

        return ret;
    }

    /* Returns true if DV is currently enabled */
    //int dap_cpdp_pvt_dv_enabled(const void   *p_params);
    int aml_dap_cpdp_pvt_dv_enabled(DAPapi *pDAPapi, const void   *p_params)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pvt_dv_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_dv_enabled)(p_params);

        return ret;
    }

    /* Returns true if DV is going to be enabled after the next call to update
     * control. This must be called under the parameter lock. */
    //int dap_cpdp_pvt_async_dv_enabled(const void   *p_params)
    int aml_dap_cpdp_pvt_async_dv_enabled(DAPapi *pDAPapi, const void   *p_params)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pvt_async_dv_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_async_dv_enabled)(p_params);

        return ret;
    }

    /* Returns true if either the volume leveler or volume modeler is
     * going to be enabled after the next call to update control
     * This must be called under the parameter lock. */
    //int dap_cpdp_pvt_async_leveler_or_modeler_enabled(const void   *p_params)
    int aml_dap_cpdp_pvt_async_leveler_or_modeler_enabled(DAPapi *pDAPapi, const void   *p_params)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pvt_async_leveler_or_modeler_enabled == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_pvt_async_leveler_or_modeler_enabled)(p_params);

        return ret;
    }
    /**
     * This function sets the gain the user would like the
     * signal chain to apply to the signal.
     */
    int aml_dap_cpdp_system_gain_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_system_gain_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_system_gain_set)(p_dap_cpdp, value);

        return ret;
    }

    /**
     * This function sets Headphone Virtualizer reverb gain
    */
    int aml_dap_cpdp_virtualizer_headphone_reverb_gain_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtualizer_headphone_reverb_gain_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        ret = (*pDAPapi->DAP_cpdp_virtualizer_headphone_reverb_gain_set)(p_dap_cpdp, value);

        return ret;
    }

    /*  This function sets how much the Leveler adjusts the loudness to
    *  normalize different audio content.
    */
    int aml_dap_cpdp_volume_leveler_amount_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volume_leveler_amount_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_volume_leveler_amount_set)(p_dap_cpdp, value);

        return ret;
    }

    /*
    *  The function specifies the strength of the Intelligent Equalizer effect to apply.
    */
    int aml_dap_cpdp_ieq_amount_set(DAPapi *pDAPapi, void *p_dap_cpdp, int value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_ieq_amount_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_ieq_amount_set)(p_dap_cpdp, value);

        return ret;
    }

    //void (*DAP_cpdp_virtualizer_speaker_angle_set)(void *,unsigned int);
    int aml_dap_cpdp_virtualizer_speaker_angle_set(DAPapi *pDAPapi, void      *p_dap_cpdp , unsigned int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtualizer_speaker_angle_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtualizer_speaker_angle_set)(p_dap_cpdp, value);

        return ret;
    }

    //void (*DAP_cpdp_virtualizer_speaker_start_freq_set)(void   *,unsigned int);
    int aml_dap_cpdp_virtualizer_speaker_start_freq_set(DAPapi *pDAPapi, void      *p_dap_cpdp , unsigned int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_virtualizer_speaker_start_freq_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_virtualizer_speaker_start_freq_set)(p_dap_cpdp, value);

        return ret;
    }

    //void (*DAP_cpdp_mi2ieq_steering_enable_set)(void   *, int);
    int aml_dap_cpdp_mi2ieq_steering_enable_set(DAPapi *pDAPapi, void      *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_mi2ieq_steering_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_mi2ieq_steering_enable_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_mi2dv_leveler_steering_enable_set)(void   *, int);
    int aml_dap_cpdp_mi2dv_leveler_steering_enable_set(DAPapi *pDAPapi, void      *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_mi2dv_leveler_steering_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_mi2dv_leveler_steering_enable_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_mi2dialog_enhancer_steering_enable_set)(void   *, int);
    int aml_dap_cpdp_mi2dialog_enhancer_steering_enable_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_mi2dialog_enhancer_steering_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_mi2dialog_enhancer_steering_enable_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_mi2surround_compressor_steering_enable_set)(void   *, int);
    int aml_dap_cpdp_mi2surround_compressor_steering_enable_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_mi2surround_compressor_steering_enable_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_mi2surround_compressor_steering_enable_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_calibration_boost_set)(void   *, int);
    int aml_dap_cpdp_calibration_boost_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_calibration_boost_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_calibration_boost_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_volume_leveler_in_target_set)(void   *, int);
    int aml_dap_cpdp_volume_leveler_in_target_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volume_leveler_in_target_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_volume_leveler_in_target_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_volume_leveler_out_target_set)(void   *, int);
    int aml_dap_cpdp_volume_leveler_out_target_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volume_leveler_out_target_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_volume_leveler_out_target_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_volume_modeler_calibration_set)(void   *, int);
    int aml_dap_cpdp_volume_modeler_calibration_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volume_modeler_calibration_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_volume_modeler_calibration_set)(p_dap_cpdp, value);

        return ret;
    }

    //void (*DAP_cpdp_pregain_set)(void *, int);
    int aml_dap_cpdp_pregain_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_pregain_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_pregain_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_postgain_set)(void *, int);
    int aml_dap_cpdp_postgain_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_postgain_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_postgain_set)(p_dap_cpdp, value);

        return ret;
    }
    //void (*DAP_cpdp_volmax_boost_set)(void   *, int);
    int aml_dap_cpdp_volmax_boost_set(DAPapi *pDAPapi, void *p_dap_cpdp , int  value)
    {
        int ret = 0;
        if (pDAPapi->DAP_cpdp_volmax_boost_set == NULL) {
            ALOGE("%s, pls load lib first.\n", __FUNCTION__);
            return -1;
        }

        (*pDAPapi->DAP_cpdp_volmax_boost_set)(p_dap_cpdp, value);

        return ret;
    }

    int audioFormat2sampleSize(audio_format_t aFormat)
    {
        int sampleSize;
        switch (aFormat) {
        case AUDIO_FORMAT_PCM_8_BIT:
            //case AUDIO_FORMAT_PCM_SUB_8_BIT:
            sampleSize = 1;
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
        case AUDIO_FORMAT_PCM:
            sampleSize = 2;
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            //case AUDIO_FORMAT_PCM_SUB_32_BIT:
            sampleSize = 4;
            break;
        default:
            sampleSize = -1;
        }
        return sampleSize;
    }



    int dap_init_api(DAPContext *pContext)
    {
        DAPdata *pDapData = NULL;
        dap_cpdp_init_info *pDapInitInfo;
        DAPapi *pDAPapi = (DAPapi *) & (pContext->gDAPapi);

        if (NULL == pContext) {
            ALOGE("%s, pContext == NULL\n", __FUNCTION__);
            return DAP_RET_FAIL;
        }

        pDapData = &(pContext->gDAPdata);

        pDapInitInfo = (dap_cpdp_init_info *) & (pDapData->init_info);
        pDapInitInfo->sample_rate = pContext->config.inputCfg.samplingRate;
        // dap_cpdp.h says Valid values are 32000, 44100 and 48000
        if ((pDapInitInfo->sample_rate != 32000)
            && (pDapInitInfo->sample_rate != 44100)
            && (pDapInitInfo->sample_rate != 48000)) {
            ALOGW("%s, pDapInitInfo->sample_rate = %lu\n", __FUNCTION__, pDapInitInfo->sample_rate);
        }
        pDapInitInfo->license_data = (unsigned char*)"full\n1589474320,0\n";
        pDapInitInfo->license_size = strlen((char*)pDapInitInfo->license_data) + 1;
        pDapInitInfo->manufacturer_id = 0;
        pDapInitInfo->mode = 0;
        pDapInitInfo->mi_process_disable = 0;
        pDapInitInfo->virtual_bass_process_enable = 1;

        // query dap persist memory usage
        pDapData->uPersistMemSize = aml_dap_cpdp_query_memory(pDAPapi, (const dap_cpdp_init_info *)pDapInitInfo);
        if (pDapData->uPersistMemSize <= 0) {
            ALOGE("<%s::%d>--[pDapData->uPersistMemSize <= 0]", __FUNCTION__, __LINE__);
            return DAP_RET_FAIL;
        }
        //ALOGI("<%s::%d>--[pDapData->uPersistMemSize = %d]", __FUNCTION__, __LINE__,pDapData->uPersistMemSize);

        // just in case pPersistMen is not free before
        if (pDapData->pPersistMem) {
            free(pDapData->pPersistMem);
            pDapData->pPersistMem = NULL;
        }

        pDapData->pPersistMem = malloc(pDapData->uPersistMemSize);
        if (pDapData->pPersistMem == NULL) {
            ALOGE("<%s::%d>--[pPersistMem malloc error]", __FUNCTION__, __LINE__);
            return DAP_RET_FAIL;
        }


        // Query DAP scratch memory usage
        pDapData->uScratchSize = aml_dap_cpdp_query_scratch(pDAPapi, (const dap_cpdp_init_info *)pDapInitInfo);
        if (pDapData->uScratchSize <= 0) {
            ALOGE("<%s::%d>--[aml_dap_cpdp_query_scratch return size illegal]", __FUNCTION__, __LINE__);
            if (pDapData->pPersistMem) {
                free(pDapData->pPersistMem);
                pDapData->pPersistMem = NULL;
            }
            //return -EINVAL;
            return DAP_RET_FAIL;
        }
        //ALOGV("%s, scratch_size = %d\n", __FUNCTION__,scratch_size);

        // prepare scratch memory
        if (pDapData->pScratchMem) {
            free(pDapData->pScratchMem);
            pDapData->pScratchMem = NULL;
        }

        // malloc scratch  memory
        pDapData->pScratchMem = malloc(pDapData->uScratchSize);
        if (pDapData->pScratchMem == NULL) {
            ALOGE("<%s::%d>--[aml_dap_cpdp_query_scratch return size illegal]", __FUNCTION__, __LINE__);
            if (pDapData->pPersistMem) {
                free(pDapData->pPersistMem);
                pDapData->pPersistMem = NULL;
            }
            //return -EINVAL;
            return DAP_RET_FAIL;
        }
        memset(pDapData->pScratchMem, 0, pDapData->uScratchSize);

        // just in case dap_cpdp is not closed before
        if (pDapData->dap_cpdp) {
            aml_dap_cpdp_shutdown(pDAPapi, pDapData->dap_cpdp);
            pDapData->dap_cpdp = NULL;
        }
        //ALOGE("<%s::%d>--aml_dap_cpdp_shutdown done", __FUNCTION__, __LINE__);

        /* The dap_cpdp_init_info struct passed here must be the same as the one
         * passed to dap_cpdp_query_memory(). */
        pDapData->dap_cpdp = aml_dap_cpdp_init(pDAPapi, pDapInitInfo, pDapData->pPersistMem);
        if (pDapData->dap_cpdp == NULL) {
            if (pDapData->pPersistMem != NULL) {
                free(pDapData->pPersistMem);
                pDapData->pPersistMem = NULL;
            }
            if (pDapData->pScratchMem) {
                free(pDapData->pScratchMem);
                pDapData->pScratchMem = NULL;
            }
            ALOGE("<%s::%d>--[aml_dap_cpdp_init init error]", __FUNCTION__, __LINE__);
            return DAP_RET_FAIL;
        }

        pDapData->bDapCPDPInited = 1;

        ALOGI("<%s::%d>--[persistent_size:%d]--[persistent_memory:0x%p]--[dap_cpdp:0x%p]",
              __FUNCTION__, __LINE__, pDapData->uPersistMemSize, pDapData->pPersistMem, pDapData->dap_cpdp);

        return DAP_RET_SUCESS;
    }


    int dap_release_api(DAPContext *pContext)
    {
        DAPdata *pDapData = NULL;
        void *dap_cpdp = NULL;
        DAPapi *pDAPapi = (DAPapi *) & (pContext->gDAPapi);

        pDapData = &(pContext->gDAPdata);

        if (pDapData->dap_cpdp) {
            aml_dap_cpdp_shutdown(pDAPapi, pDapData->dap_cpdp);
            pDapData->dap_cpdp = NULL;
        }

        /* Note that the dap_cpdp pointer is likely to be different to the
        * persistent_memory pointer, so we need to be careful to free
        * the correct pointer. */
        if (pDapData->pPersistMem) {
            free(pDapData->pPersistMem);
            pDapData->pPersistMem = NULL;
        }

        if (pDapData->pScratchMem) {
            free(pDapData->pScratchMem);
            pDapData->pScratchMem = NULL;
        }

        pDapData->bDapCPDPInited = 0;

        ALOGI("<%s::%d>--[dap_release end]", __FUNCTION__, __LINE__);
        return 0;
    }

    int do_passthrough(audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
    {
        int16_t   *in  = (int16_t *)inBuffer->raw;
        int16_t   *out = (int16_t *)outBuffer->raw;

        for (size_t i = 0; i < inBuffer->frameCount; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
        return 0;
    }


    static int dap_load_user_param(DAPContext *pContext, dolby_base *effect_mode)
    {
        void *dap_cpdp = NULL;
        int *ao_gains[DAP_CPDP_MAX_NUM_CHANNELS];
        DAPapi *pDAPapi = (DAPapi *) & (pContext->gDAPapi);
        //int **pp_ao_gains;


        dap_cpdp = pContext->gDAPdata.dap_cpdp;
        if (!dap_cpdp) {
            ALOGE("%s, !dap_cpdp\n", __FUNCTION__);
            return DAP_RET_FAIL;
        }

        int i;
        for (i = 0; i < DAP_MAX_CHANNELS; i++) {
            ao_gains[i] = effect_mode->ao_band_gains[i];
        }

        aml_dap_cpdp_pregain_set(pDAPapi, dap_cpdp, effect_mode->pregain);
        aml_dap_cpdp_postgain_set(pDAPapi, dap_cpdp, effect_mode->postgain);
        aml_dap_cpdp_system_gain_set(pDAPapi, dap_cpdp, effect_mode->systemgain);

        aml_dap_cpdp_virtualizer_headphone_reverb_gain_set(pDAPapi, dap_cpdp, effect_mode->headphone_reverb);
        aml_dap_cpdp_virtualizer_speaker_angle_set(pDAPapi, dap_cpdp, effect_mode->speaker_angle);
        aml_dap_cpdp_virtualizer_speaker_start_freq_set(pDAPapi, dap_cpdp, effect_mode->speaker_start);
        aml_dap_cpdp_mi2ieq_steering_enable_set(pDAPapi, dap_cpdp, effect_mode->mi_ieq_enable);
        aml_dap_cpdp_mi2dv_leveler_steering_enable_set(pDAPapi, dap_cpdp, effect_mode->mi_dv_enable);
        aml_dap_cpdp_mi2dialog_enhancer_steering_enable_set(pDAPapi, dap_cpdp, effect_mode->mi_de_enable);
        aml_dap_cpdp_mi2surround_compressor_steering_enable_set(pDAPapi, dap_cpdp, effect_mode->mi_surround_enable);

        aml_dap_cpdp_calibration_boost_set(pDAPapi, dap_cpdp, effect_mode->calibration_boost);
        aml_dap_cpdp_volume_leveler_in_target_set(pDAPapi, dap_cpdp, effect_mode->leveler_input);
        aml_dap_cpdp_volume_leveler_out_target_set(pDAPapi, dap_cpdp, effect_mode->leveler_output);
        aml_dap_cpdp_volume_modeler_enable_set(pDAPapi, dap_cpdp, effect_mode->modeler_enable);
        aml_dap_cpdp_volume_modeler_calibration_set(pDAPapi, dap_cpdp, effect_mode->modeler_calibration);

        aml_dap_cpdp_ieq_enable_set(pDAPapi, dap_cpdp, effect_mode->ieq_enable);
        aml_dap_cpdp_ieq_amount_set(pDAPapi, dap_cpdp, effect_mode->ieq_amount);
        aml_dap_cpdp_ieq_bands_set(pDAPapi, dap_cpdp, effect_mode->ieq_nb_bands,
                                   (const unsigned int *)effect_mode->a_ieq_band_center,
                                   (const int *)effect_mode->a_ieq_band_target);
        aml_dap_cpdp_de_ducking_set(pDAPapi, dap_cpdp, effect_mode->de_ducking);
        aml_dap_cpdp_volmax_boost_set(pDAPapi, dap_cpdp, effect_mode->volmax_boost);

        aml_dap_cpdp_audio_optimizer_enable_set(pDAPapi, dap_cpdp, effect_mode->optimizer_enable);
        aml_dap_cpdp_audio_optimizer_bands_set(pDAPapi, dap_cpdp, effect_mode->ao_bands,
                                               (const unsigned int *)effect_mode->ao_band_center_freq,
                                               ao_gains);

        aml_dap_cpdp_bass_enhancer_enable_set(pDAPapi, dap_cpdp, effect_mode->bass_enable);
        aml_dap_cpdp_bass_enhancer_boost_set(pDAPapi, dap_cpdp, effect_mode->bass_boost);
        aml_dap_cpdp_bass_enhancer_cutoff_frequency_set(pDAPapi, dap_cpdp, effect_mode->bass_cutoff);
        aml_dap_cpdp_bass_enhancer_width_set(pDAPapi, dap_cpdp, effect_mode->bass_width);

        aml_dap_cpdp_regulator_tuning_set(pDAPapi, dap_cpdp, (unsigned int) effect_mode->ar_bands,
                                          (const  unsigned int *)effect_mode->ar_band_center_freq,
                                          (const  int *)effect_mode->ar_low_thresholds,
                                          (const  int *)effect_mode->ar_high_thresholds,
                                          (const  int *)effect_mode->ar_isolated_bands);

        aml_dap_cpdp_regulator_overdrive_set(pDAPapi, dap_cpdp, effect_mode->regulator_overdrive);
        aml_dap_cpdp_regulator_timbre_preservation_set(pDAPapi, dap_cpdp, effect_mode->regulator_timbre);
        aml_dap_cpdp_regulator_relaxation_amount_set(pDAPapi, dap_cpdp, effect_mode->regulator_distortion);
        aml_dap_cpdp_regulator_speaker_distortion_enable_set(pDAPapi, dap_cpdp, effect_mode->regulator_mode);
        aml_dap_cpdp_regulator_enable_set(pDAPapi, dap_cpdp, effect_mode->regulator_enable);

        aml_dap_cpdp_virtual_bass_mode_set(pDAPapi, dap_cpdp, effect_mode->virtual_bass_mode);
        aml_dap_cpdp_virtual_bass_src_freqs_set(pDAPapi, dap_cpdp, effect_mode->virtual_bass_low_src_freq,
                                                effect_mode->virtual_bass_high_src_freq);
        aml_dap_cpdp_virtual_bass_overall_gain_set(pDAPapi, dap_cpdp, effect_mode->virtual_bass_overall_gain);
        aml_dap_cpdp_virtual_bass_slope_gain_set(pDAPapi, dap_cpdp, effect_mode->virtual_bass_slope_gain);
        aml_dap_cpdp_virtual_bass_subgains_set(pDAPapi, dap_cpdp, 3, effect_mode->virtual_bass_subgain);
        aml_dap_cpdp_virtual_bass_mix_freqs_set(pDAPapi, dap_cpdp, effect_mode->virtual_bass_mix_low_freq,
                                                effect_mode->virtual_bass_mix_high_freq);

        return 0;
    }

    int dap_set_virtual_surround(DAPContext *pContext, dolby_virtual_surround_t *aml_virtual_surround)
    {
        int ret = 0;
        int boost = 0;
        void *dap_cpdp = NULL;
        DAPdata *pDapData = &pContext->gDAPdata;

        DAPapi *pDAPapi;
        pDAPapi = &(pContext->gDAPapi);

        dap_cpdp = pDapData->dap_cpdp;
        if (!dap_cpdp) {
            return DAP_RET_FAIL;
        }

        ret = aml_dap_cpdp_surround_decoder_enable_set(pDAPapi, dap_cpdp,
                aml_virtual_surround->enable.surround_decoder_enable);
        if (ret < 0) {
            ALOGE("Error: surround_decoder_enable\n");
        } else {
            //pDapData->dapVirtualSrnd.enable.surround_decoder_enable =
            //    aml_virtual_surround->enable.surround_decoder_enable;
        }

        ret = aml_dap_cpdp_output_mode_set(pDAPapi, dap_cpdp,
                                           aml_virtual_surround->enable.virtualizer_enable);
        if (ret < 0) {
            ALOGE("Error: virtualizer_enable\n");
        } else {
            //pDapData->dapVirtualSrnd.enable.virtualizer_enable =
            //    aml_virtual_surround->enable.virtualizer_enable;
        }

        boost = (aml_virtual_surround->surround_boost<<DAP_CPDP_SURROUND_BOOST_FRAC_BITS);
        if (boost > DAP_CPDP_SURROUND_BOOST_MAX) {
            boost = DAP_CPDP_SURROUND_BOOST_MAX;
        }
        else if (boost < DAP_CPDP_SURROUND_BOOST_MIN) {
            boost = DAP_CPDP_SURROUND_BOOST_MIN;
        }
        ret = aml_dap_cpdp_surround_boost_set(pDAPapi, dap_cpdp, boost);
        if (ret < 0) {
            ALOGE("Error: surround_boost\n");
        } else {
            //pDapData->dapVirtualSrnd.surround_boost = aml_virtual_surround->surround_boost;
        }

        ALOGD("%s: dap dap_virtual_surround <enable> = %d <boost> = %d", __FUNCTION__, aml_virtual_surround->enable.surround_decoder_enable, aml_virtual_surround->surround_boost);

        return ret;
    }

    int dap_set_eq_params(DAPContext *pContext, dolby_eq_t *aml_eq)
    {
        int ret = 0, i;
        int gains[DAP_GEQ_NB_BANDS_DEFAULT];
        void *dap_cpdp = NULL;
        DAPdata *pDapData = &pContext->gDAPdata;
        DAPapi *pDAPapi;
        pDAPapi = &(pContext->gDAPapi);

        dap_cpdp = pDapData->dap_cpdp;
        if (!dap_cpdp) {
            return DAP_RET_FAIL;
        }

        ret = aml_dap_cpdp_graphic_equalizer_enable_set(pDAPapi, dap_cpdp,
                (aml_eq->eq_enable.geq_enable == GEQ_MODE_DISABLE)?0:1);
        if (ret < 0) {
            ALOGE("Error: eq_enable\n");
        } else {
            //pDapData->dapEQ.eq_enable = aml_eq->eq_enable;
        }

        for (i=0; i<DAP_GEQ_NB_BANDS_DEFAULT; i++) {
            gains[i] = (aml_eq->eq_params.a_geq_band_target[i]*576/10);
            if (gains[i] > DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MAX) {
                gains[i] = DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MAX;
            } else if (gains[i] < DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MIN) {
                gains[i] = DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MIN;
            }
        }

        ret = aml_dap_cpdp_graphic_equalizer_bands_set(pDAPapi, dap_cpdp,
                aml_eq->eq_params.geq_nb_bands,
                (const unsigned int *)aml_eq->eq_params.a_geq_band_center,
                (const int *)gains);
        if (ret < 0) {
            ALOGE("Error: eq_enable\n");
        } else {
            //pDapData->dapEQ.eq_params = aml_eq->eq_params;
        }
        ALOGD("%s dap dap_geq <enable> = %d <gains> = %d %d %d %d %d",
                __FUNCTION__, aml_eq->eq_enable.geq_enable,
                aml_eq->eq_params.a_geq_band_target[0],
                aml_eq->eq_params.a_geq_band_target[1],
                aml_eq->eq_params.a_geq_band_target[2],
                aml_eq->eq_params.a_geq_band_target[3],
                aml_eq->eq_params.a_geq_band_target[4]);
        return ret;
    }

    int dap_set_dialog_enhance(DAPContext *pContext, dolby_dialog_enhance_t *aml_dialog_enhance)
    {
        int ret = 0;
        void *dap_cpdp = NULL;
        DAPdata *pDapData = &pContext->gDAPdata;
        DAPapi *pDAPapi;
        pDAPapi = &(pContext->gDAPapi);

        dap_cpdp = pDapData->dap_cpdp;
        if (!dap_cpdp) {
            return DAP_RET_FAIL;
        }

        ret = aml_dap_cpdp_de_enable_set(pDAPapi, dap_cpdp, aml_dialog_enhance->de_enable);
        if (ret < 0) {
            ALOGE("Error: de_enable\n");
        } else {
            //pDapData->dapDialogEnhance.de_enable = aml_dialog_enhance->de_enable;
        }

        if (aml_dialog_enhance->de_amount > DAP_CPDP_DE_AMOUNT_MAX) {
            aml_dialog_enhance->de_amount = DAP_CPDP_DE_AMOUNT_MAX;
        }
        if (aml_dialog_enhance->de_amount < DAP_CPDP_DE_AMOUNT_MIN) {
            aml_dialog_enhance->de_amount = DAP_CPDP_DE_AMOUNT_MIN;
        }

        ret = aml_dap_cpdp_de_amount_set(pDAPapi, dap_cpdp, aml_dialog_enhance->de_amount);
        if (ret < 0) {
            ALOGE("Error: de_amount\n");
        } else {
            //pDapData->dapDialogEnhance.de_amount = aml_dialog_enhance->de_amount;
        }

        ALOGD("%s: dap dialog enhancer <enable> = %d <amount> = %d", __FUNCTION__, aml_dialog_enhance->de_enable, aml_dialog_enhance->de_amount);


        return ret;
    }

    int dap_set_vol_leveler(DAPContext *pContext, dolby_vol_leveler_t *aml_vol_leveler)
    {
        int ret = 0;
        void *dap_cpdp = NULL;
        DAPdata *pDapData = &pContext->gDAPdata;
        DAPapi *pDAPapi;
        pDAPapi = &(pContext->gDAPapi);

        dap_cpdp = pDapData->dap_cpdp;
        if (!dap_cpdp) {
            return DAP_RET_FAIL;
        }

        ret = aml_dap_cpdp_volume_leveler_enable_set(pDAPapi, dap_cpdp, aml_vol_leveler->vl_enable);
        if (ret < 0) {
            ALOGE("Error: vol leveler_enable\n");
        } else {
            //pDapData->dapVolLeveler.vl_enable = aml_vol_leveler->vl_enable;
        }

        if (aml_vol_leveler->vl_amount > DAP_CPDP_VOLUME_LEVELER_AMOUNT_MAX) {
            aml_vol_leveler->vl_amount = DAP_CPDP_VOLUME_LEVELER_AMOUNT_MAX;
        }
        if (aml_vol_leveler->vl_amount < DAP_CPDP_VOLUME_LEVELER_AMOUNT_MIN) {
            aml_vol_leveler->vl_amount = DAP_CPDP_VOLUME_LEVELER_AMOUNT_MIN;
        }

        ret = aml_dap_cpdp_volume_leveler_amount_set(pDAPapi, dap_cpdp, aml_vol_leveler->vl_amount);
        if (ret < 0) {
            ALOGE("Error: vol amount\n");
        } else {
            //pDapData->dapVolLeveler.vl_amount = aml_vol_leveler->vl_amount;
        }

        ALOGD("%s: dap volume leveler <enable> = %d <amount> = %d", __FUNCTION__, aml_vol_leveler->vl_enable, aml_vol_leveler->vl_amount);

        return ret;
    }

    int dap_set_postgain(DAPContext *pContext, int postgain)
    {
        int ret = 0;
        void *dap_cpdp = NULL;
        DAPdata *pDapData = &pContext->gDAPdata;
        DAPapi *pDAPapi = &(pContext->gDAPapi);

        dap_cpdp = pDapData->dap_cpdp;
        if (!dap_cpdp) {
            return DAP_RET_FAIL;
        }

        if (postgain > DAP_CPDP_POSTGAIN_MAX) {
            postgain = DAP_CPDP_POSTGAIN_MAX;
        }
        if (postgain < DAP_CPDP_POSTGAIN_MIN) {
            postgain = DAP_CPDP_POSTGAIN_MIN;
        }

        dap_cpdp = pContext->gDAPdata.dap_cpdp;
        if (!dap_cpdp) {
            ALOGE("%s, !dap_cpdp\n", __FUNCTION__);
            return DAP_RET_FAIL;
        }

        ALOGI("<%s::%d>--[postgain:%d]", __FUNCTION__, __LINE__, postgain);
        ret = aml_dap_cpdp_postgain_set(pDAPapi, dap_cpdp, postgain);
        if (ret < 0) {
            ALOGE("Error: set_postgain\n");
        } else {
            pDapData->dapPostGain = postgain;
        }
        return ret;
    }

    int dap_get_mode_param(DAPContext *pContext, uint32_t param, void* pValue, bool first) {
        DAPdata *pDapData = &pContext->gDAPdata;
        int i=0;

        switch (param) {
            case DAP_PARAM_VOL_LEVELER_ENABLE:
            case DAP_PARAM_VOL_LEVELER_AMOUNT:
            {
                dolby_vol_leveler_t* dapVolLeveler = (dolby_vol_leveler_t*)pValue;
                if (first) {
                    dapVolLeveler->vl_enable = default_vl_param[DAP_MODE_CUSTOM][0];
                    dapVolLeveler->vl_amount = default_vl_param[DAP_MODE_CUSTOM][1];
                } else if (pContext->modeValue == DAP_MODE_CUSTOM) {
                    dapVolLeveler->vl_enable = pDapData->dapVolLeveler.vl_enable;
                    dapVolLeveler->vl_amount = pDapData->dapVolLeveler.vl_amount;
                } else {
                    dapVolLeveler->vl_enable = default_vl_param[pContext->modeValue][0];
                    dapVolLeveler->vl_amount = default_vl_param[pContext->modeValue][1];
                }
                break;
            }
            case DAP_PARAM_DE_ENABLE:
            case DAP_PARAM_DE_AMOUNT:
            {
                dolby_dialog_enhance_t* dapDialogEnhance = (dolby_dialog_enhance_t*)pValue;
                if (first) {
                    dapDialogEnhance->de_enable = default_de_param[DAP_MODE_CUSTOM][0];
                    dapDialogEnhance->de_amount = default_de_param[DAP_MODE_CUSTOM][1];
                } else if (pContext->modeValue == DAP_MODE_CUSTOM) {
                    dapDialogEnhance->de_enable = pDapData->dapDialogEnhance.de_enable;
                    dapDialogEnhance->de_amount = pDapData->dapDialogEnhance.de_amount;
                } else {
                    dapDialogEnhance->de_enable = default_de_param[pContext->modeValue][0];
                    dapDialogEnhance->de_amount = default_de_param[pContext->modeValue][1];
                }
                break;
            }
            case DAP_PARAM_SUROUND_ENABLE:
            case DAP_PARAM_SURROUND_BOOST:
            case DAP_PARAM_VIRTUALIZER_ENABLE:
            {
                dolby_virtual_surround_t* dapVirtualSrnd = (dolby_virtual_surround_t*)pValue;
                if (first) {
                    dapVirtualSrnd->enable.surround_decoder_enable = default_surr_param[DAP_MODE_CUSTOM][0];
                    dapVirtualSrnd->enable.virtualizer_enable = default_surr_param[DAP_MODE_CUSTOM][1];
                    dapVirtualSrnd->surround_boost = default_surr_param[DAP_MODE_CUSTOM][2];
                } else if (pContext->modeValue == DAP_MODE_CUSTOM) {
                    dapVirtualSrnd->enable.surround_decoder_enable = pDapData->dapVirtualSrnd.enable.surround_decoder_enable;
                    dapVirtualSrnd->enable.virtualizer_enable = pDapData->dapVirtualSrnd.enable.virtualizer_enable;
                    dapVirtualSrnd->surround_boost = pDapData->dapVirtualSrnd.surround_boost;
                } else {
                    dapVirtualSrnd->enable.surround_decoder_enable = default_surr_param[pContext->modeValue][0];
                    dapVirtualSrnd->enable.virtualizer_enable = default_surr_param[pContext->modeValue][1];
                    dapVirtualSrnd->surround_boost = default_surr_param[pContext->modeValue][2];
                }
                break;
            }
            case DAP_PARAM_GEQ_ENABLE:
            case DAP_PARAM_GEQ_GAINS:
            {
                dolby_eq_t* dapGeq = (dolby_eq_t*)pValue;
                dapGeq->eq_params.geq_nb_bands = DAP_GEQ_NB_BANDS_DEFAULT;
                for (i=0; i<dapGeq->eq_params.geq_nb_bands; i++)
                    dapGeq->eq_params.a_geq_band_center[i] = default_geq_center[i];
                if (first) {
                    dapGeq->eq_enable.geq_enable = default_geq_enable[DAP_MODE_CUSTOM];
                } else if (pContext->modeValue == DAP_MODE_CUSTOM) {
                    dapGeq->eq_enable.geq_enable = pDapData->dapEQ.eq_enable.geq_enable;
                } else {
                    dapGeq->eq_enable.geq_enable = default_geq_enable[pContext->modeValue];
                }
                if (first) {
                    for (i=0; i<dapGeq->eq_params.geq_nb_bands; i++)
                        dapGeq->eq_params.a_geq_band_target[i] = default_geq_gains[GEQ_MODE_USER][i];
                } else if (dapGeq->eq_enable.geq_enable == GEQ_MODE_USER) {
                    for (i=0; i<dapGeq->eq_params.geq_nb_bands; i++)
                        dapGeq->eq_params.a_geq_band_target[i] = pDapData->dapEQ.eq_params.a_geq_band_target[i];
                } else {
                    for (i=0; i<dapGeq->eq_params.geq_nb_bands; i++)
                        dapGeq->eq_params.a_geq_band_target[i] = default_geq_gains[dapGeq->eq_enable.geq_enable][i];
                }
                break;
            }
        }
        return 0;
    }

    int dap_set_enable(DAPContext *pContext, int enable)
    {
        DAPdata *pDapData = &pContext->gDAPdata;
        pDapData->bDapEnabled = enable;
        ALOGD("<%s::%d>--[enable:%d]", __FUNCTION__, __LINE__, enable);
        return 0;
    }

    int dap_set_effect_mode(DAPContext *pContext, DAPmode eMode)
    {
        //ALOGE("<%s::%d>--[mode:%d]", __FUNCTION__, __LINE__, mode);
        dolby_virtual_surround_t dapVirtualSrnd;
        dolby_dialog_enhance_t   dapDialogEnhance;
        dolby_vol_leveler_t      dapVolLeveler;
        dolby_eq_t               dapEQ;

        DAPdata *pDapData = &pContext->gDAPdata;
        pDapData->eDapEffectMode = eMode;

        dap_get_mode_param(pContext, DAP_PARAM_VOL_LEVELER_ENABLE, (void *)&dapVolLeveler, false);
        dap_get_mode_param(pContext, DAP_PARAM_DE_ENABLE, (void *)&dapDialogEnhance, false);
        dap_get_mode_param(pContext, DAP_PARAM_SUROUND_ENABLE, (void *)&dapVirtualSrnd, false);
        dap_get_mode_param(pContext, DAP_PARAM_GEQ_ENABLE, (void *)&dapEQ, false);

        dap_set_virtual_surround(pContext, (dolby_virtual_surround_t *) & (dapVirtualSrnd));
        dap_set_dialog_enhance(pContext, (dolby_dialog_enhance_t *) & (dapDialogEnhance));
        dap_set_vol_leveler(pContext, (dolby_vol_leveler_t *) & (dapVolLeveler));
        dap_set_eq_params(pContext,(dolby_eq_t *) & (dapEQ));

        if (eMode == DAP_MODE_MUSIC) {
            dap_load_user_param(pContext, (dolby_base *)&dap_dolby_base_music);
        } else if (eMode == DAP_MODE_MOVIE) {
            dap_load_user_param(pContext, (dolby_base *)&dap_dolby_base_movie);
        } else if (eMode == DAP_MODE_NIGHT) {
            dap_load_user_param(pContext, (dolby_base *)&dap_dolby_base_night); //JUST FOR DEBUG
        } else if (eMode == DAP_MODE_DISABLE) {
            dap_load_user_param(pContext, (dolby_base *)&dap_dolby_base_disable);
            dap_set_enable(pContext, 0);
            AudioFadeSetState(&pContext->gAudFade, AUD_FADE_IDLE);
            return 0;
        } else if (eMode == DAP_MODE_CUSTOM) {
            dap_load_user_param(pContext, (dolby_base *)&dap_dolby_base_custom);
        } else {
            dap_load_user_param(pContext, (dolby_base *)&dap_dolby_base_music);
            pDapData->eDapEffectMode = DAP_MODE_MUSIC;
        }
        dap_set_enable(pContext, 1);

        ALOGD("%s: dap_effect_mode -> %s", __FUNCTION__, DapEffectModeStr[pDapData->eDapEffectMode]);

        //pDapData->bNeedReset = 1;
        return 0;
    }

#if 0
    int dap_get_vol_leveler_enable(DAPContext *pContext)
    {
        DAPdata *pDapData = &pContext->gDAPdata;
        return pDapData->bVolLevelerEnabled;
    }

    int dap_get_postgain(DAPContext *pContext)
    {
        DAPdata *pDapData = &pContext->gDAPdata;
        return pDapData->dapPostGain;
    }

    DAPmode dap_get_effect_mode(DAPContext *pContext)
    {
        //ALOGE("<%s::%d>--[mode:%d]", __FUNCTION__, __LINE__, mode);

        DAPdata *pDapData = &pContext->gDAPdata;
        return pDapData->eDapEffectMode;
    }

    int dap_get_enable(DAPContext *pContext)
    {
        DAPdata *pDapData = &pContext->gDAPdata;
        return pDapData->bDapEnabled;
    }


    int DAP_get_model_name(char *model_name, int size)
    {
        int fd;
        int ret = -1;
        char node[50] = {0};
        const char *filename = "/proc/idme/model_name";

        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            ALOGE("%s: open %s failed", __FUNCTION__, filename);
            goto exit;
        }
        if (read(fd, node, 50) < 0) {
            ALOGE("%s: read Model Name failed", __FUNCTION__);
            goto exit;
        }

        ret = 0;
exit:
        if (ret < 0) {
            snprintf(model_name, size, "DEFAULT");
        } else {
            snprintf(model_name, size, "%s", node);
        }
        ALOGD("%s: Model Name -> %s", __FUNCTION__, model_name);
        close(fd);
        return ret;
    }
#endif

    int DAP_print_base_setting(dolby_base* pDapBaseSets)
    {
        int *pInt;

        ALOGD("%s: DAP_print_base_setting", __FUNCTION__);

        ALOGD("pregain = %d", pDapBaseSets->pregain);
        ALOGD("postgain = %d", pDapBaseSets->postgain);
        ALOGD("systemgain = %d", pDapBaseSets->systemgain);

        ALOGD("headphone_reverb = %d", pDapBaseSets->headphone_reverb);
        ALOGD("speaker_angle = %d", pDapBaseSets->speaker_angle);
        ALOGD("speaker_start = %d", pDapBaseSets->speaker_start);
        ALOGD("mi_ieq_enable = %d", pDapBaseSets->mi_ieq_enable);
        ALOGD("mi_dv_enable = %d", pDapBaseSets->mi_dv_enable);
        ALOGD("mi_de_enable = %d", pDapBaseSets->mi_de_enable);
        ALOGD("mi_surround_enable = %d", pDapBaseSets->mi_surround_enable);

        ALOGD("calibration_boost = %d", pDapBaseSets->calibration_boost);
        ALOGD("leveler_input = %d", pDapBaseSets->leveler_input);
        ALOGD("leveler_output = %d", pDapBaseSets->leveler_output);
        ALOGD("modeler_enable = %d", pDapBaseSets->modeler_enable);
        ALOGD("modeler_calibration = %d", pDapBaseSets->modeler_calibration);

        ALOGD("ieq_enable = %d", pDapBaseSets->ieq_enable);
        ALOGD("ieq_amount = %d", pDapBaseSets->ieq_amount);
        ALOGD("ieq_nb_bands = %d", pDapBaseSets->ieq_nb_bands);
        pInt = pDapBaseSets->a_ieq_band_center;
        ALOGD("a_ieq_band_center = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        pInt = pDapBaseSets->a_ieq_band_target;
        ALOGD("a_ieq_band_target = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        ALOGD("de_ducking = %d", pDapBaseSets->de_ducking);
        ALOGD("volmax_boost = %d", pDapBaseSets->volmax_boost);

        ALOGD("optimizer_enable = %d", pDapBaseSets->optimizer_enable);
        ALOGD("ao_bands = %d", pDapBaseSets->ao_bands);
        pInt = pDapBaseSets->ao_band_center_freq;
        ALOGD("ao_band_center_freq = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        ALOGD("ao_band_gains = TODO");
        ALOGD("bass_enable = %d", pDapBaseSets->bass_enable);
        ALOGD("bass_boost = %d", pDapBaseSets->bass_boost);
        ALOGD("bass_cutoff = %d", pDapBaseSets->bass_cutoff);
        ALOGD("bass_width = %d", pDapBaseSets->bass_width);

        ALOGD("ar_bands = %d", pDapBaseSets->ar_bands);
        pInt = pDapBaseSets->ar_band_center_freq;
        ALOGD("ar_band_center_freq = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        pInt = pDapBaseSets->ar_low_thresholds;
        ALOGD("ar_low_thresholds = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        pInt = pDapBaseSets->ar_high_thresholds;
        ALOGD("ar_high_thresholds = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        pInt = pDapBaseSets->ar_isolated_bands;
        ALOGD("ar_isolated_bands = \n[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,\n %d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
              pInt[0], pInt[1], pInt[2], pInt[3], pInt[4],
              pInt[5], pInt[6], pInt[7], pInt[8], pInt[9],
              pInt[10], pInt[11], pInt[12], pInt[13], pInt[14],
              pInt[15], pInt[16], pInt[17], pInt[18], pInt[19]);
        ALOGD("regulator_overdrive = %d", pDapBaseSets->regulator_overdrive);
        ALOGD("regulator_timbre = %d", pDapBaseSets->regulator_timbre);
        ALOGD("regulator_distortion = %d", pDapBaseSets->regulator_distortion);
        ALOGD("regulator_mode = %d", pDapBaseSets->regulator_mode);
        ALOGD("regulator_enable = %d", pDapBaseSets->regulator_enable);

        ALOGD("virtual_bass_mode = %d", pDapBaseSets->virtual_bass_mode);
        ALOGD("virtual_bass_low_src_freq = %d", pDapBaseSets->virtual_bass_low_src_freq);
        ALOGD("virtual_bass_high_src_freq = %d", pDapBaseSets->virtual_bass_high_src_freq);
        ALOGD("virtual_bass_overall_gain = %d", pDapBaseSets->virtual_bass_overall_gain);
        ALOGD("virtual_bass_slope_gain = %d", pDapBaseSets->virtual_bass_slope_gain);
        pInt = pDapBaseSets->virtual_bass_subgain;
        ALOGD("virtual_bass_subgain = \n[%d,%d,%d]", pInt[0], pInt[1], pInt[2]);
        ALOGD("virtual_bass_mix_low_freq = %d", pDapBaseSets->virtual_bass_mix_low_freq);
        ALOGD("virtual_bass_mix_high_freq = %d", pDapBaseSets->virtual_bass_mix_high_freq);
        return 0;
    }

    int DAP_ini_parse_array(int* pIntArr, int num, const char *buffer)
    {
        int i;
        char *Rch = NULL;
        int result = -1;

        if (pIntArr == NULL || buffer == NULL) {
            ALOGE("%s: NULL pointer", __FUNCTION__);
            return DAP_RET_FAIL;
        }

        Rch = (char *)buffer;
        for (i = 0; i < num; i++) {
            if (i == 0) {
                Rch = strtok(Rch, ",");
            } else {
                Rch = strtok(NULL, ",");
            }
            if (Rch == NULL) {
                ALOGE("%s: get arrry value failed", __FUNCTION__);
                goto error;
            }
            pIntArr[i] = atoi(Rch);
            //*(pIntArr+i] = atoi(Rch);
            //ALOGD("%s: data[%d] = %s -> %d", __FUNCTION__, i, Rch, pIntArr[i]);
        }
        return DAP_RET_SUCESS;
error:
        result = DAP_RET_FAIL;
        ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
        return result;
    }

    int DAP_get_model_name(char *model_name, int size)
    {
        int ret = -1;
        char node[PROPERTY_VALUE_MAX];

        ret = property_get("tv.model_name", node, NULL);

        if (ret < 0) {
            snprintf(model_name, size, "DEFAULT");
        } else {
            snprintf(model_name, size, "%s", node);
        }
        ALOGD("%s: Model Name -> %s", __FUNCTION__, model_name);
        return ret;
    }


    int DAP_get_ini_file(char *ini_name, int size)
    {
        int result = -1;
        char model_name[50] = {0};
        IniParser* pIniParser = NULL;
        const char *ini_value = NULL;
        const char *filename = MODEL_SUM_DEFAULT_PATH;

        DAP_get_model_name(model_name, sizeof(model_name));
        pIniParser = new IniParser();
        if (pIniParser->parse(filename) < 0) {
            ALOGW("%s: Load INI file -> %s Failed", __FUNCTION__, filename);
            goto exit;
        }

        ini_value = pIniParser->GetString(model_name, "AMLOGIC_AUDIO_EFFECT_INI_PATH", AUDIO_EFFECT_DEFAULT_PATH);
        if (ini_value == NULL || access(ini_value, F_OK) == -1) {
            ALOGD("%s: INI File is not exist", __FUNCTION__);
            goto exit;
        }
        ALOGD("%s: INI File -> %s", __FUNCTION__, ini_value);
        strncpy(ini_name, ini_value, size);

        result = 0;
exit:
        delete pIniParser;
        pIniParser = NULL;
        return result;
    }

    int DAP_load_ini_file(DAPContext *pContext)
    {
        int result = -1;
        char ini_name[100] = {0};
        const char *ini_value = NULL;
        DAPdata *pDAPdata = &pContext->gDAPdata;
        IniParser* pIniParser = NULL;
        char *Rch = NULL;

        if (DAP_get_ini_file(ini_name, sizeof(ini_name)) < 0) {
            goto error;
        }

        pIniParser = new IniParser();
        if (pIniParser->parse((const char *)ini_name) < 0) {
            ALOGD("%s: %s load failed", __FUNCTION__, ini_name);
            goto error;
        }

        ini_value = pIniParser->GetString("DAP", "dap_enable", "1");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: dap_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->bDapEnabled = atoi(ini_value);

        // dap_effect_mode = 0
        ini_value = pIniParser->GetString("DAP", "dap_effect_mode", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: dap_effect_mode -> %s", __FUNCTION__, ini_value);
        pDAPdata->eDapEffectMode = (DAPmode)atoi(ini_value);

        // dap_vol_leveler = 1 , 0
        ini_value = pIniParser->GetString("DAP", "dap_vol_leveler", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        Rch = (char *)ini_value;
        Rch = strtok(Rch, ",");
        if (Rch == NULL) {
            goto error;
        }
        pDAPdata->dapVolLeveler.vl_enable = atoi(Rch);
        Rch = strtok(NULL, ",");
        if (Rch == NULL) {
            goto error;
        }
        pDAPdata->dapVolLeveler.vl_amount = atoi(Rch);
        ALOGD("%s: dap volume leveler <enable> = %d <amount> = %d", __FUNCTION__, pDAPdata->dapVolLeveler.vl_enable, pDAPdata->dapVolLeveler.vl_amount);

        // dap_dialog_enhance = 1 , 0
        ini_value = pIniParser->GetString("DAP", "dap_dialog_enhance", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        Rch = (char *)ini_value;
        Rch = strtok(Rch, ",");
        if (Rch == NULL) {
            goto error;
        }
        pDAPdata->dapDialogEnhance.de_enable = atoi(Rch);
        Rch = strtok(NULL, ",");
        if (Rch == NULL) {
            goto error;
        }
        pDAPdata->dapDialogEnhance.de_amount = atoi(Rch);
        ALOGD("%s: dap dialog enhancer <enable> = %d <amount> = %d", __FUNCTION__, pDAPdata->dapDialogEnhance.de_enable, pDAPdata->dapDialogEnhance.de_amount);

        // dap_dialog_enhance = 1 , 0
        ini_value = pIniParser->GetString("DAP", "dap_virtual_surround", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        pDAPdata->dapVirtualSrnd.surround_boost = atoi(ini_value);
        ALOGD("%s: dap dap_virtual_surround <enable> = %d <boost> = %d", __FUNCTION__, pDAPdata->dapVirtualSrnd.enable.surround_decoder_enable, pDAPdata->dapVirtualSrnd.surround_boost);

        // dap_GEQ_mode = 1
        ini_value = pIniParser->GetString("DAP", "dap_GEQ_mode", "1");
        if (ini_value == NULL) {
            goto error;
        }
        pDAPdata->dapEQ.eq_enable.geq_enable = (DAPmode)atoi(ini_value);
        ALOGD("%s: dap_GEQ_mode -> %s", __FUNCTION__, ini_value);

        // dap_GEQ_gain
        ini_value = pIniParser->GetString("DAP", "dap_GEQ_gain", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: dap_GEQ_gain -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapEQ.eq_params.a_geq_band_target, pDAPdata->dapEQ.eq_params.geq_nb_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // pregain = 0
        ini_value = pIniParser->GetString("DAP", "pregain", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: pregain -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.pregain = atoi(ini_value);

        // postgain = 0
        ini_value = pIniParser->GetString("DAP", "postgain", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: postgain -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.postgain = atoi(ini_value);

        // systemgain = 0
        ini_value = pIniParser->GetString("DAP", "systemgain", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: systemgain -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.systemgain = atoi(ini_value);

        // headphone_reverb = 0
        ini_value = pIniParser->GetString("DAP", "headphone_reverb", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: headphone_reverb -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.headphone_reverb = atoi(ini_value);

        // speaker_angle = 10
        ini_value = pIniParser->GetString("DAP", "speaker_angle", "10");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: speaker_angle -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.speaker_angle = atoi(ini_value);

        // speaker_start = 20
        ini_value = pIniParser->GetString("DAP", "speaker_start", "20");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: speaker_start -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.speaker_start = atoi(ini_value);

        // mi_ieq_enable = 0
        ini_value = pIniParser->GetString("DAP", "mi_ieq_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: mi_ieq_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.mi_ieq_enable = atoi(ini_value);

        // mi_dv_enable = 0
        ini_value = pIniParser->GetString("DAP", "mi_dv_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: mi_dv_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.mi_dv_enable = atoi(ini_value);

        // mi_de_enable = 0
        ini_value = pIniParser->GetString("DAP", "mi_de_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: mi_de_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.mi_de_enable = atoi(ini_value);

        // mi_surround_enable = 0
        ini_value = pIniParser->GetString("DAP", "mi_surround_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: mi_surround_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.mi_surround_enable = atoi(ini_value);

        // calibration_boost = 0
        ini_value = pIniParser->GetString("DAP", "calibration_boost", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: calibration_boost -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.calibration_boost = atoi(ini_value);

        // leveler_input = -384
        ini_value = pIniParser->GetString("DAP", "leveler_input", "-384");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: leveler_input -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.leveler_input = atoi(ini_value);

        // leveler_output = -384
        ini_value = pIniParser->GetString("DAP", "leveler_output", "-384");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: leveler_output -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.leveler_output = atoi(ini_value);

        // modeler_enable = 0
        ini_value = pIniParser->GetString("DAP", "modeler_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: modeler_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.modeler_enable = atoi(ini_value);

        // modeler_calibration = 0
        ini_value = pIniParser->GetString("DAP", "modeler_calibration", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: modeler_calibration -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.modeler_calibration = atoi(ini_value);

        // ieq_enable = 0
        ini_value = pIniParser->GetString("DAP", "ieq_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ieq_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.ieq_enable = atoi(ini_value);

        // ieq_amount = 10
        ini_value = pIniParser->GetString("DAP", "ieq_amount", "10");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ieq_amount -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.ieq_amount = atoi(ini_value);

        // ieq_nb_bands = 20
        ini_value = pIniParser->GetString("DAP", "ieq_nb_bands", "20");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ieq_nb_bands -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.ieq_nb_bands = atoi(ini_value);

        // a_ieq_band_center
        ini_value = pIniParser->GetString("DAP", "a_ieq_band_center", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: a_ieq_band_center -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.a_ieq_band_center, pDAPdata->dapBaseSetting.ieq_nb_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // a_ieq_band_target
        ini_value = pIniParser->GetString("DAP", "a_ieq_band_target", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: a_ieq_band_target -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.a_ieq_band_target, pDAPdata->dapBaseSetting.ieq_nb_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // de_ducking = 0
        ini_value = pIniParser->GetString("DAP", "de_ducking", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: de_ducking -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.de_ducking = atoi(ini_value);

        // volmax_boost = 0
        ini_value = pIniParser->GetString("DAP", "volmax_boost", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: volmax_boost -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.volmax_boost = atoi(ini_value);

        // optimizer_enable = 0
        ini_value = pIniParser->GetString("DAP", "optimizer_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: optimizer_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.optimizer_enable = atoi(ini_value);

        // ao_bands = 20
        ini_value = pIniParser->GetString("DAP", "ao_bands", "20");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_bands -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.ao_bands = atoi(ini_value);

        // ao_band_center_freq
        ini_value = pIniParser->GetString("DAP", "ao_band_center_freq", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: a_ieq_band_center -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_center_freq, pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ao_band_gains_ch1
        ini_value = pIniParser->GetString("DAP", "ao_band_gains_ch1", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_band_gains_ch1 -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_gains[0], pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ao_band_gains_ch2
        ini_value = pIniParser->GetString("DAP", "ao_band_gains_ch2", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_band_gains_ch2 -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_gains[1], pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ao_band_gains_ch3
        ini_value = pIniParser->GetString("DAP", "ao_band_gains_ch3", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_band_gains_ch3 -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_gains[2], pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ao_band_gains_ch4
        ini_value = pIniParser->GetString("DAP", "ao_band_gains_ch4", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_band_gains_ch4 -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_gains[3], pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ao_band_gains_ch5
        ini_value = pIniParser->GetString("DAP", "ao_band_gains_ch5", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_band_gains_ch5 -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_gains[4], pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ao_band_gains_ch6
        ini_value = pIniParser->GetString("DAP", "ao_band_gains_ch6", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ao_band_gains_ch6 -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ao_band_gains[5], pDAPdata->dapBaseSetting.ao_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // bass_enable = 0
        ini_value = pIniParser->GetString("DAP", "bass_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: bass_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.bass_enable = atoi(ini_value);

        // bass_boost = 0
        ini_value = pIniParser->GetString("DAP", "bass_boost", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: bass_boost -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.bass_boost = atoi(ini_value);

        // bass_cutoff = 0
        ini_value = pIniParser->GetString("DAP", "bass_cutoff", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: bass_cutoff -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.bass_cutoff = atoi(ini_value);

        // bass_width = 0
        ini_value = pIniParser->GetString("DAP", "bass_width", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: bass_width -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.bass_width = atoi(ini_value);

        // ar_bands = 20
        ini_value = pIniParser->GetString("DAP", "ar_bands", "20");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ar_bands -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.ar_bands = atoi(ini_value);

        // ar_band_center_freq
        ini_value = pIniParser->GetString("DAP", "ar_band_center_freq", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ar_band_center_freq -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ar_band_center_freq, pDAPdata->dapBaseSetting.ar_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ar_low_thresholds
        ini_value = pIniParser->GetString("DAP", "ar_low_thresholds", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ar_low_thresholds -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ar_low_thresholds, pDAPdata->dapBaseSetting.ar_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ar_high_thresholds
        ini_value = pIniParser->GetString("DAP", "ar_high_thresholds", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ar_high_thresholds -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ar_high_thresholds, pDAPdata->dapBaseSetting.ar_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // ar_isolated_bands
        ini_value = pIniParser->GetString("DAP", "ar_isolated_bands", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: ar_isolated_bands -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.ar_isolated_bands, pDAPdata->dapBaseSetting.ar_bands, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // regulator_overdrive = 0
        ini_value = pIniParser->GetString("DAP", "regulator_overdrive", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: regulator_overdrive -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.regulator_overdrive = atoi(ini_value);

        // regulator_timbre = 12
        ini_value = pIniParser->GetString("DAP", "regulator_timbre", "12");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: regulator_timbre -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.regulator_timbre = atoi(ini_value);

        // regulator_distortion = 96
        ini_value = pIniParser->GetString("DAP", "regulator_distortion", "96");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: regulator_distortion -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.regulator_distortion = atoi(ini_value);

        // regulator_mode = 0
        ini_value = pIniParser->GetString("DAP", "regulator_mode", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: regulator_mode -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.regulator_mode = atoi(ini_value);

        // regulator_enable = 0
        ini_value = pIniParser->GetString("DAP", "regulator_enable", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: regulator_enable -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.regulator_enable = atoi(ini_value);

        // virtual_bass_mode = 0
        ini_value = pIniParser->GetString("DAP", "virtual_bass_mode", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_mode -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_mode = atoi(ini_value);

        // virtual_bass_low_src_freq = 35
        ini_value = pIniParser->GetString("DAP", "virtual_bass_low_src_freq", "35");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_low_src_freq -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_mode = atoi(ini_value);

        // virtual_bass_high_src_freq = 160
        ini_value = pIniParser->GetString("DAP", "virtual_bass_high_src_freq", "160");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_high_src_freq -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_high_src_freq = atoi(ini_value);

        // virtual_bass_overall_gain = 0
        ini_value = pIniParser->GetString("DAP", "virtual_bass_overall_gain", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_overall_gain -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_overall_gain = atoi(ini_value);

        // virtual_bass_slope_gain = 0
        ini_value = pIniParser->GetString("DAP", "virtual_bass_slope_gain", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_slope_gain -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_slope_gain = atoi(ini_value);

        // virtual_bass_subgain
        ini_value = pIniParser->GetString("DAP", "virtual_bass_subgain", "NULL");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_subgain -> %s", __FUNCTION__, ini_value);
        result = DAP_ini_parse_array(pDAPdata->dapBaseSetting.virtual_bass_subgain, 3, ini_value);
        if (result == DAP_RET_FAIL) {
            goto error;
        }

        // virtual_bass_mix_low_freq = 0
        ini_value = pIniParser->GetString("DAP", "virtual_bass_mix_low_freq", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_mix_low_freq -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_mix_low_freq = atoi(ini_value);

        // virtual_bass_mix_high_freq = 0
        ini_value = pIniParser->GetString("DAP", "virtual_bass_mix_high_freq", "0");
        if (ini_value == NULL) {
            goto error;
        }
        ALOGD("%s: virtual_bass_mix_high_freq -> %s", __FUNCTION__, ini_value);
        pDAPdata->dapBaseSetting.virtual_bass_mix_high_freq = atoi(ini_value);

        result = 0;
error:
        ALOGD("%s: %s", __FUNCTION__, result == 0 ? "sucessful" : "failed");
        delete pIniParser;
        pIniParser = NULL;
        return result;
    }



    int DAP_unload_lib(DAPContext *pContext)
    {
        memset(&pContext->gDAPapi, 0, sizeof(pContext->gDAPapi));
        if (pContext->gDAPLibHandler) {
            dlclose(pContext->gDAPLibHandler);
            pContext->gDAPLibHandler = NULL;
        }
        return 0;
    }

    int DAP_load_lib(DAPContext *pContext)
    {
        DAPapi *pDAPapi;
        pDAPapi = (DAPapi *) & (pContext->gDAPapi);

        if (NULL == pContext) {
            ALOGE("%s, pContext == NULL\n", __FUNCTION__);
            goto Error;
        }

        ALOGD("%s, entering...\n", __FUNCTION__);

        pContext->gDAPLibHandler = dlopen(LIBDAP_PATH_A, RTLD_NOW);
        if (!pContext->gDAPLibHandler) {
            ALOGE("%s, failed to load DAP lib %s\n", __FUNCTION__, LIBDAP_PATH_A);
            pContext->gDAPLibHandler = dlopen(LIBDAP_PATH_B, RTLD_NOW);
            if (!pContext->gDAPLibHandler) {
                ALOGE("%s, failed to load DAP lib %s err = %s\n", __FUNCTION__, LIBDAP_PATH_B, dlerror());
                goto Error;
            }
            ALOGD("<%s::%d>--%s[gDAPLibHandler:0x%p]", __FUNCTION__, __LINE__, LIBDAP_PATH_B, pContext->gDAPLibHandler);
        } else {
            ALOGD("<%s::%d>--%s[gDAPLibHandler:0x%p]", __FUNCTION__, __LINE__, LIBDAP_PATH_A, pContext->gDAPLibHandler);
        }

        //int (*DAP_get_chip_ms12_license)(void);
        pDAPapi->DAP_get_chip_ms12_license = (int(*)(void))dlsym(pContext->gDAPLibHandler, "dap_get_chip_ms12_license");
        if (!pDAPapi->DAP_get_chip_ms12_license) {
            ALOGE("%s: find func get_chip_ms12_license() failed\n", __FUNCTION__);
            goto Error;
        }

        //unsigned (*DAP_cpdp_get_latency)(void *);
        pDAPapi->DAP_cpdp_get_latency = (unsigned(*)(void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_get_latency");
        if (!pDAPapi->DAP_cpdp_get_latency) {
            ALOGE("%s: find func dap_cpdp_get_latency() failed\n", __FUNCTION__);
            goto Error;
        }

        //int (*DAP_cpdp_pvt_device_processing_supported)(int);
        pDAPapi->DAP_cpdp_pvt_device_processing_supported = (int (*)(int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_device_processing_supported");
        if (pDAPapi->DAP_cpdp_pvt_device_processing_supported == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_device_processing_supported:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_device_processing_supported);
        }

        //int (*DAP_cpdp_pvt_dual_virtualizer_supported)(int);
        pDAPapi->DAP_cpdp_pvt_dual_virtualizer_supported = (int (*)(int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_dual_virtualizer_supported");
        if (pDAPapi->DAP_cpdp_pvt_dual_virtualizer_supported == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_dual_virtualizer_supported:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_dual_virtualizer_supported);
        }

        //int (*DAP_cpdp_pvt_dual_stereo_enabled)(unsigned , int);
        pDAPapi->DAP_cpdp_pvt_dual_stereo_enabled = (int (*)(unsigned , int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_dual_stereo_enabled");
        if (pDAPapi->DAP_cpdp_pvt_dual_stereo_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_dual_stereo_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_dual_stereo_enabled);
        }

        //unsigned (*DAP_cpdp_pvt_post_upmixer_channels)(const void *);
        pDAPapi->DAP_cpdp_pvt_post_upmixer_channels = (unsigned(*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_post_upmixer_channels");
        if (pDAPapi->DAP_cpdp_pvt_post_upmixer_channels == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_post_upmixer_channels:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_post_upmixer_channels);
        }

        //unsigned (*DAP_cpdp_pvt_content_sidechain_channels)(const void *);
        pDAPapi->DAP_cpdp_pvt_content_sidechain_channels = (unsigned(*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_content_sidechain_channels");
        if (pDAPapi->DAP_cpdp_pvt_content_sidechain_channels == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_content_sidechain_channels:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_content_sidechain_channels);
        }

        //int (*DAP_cpdp_pvt_surround_compressor_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_surround_compressor_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_surround_compressor_enabled");
        if (pDAPapi->DAP_cpdp_pvt_surround_compressor_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_surround_compressor_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_surround_compressor_enabled);
        }

        //int (*DAP_cpdp_pvt_content_sidechain_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_content_sidechain_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_content_sidechain_enabled");
        if (pDAPapi->DAP_cpdp_pvt_content_sidechain_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_content_sidechain_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_content_sidechain_enabled);
        }

        //int (*DAP_cpdp_pvt_ngcs_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_ngcs_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_ngcs_enabled");
        if (pDAPapi->DAP_cpdp_pvt_ngcs_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_ngcs_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_ngcs_enabled);
        }

        //int (*DAP_cpdp_pvt_audio_optimizer_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_audio_optimizer_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_audio_optimizer_enabled");
        if (pDAPapi->DAP_cpdp_pvt_audio_optimizer_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_audio_optimizer_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_audio_optimizer_enabled);
        }

        //size_t (*DAP_cpdp_query_memory)(const dap_cpdp_init_info *);
        pDAPapi->DAP_cpdp_query_memory = (size_t (*)(const dap_cpdp_init_info *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_query_memory");
        if (pDAPapi->DAP_cpdp_query_memory == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_query_memory:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_query_memory);
        }

        //size_t (*DAP_cpdp_query_scratch)(const dap_cpdp_init_info *);
        pDAPapi->DAP_cpdp_query_scratch = (size_t (*)(const dap_cpdp_init_info *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_query_scratch");
        if (pDAPapi->DAP_cpdp_query_scratch == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_query_scratch:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_query_scratch);
        }


        //void *(*DAP_cpdp_init)(const dap_cpdp_init_info *, void *);
        pDAPapi->DAP_cpdp_init = (void * (*)(const dap_cpdp_init_info *, void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_init");
        if (pDAPapi->DAP_cpdp_init == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_init:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_init);
        }


        //void (*DAP_cpdp_shutdown)(void *);
        pDAPapi->DAP_cpdp_shutdown = (void (*)(void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_shutdown");
        if (pDAPapi->DAP_cpdp_shutdown == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_shutdown:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_shutdown);
        }

        /* This function will be called from the same thread as the process function.
         * It must never block. */
        //unsigned (*DAP_cpdp_prepare)(void *, const dlb_buffer *, const dap_cpdp_metadata *, const dap_cpdp_mix_data *);
        pDAPapi->DAP_cpdp_prepare = (unsigned(*)(void *, const dlb_buffer *, const dap_cpdp_metadata *, const dap_cpdp_mix_data *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_prepare");
        if (pDAPapi->DAP_cpdp_prepare == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_prepare:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_prepare);
        }

        //dap_cpdp_metadata (*DAP_cpdp_process) (void *, const dlb_buffer *, void *);

        pDAPapi->dap_cpdp_process_api = (dap_cpdp_metadata(*)(void *, const dlb_buffer *, void *, int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_process_api");
        if (pDAPapi->dap_cpdp_process_api == NULL) {
            ALOGE("%s,cant find dap_cpdp_process_api,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[dap_cpdp_process_api:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->dap_cpdp_process_api);
        }


        /* Output channel count */
        //void (*DAP_cpdp_output_mode_set)(void *, int);
        pDAPapi->DAP_cpdp_output_mode_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_output_mode_set");
        if (pDAPapi->DAP_cpdp_output_mode_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_output_mode_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_output_mode_set);
        }

        /*Dialog Enhancer enable setting */
        //void (*DAP_cpdp_de_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_de_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_de_enable_set");
        if (pDAPapi->DAP_cpdp_de_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_de_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_de_enable_set);
        }

        /* Dialog Enhancer amount setting */
        //void (*DAP_cpdp_de_amount_set)(void *, int);
        pDAPapi->DAP_cpdp_de_amount_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_de_amount_set");
        if (pDAPapi->DAP_cpdp_de_amount_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_de_amount_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_de_amount_set);
        }

        /* Dialog Enhancement ducking setting */
        //void (*DAP_cpdp_de_ducking_set)(void *, int);
        pDAPapi->DAP_cpdp_de_ducking_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_de_ducking_set");
        if (pDAPapi->DAP_cpdp_de_ducking_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_de_ducking_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_de_ducking_set);
        }

        /* Surround boost setting */
        //void (*DAP_cpdp_surround_boost_set)(void *, int);
        pDAPapi->DAP_cpdp_surround_boost_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_surround_boost_set");
        if (pDAPapi->DAP_cpdp_surround_boost_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_surround_boost_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_surround_boost_set);
        }

        /* Surround Decoder enable setting */
        //void (*DAP_cpdp_surround_decoder_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_surround_decoder_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_surround_decoder_enable_set");
        if (pDAPapi->DAP_cpdp_surround_decoder_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_surround_decoder_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_surround_decoder_enable_set);
        }

        /* Graphic Equalizer enable setting */
        //void (*DAP_cpdp_graphic_equalizer_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_graphic_equalizer_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_graphic_equalizer_enable_set");
        if (pDAPapi->DAP_cpdp_graphic_equalizer_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_graphic_equalizer_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_graphic_equalizer_enable_set);
        }

        /* Graphic Equalizer bands setting */
        //void (*DAP_cpdp_graphic_equalizer_bands_set)(void *, unsigned int, const unsigned int *, const int *);
        pDAPapi->DAP_cpdp_graphic_equalizer_bands_set = (void (*)(void *, unsigned int, const unsigned int *, const int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_graphic_equalizer_bands_set");
        if (pDAPapi->DAP_cpdp_graphic_equalizer_bands_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_graphic_equalizer_bands_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_graphic_equalizer_bands_set);
        }

        /* Audio optimizer enable setting */
        //void (*DAP_cpdp_audio_optimizer_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_audio_optimizer_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_audio_optimizer_enable_set");
        if (pDAPapi->DAP_cpdp_audio_optimizer_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_audio_optimizer_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_audio_optimizer_enable_set);
        }

        /* Audio optimizer bands setting */
        //void (*DAP_cpdp_audio_optimizer_bands_set)(void *, unsigned int, const unsigned int *, int **);
        pDAPapi->DAP_cpdp_audio_optimizer_bands_set = (void (*)(void *, unsigned int, const unsigned int *, int **))dlsym(pContext->gDAPLibHandler, "dap_cpdp_audio_optimizer_bands_set");
        if (pDAPapi->DAP_cpdp_audio_optimizer_bands_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_audio_optimizer_bands_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_audio_optimizer_bands_set);
        }

        /* Bass enhancer enable setting */
        //void (*DAP_cpdp_bass_enhancer_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_bass_enhancer_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_bass_enhancer_enable_set");
        if (pDAPapi->DAP_cpdp_bass_enhancer_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_bass_enhancer_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_bass_enhancer_enable_set);
        }

        //void (*DAP_cpdp_bass_enhancer_boost_set)(void *, int);
        pDAPapi->DAP_cpdp_bass_enhancer_boost_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_bass_enhancer_boost_set");
        if (pDAPapi->DAP_cpdp_bass_enhancer_boost_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_bass_enhancer_boost_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_bass_enhancer_boost_set);
        }

        /* Bass enhancer cutoff frequency setting */
        //void (*DAP_cpdp_bass_enhancer_cutoff_frequency_set)(void *, unsigned);
        pDAPapi->DAP_cpdp_bass_enhancer_cutoff_frequency_set = (void (*)(void *, unsigned))dlsym(pContext->gDAPLibHandler, "dap_cpdp_bass_enhancer_cutoff_frequency_set");
        if (pDAPapi->DAP_cpdp_bass_enhancer_cutoff_frequency_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_bass_enhancer_cutoff_frequency_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_bass_enhancer_cutoff_frequency_set);
        }

        /* Bass enhancer width setting */
        //void (*DAP_cpdp_bass_enhancer_width_set)(void *, int);
        pDAPapi->DAP_cpdp_bass_enhancer_width_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_bass_enhancer_width_set");
        if (pDAPapi->DAP_cpdp_bass_enhancer_width_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_bass_enhancer_width_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_bass_enhancer_width_set);
        }

        /* Visualizer bands get */
        //void (*DAP_cpdp_vis_bands_get)(void *, unsigned int *, unsigned int *, int *, int *);
        pDAPapi->DAP_cpdp_vis_bands_get = (void (*)(void *, unsigned int *, unsigned int *, int *, int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_vis_bands_get");
        if (pDAPapi->DAP_cpdp_vis_bands_get == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_vis_bands_get:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_vis_bands_get);
        }

        /* Visualizer custom bands get */
        //void (*DAP_cpdp_vis_custom_bands_get)(void *, unsigned int, const unsigned int *, int *, int *);
        pDAPapi->DAP_cpdp_vis_custom_bands_get = (void (*)(void *, unsigned int, const unsigned int *, int *, int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_vis_custom_bands_get");
        if (pDAPapi->DAP_cpdp_vis_custom_bands_get == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_vis_custom_bands_get:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_vis_custom_bands_get);
        }

        /* Audio Regulator Overdrive setting */
        //void (*DAP_cpdp_regulator_overdrive_set)(void *, int);
        pDAPapi->DAP_cpdp_regulator_overdrive_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_overdrive_set");
        if (pDAPapi->DAP_cpdp_regulator_overdrive_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_overdrive_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_overdrive_set);
        }

        /* Audio Regulator Timbre Preservation setting */
        //void (*DAP_cpdp_regulator_timbre_preservation_set)(void *, int);
        pDAPapi->DAP_cpdp_regulator_timbre_preservation_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_timbre_preservation_set");
        if (pDAPapi->DAP_cpdp_regulator_timbre_preservation_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_timbre_preservation_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_timbre_preservation_set);
        }

        /* Audio Regulator Distortion Relaxation Amount setting */
        //void (*DAP_cpdp_regulator_relaxation_amount_set)(void *, int);
        pDAPapi->DAP_cpdp_regulator_relaxation_amount_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_relaxation_amount_set");
        if (pDAPapi->DAP_cpdp_regulator_relaxation_amount_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_relaxation_amount_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_relaxation_amount_set);
        }

        /* Audio Regulator Distortion Operating Mode setting */
        //void (*DAP_cpdp_regulator_speaker_distortion_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_regulator_speaker_distortion_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_speaker_distortion_enable_set");
        if (pDAPapi->DAP_cpdp_regulator_speaker_distortion_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_speaker_distortion_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_speaker_distortion_enable_set);
        }

        /* Audio Regulator Enable setting */
        //void (*DAP_cpdp_regulator_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_regulator_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_enable_set");
        if (pDAPapi->DAP_cpdp_regulator_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_enable_set);
        }

        /* Audio Regulator tuning setting */
        //void (*DAP_cpdp_regulator_tuning_set)(void *, unsigned, const unsigned *, const int *, const int *, const int *);
        pDAPapi->DAP_cpdp_regulator_tuning_set = (void (*)(void *, unsigned, const unsigned *, const int *, const int *, const int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_tuning_set");
        if (pDAPapi->DAP_cpdp_regulator_tuning_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_tuning_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_tuning_set);
        }

        /* Audio Regulator tuning info get */
        //void (*DAP_cpdp_regulator_tuning_info_get)(void *, unsigned int *, int *, int *);
        pDAPapi->DAP_cpdp_regulator_tuning_info_get = (void (*)(void *, unsigned int *, int *, int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_regulator_tuning_info_get");
        if (pDAPapi->DAP_cpdp_regulator_tuning_info_get == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_regulator_tuning_info_get:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_regulator_tuning_info_get);
        }

        /* Virtual Bass mode setting */
        //void (*DAP_cpdp_virtual_bass_mode_set)(void *, int);
        pDAPapi->DAP_cpdp_virtual_bass_mode_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtual_bass_mode_set");
        if (pDAPapi->DAP_cpdp_virtual_bass_mode_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtual_bass_mode_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtual_bass_mode_set);
        }

        /* Virtual Bass source frequency boundaries setting */
        //void (*DAP_cpdp_virtual_bass_src_freqs_set)(void *, unsigned, unsigned);
        pDAPapi->DAP_cpdp_virtual_bass_src_freqs_set = (void (*)(void *, unsigned, unsigned))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtual_bass_src_freqs_set");
        if (pDAPapi->DAP_cpdp_virtual_bass_src_freqs_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtual_bass_src_freqs_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtual_bass_src_freqs_set);
        }

        //void (*DAP_cpdp_virtual_bass_overall_gain_set)(void *, int);
        pDAPapi->DAP_cpdp_virtual_bass_overall_gain_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtual_bass_overall_gain_set");
        if (pDAPapi->DAP_cpdp_virtual_bass_overall_gain_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtual_bass_overall_gain_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtual_bass_overall_gain_set);
        }

        //void (*DAP_cpdp_virtual_bass_slope_gain_set)(void *, int);
        pDAPapi->DAP_cpdp_virtual_bass_slope_gain_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtual_bass_slope_gain_set");
        if (pDAPapi->DAP_cpdp_virtual_bass_slope_gain_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtual_bass_slope_gain_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtual_bass_slope_gain_set);
        }

        //void (*DAP_cpdp_virtual_bass_subgains_set)(void *, unsigned, int *);
        pDAPapi->DAP_cpdp_virtual_bass_subgains_set = (void (*)(void *, unsigned, int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtual_bass_subgains_set");
        if (pDAPapi->DAP_cpdp_virtual_bass_subgains_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtual_bass_subgains_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtual_bass_subgains_set);
        }

        //void (*DAP_cpdp_virtual_bass_mix_freqs_set)(void *, unsigned, unsigned);
        pDAPapi->DAP_cpdp_virtual_bass_mix_freqs_set = (void (*)(void *, unsigned, unsigned))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtual_bass_mix_freqs_set");
        if (pDAPapi->DAP_cpdp_virtual_bass_mix_freqs_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtual_bass_mix_freqs_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtual_bass_mix_freqs_set);
        }

        //int (*DAP_cpdp_ieq_bands_set)(void *, unsigned int, const unsigned int *, const int *);
        pDAPapi->DAP_cpdp_ieq_bands_set = (int (*)(void *, unsigned int, const unsigned int *, const int *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_ieq_bands_set");
        if (pDAPapi->DAP_cpdp_ieq_bands_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_ieq_bands_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_ieq_bands_set);
        }
        //int (*DAP_cpdp_ieq_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_ieq_enable_set = (int (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_ieq_enable_set");
        if (pDAPapi->DAP_cpdp_ieq_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_ieq_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_ieq_enable_set);
        }
        //int (*DAP_cpdp_volume_leveler_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_volume_leveler_enable_set = (int (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volume_leveler_enable_set");
        if (pDAPapi->DAP_cpdp_volume_leveler_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volume_leveler_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volume_leveler_enable_set);
        }
        //int (*DAP_cpdp_volume_modeler_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_volume_modeler_enable_set = (int (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volume_modeler_enable_set");
        if (pDAPapi->DAP_cpdp_volume_modeler_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volume_modeler_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volume_modeler_enable_set);
        }

        /*---------------------------------------------------------------------------------*/
        /*DAP_CPDP_DV_PARAMS*/
        /*---------------------------------------------------------------------------------*/
        //size_t (*DAP_cpdp_pvt_dv_params_query_memory)(void);
        pDAPapi->DAP_cpdp_pvt_dv_params_query_memory = (size_t (*)(void))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_dv_params_query_memory");
        if (pDAPapi->DAP_cpdp_pvt_dv_params_query_memory == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_dv_params_query_memory:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_dv_params_query_memory);
        }

        //void (*DAP_cpdp_pvt_dv_params_init)(void *, const unsigned, unsigned, void);
        pDAPapi->DAP_cpdp_pvt_dv_params_init = (void (*)(void *, const unsigned *, unsigned, void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_dv_params_init");
        if (pDAPapi->DAP_cpdp_pvt_dv_params_init == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_dv_params_init:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_dv_params_init);
        }

        //void (*DAP_cpdp_pvt_volume_and_ieq_update_control)(void *, unsigned);
        pDAPapi->DAP_cpdp_pvt_volume_and_ieq_update_control = (void (*)(void *, unsigned))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_volume_and_ieq_update_control");
        if (pDAPapi->DAP_cpdp_pvt_volume_and_ieq_update_control == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_volume_and_ieq_update_control:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_volume_and_ieq_update_control);
        }

        //int (*DAP_cpdp_pvt_dv_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_dv_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_dv_enabled");
        if (pDAPapi->DAP_cpdp_pvt_dv_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_dv_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_dv_enabled);
        }

        //int (*DAP_cpdp_pvt_async_dv_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_async_dv_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_async_dv_enabled");
        if (pDAPapi->DAP_cpdp_pvt_async_dv_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_async_dv_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_async_dv_enabled);
        }

        //int (*DAP_cpdp_pvt_async_leveler_or_modeler_enabled)(const void *);
        pDAPapi->DAP_cpdp_pvt_async_leveler_or_modeler_enabled = (int (*)(const void *))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pvt_async_leveler_or_modeler_enabled");
        if (pDAPapi->DAP_cpdp_pvt_async_leveler_or_modeler_enabled == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pvt_async_leveler_or_modeler_enabled:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pvt_async_leveler_or_modeler_enabled);
        }

        //void (*DAP_cpdp_volume_modeler_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_system_gain_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_system_gain_set");
        if (pDAPapi->DAP_cpdp_system_gain_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_system_gain_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_system_gain_set);
        }

        //int (*DAP_cpdp_virtualizer_headphone_reverb_gain_set)(void *, int);
        pDAPapi->DAP_cpdp_virtualizer_headphone_reverb_gain_set = (int (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtualizer_headphone_reverb_gain_set");
        if (pDAPapi->DAP_cpdp_virtualizer_headphone_reverb_gain_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtualizer_headphone_reverb_gain_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtualizer_headphone_reverb_gain_set);
        }

        //int (*DAP_cpdp_volume_leveler_amount_set)(void *, int);
        pDAPapi->DAP_cpdp_volume_leveler_amount_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volume_leveler_amount_set");
        if (pDAPapi->DAP_cpdp_volume_leveler_amount_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volume_leveler_amount_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volume_leveler_amount_set);
        }

        //int (*DAP_cpdp_ieq_amount_set)(void *, int);
        pDAPapi->DAP_cpdp_ieq_amount_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_ieq_amount_set");
        if (pDAPapi->DAP_cpdp_ieq_amount_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_ieq_amount_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_ieq_amount_set);
        }

        //void (*DAP_cpdp_virtualizer_speaker_angle_set)(void *,unsigned int);
        pDAPapi->DAP_cpdp_virtualizer_speaker_angle_set = (void (*)(void *, unsigned int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtualizer_speaker_angle_set");
        if (pDAPapi->DAP_cpdp_virtualizer_speaker_angle_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtualizer_speaker_angle_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtualizer_speaker_angle_set);
        }

        //void (*DAP_cpdp_virtualizer_speaker_start_freq_set)(void *,unsigned int);
        pDAPapi->DAP_cpdp_virtualizer_speaker_start_freq_set = (void (*)(void   *, unsigned int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_virtualizer_speaker_start_freq_set");
        if (pDAPapi->DAP_cpdp_virtualizer_speaker_start_freq_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_virtualizer_speaker_start_freq_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_virtualizer_speaker_start_freq_set);
        }

        //void (*DAP_cpdp_mi2ieq_steering_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_mi2ieq_steering_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_mi2ieq_steering_enable_set");
        if (pDAPapi->DAP_cpdp_mi2ieq_steering_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_mi2ieq_steering_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_mi2ieq_steering_enable_set);
        }

        //void (*DAP_cpdp_mi2dv_leveler_steering_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_mi2dv_leveler_steering_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_mi2dv_leveler_steering_enable_set");
        if (pDAPapi->DAP_cpdp_mi2dv_leveler_steering_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_mi2dv_leveler_steering_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_mi2dv_leveler_steering_enable_set);
        }

        //void (*DAP_cpdp_mi2dialog_enhancer_steering_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_mi2dialog_enhancer_steering_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_mi2dialog_enhancer_steering_enable_set");
        if (pDAPapi->DAP_cpdp_mi2dialog_enhancer_steering_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_mi2dialog_enhancer_steering_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_mi2dialog_enhancer_steering_enable_set);
        }

        //void (*DAP_cpdp_mi2surround_compressor_steering_enable_set)(void *, int);
        pDAPapi->DAP_cpdp_mi2surround_compressor_steering_enable_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_mi2surround_compressor_steering_enable_set");
        if (pDAPapi->DAP_cpdp_mi2surround_compressor_steering_enable_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_mi2surround_compressor_steering_enable_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_mi2surround_compressor_steering_enable_set);
        }

        //void (*DAP_cpdp_calibration_boost_set)(void *, int);
        pDAPapi->DAP_cpdp_calibration_boost_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_calibration_boost_set");
        if (pDAPapi->DAP_cpdp_calibration_boost_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_calibration_boost_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_calibration_boost_set);
        }

        //void (*DAP_cpdp_volume_leveler_in_target_set)(void *, int);
        pDAPapi->DAP_cpdp_volume_leveler_in_target_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volume_leveler_in_target_set");
        if (pDAPapi->DAP_cpdp_volume_leveler_in_target_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volume_leveler_in_target_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volume_leveler_in_target_set);
        }

        //void (*DAP_cpdp_volume_leveler_out_target_set)(void *, int);
        pDAPapi->DAP_cpdp_volume_leveler_out_target_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volume_leveler_out_target_set");
        if (pDAPapi->DAP_cpdp_volume_leveler_out_target_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volume_leveler_out_target_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volume_leveler_out_target_set);
        }

        //void (*DAP_cpdp_volume_modeler_calibration_set)(void *, int);
        pDAPapi->DAP_cpdp_volume_modeler_calibration_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volume_modeler_calibration_set");
        if (pDAPapi->DAP_cpdp_volume_modeler_calibration_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volume_modeler_calibration_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volume_modeler_calibration_set);
        }

        //void (*DAP_cpdp_pregain_set)(void *, int);
        pDAPapi->DAP_cpdp_pregain_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_pregain_set");
        if (pDAPapi->DAP_cpdp_pregain_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_pregain_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_pregain_set);
        }

        //void (*DAP_cpdp_postgain_set)(void *, int);
        pDAPapi->DAP_cpdp_postgain_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_postgain_set");
        if (pDAPapi->DAP_cpdp_postgain_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_postgain_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_postgain_set);
        }

        //void (*DAP_cpdp_volmax_boost_set)(void   *, int);
        pDAPapi->DAP_cpdp_volmax_boost_set = (void (*)(void *, int))dlsym(pContext->gDAPLibHandler, "dap_cpdp_volmax_boost_set");
        if (pDAPapi->DAP_cpdp_volmax_boost_set == NULL) {
            ALOGE("%s,cant find decoder lib,%s\n", __FUNCTION__, dlerror());
            goto Error;
        } else {
            LOGFUNC("<%s::%d>--[DAP_cpdp_volmax_boost_set:0x%x]", __FUNCTION__, __LINE__, (unsigned int)pDAPapi->DAP_cpdp_volmax_boost_set);
        }


        ALOGD("%s: sucessful", __FUNCTION__);

        return 0;
Error:
        memset(&pContext->gDAPapi, 0, sizeof(pContext->gDAPapi));
        dlclose(pContext->gDAPLibHandler);
        pContext->gDAPLibHandler = NULL;
        return -EINVAL;
    }

    int DAP_init(DAPContext *pContext)
    {
        DAPdata *pDapData = &pContext->gDAPdata;
        DAPapi *pDAPapi = (DAPapi *) & (pContext->gDAPapi);

        pContext->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
        pContext->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
        pContext->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        pContext->config.inputCfg.samplingRate = 48000;
        pContext->config.inputCfg.bufferProvider.getBuffer = NULL;
        pContext->config.inputCfg.bufferProvider.releaseBuffer = NULL;
        pContext->config.inputCfg.bufferProvider.cookie = NULL;
        pContext->config.inputCfg.mask = EFFECT_CONFIG_ALL;
        pContext->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
        pContext->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
        pContext->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        pContext->config.outputCfg.samplingRate = 48000;
        pContext->config.outputCfg.bufferProvider.getBuffer = NULL;
        pContext->config.outputCfg.bufferProvider.releaseBuffer = NULL;
        pContext->config.outputCfg.bufferProvider.cookie = NULL;
        pContext->config.outputCfg.mask = EFFECT_CONFIG_ALL;

        //DAP_print_base_setting(&pDapData->dapBaseSetting);

        /*
            ALOGI("%s, InputCfg: accessMode[0x%x],channels[0x%x](%d chs),format[0x%x],SampleRate[0x%x]\n",__FUNCTION__,
                   pContext->config.inputCfg.accessMode,
                   pContext->config.inputCfg.channels,in_channel_cnt,
                   pContext->config.inputCfg.format,
                   pContext->config.inputCfg.samplingRate);

            ALOGI("%s, outputCfg: accessMode[0x%x],channels[0x%x](%d chs),format[0x%x],SampleRate[0x%x]\n",__FUNCTION__,
                   pContext->config.outputCfg.accessMode,
                   pContext->config.outputCfg.channels,in_channel_cnt,
                   pContext->config.outputCfg.format,
                   pContext->config.outputCfg.samplingRate);
        */


        /*
        ret = dap_init_api(pContext);
        if (ret == DAP_RET_FAIL) {
            return DAP_RET_FAIL;
        }

        // Initializing default setting
        aml_dap_cpdp_output_mode_set(pDAPapi,pDapData->dap_cpdp, dap_out_mode);
        dap_set_effect_mode(pContext,pDapData->eDapEffectMode);
        dap_set_postgain(pContext,pDapData->dapPostGain);
        dap_set_vol_leveler(pContext,(dolby_vol_leveler_t *)&pDapData->dapVolLeveler);
        dap_set_dialog_enhance(pContext,(dolby_dialog_enhance_t *)&pDapData->dapDialogEnhance);
        dap_set_eq_params(pContext,(dolby_eq_t *)&pDapData->dapEQ);
        dap_set_virtual_surround(pContext,(dolby_virtual_surround_t *)&pDapData->dapVirtualSrnd);
        dap_set_enable(pContext,pDapData->bDapEnabled);
        */

        ALOGD("%s: sucessful", __FUNCTION__);

        return 0;
    }

    int DAP_reset(DAPContext *pContext)
    {
        DAPapi *pDAPapi = (DAPapi *) & (pContext->gDAPapi);
        if (pContext->gDAPLibHandler) {
            return -EINVAL;
        }

        dap_release_api(pContext);
        dap_init_api(pContext);

        // maybe we don't need to restore all thing to initial value
        if (0) {
            pContext->modeValue = DAP_MODE_MUSIC;
            pContext->gDAPdata.bDapEnabled = 1;
            pContext->gDAPdata.dapCPDPOutputMode = DAP_CPDP_OUTPUT_2_SPEAKER;
            pContext->gDAPdata.dapPostGain = DEFAULT_POSTGAIN;
            pContext->gDAPdata.eDapEffectMode = (DAPmode)pContext->modeValue;
            memcpy((void *) & (pContext->gDAPdata.dapBaseSetting), (void *)&dap_dolby_base_music, sizeof(dolby_base));
            dap_get_mode_param(pContext, DAP_PARAM_VOL_LEVELER_ENABLE, (void *)&pContext->gDAPdata.dapVolLeveler, true);
            dap_get_mode_param(pContext, DAP_PARAM_DE_ENABLE, (void *)&pContext->gDAPdata.dapDialogEnhance, true);
            dap_get_mode_param(pContext, DAP_PARAM_SUROUND_ENABLE, (void *)&pContext->gDAPdata.dapVirtualSrnd, true);
            dap_get_mode_param(pContext, DAP_PARAM_GEQ_ENABLE, (void *)&pContext->gDAPdata.dapEQ, true);

            void *dap_cpdp = NULL;
            DAPdata *pDapData = &pContext->gDAPdata;
            dap_cpdp = pDapData->dap_cpdp;
            if (!dap_cpdp) {
                return -EINVAL;
            }
            // Initializing default setting
            aml_dap_cpdp_output_mode_set(pDAPapi, pDapData->dap_cpdp, pContext->gDAPdata.dapCPDPOutputMode);
            dap_set_effect_mode(pContext, pDapData->eDapEffectMode);
            dap_set_postgain(pContext, pDapData->dapPostGain);
            dap_set_vol_leveler(pContext, (dolby_vol_leveler_t *)&pDapData->dapVolLeveler);
            dap_set_dialog_enhance(pContext, (dolby_dialog_enhance_t *)&pDapData->dapDialogEnhance);
            dap_set_eq_params(pContext, (dolby_eq_t *)&pDapData->dapEQ);
            dap_set_virtual_surround(pContext, (dolby_virtual_surround_t *)&pDapData->dapVirtualSrnd);
            dap_set_enable(pContext, pDapData->bDapEnabled);
        }

        return 0;
    }

    int DAP_configure(DAPContext *pContext, effect_config_t *pConfig)
    {
        DAPdata *pDapData = &pContext->gDAPdata;

        ALOGI("%s In: sampleRate = %d, channel = %d, format = 0x%x, accessmode = 0x%x",
              __FUNCTION__, pConfig->inputCfg.samplingRate, pConfig->inputCfg.channels, pConfig->inputCfg.format, pConfig->inputCfg.accessMode);
        ALOGI("%s Out: sampleRate = %d, channel = %d, format = 0x%x, accessmode = 0x%x",
              __FUNCTION__, pConfig->outputCfg.samplingRate, pConfig->outputCfg.channels, pConfig->outputCfg.format, pConfig->outputCfg.accessMode);

        if (pConfig->inputCfg.samplingRate != pConfig->outputCfg.samplingRate) {
            ALOGE("%s:inputCfg.samplingRate(%d) != outputCfg.samplingRate(%d)", __FUNCTION__, pConfig->inputCfg.samplingRate, pConfig->outputCfg.samplingRate);
            return -EINVAL;
        }
        if (pConfig->inputCfg.channels != pConfig->outputCfg.channels) {
            ALOGE("%s:inputCfg.channels(%d) != outputCfg.channels(%d)", __FUNCTION__, pConfig->inputCfg.channels, pConfig->outputCfg.channels);
            return -EINVAL;
        }
        if (pConfig->inputCfg.format != pConfig->outputCfg.format) {
            ALOGE("%s:inputCfg.format(%d) != outputCfg.format(%d)", __FUNCTION__, pConfig->inputCfg.format, pConfig->outputCfg.format);
            return -EINVAL;
        }

        if (pConfig->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) {
            ALOGW("%s: channels in = 0x%x channels out = 0x%x", __FUNCTION__, pConfig->inputCfg.channels, pConfig->outputCfg.channels);
            pConfig->inputCfg.channels = pConfig->outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
        }
        if (pConfig->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_WRITE &&
            pConfig->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_ACCUMULATE) {
            ALOGE("%s:outputCfg.accessMode != EFFECT_BUFFER_ACCESS_WRITE", __FUNCTION__);
            return -EINVAL;
        }

        if (pConfig->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT) {
            ALOGW("%s: format in = 0x%x format out = 0x%x", __FUNCTION__, pConfig->inputCfg.format, pConfig->outputCfg.format);
            pConfig->inputCfg.format = pConfig->outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        }

        memcpy(&pContext->config, pConfig, sizeof(effect_config_t));

        if (pContext->bUseFade) {
            int channels = 2;
            if (pConfig->inputCfg.channels == AUDIO_CHANNEL_OUT_STEREO) {
                channels = 2;
            }
            AudioFadeSetFormat(&pContext->gAudFade, pConfig->inputCfg.samplingRate, channels, pConfig->inputCfg.format);
        }

        //return DAP_reset(pContext);
        return 0;
    }

    int DAP_getParameter(DAPContext *pContext, void *pParam, size_t *pValueSize, void *pValue)
    {
        uint32_t param = *(uint32_t *)pParam;
        int32_t value, i;
        DAPcfg_8bit_s custom_value;
        //float scale;
        DAPdata *pDapData = &pContext->gDAPdata;

        switch (param) {
        case DAP_PARAM_ENABLE:
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = (uint32_t)pDapData->bDapEnabled;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get DAP enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        case DAP_PARAM_EFFECT_MODE:
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            //pDapData->eDapEffectMode may be changed later because of fade
            value = (uint32_t)pContext->modeValue;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get DAP effect module -> %s", __FUNCTION__, DapEffectModeStr[value]);
            break;
        case DAP_PARAM_POST_GAIN:
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = (uint32_t)pDapData->dapPostGain;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get DAP post gain -> %d", __FUNCTION__, value);
            break;
        case DAP_PARAM_VOL_LEVELER_ENABLE:
        {
            dolby_vol_leveler_t dapVolLeveler;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapVolLeveler, false);
            value = (uint32_t)dapVolLeveler.vl_enable;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Volume Leveler enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        }
        case DAP_PARAM_VOL_LEVELER_AMOUNT:
        {
            dolby_vol_leveler_t dapVolLeveler;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapVolLeveler, false);
            value = (uint32_t)dapVolLeveler.vl_amount;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Volume Leveler amount -> %d", __FUNCTION__, value);
            break;
        }
        case DAP_PARAM_DE_ENABLE:
        {
            dolby_dialog_enhance_t dapDialogEnhance;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapDialogEnhance, false);
            value = (uint32_t)dapDialogEnhance.de_enable;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Dialog Enhance Enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        }
        case DAP_PARAM_DE_AMOUNT:
        {
            dolby_dialog_enhance_t dapDialogEnhance;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapDialogEnhance, false);
            value = (uint32_t)dapDialogEnhance.de_amount;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Dialog Enhance amount -> %d", __FUNCTION__, value);
            break;
        }
        case DAP_PARAM_GEQ_ENABLE:
        {
            dolby_eq_t dapEQ;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapEQ, false);
            value = (uint32_t)dapEQ.eq_enable.geq_enable;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get EQ enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        }
        case DAP_PARAM_GEQ_GAINS:
        {
            dolby_eq_t dapEQ;
            if (*pValueSize < sizeof(DAPcfg_8bit_s)) {
                *pValueSize = 0;
                return -EINVAL;
            }

            dap_get_mode_param(pContext, param, (void *)&dapEQ, false);
            custom_value.band1 = (signed char)(dapEQ.eq_params.a_geq_band_target[0]);
            custom_value.band2 = (signed char)(dapEQ.eq_params.a_geq_band_target[1]);
            custom_value.band3 = (signed char)(dapEQ.eq_params.a_geq_band_target[2]);
            custom_value.band4 = (signed char)(dapEQ.eq_params.a_geq_band_target[3]);
            custom_value.band5 = (signed char)(dapEQ.eq_params.a_geq_band_target[4]);
            *(DAPcfg_8bit_s *) pValue = custom_value;
            ALOGD("%s: Get band: %d, %d, %d, %d, %d", __FUNCTION__, custom_value.band1,
                    custom_value.band2, custom_value.band3, custom_value.band4, custom_value.band5);
            break;
        }
        case DAP_PARAM_VIRTUALIZER_ENABLE:
        {
            dolby_virtual_surround_t dapVirtualSrnd;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapVirtualSrnd, false);
            value = (uint32_t)dapVirtualSrnd.enable.virtualizer_enable;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Virtual Surround enable -> %d", __FUNCTION__, value);
            break;
        }
        case DAP_PARAM_SUROUND_ENABLE:
        {
            dolby_virtual_surround_t dapVirtualSrnd;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapVirtualSrnd, false);
            value = (uint32_t)dapVirtualSrnd.enable.surround_decoder_enable;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Surround decoder enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        }
        case DAP_PARAM_SURROUND_BOOST:
        {
            dolby_virtual_surround_t dapVirtualSrnd;
            if (*pValueSize < sizeof(uint32_t)) {
                *pValueSize = 0;
                return -EINVAL;
            }
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            dap_get_mode_param(pContext, param, (void *)&dapVirtualSrnd, false);
            value = (uint32_t)dapVirtualSrnd.surround_boost;
            *(uint32_t *) pValue = value;
            ALOGD("%s: get Surround Boost -> %d", __FUNCTION__, value);
            break;
        }
        default:
            ALOGE("%s: unknown param %d", __FUNCTION__, param);
            return -EINVAL;
        }
        return 0;
    }

    int DAP_setParameter(DAPContext *pContext, void *pParam, void *pValue)
    {
        uint32_t param = *(uint32_t *)pParam;
        int32_t value;
        DAPcfg_8bit_s custom_value;
        int needFade = 0;
        //float scale;
        DAPdata *pDapData = &pContext->gDAPdata;
        DAPapi *pDAPapi = (DAPapi *) & (pContext->gDAPapi);

        switch (param) {
        case DAP_PARAM_ENABLE:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            dap_set_enable(pContext, value);
            ALOGD("%s: Set DAP enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        case DAP_PARAM_EFFECT_MODE:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            ALOGD("%s: Set value %d", __FUNCTION__, value);
            if (value > DAP_MODE_CUSTOM) {
                value = DAP_MODE_MUSIC;
            }

            pContext->modeValue = value;

            if (pContext->bUseFade) {
                AudioFade_t *pAudioFade = (AudioFade_t *) & (pContext->gAudFade);
                AudioFadeInit(pAudioFade, fadeLinear, 10, 0);
                if ((pDapData->eDapEffectMode == DAP_MODE_DISABLE) && (value != DAP_MODE_DISABLE)) {
                    AudioFadeSetState(pAudioFade, AUD_FADE_MUTE);
                    dap_set_enable(pContext, 1);
                } else
                    AudioFadeSetState(pAudioFade, AUD_FADE_OUT_START);
            } else {
                // origninal code
                dap_set_effect_mode(pContext, (DAPmode)value);
            }
            ALOGD("%s: Set DAP effect -> %s", __FUNCTION__, DapEffectModeStr[value]);
            break;
        case DAP_PARAM_POST_GAIN:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            pDapData->dapPostGain = value;
            dap_set_postgain(pContext, value);
            ALOGD("%s: Set DAP post gain -> %d", __FUNCTION__, value);
            break;
        case DAP_PARAM_VOL_LEVELER_ENABLE:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            value = (value == 0) ? 0 : 1;
            pDapData->dapVolLeveler.vl_enable = value;
            dap_set_vol_leveler(pContext, (dolby_vol_leveler_t *)&pDapData->dapVolLeveler);
            ALOGD("%s: Set Volume Leveler enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        case DAP_PARAM_VOL_LEVELER_AMOUNT:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            if (value > DAP_CPDP_VOLUME_LEVELER_AMOUNT_MAX) {
                value = DAP_CPDP_VOLUME_LEVELER_AMOUNT_MAX;
            }
            if (value < DAP_CPDP_VOLUME_LEVELER_AMOUNT_MIN) {
                value = DAP_CPDP_VOLUME_LEVELER_AMOUNT_MIN;
            }
            pDapData->dapVolLeveler.vl_amount = value;
            dap_set_vol_leveler(pContext, (dolby_vol_leveler_t *)&pDapData->dapVolLeveler);
            ALOGD("%s: Set Volume Leveler amount -> %d", __FUNCTION__, value);
            break;
        case DAP_PARAM_DE_ENABLE:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            value = (value == 0) ? 0 : 1;
            pDapData->dapDialogEnhance.de_enable = value;
            dap_set_dialog_enhance(pContext, (dolby_dialog_enhance_t *) & (pDapData->dapDialogEnhance));
            ALOGD("%s: Set Dialog Enhance Enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        case DAP_PARAM_DE_AMOUNT:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            pDapData->dapDialogEnhance.de_amount = value;
            if (value > DAP_CPDP_DE_AMOUNT_MAX) {
                value = DAP_CPDP_DE_AMOUNT_MAX;
            }
            if (value < DAP_CPDP_DE_AMOUNT_MIN) {
                value = DAP_CPDP_DE_AMOUNT_MIN;
            }
            dap_set_dialog_enhance(pContext, (dolby_dialog_enhance_t *) & (pDapData->dapDialogEnhance));
            ALOGD("%s: Set Dialog Enhance amount -> %d", __FUNCTION__, value);
            break;
        case DAP_PARAM_GEQ_ENABLE:
        {
            dolby_eq_t dapEQ;
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            //value = (value == 0) ? 0 : 1;
            pDapData->dapEQ.eq_enable.geq_enable = value;
            dap_get_mode_param(pContext, param, (void *)&dapEQ, false);
            dap_set_eq_params(pContext, (dolby_eq_t *) &dapEQ);
            ALOGD("%s: Set EQ enable -> %d", __FUNCTION__, value);
            break;
        }
        case DAP_PARAM_GEQ_GAINS:
            custom_value = *(DAPcfg_8bit_s *)pValue;
            int i;

            // UI gain value range is [-10 , 10]
            pDapData->dapEQ.eq_params.a_geq_band_target[0] = (signed int)custom_value.band1;
            pDapData->dapEQ.eq_params.a_geq_band_target[1] = (signed int)custom_value.band2;
            pDapData->dapEQ.eq_params.a_geq_band_target[2] = (signed int)custom_value.band3;
            pDapData->dapEQ.eq_params.a_geq_band_target[3] = (signed int)custom_value.band4;
            pDapData->dapEQ.eq_params.a_geq_band_target[4] = (signed int)custom_value.band5;
            ALOGD("%s: Set band: %d %d %d %d %d", __FUNCTION__, custom_value.band1,
                    custom_value.band2, custom_value.band3, custom_value.band4, custom_value.band5);

            // apply standard DAP audio effect when tuning EQ bands
            // customer request
            // java set wrong value when it [user]=DAP_MODE_CUSTOM, we have to modify it here
            // TODO: Ask java application to fix it
            pContext->modeValue = DAP_MODE_CUSTOM;

            if (pDapData->eDapEffectMode != DAP_MODE_CUSTOM) {
                // if we change from other mode to "CUSTOM" mode , need to do fade
                needFade = 1;
            } else {
                // if we are already in "CUSTOM" mode , no need to do fade
                needFade = 0;
            }

            if (pContext->bUseFade && needFade) {
                AudioFade_t *pAudioFade = (AudioFade_t *) & (pContext->gAudFade);
                AudioFadeInit(pAudioFade, fadeLinear, 10, 0);
                if (pDapData->eDapEffectMode == DAP_MODE_DISABLE) {
                    AudioFadeSetState(pAudioFade, AUD_FADE_MUTE);
                    dap_set_enable(pContext, 1);
                } else
                    AudioFadeSetState(pAudioFade, AUD_FADE_OUT_START);
            } else {
                // origninal code
                dap_set_effect_mode(pContext, (DAPmode)value);
            }
            break;
        case DAP_PARAM_VIRTUALIZER_ENABLE:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            //value = (value == 0) ? 0 : 1;
            pDapData->dapVirtualSrnd.enable.virtualizer_enable = value;
            dap_set_virtual_surround(pContext, (dolby_virtual_surround_t *) & (pDapData->dapVirtualSrnd));
            ALOGD("%s: Set virtualizer enable -> %d", __FUNCTION__, value);
            break;
        case DAP_PARAM_SUROUND_ENABLE:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            value = (value == 0) ? 0 : 1;
            pDapData->dapVirtualSrnd.enable.surround_decoder_enable = value;
            dap_set_virtual_surround(pContext, (dolby_virtual_surround_t *) & (pDapData->dapVirtualSrnd));
            ALOGD("%s: Set surround_decoder enable -> %s", __FUNCTION__, DapEnableStr[value]);
            break;
        case DAP_PARAM_SURROUND_BOOST:
            if (!pContext->gDAPLibHandler) {
                return 0;
            }
            value = *(uint32_t *)pValue;
            pDapData->dapVirtualSrnd.surround_boost = value;
            dap_set_virtual_surround(pContext, (dolby_virtual_surround_t *) & (pDapData->dapVirtualSrnd));
            ALOGD("%s: Set Surround Boost value -> %d", __FUNCTION__, value);
            break;


        default:
            ALOGE("%s: unknown param %08x", __FUNCTION__, param);
            return -EINVAL;
        }

        return 0;
    }

    int DAP_release(DAPContext *pContext)
    {
        dap_release_api(pContext);
        return 0;
    }

    //-------------------Effect Control Interface Implementation--------------------------
    /* In current design, in audio_hw.c,  audio_hal_data_processing()->audio_post_process()
     *  audio_post_process() call DAP_process() with "inBuffer" and "outBuffer" share same memory address
     *  to couple with this use case, DAP_process() copy "inBuffer" into "input_storage_buf"
     *  to assure input data un touched
     */
    int DAP_process(effect_handle_t self, audio_buffer_t *inBuffer, audio_buffer_t *outBuffer)
    {
        DAPContext *pContext = (DAPContext *)self;
        int inSampleSize, outSampleSize;
        int inChannels, outChannels;
        int ret;
        unsigned int tmpSize;
        unsigned int i;
        int status = EXIT_SUCCESS;
        DAPdata *pDapData = &(pContext->gDAPdata);
        DAPapi * pDAPapi = &(pContext->gDAPapi);

        //ALOGI("%s, entering...\n", __FUNCTION__);

        if (pContext == NULL) {
            ALOGE("%s, pContext == NULL", __FUNCTION__);
            return -EINVAL;
        }

        if (inBuffer == NULL || inBuffer->raw == NULL ||
            outBuffer == NULL || outBuffer->raw == NULL ||
            inBuffer->frameCount != outBuffer->frameCount ||
            inBuffer->frameCount == 0) {

            // per customer's request to add log info
            if (inBuffer == NULL) {
                ALOGE("%s, inBuffer == NULL", __FUNCTION__);
            }
            if (inBuffer->raw == NULL) {
                ALOGE("%s, inBuffer->raw == NULL", __FUNCTION__);
            }
            if (outBuffer == NULL) {
                ALOGE("%s, outBuffer == NULL", __FUNCTION__);
            }
            if (outBuffer->raw == NULL) {
                ALOGE("%s, outBuffer->raw == NULL", __FUNCTION__);
            }
            if (inBuffer->frameCount != outBuffer->frameCount) {
                ALOGE("%s, inBuffer->frameCount = %d,outBuffer->frameCount = %d", __FUNCTION__, inBuffer->frameCount, outBuffer->frameCount);
            }
            if (inBuffer->frameCount == 0) {
                ALOGE("%s, inBuffer->frameCount == 0", __FUNCTION__);
            }

            ALOGE("%s, invalid buffer config", __FUNCTION__);
            return -EINVAL;
        }

        if (pContext->state != DAP_STATE_ACTIVE) {
            ALOGE("%s, state != DAP_STATE_ACTIVE", __FUNCTION__);
            return -ENODATA;
        }

        // read input / output data format from configurations
        inSampleSize = audioFormat2sampleSize((audio_format_t)pContext->config.inputCfg.format);
        //if (-1 == inSampleSize) {
        if (2 != inSampleSize) {
            ALOGE("%s, invalid input sample size to process %d\n", __FUNCTION__, inSampleSize);
            return -EINVAL;
        }
        outSampleSize = audioFormat2sampleSize((audio_format_t)pContext->config.outputCfg.format);
        //if (-1 == outSampleSize) {
        if (2 != outSampleSize) {
            ALOGE("%s, invalid output sample size to process %d\n", __FUNCTION__, outSampleSize);
            return -EINVAL;
        }

        inChannels = popcount(pContext->config.inputCfg.channels);
        outChannels = popcount(pContext->config.outputCfg.channels);

        // TODO: out channel num not match with DAP returned size???s
        ALOGV("%s, inSampleSize = %d, outSampleSize = %d, inChannels = %d, outChannels = %d, inFrameCnt = %d\n",
              __FUNCTION__, inSampleSize, outSampleSize, inChannels, outChannels, inBuffer->frameCount);

        if (!pDapData->bDapCPDPInited || pDapData->bNeedReset) {
            if (NULL == pDapData->inStorgeBuf) {
                tmpSize = inBuffer->frameCount * inChannels * inSampleSize;
                pDapData->inStorgeBuf = malloc(tmpSize);
                if (pDapData->inStorgeBuf == NULL) {
                    ALOGE("%s,no memory for headphone output buffer", __FUNCTION__);
                    //ret = -ENOMEM;
                    return -EINVAL;
                }
                pDapData->inStorgeBufSize = tmpSize;
                pDapData->curInfrmCnts = inBuffer->frameCount;
                ALOGE("%s,malloc inStorgeBufSize [%p], size = %d", __FUNCTION__, pDapData->inStorgeBuf, pDapData->inStorgeBufSize);
            }

            dap_release_api(pContext);
            ret = dap_init_api(pContext);
            if (ret == DAP_RET_FAIL) {
                ALOGE("%s, dap_init_api() fail!!", __FUNCTION__);
                return DAP_RET_FAIL;
            }

            int in_channel_cnt = audio_channel_count_from_out_mask(pContext->config.inputCfg.channels);
            int out_channel_cnt = audio_channel_count_from_out_mask(pContext->config.outputCfg.channels);
            int dap_out_mode = DAP_CPDP_OUTPUT_2_SPEAKER;
            if (in_channel_cnt == 2) {
                dap_out_mode = DAP_CPDP_OUTPUT_2_SPEAKER;
            } else if (in_channel_cnt == 4) {
                dap_out_mode = DAP_CPDP_OUTPUT_6;
            } else if (in_channel_cnt == 6) {
                dap_out_mode = DAP_CPDP_OUTPUT_6;
            }

            pDapData->dapCPDPOutputMode = dap_out_mode;
            ALOGD("%s:channel_cnt = %d, dap_out_mode = %d", __FUNCTION__, in_channel_cnt, dap_out_mode);

            // Initializing all parameers
            aml_dap_cpdp_output_mode_set(pDAPapi, pDapData->dap_cpdp, pDapData->dapCPDPOutputMode);
            dap_set_effect_mode(pContext, pDapData->eDapEffectMode);
            //dap_set_vol_leveler(pContext, (dolby_vol_leveler_t *)&pDapData->dapVolLeveler);
            //dap_set_dialog_enhance(pContext, (dolby_dialog_enhance_t *)&pDapData->dapDialogEnhance);
            //dap_set_virtual_surround(pContext, (dolby_virtual_surround_t *)&pDapData->dapVirtualSrnd);
            //dap_set_eq_params(pContext, (dolby_eq_t *)&pDapData->dapEQ);
            //dap_set_enable(pContext, pDapData->bDapEnabled);

            pDapData->bNeedReset = 0;
        }
        pDapData->curInfrmCnts = inBuffer->frameCount;

        // pass through data
        if (!pDapData->bDapEnabled || !pContext->gDAPLibHandler || !pDapData->islicenseOK) {
            // TODO: different sample size support
            ALOGD("%s, do_passthrough() ,bLicense = %d, bDapEnabled= %d\n", __FUNCTION__, pDapData->islicenseOK, pDapData->bDapEnabled);
            ret = do_passthrough(inBuffer, outBuffer);
            pDapData->is_passthrough = 1;
            return ret;
        }

        // check dap_cpdp
        if (pDapData->dap_cpdp == NULL) {
            ALOGE("<%s::%d>--[dap_cpdp is illegal]", __FUNCTION__, __LINE__);
            return -EINVAL;
        }
        if (pDapData->pScratchMem == NULL) {
            ALOGE("<%s::%d>--[!pDapData->pScratchMem]", __FUNCTION__, __LINE__);
            return -EINVAL;
        }

        dlb_buffer inDlbBuf;
        dlb_buffer outDlbBuf;
        short *pBlkBuf16In = NULL;
        short *pBlkBuf16Out = NULL;
        void *channel_pointers[DAP_CPDP_MAX_NUM_CHANNELS];

        /* Audio input and output is done using dlb_buffer structures. These have
         * an array of channel pointers, and a stride. We create two dlb_buffer
         * structures which are completely identical except for perhaps the channel
         * counts and strides. This means that processing will be done inplace.
         * We also set up our processing to be done on interleaved data, this is
         * mainly to simplify our IO code. Non-interleaved processing is possible
         * by changing the channel pointers and stride. */
        inDlbBuf.ppdata = channel_pointers;
        inDlbBuf.nchannel = inChannels;//INPUT_CHANNELS;
        inDlbBuf.data_type = 4;//DLB_BUFFER_SHORT_16;
        inDlbBuf.nstride = 1;//INPUT_CHANNELS;

        /* We don't know the output channel count here yet, but we can set up
         * our channel pointers to handle the worst case now. */
        outDlbBuf.ppdata = channel_pointers;
        outDlbBuf.data_type = 4;//DLB_BUFFER_SHORT_16;

        int16_t   *pIn16  = (int16_t *)inBuffer->raw;
        int16_t   *pOut16 = (int16_t *)outBuffer->raw;
        unsigned char *pInChar = (unsigned char *)inBuffer->raw;
        unsigned char *pOutChar = (unsigned char *)outBuffer->raw;

        if (pContext->bUseFade) {
            int i;
            AudioFade_t *pAudFade = (AudioFade_t *) & (pContext->gAudFade);
            unsigned int modeValue;
            unsigned int nSamples = (unsigned int)inBuffer->frameCount;

            if (pAudFade->mFadeState != AUD_FADE_IDLE) {
                ALOGI("%s: mFadeState -> %d, mCurrentVolume = %d,nSamples = %d", __FUNCTION__, pAudFade->mFadeState, pAudFade->mCurrentVolume, nSamples);
            }

            // do audio fade out, set mode change, do audio fade in
            switch (pAudFade->mFadeState) {
            case AUD_FADE_OUT_START: {
                pAudFade->mTargetVolume = 0;
                pAudFade->mStartVolume = 1 << 16;
                pAudFade->mCurrentVolume = 1 << 16;
                pAudFade->mfadeTimeUsed = 0;
                pAudFade->mfadeFramesUsed = 0;
                pAudFade->mfadeTimeTotal = DEFAULT_FADE_OUT_MS;
                pAudFade->muteCounts = 1;
                AudioFadeBuf(pAudFade, pIn16, nSamples);
                pAudFade->mFadeState = AUD_FADE_OUT;
            }
            break;
            case AUD_FADE_OUT: {
                // do fade out process
                if (pAudFade->mCurrentVolume != 0) {
                    AudioFadeBuf(pAudFade, pIn16, nSamples);
                } else {
                    pAudFade->mFadeState = AUD_FADE_MUTE;
                    mutePCMBuf(pAudFade, pIn16, nSamples);
                }
            }
            break;
            case AUD_FADE_MUTE: {
                if (pAudFade->muteCounts <= 0) {
                    pAudFade->mFadeState = AUD_FADE_IN;
                    // slowly increase audio volume
                    pAudFade->mTargetVolume = 1 << 16;
                    pAudFade->mStartVolume = 0;
                    pAudFade->mCurrentVolume = 0;
                    pAudFade->mfadeTimeUsed = 0;
                    pAudFade->mfadeFramesUsed = 0;
                    pAudFade->mfadeTimeTotal = DEFAULT_FADE_IN_MS;
                    mutePCMBuf(pAudFade, pIn16, nSamples);
                    // do actrually setting
                    modeValue = pContext->modeValue;
                    // origninal code
                    dap_set_effect_mode(pContext, (DAPmode)modeValue);
                } else {
                    mutePCMBuf(pAudFade, pIn16, nSamples);
                    pAudFade->muteCounts--;
                }
            }
            break;
            case AUD_FADE_IN: {
                AudioFadeBuf(pAudFade, pIn16, nSamples);
                if (pAudFade->mCurrentVolume == 1 << 16) {
                    pAudFade->mFadeState = AUD_FADE_IDLE;
                }
            }
            break;
            case AUD_FADE_IDLE:
                // do nothing
                break;
            default:
                break;
            }
        }

        // begain start processing data
        tmpSize = inBuffer->frameCount * inChannels * inSampleSize;
        if (pDapData->inStorgeBufSize < tmpSize) {
            ALOGI("%s,realloc pDapData->inStorgeBuf size from %zu to %zu", __FUNCTION__, pDapData->inStorgeBufSize, tmpSize);
            pDapData->inStorgeBuf = realloc(pDapData->inStorgeBuf, tmpSize);
            if (pDapData->inStorgeBuf == NULL) {
                ALOGE("%s,no memory for inStorgeBuf buffer", __FUNCTION__);
                //ret = -ENOMEM;
                return -EINVAL;
            }
            pDapData->inStorgeBufSize = tmpSize;
        }

        memset(pDapData->inStorgeBuf, 0, pDapData->inStorgeBufSize);
        char *input_storage_buf = (char *)pDapData->inStorgeBuf;
        memcpy((void *)input_storage_buf, (const void *)pInChar, tmpSize);

        for (i = 0; i < DAP_CPDP_MAX_NUM_CHANNELS; i++) {
            // we don't have malloc these data,we use .DATA area.
            channel_pointers[i] = pDapData->audioInTmp16[i];
            inDlbBuf.ppdata[i] = channel_pointers[i];
            outDlbBuf.ppdata[i] = channel_pointers[i] ;
        }

        pDapData->totalFrmCnts += inBuffer->frameCount;
        if (!(pDapData->totalFrmCnts & 0x7fff)) {
            ALOGI("%s, FrmCnts = %lu,inSampleSize = %d, outSampleSize = %d, inChs = %d, outChs = %d, perInFrameCnt = %d licenseOK = %d, pass = %d\n",
                  __FUNCTION__, pDapData->totalFrmCnts, inSampleSize, outSampleSize, inChannels, outChannels, inBuffer->frameCount, pDapData->islicenseOK, pDapData->is_passthrough);
            //android::CallStack stack;
            //stack.update();
            //stack.log("MS12DAP_Effect_callstack");
        }

        if (inBuffer->frameCount % DAP_CPDP_PCM_SAMPLES_PER_BLOCK != 0) {
            ALOGE("%s,frameCount(%d) not muliplier of %d", __FUNCTION__, inBuffer->frameCount, DAP_CPDP_PCM_SAMPLES_PER_BLOCK);
        }

        int cnt = 0;
        for (i = 0; i < inBuffer->frameCount / DAP_CPDP_PCM_SAMPLES_PER_BLOCK; i++) {
            int offset = i * DAP_CPDP_PCM_SAMPLES_PER_BLOCK * inChannels * sizeof(short);
            pBlkBuf16In = (short *)(input_storage_buf + offset);
            unsigned ch_id = 0;
            unsigned nch = inDlbBuf.nchannel;

            /* Interleave data into DLB buffer structure */
            for (cnt = 0; cnt < DAP_CPDP_PCM_SAMPLES_PER_BLOCK; cnt++) {
                for (ch_id = 0; ch_id < nch; ch_id++) {
                    //NOTE: inDlbBuf.ppdata[i] = pDapData->audioInTmp16[i];
                    pDapData->audioInTmp16[ch_id][cnt] = pBlkBuf16In[cnt * nch + ch_id];
                    //ALOGE("[%x]", audio[ch_id][cnt]);
                }
            }

            outDlbBuf.nchannel = aml_dap_cpdp_prepare(pDAPapi, pDapData->dap_cpdp, &inDlbBuf, NULL, NULL);
            outDlbBuf.nstride = 1;

            /* The call to aml_dap_cpdp_process_api() requires scratch memory. This memory
             * does not need to persist between calls to aml_dap_cpdp_process_api() and
             * can be allocated on the stack if convenient. */
            int errDAP;
            memset(pDapData->pScratchMem, 0, pDapData->uScratchSize);
            aml_dap_cpdp_process_api(pDAPapi, pDapData->dap_cpdp, &outDlbBuf, pDapData->pScratchMem, &errDAP);

            if (errDAP) {
                offset = i * DAP_CPDP_PCM_SAMPLES_PER_BLOCK * outDlbBuf.nchannel * (sizeof(short)) ;
                pBlkBuf16Out = (short *)(pOutChar + offset);
                ch_id = 0;
                nch = outDlbBuf.nchannel;

                // output data also point to inplace memory
                for (cnt = 0; cnt < DAP_CPDP_PCM_SAMPLES_PER_BLOCK; cnt++) {
                    for (ch_id = 0; ch_id < nch; ch_id++) {
                        /* pass through mode */
                        pBlkBuf16Out[cnt * nch + ch_id] = pBlkBuf16In[cnt * nch + ch_id];
                        //pBlkBuf16Out[cnt * nch + ch_id] = 0;
                        //ALOGE("[%x]", audio[ch_id][cnt]);
                    }
                }

                pDapData->is_passthrough = 1;
                //ALOGI("%s, errDAP = [%d], passthrough case",__FUNCTION__, errDAP);
            } else {
                offset = i * DAP_CPDP_PCM_SAMPLES_PER_BLOCK * outDlbBuf.nchannel * (sizeof(short)) ;
                pBlkBuf16Out = (short *)(pOutChar + offset);
                ch_id = 0;
                nch = outDlbBuf.nchannel;

                // output data also point to inplace memory
                for (cnt = 0; cnt < DAP_CPDP_PCM_SAMPLES_PER_BLOCK; cnt++) {
                    for (ch_id = 0; ch_id < nch; ch_id++) {
                        /* Write the resulting data to the array */
                        pBlkBuf16Out[cnt * nch + ch_id] = pDapData->audioInTmp16[ch_id][cnt];
                        //ALOGE("[%x]", audio[ch_id][cnt]);
                    }
                }
            }
        }

        return status;
    }

    int DAP_command(effect_handle_t self, uint32_t cmdCode, uint32_t cmdSize,
                    void *pCmdData, uint32_t *replySize, void *pReplyData)
    {
        DAPContext * pContext = (DAPContext *)self;
        effect_param_t *p;
        int voffset;

        if (pContext == NULL || pContext->state == DAP_STATE_UNINITIALIZED) {
            return -EINVAL;
        }

        ALOGD("%s: cmd = %u", __FUNCTION__, cmdCode);
        switch (cmdCode) {
        case EFFECT_CMD_INIT:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
                ALOGE("%s: ERR:EFFECT_CMD_INIT", __FUNCTION__);
                return -EINVAL;
            }
            *(int *) pReplyData = DAP_init(pContext);
            break;
        case EFFECT_CMD_SET_CONFIG:
            if (pCmdData == NULL || cmdSize != sizeof(effect_config_t) || pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
                ALOGE("%s: ERR:EFFECT_CMD_SET_CONFIG", __FUNCTION__);
                return -EINVAL;
            }
            *(int *) pReplyData = DAP_configure(pContext, (effect_config_t *) pCmdData);
            break;
        case EFFECT_CMD_RESET:
            DAP_reset(pContext);
            break;
        case EFFECT_CMD_ENABLE:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
                return -EINVAL;
            }
            if (pContext->state != DAP_STATE_INITIALIZED) {
                return -ENOSYS;
            }
            pContext->state = DAP_STATE_ACTIVE;
            *(int *)pReplyData = 0;
            break;
        case EFFECT_CMD_DISABLE:
            if (pReplyData == NULL || replySize == NULL || *replySize != sizeof(int)) {
                return -EINVAL;
            }
            if (pContext->state != DAP_STATE_ACTIVE) {
                return -ENOSYS;
            }
            pContext->state = DAP_STATE_INITIALIZED;;
            *(int *)pReplyData = 0;
            break;
        case EFFECT_CMD_GET_PARAM:
            if (pCmdData == NULL ||
                cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t)) ||
                pReplyData == NULL || replySize == NULL ||
                (*replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) &&
                 *replySize < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(DAPcfg_8bit_s)))) {
                return -EINVAL;
            }

            p = (effect_param_t *)pCmdData;
            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);
            p = (effect_param_t *)pReplyData;

            voffset = ((p->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);

            p->status = DAP_getParameter(pContext, p->data, (size_t  *)&p->vsize, p->data + voffset);
            *replySize = sizeof(effect_param_t) + voffset + p->vsize;
            break;
        case EFFECT_CMD_SET_PARAM:
            if (pCmdData == NULL ||
                (cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) &&
                 cmdSize != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(DAPcfg_8bit_s))) ||
                pReplyData == NULL || replySize == NULL || *replySize != sizeof(int32_t)) {
                ALOGE("%s: EFFECT_CMD_SET_PARAM cmd size error!", __FUNCTION__);
                return -EINVAL;
            }
            p = (effect_param_t *)pCmdData;
            if (p->psize != sizeof(uint32_t) || (p->vsize != sizeof(uint32_t) &&
                                                 p->vsize != sizeof(DAPcfg_8bit_s))) {
                ALOGE("%s: EFFECT_CMD_SET_PARAM value size error!", __FUNCTION__);
                *(int32_t *)pReplyData = -EINVAL;
                break;
            }
            *(int *)pReplyData = DAP_setParameter(pContext, (void *)p->data, p->data + p->psize);
            break;
        case EFFECT_CMD_OFFLOAD:
            ALOGI("%s: EFFECT_CMD_OFFLOAD do nothing", __FUNCTION__);
            *(int *)pReplyData = 0;
            break;
        case EFFECT_CMD_SET_DEVICE:
        case EFFECT_CMD_SET_VOLUME:
        case EFFECT_CMD_SET_AUDIO_MODE:
            break;
        default:
            ALOGE("%s: invalid command %d", __FUNCTION__, cmdCode);
            return -EINVAL;
        }

        return 0;
    }

    int DAP_getDescriptor(effect_handle_t self, effect_descriptor_t *pDescriptor)
    {
        DAPContext * pContext = (DAPContext *) self;

        if (pContext == NULL || pDescriptor == NULL) {
            ALOGE("%s: invalid param", __FUNCTION__);
            return -EINVAL;
        }

        *pDescriptor = DAPDescriptor;

        return 0;
    }

    //-------------------- Effect Library Interface Implementation------------------------

    int DAPLib_Create(const effect_uuid_t *uuid, int32_t sessionId __unused, int32_t ioId __unused, effect_handle_t *pHandle)
    {
        if (pHandle == NULL || uuid == NULL) {
            return -EINVAL;
        }

        if (memcmp(uuid, &DAPDescriptor.uuid, sizeof(effect_uuid_t)) != 0) {
            return -EINVAL;
        }

        DAPContext *pContext = new DAPContext;
        if (!pContext) {
            ALOGE("%s: alloc DAPContext failed", __FUNCTION__);
            return -EINVAL;
        }
        memset(pContext, 0, sizeof(DAPContext));

        // MUST first set DAP default value, then load from ini file.
        pContext->modeValue = DAP_MODE_MUSIC;
        pContext->gDAPdata.bDapEnabled = 1;
        pContext->gDAPdata.bNeedReset = 0;
        pContext->gDAPdata.dapCPDPOutputMode = DAP_CPDP_OUTPUT_2_SPEAKER;
        pContext->gDAPdata.dapPostGain = DEFAULT_POSTGAIN;
        pContext->gDAPdata.eDapEffectMode = (DAPmode)pContext->modeValue;
        pContext->gDAPdata.curInfrmCnts = 0;
        pContext->gDAPdata.totalFrmCnts = 0;
        pContext->gDAPdata.is_passthrough = 0;
        pContext->gDAPdata.islicenseOK = 0;

        memcpy((void *) & (pContext->gDAPdata.dapBaseSetting), (void *)&dap_dolby_base_music, sizeof(dolby_base));

        dap_get_mode_param(pContext, DAP_PARAM_VOL_LEVELER_ENABLE, (void *)&pContext->gDAPdata.dapVolLeveler, true);
        dap_get_mode_param(pContext, DAP_PARAM_DE_ENABLE, (void *)&pContext->gDAPdata.dapDialogEnhance, true);
        dap_get_mode_param(pContext, DAP_PARAM_SUROUND_ENABLE, (void *)&pContext->gDAPdata.dapVirtualSrnd, true);
        dap_get_mode_param(pContext, DAP_PARAM_GEQ_ENABLE, (void *)&pContext->gDAPdata.dapEQ, true);

        // DAP configuration sets are too big, here only part of settings are put in ini file.
        if (DAP_load_ini_file(pContext) < 0) {
            ALOGE("%s: Load INI File faied, all use default param", __FUNCTION__);
        }


        // after loading dapBaseSetting from ini file , make it as custom settings.
        memcpy((void *)&dap_dolby_base_custom, (void *) & (pContext->gDAPdata.dapBaseSetting) , sizeof(dolby_base));

        if (DAP_load_lib(pContext) < 0) {
            ALOGE("%s: Load Library File faied", __FUNCTION__);
            delete pContext;
            return -EINVAL;
        }

        // first try to get DAP license, then all other function can work normally
        // authorized chip can get valid license
        pContext->gDAPdata.islicenseOK = aml_get_chip_ms12_license((DAPapi *)&pContext->gDAPapi);
        ALOGI("%s,islicenseOK = %d", __FUNCTION__, pContext->gDAPdata.islicenseOK);

        pContext->gDAPdata.inStorgeBufSize = DAP_INPUT_STORE_SIZE;
        pContext->gDAPdata.inStorgeBuf = malloc(pContext->gDAPdata.inStorgeBufSize);
        if (pContext->gDAPdata.inStorgeBuf == NULL) {
            ALOGE("%s,no memory for inStorgeBuf buffer", __FUNCTION__);
            delete pContext;
            return -EINVAL;
        }
        ALOGI("%s,malloc inStorgeBufSize [%p], size = %d", __FUNCTION__, pContext->gDAPdata.inStorgeBuf, pContext->gDAPdata.inStorgeBufSize);

        pContext->itfe = &DAPInterface;
        pContext->state = DAP_STATE_UNINITIALIZED;

        *pHandle = (effect_handle_t)pContext;

        pContext->bUseFade = 1;
        AudioFadeInit((AudioFade_t *) & (pContext->gAudFade), fadeLinear, 10, 0);
        AudioFadeSetState((AudioFade_t *) & (pContext->gAudFade), AUD_FADE_IDLE);

        pContext->state = DAP_STATE_INITIALIZED;

        ALOGD("%s: %p", __FUNCTION__, pContext);

        return 0;
    }

    int DAPLib_Release(effect_handle_t handle)
    {
        DAPContext * pContext = (DAPContext *)handle;

        if (pContext == NULL) {
            return -EINVAL;
        }

        if (pContext->gDAPdata.inStorgeBuf) {
            free(pContext->gDAPdata.inStorgeBuf);
            pContext->gDAPdata.inStorgeBuf = NULL;
            return -EINVAL;
        }

        DAP_release(pContext);
        DAP_unload_lib(pContext);
        pContext->state = DAP_STATE_UNINITIALIZED;

        delete pContext;

        return 0;
    }

    int DAPLib_GetDescriptor(const effect_uuid_t *uuid, effect_descriptor_t *pDescriptor)
    {
        if (pDescriptor == NULL || uuid == NULL) {
            ALOGE("%s: called with NULL pointer", __FUNCTION__);
            return -EINVAL;
        }

        if (memcmp(uuid, &DAPDescriptor.uuid, sizeof(effect_uuid_t)) == 0) {
            *pDescriptor = DAPDescriptor;
            return 0;
        }

        return  -EINVAL;
    }

    // effect_handle_t interface implementation for DAP effect
    const struct effect_interface_s DAPInterface = {
        DAP_process,
        DAP_command,
        DAP_getDescriptor,
        NULL,
    };

    audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
        .tag = AUDIO_EFFECT_LIBRARY_TAG,
        .version = EFFECT_LIBRARY_API_VERSION,
        .name = "MS12 DAP",
        .implementor = "Dobly Labs",
        .create_effect = DAPLib_Create,
        .release_effect = DAPLib_Release,
        .get_descriptor = DAPLib_GetDescriptor,
    };

}; // extern "C"
