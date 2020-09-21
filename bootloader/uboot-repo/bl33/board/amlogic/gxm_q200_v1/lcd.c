/*
 * AMLOGIC LCD panel driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <amlogic/aml_lcd.h>
#include <asm/arch/gpio.h>

//Rsv_val = 0xffffffff

static char lcd_cpu_gpio[LCD_CPU_GPIO_NUM_MAX][LCD_CPU_GPIO_NAME_MAX] = {
	"invalid", /* ending flag */
};

static struct lcd_power_step_s lcd_power_on_step[] = {
	{LCD_POWER_TYPE_SIGNAL,0,0,0,},  /* signal */
	{LCD_POWER_TYPE_MAX,   0,0,0,},  /* ending flag */
};
static struct lcd_power_step_s lcd_power_off_step[] = {
	{LCD_POWER_TYPE_SIGNAL,0,0,50,},  /* signal */
	{LCD_POWER_TYPE_MAX,   0,0,0,},   /* ending flag */
};

static char lcd_bl_gpio[BL_GPIO_NUM_MAX][LCD_CPU_GPIO_NAME_MAX] = {
	"invalid", /* ending flag */
};

struct ext_lcd_config_s ext_lcd_config[LCD_NUM_MAX] = {
	{
	"lcd_0",LCD_TTL,6,
	/* basic timing */
	1280,720,1650,750,40,220,1,5,20,1,
	/* clk_attr */
	0,0,1,74250000,Rsv_val,Rsv_val,Rsv_val,Rsv_val,Rsv_val,Rsv_val,
	/* ttl_attr */
	0,1,1,0,0,Rsv_val,Rsv_val,Rsv_val,Rsv_val,Rsv_val,
	/* power step */
	lcd_power_on_step, lcd_power_off_step,
	/* backlight */
	60,255,10,128,128,
	0xff,0,1,0,200,200,
	BL_PWM_POSITIVE,BL_PWM_B,180,100,25,1,0,
	Rsv_val,Rsv_val,Rsv_val,Rsv_val,Rsv_val,Rsv_val,Rsv_val,
	Rsv_val,Rsv_val,Rsv_val,Rsv_val,
	10,10,Rsv_val},
};

//**** Special parameters just for ttl ***//
static struct ttl_config_s lcd_ttl_config = {
	.clk_pol    = 0,
	.sync_valid = (1 << 1) | (1 << 0), /* [1]DE, [0]hvsync */
	.swap_ctrl  = (0 << 1) | (0 << 0), /* [1]rb swap, [0]bit swap */
};

//**** Special parameters just for lvds ***//
static struct lvds_config_s lcd_lvds_config = {
	.lvds_repack  = 1, //0=JEDIA mode,  1=VESA mode
	.dual_port    = 1, //0=single port, 1=double port
	.pn_swap      = 0, //0=normal,      1=swap
	.port_swap    = 0, //0=normal,      1=swap
	.lane_reverse = 0, //0=normal,      1=swap
};

static struct lcd_power_ctrl_s lcd_power_ctrl = {
	.power_on_step = {
		{
			.type = LCD_POWER_TYPE_SIGNAL,
			.delay = 0, /* unit: ms */
		},
		{
			.type = LCD_POWER_TYPE_MAX, /* ending flag */
		},
	},
	.power_off_step = {
		{
			.type = LCD_POWER_TYPE_SIGNAL,
			.delay = 50, /* unit: ms */
		},
		{
			.type = LCD_POWER_TYPE_MAX, /* ending flag */
		},
	},
};

struct lcd_config_s lcd_config_dft = {
	.lcd_mode = LCD_MODE_TABLET,
	.lcd_key_valid = 0,
	.lcd_basic = {
		.model_name = "default",
		.lcd_type = LCD_TYPE_MAX, //LCD_TTL /LCD_LVDS/LCD_VBYONE
		.lcd_bits = 8,
		.h_active = 1920,
		.v_active = 1080,
		.h_period = 2200,
		.v_period = 1125,

		.screen_width   = 16,
		.screen_height  = 9,
	},

	.lcd_timing = {
		.clk_auto = 1,
		.lcd_clk = 60,
		.ss_level = 0,
		.fr_adjust_type = 0,

		.hsync_width = 44,
		.hsync_bp    = 148,
		.hsync_pol   = 0,
		.vsync_width = 5,
		.vsync_bp    = 36,
		.vsync_pol   = 0,
	},

	.lcd_control = {
		.ttl_config    = &lcd_ttl_config,
		.lvds_config   = &lcd_lvds_config,
	},
	.lcd_power = &lcd_power_ctrl,

	.pinctrl_ver = 0,
	.pinmux_set = {{3, 0x000002a0}, {LCD_PINMUX_END, 0x0}},
	.pinmux_clr = {{1, 0xfa030000}, {2, 0xf9fe0409}, {3, 0x00000001}, {LCD_PINMUX_END, 0x0}},
};

#ifdef CONFIG_AML_LCD_EXTERN
static char lcd_ext_gpio[LCD_EXTERN_GPIO_NUM_MAX][LCD_EXTERN_GPIO_LEN_MAX] = {
	"invalid", /* ending flag */
};

static unsigned char init_on_table[LCD_EXTERN_INIT_ON_MAX] = {
	0xff, 0,  /* ending */
};

static unsigned char init_off_table[LCD_EXTERN_INIT_OFF_MAX] = {
	0xff, 0, /* ending */
};

struct lcd_extern_common_s ext_common_dft = {
	.lcd_ext_key_valid = 0,
	.lcd_ext_num = 1,
	.i2c_bus = LCD_EXTERN_I2C_BUS_D, /* LCD_EXTERN_I2C_BUS_A/B/C/D/AO */
	.pinmux_set = {{LCD_PINMUX_END, 0x0}},
	.pinmux_clr = {{LCD_PINMUX_END, 0x0}},
};

struct lcd_extern_config_s ext_config_dtf[LCD_EXTERN_NUM_MAX] = {
	{
		.index = 0,
		.name = "ext_default",
		.type = LCD_EXTERN_I2C, /* LCD_EXTERN_I2C, LCD_EXTERN_SPI, LCD_EXTERN_MAX */
		.status = 1, /* 0=disable, 1=enable */
		.i2c_addr = 0x1c, /* 7bit i2c address */
		.i2c_addr2 = 0xff, /* 7bit i2c address, 0xff for none */
		.spi_gpio_cs = 0,
		.spi_gpio_clk = 1,
		.spi_gpio_data = 2,
		.spi_clk_freq = 0, /* hz */
		.spi_clk_pol = 0,
		.cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC,
		.table_init_on = init_on_table,
		.table_init_on_cnt = sizeof(init_on_table),
		.table_init_off = init_off_table,
		.table_init_off_cnt = sizeof(init_off_table),
	},
	{
		.index = LCD_EXTERN_INDEX_INVALID,
	},
};
#endif

struct bl_config_s bl_config_dft = {
	.name = "default",
	.bl_key_valid = 0,

	.level_default = 100,
	.level_min = 10,
	.level_max = 255,
	.level_mid = 128,
	.level_mid_mapping = 128,
	.level = 0,

	.method = BL_CTRL_MAX,
	.power_on_delay = 200,
	.power_off_delay = 200,

	.en_gpio = 0xff,
	.en_gpio_on = 1,
	.en_gpio_off = 0,

	.bl_pwm = NULL,
	.bl_pwm_combo0 = NULL,
	.bl_pwm_combo1 = NULL,
	.pwm_on_delay = 10,
	.pwm_off_delay = 10,

	.pinctrl_ver = 0,
	.pinmux_set = {{2, 0x00000800}, {LCD_PINMUX_END, 0x0}},
	.pinmux_clr = {{1, 0x00000400}, {2, 0x00000020}, {LCD_PINMUX_END, 0x0}},
};

void lcd_config_bsp_init(void)
{
	int i, j;

	for (i = 0; i < LCD_CPU_GPIO_NUM_MAX; i++) {
		if (strcmp(lcd_cpu_gpio[i], "invalid") == 0)
			break;
		strcpy(lcd_power_ctrl.cpu_gpio[i], lcd_cpu_gpio[i]);
	}
	for (j = i; j < LCD_CPU_GPIO_NUM_MAX; j++)
		strcpy(lcd_power_ctrl.cpu_gpio[j], "invalid");
	for (i = 0; i < BL_GPIO_NUM_MAX; i++) {
		if (strcmp(lcd_bl_gpio[i], "invalid") == 0)
			break;
		strcpy(bl_config_dft.gpio_name[i], lcd_bl_gpio[i]);
	}
	for (j = i; j < BL_GPIO_NUM_MAX; j++)
		strcpy(bl_config_dft.gpio_name[j], "invalid");

#ifdef CONFIG_AML_LCD_EXTERN
	for (i = 0; i < LCD_EXTERN_NUM_MAX; i++) {
		if (ext_config_dtf[i].index == LCD_EXTERN_INDEX_INVALID)
			break;
	}
	ext_common_dft.lcd_ext_num = i;

	for (i = 0; i < LCD_EXTERN_GPIO_NUM_MAX; i++) {
		if (strcmp(lcd_ext_gpio[i], "invalid") == 0)
			break;
		strcpy(ext_common_dft.gpio_name[i], lcd_ext_gpio[i]);
	}
	for (j = i; j < LCD_EXTERN_GPIO_NUM_MAX; j++)
		strcpy(ext_common_dft.gpio_name[j], "invalid");

#endif
}
