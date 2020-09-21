#ifdef CONFIG_CEC_WAKEUP
#include <cec_tx_reg.h>
#endif
/*
 * board/amlogic/gxtvbb_skt_v1/firmware/scp_task/pwr_ctrl.c
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

static unsigned int ao_timer_ctrl;
static unsigned int ao_timera;

static void reset_ao_timera(void)
{
	unsigned int val;
	ao_timer_ctrl = readl(AO_TIMER_REG);
	ao_timera = readl(AO_TIMERA_REG);
	/* set ao timera work mode and interrrupt time 100us resolution*/
	val = (1 << 2) | (3 << 0) | (1 << 3);
	writel(val, AO_TIMER_REG);
	/* periodic time 10ms */
	writel(100,AO_TIMERA_REG);
}

static void restore_ao_timer(void)
{
	writel(ao_timer_ctrl,AO_TIMER_REG);
	writel(ao_timera,AO_TIMERA_REG);
}

static void power_off_at_clk81(void)
{
}
static void power_on_at_clk81(unsigned int suspend_from)
{
}

static void power_off_at_24M(void)
{
}
static void power_on_at_24M(void)
{
}

static void power_off_at_32k(unsigned int suspend_from)
{
}
static void power_on_at_32k(unsigned int suspend_from)
{
}

static void wakeup_timer_setup(void)
{
	/* 1ms resolution*/
	unsigned value;
	value = readl(P_ISA_TIMER_MUX);
	value |= ((0x3<<0) | (0x1<<12) | (0x1<<16));
	writel(value, P_ISA_TIMER_MUX);
	writel(9, P_ISA_TIMERA);
}
static void wakeup_timer_clear(void)
{
	unsigned value;
	value = readl(P_ISA_TIMER_MUX);
	value &= ~((0x1<<12) | (0x1<<16));
	writel(value, P_ISA_TIMER_MUX);
}

static void get_wakeup_source(void *response, unsigned int suspend_from)
{
	struct wakeup_info *p = (struct wakeup_info *)response;
	unsigned val;
	p->gpio_info_count = 0;
	p->status = RESPONSE_OK;
	val = (AUTO_WAKEUP_SRC | REMOTE_WAKEUP_SRC);

#ifdef CONFIG_CEC_WAKEUP
	if (suspend_from != SYS_POWEROFF)
		val |= CEC_WAKEUP_SRC;
#endif

	p->sources = val;

}
static unsigned int detect_key(unsigned int suspend_from)
{
	int exit_reason = 0;
	unsigned int time_out = readl(AO_DEBUG_REG2);
	unsigned time_out_ms = time_out*100;
	unsigned char adc_key_cnt = 0;
	unsigned *irq = (unsigned *)WAKEUP_SRC_IRQ_ADDR_BASE;
	/* unsigned *wakeup_en = (unsigned *)SECURE_TASK_RESPONSE_WAKEUP_EN; */

	/* setup wakeup resources*/
	/*auto suspend: timerA 10ms resolution*/
	if (time_out_ms != 0)
		wakeup_timer_setup();
	reset_ao_timera();
	init_remote();
#ifdef CONFIG_CEC_WAKEUP
	if (hdmi_cec_func_config & 0x1) {
		remote_cec_hw_reset();
		cec_node_init();
	}
#endif

	/* *wakeup_en = 1;*/
	do {
#ifdef CONFIG_CEC_WAKEUP
		if (irq[IRQ_AO_CEC] == IRQ_AO_CEC_NUM) {
			irq[IRQ_AO_CEC] = 0xFFFFFFFF;
			if (suspend_from == SYS_POWEROFF)
				continue;
			if (cec_msg.log_addr) {
				if (hdmi_cec_func_config & 0x1) {
					cec_handler();
					if (cec_msg.cec_power == 0x1) {
						/*cec power key*/
						exit_reason = CEC_WAKEUP;
						break;
					}
				}
			} else if (hdmi_cec_func_config & 0x1)
				cec_node_init();
		}
#endif
		if (irq[IRQ_TIMERA] == IRQ_TIMERA_NUM) {
			irq[IRQ_TIMERA] = 0xFFFFFFFF;
			if (time_out_ms != 0)
				time_out_ms--;
			if (time_out_ms == 0) {
				wakeup_timer_clear();
				exit_reason = AUTO_WAKEUP;
			}
		}
		if (irq[IRQ_AO_TIMERA] == IRQ_AO_TIMERA_NUM) {
			irq[IRQ_AO_TIMERA] = 0xFFFFFFFF;
			saradc_enable();
			if (check_adc_key_resume()) {
				adc_key_cnt++;
				/*using variable 'adc_key_cnt' to eliminate the dithering of the key*/
				if (2 == adc_key_cnt)
					exit_reason = POWER_KEY_WAKEUP;
			} else {
				adc_key_cnt = 0;
			}
			saradc_disable();
		}
		if (irq[IRQ_AO_IR_DEC] == IRQ_AO_IR_DEC_NUM) {
			irq[IRQ_AO_IR_DEC] = 0xFFFFFFFF;
			if (remote_detect_key())
				exit_reason = REMOTE_WAKEUP;
		}
		if (exit_reason)
			break;
		else
			asm volatile("wfi");
	} while (1);
	wakeup_timer_clear();
	restore_ao_timer();
	return exit_reason;
}

static void pwr_op_init(struct pwr_op *pwr_op)
{
	pwr_op->power_off_at_clk81 = power_off_at_clk81;
	pwr_op->power_on_at_clk81 = power_on_at_clk81;
	pwr_op->power_off_at_24M = power_off_at_24M;
	pwr_op->power_on_at_24M = power_on_at_24M;
	pwr_op->power_off_at_32k = power_off_at_32k;
	pwr_op->power_on_at_32k = power_on_at_32k;

	pwr_op->detect_key = detect_key;
	pwr_op->get_wakeup_source = get_wakeup_source;
}
