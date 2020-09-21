/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2011-2018 ARM or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include <linux/delay.h>
#include "acamera_types.h"
#include "system_spi.h"
#include "system_sensor.h"
#include "sensor_bus_config.h"
#include "acamera_command_api.h"
#include "acamera_sbus_api.h"
#include "acamera_sensor_api.h"
#include "system_timer.h"
#include "sensor_init.h"
#include "IMX307_seq.h"
#include "IMX307_config.h"
#include "acamera_math.h"
#include "system_am_mipi.h"
#include "system_am_adap.h"
#include "sensor_bsp_common.h"

#define NEED_CONFIG_BSP 1

// As per IMX307 datasheet
// Max gain is 69 DB
// Analog gain is 0 - 27 dB (step pitch 0.3 dB)
// Digital gain is 27.3 dB to 69 dB
// Gain set in register as (10/3 * gain)
#define AGAIN_MAX_DB 0x5A  // (27 * 10/3)
#define DGAIN_MAX_DB 0x8C  // (42 * 10/3)

// These offsets are calculated from the start of the effective
// margin for color processing
#define VERTICAL_OFFSET_TO_RECORDING_PIXEL_SHORT_FRAME  0x08
#define VERTICAL_OFFSET_TO_RECORDING_PIXEL_LONG_FRAME   0x08

// Horizontal offesets of recording pixel is sum of
// Ignored effective pixed area(4) +  effective color processing
// margin(8)
#define HORIZONTAL_OFFSET_TO_RECORDING_PIXEL 12

// Vmax for IMX307 is 1125. This can be changed in order to support
// long exposure operation
//#define SENSOR_VMAX_12bit 1125

// Basic read out lines from the specs
#define WDR_2_DOL_1080P_BRL 1109
#define WDR_2_DOL_720P_BRL  735



static int sen_mode = 0;
static void start_streaming( void *ctx );
static void stop_streaming( void *ctx );

static sensor_mode_t supported_modes[] = {
    {
        .wdr_mode = WDR_MODE_LINEAR, // 4 Lanes
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 1,
        .lanes = 4,
        .bps = 446,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
        .num = SENSOR_IMX307_SEQUENCE_1080P_30FPS_12BIT_4LANE,
    },
    {
        .wdr_mode = WDR_MODE_LINEAR, // 4 Lanes
        .fps = 60 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 1,
        .lanes = 4,
        .bps = 446,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_NON,
        .num = SENSOR_IMX307_SEQUENCE_1080P_60FPS_12BIT_4LANE,
    },
/*
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 12,
        .exposures = 2,
        .lanes = 4,
        .bps = 446,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_LINEINFO,
        .num = SENSOR_IMX307_SEQUENCE_1080P_30FPS_12BIT_4LANE_WDR,
    }
*/
    {
        .wdr_mode = WDR_MODE_FS_LIN,
        .fps = 30 * 256,
        .resolution.width = 1920,
        .resolution.height = 1080,
        .bits = 10,
        .exposures = 2,
        .lanes = 4,
        .bps = 446,
        .bayer = BAYER_RGGB,
        .dol_type = DOL_LINEINFO,
        .num = SENSOR_IMX307_SEQUENCE_1080P_30FPS_10BIT_4LANE_WDR,
    }
};


typedef struct _sensor_context_t {
    uint8_t address; // Sensor address for direct write (not used currently)
    uint8_t seq_width;
    uint8_t streaming_flg;
    uint16_t again[4];
    uint8_t again_delay;
    uint16_t dgain;
    uint8_t dgain_change;
    uint16_t int_time_S;
    uint16_t int_time_M;
    uint16_t int_time_L;
    uint32_t shs1;
    uint32_t shs2;
    uint32_t shs3;
    uint32_t shs1_old;
    uint32_t shs2_old;
    uint32_t rhs1;
    uint32_t rhs2;
    uint32_t again_limit;
    uint32_t dgain_limit;
    uint8_t s_fps;
    uint32_t vmax;
    uint8_t int_cnt;
    uint8_t gain_cnt;
    uint32_t pixel_clock;
    uint16_t max_S;
    uint16_t max_M;
    uint16_t max_L;
    uint16_t frame;
    uint32_t wdr_mode;
    acamera_sbus_t sbus;
    sensor_param_t param;
    void *sbp;
} sensor_context_t;

//-------------------------------------------------------------------------------------
#if SENSOR_BINARY_SEQUENCE
static const char p_sensor_data[] = SENSOR_IMX307_SEQUENCE_DEFAULT;
static const char p_isp_data[] = SENSOR_IMX307_ISP_CONTEXT_SEQ;
#else
static const acam_reg_t **p_sensor_data = imx307_seq_table;
static const acam_reg_t **p_isp_data = isp_seq_table;
#endif
//--------------------RESET------------------------------------------------------------
static void sensor_hw_reset_enable( void )
{
    system_reset_sensor( 0 );
}

static void sensor_hw_reset_disable( void )
{
    system_reset_sensor( 3 );
}

//-------------------------------------------------------------------------------------
static int32_t sensor_alloc_analog_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;

    // Input gain is in Log2 format of linear gain with LOG2_GAIN_SHIFT
    // Gain(dB) = (20 * Log10(linear_gain)) = 20 * Log2(linear_gain) * Log10(2)
    // As per specs, analog gain register setting is calculated as
    // gain(register) = gain(dB) * 10 / 3
    uint16_t again = ( 20 * gain ) >> LOG2_GAIN_SHIFT;
    if ( again > p_ctx->again_limit ) again = p_ctx->again_limit;

    if ( p_ctx->again[0] != again ) {
        p_ctx->gain_cnt = p_ctx->again_delay + 1;
        p_ctx->again[0] = again;
    }

    return ( ( (int32_t)again ) << LOG2_GAIN_SHIFT ) / 20;
}

static int32_t sensor_alloc_digital_gain( void *ctx, int32_t gain )
{
    sensor_context_t *p_ctx = ctx;

    // Input gain is in Log2 format of linear gain with LOG2_GAIN_SHIFT
    // Gain(dB) = (20 * Log10(linear_gain)) = 20 * Log2(linear_gain) * Log10(2)
    // As per specs, gain register setting is calculated as
    // gain(register) = gain(dB) * 10 / 3
    uint16_t dgain = ( 20 * gain ) >> LOG2_GAIN_SHIFT;

    if ( dgain > p_ctx->dgain_limit ) dgain = p_ctx->dgain_limit;

    if ( p_ctx->dgain != dgain ) {
        p_ctx->dgain_change = 1;
        p_ctx->dgain = dgain;
    }

    return ( ( (int32_t)dgain ) << LOG2_GAIN_SHIFT ) / 20;
}

static void sensor_alloc_integration_time(
    void *ctx, uint16_t *int_time_S, uint16_t *int_time_M, uint16_t *int_time_L)
{
    sensor_context_t *p_ctx = ctx;

    switch ( p_ctx->wdr_mode ) {
    case WDR_MODE_LINEAR: // Normal mode
        if ( *int_time_S > p_ctx->vmax - 2 ) *int_time_S = p_ctx->vmax - 2;
        if ( *int_time_S < 1 ) *int_time_S = 1;
        // Integration time is calculated using following formula
        // Integration time = 1 frame period - (SHS1 + 1) lines
        uint16_t shs1 = p_ctx->vmax - *int_time_S - 1;;
        if ( p_ctx->shs1 != shs1 ) {
            p_ctx->int_cnt = 2;
            p_ctx->shs1 = shs1;
            p_ctx->int_time_S = *int_time_S;
        }
        break;
    case WDR_MODE_FS_LIN: // DOL3 Frames
#ifdef FS_LIN_3DOL
        if ( *int_time_S < 2 ) *int_time_S = 2;
        if ( *int_time_S > p_ctx->max_S ) *int_time_S = p_ctx->max_S;
        if ( *int_time_L < 2 ) *int_time_L = 2;
        if ( *int_time_L > p_ctx->max_L ) *int_time_L = p_ctx->max_L;

        if ( *int_time_M < 2 ) *int_time_M = 2;
        if ( *int_time_M > p_ctx->max_M ) *int_time_M = p_ctx->max_M;

        if ( p_ctx->int_time_S != *int_time_S ||
            p_ctx->int_time_M != *int_time_M ||
            p_ctx->int_time_L != *int_time_L ) {
            p_ctx->int_cnt = 3;

            p_ctx->int_time_S = *int_time_S;
            p_ctx->int_time_M = *int_time_M;
            p_ctx->int_time_L = *int_time_L;

            p_ctx->shs3 = p_ctx->frame - *int_time_L - 1;
            p_ctx->shs1 = p_ctx->rhs1 - *int_time_M - 1;
            p_ctx->shs2 = p_ctx->rhs2 - *int_time_S - 1;
        }
#else  //default DOL2 Frames
        if ( *int_time_S < 2 ) *int_time_S = 2;
        if ( *int_time_S > p_ctx->max_S ) *int_time_S = p_ctx->max_S;
        if ( *int_time_L < 2 ) *int_time_L = 2;
        if ( *int_time_L > p_ctx->max_L ) *int_time_L = p_ctx->max_L;

        if ( p_ctx->int_time_S != *int_time_S ||
             p_ctx->int_time_L != *int_time_L ) {
            p_ctx->int_cnt = 2;

            p_ctx->int_time_S = *int_time_S;
            p_ctx->int_time_L = *int_time_L;

            p_ctx->shs2 = p_ctx->frame - *int_time_L - 1;
            p_ctx->shs1 = p_ctx->rhs1 - *int_time_S - 1;
        }
#endif
        break;
    }
}

static int32_t sensor_ir_cut_set( void *ctx, int32_t ir_cut_state )
{
    sensor_context_t *t_ctx = ctx;
    int ret;
    sensor_bringup_t* sensor_bp = t_ctx->sbp;

    LOG( LOG_ERR, "ir_cut_state = %d", ir_cut_state);
    LOG( LOG_INFO, "entry ir cut" );

//ir_cut_GPIOZ_7 =1 && ir_cut_GPIOZ_11=0, open ir cut
//ir_cut_GPIOZ_7 =0 && ir_cut_GPIOZ_11=1, close ir cut
//ir_cut_srate, 2: no operation

   if (sensor_bp->ir_gname[0] <= 0 && sensor_bp->ir_gname[1] <= 0) {
       pr_err("get gpio id fail\n");
       return 0;
   }

   if (ir_cut_state == 1)
        {
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[1], 1);
            if (ret < 0 )
            pr_err("set power fail\n");

            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 0);
            if (ret < 0 )
            pr_err("set power fail\n");

            mdelay(500);
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 1);
            if (ret < 0 )
            pr_err("set power fail\n");
        }
    else if(ir_cut_state == 0)
        {
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[1], 0);
            if (ret < 0 )
            pr_err("set power fail\n");

            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[0], 1);
            if (ret < 0 )
            pr_err("set power fail\n");

            mdelay(500);
            ret = pwr_ir_cut_enable(sensor_bp, sensor_bp->ir_gname[1], 1);
            if (ret < 0 )
            pr_err("set power fail\n");
       }
    else if(ir_cut_state == 2)
        return 0;
    else
        LOG( LOG_ERR, "sensor ir cut set failed" );

    LOG( LOG_INFO, "exit ir cut" );
    return 0;
}

static void sensor_update( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;

    if ( p_ctx->int_cnt || p_ctx->gain_cnt || p_ctx->dgain_change) {
        // ---------- Start Changes -------------
        acamera_sbus_write_u8( p_sbus, 0x3001, 1 );

        // ---------- Analog & Digital Gain -------------
        // IMX307 uses same register to update analog and digital gain
        // gain = analog gain (0 - 27dB) + digital gain (0 - 42dB)
        if ( p_ctx->gain_cnt || p_ctx->dgain_change) {
            uint16_t reg_gain = 0;
            if ( p_ctx->gain_cnt ) {
                reg_gain = p_ctx->again[p_ctx->again_delay];
                p_ctx->gain_cnt--;
            }
            if ( p_ctx->dgain_change ) {
                reg_gain = p_ctx->again[p_ctx->again_delay] + p_ctx->dgain;
                p_ctx->dgain_change = 0;
            }
            acamera_sbus_write_u8(
                p_sbus, 0x3014, reg_gain );
        }

        // -------- Integration Time ----------
        if ( p_ctx->int_cnt ) {
            p_ctx->int_cnt--;
            switch ( p_ctx->wdr_mode ) {
            case WDR_MODE_LINEAR:
                acamera_sbus_write_u8(
                    p_sbus, 0x3021, ( p_ctx->shs1 >> 8 ) & 0xFF );
                acamera_sbus_write_u8(
                    p_sbus, 0x3020, ( p_ctx->shs1 >> 0 ) & 0xFF );
                break;
            case WDR_MODE_FS_LIN:
                p_ctx->shs2_old = p_ctx->shs2;
                p_ctx->shs1_old = p_ctx->shs1;
#ifdef FS_LIN_3DOL
                // SHS3
                acamera_sbus_write_u8(
                    p_sbus, 0x3029, ( p_ctx->shs3 >> 8 ) & 0xFF );
                acamera_sbus_write_u8(
                    p_sbus, 0x3028, ( p_ctx->shs3 >> 0 ) & 0xFF );
#endif
                // SHS1
                acamera_sbus_write_u8(
                    p_sbus, 0x3021, ( p_ctx->shs1_old >> 8 ) & 0xFF );
                acamera_sbus_write_u8(
                    p_sbus, 0x3020, ( p_ctx->shs1_old >> 0 ) & 0xFF );

                // SHS2
                acamera_sbus_write_u8(
                    p_sbus, 0x3025, ( p_ctx->shs2_old >> 8 ) & 0xFF );
                acamera_sbus_write_u8(
                    p_sbus, 0x3024, ( p_ctx->shs2_old >> 0 ) & 0xFF );
                break;
            }
        }

        // ---------- End Changes -------------
        acamera_sbus_write_u8( p_sbus, 0x3001, 0 );
    }
    p_ctx->shs1_old = p_ctx->shs1;
    p_ctx->shs2_old = p_ctx->shs2;
    p_ctx->again[3] = p_ctx->again[2];
    p_ctx->again[2] = p_ctx->again[1];
    p_ctx->again[1] = p_ctx->again[0];
}

static uint16_t sensor_get_id( void *ctx )
{
    /* return that sensor id register does not exist */
    sensor_context_t *p_ctx = ctx;
    uint32_t sensor_id = 0;

    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x301e) << 8;
    sensor_id |= acamera_sbus_read_u8(&p_ctx->sbus, 0x301f);

    if (sensor_id != SENSOR_CHIP_ID) {
        LOG(LOG_ERR, "%s: Failed to read sensor id\n", __func__);
        return 0xFF;
    }
    return 0;
}

static void sensor_set_iface(sensor_mode_t *mode)
{
    am_mipi_info_t mipi_info;
    struct am_adap_info info;

    if (mode == NULL) {
        LOG(LOG_ERR, "Error input param\n");
        return;
    }

    memset(&mipi_info, 0, sizeof(mipi_info));
    memset(&info, 0, sizeof(struct am_adap_info));
    mipi_info.fte1_flag = get_fte1_flag();
    mipi_info.lanes = mode->lanes;
    mipi_info.ui_val = 1000 / mode->bps;

    if ((1000 % mode->bps) != 0)
        mipi_info.ui_val += 1;

    am_mipi_init(&mipi_info);

    switch (mode->bits) {
    case 10:
        info.fmt = AM_RAW10;
        break;
    case 12:
        info.fmt = AM_RAW12;
        break;
    default :
        info.fmt = AM_RAW10;
        break;
    }

    info.img.width = mode->resolution.width;
    info.img.height = mode->resolution.height;
    info.offset.offset_x = 4 + 8;
    info.path = PATH0;
    if (mode->wdr_mode == WDR_MODE_FS_LIN) {
        info.mode = DOL_MODE;
        info.type = mode->dol_type;
        if (info.type == DOL_LINEINFO) {
           // Long and short offsets are calculated from the start of valid pixels
           // i.e. Effective margins should be skipped
           info.offset.long_offset = VERTICAL_OFFSET_TO_RECORDING_PIXEL_LONG_FRAME;
           info.offset.short_offset = VERTICAL_OFFSET_TO_RECORDING_PIXEL_SHORT_FRAME;
        }
    } else
        info.mode = DIR_MODE;
    am_adap_set_info(&info);
    am_adap_init();
    am_adap_start(0);
}

static void sensor_set_mode( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    sensor_param_t *param = &p_ctx->param;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    uint8_t setting_num = 0;
    uint16_t s_id = 0xff;
    sen_mode = mode;

    sensor_hw_reset_enable();
    system_timer_usleep( 10000 );
    sensor_hw_reset_disable();
    system_timer_usleep( 10000 );

    setting_num = param->modes_table[mode].num;

    s_id = sensor_get_id(ctx);
    if (s_id != 0)
        return;
    switch ( param->modes_table[mode].wdr_mode ) {
    case WDR_MODE_LINEAR:
        sensor_load_sequence(
            p_sbus, p_ctx->seq_width, p_sensor_data, setting_num );
        p_ctx->s_fps = 30;
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;
        break;
    case WDR_MODE_FS_LIN:
        p_ctx->again_delay = 0;
        param->integration_time_apply_delay = 2;
        param->isp_exposure_channel_delay = 0;

        sensor_load_sequence(
            p_sbus, p_ctx->seq_width, p_sensor_data, setting_num);
        p_ctx->s_fps = 30;
        break;
    default:
        LOG(LOG_ERR, "Invalide wdr mode. Returning!");
        return;
    }

    p_ctx->vmax = (((uint32_t)acamera_sbus_read_u8(p_sbus,0x3019)<<8)|acamera_sbus_read_u8(p_sbus,0x3018));

    uint8_t r = ( acamera_sbus_read_u8( p_sbus, 0x3007 ) >> 4 );
    switch ( r ) {
    case 0: // HD 1080p
        param->active.width = 1920;
        param->active.height = 1080;
        // TODO (brgurung) In order to support specific exposure ratio
        // (8:1) this rhs1 limit may not be enough. Need to check if VMAX
        // and rhs1 needs to be updated.

        // RHS1 should be between 2n+5 and (FSC - BRL * 2 - 21)
        // FSC = 2 * Vmax = 2250
        p_ctx->rhs1 = 2 * p_ctx->vmax - WDR_2_DOL_1080P_BRL * 2 - 21;

        // Long integration time = FSC - (SHS2 + 1)
        // SHS2 should be between 2 and RHS1 - 2
        p_ctx->max_L = 2 * p_ctx->vmax - 3;

        // Short integration time = RHS1 - (SHS1 + 1)
        // Shutter time SHS1 should be between 2 and RHS1 - 2
        p_ctx->max_S = p_ctx->rhs1 - 3;
        break;
    case 1:  //HD 720P
        param->active.width = 1280;
        param->active.height = 720;
        p_ctx->rhs1 = 2 * p_ctx->vmax - WDR_2_DOL_720P_BRL * 2 - 21;
        p_ctx->max_L = 2 * p_ctx->vmax - 3;
        p_ctx->max_S = p_ctx->rhs1 - 3;
        break;
    default:
        LOG(LOG_INFO, "Invalid window mode");
        break;
    }

    // Enable syncs on XHS and XVS pins
    //acamera_sbus_write_u8(p_sbus, 0x024b, 0x0A);
    if (param->modes_table[mode].exposures > 1) {
        acamera_sbus_write_u8( p_sbus, 0x3031, ( p_ctx->rhs1 >> 8 ) & 0xFF );
        acamera_sbus_write_u8( p_sbus, 0x3030, ( p_ctx->rhs1 >> 0 ) & 0xFF );
        //acamera_sbus_write_u8( p_sbus, 0x3035, ( p_ctx->rhs2 >> 8 ) & 0xFF );
        //acamera_sbus_write_u8( p_sbus, 0x3034, ( p_ctx->rhs2 >> 0 ) & 0xFF );
    }

    param->total.width =
        ( (uint16_t)acamera_sbus_read_u8( p_sbus, 0x301D ) << 8 ) |
        acamera_sbus_read_u8( p_sbus, 0x301C );
    param->lines_per_second = p_ctx->pixel_clock / param->total.width;
    param->total.height = (uint16_t)p_ctx->vmax;
    param->pixels_per_line = param->total.width;
    param->integration_time_min = SENSOR_MIN_INTEGRATION_TIME;
    if ( param->modes_table[mode].wdr_mode == WDR_MODE_LINEAR ) {
        param->integration_time_limit = p_ctx->vmax - 2;
        param->integration_time_max = p_ctx->vmax - 2;
    } else {
        param->integration_time_limit = p_ctx->max_S;
        param->integration_time_max = p_ctx->max_S;
        if ( param->modes_table[mode].exposures == 2 ) {
            param->integration_time_long_max = ( p_ctx->vmax << 1 ) - 256;
            param->lines_per_second = param->lines_per_second >> 1;
            p_ctx->frame = p_ctx->vmax << 1;
        } else {
            param->integration_time_long_max = ( p_ctx->vmax << 2 ) - 256;
            param->lines_per_second = param->lines_per_second >> 2;
            p_ctx->frame = p_ctx->vmax << 2;
        }
    }
    param->sensor_exp_number = param->modes_table[mode].exposures;
    param->mode = mode;
    p_ctx->wdr_mode = param->modes_table[mode].wdr_mode;
    param->bayer = param->modes_table[mode].bayer;

    sensor_set_iface(&param->modes_table[mode]);

    LOG( LOG_ERR, "Output resolution from sensor: %dx%d",
         param->active.width, param->active.height );
}

static const sensor_param_t *sensor_get_parameters( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    return (const sensor_param_t *)&p_ctx->param;
}

static void sensor_disable_isp( void *ctx )
{
}

static uint32_t read_register( void *ctx, uint32_t address )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    return acamera_sbus_read_u8( p_sbus, address );
}

static void write_register( void *ctx, uint32_t address, uint32_t data )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    acamera_sbus_write_u8( p_sbus, address, data );
}

static void stop_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    p_ctx->streaming_flg = 0;

    acamera_sbus_write_u8( p_sbus, 0x3000, 0x01 );

    reset_sensor_bus_counter();
    am_adap_deinit();
    am_mipi_deinit();
}

static void start_streaming( void *ctx )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    sensor_param_t *param = &p_ctx->param;
    sensor_set_iface(&param->modes_table[sen_mode]);
    p_ctx->streaming_flg = 1;
    acamera_sbus_write_u8( p_sbus, 0x3000, 0x00 );

#ifdef ACAMERA_FPGA_SENSOR_START_AUTO_CALIBRATION_FSM_0123_DEFAULT
    // trigger auto calibration process for the imx307 sensor
    acamera_fpga_sensor_start_auto_calibration_fsm_0123_write( 0, 0 );
    acamera_fpga_sensor_start_auto_calibration_fsm_0123_write( 0, 1 );

    acamera_fpga_sensor_start_auto_calibration_fsm_4567_write( 0, 0 );
    acamera_fpga_sensor_start_auto_calibration_fsm_4567_write( 0, 1 );
#endif
}

static void sensor_test_pattern( void *ctx, uint8_t mode )
{
    sensor_context_t *p_ctx = ctx;
    acamera_sbus_ptr_t p_sbus = &p_ctx->sbus;
    // acamera_3aalg_preset will call sensor_test_pattern with mode 0xff during init, do not set test pattern.
    if (mode == 0xff) {
        return;
    }
    sensor_load_sequence( p_sbus, p_ctx->seq_width, p_sensor_data, SENSOR_IMX307_SEQUENCE_DEFAULT_TEST_PATTERN );
}

void sensor_deinit_imx307( void *ctx )
{
    sensor_context_t *t_ctx = ctx;
    reset_sensor_bus_counter();
    am_adap_deinit();
    am_mipi_deinit();
    acamera_sbus_deinit(&t_ctx->sbus,  sbus_i2c);
    if (t_ctx != NULL && t_ctx->sbp != NULL)
        clk_am_disable(t_ctx->sbp);
}
//--------------------Initialization------------------------------------------------------------
void sensor_init_imx307( void **ctx, sensor_control_t *ctrl, void* sbp)
{
    // Local sensor data structure
    static sensor_context_t s_ctx;
    int ret;
    sensor_bringup_t* sensor_bp = (sensor_bringup_t*) sbp;
    *ctx = &s_ctx;
    s_ctx.sbp = sbp;

#if NEED_CONFIG_BSP
    ret = reset_am_enable(sensor_bp,"reset", 1);
    if (ret < 0 )
        pr_info("set reset fail\n");
#endif

    s_ctx.sbus.mask =
        SBUS_MASK_SAMPLE_8BITS | SBUS_MASK_ADDR_16BITS |
        SBUS_MASK_ADDR_SWAP_BYTES;
    s_ctx.sbus.control = 0;
    s_ctx.sbus.bus = 1;
    s_ctx.sbus.device = SENSOR_DEV_ADDRESS;
    acamera_sbus_init( &s_ctx.sbus, sbus_i2c );

    sensor_get_id(&s_ctx);

    // Initial local parameters
    s_ctx.address = SENSOR_DEV_ADDRESS;
    s_ctx.seq_width = 1;
    s_ctx.streaming_flg = 0;
    s_ctx.again[0] = 0;
    s_ctx.again[1] = 0;
    s_ctx.again[2] = 0;
    s_ctx.again[3] = 0;
    s_ctx.dgain = 0;
    s_ctx.dgain_change = 0;
    s_ctx.again_limit = AGAIN_MAX_DB;
    s_ctx.dgain_limit = DGAIN_MAX_DB;
    s_ctx.pixel_clock = 148500000;

    s_ctx.param.again_accuracy = 1 << LOG2_GAIN_SHIFT;
    s_ctx.param.sensor_exp_number = 1;
    s_ctx.param.again_log2_max = AGAIN_MAX_DB << LOG2_GAIN_SHIFT;
    s_ctx.param.dgain_log2_max = DGAIN_MAX_DB << LOG2_GAIN_SHIFT;
    s_ctx.param.integration_time_apply_delay = 2;
    s_ctx.param.isp_exposure_channel_delay = 0;
    s_ctx.param.modes_table = supported_modes;
    s_ctx.param.modes_num = array_size_s( supported_modes );
    s_ctx.param.sensor_ctx = &s_ctx;
    s_ctx.param.isp_context_seq.sequence = p_isp_data;
    s_ctx.param.isp_context_seq.seq_num= SENSOR_IMX307_ISP_CONTEXT_SEQ;
    s_ctx.param.isp_context_seq.seq_table_max = array_size_s( isp_seq_table );

    ctrl->alloc_analog_gain = sensor_alloc_analog_gain;
    ctrl->alloc_digital_gain = sensor_alloc_digital_gain;
    ctrl->alloc_integration_time = sensor_alloc_integration_time;
    ctrl->ir_cut_set= sensor_ir_cut_set;
    ctrl->sensor_update = sensor_update;
    ctrl->set_mode = sensor_set_mode;
    ctrl->get_id = sensor_get_id;
    ctrl->get_parameters = sensor_get_parameters;
    ctrl->disable_sensor_isp = sensor_disable_isp;
    ctrl->read_sensor_register = read_register;
    ctrl->write_sensor_register = write_register;
    ctrl->start_streaming = start_streaming;
    ctrl->stop_streaming = stop_streaming;
    ctrl->sensor_test_pattern = sensor_test_pattern;

    // Reset sensor during initialization
    sensor_hw_reset_enable();
    system_timer_usleep( 1000 ); // reset at least 1 ms
    sensor_hw_reset_disable();
    system_timer_usleep( 1000 );
}
//********************CONSTANT SECTION END**************************************
//******************************************************************************
