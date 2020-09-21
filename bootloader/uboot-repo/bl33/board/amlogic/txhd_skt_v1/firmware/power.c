
/*
 * board/amlogic/txl_skt_v1/firmware/power.c
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

#include "config.h"
#include <serial.h>
//#include <stdio.h>
#include <asm/arch/secure_apb.h>

#define P_EE_TIMER_E		(P_ISA_TIMERE)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int pwm_voltage_table[][2] = {
	{ 0x1c0000,  860},
	{ 0x1b0001,  870},
	{ 0x1a0002,  880},
	{ 0x190003,  890},
	{ 0x180004,  900},
	{ 0x170005,  910},
	{ 0x160006,  920},
	{ 0x150007,  930},
	{ 0x140008,  940},
	{ 0x130009,  950},
	{ 0x12000a,  960},
	{ 0x11000b,  970},
	{ 0x10000c,  980},
	{ 0x0f000d,  990},
	{ 0x0e000e, 1000},
	{ 0x0d000f, 1010},
	{ 0x0c0010, 1020},
	{ 0x0b0011, 1030},
	{ 0x0a0012, 1040},
	{ 0x090013, 1050},
	{ 0x080014, 1060},
	{ 0x070015, 1070},
	{ 0x060016, 1080},
	{ 0x050017, 1090},
	{ 0x040018, 1100},
	{ 0x030019, 1110},
	{ 0x02001a, 1120},
	{ 0x01001b, 1130},
	{ 0x00001c, 1140}
};
static int pwm_voltage_table_ee[][2] = {
	{ 0x1c0000,  810},
	{ 0x1b0001,  820},
	{ 0x1a0002,  830},
	{ 0x190003,  840},
	{ 0x180004,  850},
	{ 0x170005,  860},
	{ 0x160006,  870},
	{ 0x150007,  880},
	{ 0x140008,  890},
	{ 0x130009,  900},
	{ 0x12000a,  910},
	{ 0x11000b,  920},
	{ 0x10000c,  930},
	{ 0x0f000d,  940},
	{ 0x0e000e,  950},
	{ 0x0d000f,  960},
	{ 0x0c0010,  970},
	{ 0x0b0011,  980},
	{ 0x0a0012,  990},
	{ 0x090013, 1000},
	{ 0x080014, 1010},
	{ 0x070015, 1020},
	{ 0x060016, 1030},
	{ 0x050017, 1040},
	{ 0x040018, 1050},
	{ 0x030019, 1060},
	{ 0x02001a, 1070},
	{ 0x01001b, 1080},
	{ 0x00001c, 1090}
};

enum pwm_id {
    pwm_a = 0,
    pwm_ao_b,
};

unsigned int _get_time(void)
{
    return readl(P_EE_TIMER_E);
}

void _udelay_(unsigned int us)
{
	unsigned int t0 = _get_time();

	while (_get_time() - t0 <= us)
		;
}

void pwm_init(int id)
{
	unsigned int reg;

	/*
	 * TODO: support more pwm controllers, right now only support
	 */

	switch (id) {
	case pwm_a:
		reg = readl(P_PWM_MISC_REG_AB);
		reg &= ~(0x7f << 8);
		reg |=  ((1 << 15) | (1 << 0));
		writel(reg, P_PWM_MISC_REG_AB);

		/* set PWM_A pinmux */
		reg  = readl(P_PERIPHS_PIN_MUX_2);
		reg |= ((1 & 0xf) << 20);
		writel(reg, P_PERIPHS_PIN_MUX_2);
		break;

	case pwm_ao_b:
		reg  = readl(P_AO_PWM_MISC_REG_AB);
		reg &= ~(0x7f << 16);
		reg |=  ((1 << 23) | (1 << 1));
		writel(reg, P_AO_PWM_MISC_REG_AB);

		/* set PWM_AO_B pinmux */
		reg  = readl(P_AO_RTI_PIN_MUX_REG2);;
		reg |= ((4 & 0xf) << 4);
		writel(reg, P_AO_RTI_PIN_MUX_REG2);
		break;

	default:
		break;
	}

	_udelay_(200);
}
/*
void pwm_set_voltage(unsigned int id, unsigned int voltage)
{
	int to;

	for (to = 0; to < ARRAY_SIZE(pwm_voltage_table); to++) {
		if (pwm_voltage_table[to][1] >= voltage) {
			break;
		}
	}
	if (to >= ARRAY_SIZE(pwm_voltage_table)) {
		to = ARRAY_SIZE(pwm_voltage_table) - 1;
	}
	switch (id) {
	case pwm_a:
		P_PWM_PWM_A = pwm_voltage_table[to][0];
		break;

	case pwm_ao_b:
		writel(pwm_voltage_table[to][0], AO_PWM_PWM_B);
		break;
	default:
		break;
	}
	_udelay_(200);
}
*/
void pwm_set_voltage(unsigned int id, unsigned int voltage)
{
	serial_puts("enter pwm_set_voltage\n");
	int to;

	switch (id) {
	case pwm_a:
		for (to = 0; to < ARRAY_SIZE(pwm_voltage_table); to++) {
			if (pwm_voltage_table[to][1] >= voltage) {
				break;
			}
		}
		if (to >= ARRAY_SIZE(pwm_voltage_table)) {
			to = ARRAY_SIZE(pwm_voltage_table) - 1;
		}
		writel(pwm_voltage_table[to][0], P_PWM_PWM_A);
		break;

	case pwm_ao_b:
		for (to = 0; to < ARRAY_SIZE(pwm_voltage_table_ee); to++) {
			if (pwm_voltage_table_ee[to][1] >= voltage) {
				break;
			}
		}
		if (to >= ARRAY_SIZE(pwm_voltage_table_ee)) {
			to = ARRAY_SIZE(pwm_voltage_table_ee) - 1;
		}
		writel(pwm_voltage_table_ee[to][0], P_AO_PWM_PWM_B);
		break;

	default:
		break;
	}
	_udelay_(200);
}

void power_init(int mode)
{
	unsigned int reg;

	serial_puts("set vcck to ");
	serial_put_dec(CONFIG_VCCK_INIT_VOLTAGE);
	serial_puts(" mv\n");
	pwm_set_voltage(pwm_a, CONFIG_VCCK_INIT_VOLTAGE);
	serial_puts("set vddee to ");
	serial_put_dec(CONFIG_VDDEE_INIT_VOLTAGE);
	serial_puts(" mv\n");
	pwm_set_voltage(pwm_ao_b, CONFIG_VDDEE_INIT_VOLTAGE);
	pwm_init(pwm_a);
	pwm_init(pwm_ao_b);
	serial_puts("set vddee enable AO 2/10 to high\n ");
	reg = readl(AO_GPIO_O_EN_N);
	reg &= ~((1 << 2)|(1 << 10));
	reg |=  ((1 << 18)|(1 << 26));
	writel(reg, AO_GPIO_O_EN_N);
	serial_puts("set test_n to high\n ");
	reg = readl(AO_SEC_REG0);
	reg |= (1 << 0);
	writel(reg, AO_SEC_REG0);
	reg = readl(AO_GPIO_O_EN_N);
	reg |= (1 << 31);
	writel(reg, AO_GPIO_O_EN_N);
}
