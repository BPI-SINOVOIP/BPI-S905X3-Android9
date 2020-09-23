/******************************************************************************
 * This program is protected under international and U.S. copyright laws as
 * an unpublished work. This program is confidential and proprietary to the
 * copyright owners. Reproduction or disclosure, in whole or in part, or the
 * production of derivative works therefrom without the express permission of
 * the copyright owners is prohibited.
 *
 *                  Copyright (C) 2011 by Dolby Laboratories.
 *                            All rights reserved.
 ******************************************************************************/

#ifndef __DOLBY_ADUIO_PROCESSING_CONTROL_H__
#define __DOLBY_ADUIO_PROCESSING_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "dap_dlb_buffer.h"

/** @defgroup version Version Information
 * This is the API for discovering the version number of the dap_cpdp library.
 *  @{ */
#define DAP_CPDP_VERSION       "2.4.1"

#define DAP_MAX_CHANNELS        (8)
#define DAP_IEQ_MAX_BANDS       (20)
#define DAP_OPT_MAX_BANDS       (20)
#define DAP_REG_MAX_BANDS       (20)

#if 0
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
    int leveler_amount; // dap_cpdp_volume_leveler_amount_set
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

typedef struct dolby_eq_enable {
    int geq_enable;
} dolby_eq_enable_t;

typedef struct dolby_eq_params {
    int geq_nb_bands;      //5
    int a_geq_band_center[5];//0-20000
    int a_geq_band_target[5];//-10~10dB
} dolby_eq_params_t;

typedef struct dolby_eq_t {
    dolby_eq_enable_t eq_enable;
    dolby_eq_params_t eq_params;
} dolby_eq;
#endif

/*
int set_virtual_surround(dolby_virtual_surround_t *aml_virtual_surround);
int get_virtual_surround(dolby_virtual_surround_t *aml_virtual_surround);
int set_dialog_enhance(dolby_dialog_enhance_t *aml_dialog_enhance);
int get_dialog_enhance(dolby_dialog_enhance_t *aml_dialog_enhance);
int set_eq_params(dolby_eq *aml_eq);
int get_eq_params(dolby_eq *aml_eq);
int set_leveler_enable(int leveler_enable);
int get_leveler_enable(void);
int set_postgain(int postgain);
int get_postgain(void);
int set_effect_mode(int mode); //movie:0 music:1
int get_effect_mode(void);

int load_DAP_lib(void);
int unload_DAP_lib(void);
int dap_init(void);
int dap_release(void);
int dap_process(char *in_buf, char *out_buf, int length, int input_channels);
int test_dap(void);

extern void *dap_cpdp;
static void update_controls(void *p_dap_cpdp);
*/
/**
 * Get component dap_cpdp version.
 *
 * @return Version information string.
 */

//const char *DAP_cpdp_get_version(void);
/**@}*/


/** @ingroup init Initialize DAP
 *  @{*/
/** @defgroup init-mode DAP Initialization Mode
 *   These modes are provided to enable the bundle to be initialized without
 * certain features. This enables DAP to be instantiated using less memory
 * than would otherwise be required to have full signal chain support. These
 * are specified using the mode flag in the dap_cpdp_init_info structure.
 *  @{*/
/**
 * ### DAP_CPDP_MODE_FULL_SUPPORT
 *
 *   The bundle will be instantiated with complete API support. */
#define DAP_CPDP_MODE_FULL_SUPPORT          (0)

/**
 * ### DAP_CPDP_MODE_NO_DUAL_VIRTUALIZER
 *   If this is specified as the initialization mode, the complete API will be
 * supported with the exception of the following output modes:
 *     - DAP_CPDP_OUTPUT_2_HEADPHONE_2_SPEAKER
 *     - DAP_CPDP_OUTPUT_2_HEADPHONE_2DOT1_SPEAKER
 *     - DAP_CPDP_OUTPUT_2_LORO_2_LORO
 *     - DAP_CPDP_OUTPUT_2_LTRT_2_LTRT
 *
 * If either of the above modes are requested when dual virtualizer
 * support is disabled, the system state will not change. */
#define DAP_CPDP_MODE_NO_DUAL_VIRTUALIZER   (1)

/**
 * ### DAP_CPDP_MODE_NO_DEVICE_PROCESSING
 *   If this is specified as the initialization mode, the following features
 * will be disabled:
 *   - Audio Optimizer
 *   - Audio Regulator
 *   - Bass Enhancer
 *   - Graphic Equalizer
 *   - Visualization
 *
 * Their corresponding API calls will no longer have any effect on the
 * operation of the bundle.
 * In addition, the dual virtualizer support is also disabled in this mode and
 * the disabled output modes listed for DAP_CPDP_MODE_NO_DUAL_VIRTUALIZER also
 * apply here. */
#define DAP_CPDP_MODE_NO_DEVICE_PROCESSING  (2)

/**
 * ### DAP_CPDP_MODE_DV_DE_ONLY
 *   If this is specified as the initialization mode, all of the limitations
 * specified by the DAP_CPDP_MODE_NO_DEVICE_PROCESSING and
 * DAP_CPDP_MODE_NO_DUAL_VIRTUALIZER modes will apply. In addition,
 * virtualization will be disabled completely and the following output modes
 * (in addition to those specified by DAP_CPDP_MODE_NO_DUAL_VIRTUALIZER) will
 * be disabled:
 *   - DAP_CPDP_OUTPUT_2_HEADPHONE
 *   - DAP_CPDP_OUTPUT_2_SPEAKER
 *   - DAP_CPDP_OUTPUT_2DOT1_SPEAKER
 * Using the API to select an output mode or configure a feature which has
 * been disabled will have no effect. */
#define DAP_CPDP_MODE_DV_DE_ONLY            (3)
/**@}*/
/**@}*/
/** @ingroup init Initialize DAP
 *  @{*/
/**
 * DAP_CPDP initialization information
 *
 * This structure is used to store information required during DAP_CPDP
 * initialization. The fields of this structure must be configured
 * properly to initialize DAP_CPDP successfully and correctly. For the
 * usage of each field, please refer to field descriptions.
 *
 */
typedef struct {
    /** Sample rate of the signal being passed in (in hertz)
     * Valid values are 32000, 44100 and 48000. */
    unsigned long           sample_rate;

    /** This should be the number of bytes pointed to by license_data. Ignored
     * if license_data is NULL. */
    size_t                  license_size;

    /** This should point to memory containing license information. The pointer
     * must be valid until dap_cpdp_init() has returned, after this time, the
     * license_data pointer will not be accessed. You may use a NULL pointer
     * to indicate that you don't have a license. */
    const unsigned char    *license_data;

    /** The manufacturer id, which should be documented with the license. */
    unsigned long           manufacturer_id;

    /** Disable MI processing
     * The MI processing and its memory allocation will be disabled if
     * this is set to !0. However the MI steering will still
     * work if the MI metadata is passed into DAP instance. */
    int                     mi_process_disable;

    /** Enable Virtual Bass processing
     * The Virtual Bass processing and its memory allocation will be enabled if
     * this is set to !0. */
    int                     virtual_bass_process_enable;

    /** Initialization mode
     *
     * See the DAP_CPDP_MODE_* specifiers. If an invalid mode is given,
     * the DAP_CPDP_MODE_FULL_SUPPORT mode will be assumed. */
    int                     mode;
} dap_cpdp_init_info;
/**@}*/




/**
 * Media Intelligence Metadata
 *
 * This structure is only used internally for passing the MI information
 * from one DAP_CPDP instance to another DAP_CPDP instance.
 *
 * The reason why passing the MI info from one instance to another is:
 * There might be more than one DAP_CPDP instance in the system with different
 * configurations, while the MI information could be identical.
 * To save resource, MI info only needs to be calculated once and shared among
 * all the instances. Therefore we need to pass the MI info from the instance
 * which calculates it to the one referring it.
 *
 * The info contained in this structure can't be accessed by the user via
 * external (product) API.
 *
 * The DAP_CPDP instance which updates the MI info with its own Mi library
 * (internally) will return the MI info to its caller.
 * The DAP_CPDP instance which updates the MI info with the input from the instance
 * above (externally) will update the MI info by passing it to dap_cpdp_prepare().
 * And there is no MI processing inside this instance any more.
 */
typedef struct {
    long volume_leveler_weight;                /**< weighting for volume leveler in range [0,1], Q31 */
    long intelligent_equalizer_weight;         /**< weighting for intelligent_equalizer in range [0,1],Q31 */
    long dialog_enhancer_speech_confidence;    /**< speech confidence value for dialog_enhancer in range[0,1],Q31 */
    long surround_compressor_music_confidence; /**< music confidence value for surround_compressor in range[0,1],Q31 */
} dap_cpdp_metadata;

/**
 * Downmixer level configuration
 *
 * The dap_cpdp_mix_data structure is an optional set of parameters which can
 * be passed to dap_cpdp_prepare() to configure certain downmix parameters.
 * The cmixlev_q14 parameter specifies how much the center channel should be
 * mixed into the left and right channels. The surmixlev_q14 parameter
 * specifies how much the left surround/back channels should be mixed into the
 * left channel and how much the right surround/back channels should be mixed
 * into the right channel. Both of these values are linear gains specified in
 * a q14 format. For example, to set the surround mixing levels to 0.75,
 * surmixlev_q14 should be set to 0.75*2^14 (=12288).
 *
 * These settings only have an effect when the input and output configuration
 * of DAP meets one of the following conditions:
 *   - The output mode is DAP_CPDP_OUTPUT_1 with 6 or 8 channels of input
 *   - The output mode is DAP_CPDP_OUTPUT_2 with 6 or 8 channels of input
 *
 * If the none of the above conditions are met, these parameters have no
 * effect.
 *
 * If the parameters are not supplied and the above conditions are met, the
 * downmixer will behave as if cmixlev is 0.707 and surmixlev is 1.0.
 *
 * If the arguments exceed their specified ranges, they will be clipped into
 * their valid range. */
typedef struct {
    int cmixlev_q14;   /**< Linear q14 gain [0, 1.412 (23134)] */
    int surmixlev_q14; /**< Linear q14 gain [0, 1.0 (16384)] */
} dap_cpdp_mix_data;


/** @defgroup init Initialize DAP
 *   These are the API functions and data structures used in the initialization
 * of DAP signal chain. The fields of dap_cpdp_init_info structure must be
 * configured properly prior to calls to the API functions to initialize the
 * signal chain successfully and correctly.
 *  @{*/
/** @defgroup memory Query DAP State Memory Requirements
 *  @{*/
/**
 * Returns the amount of memory required for an instance of dap_cpdp for a
 * particular configuration. Returns 0 if the configuration is invalid.
 *
 * @returns                     Size of memory required to store dap_cpdp state.
 *
 * @param p_info                dap_cpdp initialization information.
 * */
size_t
aml_ex_dap_cpdp_query_memory
(const dap_cpdp_init_info *p_info
);
/**@}*/

/** @defgroup scratch Query DAP Scratch Memory Requirements
 *  @{*/
/**
 * Returns the amount of memory required for running dap_cpdp_process(). This
 * memory does not need to persist between calls to dap_cpdp_process(), and
 * you can freely pass different pointers to each call (for example, you
 * can allocate this memory on the stack
 *
 * @returns                     Size of scratch memory required to process a
 *                              block with dap_cpdp_process().
 *
 * @param p_info                dap_cpdp initialization information.
 */
size_t
aml_ex_dap_cpdp_query_scratch(const dap_cpdp_init_info *p_info);
/**@}*/

/** @defgroup init-fun Initialize DAP
 *  @{*/
/**
 * Create a new dap_cpdp instance. p_mem must point to the amount of memory
 * as returned by dap_cpdp_query_memory() for this particular p_info.
 * The returned pointer will not necessarily be the same as p_mem.
 *
 * @returns                     Instance of dap_cpdp state structure.
 *
 * @param p_info                dap_cpdp initialization information.
 * @param p_mem                 Memory pointer with allocateded size for dap_cpdp state.
 */
void *aml_ex_dap_cpdp_init(const dap_cpdp_init_info *p_info, void *p_mem);
/**@}*/
/**@}*/

/** @defgroup shutdown Shutdown DAP
 *  @{*/
/**
 * Clean up any resources owned by this object instance.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 */
int
aml_ex_dap_cpdp_shutdown
(void *p_dap_cpdp
);
/**@}*/

/** @defgroup dap-prepare Prepare DAP CPDP
 *  @{*/
#define DAP_CPDP_MAX_NUM_CHANNELS        (8)
#define DAP_CPDP_PCM_SAMPLES_PER_BLOCK (256)
/**
 *  dap_cpdp_prepare() is used to pass input to dap_cpdp. This function returns
 *  the number of output channels to expect from dap_cpdp_process().
 *  dap_cpdp_prepare() should be called once before every call to dap_cpdp_process()
 *  Processing is always done on DAP_CPDP_PCM_SAMPLES_PER_BLOCK samples.
 *
 * ### THREADING CONSIDERATIONS
 *
 *   The dap_cpdp_prepare() and dap_cpdp_process() functions must be called from the
 *   same thread. You may continue to set parameters at any time from a
 *   different thread while calling these functions, though it is unspecified
 *   when the results from the parameter sets will actually take effect.
 *   dap_cpdp_prepare() will never block.
 *
 * ### MEMORY
 *
 *   The memory pointed to by p_input is owned by dap_cpdp until the
 *   dap_cpdp_process() function has returned. Any modifications to this memory in
 *   the time between the call to dap_cpdp_prepare() and the return from
 *   dap_cpdp_process() lead to undefined behaviour.
 *
 * ### INPLACE PROCESSING
 *
 *   The buffer passed to dap_cpdp_process() may be exactly the same buffer passed
 *   to dap_cpdp_prepare() as long as the channel counts are identical. If the
 *   channel counts are different, then you will need two distinct dlb_buffer
 *   structures with different channel counts, but they may point to
 *   overlapping memory regions.
 *
 * ### CHANNEL CONFIGURATIONS
 *
 *   Input channels may be any of the following:
 *   1 channel: Mono
 *   2 channels: Left, Right
 *   6 channels: Left, Right, Centre, LFE, Left Surround, Right Surround
 *   8 channels: Left, Right, Centre, LFE, Left Surround,
 *               Right Surround, Left Rear Surround, Right Rear Surround
 *
 * ### MI INFORMATION UPDATE
 *
 *   There might be more than one DAP_CPDP instance in the system with different
 *   configuration, while the MI information could be identical.
 *   To save resource, MI info only needs to be calculated once and shared among
 *   all the instances. Therefore we need to pass the MI info from the instance
 *   which calculates it to the one referring it.
 *   The DAP_CPDP instance which updates the MI info with its own Mi library will
 *   return the MI info to its caller. In this case the pointer "p_metadata_in"
 *   passed here is NULL.
 *   The DAP_CPDP instance which updates the MI info with the input from the instance
 *   above will update the MI info by passing it into dap_cpdp_prepare() in pointer
 *   "p_metadata_in". And there is no MI processing inside this instance any more.
 *
 * ### DOWNMIXER EXTERNAL CONFIGURATION
 *
 *   The p_downmix_data argument points to a dap_cpdp_mix_data structure which
 *   specifies how the downmixer should mix certain channels together when a
 *   downmix is applied. This parameter can be NULL and the behavior for this
 *   case is defined in the above "Downmixer level configuration" section in
 *   addition to the details of the individual structure members.
 *
 * ### ERRORS
 *
 *   If the input channel count is not supported then dap_cpdp_prepare() will
 *   return 0.
 *
 * @returns                     Number of output channels expected from dap_cpdp_process()
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param p_input               Input data buffer.
 * @param p_metadata_in         MI metadata information.
 * @param p_downmix_data        Downmix configuration structure.
 */
int
aml_ex_dap_cpdp_prepare
(void          *p_dap_cpdp
 , const dlb_buffer        *p_input
 , const dap_cpdp_metadata *p_metadata_in
 , const dap_cpdp_mix_data *p_downmix_data
);
/**@}*/

/** @defgroup dap-process Process DAP CPDP
 *  @{*/
/**
 * This will process the data passed in by dap_cpdp_prepare().
 * p_output must contain the number of channels returned by dap_cpdp_prepare().
 * scratch must point to the amount of memory requested by
 * dap_cpdp_query_scratch().
 *
 * The output channel order is the same order described for dap_cpdp_prepare().
 *
 * This function returns the updated MI metadata to share with other DAP_CPDP instances
 *
 * @returns                     Updated MI metadata.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param p_output              Output data buffer.
 * @param scratch               Scratch memory pointer with allocated size for dap_cpdp scratch.
 */
dap_cpdp_metadata
aml_ex_dap_cpdp_process
(void   *p_dap_cpdp
 , const dlb_buffer       *p_output
 , void                   *scratch
);
/**@}*/

/** @defgroup latency Get DAP Latency
 *  @{*/
/**
 * Returns the latency in samples for DAP library.
 *
 * @returns                     DAP_CPDP signal chain latency in samples.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * */
int
aml_ex_dap_cpdp_get_latency
(void *p_dap_cpdp
);
/**@}*/

/** @defgroup output-mode Set Output Mode
 *  @{*/
/**
 * The function sets the user desired output mode.
 *
 * ### INPUT RANGE
 * The input must be one of the valid output modes given in the list below.
 *  - DAP_CPDP_OUTPUT_1 - Output channel count is 1(mono)
 *  - DAP_CPDP_OUTPUT_2 - Output channel count is 2 with order L, R
 *  - DAP_CPDP_OUTPUT_2_LTRT - Output channel count is 2 with order L, R,
 *      If a ch8to2 or ch6to2 downmix is required, a PLII LtRt compatible downmix will be carried out.
 *      The processing assumes that the surround channels already have the 90 degree phase shift applied and will not apply this phase shift.
 *  - DAP_CPDP_OUTPUT_6 - Output channel count is 6 with order L, R, C, LFE, Ls, Rs
 *  - DAP_CPDP_OUTPUT_6_PLIIZ_COMPATIBLE - Output channel count is 6 with order L, R, C, LFE, Ls, Rs
 *      If a ch8to6 downmix is required, a PLIIz decode compatible downmix of Lrs and Rrs into Ls and Rs will be carried out
 *  - DAP_CPDP_OUTPUT_8 - Output channel count is 8 with order L, R, C, LFE, Ls, Rs, Lrs, Rrs
 *  - DAP_CPDP_OUTPUT_2_HEADPHONE - Output channel count is 2 with order L, R,
 *      Headphone Virtualizer enabled as virtualizer
 *  - DAP_CPDP_OUTPUT_2_SPEAKER - Output channel count is 2 with order L, R,
 *      Speaker Virtualizer enabled as virtualizer
 *  - DAP_CPDP_OUTPUT_2DOT1_SPEAKER - Output channel count is 3 with order L, R, LFE
 *      Speaker Virtualizer enabled as virtualizer
 *  - DAP_CPDP_OUTPUT_2_HEADPHONE_2_SPEAKER - Output channel count is 4 with order L headphone output, R headphone output,
 *      L speaker output, R speaker output
 *      Dual virtualizer mode is enabled: 2.0 headphone virtualizer + 2.0 speaker virtualizer
 *  - DAP_CPDP_OUTPUT_2_HEADPHONE_2DOT1_SPEAKER - Output channel count is 5 with order L headphone output, R headphone output,
 *      L speaker output, R speaker output, LFE speaker output
 *      Dual virtualizer mode is enabled: 2.0 headphone virtualizer + 2.1 speaker virtualizer
 *  - DAP_CPDP_OUTPUT_2_LORO_2_LORO - Output channel count is 4 with order L headphone output, R headphone output,
 *      L speaker output, R speaker output.
 *      Dual stereo mode is enabled.
 *      Processing is similar to DAP_CPDP_OUTPUT_2_HEADPHONE_2_SPEAKER, but with both speaker and headphone virtualizers disabled.
 *      If input channel count is 6 or 8, a simple LoRo downmix will be carried out.
 *  - DAP_CPDP_OUTPUT_2_LTRT_2_LTRT - Output channel count is 4 with order L headphone output, R headphone output,
 *      L speaker output, R speaker output.
 *      Dual stereo mode is enabled.
 *      Processing is similar to DAP_CPDP_OUTPUT_2_HEADPHONE_2_SPEAKER, but with both speaker and headphone virtualizers disabled.
 *      If input channel count is 6 or 8, a PLII LtRt compatible downmix will be carried out.
 *      The processing assumes that the surround channels already have the 90 degree phase shift applied and will not apply this phase shift.
 *
 * ### DEFAULT VALUE
 *  - DAP_CPDP_OUTPUT_2
 *
 * ### NOTE
 *  - If an invalid value is given, then it will be ignored and no change will
 *    be made to the output mode.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Output mode selection.
 */
int
aml_ex_dap_cpdp_output_mode_set
(void *p_dap_cpdp
 , int value
);

#define DAP_CPDP_OUTPUT_1                         (0)
#define DAP_CPDP_OUTPUT_2                         (1)
#define DAP_CPDP_OUTPUT_2_LTRT                    (2)
#define DAP_CPDP_OUTPUT_6                         (3)
#define DAP_CPDP_OUTPUT_6_PLIIZ_COMPATIBLE        (4)
#define DAP_CPDP_OUTPUT_8                         (5)
#define DAP_CPDP_OUTPUT_2_HEADPHONE               (6)
#define DAP_CPDP_OUTPUT_2_SPEAKER                 (7)
#define DAP_CPDP_OUTPUT_2DOT1_SPEAKER             (8)
#define DAP_CPDP_OUTPUT_2_HEADPHONE_2_SPEAKER     (9)
#define DAP_CPDP_OUTPUT_2_HEADPHONE_2DOT1_SPEAKER (10)
#define DAP_CPDP_OUTPUT_2_LORO_2_LORO             (11)
#define DAP_CPDP_OUTPUT_2_LTRT_2_LTRT             (12)
/**@}*/

/** @defgroup pregain Set Pregain
 *  @{*/
/**
 * The function sets the gain applied to the signal prior to
 * entering the signal chain.
 *
 * If the audio entering the signal chain is known to
 * have been boosted or attenuated, this parameter should be
 * set to reflect how much gain has been applied.
 *
 * ### INPUT RANGE
 *  - [-130.0 dB, 30.0 dB]
 *  - This is represented as a fixed point number with DAP_CPDP_PREGAIN_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_PREGAIN_MIN, DAP_CPDP_PREGAIN_MAX]
 *
 * ### DEFAULT VALUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input
 *    is out of the valid range.
 *  - For example, -140.0 dB input will be clipped to -130.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Pregain level.
 */
int
aml_ex_dap_cpdp_pregain_set
(void *p_dap_cpdp
 , int value
);

#define DAP_CPDP_PREGAIN_MIN (-2080)
#define DAP_CPDP_PREGAIN_MAX (480)
#define DAP_CPDP_PREGAIN_DEFAULT (0)
#define DAP_CPDP_PREGAIN_FRAC_BITS (4u)
/**@}*/

/** @defgroup postgain Set Postgain
 *  @{*/
/**
 * This function sets the gain applied to the signal after
 * leaving the signal chain.
 *
 * For example: by an analog volume control.
 *
 * ### INPUT RANGE
 *  - [-130.0, 30.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_POSTGAIN_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_POSTGAIN_MIN, DAP_CPDP_POSTGAIN_MAX]
 *
 * ### DEFAULT VALUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input
 *    is out of the valid range.
 *  - For example, -140.0 dB input will be clipped to -130.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Postgain level.
 */
int
aml_ex_dap_cpdp_postgain_set
(void *p_dap_cpdp
 , int value
);

#define DAP_CPDP_POSTGAIN_MIN (-2080)
#define DAP_CPDP_POSTGAIN_MAX (480)
#define DAP_CPDP_POSTGAIN_DEFAULT (0)
#define DAP_CPDP_POSTGAIN_FRAC_BITS (4u)
/**@}*/

/** @defgroup system-gain Set System Gain
 *  @{*/
/**
 * This function sets the gain the user would like the
 * signal chain to apply to the signal.
 *
 * ### INPUT RANGE
 *  - [-130.0 dB, 30.0 dB]
 *  - This is represented as a fixed point number with DAP_CPDP_SYSTEM_GAIN_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_SYSTEM_GAIN_MIN, DAP_CPDP_SYSTEM_GAIN_MAX]
 *
 * ### DEFAULT VALUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input
 *    is out of the valid range.
 *  - For example, -140.0 dB input will be clipped to -130.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 System gain level.
 */
int
aml_ex_dap_cpdp_system_gain_set
(void *p_dap_cpdp
 , int value
);

#define DAP_CPDP_SYSTEM_GAIN_MIN (-2080)
#define DAP_CPDP_SYSTEM_GAIN_MAX (480)
#define DAP_CPDP_SYSTEM_GAIN_DEFAULT (0)
#define DAP_CPDP_SYSTEM_GAIN_FRAC_BITS (4u)
/**@}*/

/** @defgroup surround-decoder Set Surround Decoder Enable
 *  @{*/
/**
 * This function enables or disables Surround Decoder.
 *
 * ### INPUT RANGE
 *  - 0 - The Surround Decoder is disabled.
 *  - !0 - The Surround Decoder is enabled.
 *
 * ### DEFAULT VALUE
 *  - 1
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Surround decoder enabled select.
 */
int
aml_ex_dap_cpdp_surround_decoder_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup headphone-reverb Set Headphone Virtualizer Reverb Gain
 *  @{*/
/**
 * This function sets Headphone Virtualizer reverb gain
 *
 * ### INPUT RANGE
 *  - [-130.0 dB, 12.0 dB]
 *  - This is represented as a fixed point number with
 *    DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_FRAC_BITS fractional bits.
 *    The raw values are in
 *    [DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_MIN, DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_MAX]
 *
 * ### DEFAULT VALUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input
 *    is out of the valid range.
 *  - For example, 15.0 dB input will be clipped to 12.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Headphone virtualizer reverb gain level.
 */
int
aml_ex_dap_cpdp_virtualizer_headphone_reverb_gain_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_DEFAULT      (0)
#define DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_MIN          (-2080)
#define DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_MAX          (192)
#define DAP_CPDP_HEADPHONE_VIRTUALIZER_REVERB_FRAC_BITS    (4)
/**@}*/


/** @defgroup speaker-angle Set Speaker Virtualizer Angle
 *  @{*/
/**
 * This function informs Speaker Virtualizer of the horizontal angle
 * of each loudspeaker from the central listening position.
 *
 * ### INPUT RANGE
 *  - [5,30] degrees in 5 degree steps
 *
 * ### DEFAULT VALUE
 *  - 10 degrees
 *
 * ### NOTE
 *  - All values will be rounded to the nearest multiple of 5 within the input range.
 *  - For example, setting a value of 13 degrees will be rounded to 15 degrees
 *    and setting a value of 35 degrees will round down to 30 degrees.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Speaker virtualizer angle.
 */
int
aml_ex_dap_cpdp_virtualizer_speaker_angle_set
(void      *p_dap_cpdp
 , unsigned int         value
);

#define DAP_CPDP_SPEAKER_VIRTUALIZER_ANGLE_DEFAULT (10u)
#define DAP_CPDP_SPEAKER_VIRTUALIZER_ANGLE_MIN     (5u)
#define DAP_CPDP_SPEAKER_VIRTUALIZER_ANGLE_MAX     (30u)
/**@}*/

/** @defgroup speaker-start Set Speaker Virtualizer Start Frequency
 *  @{*/
/**
 * This function sets the start frequency to be virtualized in Speaker Virtualizer.
 * Frequencies below this value will not get virtualized.
 * This is useful for improving bass response.
 *
 * ### INPUT RANGE
 *  - [20, 20000] Hz
 *
 * ### DEFAULT VALUE
 *  - 20 Hz
 *
 * ### NOTE
 * - It will clip the input to valid range if the input
 *   is out of the valid range.
 * - For example, 24000 Hz input will be clipped to 20000 Hz.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Speaker virtualizer start frequency.
 */
int
aml_ex_dap_cpdp_virtualizer_speaker_start_freq_set
(void   *p_dap_cpdp
 , unsigned int      value
);
#define DAP_CPDP_SPEAKER_VIRTUALIZER_START_FREQ_DEFAULT    (20u)
#define DAP_CPDP_SPEAKER_VIRTUALIZER_START_FREQ_MIN        (20u)
#define DAP_CPDP_SPEAKER_VIRTUALIZER_START_FREQ_MAX        (20000u)
/**@}*/

/** @defgroup surround-boost Set Surround Boost
 *  @{*/
/**
 * The function specifies the amount of boost applied to the surround channels.
 *
 * ### INPUT RANGE
 *  - [0.00,6.00] dB
 *  - This is represented as a fixed point number with DAP_CPDP_SURROUND_BOOST_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_SURROUND_BOOST_MIN, DAP_CPDP_SURROUND_BOOST_MAX]
 *
 * ### DEFAULT VALUE
 *  - 6.00 dB
 *
 * ### NOTE
 * - Invalid value of input will be clipped to the range.
 * - If boost is set to 0 dB, it is equivalent to off.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Surround boost level.
 */
int
aml_ex_dap_cpdp_surround_boost_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_SURROUND_BOOST_MIN (0)
#define DAP_CPDP_SURROUND_BOOST_MAX (96)
#define DAP_CPDP_SURROUND_BOOST_DEFAULT (96)
#define DAP_CPDP_SURROUND_BOOST_FRAC_BITS (4u)
/**@}*/

/** @defgroup mi-ieq Set Media Intelligence to Intelligent Equalizer Steering Enable
 *  @{*/
/**
 * This function enables or disables Media Intelligence to Intelligent Equalizer
 * steering. If this parameter is enabled, Intelligent Equalizer will use
 * information from MI to improve the quality of the processing it does.
 * If Intelligent Equalizer is disabled, then this parameter will not have
 * any effect.
 *
 * If MI processing is disabled and there is no MI metadata passed in, then
 * this parameter will not have any effect.
 *
 * ### INPUT RANGE
 *  - 0 - MI to IEQ steering is disabled.
 *  - !0 - MI to IEQ steering is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 MI steering to Inteligent Equalizer enable select.
 */
int
aml_ex_dap_cpdp_mi2ieq_steering_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup mi-dv Set Media Intelligence to Volume Leveler Steering Enable
 *  @{*/
/**
 * This function enables or disables Media Intelligence to Volume Leveler
 * steering. If this parameter is enabled, Volume Leveler will use information
 * from MI to improve the quality of processing it does. If Volume Leveler is
 * disabled, then this parameter will not have any effect.
 *
 * If MI processing is disabled and there is no MI metadata passed in, then
 * this parameter will not have any effect.
 *
 * ### INPUT RANGE
 *  - 0 - MI to Volume Leveler steering is disabled.
 *  - !0 - MI to Volume Leveler steering is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 MI steering to Volume Leveler enable select.
 */
int
aml_ex_dap_cpdp_mi2dv_leveler_steering_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup mi-de Set Media Intelligence to Dialog Enhancer Steering Enable
 *  @{*/
/**
 * This function enables or disables Media Intelligence to Dialog Enhancer
 * steering. If this parameter is enabled, Dialog Enhancer will use information
 * from MI to improve the quality of processing it does. If Dialog Enhancer is
 * disabled, then this parameter will not have any effect.
 *
 * If MI processing is disabled and there is no MI metadata passed in, then
 * this parameter will not have any effect.
 *
 * ### INPUT RANGE
 *  - 0 - MI to Dialog Enhancer steering is disabled.
 *  - !0 - MI to Dialog Enhancer steering is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 MI steering to Dialog Enhancer enable select.
 */
int
aml_ex_dap_cpdp_mi2dialog_enhancer_steering_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup mi-surround Set Media Intelligence to Surround Compressor Steering Enable
 *  @{*/
/**
 * This function enables or disables Media Intelligence to Surround Compressor
 * steering. The Surround Compressor is only used when the headphone or speaker
 * virtualizer is enabled. If this parameter is enabled, then Surround
 * Compressor will use information from MI to improve the quality of the
 * processing it does. If the output mode doesn't require virtualization, then
 * this parameter will not have any effect.
 *
 *  If MI processing is disabled and there is no MI metadata passed in, then
 *  this parameter will be ignored.
 *
 * ### INPUT RANGE
 *  - 0 - MI to Surround Compressor steering is disabled.
 *  - !0 - MI to Surround Compressor steering is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 MI steering to Surround Compressor enable select.
 */
int
aml_ex_dap_cpdp_mi2surround_compressor_steering_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup calibration-boost Set Calibration Boost
 *  @{*/
/**
 * This function sets a boost gain to be applied to the signal. It gives
 * the ability to give the different presets a different boost level,
 * that is independent of the volume control (if it were used as
 * the main volume control).
 *
 * This gain will be processed together with the volume maximizer
 * boost gain as part of the system gain.
 *
 * ### INPUT RANGE
 *  - [0.0, 12.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_CALIBRATION_BOOST_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_CALIBRATION_BOOST_MIN, DAP_CPDP_CALIBRATION_BOOST_MAX])
 *
 * ### DEFAULT VLAUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - The invalid input will be clipped into valid range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Calibration boost amount
 */
int
aml_ex_dap_cpdp_calibration_boost_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_CALIBRATION_BOOST_MIN (0)
#define DAP_CPDP_CALIBRATION_BOOST_MAX (192)
#define DAP_CPDP_CALIBRATION_BOOST_DEFAULT (0)
#define DAP_CPDP_CALIBRATION_BOOST_FRAC_BITS (4u)
/**@}*/

/** Dolby Volume parameters */

/** @defgroup leveler-amount Set Volume Leveler Amount
 *  @{*/
/**
 *  This function sets how much the Leveler adjusts the loudness to
 *  normalize different audio content.
 *
 * ### INPUT RANGE
 *  - [0, 10]
 *
 * ### DEFAULT VALUE
 *  - 7
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume leveler amount.
 */
int
aml_ex_dap_cpdp_volume_leveler_amount_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_VOLUME_LEVELER_AMOUNT_MIN (0)
#define DAP_CPDP_VOLUME_LEVELER_AMOUNT_MAX (10)
#define DAP_CPDP_VOLUME_LEVELER_AMOUNT_DEFAULT (7)
/**@}*/

/** @defgroup leveler-input Set Volume Leveler Input Target
 *  @{*/
/**
 * This function sets the target average loudness level of the Volume Leveler.
 *
 * Volume Leveler will level the audio to the average loudness level specified by
 * the Leveler Input Target.
 *
 * The level is specified according to a K loudness weighting.
 *
 * ### INPUT RANGE
 *  - [-40.00,0.00] dBFS
 *  - This is represented as a fixed point number with DAP_CPDP_VOLUME_LEVELER_IN_TARGET_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_VOLUME_LEVELER_IN_TARGET_MIN, DAP_CPDP_VOLUME_LEVELER_IN_TARGET_MAX]
 *
 * ### DEFAULT VALUE
 *  - -20.00 dBFS
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume leveler target loudness level.
 */
int
aml_ex_dap_cpdp_volume_leveler_in_target_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_VOLUME_LEVELER_IN_TARGET_MIN (-640)
#define DAP_CPDP_VOLUME_LEVELER_IN_TARGET_MAX (0)
#define DAP_CPDP_VOLUME_LEVELER_IN_TARGET_DEFAULT (-320)
#define DAP_CPDP_VOLUME_LEVELER_IN_TARGET_FRAC_BITS (4u)
/**@}*/

/** @defgroup leveler-output Set Volume Leveler Output Target
 *  @{*/
/**
 * This function sets the the Leveler output target.
 *
 * The Leveler output target is adjusted at manufacture to calibrate
 * the system to a reference playback sound pressure level.
 *
 * To compensate for the difference between mastering and playback levels,
 * Volume Leveler applies an un-shaped digital gain of Leveler output target
 * minus Leveler input target so that zero digital and analog volume
 * produces reference loudness levels.
 *
 * ### INPUT RANGE
 *  - [-40.00,0.00] dBFS
 *  - This is represented as a fixed point number with DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_MIN, DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_MAX]
 *
 * ### DEFAULT VALUE
 *  - -20.00 dBFS
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume leveler target loudness level.
 */
int
aml_ex_dap_cpdp_volume_leveler_out_target_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_MIN (-640)
#define DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_MAX (0)
#define DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_DEFAULT (-320)
#define DAP_CPDP_VOLUME_LEVELER_OUT_TARGET_FRAC_BITS (4u)
/**@}*/

/** @defgroup leveler-enable Set Volume Leveler Enable
 *  @{*/
/**
 * This function specifies the preferential enable for the
 * Volume Leveler feature.
 *
 * The Volume leveler, reduces the loud parts of the audio,
 * and increases the quiet parts, to enhance the listening experience
 * for non-ideal listening environments, while maintaining
 * the expected dynamics of the signal.
 *
 * ### INPUT RANGE
 *  - 0 - Volume Leveler is disabled.
 *  - !0 - Volume Leveler is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume leveler enable select.
 */
int
aml_ex_dap_cpdp_volume_leveler_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup modeler-calibration Set Volume Modeler Calibration
 *  @{*/
/**
 * This function sets Modeler calibration.
 *
 * It is used to fine-tune the manufacturer calibrated reference level
 * to the listening environment.
 *
 * This input parameter allows end users to adjust for variables
 * such as listening positions or differing speaker sensitivities.
 *
 * For example, the end user of an A/V receiver (AVR) with Dolby Volume
 * can recalibrate an AVR to optimize the reference level for the
 * user's actual listening position.
 *
 * ### INPUT RANGE
 *  - [-20.00,20.00] dB
 *  - This is represented as a fixed point number with DAP_CPDP_VOLUME_MODELER_CALIBRATION_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_VOLUME_MODELER_CALIBRATION_MIN, DAP_CPDP_VOLUME_MODELER_CALIBRATION_MAX])
 *
 * ### DEFAULT VALUE
 *  - 0.00 dB
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume modeler calibration level.
 */
int
aml_ex_dap_cpdp_volume_modeler_calibration_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_VOLUME_MODELER_CALIBRATION_MIN (-320)
#define DAP_CPDP_VOLUME_MODELER_CALIBRATION_MAX (320)
#define DAP_CPDP_VOLUME_MODELER_CALIBRATION_DEFAULT (0)
#define DAP_CPDP_VOLUME_MODELER_CALIBRATION_FRAC_BITS (4u)
/**@}*/

/** @defgroup modeler-enable Set Volume Modeler Enable
 *  @{*/
/**
 * This function specifies the preferential enable for the Volume Modeler feature.
 *
 * The Volume Modeler, provides calibrated spectral shaping of the audio,
 * to compensate for the differences between your current playback level, and
 * the mastering loudness levels at time of recording.
 *
 * ### INPUT RANGE
 *  - 0 - Volume Modeler is disabled.
 *  - !0 - Volume Modeler is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume modeler enable select.
 */
int
aml_ex_dap_cpdp_volume_modeler_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup ieq-bands Set Intelligent Equalizer Bands
 *  @{*/
/**
 * The function sets the Intelligent Equalizer bands including number of bands,
 * frequencies for each band supplied as integral values in Hertz, and corresponding
 * target gains for each band supplied.
 *
 * ### INPUT RANGE
 *  - Band Count: [1, 20]
 *  - Band Frequencies: [20, 20000] Hz
 *  - Band Targets: [-30.0, 30.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_IEQ_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_IEQ_MIN_BANDS_TARGETS, DAP_CPDP_IEQ_MAX_BANDS_TARGETS]
 *
 * ### DEFAULT VALUE
 *  - Band Count: 10
 *  - Band Frequencies: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000} Hz
 *  - Band Targets: {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0} dB
 *
 * ### NOTE
 *  - Invalid values of input will be clipped to the range.
 *  - The frequency values must increase through the array, if not, the invalid setting will be
 *    ignored and current band setting will be kept.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param nb_bands              Number of bands of data supplied to the intelliget equalizer.
 * @param p_band_centers        Center frequencies of each band.
 * @param p_band_targets        Target gain for each band.
 */
int
aml_ex_dap_cpdp_ieq_bands_set
(void   *p_dap_cpdp
 ,      unsigned int      nb_bands
 , const unsigned int     *p_band_centers
 , const int              *p_band_targets
);

#define DAP_CPDP_IEQ_MIN_BANDS_FREQS    (20u)
#define DAP_CPDP_IEQ_MAX_BANDS_FREQS    (20000u)
#define DAP_CPDP_IEQ_MIN_BANDS_TARGETS  (-480)
#define DAP_CPDP_IEQ_MAX_BANDS_TARGETS  (480)
#define DAP_CPDP_IEQ_DEFAULT_BANDS_NUM  (10u)
#define DAP_CPDP_IEQ_MAX_BANDS_NUM      (20u)
#define DAP_CPDP_IEQ_MIN_BANDS_SIZE     (1u)
#define DAP_CPDP_IEQ_MAX_BANDS_SIZE     (DAP_CPDP_IEQ_MAX_BANDS_NUM)
#define DAP_CPDP_IEQ_DEFAULT_BANDS_SIZE (DAP_CPDP_IEQ_DEFAULT_BANDS_NUM)
#define DAP_CPDP_IEQ_FRAC_BITS          (4u)
#define DAP_CPDP_IEQ_DEFAULT_BAND_FREQUENCIES {32u, 64u, 125u, 250u, 500u, 1000u, 2000u, 4000u, 8000u, 16000u}
#define DAP_CPDP_IEQ_DEFAULT_BAND_TARGETS     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
/**@}*/

/** @defgroup ieq-enable Set Intelligent Equalizer Enable
 *  @{*/
/**
 *  The function specifies the preferential enable for the Intelligent Equalizer feature.
 *
 *  The Intelligent Equalizer is a new generation of graphic equalizer that dynamically
 *  adjusts your audio to meet your spectral preferences.
 *
 *  Instead of telling the equalizer just to boost a bass frequency,
 *  you tell the intelligent equalizer I like this much bass relative to higher frequencies,
 *  so adjust the audio appropriately to achieve this.
 *
 * ### INPUT RANGE
 *  - 0 - Intelligent Equalizer is disabled.
 *  - !0 - Intelligent Equalizer is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Intelligent Equalizer enable select.
 */
int
aml_ex_dap_cpdp_ieq_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup ieq-amount Set Intelligent Equalizer Amount
 *  @{*/
/**
 *  The function specifies the strength of the Intelligent Equalizer effect to apply.
 *
 * ### INPUT RANGE
 *  - [0.00,1.00]
 *  - This is represented as a fixed point number with DAP_CPDP_IEQ_AMOUNT_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_IEQ_AMOUNT_MIN, DAP_CPDP_IEQ_AMOUNT_MAX])
 *
 * ### DEFAULT VALUE
 *  - 0.625
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Intelligent Equalizer amount.
 */
int
aml_ex_dap_cpdp_ieq_amount_set
(void *p_dap_cpdp
 , int             value
);

#define DAP_CPDP_IEQ_AMOUNT_MIN (0)
#define DAP_CPDP_IEQ_AMOUNT_MAX (16)
#define DAP_CPDP_IEQ_AMOUNT_DEFAULT (10)
#define DAP_CPDP_IEQ_AMOUNT_FRAC_BITS (4u)
/**@}*/

/** Dialog Enhancement parameters */

/** @defgroup de-enable Set Dialog Enhancement Enable
 *  @{*/
/**
 *  The function specifies the preferential enable for the Dialog Enhancement feature.
 *
 * ### INPUT RANGE
 *  - 0 - Dialog Enhancement is disabled.
 *  - !0 - Dialog Enhancement is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Dialog enhancer enable select.
 */
int
aml_ex_dap_cpdp_de_enable_set
(void *p_dap_cpdp
 , int             value
);
/**@}*/

/** @defgroup de-amount Set Dialog Enhancer Amount
 *  @{*/
/**
 *  The function specifies the strength of the Dialog Enhancer effect.
 *
 * ### INPUT RANGE
 *  - [0.00,1.00]
 *  - This is represented as a fixed point number with DAP_CPDP_DE_AMOUNT_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_DE_AMOUNT_MIN, DAP_CPDP_DE_AMOUNT_MAX])
 *
 * ### DEFAULT VALUE
 *  - 0.00
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Dialog enhancer amount.
 */
int
aml_ex_dap_cpdp_de_amount_set
(void *p_dap_cpdp
 , int             value
);

#define  DAP_CPDP_DE_AMOUNT_MIN  (0)
#define  DAP_CPDP_DE_AMOUNT_MAX  (16)
#define  DAP_CPDP_DE_AMOUNT_DEFAULT  (0)
#define  DAP_CPDP_DE_AMOUNT_FRAC_BITS (4u)
/**@}*/

/** @defgroup de-ducking Set Dialog Enhancer Ducking
 *  @{*/
/**
 *  The function specifies the degree of suppresion of channels that don't contain dialog.
 *
 * ### INPUT RANGE
 *  - [0.00,1.00]
 *  - This is represented as a fixed point number with DAP_CPDP_DE_DUCKING_FRAC_BITS
 *    fracional bits. The raw values are in [DAP_CPDP_DE_DUCKING_MIN, DAP_CPDP_DE_DUCKING_MAX])
 *
 * ### DEFAULT VALUE
 *  - 0.00
 *
 * ### NOTE
 *  - Invalid value of input will be clipped to the range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Dialog enhancer ducking level.
 */
int
aml_ex_dap_cpdp_de_ducking_set
(void *p_dap_cpdp
 , int             value
);

#define  DAP_CPDP_DE_DUCKING_MIN  (0)
#define  DAP_CPDP_DE_DUCKING_MAX  (16)
#define  DAP_CPDP_DE_DUCKING_DEFAULT  (0)
#define  DAP_CPDP_DE_DUCKING_FRAC_BITS (4u)
/**@}*/

/** @defgroup volume-boost Set Volume Maximizer Boost
 *  @{*/
/**
 * The function sets the boost gain applied to the signal
 * in the signal chain.
 *
 * Volume maximization will be performed only if Volume Leveler
 * is enabled.
 *
 * ### INPUT RANGE
 *  - [0.0, 12.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_VOLMAX_BOOST_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_VOLMAX_BOOST_MIN, DAP_CPDP_VOLMAX_BOOST_MAX]
 *
 * ### DEFAULT VALUE
 *  - 9.0 dB
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input
 *    is out of the valid range.
 *  - For example, 15.0 dB input will be clipped to 12.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Volume maximizer boost level.
 */
int
aml_ex_dap_cpdp_volmax_boost_set
(void  *p_dap_cpdp
 , int              value
);

#define DAP_CPDP_VOLMAX_BOOST_MIN (0)
#define DAP_CPDP_VOLMAX_BOOST_MAX (192)
#define DAP_CPDP_VOLMAX_BOOST_DEFAULT (144)
#define DAP_CPDP_VOLMAX_BOOST_FRAC_BITS (4u)
/**@}*/

/** @defgroup eq-enable Set Graphic Equalizer Enable
 *  @{*/
/**
 * This function specifies the enable for the Graphic Equalizer feature.
 *
 * The Graphic Equalizer feature is a traditional equalizer, with configurable
 * number of bands and centre frequencies. The same equalization
 * gains will be applied to all channels.
 *
 * ### INPUT RANGE
 *  - 0 - Graphic Equalizer is disabled.
 *  - !0 - Graphic Equalizer is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Graphic equalizer enable select.
 */
int
aml_ex_dap_cpdp_graphic_equalizer_enable_set
(void  *p_dap_cpdp
 , int              value
);
/**@}*/

/** @defgroup eq-bands Set Graphic Equalizer Bands
 *  @{*/
/**
 *  The function sets the Graphic Equalizer bands including number of bands,
 *  frequencies for each band supplied as integral values in Hertz, and corresponding
 *  target gains for each band supplied.
 *
 * ### INPUT RANGE
 *  - Band Count: [1, 20]
 *  - Band Frequencies: [20, 20000] Hz
 *  - Band Gains: [-36.0, 36.0]
 *    - This is represented as a fixed point number with DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MIN,
 *      DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MAX]
 *
 * ### DEFAULT VALUE
 *  - Band Count: 10
 *  - Band Frequencies: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000} Hz
 *  - Band Gains: {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0} dB
 *
 * ### NOTE
 *  - If
 *    - The number of bands given is outside the valid range, OR
 *    - The band frequencies are not given in a strictly increasing order OR
 *    - The p_freq or p_gains pointers are NULL
 *    Then this function will return without doing anything.
 *
 *  - Invalid values for band frequencies or gains will be clipped to their
 *    respective valid ranges.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param nb_bands              Number of bands of data supplied to the graphic equalizer.
 * @param p_freq                Center frequencies of each band.
 * @param p_gains               Gain for each band.
 */
int
aml_ex_dap_cpdp_graphic_equalizer_bands_set
(void   *p_dap_cpdp
 ,      unsigned int      nb_bands
 , const unsigned int     *p_freq
 , const int              *p_gains
);

#define DAP_CPDP_GRAPHIC_EQUALIZER_BANDS_MIN        (1u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_BANDS_MAX        (20u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_FREQ_MIN         (20u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_FREQ_MAX         (20000u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_FRAC_BITS   (4u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MIN         (-576)
#define DAP_CPDP_GRAPHIC_EQUALIZER_GAIN_MAX         (576)
#define DAP_CPDP_GRAPHIC_EQUALIZER_DEFAULT_NB_BANDS (10u)
#define DAP_CPDP_GRAPHIC_EQUALIZER_DEFAULT_FREQS    {32u, 64u, 125u, 250u \
                                                    ,500u, 1000u, 2000u \
                                                    ,4000u, 8000u, 16000u}
#define DAP_CPDP_GRAPHIC_EQUALIZER_DEFAULT_GAINS    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
/**@}*/

/** @defgroup optimizer-enable Set Audio Optimizer Enable
 *  @{*/
/**
 *  This function specifies the preferential enable for the Audio Optimizer feature.
 *
 *  The Audio Optimizer provides a static equalization curve with a configurable
 *  number of bands and centre frequencies. This curve may be different for
 *  different channels.
 *
 *  The curves are typically produced by a tuning tool to compensate for imperfections in the
 *  speakers internal to a device.
 *
 *  Audio optimizer enable behaves differently depending on whether it is in dual
 *  virtualizer mode or not.
 *
 *  When in dual virtualizer mode, audio optimizer in the speaker virtualizer
 *  audio stream will be enabled if the top level API has turned this feature on,
 *  whereas audio optimizer in the headphone virtualizer audio stream will always be off.
 *
 *  When not in dual virtualizer mode, audio optimizer will be enabled if the top level
 *  API has turned this feature on.
 *
 * ### INPUT RANGE
 *  - 0 - Audio Optimizer is disabled.
 *  - !0 - Audio Optimizer is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Audio optimizer enable select.
 */
int
aml_ex_dap_cpdp_audio_optimizer_enable_set
(void  *p_dap_cpdp
 , int              value
);
/**@}*/

/** @defgroup optimizer-bands Set Audio Optimizer Bands
 *  @{*/
/**
 *  The function sets the Audio Optimizer bands including number of bands,
 *  frequencies for each band supplied as integral values in Hertz, and
 *  corresponding gains for each band and each channel supplied.
 *
 *  *ap_gains[] is an array of DAP_CPDP_MAX_NUM_CHANNELS pointers, each pointing
 *  to the gain array of a particular channel.
 *
 *  The channel order is L, R, C, LFE, Ls, Rs, Lrs, Rrs.
 *
 *  For example, the array of gains for channel ch is pointed to by
 *  *ap_gains[ch].
 *
 *  For channel ch that does not exist or does not need processing,
 *  *ap_gains[ch] points to NULL.
 *
 *  Additionally, when the audio optimizer is in dual-virtualizer or dual-stereo mode, only the
 *  channels corresponding to the virtual speaker output will be processed;
 *  the channels corresponding to the headphone virtualizer output will not be
 *  processed.
 *
 * ### INPUT RANGE
 *  - Band Count: [1, 20]
 *  - Band Frequencies: [20, 20000] Hz
 *  - Band Gains: [-30.0, 30.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_AUDIO_OPTIMIZER_GAIN_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_AUDIO_OPTIMIZER_GAIN_MIN, DAP_CPDP_AUDIO_OPTIMIZER_GAIN_MAX]
 *
 * ### DEFAULT VALUE
 *  - Band Count: 10
 *  - Band Frequencies: {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000} Hz
 *  - Band Gains: {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL} dB
 *
 * ### NOTE
 *  - ap_gains must contain pointers for the maximum number of channels, not
 *    the current number of channels.
 *  - Band gains and frequencies will be clipped to their valid ranges if:
 *    - The frequency values are not strictly increasing. OR
 *    - p_freq or ap_gains are NULL. OR
 *    - nb_bands is out of range.
 *    Then the call is ignored, and no change is made to any parameters.
 *  - The ap_gains parameter is read only, even though the type signature
 *    suggests that our implementation may modify ap_gains[x][y]. This is
 *    done because limitations in C prevent an int** from being automatically
 *    converted to a const int **, so actually calling this function would be
 *    cumbersome if we were to mark it as fully const.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param nb_bands              Number of bands of data supplied to the audio optimizer.
 * @param p_freq                Center frequencies of each band.
 * @param ap_gains              Gain for each band, upto maximum number of bands.
 */
int
aml_ex_dap_cpdp_audio_optimizer_bands_set
(void       *p_dap_cpdp
 ,      unsigned int          nb_bands
 , const unsigned int         *p_freq
 ,               int * const  ap_gains[DAP_CPDP_MAX_NUM_CHANNELS]
);

#define DAP_CPDP_AUDIO_OPTIMIZER_BANDS_MIN        (1u)
#define DAP_CPDP_AUDIO_OPTIMIZER_BANDS_MAX        (20u)
#define DAP_CPDP_AUDIO_OPTIMIZER_FREQ_MIN         (20u)
#define DAP_CPDP_AUDIO_OPTIMIZER_FREQ_MAX         (20000u)
#define DAP_CPDP_AUDIO_OPTIMIZER_GAIN_FRAC_BITS   (4u)
#define DAP_CPDP_AUDIO_OPTIMIZER_GAIN_MIN         (-480)
#define DAP_CPDP_AUDIO_OPTIMIZER_GAIN_MAX         (480)
#define DAP_CPDP_AUDIO_OPTIMIZER_DEFAULT_NB_BANDS (10u)
#define DAP_CPDP_AUDIO_OPTIMIZER_DEFAULT_FREQS    {32u, 64u, 125u, 250u \
                                                  ,500u, 1000u, 2000u \
                                                  ,4000u, 8000u, 16000u}
/**@}*/

/** @defgroup bass-enable Set Bass Enhancer Enable
 *  @{*/
/**
 *  The function enables or disables Bass Enhancer.
 *  Bass Enhancer enhances the bass response of a speaker by adding a low frequency shelf.
 *
 *  In conjunction with the audio regulator in distortion protection mode,
 *  Bass Enhancer gives signal dependent enhancement without distortion.
 *
 * ### INPUT RANGE
 *  - 0 - Bass Enhancer is disabled.
 *  - !0 - Bass Enhancer is enabled.
 *
 * ### DEFAULT VALUE
 *  - 0
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Bass enhancer enable select.
 */
int
aml_ex_dap_cpdp_bass_enhancer_enable_set
(void  *p_dap_cpdp
 , int              value
);
/**@}*/

/** @defgroup bass-boost Set Bass Enhancer Enhancement Boost
 * @{*/
/**
 *  The function sets the amount of bass enhancement boost applied by Bass Enhancer.
 *
 * ### INPUT RANGE
 *  - [0.0, 24.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_BASS_ENHANCER_BOOST_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_BASS_ENHANCER_BOOST_MIN, DAP_CPDP_BASS_ENHANCER_BOOST_MAX])
 *
 * ### DEFAULT VALUE
 *  - 12.0 dB
 *
 * ### NOTE
 *  - Input value that is out of the valid range will be clipped
 *    to the valid range.
 *  - For example, 25.0 dB input will be clipped to 24.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param boost                 Bass enhancer boost level.
 */
int
aml_ex_dap_cpdp_bass_enhancer_boost_set
(void  *p_dap_cpdp
 , int              boost
);

#define DAP_CPDP_BASS_ENHANCER_BOOST_MIN (0)
#define DAP_CPDP_BASS_ENHANCER_BOOST_MAX (384)
#define DAP_CPDP_BASS_ENHANCER_BOOST_DEFAULT (192)
#define DAP_CPDP_BASS_ENHANCER_BOOST_FRAC_BITS (4u)
/**@}*/

/** @defgroup bass-cutoff Set Bass Enhancer Cutoff Frequency
 *  @{*/
/**
 *  The function sets the bass enhancement cutoff frequency used by Bass Enhancer.
 *
 * ### INPUT RANGE
 *  - [20, 2000] Hz
 *
 * ### DEFAULT VALUE
 *  - 200 Hz
 *
 * ### NOTE
 *  - Input value that is out of the valid range will be clipped
 *    to the valid range.
 *  - For example, input of 2400 Hz will be clipped to 2000 Hz.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param cutoff_freq           Bass enhancer cutoff frequency.
 */
int
aml_ex_dap_cpdp_bass_enhancer_cutoff_frequency_set
(void  *p_dap_cpdp
 , unsigned         cutoff_freq
);

#define DAP_CPDP_BASS_ENHANCER_CUTOFF_MIN (20u)
#define DAP_CPDP_BASS_ENHANCER_CUTOFF_MAX (2000u)
#define DAP_CPDP_BASS_ENHANCER_CUTOFF_DEFAULT (200u)
/**@}*/

/** @defgroup bass-width Set Bass Enhancer Width
 *  @{*/
/**
 *  The function sets the width of the bass enhancement boost curve
 *  used by Bass Enhancer, in units of octaves below the cutoff frequency.
 *
 * ### INPUT RANGE
 *  - [0.125, 4.0] Octaves
 *  - This is represented as a fixed point number with DAP_CPDP_BASS_ENHANCER_WIDTH_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_BASS_ENHANCER_WIDTH_MIN, DAP_CPDP_BASS_ENHANCER_WIDTH_MAX]
 *
 * ### DEFAULT VALUE
 *  - 1.0 Octave
 *
 * ### NOTE
 *  - Input value that is out of the valid range will be clipped
 *    to the valid range.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param width                 Width of the bass enhancer boost curve.
 */
int
aml_ex_dap_cpdp_bass_enhancer_width_set
(void  *p_dap_cpdp
 , int              width
);

#define DAP_CPDP_BASS_ENHANCER_WIDTH_MIN (2)
#define DAP_CPDP_BASS_ENHANCER_WIDTH_MAX (64)
#define DAP_CPDP_BASS_ENHANCER_WIDTH_DEFAULT (16)
#define DAP_CPDP_BASS_ENHANCER_WIDTH_FRAC_BITS (4u)
/**@}*/

/** @defgroup visualization Get Visualization Band Info
 *  @{*/

#define DAP_CPDP_VIS_NB_BANDS_MAX         (20u)
#define DAP_CPDP_VIS_GAIN_FRAC_BITS       (4u)
#define DAP_CPDP_VIS_EXCITATION_FRAC_BITS (4u)
#define DAP_CPDP_VIS_GAIN_MIN             (-192)
#define DAP_CPDP_VIS_GAIN_MAX             (576)
#define DAP_CPDP_VIS_EXCITATION_MIN       (-192)
#define DAP_CPDP_VIS_EXCITATION_MAX       (576)

/**
 *  The function gets the Visualization bands including number of bands,
 *  frequencies for each band supplied as integral values in Hertz,
 *  and corresponding gains and excitation info.
 *
 * ### OUTPUT RANGE
 *  - Band Count: [19, 20]
 *  - Band Frequencies: [20, 20000] Hz
 *  - Band Gains: [-12.0, 36.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_VIS_GAIN_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_VIS_GAIN_MIN, DAP_CPDP_VIS_GAIN_MAX]
 *  - Band Excitations: [-12.0, 36.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_VIS_EXCITATION_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_VIS_EXCITATION_MIN, DAP_CPDP_VIS_EXCITATION_MAX]
 *
 * ### NOTE
 *  - The number of bands and their frequencies depends on the sample rate.
 *  - If these are not suitable, then you should use dap_cpdp_vis_custom_bands_get()
 *    instead.
 *  - Any of p_nb_bands, p_band_centers, p_band_gains or p_band_excitations may
 *    be NULL.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param nb_bands              Number of bands of data supplied to the visualizer.
 * @param p_band_centers        Center frequencies of each band.
 * @param p_band_gains          Regulator gain information.
 * @param p_band_excitation     Regulator excitation information.
 */
int
aml_ex_dap_cpdp_vis_bands_get
(void   *p_dap_cpdp
 , unsigned int     *p_nb_bands
 , unsigned int      p_band_centers[DAP_CPDP_VIS_NB_BANDS_MAX]
 , int               p_band_gains[DAP_CPDP_VIS_NB_BANDS_MAX]
 , int               p_band_excitations[DAP_CPDP_VIS_NB_BANDS_MAX]
);
/**@}*/

/** @defgroup visualization-custom Get Visualization Custom Band Info
 *  @{*/
/**
 *  The function gets the Visualization gains and excitation info
 *  in custom bands which is calculated in this function with the input
 *  of nb_bands and p_band_centers.
 *
 * ### INPUT RANGE
 *  - Band Count: [1, 20]
 *  - Band Frequencies: [20, 20000] Hz
 *
 * ### OUTPUT RANGE
 *  - Band Gains: [-12.0, 36.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_VIS_GAIN_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_VIS_GAIN_MIN, DAP_CPDP_VIS_GAIN_MAX]
 *  - Band Excitations: [-12.0, 36.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_VIS_EXCITATION_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_VIS_EXCITATION_MIN, DAP_CPDP_VIS_EXCITATION_MAX]
 *
 * ### NOTE
 *  - The band centers must be in strictly increasing order. Values outside
 *    the valid range will be clipped.
 *  - If p_band_centers is NULL, nb_bands is outside the valid range, or the
 *    frequencies are not in increasing order, then this function will return
 *    without any side effects.
 *    p_band_gains or p_band_excitation may be NULL, otherwise they must point
 *    to at least nb_bands ints.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param nb_bands              Number of bands of data supplied to the visualizer.
 * @param p_band_centers        Center frequencies of each band.
 * @param p_band_gains          Regulator gain information.
 * @param p_band_excitation     Regulator excitation information.
 */
int
aml_ex_dap_cpdp_vis_custom_bands_get
(void   *p_dap_cpdp
 ,      unsigned int      nb_bands
 , const unsigned int     *p_band_centers
 ,      int              *p_band_gains
 ,      int              *p_band_excitation
);

#define DAP_CPDP_VIS_CUSTOM_NB_BANDS_MIN         (1u)
#define DAP_CPDP_VIS_CUSTOM_NB_BANDS_MAX         (20u)
#define DAP_CPDP_VIS_CUSTOM_GAIN_FRAC_BITS       (4u)
#define DAP_CPDP_VIS_CUSTOM_EXCITATION_FRAC_BITS (4u)
#define DAP_CPDP_VIS_CUSTOM_FREQ_MIN             (20u)
#define DAP_CPDP_VIS_CUSTOM_FREQ_MAX             (20000u)
#define DAP_CPDP_VIS_CUSTOM_GAIN_MIN             (-192)
#define DAP_CPDP_VIS_CUSTOM_GAIN_MAX             (576)
#define DAP_CPDP_VIS_CUSTOM_EXCITATION_MIN       (-192)
#define DAP_CPDP_VIS_CUSTOM_EXCITATION_MAX       (576)
/**@}*/

/** @defgroup regulator-threshold Set Audio Regulator Tuning Thresholds
 *  @{*/
/**
 *  This function is used to provide the Audio Regulator with tuning
 *  coefficients. These coefficients are only used when the Audio Regulator
 *  is operating in speaker distortion mode (see the
 *  dap_cpdp_regulator_speaker_distortion_enable_set() API function).
 *
 * ### INPUT RANGE
 *  - nb_bands: [1, 20]
 *  - p_band_centers: [20, 20000] Hz
 *  - p_low_thresholds: [-130.0, 0.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_REGULATOR_TUNING_THRESHOLD_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_REGULATOR_TUNING_THRESHOLD_MIN,
 *      DAP_CPDP_REGULATOR_TUNING_THRESHOLD_MAX]
 *  - p_high_thresholds: [-130.0, 0.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_REGULATOR_TUNING_THRESHOLD_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_REGULATOR_TUNING_THRESHOLD_MIN,
 *      DAP_CPDP_REGULATOR_TUNING_THRESHOLD_MAX]
 *
 * ### DEFAULT VALUES
 *  - By default, the Audio Regulator will behave as if the number of bands is
 *  1 (thus the value of center frequency at index zero will not matter), with
 *  the low threshold set to -12.0 dB and the high threshold set to 0.0 dB.
 *
 * ### NOTES
 *  1. If nb_bands is outside of the specified range, the function will have
 *     no effect.
 *  2. If p_band_centers contains values outside of the specified range or if
 *     the values are not incremental throughout the array, the function will
 *     have no effect.
 *  3. If any values of the p_low_thresholds array are greater than the
 *     corresponding element of p_high_thresholds, the function will have no
 *     effect.
 *  4. If any values of the p_low_thresholds or p_high_thresholds arrays are
 *     outside of the specified permissible range, they will be clipped to be
 *     within the specified range.
 *  5. If p_band_centers, p_low_thresholds, p_high_thresholds or
 *     p_isolated_bands is NULL, the function will have no effect.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param nb_bands              Number of bands of data supplied to the regulator.
 * @param p_band_centers        Center frequencies of each band.
 * @param p_low_thresholds      Lower excitation thresholds which when exceeded will
 *                              cause the Audio Regulator to begin attenuating.
 * @param p_high_thresholds     Upper excitation thresholds which will not be
 *                              permitted to be exceeded.
 * @param p_isolated_bands      An array of booleans specifying whether the band
 *                              is isolated from the gain-frequency smoothing.
 *                              Non-zero values in this array indicate that the
 *                              band is isolated and surrounding bands will not
 *                              be affected by the amount of regulation being
 *                              applied to it.
 */
int
aml_ex_dap_cpdp_regulator_tuning_set
(void    *p_dap_cpdp
 ,       unsigned           nb_bands
 , const  unsigned          *p_band_centers
 , const  int               *p_low_thresholds
 , const  int               *p_high_thresholds
 , const  int               *p_isolated_bands
);

#define DAP_CPDP_REGULATOR_TUNING_BANDS_MIN            (1u)
#define DAP_CPDP_REGULATOR_TUNING_BANDS_MAX            (20u)
#define DAP_CPDP_REGULATOR_TUNING_CENTER_MIN           (20u)
#define DAP_CPDP_REGULATOR_TUNING_CENTER_MAX           (20000u)
#define DAP_CPDP_REGULATOR_TUNING_THRESHOLD_FRAC_BITS  (4u)
#define DAP_CPDP_REGULATOR_TUNING_THRESHOLD_MIN        (-2080)
#define DAP_CPDP_REGULATOR_TUNING_THRESHOLD_MAX        (0)
#define DAP_CPDP_REGULATOR_TUNING_DEFAULT_NB_BANDS     (2u)
#define DAP_CPDP_REGULATOR_TUNING_DEFAULT_CENTERS      {20u, 20000u}
#define DAP_CPDP_REGULATOR_TUNING_DEFAULT_LOW_THRESHS  {-192, -192}
#define DAP_CPDP_REGULATOR_TUNING_DEFAULT_HIGH_THRESHS {0, 0}
#define DAP_CPDP_REGULATOR_TUNING_DEFAULT_ISOLATES     {0, 0}
/**@}*/

/** @defgroup regulator-overdrive Set Audio Regulator Overdrive
 *  @{*/
/**
 *  This function sets a boost which will be applied to all of the tuned low
 *  and high thresholds (as set by dap_cpdp_regulator_tuning_configure). This
 *  permits the Audio Regulator to increase the maximum output at the expense
 *  of increasing the maximum output distortion. This value only has an effect
 *  when the Audio Regulator is operating in speaker distortion mode (see the
 *  dap_cpdp_regulator_speaker_distortion_enable_set() API function).
 *
 * ### INPUT RANGE
 *  - [0.0, 12.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_REGULATOR_OVERDRIVE_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_REGULATOR_OVERDRIVE_MIN, DAP_CPDP_REGULATOR_OVERDRIVE_MAX]
 *
 * ### DEFAULT VALUE
 *  - 0.0
 *
 * ### NOTES
 *  - If the value is outside the specified range, it will be clipped to be
 *  valid.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param overdrive             Overdrive value.
 */
int
aml_ex_dap_cpdp_regulator_overdrive_set
(void    *p_dap_cpdp
 , int                overdrive
);

#define DAP_CPDP_REGULATOR_OVERDRIVE_MIN       (0)
#define DAP_CPDP_REGULATOR_OVERDRIVE_MAX       (192)
#define DAP_CPDP_REGULATOR_OVERDRIVE_FRAC_BITS (4u)
#define DAP_CPDP_REGULATOR_OVERDRIVE_DEFAULT   (0)
/**@}*/

/** @defgroup regulator-timbre Set Audio Regulator Timbre Preservation
 *  @{*/
/**
 *  This function sets the timbre preservation amount for the Audio Regulator.
 *  This is a dimensionless quantity which specifies a trade-off between
 *  maximum output loudness and preserving the spectral shape of the incoming
 *  signal. Values close to zero maximize loudness and values close to one
 *  maximize the preservation of signal tonality.
 *
 *  This value is used in both the speaker distortion and peak protection
 *  operating modes of the Audio Regulator.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Audio Regulator attached to the Speaker
 *  Virtualizer. The Audio Regulator operating on the Headphone Virtualizer
 *  always has a Timbre Preservation amount of 1.0 with dual-virtualizers.
 *
 * ### INPUT RANGE
 *  - [0.0, 1.0]
 *  - This is represented as a fixed point number with DAP_CPDP_REGULATOR_TP_AMOUNT_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_REGULATOR_TP_AMOUNT_MIN, DAP_CPDP_REGULATOR_TP_AMOUNT_MAX]
 *
 * ### DEFAULT VALUE
 *  - 1.0
 *
 * ### NOTES
 *  - If the value is outside the specified range, it will be clipped to be
 *    valid.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param timbre_preservation   Timbre preservation value.
 */
int
aml_ex_dap_cpdp_regulator_timbre_preservation_set
(void    *p_dap_cpdp
 , int                timbre_preservation
);

#define DAP_CPDP_REGULATOR_TP_AMOUNT_MIN       (0)
#define DAP_CPDP_REGULATOR_TP_AMOUNT_MAX       (16)
#define DAP_CPDP_REGULATOR_TP_AMOUNT_FRAC_BITS (4u)
#define DAP_CPDP_REGULATOR_TP_AMOUNT_DEFAULT   (16)
/**@}*/


/** @defgroup regulator-distortion Set Audio Regulator Distortion Relaxation Amount
 *  @{*/
/**
 *  This function sets the audio regulator distortion relaxation amount.
 *  This parameter specifies the maximum amount that the distortion masking
 *  algorithm can relax (increase) the regulator threshold.
 *
 * ### INPUT RANGE
 *  - [0.0, 9.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_MIN, DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_MAX]
 *
 * ### DEFAULT VALUE
 *  - 6.0
 *
 * ### NOTES
 *  - If the value is outside the specified range, it will be clipped to be
 *    valid.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param relaxation_amount     Regulator relaxation amount.
 */
int
aml_ex_dap_cpdp_regulator_relaxation_amount_set
(void   *p_dap_cpdp
 , int               relaxation_amount
);

#define DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_MIN       (0)
#define DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_MAX       (144)
#define DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_FRAC_BITS (4u)
#define DAP_CPDP_REGULATOR_RELAXATION_AMOUNT_DEFAULT   (96)
/**@}*/

/** @defgroup regulator-mode Set Audio Regulator Operating Mode
 *  @{*/
/**
 *  This function sets the operating mode of the Audio Regulator. There are
 *  two valid modes which the Audio Regulator can operate in:
 *    1. Peak Protection mode
 *     - The tuning configuration supplied by dap_cpdp_regulator_tuning_configure
 *       and dap_cpdp_regulator_overdrive_set is ignored and the Audio Regulator
 *       has the same operating characteristics for each band.
 *    2. Speaker Distortion mode
 *     - The tuning configuration supplied by dap_cpdp_regulator_tuning_configure
 *       and dap_cpdp_regulator_overdrive_set is used to give the Audio Regulator
 *       per-band operating characteristics.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Audio Regulator attached to the Speaker
 *  Virtualizer. The Audio Regulator operating on the Headphone Virtualizer
 *  always operates in Peak Protection mode with dual-virtualizers.
 *
 * ### INPUT RANGE
 *  - 0 - Peak Protection mode
 *  - !0 - Speaker Distortion mode
 *
 * ### DEFAULT VALUE
 *  - DAP_CPDP_REGULATOR_SPEAKER_DIST_ENABLE_DEFAULT
 *
 * @param p_dap_cpdp                    Instance of dap_cpdp state structure.
 * @param speaker_distortion_enable     Regulator mode select.
 */
int
aml_ex_dap_cpdp_regulator_speaker_distortion_enable_set
(void    *p_dap_cpdp
 , int                speaker_distortion_enable
);

#define DAP_CPDP_REGULATOR_SPEAKER_DIST_ENABLE_DEFAULT (0)
/**@}*/

/** @defgroup regulator-enable Set Audio Regulator Enable
 *  @{*/
/**
 *  This function enables or disables the Audio Regulator.
 *
 * ### INPUT RANGE
 *  - 0 - The Audio Regulator is disabled
 *  - !0 - The Audio Regulator is enabled
 *
 * ### DEFAULT VALUE
 *  - DAP_CPDP_REGULATOR_ENABLE_DEFAULT
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param regulator_enable      Regulator enable select.
 */
int
aml_ex_dap_cpdp_regulator_enable_set
(void    *p_dap_cpdp
 , int                regulator_enable
);

#define DAP_CPDP_REGULATOR_ENABLE_DEFAULT  (1)
/**@}*/

/** @defgroup regulator-getter Get Audio Regulator Tuning Info
 *  @{*/

#define DAP_CPDP_REGULATOR_TUNING_INFO_NB_BANDS_MAX             (20u)
#define DAP_CPDP_REGULATOR_TUNING_INFO_GAINS_FRAC_BITS          (4u)
#define DAP_CPDP_REGULATOR_TUNING_INFO_EXCITATIONS_FRAC_BITS    (4u)
#define DAP_CPDP_REGULATOR_TUNING_INFO_GAINS_MIN                (-2080)
#define DAP_CPDP_REGULATOR_TUNING_INFO_GAINS_MAX                (0)
#define DAP_CPDP_REGULATOR_TUNING_INFO_EXCITATIONS_MIN          (-2080)
#define DAP_CPDP_REGULATOR_TUNING_INFO_EXCITATIONS_MAX          (0)

/**
 *  The function gets the number of ERB bands, regulator gains,
 *  and band powers (excitations) seen by the audio regulator.
 *
 *  This information is useful for tuning of the regulator coefficients.
 *
 * ### OUTPUT RANGE
 *  - Band Count: [19, 20]
 *  - Regulator band gains: [-130.0, 0.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_REGULATOR_TUNING_INFO_GAINS_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_REGULATOR_TUNING_INFO_GAINS_MIN, DAP_CPDP_REGULATOR_TUNING_INFO_GAINS_MAX]
 *  - Regulator band excitations: [-130.0, 0.0] dB
 *    - This is represented as a fixed point number with DAP_CPDP_REGULATOR_TUNING_INFO_EXCITATIONS_FRAC_BITS
 *      fractional bits. The raw values are in [DAP_CPDP_REGULATOR_TUNING_INFO_EXCITATIONS_MIN, DAP_CPDP_REGULATOR_TUNING_INFO_EXCITATIONS_MAX]
 *
 * ### NOTE
 *  - The number of bands depend on the sample rate.
 *  - Any of p_nb_bands, p_regulator_gains and p_band_powers may be NULL.
 *  - If regulator is disabled, p_regulator_gains and p_band_powers return all zero.
 *
 * @param p_dap_cpdp                Instance of dap_cpdp state structure.
 * @param p_nb_bands                Number of bands of data supplied to the regulator.
 * @param p_regulator_gains         Regulator gain information.
 * @param p_regulator_excitations   Regulator excitation information.
 */
int
aml_ex_dap_cpdp_regulator_tuning_info_get
(void     *p_dap_cpdp
 , unsigned int       *p_nb_bands
 , int                 p_regulator_gains[DAP_CPDP_REGULATOR_TUNING_INFO_NB_BANDS_MAX]
 , int                 p_regulator_excitations[DAP_CPDP_REGULATOR_TUNING_INFO_NB_BANDS_MAX]
);

/**@}*/

/** @defgroup virtual-bass-enable Set Virtual Bass mode
 *  @{*/
/**
 *  This function sets Virtual Bass working mode.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Virtual Bass attached to the Speaker
 *  Virtualizer. The Virtual Bass operating on the Headphone Virtualizer
 *  always operates with default tuning parameter values in dual-virtualizer or dual-stereo mode.
 *  Since the default value is DAP_CPDP_VIRTUAL_BASS_MODE_DELAY, virtual bass node on Headphone
 *  Virtualizer would work in delay only mode, which means virtual bass will not change the input
 *  signal except the latency introduced.
 *
 * ### INPUT RANGE
 *  - DAP_CPDP_VIRTUAL_BASS_MODE_DELAY - delay only
 *  - DAP_CPDP_VIRTUAL_BASS_MODE_MIN   - only 2nd order harmonics generated
 *  - DAP_CPDP_VIRTUAL_BASS_MODE_MID   - only 2nd and 3rd order harmonics generated
 *  - DAP_CPDP_VIRTUAL_BASS_MODE_FULL  - all of the 2nd, 3rd and 4th order harmonics generated
 * ### DEFAULT VALUE
 *  - DAP_CPDP_VIRTUAL_BASS_MODE_DELAY
 *
 * ### NOTE
 *  - If an invalid value is given, then it will be ignored and no change will
 *    be made to the virtual bass mode.
 *
 * @param p_dap_cpdp              Instance of dap_cpdp state structure.
 * @param mode                    Virtual Bass mode select.
 */
int
aml_ex_dap_cpdp_virtual_bass_mode_set
(void    *p_dap_cpdp
 , int                mode
);

#define DAP_CPDP_VIRTUAL_BASS_MODE_DELAY           (0)
#define DAP_CPDP_VIRTUAL_BASS_MODE_MIN             (1)
#define DAP_CPDP_VIRTUAL_BASS_MODE_MID             (2)
#define DAP_CPDP_VIRTUAL_BASS_MODE_FULL            (3)
#define DAP_CPDP_VIRTUAL_BASS_MODE_DEFAULT         DAP_CPDP_VIRTUAL_BASS_MODE_DELAY
/**@}*/

/** @defgroup virtual-bass-src-freqs Set Virtual Bass source low and high frequency boundaries
 *  @{*/
/**
 *  This function sets the lowest and highest frequency boundaries that defines the source frequency range
 *  for Virtual Bass transpose.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Virtual Bass attached to the Speaker
 *  Virtualizer. The Virtual Bass operating on the Headphone Virtualizer
 *  always operates with default tuning parameter values in dual-virtualizer or dual-stereo mode.
 *
 * ### INPUT RANGE
 *  - low frequency in  [30, 90] Hz
 *  - high frequency in [90, 270]Hz
 *
 * ### DEFAULT VALUE
 *  - low frequency is  35  Hz
 *  - high frequency is 160 Hz
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input is out of the valid range.
 *    The low frequency must be lower than high frequency otherwise this setting will not take effect.
 *  - For example, low freq 100 Hz will be clipped to 90 Hz.
 *
 * @param p_dap_cpdp               Instance of dap_cpdp state structure.
 * @param low_src_freq             Virtual Bass source low frequency.
 * @param high_src_freq            Virtual Bass source high frequency.
 */
int
aml_ex_dap_cpdp_virtual_bass_src_freqs_set
(void    *p_dap_cpdp
 , unsigned           low_src_freq
 , unsigned           high_src_freq
);
#define DAP_CPDP_VIRTUAL_BASS_LOW_SRC_FREQ_MAX       (90)
#define DAP_CPDP_VIRTUAL_BASS_LOW_SRC_FREQ_MIN       (30)
#define DAP_CPDP_VIRTUAL_BASS_LOW_SRC_FREQ_DEFAULT   (35)
#define DAP_CPDP_VIRTUAL_BASS_HIGH_SRC_FREQ_MAX      (270)
#define DAP_CPDP_VIRTUAL_BASS_HIGH_SRC_FREQ_MIN      (90)
#define DAP_CPDP_VIRTUAL_BASS_HIGH_SRC_FREQ_DEFAULT  (160)
/**@}*/

/** @defgroup virtual-bass-overall-gain Set Virtual Bass overall gain
 *  @{*/
/**
 *  This function sets Virtual Bass overall gain which is applied to the output of transposer.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Virtual Bass attached to the Speaker
 *  Virtualizer. The Virtual Bass operating on the Headphone Virtualizer
 *  always operates with default tuning parameter values in dual-virtualizer or dual-stereo mode.
 *
 * ### INPUT RANGE
 *  - [-30.0, 0.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_MIN, DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_MAX]
 *
 * ### DEFAULT VALUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - The subgains and overall gain will be summed up before applied to the corresponding order of harmonics. When the
 *    result is out of the valid range [-30.0, 0.0] dB, it will be clipped to valid range.
 *  - For example, when the subgain of 2nd order harmonics is set to -10.0 dB and the overall gain is -25.0 dB, then
 *    actual gain of 2nd order harmonics would be clipped to -30.0 dB as -35.0 dB is out of the range of [-30.0, 0.0] dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Virtual Bass overall gain.
 */

int
aml_ex_dap_cpdp_virtual_bass_overall_gain_set
(void    *p_dap_cpdp
 , int                value
);

#define DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_FRAC_BITS        (4)
#define DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_DEFAULT          (0)
#define DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_MIN              (-480)
#define DAP_CPDP_VIRTUAL_BASS_OVERALL_GAIN_MAX              (0)
/**@}*/


/** @defgroup virtual-bass-slope-gain Set Virtual Bass slope gain
 *  @{*/
/**
 *  This function sets Virtual Bass slope gain which is used to adjust the envelope of the transposer output.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Virtual Bass attached to the Speaker
 *  Virtualizer. The Virtual Bass operating on the Headphone Virtualizer
 *  always operates with default tuning parameter values in dual-virtualizer or dual-stereo mode.
 *
 * ### INPUT RANGE
 *  -  discrete value in [-3.0, -2.0, -1.0, 0.0] dB
 *
 * ### DEFAULT VALUE
 *  - 0.0 dB
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input
 *    is out of the valid range.
 *  - For example, -4.0 dB input will be clipped to -3.0 dB.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param value                 Virtual Bass slope gain.
 */

int
aml_ex_dap_cpdp_virtual_bass_slope_gain_set
(void    *p_dap_cpdp
 , int                value
);
#define DAP_CPDP_VIRTUAL_BASS_SLOPE_GAIN_DEFAULT          (0)
#define DAP_CPDP_VIRTUAL_BASS_SLOPE_GAIN_MIN              (-3)
#define DAP_CPDP_VIRTUAL_BASS_SLOPE_GAIN_MAX              (0)
/**@}*/


/** @defgroup virtual-bass-subgains Set Virtual Bass subgains
 *  @{*/
/**
 *  This function sets Virtual Bass subgains for the 2nd, 3rd and 4th harmonics.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Virtual Bass attached to the Speaker
 *  Virtualizer. The Virtual Bass operating on the Headphone Virtualizer
 *  always operates with default tuning parameter values in dual-virtualizer or dual-stereo mode.
 *
 * ### INPUT RANGE
 *  - [-30.0, 0.0] dB
 *  - This is represented as a fixed point number with DAP_CPDP_VIRTUAL_BASS_SUBGAINS_FRAC_BITS
 *    fractional bits. The raw values are in [DAP_CPDP_VIRTUAL_BASS_SUBGAINS_MIN, DAP_CPDP_VIRTUAL_BASS_SUBGAINS_MAX]
 *
 * ### DEFAULT VALUE
 *  - {-2.0, -9.0, -12.0} dB represents the gains of 2nd, 3rd, 4th order harmonics respectively.
 *
 * ### NOTE
 *  - The subgains and overall gain will be summed up before applied to the corresponding order of harmonics. When the
 *    result is out of the valid range [-30.0, 0.0] dB, it will be clipped to valid range.
 *  - For example, when the subgain of 2nd order harmonics is set to -10.0 dB and the overall gain is -25.0 dB, then
 *    actual gain of 2nd order harmonics would be clipped to -30.0 dB as -35.0 dB is out of the range of [-30.0, 0.0] dB.
 *
 * @param p_dap_cpdp                   Instance of dap_cpdp state structure.
 * @param size                         Number of subgains.
 * @param p_subgains                   Virtual Bass subgains.
 */
int
aml_ex_dap_cpdp_virtual_bass_subgains_set
(void    *p_dap_cpdp
 , unsigned           size
 , int               *p_subgains
);

#define DAP_CPDP_VIRTUAL_BASS_SUBGAINS_NUM          (3)
#define DAP_CPDP_VIRTUAL_BASS_SUBGAINS_FRAC_BITS    (4)
#define DAP_CPDP_VIRTUAL_BASS_SUBGAINS_DEFAULT      {-32, -144, -192}
#define DAP_CPDP_VIRTUAL_BASS_SUBGAINS_MIN          (-480)
#define DAP_CPDP_VIRTUAL_BASS_SUBGAINS_MAX          (0)

/**@}*/

/** @defgroup virtual-bass-mixed-freqs Set Virtual Bass mixed frequency boundaries
 *  @{*/
/**
 *  This function sets virtual bass the lower and upper frequency boundaries that define
 *  the transposed harmonics which need to be mixed.
 *
 *  When Dolby Audio Processing is operating in dual-virtualizer or dual-stereo mode, this
 *  setting will only affect the Virtual Bass attached to the Speaker
 *  Virtualizer. The Virtual Bass operating on the Headphone Virtualizer
 *  always operates with default tuning parameter values in dual-virtualizer or dual-stereo mode.
 *
 * ### INPUT RANGE
 *  - low frequency in [0, 375]Hz
 *  - high frequency in [281, 938]Hz
 *
 * ### DEFAULT VALUE
 *  - DAP_CPDP_VIRTUAL_BASS_LOW_MIX_FREQ_DEFAULT
 *  - DAP_CPDP_VIRTUAL_BASS_HIGH_MIX_FREQ_DEFAULT
 *
 * ### NOTE
 *  - It will clip the input to valid range if the input is out of the valid range.
 *    The low frequency must not be higher than high frequency
 *    otherwise this setting will not take effect.
 *    - For example, low freq 400 Hz will be clipped to 375 Hz.
 *  - The actual mixing range would be no greater than the range specified by low_mix_freq
 *    and high_mix_freq, however it could be smaller.
 *    - For example, when setting the boundary as [94, 938] Hz at 32 kHz, the actual mix
 *      range would be [125, 625] Hz.
 *
 * @param p_dap_cpdp            Instance of dap_cpdp state structure.
 * @param low_mix_freq          Virtual Bass low mix frequency.
 * @param high_mix_freq         Virtual Bass high mix frequency.
 */
int
aml_ex_dap_cpdp_virtual_bass_mix_freqs_set
(void    *p_dap_cpdp
 , unsigned           low_mix_freq
 , unsigned           high_mix_freq
);
#define DAP_CPDP_VIRTUAL_BASS_LOW_MIX_FREQ_MIN         (0)
#define DAP_CPDP_VIRTUAL_BASS_LOW_MIX_FREQ_MAX         (375)
#define DAP_CPDP_VIRTUAL_BASS_LOW_MIX_FREQ_DEFAULT     (94)

#define DAP_CPDP_VIRTUAL_BASS_HIGH_MIX_FREQ_MIN        (281)
#define DAP_CPDP_VIRTUAL_BASS_HIGH_MIX_FREQ_MAX        (938)
#define DAP_CPDP_VIRTUAL_BASS_HIGH_MIX_FREQ_DEFAULT    (469)


/*---------------------------------------------------------------------------------*/
/*DAP_CPDP_DV_PARAMS*/
/*---------------------------------------------------------------------------------*/

size_t dap_cpdp_pvt_dv_params_query_memory(void);

int
aml_ex_dap_cpdp_pvt_dv_params_init
(void    *p_params
 , const  unsigned                   *p_fixed_band_frequencies
 ,       unsigned                    nb_fixed_bands
 ,       void                       *p_mem
);

int
aml_ex_dap_cpdp_pvt_volume_and_ieq_update_control
(void    *p_params
 , unsigned                    nb_fixed_bands
);

/* Returns true if DV is currently enabled */
int
aml_ex_dap_cpdp_pvt_dv_enabled
(const void   *p_params
);

/* Returns true if DV is going to be enabled after the next call to update
 * control. This must be called under the parameter lock. */
int
aml_ex_dap_cpdp_pvt_async_dv_enabled
(const void   *p_params
);

/* Returns true if either the volume leveler or volume modeler is
 * going to be enabled after the next call to update control.
 * This must be called under the parameter lock. */
int
aml_ex_dap_cpdp_pvt_async_leveler_or_modeler_enabled
(const void   *p_params
);


#ifdef __cplusplus
}
#endif

#endif //__DOLBY_ADUIO_PROCESSING_CONTROL_H__
