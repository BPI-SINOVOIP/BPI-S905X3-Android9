/*
 * include/amlogic/aml_lcd_extern.h
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

#ifndef _INC_AML_LCD_EXTERN_H_
#define _INC_AML_LCD_EXTERN_H_

#include <amlogic/aml_lcd_vout.h>

enum lcd_extern_type_e {
	LCD_EXTERN_I2C = 0,
	LCD_EXTERN_SPI,
	LCD_EXTERN_MIPI,
	LCD_EXTERN_MAX,
};

#define LCD_EXTERN_I2C_BUS_A          0
#define LCD_EXTERN_I2C_BUS_B          1
#define LCD_EXTERN_I2C_BUS_C          2
#define LCD_EXTERN_I2C_BUS_D          3
#define LCD_EXTERN_I2C_BUS_AO         4

#define LCD_EXTERN_I2C_BUS_0          LCD_EXTERN_I2C_BUS_A
#define LCD_EXTERN_I2C_BUS_1          LCD_EXTERN_I2C_BUS_B
#define LCD_EXTERN_I2C_BUS_2          LCD_EXTERN_I2C_BUS_C
#define LCD_EXTERN_I2C_BUS_3          LCD_EXTERN_I2C_BUS_D
#define LCD_EXTERN_I2C_BUS_4          LCD_EXTERN_I2C_BUS_AO
#define LCD_EXTERN_I2C_BUS_MAX        5


#define LCD_EXTERN_INIT_ON_MAX        3000
#define LCD_EXTERN_INIT_OFF_MAX       100

#define LCD_EXTERN_GPIO_NUM_MAX       6
#define LCD_EXTERN_GPIO_LEN_MAX       10
#define LCD_EXTERN_INDEX_INVALID      0xff
#define LCD_EXTERN_NAME_LEN_MAX       30

#define LCD_EXTERN_NUM_MAX            20

struct lcd_extern_config_s {
	unsigned char index;
	char name[LCD_EXTERN_NAME_LEN_MAX];
	enum lcd_extern_type_e type;
	unsigned char status;
	unsigned char pinmux_valid;
	unsigned char pinmux_gpio_off;

	unsigned char i2c_addr;
	unsigned char i2c_addr2;
	unsigned char i2c_bus;
	unsigned char i2c_sck_gpio;
	unsigned char i2c_sda_gpio;

	unsigned char spi_gpio_cs;
	unsigned char spi_gpio_clk;
	unsigned char spi_gpio_data;
	unsigned char spi_clk_pol;
	unsigned short spi_clk_freq; /*KHz */
	unsigned short spi_delay_us;

	unsigned char cmd_size;
	unsigned char table_init_loaded; /* internal use */
	unsigned int table_init_on_cnt;
	unsigned int table_init_off_cnt;
	unsigned char *table_init_on;
	unsigned char *table_init_off;
};

struct lcd_extern_common_s {
	unsigned char lcd_ext_key_valid;
	char gpio_name[LCD_EXTERN_GPIO_NUM_MAX][LCD_EXTERN_GPIO_LEN_MAX];
	unsigned int lcd_ext_num;

	unsigned char i2c_bus;
	unsigned char i2c_sck_gpio;
	unsigned char i2c_sck_gpio_off;
	unsigned char i2c_sda_gpio;
	unsigned char i2c_sda_gpio_off;
	unsigned int pinmux_set[LCD_PINMUX_NUM][2];
	unsigned int pinmux_clr[LCD_PINMUX_NUM][2];
};

//global API
struct aml_lcd_extern_driver_s {
	struct lcd_extern_config_s *config;
	struct lcd_extern_common_s *common;
	int (*reg_read)  (unsigned char reg, unsigned char *buf);
	int (*reg_write) (unsigned char *buf, unsigned int len);
	int (*power_on)(void);
	int (*power_off)(void);
	void (*info_print)(void);
};

extern struct aml_lcd_extern_driver_s *aml_lcd_extern_get_driver(void);
extern int aml_lcd_extern_probe(char *dtaddr, int index);
extern int aml_lcd_extern_remove(void);

extern struct lcd_extern_config_s ext_config_dtf[LCD_EXTERN_NUM_MAX];
extern struct lcd_extern_common_s ext_common_dft;

#endif

