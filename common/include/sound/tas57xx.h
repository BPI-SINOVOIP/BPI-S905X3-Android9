#ifndef _TAS57xx_H
#define _TAS57xx_H

#include <sound/soc.h>
#include <linux/gpio/consumer.h>

#define TAS57XX_EQ_REGS 20
#define TAS57XX_EQ_BQS 9
#define TAS57XX_EQ_CHNLS 2
#define TAS57XX_EQ_BYTES (TAS57XX_EQ_REGS * TAS57XX_EQ_BQS * TAS57XX_EQ_CHNLS)

#ifndef NAME_SIZE
#define NAME_SIZE 32
#endif

struct tas57xx_reg_cfg {
	const char *reg_data;
};

struct tas57xx_eq_cfg {
	char name[NAME_SIZE];
	char *regs;
	int reg_bytes;
};

struct tas57xx_platform_data {
	int (*init_func)(void);
	int (*early_suspend_func)(void);
	int (*suspend_func)(void);
	int (*resume_func)(void);
	int (*late_resume_func)(void);
	char *custom_init_value_table;
	int init_value_table_len;
	char *init_regs;
	int num_init_regs;
	char *custom_drc1_table;
	int custom_drc1_table_len;
	char *custom_drc1_tko_table;
	int custom_drc1_tko_table_len;
	char *custom_drc2_table;
	int custom_drc2_table_len;
	char *custom_drc2_tko_table;
	int custom_drc2_tko_table_len;
	int num_eq_cfgs;
	struct tas57xx_eq_cfg *eq_cfgs;
	char *custom_sub_bq_table;
	int custom_sub_bq_table_len;
	unsigned int custom_master_vol;

	int eq_enable;
	int drc_enable;
	int enable_ch1_drc;
	int enable_ch2_drc;
	int enable_hardmute;
	int i2c_addr;
	int reset_pin;
	int phone_pin;
	int scan_pin;
};

#endif
