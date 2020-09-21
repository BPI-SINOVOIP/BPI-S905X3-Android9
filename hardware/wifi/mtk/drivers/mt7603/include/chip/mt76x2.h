#ifndef __MT76X2_H__
#define __MT76X2_H__

#include "../mcu/andes_core.h"
#include "../mcu/andes_rlt.h"
#include "../phy/mt76x2_rf.h"

struct _RTMP_ADAPTER;

#define MAX_RF_ID	127
#define MAC_RF_BANK 7

void mt76x2_init(struct _RTMP_ADAPTER *ad);
void mt76x2_adjust_per_rate_pwr_delta(struct _RTMP_ADAPTER *ad, UINT8 channel);
void mt76x2_get_tx_pwr_per_rate(struct _RTMP_ADAPTER *ad);
void mt76x2_antenna_sel_ctl(struct _RTMP_ADAPTER *ad);
int mt76x2_read_chl_pwr(struct _RTMP_ADAPTER *ad);
void mt76x2_pwrOn(struct _RTMP_ADAPTER *ad);
void mt76x2_calibration(struct _RTMP_ADAPTER *ad, UINT8 channel);
void mt76x2_tssi_calibration(struct _RTMP_ADAPTER *ad, UINT8 channel);
void mt76x2_tssi_compensation(struct _RTMP_ADAPTER *ad, UINT8 channel);
int mt76x2_set_ed_cca(struct _RTMP_ADAPTER *ad, UINT8 enable);
int mt76x2_reinit_agc_gain(struct _RTMP_ADAPTER *ad, UINT8 channel);
int mt76x2_reinit_hi_lna_gain(struct _RTMP_ADAPTER *ad, UINT8 channel);
void mt76x2_get_agc_gain(struct _RTMP_ADAPTER *ad);
char get_chl_grp(UINT8 channel);

#ifdef CONFIG_ATE
VOID mt76x2_ate_do_calibration(
	struct _RTMP_ADAPTER *ad, UINT32 cal_id, UINT32 param);
#endif /* CONFIG_ATE */

struct mt76x2_frequency_item {
	UINT8 channel;
	UINT32 fcal_target;
	UINT32 sdm_integer;
	UINT32 sdm_fraction;	
};

#ifdef CONFIG_CALIBRATION_COLLECTION
#define MAX_MT76x2_CALIBRATION_ID (RXIQC_FD_CALIBRATION_7662+1)
#define MAX_RECORD_REG	50
enum reg_rf0_addr {
	RF0_214 = 0,
	RF0_218 = 1,
	RF0_21C = 2,
	RF0_220 = 3,
	RF0_224 = 4,
	RF0_228 = 5,
	RF0_24C = 6,
	RF0_250 = 7,
	RF0_264 = 8,
	RF0_278 = 9,
};
enum reg_rf1_addr {
	RF1_214 = 20,
	RF1_218 = 21,
	RF1_21C = 22,
	RF1_220 = 23,
	RF1_224 = 24,
	RF1_228 = 25,
	RF1_24C = 26,
	RF1_250 = 27,
	RF1_264 = 28,
	RF1_278 = 29,
};

enum reg_bbp_addr {
	BBP_2774 = 0,
	BBP_2780 = 1,
	BBP_2784 = 2,
	BBP_2788 = 3,
	BBP_278C = 4,
	BBP_2790 = 5,
	BBP_2794 = 6,
	BBP_2798 = 7,
	BBP_279C = 8,
	BBP_27A0 = 9,
	BBP_27A4 = 10,
	BBP_27A8 = 11,
	BBP_27AC = 12,
	BBP_27B0 = 13,
	BBP_27B4 = 14,
	BBP_27B8 = 15,
	BBP_27BC = 16,
	BBP_27C0 = 17,
	BBP_27C4 = 18,
	BBP_27C8 = 19,
	BBP_27CC = 20,
	BBP_208C = 21,
	BBP_2720 = 22,
	BBP_2C60 = 23,
	BBP_2C64 = 24,
	BBP_2C70 = 25,
	BBP_2C74 = 26,
	BBP_2818 = 27,
	BBP_281C = 28,
	BBP_2820 = 29,
	BBP_2824 = 30,
	BBP_2828 = 31,
	BBP_282C = 32,
};

struct mt76x2_calibration_info {
	UINT16 	addr[MAX_RECORD_REG];
	UINT32 	value[MAX_RECORD_REG];
};

void record_calibration_info(struct _RTMP_ADAPTER *ad, UINT32 cal_id);
void dump_calibration_info(struct _RTMP_ADAPTER *ad, UINT32 cal_id);
#endif

#endif
