/*
 * sound/soc/codecs/amlogic/aml_codec_t9015S.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef AML_T9015S_H_
#define AML_T9015S_H_

#define ACODEC_BASE_ADD    0xc8832000
#define ACODEC_TOP_ADDR(x) (x)

#define AUDIO_CONFIG_BLOCK_ENABLE       ACODEC_TOP_ADDR(0x00)
#define MCLK_FREQ                   0x1F
#define I2S_MODE                    0x1E
#define ADC_HPF_EN                  0x1D
#define ADC_HPF_MODE                0x1C
#define ADC_OVERLOAD_DET_EN         0x1B
#define ADC_DEM_EN                  0x1A
#define ADC_CLK_TO_GPIO_EN          0x19
#define DAC_CLK_TO_GPIO_EN          0x18
#define DACL_DATA_SOURCE            0x17
#define DACR_DATA_SOURCE            0x16
#define DACL_INV                    0x15
#define DACR_INV                    0x14
#define ADCDATL_SOURCE              0x13
#define ADCDATR_SOURCE              0x12
#define ADCL_INV                    0x11
#define ADCR_INV                    0x10
#define VMID_GEN_EN                 0x0F
#define VMID_GEN_FAST               0x0E
#define BIAS_CURRENT_EN             0x0D
#define REFP_BUF_EN                 0x0C
#define PGAL_IN_EN                  0x0B
#define PGAR_IN_EN                  0x0A
#define PGAL_IN_ZC_EN               0x09
#define PGAR_IN_ZC_EN               0x08
#define ADCL_EN                     0x07
#define ADCR_EN                     0x06
#define DACL_EN                     0x05
#define DACR_EN                     0x04
#define LOLP_EN                     0x03
#define LOLN_EN                     0x02
#define LORP_EN                     0x01
#define LORN_EN                     0x00

#define ADC_VOL_CTR_PGA_IN_CONFIG       ACODEC_TOP_ADDR(0x04)
#define DAC_GAIN_SEL_H              0x1F
#define ADCL_VC                     0x18
#define DAC_GAIN_SEL_L              0x17
#define ADCR_VC                     0x10
#define PGAL_IN_SEL                 0x0D
#define PGAL_IN_GAIN                0x08
#define PGAR_IN_SEL                 0x05
#define PGAR_IN_GAIN                0x00

#define DAC_VOL_CTR_DAC_SOFT_MUTE       ACODEC_TOP_ADDR(0x08)
#define DACL_VC                     0x18
#define DACR_VC                     0x10
#define DAC_SOFT_MUTE               0x0F
#define DAC_UNMUTE_MODE             0x0E
#define DAC_MUTE_MODE               0x0D
#define DAC_VC_RAMP_MODE            0x0C
#define DAC_RAMP_RATE               0x0A
#define DAC_MONO                    0x08

#define LINE_OUT_CONFIG                 ACODEC_TOP_ADDR(0x0c)
#define LOLP_SEL_DACL               0x0E
#define LOLP_SEL_AIL                0x0D
#define LOLP_SEL_AIL_INV            0x0C
#define LOLN_SEL_DACL_INV           0x0A
#define LOLN_SEL_DACL               0x09
#define LOLN_SEL_AIL                0x08
#define LORP_SEL_DACR               0x06
#define LORP_SEL_AIR                0x05
#define LORP_SEL_AIR_INV            0x04
#define LORN_SEL_DACR_INV           0x02
#define LORN_SEL_DACR               0x01
#define LORN_SEL_AIR                0x00

#define POWER_CONFIG                    ACODEC_TOP_ADDR(0x10)
#define MUTE_DAC_PD_EN              0x1F
#define IB_CON                      0x10

#endif
