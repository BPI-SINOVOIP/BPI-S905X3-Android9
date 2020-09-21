
/*
 * arch/arm/cpu/armv8/txl/gpio.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <common.h>
#include <dm.h>
#include <linux/compiler.h>
#include <aml_gpio.h>
#include <asm/arch/gpio.h>

#define NE 0xffffffff
#define PK(reg, bit) ((reg<<5)|bit)
/*AO REG */
#define AO 0x10
#define AO2 0x11

static unsigned int gpio_to_pin[][5] = {
		[PIN_BOOT_0] = {PK(7, 31), NE, NE, NE, NE,},
		[PIN_BOOT_1] = {PK(7, 31), NE, NE, NE, NE,},
		[PIN_BOOT_2] = {PK(7, 31), NE, NE, NE, NE,},
		[PIN_BOOT_3] = {PK(7, 31), NE, NE, PK(7, 23), NE,},
		[PIN_BOOT_4] = {PK(7, 31), PK(7, 13), NE, PK(7, 22), NE,},

		[PIN_BOOT_5] = {PK(7, 31), PK(7, 12), NE, PK(7, 21), NE,},

		[PIN_BOOT_6] = {PK(7, 31), PK(7, 11), NE, PK(7, 20), NE,},

		[PIN_BOOT_7] = {PK(7, 31), NE, NE, PK(7, 19), NE,},

		[PIN_BOOT_8] = {PK(7, 30), NE, NE, NE, NE,},
		[PIN_BOOT_9] = {NE, NE, NE, NE, NE,},
		[PIN_BOOT_10] = {PK(7, 29), NE, NE, NE, NE,},
		[PIN_BOOT_11] = {PK(7, 28), PK(7, 10), NE, NE, NE,},
//
		[PIN_GPIOH_0] = {PK(0, 31), NE, NE, NE, NE,},
		[PIN_GPIOH_1] = {PK(0, 30), NE, NE, NE, NE,},
		[PIN_GPIOH_2] = {PK(0, 29), NE, NE, NE, NE,},
		[PIN_GPIOH_3] = {PK(0, 28), NE, NE, NE, NE,},
		[PIN_GPIOH_4] = {PK(0, 27), PK(0, 26), NE, NE, NE,},
		[PIN_GPIOH_5] = {NE, NE, PK(0, 18), NE, NE,},
		[PIN_GPIOH_6] = {NE, NE, PK(0, 17), NE, NE,},
		[PIN_GPIOH_7] = {NE, NE, PK(0, 16), NE, NE,},
		[PIN_GPIOH_8] = {NE, PK(0, 13), PK(0, 15), NE, NE,},
		[PIN_GPIOH_9] = {PK(0, 12),NE,  PK(0, 14), NE, NE,},
//
		[PIN_GPIOZ_0] = {PK(4, 31), PK(4, 25), NE, PK(3, 21), PK(3, 16),},

		[PIN_GPIOZ_1] = {PK(4, 30), PK(4, 24), NE, PK(3, 21), PK(3, 15),},
		[PIN_GPIOZ_2] = {PK(4, 29), PK(4, 23), PK(4, 21), PK(3, 21), PK(3, 14),},
		[PIN_GPIOZ_3] = {PK(4, 28), PK(4, 22), PK(4, 20), PK(3, 21), PK(3, 13),},

		[PIN_GPIOZ_4] = {PK(4, 27), PK(4, 19), PK(4, 18), PK(3, 21), NE,},
		[PIN_GPIOZ_5] = {PK(4, 26), PK(4, 17), NE, PK(3, 21), NE,},

		[PIN_GPIOZ_6] = {PK(4, 16), PK(4, 15), NE, PK(3, 21), NE,},
		[PIN_GPIOZ_7] = {PK(4, 14), PK(4, 13), NE, PK(3, 21), NE,},
		[PIN_GPIOZ_8] = {PK(4, 12), NE, NE, PK(3, 21), NE,},
		[PIN_GPIOZ_9] = {PK(4, 11), NE, NE, PK(3, 21), NE,},
		[PIN_GPIOZ_10] = {PK(4, 10), NE, PK(3, 20), PK(3, 21), NE,},
		[PIN_GPIOZ_11] = {PK(4, 9), PK(4, 3), PK(3, 19), PK(3, 21), NE,},
		[PIN_GPIOZ_12] = {PK(4, 8), PK(4, 2), PK(3, 18), PK(3, 21), NE,},
		[PIN_GPIOZ_13] = {NE, PK(4, 1), PK(3, 17), PK(3, 21), NE,},
		[PIN_GPIOZ_14] = {PK(4, 7), NE, NE, PK(3, 21), NE,},
		[PIN_GPIOZ_15] = {PK(4, 6), NE, PK(3, 25), PK(3, 21), NE,},
		[PIN_GPIOZ_16] = {PK(4, 5), PK(3, 31), PK(3, 24), PK(3, 21), NE,},
		[PIN_GPIOZ_17] = {PK(3, 30), NE, NE, PK(3, 21), NE,},
		[PIN_GPIOZ_18] = {PK(3, 29), PK(4, 4), PK(3, 23), NE, NE,},
		[PIN_GPIOZ_19] = {PK(3, 28), PK(3, 27), PK(3, 22), PK(3, 26), NE,},
		[PIN_GPIOZ_20] = {NE, NE, NE, NE, NE,},
		[PIN_GPIOZ_21] = {NE, NE, NE, NE, NE,},
//
		[PIN_GPIODV_0] = {PK(2, 25), NE, NE, PK(2, 31), NE,},
		[PIN_GPIODV_1] = {PK(2, 24), NE, NE, PK(2, 31), NE,},
		[PIN_GPIODV_2] = {PK(2, 22), PK(2, 23), PK(2, 20), PK(2, 31), PK(2, 19),},
		[PIN_GPIODV_3] = {PK(2, 18), PK(2, 21), NE, PK(2, 31), PK(2, 17),},
		[PIN_GPIODV_4] = {PK(2, 16), NE, NE, PK(2, 31), NE,},
		[PIN_GPIODV_5] = {PK(2, 15), NE, NE, PK(2, 31), NE,},
		[PIN_GPIODV_6] = {NE, PK(2, 8),NE,  PK(2, 31), NE,},
		[PIN_GPIODV_7] = {NE, PK(2, 7), PK(2, 4), PK(2, 30), NE,},
		[PIN_GPIODV_8] = {PK(2, 14), PK(2, 6), PK(2, 3), PK(2, 29), NE,},
		[PIN_GPIODV_9] = {PK(2, 13), PK(2, 5), NE, PK(2, 28), NE,},
		[PIN_GPIODV_10] = {PK(2, 12), PK(2, 10), PK(2, 2), PK(2, 27), NE,},
		[PIN_GPIODV_11] = {PK(2, 11), PK(2, 9), PK(2, 1), PK(2, 26), PK(2, 0),},
//
		[GPIOAO_0] = {PK(AO, 12), PK(AO, 26), NE, NE, NE,},
		[GPIOAO_1] = {PK(AO, 11), PK(AO, 25), NE, NE, NE,},
		[GPIOAO_2] = {PK(AO, 10), PK(AO, 8), PK(AO, 28), NE, NE,},
		[GPIOAO_3] = {PK(AO, 9), PK(AO, 7), NE, PK(AO, 22), NE,},
		[GPIOAO_4] = {PK(AO, 24), PK(AO, 6), PK(AO, 2), NE, NE,},
		[GPIOAO_5] = {PK(AO, 23), PK(AO, 5), PK(AO, 1), NE, NE,},
		[GPIOAO_6] = {PK(AO, 0), PK(AO, 21), NE, NE, NE,},
		[GPIOAO_7] = {PK(AO, 15), PK(AO, 14), PK(AO, 17), NE, NE,},
		[GPIOAO_8] = {NE, PK(AO, 27), NE, NE, NE,},
		[GPIOAO_9] = {NE, PK(AO, 3), NE, NE, NE,},
		[GPIOAO_10] = {NE, NE, NE, NE, NE,},
		[GPIOAO_11] = {NE, NE, NE, NE, NE,},

		[PIN_GPIOW_0] = {PK(5, 31), NE, NE, NE, NE,},
		[PIN_GPIOW_1] = {PK(5, 30), NE, NE, NE, NE,},
		[PIN_GPIOW_2] = {PK(5, 29), PK(5, 15), NE, NE, NE,},
		[PIN_GPIOW_3] = {PK(5, 28), PK(5, 14), NE, NE, NE,},
		[PIN_GPIOW_4] = {PK(5, 27), NE, NE, NE, NE,},
		[PIN_GPIOW_5] = {PK(5, 26), NE, NE, NE, NE,},
		[PIN_GPIOW_6] = {PK(5, 25), PK(5, 13), NE, NE, NE,},
		[PIN_GPIOW_7] = {PK(5, 24), PK(5, 12), NE, NE, NE,},
		[PIN_GPIOW_8] = {PK(5, 23), NE, NE, NE, NE,},
		[PIN_GPIOW_9] = {PK(5, 22), NE, NE, NE, NE,},
		[PIN_GPIOW_10] = {PK(5, 21), PK(5, 11), NE, NE, NE,},
		[PIN_GPIOW_11] = {PK(5, 20), PK(5, 10), NE, NE, NE,},
		[PIN_GPIOW_12] = {PK(5, 19), NE, NE, NE, NE,},
		[PIN_GPIOW_13] = {PK(5, 18), NE, NE, NE, NE,},
		[PIN_GPIOW_14] = {PK(5, 17), PK(5, 9), NE, NE, NE,},
		[PIN_GPIOW_15] = {PK(5, 16), PK(5, 8), NE, NE, NE,},

		[PIN_CARD_0] = {PK(6, 5), NE, NE, PK(6, 15), PK(6, 31),},
		[PIN_CARD_1] = {PK(6, 4), NE, NE, PK(6, 14), PK(6, 30),},
		[PIN_CARD_2] = {PK(6, 3), NE, NE, PK(6, 13), PK(6, 29),},
		[PIN_CARD_3] = {PK(6, 2), NE, NE, PK(6, 12), PK(6, 28),},
		[PIN_CARD_4] = {PK(6, 0), PK(6, 9), PK(6, 11), NE, PK(6, 27),},
		[PIN_CARD_5] = {PK(6, 1), PK(6, 8), PK(6, 10), NE, PK(6, 26),},
		[PIN_CARD_6] = {NE, NE, NE, NE, NE,},
};

#define BANK(n, f, l, per, peb, pr, pb, dr, db, or, ob, ir, ib)		\
	{								\
		.name	= n,						\
		.first	= f,						\
		.last	= l,						\
		.regs	= {						\
			[REG_PULLEN]	= { (0xc8834120 + (per<<2)), peb },			\
			[REG_PULL]	= { (0xc88344e8 + (pr<<2)), pb },			\
			[REG_DIR]	= { (0xc8834430 + (dr<<2)), db },			\
			[REG_OUT]	= { (0xc8834430 + (or<<2)), ob },			\
			[REG_IN]	= { (0xc8834430 + (ir<<2)), ib },			\
		},							\
	 }
#define AOBANK(n, f, l, per, peb, pr, pb, dr, db, or, ob, ir, ib)		\
	{								\
		.name	= n,						\
		.first	= f,						\
		.last	= l,						\
		.regs	= {						\
			[REG_PULLEN]	= { (0xc810002c + (per<<2)), peb },			\
			[REG_PULL]	= { (0xc810002c + (pr<<2)), pb },			\
			[REG_DIR]	= { (0xc8100024 + (dr<<2)), db },			\
			[REG_OUT]	= { (0xc8100024 + (or<<2)), ob },			\
			[REG_IN]	= { (0xc8100024 + (ir<<2)), ib },			\
		},							\
	 }

static struct meson_bank mesongxbb_banks[] = {
	/*   name    first         last
	 *   pullen  pull     dir     out     in  */
		BANK("GPIOW_",    PIN_GPIOW_0,  PIN_GPIOW_15,
		     4,  0,  4,  0,  12,  0,  13,  0,  14,  0),
		BANK("GPIODV_",  PIN_GPIODV_0, PIN_GPIODV_11,
		     0,  0,  0,  0,  0,  0,  1,  0,  2,  0),
		BANK("GPIOH_",    PIN_GPIOH_0,  PIN_GPIOH_9,
		     1, 20,  1, 20,  3, 20, 4, 20, 5, 20),
		BANK("GPIOZ_",    PIN_GPIOZ_0,  PIN_GPIOZ_21,
		     3,  0,  3,  0,  9, 0,  10, 0,  11, 0),
		BANK("CARD_", PIN_CARD_0,   PIN_CARD_6,
		     2, 20,  2, 20,  6, 20,  7, 20,  8, 20),
		BANK("BOOT_", PIN_BOOT_0,   PIN_BOOT_11,
		     2,  0,  2,  0,  6,  0, 7,  0, 8,  0),
		BANK("GPIOCLK_", PIN_GPIOCLK_0,   PIN_GPIOCLK_1,
		     3,  28,  3,  28,  9,  28, 10,  28, 11,  28),
		AOBANK("GPIOAO_",   GPIOAO_0, GPIOAO_11,
		     0,  0,  0, 16,  0,  0,  0, 16,  1,  0),
};

U_BOOT_DEVICES(gxbb_gpios) = {
	{ "gpio_aml", &mesongxbb_banks[0] },
	{ "gpio_aml", &mesongxbb_banks[1] },
	{ "gpio_aml", &mesongxbb_banks[2] },
	{ "gpio_aml", &mesongxbb_banks[3] },
	{ "gpio_aml", &mesongxbb_banks[4] },
	{ "gpio_aml", &mesongxbb_banks[5] },
	{ "gpio_aml", &mesongxbb_banks[6] },
	{ "gpio_aml", &mesongxbb_banks[7] },
};
static unsigned long domain[]={
	[0] = 0xc88344b0,
	[1] = 0xc8100014,
};
int  clear_pinmux(unsigned int pin)
{
	unsigned int *gpio_reg =  &gpio_to_pin[pin][0];
	int i, dom, reg, bit;
	for (i = 0;
	     i < sizeof(gpio_to_pin[pin])/sizeof(gpio_to_pin[pin][0]); i++) {
		if (gpio_reg[i] != NE) {
			reg = GPIO_REG(gpio_reg[i])&0xf;
			bit = GPIO_BIT(gpio_reg[i]);
			dom = GPIO_REG(gpio_reg[i])>>4;
			regmap_update_bits(domain[dom]+reg*4,BIT(bit),0);
		}
	}
	return 0;

}

#ifdef CONFIG_AML_SPICC
#include <asm/arch/secure_apb.h>
/* generic pins control for txlx spicc.
 * if deleted, you have to add it into all txl board files as necessary.
 * 		 SPI mux	other mux
 * GPIOZ_0(MOSI) reg4[31]	reg4[25]     reg3[21][16]
 * GPIOZ_1(MISO) reg4[30]	reg4[24]     reg3[21][15]
 * GPIOZ_2(CLK ) reg4[29]	reg4[23][21] reg3[21][14]
 */
int spicc_pinctrl_enable(void *pinctrl, bool enable)
{
	u32 regv;

	regv = readl(P_PERIPHS_PIN_MUX_3);
	regv &= ~((1 << 21) | (0x7 << 14));
	writel(regv, P_PERIPHS_PIN_MUX_3);

	regv = readl(P_PERIPHS_PIN_MUX_4);
	regv &= ~((0x7 << 29) | (0x7 << 23) | (1 << 21));
	if (enable)
		regv |= 0x7 << 29;
	writel(regv, P_PERIPHS_PIN_MUX_4);

	return 0;
}
#endif /* CONFIG_AML_SPICC */
