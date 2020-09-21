/*
 * Copyright 2018-, altobeam.inc
 */

#include <net/atbm_mac80211.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <linux/bitmap.h>
#include <linux/pm_qos.h>
#include <linux/inetdevice.h>
#include <net/net_namespace.h>
#include <net/cfg80211.h>
#ifdef CONFIG_WIRELESS_EXT
#include <net/iw_handler.h>
#endif
#ifdef IPV6_FILTERING
#include <net/if_inet6.h>
#include <net/addrconf.h>
#endif /*IPV6_FILTERING*/

#include "ieee80211_i.h"
#include "driver-ops.h"
#include "rate.h"
#include "mesh.h"
#include "wep.h"
#include "led.h"
#include "cfg.h"
#include "debugfs.h"
#include "../sbus.h"
static u8 ETF_bStartTx = 0;
static u8 ETF_bStartRx = 0;

static u8 CodeStart = 0;
static u8 CodeEnd = 0;

static u8 ucWriteEfuseFlag = 0;
static struct rxstatus_signed gRxs_s;

int Atbm_Test_Success = 0;

#define DCXO_CODE_MINI		0//24//0
#define DCXO_CODE_MAX		127//38//63
#define TARGET_FREQOFFSET_HZ  (7000)

#ifdef CONFIG_WIRELESS_EXT
#include "../apollo.h"
#include "../smartconfig.h"
#include "../wsm.h"

enum atbm_test{
		ALTM_RATE_SET_FIX = 1,
		ALTM_RATE_SET_FLAGS = 2,
		ALTM_RATE_SET_INDEX = 3,
		ALTM_RATE_GET_FIX = 4,
		ALTM_RATE_GET_FLAGS = 5,
		ALTM_RATE_GET_INDEX = 6,
};


struct altm_msg{
	unsigned int type;
	unsigned int value;
	unsigned int externData[30];
};

typedef struct bitrate{
	char *rate;
	int flags;
	int index;
	char *ratename;
}BITRATE;


struct rxstatus{
	u32 GainImb;
	u32 PhaseImb;
	u32 Cfo;
	u32 evm;
	u32  RSSI;
	u32 probcnt;
};

struct rxstatus_signed{
	u8 valid;
	s32 GainImb;
	s32 PhaseImb;
	s32 Cfo;
	s32 evm;
	s32  RxRSSI;
	s32  TxRSSI;
	u16 dcxo;
	s32 result;
};


BITRATE ratetab[] = {
	{"1000000", 0, 0, "1"},
	{"2000000", 0, 1, "2"},
	{"5500000", 0, 2, "5.5"},
	{"11000000", 0, 3, "11"},
	{"6000000", 0, 4, "6"},
	{"9000000", 0, 5, "9"},
	{"12000000", 0, 6, "12"},
	{"18000000", 0, 7, "18"},
	{"24000000", 0, 8, "24"},
	{"36000000", 0, 9, "36"},
	{"48000000", 0, 10, "48"},
	{"54000000", 0, 11, "54"},
	{"6500000", 8, 0, "6.5"},
	{"13000000", 8, 1, "13"},
	{"19500000", 8, 2, "19.5"},
	{"26000000", 8, 3, "26"},
	{"39000000", 8, 4, "39"},
	{"52000000", 8, 5, "52"},
	{"58500000", 8, 6, "58.5"},
	{"65000000", 8, 7, "65"},
	{"6000000",  8, 8,  "32"},
};


int atbm_test_rx_cnt = 0;


#define RATE_CONTROL_UNICAST 1

#define	arraysize(a)	sizeof(a)/sizeof(a[0])


extern int atbm_tool_rts_threshold;
extern int atbm_tool_shortGi;
extern struct tagAtbmRateControl g_atbm_rate_Ctl;

extern int atbm_tesmode_reply(struct wiphy *wiphy,const void *data, int len);
extern int rate_altm_control_test(struct wiphy *wiphy, void *data, int len);
extern void atbm_set_shortGI(u32 shortgi);
extern int wsm_start_tx(struct atbm_common *hw_priv, struct ieee80211_vif *vif);
extern int wsm_stop_tx(struct atbm_common *hw_priv);

extern int wsm_start_tx_v2(struct atbm_common *hw_priv, struct ieee80211_vif *vif );

extern int wsm_send_result(struct atbm_common *hw_priv, struct ieee80211_vif *vif );
extern int atbm_ioctl_etf_result_get(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra);
extern int atbm_upload_beacon_private(struct atbm_vif *priv);

#ifdef ATBM_PRIVATE_IE
extern int atbm_upload_proberesp_private(struct atbm_vif *priv);
#endif
static char *ch_and_type = NULL;
/*
static const u32 band_table[21] = {10, 20, 55, 110, 60, 90, 120,
								180, 240, 360, 480, 540, 65, 130,
								195, 260, 390, 520, 585, 650, 320};

*/

struct iw_handler_def atbm_handlers_def;
static const struct iw_priv_args atbm_privtab[] = {
		{SIOCIWFIRSTPRIV + 0x0, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 17, 0, "efuse_set_mac"},
		{SIOCIWFIRSTPRIV + 0x1, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 1, 0, "smartcfg_start"},	
		{SIOCIWFIRSTPRIV + 0x2, IW_PRIV_TYPE_CHAR | 50, 0, "start_tx"},
		{SIOCIWFIRSTPRIV + 0x3, IW_PRIV_TYPE_CHAR | 10, 0, "stop_tx"},
		{SIOCIWFIRSTPRIV + 0x4, IW_PRIV_TYPE_CHAR | 50, 0, "start_rx"},
		{SIOCIWFIRSTPRIV + 0x5, IW_PRIV_TYPE_CHAR | 10, 0, "stop_rx"},
		{SIOCIWFIRSTPRIV + 0x6, IW_PRIV_TYPE_CHAR | 5, 0, "fwdbg"},
		{SIOCIWFIRSTPRIV + 0x7, IW_PRIV_TYPE_CHAR | 10, 0, "help"},
		{SIOCIWFIRSTPRIV + 0x8, IW_PRIV_TYPE_CHAR | 50, 0, "fwcmd"},
		{SIOCIWFIRSTPRIV + 0x9, IW_PRIV_TYPE_CHAR | 10, 0, "rate"},
		{SIOCIWFIRSTPRIV + 0xa, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 1, 0, "smartcfg_stop"},
		{SIOCIWFIRSTPRIV + 0xb, IW_PRIV_TYPE_CHAR | 10, 0, "rts_threshold"},
		{SIOCIWFIRSTPRIV + 0xc, IW_PRIV_TYPE_CHAR | 10, 0, "set_gi"},
		{SIOCIWFIRSTPRIV + 0xd, IW_PRIV_TYPE_CHAR | 10, 0, "getmac"},
		{SIOCIWFIRSTPRIV + 0xe, IW_PRIV_TYPE_CHAR | 10, 0, "wolEn"},
		{SIOCIWFIRSTPRIV + 0xf, IW_PRIV_TYPE_CHAR | 16, 0, "get_rx_stats"},
		{SIOCIWFIRSTPRIV + 0x10, IW_PRIV_TYPE_CHAR | 16, 0, "freqoffset"},

		{SIOCIWFIRSTPRIV + 0x11, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 1, 0, "smartonv2"},
		{SIOCIWFIRSTPRIV + 0x12, IW_PRIV_TYPE_CHAR | 16, 0, "etf_result_get"},
		{SIOCIWFIRSTPRIV + 0x13, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 1, 0, "smartoffv2"},
		#ifdef ATBM_PRIVATE_IE
		/*Private IE for Scan*/
		{SIOCIWFIRSTPRIV + 0x14, IW_PRIV_TYPE_CHAR | 500, 0, "insert_data"},
		{SIOCIWFIRSTPRIV + 0x15, IW_PRIV_TYPE_CHAR | 500, 0, "get_data"},
		{SIOCIWFIRSTPRIV + 0x16, IW_PRIV_TYPE_CHAR | 500, 0, "send_msg"},
		{SIOCIWFIRSTPRIV + 0x17, IW_PRIV_TYPE_CHAR | 500, 0, "recv_msg"},
		{SIOCIWFIRSTPRIV + 0x18, IW_PRIV_TYPE_CHAR | 32, 0, "private_test"},
		{SIOCIWFIRSTPRIV + 0x19, IW_PRIV_TYPE_CHAR | 32, 0, "hide_ssid"},
		{SIOCIWFIRSTPRIV + 0x1A, IW_PRIV_TYPE_CHAR | 32, 0, "set_freq"},
		{SIOCIWFIRSTPRIV + 0x1B, IW_PRIV_TYPE_CHAR | 32, 0, "set_txpower"},
		{SIOCIWFIRSTPRIV + 0x1C, IW_PRIV_TYPE_CHAR | 32, 0, "ipc_reset"},
		#ifndef ATBM_RATE
		{SIOCIWFIRSTPRIV + 0x1D, IW_PRIV_TYPE_CHAR | 32, 0, "get_tp_rate"},
		#endif
		#else
		{SIOCIWFIRSTPRIV + 0x14, IW_PRIV_TYPE_CHAR | 32, 0, "hide_ssid"},
		{SIOCIWFIRSTPRIV + 0x15, IW_PRIV_TYPE_CHAR | 32, 0, "set_freq"},
		{SIOCIWFIRSTPRIV + 0x16, IW_PRIV_TYPE_CHAR | 32, 0, "set_txpower"},
		#ifndef ATBM_RATE
		{SIOCIWFIRSTPRIV + 0x17, IW_PRIV_TYPE_CHAR | 32, 0, "get_tp_rate"},
		#endif
		#endif
};

static int atbm_ioctl_command_help(struct net_device * dev,struct iw_request_info * ifno,union iwreq_data * wrqu,char * ext)
{
	int ret = 0;
		printk("usage: 		iwpriv wlan0 <cmd> [para]..,\n");
		printk("fwdbg		<0|1> = gets/sets firmware debug message switch, when setting, 0, close, 1, open\n");
		printk("fwcmd		<firmware cmd line> = letting firmware performance command,e.g:fwcmd para1,para2,parq3....\n");
		printk("start_tx 	<channel,rate,len,is_40M,greedfiled> = start transmmit \n");
		printk("		channel value:1~14 \n");
		printk("		rate id: 1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54, 6.5, 13, 19.5,26, 39, 52, 58.5, 65, 32\n");
		printk("		len range:100~1024\n");
		printk("		is_40M value: 1:40M,0:20M\n");
		printk("		greedfiled value: 1:greedfiled,0:\n");
		printk("stop_tx		NO prarameter = stop transmmit\n");
		printk("start_rx 	<[1~14],[0|1]> = start receive,parameter:channel,channel type;1:40M,0:20M\n");
		printk("stop_rx		NO parameter = stop receive\n");
		printk("set_gi		<0|1> = 1,support shortGI; 0, not support shortGI\n");
		printk("rts_threshold		<show|value> = show or set rts threshold\n");
		printk("rate		<show|free|rate id> = show rate,or free a fixed rate,or forces a fixed rate support");
		printk(" rate id: 1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54, 6.5, 13, 19.5,26, 39, 52, 58.5, 65\n");
		printk("getmac		NO Parameter = get mac address\n");
		printk("wolEn		<0|1> = 1,enable wol; 0, disable wol\n");
		printk("get_rx_stats	<1>\n");
		printk("freqoffset		<0|1> 0:not write efuse; 1: write efuse \n");
		printk("etf_result_get  get the test result \n");
#ifdef ATBM_PRIVATE_IE
		printk("insert_data      insert private user data to (Probe Req or Beacon or Probe Resp) \n");
		printk("get_data         obtain private user data from the broadcast packet \n");
		printk("send_msg         start private scan according to the channel setting \n");
		printk("recv_msg         recv broadcast device information of contain private IE \n");
		printk("ipc_reset        reset ipc private scan function \n");
#endif
		printk("hide_ssid        Enable or Disble hidden ssid func, only for AP mode \n");
		printk("set_freq		 param1: channel [1:14],param2: freq val(2380 or 2504) \n");
		printk("set_txpower         1: high tx power, 0: normal tx power \n");

		
	return ret;
}


static int atbm_ioctl_smartconfig_start(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	#ifdef ATBM_SUPPORT_SMARTCONFIG
	u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	
	value = *extra - 0x30;
	
	atbm_smartconfig_start(hw_priv,value);
	#endif

	return 0;
}

static int atbm_ioctl_smartconfig_stop(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	#ifdef ATBM_SUPPORT_SMARTCONFIG
	u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	value = *extra - 0x30;
	atbm_smartconfig_stop(hw_priv);
	#endif

	return 0;
}

static int atbm_ioctl_smartconfig_start_v2(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	#ifdef ATBM_SUPPORT_SMARTCONFIG
	u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	int i = 0;	
	u32 enable = 1;
	
	value = *extra - 0x30;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_SMARTCONFIG_START,
						&enable, sizeof(enable), vif->if_id));
					break;
				}
			}
	atbm_smartconfig_start(hw_priv,enable);
	#endif

	return 0;
}

static int atbm_ioctl_smartconfig_stop_v2(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	#ifdef ATBM_SUPPORT_SMARTCONFIG
	u32 value;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	int i = 0;	
	u32 enable = 0;

	value = *extra - 0x30;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_SMARTCONFIG_START,
						&enable, sizeof(enable), vif->if_id));
					break;
				}
			}
	atbm_smartconfig_start(hw_priv,enable);
	#endif

	return 0;
}

static int atbm_ioctl_fwdbg(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{
	int i = 0;	
	int ret = 0;
	u8 ucDbgPrintOpenFlag = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	
	ucDbgPrintOpenFlag = *ext - 0x30;
	printk(KERN_ERR "ALTM_GET_DBG_PRINT_TO_HOST\n");
	printk(KERN_ERR "%s dbgflag:%d\n", __func__, ucDbgPrintOpenFlag);
			atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}


	return ret;
}

static int atbm_ioctl_fwcmd(struct net_device *dev, struct iw_request_info *ifno, union iwreq_data *wrqu, char *ext)
{
	
	int i = 0;
	int ret = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	if(!(extra = atbm_kmalloc(wrqu->data.length+1,GFP_KERNEL)))
		return -ENOMEM;
	if(copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if(wrqu->data.length <= 1){
		printk("invalid parameter!\n");
		printk("e.g:./iwpriv wlan0 fwcmd intr\n");
		return -EINVAL;
	}

	for(i=0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] =' ';
	}

	if(extra[0] == ' '){
		printk("invalid parameter!\n");
		return -EINVAL;
	}
	//printk("exttra = %s  %d\n", extra, wrqu->data.length);
	i = 0;
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			printk("exttra = %s\n", extra);
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				extra, wrqu->data.length, vif->if_id));
			break;
		}
	}
	atbm_kfree(extra);
	return ret;
	
}


static int atbm_ioctl_start_tx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = 0;
	int len = 0;
	int num = 0;
	int flag = 0;
	int channel = 0;
	u32 rate = 0;
	u32 is_40M = 0;
	int band_value = 0;
	int greedfiled = 0;
	char *extra  = NULL;
	char *rate_p = NULL;
	char *len_p = NULL;
	char *is_40M_p = NULL;
	char *greedfiled_p = NULL;
	int etf_v2 = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	atbm_test_rx_cnt = 0;


	memset(&gRxs_s, 0, sizeof(struct rxstatus_signed));

	if(ETF_bStartTx || ETF_bStartRx){
		printk("Error! already start_tx, please stop_tx first!\n");
		return 0;
	}

	if(!(extra = atbm_kmalloc(wrqu->data.length + 1, GFP_KERNEL)))
		return -ENOMEM;
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}

	printk("atbm_ioctl_start_tx:%s\n",extra);
	if(wrqu->data.length < 10){
		printk("need to input parameters\n");
		printk("e.g: ./iwpriv wlan0 start_tx channel,rate,len,is_40M,greedfiled\n");
		printk("e.g: ./iwpriv wlan0 start_tx 1,1,300,1\n");
		return -EINVAL;
	}

	for(i=0;extra[i] != ',';i++){
		if(extra[i] == ','){
			break;
		}
		channel = channel * 10 +(extra[i] - 0x30);
	}
	
	for(i=0;i<wrqu->data.length;i++){
		if((extra[i] == ',') && (num == 0)){
				num++;
				rate_p = extra + i + 1;
		}
		else if((extra[i] == ',') && (num == 1)){
			len_p = extra + i + 1;
			num++;
		}
		else if((extra[i] == ',') && (num == 2)){
			is_40M_p = extra + i + 1;
			num++;
		}
		else if((extra[i] == ',') && (num == 3)){
			greedfiled_p = extra + i + 1;
			break;
		}
	}

	if((NULL == rate_p) || (NULL == len_p) || (NULL == is_40M_p) || (NULL == greedfiled_p)){
		printk("need more parameter,please try again!\n");
		printk("e.g: ./iwpriv wlan0 start_tx channel,rate,len,is_40M,greedfiled\n");
		return -EINVAL;
	}

	for(i=0;rate_p[i] != ',';i++){
		if(rate_p[i] == ','){
			break;
		}
		if(rate_p[i] == '.'){
			flag = 1;
			continue;
		}	
		band_value = band_value* 10 +(rate_p[i] - 0x30);
	}

	if(flag == 0)
		band_value = band_value * 10;

	for(i=0;len_p[i] != ',';i++){
		if(len_p[i] == ','){
			break;
		}
		len = len * 10 +(len_p[i] - 0x30);
	}

	
	if(is_40M_p[0] == ','){
		printk("invalid channnel type\n");
		printk("is_40M value: 1:40M,0:20M\n");
		return -EINVAL;
	}

	if((greedfiled_p[0] == ',') || (greedfiled_p[0] == ' ') || (greedfiled_p[0]) == '\0'){
		printk("invalid channnel type\n");
		printk("ps_mode value: 1:greedfiled,0:\n");
		return -EINVAL;
	}

	//printk("is_40M :%s greedfiled_p:%s\n", is_40M_p, greedfiled_p);
	

	for(i=0;is_40M_p[i] != ',';i++){
		if(1 == i){
			printk("invalid channel type!\n");
			printk("is_40M value: 1:40M,0:20M\n");
			return 	-EINVAL;
		}
		is_40M = is_40M * 10 + (is_40M_p[i] - 0x30);
	}

	for(i=0;greedfiled_p[i] != '\0';i++){
		greedfiled = greedfiled * 10 + (greedfiled_p[i] - 0x30);
	}


	//printk("is_40M = %d\n", is_40M);

	if((is_40M != 0) && (is_40M != 1)){
			printk("invalid 40M or 20M %d\n",is_40M);
			return -EINVAL;
	}

	if((greedfiled != 0) && (greedfiled != 1)){
			printk("invalid greedfiled %d\n",greedfiled);
			return -EINVAL;
	}
	
	//check channel
	if(channel <= 0 || channel > 14){
		printk("invalid channel!\n");
		return -EINVAL;
	}
	//printk("rate===%d\n",rate);

	//check rate 
		switch(band_value){
			case 10: rate = WSM_TRANSMIT_RATE_1;
			break;
			case 20: rate = WSM_TRANSMIT_RATE_2;
			break;
			case 55: rate = WSM_TRANSMIT_RATE_5;
			break;
			case 110: rate = WSM_TRANSMIT_RATE_11;
			break;
			case 60: rate = WSM_TRANSMIT_RATE_6;
			break;
			case 90: rate = WSM_TRANSMIT_RATE_9;
			break;
			case 120: rate = WSM_TRANSMIT_RATE_12;
			break;
			case 180: rate = WSM_TRANSMIT_RATE_18;
			break;
			case 240: rate = WSM_TRANSMIT_RATE_24;
			break;
			case 360: rate = WSM_TRANSMIT_RATE_36;
			break;
			case 480: rate = WSM_TRANSMIT_RATE_48;
			break;
			case 540: rate = WSM_TRANSMIT_RATE_54;
			break;
			case 65: rate = WSM_TRANSMIT_RATE_HT_6;
			break;
			case 130: rate = WSM_TRANSMIT_RATE_HT_13;
			break;
			case 195: rate = WSM_TRANSMIT_RATE_HT_19;
			break;
			case 260: rate = WSM_TRANSMIT_RATE_HT_26;
			break;
			case 390: rate = WSM_TRANSMIT_RATE_HT_39;
			break;
			case 520: rate = WSM_TRANSMIT_RATE_HT_52;
			break;
			case 585: rate = WSM_TRANSMIT_RATE_HT_58;
			break;
			case 650: rate = WSM_TRANSMIT_RATE_HT_65;
			break;
			default:
				printk("invalid rate!\n");
				return -EINVAL;
				
		}

	if((is_40M == 1 )&& (rate < WSM_TRANSMIT_RATE_HT_6)){
		printk("invalid 40M rate\n");
		return -EINVAL;
	}	
	if((is_40M == 1 )&& ((channel < 3)||(channel > 11))){
		printk("invalid 40M rate,channel value range:3~11\n");
		return -EINVAL;
	}
	
	if(len == 99999){
		ucWriteEfuseFlag = 1;
		etf_v2 = 1;	
		len = hw_priv->etf_len = 1000; 
	}else if(len == 99998)
	{
		ucWriteEfuseFlag = 0;
		etf_v2 = 1;	
		len = hw_priv->etf_len = 1000; 
	}

	//check len
	if(len < 100 || len > 1024){
		printk("len:%d\n", len);
		printk("invalid len!\n");
		return -EINVAL;
	}
	if(is_40M == 1){
		is_40M = NL80211_CHAN_HT40PLUS;//
		channel -= 2;
	}

	printk("NL80211_CHAN_HT40PLUS:%d\n", NL80211_CHAN_HT40PLUS);

	//printk("%d, %d, %d, %d\n", channel, rate, len, is_40M);
	hw_priv->etf_channel = channel;
	hw_priv->etf_channel_type = is_40M;
	hw_priv->etf_rate = rate;
	hw_priv->etf_len = len; 
	hw_priv->etf_greedfiled = greedfiled;
	
	atbm_for_each_vif(hw_priv,vif,i){
		if((vif != NULL)){
			printk("*******\n");

			down(&hw_priv->scan.lock);

			
			mutex_lock(&hw_priv->conf_mutex);
			ETF_bStartTx = 1;
			if(etf_v2){
				
				CodeStart = DCXO_CODE_MINI;
				CodeEnd = DCXO_CODE_MAX;
				wsm_start_tx_v2(hw_priv, vif->vif);
			}
			else
				wsm_start_tx(hw_priv, vif->vif);
			mutex_unlock(&hw_priv->conf_mutex);
			break;
		}
	}
	
	atbm_kfree(extra);
	return ret;
}

static int atbm_ioctl_stop_tx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = 0;
	u8 ucDbgPrintOpenFlag = 1;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	struct atbm_vif *vif;

	if(0 == ETF_bStartTx){
		printk("please start start_rx first,then stop_rx\n");
		return -EINVAL;
	}

	if(wrqu->data.length > 1){
		printk("redundant parameters,please try again!\n");
		return -EINVAL;
	}
	
	mutex_lock(&hw_priv->conf_mutex);
	wsm_stop_tx(hw_priv);
	ETF_bStartTx = 0;
	mutex_unlock(&hw_priv->conf_mutex);
	//./iwpriv wlan0 fwdbg 0
	
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}
	
	//printk("%s %d\n", __FUNCTION__, __LINE__);

	return ret;
}

static int atbm_ioctl_start_rx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = 0;
	int is_40M = 0;
	int channel = 0;
	char *extra = NULL;
	char *is_40M_p = NULL;
	char cmd[20] = "monitor 1 ";
	u8 ucDbgPrintOpenFlag = 1;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;


	if(ETF_bStartTx || ETF_bStartRx){
		printk("Error! already ETF_bStartRx/ETF_bStartTx, please stop first!\n");
		return 0;
	}


	//./iwpriv wlan0 fwdbg 1
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}

	if(wrqu->data.length <= 3){
		printk("need to input parameters\n");
		printk("e.g: ./iwpriv wlan0 start_rx 1,0\n");
		return -EINVAL;
	}

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL)))
		return -EINVAL;

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}

	for(i=0;extra[i] != ',';i++){
		if(extra[i] == ','){
			break;
		}
		channel = channel * 10 +(extra[i] - 0x30);
	}

	for(i=0;i<wrqu->data.length;i++){
			if(extra[i] == ','){
				is_40M_p = extra +i + 1;
				break;
			}	
		}

	if((NULL == is_40M_p) || (is_40M_p[0] == '\0')){
		printk("invalid channnel type\n");
		printk("is_40M value: 1:40M,0:20M\n");
		return -EINVAL;
	}

	for(i=0;is_40M_p[i] != '\0';i++){
		if(1 == i){
			printk("invalid channel type!\n");
			printk("is_40M value: 1:40M,0:20M\n");
			return 	-EINVAL;
		}
		is_40M = is_40M * 10 + (is_40M_p[i] - 0x30);
	}

	printk("is_40M:%d\n", is_40M);
	if((is_40M != 0) && (is_40M != 1)){
		printk("invalid 40M or 20M\n");
		return -EINVAL;
	}
	
	if(channel <= 0 || channel > 14){
			printk("invalid channel!\n");
			return -EINVAL;
		}

	for(i=0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] =' ';
	}


	if((is_40M == 1 )&& ((channel == 1)||(channel > 11))){
		printk("invalid 40M rate\n");
		return -EINVAL;
	}
	
	ch_and_type = extra;
	memcpy(cmd+10, extra, wrqu->data.length);
	
	printk("CMD:%s\n", cmd);
	i = 0;
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			ETF_bStartRx = 1;
			printk("extra = %s %d\n", extra, wrqu->data.length + 10);
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				cmd, wrqu->data.length + 10, vif->if_id));
			break;
		}
	}	

	return ret;
}


static int atbm_ioctl_stop_rx(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = 0;
	char cmd[20] = "monitor 0 ";
	u8 ucDbgPrintOpenFlag = 0;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;

	if(wrqu->data.length > 1){
		printk("redundant parameters, please try again!\n");
		return -EINVAL;
	}

	if((0 == ETF_bStartRx) || (NULL == ch_and_type)){
		printk("please start start_rx first,then stop_rx\n");
		return -EINVAL;
	}

	ETF_bStartRx = 0;
	
	memcpy(cmd+10, ch_and_type, 3);
	//printk("cmd %s\n", cmd);
	i = 0;
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				cmd, 13, vif->if_id));
			break;
		}
	}

	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}
	return ret;
}


static int atbm_ioctl_get_rx_stats(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;
	int i = 0;
	
	if(!(extra = atbm_kmalloc(16, GFP_KERNEL)))
		return -ENOMEM;

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_read_mib(hw_priv, WSM_MIB_ID_GET_ETF_RX_STATS,
						extra, 16, vif->if_id));
					break;
				}
			}

	if((ret = copy_to_user(wrqu->data.pointer, extra, 16)) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}
	atbm_kfree(extra);
	return ret;
}

static int atbm_ioctl_set_rate(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = 0;
	int value = 0;
	int arraylen = 0;
	int counter;
	char *extra = NULL;
	struct altm_msg msg;
	//struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	
	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL)))
		return -ENOMEM;
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if (wrqu->data.length == 1)
	{
		printk("rate need argument\n");
		return ret;
	}

	if(!(memcmp(extra, "show", wrqu->data.length))){
		//atbm_tesmode_reply(wiphy, &g_atbm_rate_Ctl.rate_fix[RATE_CONTROL_UNICAST], sizeof(u32));
		printk("rate fix=%d\n", g_atbm_rate_Ctl.rate_fix[RATE_CONTROL_UNICAST]);
		//atbm_tesmode_reply(wiphy, &g_atbm_rate_Ctl.my_flags[RATE_CONTROL_UNICAST], sizeof(u8));
		printk("rate flags=%d\n", g_atbm_rate_Ctl.my_flags[RATE_CONTROL_UNICAST]);
		//atbm_tesmode_reply(wiphy, &g_atbm_rate_Ctl.my_index[RATE_CONTROL_UNICAST], sizeof(u8));
		printk("rate index=%d\n", g_atbm_rate_Ctl.my_index[RATE_CONTROL_UNICAST]);
	}
	else if (!(memcmp(extra, "free", wrqu->data.length)))
	{
		//msg.type = ALTM_RATE_SET_FIX;
		msg.value = 0;
		g_atbm_rate_Ctl.rate_fix[RATE_CONTROL_UNICAST] = msg.value;
		//msg.type = ALTM_RATE_SET_FLAGS;
		msg.value = 0;
		g_atbm_rate_Ctl.my_flags[RATE_CONTROL_UNICAST] = msg.value;
		//msg.type = ALTM_RATE_SET_INDEX;
		msg.value = 0;
		g_atbm_rate_Ctl.my_index[RATE_CONTROL_UNICAST] = msg.value;
	}else{
		arraylen = arraysize(ratetab);
		for(counter = 0; counter < arraylen; counter++)
		{
			if (strcasecmp(extra, ratetab[counter].ratename) == 0)
			{
					//msg.type = ALTM_RATE_SET_FIX;
					msg.value = 1;
					g_atbm_rate_Ctl.rate_fix[RATE_CONTROL_UNICAST] = msg.value;
					//atbm_tesmode_reply(wiphy, &g_atbm_rate_Ctl.my_flags[RATE_CONTROL_UNICAST], sizeof(u8));
					//msg.type = ALTM_RATE_SET_FLAGS;
					value &= ~0x8;
					msg.value = ratetab[counter].flags | value;
					g_atbm_rate_Ctl.my_flags[RATE_CONTROL_UNICAST] = msg.value;
					//msg.type = ALTM_RATE_SET_INDEX;
					msg.value = ratetab[counter].index;
					g_atbm_rate_Ctl.my_index[RATE_CONTROL_UNICAST] = msg.value;
					break;
			}
		}
		if (counter == arraylen)
		{
			printk("Maybe do not support '%s' rate or invalid parameter!\n", extra);
			return -EINVAL;
		}
	}


	return ret;
}

static int atbm_ioctl_set_rts(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i = 0;
	int ret = 0;
	int value = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;

	if(wrqu->data.length <= 1){
		printk("need parameter,please try again!\n");
		return -EINVAL;
	}

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL)))
		return -EINVAL;
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if(!(memcmp(extra, "show", wrqu->data.length))){
		printk("get_rts_threshold = %d\n", atbm_tool_rts_threshold);
		return ret;
	}
	else{
		for(i=0;extra[i] != '\0';i++){
			value = value * 10 + (extra[i] - 0x30);
		}

		if((value <0 ) || (value >= 2048)){
			printk("invalid parameter!\n");
			return -EINVAL;
		}
		
		printk(KERN_ERR "set_rtsthr is %d\n", value);
		atbm_for_each_vif(hw_priv,vif,i){
			if(vif != NULL)
			{
				__le32 val32;
				if (value != (u32) -1)
					val32 = __cpu_to_le32(value);
				else
					val32 = 0; /* disabled */

				/* mutex_lock(&priv->conf_mutex); */
				ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_RTS_THRESHOLD,
					&val32, sizeof(val32), vif->if_id));
				atbm_tool_rts_threshold = val32;
				break;
			}
		}
	}
	
	return ret;
}

static int atbm_ioctl_ctl_gi(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int ret = 0;
	int value = 0;
	char *extra = NULL;

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		printk("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if((wrqu->data.length <= 1) || (wrqu->data.length > 2)){
		printk("invalid parameter,please try again!\n");
		return -EINVAL;
	}

	value = *extra - 0x30;
	if((value < 0 ) || (value > 1)){
		printk("invalid parameter,parameter must be 1 or 0\n");
		return -EINVAL;
	}
	printk(KERN_ERR "set_short_gi(%d)\n", value);
	atbm_set_shortGI(value);
	atbm_tool_shortGi = value;

	return ret;
}


static int atbm_ioctl_setmac(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	int index;
	u8 macAddr[6] = {0};
	u8 extraBuff[18] = {0};
	int ret = -EINVAL;
#define   isHex(value)  	(((value > '9') && (value < '0')) && \
							 ((value > 'f') && (value < 'a')) && \
							 ((value > 'F') && (value < 'A')))
	memcpy(extraBuff, extra, 17);
	extraBuff[17] = ':';
	for (index  = 0 ; index < 17; index+= 3)
	{
		if (isHex(extraBuff[index]))
		{
					
			printk(KERN_ERR  "mac addr format error\n");
			return -EINVAL;
		}
		if (isHex(extraBuff[index + 1]))
		{
					
			printk(KERN_ERR  "mac addr format error\n");
			return -EINVAL;
		}
		
		if (extraBuff[index + 2] != ':')
		{
			printk(KERN_ERR "mac addr format error\n");
			return -EINVAL;
		}
		
	}

#undef isHex

	sscanf(extraBuff, "%x:%x:%x:%x:%x:%x", (int *)&macAddr[0], (int *)&macAddr[1], (int *)&macAddr[2], (int *)&macAddr[3], (int *)&macAddr[4], (int *)&macAddr[5]);

	if (local->ops != NULL)
	{
		ret = local->ops->set_mac_addr2efuse(&local->hw, macAddr);
	}

	return ret;
}

static int atbm_ioctl_getmac(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
	//int index;
	u8 macAddr[6] = {0};
	//u8 extraBuff[18] = {0};
	int ret = -EINVAL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	if(wrqu->data.length > 1){
		printk("command 'getmac' need not parameters\n");
		return ret;
	}

	if ((ret = wsm_get_mac_address(hw_priv, &macAddr[0])) == 0){
		printk("macAddr:%02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
		return ret;
	}
	else{
		printk("read mac address failed\n");
		return ret;
	}
	
	return ret;
}
static int atbm_ioctl_start_wol(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *ext)
{
	int i;
	int ret = 0;
	int value = 0;
	char *extra = NULL;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *vif;

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL))){
		printk("atbm_kmalloc failed!\n");
		return -EINVAL;
	}
	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))){
		atbm_kfree(extra);
		return -EINVAL;
	}

	if((wrqu->data.length <= 1) || (wrqu->data.length > 2)){
		printk("invalid parameter,please try again!\n");
		return -EINVAL;
	}

	value = *extra - 0x30;
	if((value < 0 ) || (value > 1)){
		printk("invalid parameter,parameter must be 1 or 0\n");
		return -EINVAL;
	}
	printk(KERN_ERR "start wol func(%d)\n", value);
	atbm_for_each_vif(hw_priv,vif,i){
		if(vif != NULL)
		{
			__le32 val32;
			if (value != (u32) -1)
				val32 = __cpu_to_le32(value);
			else
				val32 = 0; /* disabled */
	
			ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_START_WOL,
				&val32, sizeof(val32), vif->if_id));
			break;
		}
	}

	return ret;

}

#ifdef ATBM_PRIVATE_IE

#define USER_DATE_LEN 255
#define MAC_FILTER_NUM 8
static unsigned char filter_mac[MAC_FILTER_NUM][ETH_ALEN];

typedef struct _Wifi_Recv_Info
{
	unsigned char Ssid[32];
	unsigned char channel;
	unsigned int Rssi;		//信号强度
	unsigned int Quality;	 //信号质量
	unsigned char phy_noise;	//噪声
	unsigned char Bssid[6];	//热点 WiFi MAC地址
	unsigned char User_data[USER_DATE_LEN+1]; //存放收到广播包后要回复私有数据
}Wifi_Recv_Info_t;	  //接收结构体

typedef struct _Wifi_AP_Info
{
	unsigned int flag;
	Wifi_Recv_Info_t priv_recv_info;
}Wifi_Ap_Info_t;
static Wifi_Ap_Info_t priv_recv_info[MAC_FILTER_NUM];

typedef struct _Wifi_Send_Info
{
	unsigned char Ssid[32];
	unsigned short channel;
	unsigned char Bssid[MAC_FILTER_NUM][ETH_ALEN];    //需要过滤的mac地址
	unsigned char User_data[USER_DATE_LEN+1]; //存放要发送的私有数据
}Wifi_Send_Info_t;	   //发送结构体

typedef struct atbm_private_data{
	int set_flag;
	int data_len;
	char user_data[USER_DATE_LEN+1];
}ATBM_PRIVATE_DATA;
ATBM_PRIVATE_DATA atbm_priv_data;

#define MAC2STR(a) (a)[0],(a)[1],(a)[2],(a)[3],(a)[4],(a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

static int not_filter = 0;
static unsigned short ch_filter = 0;
extern void atbm_private_scan(struct atbm_vif *priv, u16 channel);

unsigned int atbm_get_filter_mac_addr(char *mac_addr)
{

	if(not_filter == 1)
		return 99;//No filter
		
	memcpy(mac_addr, &filter_mac[0][0],ETH_ALEN*MAC_FILTER_NUM);
	return 0;
}

void atbm_set_ap_info(Wifi_Recv_Info_t *infor)
{
	int i;
	
	printk("%s:\n", __func__);

	//ch_filter如果是0，代表是AP 模式，所以不需要判断channel
	//ch_filter如果不为0，代表是STA 模式，需要根据channel 进行过滤
	if((ch_filter != 0) && (ch_filter != infor->channel))
		return;
	
	for(i=0; i<MAC_FILTER_NUM;i++){
		if(!memcmp(priv_recv_info[i].priv_recv_info.Bssid, infor->Bssid, ETH_ALEN)){
			memcpy(&priv_recv_info[i].priv_recv_info, infor, sizeof(Wifi_Recv_Info_t));
			printk("Refresh info...\n");
			printk("[IE] [%d] ssid    %s\n", i,priv_recv_info[i].priv_recv_info.Ssid);
			printk("          channel %d\n", priv_recv_info[i].priv_recv_info.channel);
			printk("          Rssi    %d\n", priv_recv_info[i].priv_recv_info.Rssi);
			printk("          Qual    %d\n", priv_recv_info[i].priv_recv_info.Quality);
			printk("          Nosi    %d\n", priv_recv_info[i].priv_recv_info.phy_noise);
			printk("          Bssid   "MACSTR"\n", MAC2STR(priv_recv_info[i].priv_recv_info.Bssid));
			printk("          UserD   %s\n", priv_recv_info[i].priv_recv_info.User_data);
			break;
		}
		
		if(priv_recv_info[i].flag == 0){
			priv_recv_info[i].flag = 1;
			memcpy(&priv_recv_info[i].priv_recv_info, infor, sizeof(Wifi_Recv_Info_t));
			printk("[IE] [%d] ssid    %s\n", i,priv_recv_info[i].priv_recv_info.Ssid);
			printk("          channel %d\n", priv_recv_info[i].priv_recv_info.channel);
			printk("          Rssi    %d\n", priv_recv_info[i].priv_recv_info.Rssi);
			printk("          Qual    %d\n", priv_recv_info[i].priv_recv_info.Quality);
			printk("          Nosi    %d\n", priv_recv_info[i].priv_recv_info.phy_noise);
			printk("          Bssid   "MACSTR"\n", MAC2STR(priv_recv_info[i].priv_recv_info.Bssid));
			printk("          UserD   %s\n", priv_recv_info[i].priv_recv_info.User_data);
			break;
		}
	}
	
	if(i == MAC_FILTER_NUM)
		printk("[IE] warning, ap info pool is full\n");
	
	return;
}
void atbm_get_ap_info(void *buffer)
{
	int i;
	Wifi_Recv_Info_t *pdata = (Wifi_Recv_Info_t *)buffer;
	printk("%s:\n", __func__);
	for(i=0;i<MAC_FILTER_NUM;i++){
		if(priv_recv_info[i].flag == 1){
			memcpy(pdata, &priv_recv_info[i].priv_recv_info, sizeof(Wifi_Recv_Info_t));
			pdata += 1;
			printk("[IE] [%d] ssid    %s\n", i,priv_recv_info[i].priv_recv_info.Ssid);
			printk("          channel %d\n", priv_recv_info[i].priv_recv_info.channel);
			printk("          Rssi    %d\n", priv_recv_info[i].priv_recv_info.Rssi);
			printk("          Qual    %d\n", priv_recv_info[i].priv_recv_info.Quality);
			printk("          Nosi    %d\n", priv_recv_info[i].priv_recv_info.phy_noise);
			printk("          Bssid   "MACSTR"\n", MAC2STR(priv_recv_info[i].priv_recv_info.Bssid));
			printk("		  UserD   %s\n", priv_recv_info[i].priv_recv_info.User_data);

		}
	}

	return;
}

int atbm_get_insert_data(char *priv_data, int *data_len)
{
	if(atbm_priv_data.set_flag == 0)
		return -1;

	if(priv_data == NULL)
		return -1;

	//copy data
	memcpy(priv_data, atbm_priv_data.user_data, atbm_priv_data.data_len);
	//copy length
	*data_len = atbm_priv_data.data_len;

	printk("[IE] atbm_get_insert_data() len %d, data %s\n", *data_len, priv_data);
	printk("[IE] src data %s\n", atbm_priv_data.user_data);
	
	return 0;
}
/*
*1,  SSTAR_INSERT_USERDATA_CMD    	
*	函数  ioctl(global->ioctl_sock, SSTAR_INSERT_USERDATA_CMD, &user_data)
*	说明：插入用户私有信息，收到广播时来修改回复的私有数据
*/
static int atbm_ioctl_ie_insert_user_data(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	int i;
	int len;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;

	//struct atbm_vif *priv = ABwifi_hwpriv_to_vifpriv(hw_priv,0);
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;

	printk("[IE] atbm_ioctl_ie_insert_user_data()\n");
	printk("[IE] length %d, data %s\n", wdata->data.length, wdata->data.pointer);
	printk("[IE] extra data %s, dev 0x%x, ifname %s, ifid %d\n", extra, dev, sdata->name, priv->if_id);


	len =  wdata->data.length-1;
	if(len > 0 && len < USER_DATE_LEN+1){
		memset(atbm_priv_data.user_data, 0, USER_DATE_LEN+1);
		copy_from_user(atbm_priv_data.user_data, wdata->data.pointer, len);
		
		atbm_priv_data.data_len = len;
		atbm_priv_data.set_flag = 1;

		if(sdata->vif.type != NL80211_IFTYPE_STATION){
			ret = atbm_upload_beacon_private(priv);
			if(ret < 0){
				printk("[IE] upload beacon private failed %d\n", ret);
			}

			printk("[IE] vif 0x%x\n", priv);

			//ret = atbm_upload_proberesp_private(priv);
			//if(ret < 0){
			//	printk("[IE] upload probe resp private failed %d\n", ret);
			//}
		}
	}else{
		printk("[IE] warning, length invalid\n");
	}
	
	return ret;
}

/*
*2,  SSTAR_GET_USERDATA_CMD    			
*	函数  ioctl(global->ioctl_sock, SSTAR_INSERT_USERDATA_CMD, &user_data)
*	说明：接收广播发送过来的私有信息
*/
static int atbm_ioctl_ie_get_user_data(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
#if 0
	int ret = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	printk("[IE] atbm_ioctl_ie_get_user_data()\n");
	atbm_get_insert_data(wdata->data.pointer, &wdata->data.length);

	return ret;
#else
	int i;
	int ret = 0;
	char *ptr;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	printk("[IE] atbm_ioctl_ie_get_user_data()\n");
	
	ptr = wdata->data.pointer;
	
	for(i=0;i<MAC_FILTER_NUM;i++){
		if(priv_recv_info[i].flag == 1){
			memcpy(ptr, priv_recv_info[i].priv_recv_info.User_data, USER_DATE_LEN);
			printk("[IE] [%d] %s\n", i, ptr);

			ptr += USER_DATE_LEN;
		}
	}

	return ret;
#endif
}


/*
*3,  SSTAR_SEND_MSG_CMD       		     
*     函数 ioctl(global->ioctl_sock, SSTAR_SEND_MSG_CMD, &Wifi_Send_Info_t)
*	说明：发送广播命令，驱动收到命令会去scan,然后通channel 和 Bssid去过滤信息   
*                    Wifi_Send_Info_t.User_data为用户要发送的私有信息
*/
static int atbm_ioctl_ie_send_msg(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
#if 1
	int i;
	int len;
	unsigned short channel;
	//char test_mac[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
	Wifi_Send_Info_t *send_info;
	union iwreq_data *wdata;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	///struct atbm_vif *vif;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;


	printk("[IE] atbm_ioctl_ie_send_msg()\n");

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		printk("[IE] atbm_ioctl_ie_send_msg(), not sta mode\n");
		return -1;
	}

	wdata = (union iwreq_data *)wrqu;
	
	len = wdata->data.length-1;
	printk("[IE] len %d\n", len);
	
	if(len > 0){
		send_info = (Wifi_Send_Info_t *)wdata->data.pointer;
		printk("[IE] ssid %s\n", send_info->Ssid);
		printk("[IE] ch %d\n", send_info->channel);
		ch_filter = channel = send_info->channel;
		
		for(i=0; i<8; i++){
			printk("[IE] bssid "MACSTR"\n", MAC2STR(send_info->Bssid[i]));
		}
	}else{
		ch_filter = channel = 6; //固定信道
		not_filter = 1; //不过滤MAC 地址
		printk("[IE] const ch %d, not filter.\n", channel);
	}
	
	memcpy(&filter_mac[0][0], &send_info->Bssid[0][0], MAC_FILTER_NUM*ETH_ALEN);

	//test code
	//memcpy(&filter_mac[0][0], test_mac, ETH_ALEN);
	
	//clear recv buffer
	memset(&priv_recv_info[0], 0, sizeof(Wifi_Ap_Info_t)*MAC_FILTER_NUM);
	
	atbm_private_scan(priv, channel);
#endif
	return ret;
}
/*
*4,  SSTAR_RECV_MSG_CMD       接收命令
*	函数 ioctl(global->ioctl_sock, SSTAR_RECV_MSG_CMD, &Wifi_Recv_Info_t)
*	说明：接收广播命令，接收到广播信息后可以收到Wifi_Recv_Info_t结构体信息
*/
static int atbm_ioctl_ie_recv_msg(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	
	printk("[IE] atbm_ioctl_ie_recv_msg()\n");

	atbm_get_ap_info((void *)wdata->data.pointer);

	return ret;
}
int my_isdigit(char val)
{
	if('0'/*0x30*/ <= val <= '9'/*0x39*/)
		return 1;
	else
		return 0;
}
int my_atoi(const char *str)
{
	int ret=0,sign=1;
	printk("@@@atoi() 1\n");

	if(NULL == str);
		return -1;

	printk("@@@atoi() 2\n");

	//skip tab and space
	for(; *str==' '||*str=='\t'; str++);
	
	if(*str == '-')
		sign = -1;
	
	if(*str == '-' || *str == '+')
		str++;
	printk("@@@atoi() 3\n");
	while(my_isdigit(*str)){
		printk("@@@atoi() 0x%x\n", *str);
		ret = ret*10 + *str - '0';
		str++;
	}
	return sign*ret;
}

unsigned short str2channel(char *ptr)
{
	switch(*ptr){
		case 0x31: 
			if(*(ptr+1) == 0)
				return 1;
			else if(*(ptr+1) == 0x30)
				return 10;
			else if(*(ptr+1) == 0x31)
				return 11;
			else if(*(ptr+1) == 0x32)
				return 12;
			else if(*(ptr+1) == 0x33)
				return 13;
			else if(*(ptr+1) == 0x34)
				return 14;
		case 0x32: return 2;
		case 0x33: return 3;
		case 0x34: return 4;
		case 0x35: return 5;
		case 0x36: return 6;
		case 0x37: return 7;
		case 0x38: return 8;
		case 0x39: return 9;

		default:
			return 3;

	}
}

static int atbm_ioctl_ie_ipc_reset(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	
	printk("\n[IE] atbm_ioctl_ie_ipc_reset()\n\n");

	//clear private data
	memset(&atbm_priv_data, 0, sizeof(atbm_priv_data));

	//clear mac addr set
	memset(&filter_mac[0][0], 0, ETH_ALEN*MAC_FILTER_NUM);

	//clear recv buffer
	memset(&priv_recv_info[0], 0, sizeof(Wifi_Ap_Info_t)*MAC_FILTER_NUM);

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		ret = atbm_upload_beacon_private(priv);
		if(ret < 0){
			printk("[IE] reset: upload beacon private failed %d\n", ret);
		}

		//ret = atbm_upload_proberesp_private(priv);
		//if(ret < 0){
		//	printk("[IE] upload probe resp private failed %d\n", ret);
		//}
	}else{
		//not do anything
	}

	return ret;
}
static int atbm_ioctl_ie_test(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;

	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	//struct atbm_vif *priv = ABwifi_hwpriv_to_vifpriv(hw_priv,0);
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	Wifi_Send_Info_t send_info;
	char *ptr = NULL;
	unsigned short channel = 0;

	printk("\n[IE] atbm_ioctl_ie_test()\n\n");

	if(sdata->vif.type == NL80211_IFTYPE_STATION){
		printk("[IE] STA Test Start\n");
		not_filter = 1;
		ptr = wdata->data.pointer;

		frame_hexdump("Test", ptr, 10);
		
		//channel = my_atoi(ptr);
		//channel = 3;
		//channel = *(unsigned short *)ptr;
		//memcpy(&channel, ptr, sizeof(unsigned short));
		channel = str2channel(ptr);
		printk("[IE] channel is %d\n", channel);
		
		memset(&send_info, 0, sizeof(Wifi_Send_Info_t));
		
		send_info.channel = channel;

		memcpy(wdata->data.pointer, (char *)&send_info, sizeof(Wifi_Send_Info_t));
		wdata->data.length = sizeof(Wifi_Send_Info_t) + 1;
		
		atbm_ioctl_ie_send_msg(dev, info, (void *)wrqu, extra);
	}else{
		printk("[IE] AP Test Start\n");
	}

	return ret;
}
#endif
static int atbm_ioctl_set_hidden_ssid(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	int len = 0;
	char *ptr = NULL;
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	//struct ieee80211_local *local = sdata->local;
	//struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;


	printk("atbm_ioctl_set_hidden_ssid()\n\n");

	if(sdata->vif.type != NL80211_IFTYPE_STATION){
#ifdef HIDDEN_SSID
		ptr = wdata->data.pointer;
		len = wdata->data.length-1;

		if((len != 6/*strlen("enable")*/) && (len != 7/*strlen("disable")*/)){
			printk("error, invalide length %d\n", len);
			ret = -1;
			goto Error;
		}

		if((0 == memcmp(ptr, "enable", 6)) || 
			(0 == memcmp(ptr, "ENABLE", 6)) ||
			(0 == memcmp(ptr, "Enable", 6))){
			
			priv->hidden_ssid = 1;
		}else if((0 == memcmp(ptr, "disable", 7)) ||
					(0 == memcmp(ptr, "DISABLE", 7)) ||
					(0 == memcmp(ptr, "Disable", 7))){

			priv->hidden_ssid = 0;

		}else{
			printk("error, invalide cmd: %s\n", ptr);
			ret = -1;
			goto Error;
		}
		
		ret = atbm_upload_beacon_private(priv);
		if(ret < 0){
			printk("error, upload beacon private failed %d\n", ret);
			goto Error;
		}
#else
		printk("error, the hidden ssid func is not enabled !!!\n\n");
#endif
	}else{
		printk("warning, only AP mode support hidden ssid func, type(%d) \n", sdata->vif.type);
	}

Error:
	return ret;
}

static int atbm_ioctl_set_freq(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	int len = 0;
	char *ptr = NULL;
	char cmd[32];
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;


	printk("atbm_ioctl_set_freq()\n\n");

	ptr = wdata->data.pointer;
	len = wdata->data.length-1;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "set_freq %s ", ptr);
	printk("atbm: %s\n", cmd);
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, cmd, strlen(cmd), priv->if_id);
	if(ret < 0)
		printk("atbm: freq wsm write mib failed. \n");


	return ret;
}

static int atbm_ioctl_set_txpw(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	int len = 0;
	char *ptr = NULL;
	char cmd[32];

	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;


	printk("atbm_ioctl_set_txpw()\n\n");

	ptr = wdata->data.pointer;
	len = wdata->data.length-1;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "set_txpower %s ", ptr);
	printk("atbm: %s\n", cmd);
	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD, cmd, strlen(cmd), priv->if_id);
	if(ret < 0)
		printk("atbm: txpw wsm write mib failed. \n");


	return ret;
}

#ifndef ATBM_RATE

unsigned char char2Hex(const char chart)
{
	unsigned char ret;
	
	switch(chart){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ret = 0x0 + (chart - '0');
			break;

		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			ret = 0xA + (chart - 'a');
			break;
			
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			ret = 0xA + (chart - 'A');
			break;

		default:
			printk("char2Hex: error %c %d\n", chart, chart);
			ret = 0;
			
	}

	return ret;
}

/*
Func: str2mac
Param: 
	str->string format of MAC address
	i.e. 00:11:22:33:44:55
Return: 
	error -1
	OK 0
*/
int str2mac(char *dst_mac, char *src_str)
{
	int i;
	
	if(dst_mac == NULL || src_str == NULL)
		return -1;

	for(i=0; i<6; i++){
		dst_mac[i] = (char2Hex(src_str[i*3]) << 4) + (char2Hex(src_str[i*3 + 1]));
		printk("str2mac: %x\n", dst_mac[i]);
	}

	return 0;	
}
extern int atbm_find_link_id(struct atbm_vif *priv, const u8 *mac);
static int atbm_ioctl_get_rate(struct net_device *dev, struct iw_request_info *info, void *wrqu, char *extra)
{
	int ret = 0;
	char mac_addr[6];
//	int if_id = 0;
	int sta_id = 0;
	
	unsigned char *ptr = NULL;
	unsigned int rate_val = 0;
	
	union iwreq_data *wdata = (union iwreq_data *)wrqu;
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	struct atbm_vif *priv = (struct atbm_vif *)sdata->vif.drv_priv;
	
	ptr = wdata->data.pointer;

	printk("atbm_ioctl_get_rate() %s\n", ptr);

	//ap mode
	if(sdata->vif.type != NL80211_IFTYPE_STATION){
		//clear mac addr buffer
		memset(mac_addr, 0, 6);
		
		//convert mac string to mac hex format
		str2mac(mac_addr, ptr);
		
		//according to mac hex, find out sta link id
		sta_id = atbm_find_link_id(priv, mac_addr);

		printk("atbm_ioctl_get_rate() sta_id %d\n", sta_id);
		wsm_write_mib(hw_priv, WSM_MIB_ID_GET_RATE, &sta_id, 1, priv->if_id);
	}

	wsm_read_mib(hw_priv, WSM_MIB_ID_GET_RATE, &rate_val, sizeof(unsigned int), priv->if_id);

	//convert bits/s
	rate_val = rate_val/2;
	printk("rate: %d bits/s\n", rate_val);

	//memcpy(extra, (char *)&rate_val, sizeof(unsigned int));
	memcpy(ptr, (char *)&rate_val, sizeof(unsigned int));

	return ret;
}
#endif

#define DCXO_TRIM_REG 0x1610100c //bit 5:0
int DCXOCodeWrite(struct atbm_common *hw_priv,u8 data)
{
#ifndef SPI_BUS
	u32 uiRegData;

	hw_priv->sbus_ops->sbus_read_sync(hw_priv->sbus_priv,DCXO_TRIM_REG,&uiRegData,4);
	uiRegData &= ~0x40003F;

	uiRegData |= (((data&0x40)<<16)|(data&0x3f));
	
	hw_priv->sbus_ops->sbus_write_sync(hw_priv->sbus_priv,DCXO_TRIM_REG,&uiRegData,4);
#endif
	return 0;
}

u8 DCXOCodeRead(struct atbm_common *hw_priv)
{	
#ifndef SPI_BUS

	u32 uiRegData;
	u8 dcxo;
	u8 dcxo_hi,dcxo_low;


	hw_priv->sbus_ops->sbus_read_sync(hw_priv->sbus_priv,DCXO_TRIM_REG,&uiRegData,4);	
	dcxo_hi = (uiRegData>>22)&0x01;
	dcxo_low = uiRegData&0x3f;
	dcxo = (dcxo_hi << 6) + (dcxo_low&0x3f);
	
	return dcxo;
#else
	return 0;
#endif
}

//#include <math.h>

#define N_BIT_TO_SIGNED_32BIT(v,n)	(s32)(((v) & BIT(n-1))?((v)|0xffffffff<<n):(v))



void etf_rx_status_get(struct atbm_common *hw_priv)
{
	int i = 0;
	struct rxstatus rxs; 
	char *extra = NULL;
	struct atbm_vif *vif;
	
	if(!(extra = atbm_kmalloc(sizeof(struct rxstatus), GFP_KERNEL)))
		return;

	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_read_mib(hw_priv, WSM_MIB_ID_GET_ETF_RX_STATS,
						extra, sizeof(struct rxstatus), vif->if_id));
					break;
				}
			}
	memcpy(&rxs, extra, sizeof(struct rxstatus));
	
	atbm_kfree(extra);

	if(rxs.probcnt == 0)
		return;
	
	gRxs_s.evm				= rxs.evm/rxs.probcnt;
	gRxs_s.RxRSSI			= (s16)N_BIT_TO_SIGNED_32BIT(rxs.RSSI, 8)*4;
	return;

}


int getFreqoffsetHz(struct atbm_common *hw_priv, struct rxstatus_signed *rxs_s)
{
	struct rxstatus rxs; 
	int FreqOffsetHz;
	char *extra = NULL;
	int i = 0;
	long lcfo = 0;
	struct atbm_vif *vif;
	if(!(extra = atbm_kmalloc(16, GFP_KERNEL)))
	return -ENOMEM;

	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_read_mib(hw_priv, WSM_MIB_ID_GET_ETF_RX_STATS,
						extra, 16, vif->if_id));
					break;
				}
			}
	memcpy(&rxs, extra, sizeof(struct rxstatus));
	
	atbm_kfree(extra);
	
#if 0
	printk("Cfo:%d,RSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d, FreqOffsetHz:%d\n",
	rxs.Cfo,
	rxs.RSSI,
	rxs.evm,
	rxs.GainImb,
	rxs.PhaseImb,
	FreqOffsetHz
	);
#endif
	
	rxs_s->GainImb		= (s16)N_BIT_TO_SIGNED_32BIT(rxs.GainImb, 10);
	rxs_s->PhaseImb		= (s16)N_BIT_TO_SIGNED_32BIT(rxs.PhaseImb, 10);
	rxs_s->Cfo			= (s16)N_BIT_TO_SIGNED_32BIT(rxs.Cfo, 16);
	rxs_s->evm			= rxs.evm;
	rxs_s->RxRSSI			= (s8)N_BIT_TO_SIGNED_32BIT(rxs.RSSI, 8);

	lcfo = rxs_s->Cfo;
	FreqOffsetHz = (int)(((lcfo*12207)/10)*(-1));

	printk("Host Rx: Cfo:%d,RxRSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d, FreqOffsetHz:%d\n",
	rxs_s->Cfo,
	rxs_s->RxRSSI,
	rxs_s->evm,
	rxs_s->GainImb,
	rxs_s->PhaseImb,
	FreqOffsetHz
	);
	

	return FreqOffsetHz;
}

int _getMaxRssiInd(struct rxstatus_signed rxs_arr[], int cnt)
{
	int i=0;
	struct rxstatus_signed *rxsMax;
	int cntMax = 0;

	rxsMax = &rxs_arr[0];

	printk("_getMaxRssiInd()\n");
	
	for(i=1; i<cnt; i++)
	{
#if 0
		printk("#Cfo:%d,RxRSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d\n",
		rxs_arr[i].Cfo,
		rxs_arr[i].RxRSSI,
		rxs_arr[i].evm,
		rxs_arr[i].GainImb,
		rxs_arr[i].PhaseImb
		);
#endif
		if(rxs_arr[i].RxRSSI >= rxsMax->RxRSSI)
		{
			if(rxs_arr[i].evm <= rxsMax->evm)
			{
				rxsMax = &rxs_arr[i];
				cntMax = i;
			}
		}
	}

	printk("Host Rx: MaxRssi[%d]#Cfo:%d,RxRSSI:%d,evm:%d,GainImb:%d, PhaseImb:%d\n",
	cntMax,
	rxsMax->Cfo,
	rxsMax->RxRSSI,
	rxsMax->evm,
	rxsMax->GainImb,
	rxsMax->PhaseImb
	);

	return cntMax;
}

#define FREQ_CNT	(10)


int Test_FreqOffset(struct atbm_common *hw_priv, u32 *dcxo, int *pfreqErrorHz, struct rxstatus_signed *rxs_s, int channel)
{
	u8 CodeValue,CodeValuebak;
	u8 CodeStart,CodeEnd;
	int b_fail =1;
	int freqErrorHz = 0;
	int targetFreqOffset = TARGET_FREQOFFSET_HZ;
	struct atbm_vif *vif;

	char cmd[64];
	int i = 0;
	int itmp=0;
	int index=0;
	u8 ucDbgPrintOpenFlag = 1;
	struct rxstatus_signed rxs_arr[FREQ_CNT];
	int freqErrorHz_arr[FREQ_CNT];
	
	CodeValue = DCXOCodeRead(hw_priv);	
	DCXOCodeWrite(hw_priv,CodeValue);	

	if(ETF_bStartTx || ETF_bStartRx){
		printk("Error! already start_tx, please stop_tx first!\n");
		return b_fail;
	}


	//./iwpriv wlan0 fwdbg 1
	ucDbgPrintOpenFlag = 1;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}


	//start DUT Rx

	sprintf(cmd,  "monitor 1,%d,0",channel);
	
	printk("start DUT Rx CMD:%s\n", cmd);
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			ETF_bStartRx = 1;
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				cmd, strlen(cmd) + 10, vif->if_id));
			break;
		}
	}

	CodeStart = DCXO_CODE_MINI;
	CodeEnd = DCXO_CODE_MAX;

	msleep(50);
	printk("CodeValue default:%d\n",CodeValue);
	while(1)
	{
		CodeValuebak = CodeValue;
		for (itmp=0;itmp<FREQ_CNT;itmp++)
		{
			msleep(10);
			freqErrorHz_arr[itmp] = getFreqoffsetHz(hw_priv, &rxs_arr[itmp]);
		}
		index = _getMaxRssiInd(rxs_arr,FREQ_CNT);
		freqErrorHz = freqErrorHz_arr[index];
		memcpy(rxs_s, &rxs_arr[index], sizeof(struct rxstatus_signed));

		if (freqErrorHz >= targetFreqOffset)
		{
			CodeStart = CodeValue;
			CodeValue += (CodeEnd - CodeStart)/2;
			CodeStart = CodeValuebak;

			printk("freqErrorHz:%d >= targetFreqOffset%d,CodeValue%d CodeEnd[%d]. CodeStart[%d]\n",
				freqErrorHz,targetFreqOffset,	CodeValue,CodeEnd , CodeStart);
			
			DCXOCodeWrite(hw_priv,CodeValue);
			if (CodeValue >= 0xff)
			{
				break;
			}
		}
		else if ((int)freqErrorHz <= -targetFreqOffset)
		{
			CodeEnd = CodeValue;
			CodeValue -= (CodeEnd - CodeStart)/2;
			CodeEnd = CodeValuebak;
			
			printk("freqErrorHz:%d <= targetFreqOffset%d,CodeValue%d CodeEnd[%d]. CodeStart[%d]\n",
				freqErrorHz,targetFreqOffset,	CodeValue,CodeEnd , CodeStart);
			
			DCXOCodeWrite(hw_priv,CodeValue);
			if (CodeValue < 0x01)
			{
				break;
			}
			if (0x01 == CodeEnd)
			{
				break;
			}
		}
		else
		{
			printk("[PASS]freqErrorKHz[%d] CodeValue[%d]!\n",freqErrorHz/1000,CodeValue);
			b_fail = 0;
			*dcxo = CodeValue;
			*pfreqErrorHz = freqErrorHz;
			break;
		}

		if(CodeValue == CodeValuebak)
		{
			break;
		}

	}


	mutex_lock(&hw_priv->conf_mutex);
	memset(cmd,0,sizeof(cmd));
	memcpy(cmd, "monitor 0", sizeof(cmd));
	//stop DUT Rx
#if 1	
	printk("stop DUT Rx CMD:%s\n", cmd);
	atbm_for_each_vif(hw_priv,vif,i){
		if (vif != NULL)
		{
			ETF_bStartRx = 0;
			WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_FW_CMD,
				cmd, strlen(cmd) + 10, vif->if_id));
			break;
		}
	}
	mutex_unlock(&hw_priv->conf_mutex);

	//./iwpriv wlan0 fwdbg 1
	ucDbgPrintOpenFlag = 0;
	atbm_for_each_vif(hw_priv,vif,i){
				if (vif != NULL)
				{
					WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DBG_PRINT_TO_HOST,
						&ucDbgPrintOpenFlag, sizeof(ucDbgPrintOpenFlag), vif->if_id));
					break;
				}
			}

	
#endif
	return b_fail;
}


/*
return value:
	0: success.
	1:freq offset not ok yet,need next test
	2:dcxo cal max
	3.dcxo cal min
*/
int Test_FreqOffset_v2(struct atbm_common *hw_priv, u32 *dcxo, int *pfreqErrorHz)
{
	u8 CodeValue,CodeValuebak;
	int b_fail =1;
	int freqErrorHz = 0;
	int targetFreqOffset = TARGET_FREQOFFSET_HZ;
		
	CodeValue = DCXOCodeRead(hw_priv);	
	DCXOCodeWrite(hw_priv,CodeValue);	


	printk("CodeValue default:%d\n",CodeValue);

	
		CodeValuebak = CodeValue;

		freqErrorHz = gRxs_s.Cfo;

		if (freqErrorHz > targetFreqOffset)
		{
			CodeStart = CodeValue;
			CodeValue += (CodeEnd - CodeStart)/2;
			CodeStart = CodeValuebak;

			printk("freqErrorHz[%d] > targetFreqOffset[%d],CodeValue[%d] ,CodeStart[%d], CodeEnd[%d] . \n",
				freqErrorHz,targetFreqOffset,	CodeValue, CodeStart ,CodeEnd );
			
			DCXOCodeWrite(hw_priv,CodeValue);

			b_fail = 1;
			if (CodeValue >= 126)
			{
				b_fail = 2;
			}
			if (CodeValue >= 0xff)
			{
				b_fail = 2;
			}
		}
		else if ((int)freqErrorHz < -targetFreqOffset)
		{
			CodeEnd = CodeValue;
			CodeValue -= (CodeEnd - CodeStart)/2;
			CodeEnd = CodeValuebak;

			printk("freqErrorHz[%d] < targetFreqOffset[%d],CodeValue[%d] ,CodeStart[%d], CodeEnd[%d] . \n",
				freqErrorHz,targetFreqOffset,	CodeValue, CodeStart ,CodeEnd );
			DCXOCodeWrite(hw_priv,CodeValue);

			b_fail = 1;
			
			if (CodeValue <= 2)
			{
				b_fail = 3;
			}
			if (0x01 == CodeEnd)
			{
				b_fail = 3;
			}
		}
		else
		{
			printk("[dcxo PASS]freqErrorKHz[%d] CodeValue[%d]!\n",freqErrorHz/1000,CodeValue);
			b_fail = 0;
			*dcxo = CodeValue;
			*pfreqErrorHz = freqErrorHz;
		}

		
		return b_fail;

}


static int atbm_freqoffset_save_efuse(struct atbm_common *hw_priv,struct rxstatus_signed rxs_s,u32 dcxo)
{
	int ret = 0;
	int iResult=0;
	//struct atbm_vif *vif;
	struct efuse_headr efuse_d,efuse_bak;
	
	
	//u8 buff[512];

	memset(&efuse_d,0,sizeof(struct efuse_headr));
	memset(&efuse_bak,0,sizeof(struct efuse_headr));

	

	//tmp = DCXOCodeRead(hw_priv);printk("tmp %d\n"tmp);	
	if(ucWriteEfuseFlag)
	{
		printk("ucWriteEfuseFlag :%d\n",ucWriteEfuseFlag);
		wsm_get_efuse_data(hw_priv,(void *)&efuse_d,sizeof(struct efuse_headr));

		if(efuse_d.version == 0)
		{
			iResult = -3;
			goto FEEQ_ERR;
		}

		if(efuse_d.dcxo_trim == dcxo) // old dcxo equal new dcxo, no need to write efuse.
		{
			printk(" old dcxo equal new dcxo, no need to write efuse.\n");
			iResult = 0;
			goto FEEQ_ERR;
		}
		efuse_d.dcxo_trim = dcxo;
		/*
		*LMC_STATUS_CODE__EFUSE_VERSION_CHANGE	failed because efuse version change  
		*LMC_STATUS_CODE__EFUSE_FIRST_WRITE, 		failed because efuse by first write   
		*LMC_STATUS_CODE__EFUSE_PARSE_FAILED,		failed because efuse data wrong, cannot be parase
		*LMC_STATUS_CODE__EFUSE_FULL,				failed because efuse have be writen full
		*/
		ret = wsm_efuse_change_data_cmd(hw_priv, &efuse_d,0);
		if (ret == LMC_STATUS_CODE__EFUSE_FIRST_WRITE)
		{
			iResult = -3;
		}else if (ret == LMC_STATUS_CODE__EFUSE_PARSE_FAILED)
		{
			iResult = -4;
		}else if (ret == LMC_STATUS_CODE__EFUSE_FULL)
		{
			iResult = -5;
		}else if (ret == LMC_STATUS_CODE__EFUSE_VERSION_CHANGE)
		{
			iResult = -6;
		}else
		{
			iResult = 0;
		}
		frame_hexdump("efuse_d", (u8 *)&efuse_d, sizeof(struct efuse_headr));
		wsm_get_efuse_data(hw_priv,(void *)&efuse_bak, sizeof(struct efuse_headr));
		frame_hexdump("efuse_bak", (u8 *)&efuse_bak, sizeof(struct efuse_headr));
		
		if(memcmp((void *)&efuse_bak,(void *)&efuse_d, sizeof(struct efuse_headr)) !=0)
		{
			iResult = -2;
		}else
		{
			iResult = 0;
		}
		
	}

	
FEEQ_ERR:	
	
	/*sprintf(buff, "cfo:%d,evm:%d,gainImb:%d, phaseImb:%d,dcxo:%d,result:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change)",
	rxs_s.Cfo,
	rxs_s.evm,
	rxs_s.GainImb,
	rxs_s.PhaseImb,
	dcxo,
	iResult
	);*/

	//if((ret = copy_to_user(wrqu->data.pointer, buff, strlen(buff))) != 0){
	//	return -EINVAL;
	//}

	return iResult;
}


static int atbm_ioctl_freqoffset(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra)
{
	int i = 0;	
	int ret = 0;
	int iResult=0;
	
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = sdata->local;
	struct atbm_common *hw_priv=local->hw.priv;
	//struct atbm_vif *vif;
	struct efuse_headr efuse_d,efuse_bak;
	u32 dcxo;
	int freqErrorHz;
	int ucWriteEfuseFlag = 0;
	int channel = 0;
	char *SavaEfuse_p = NULL;
	
	u8 buff[512];
	struct rxstatus_signed rxs_s;

	memset(&rxs_s,0,sizeof(struct rxstatus));
	memset(&efuse_d,0,sizeof(struct efuse_headr));
	memset(&efuse_bak,0,sizeof(struct efuse_headr));

	if(wrqu->data.length <= 3){
		printk("need to input parameters\n");
		printk("e.g: ./iwpriv wlan0 start_rx 1,0\n");
		return -EINVAL;
	}

	if(!(extra = atbm_kmalloc(wrqu->data.length+1, GFP_KERNEL)))
		return -EINVAL;

	if((ret = copy_from_user(extra, wrqu->data.pointer, wrqu->data.length)) != 0){
		atbm_kfree(extra);
		return -EINVAL;
	}

	for(i=0;extra[i] != ',';i++){
		if(extra[i] == ','){
			break;
		}
		channel = channel * 10 +(extra[i] - 0x30);
	}

	for(i=0;i<wrqu->data.length;i++){
			if(extra[i] == ','){
				SavaEfuse_p = extra +i + 1;
				break;
			}	
		}

	if((NULL == SavaEfuse_p) || (SavaEfuse_p[0] == '\0')){
		printk("invalid SavaEfuse_p\n");
		printk("ucWriteEfuseFlag value: 1:save efuse,0:not save efuse\n");
		return -EINVAL;
	}

	for(i=0;SavaEfuse_p[i] != '\0';i++){
		if(1 == i){
		printk("invalid SavaEfuse_p1\n");
		printk("ucWriteEfuseFlag value: 1:save efuse,0:not save efuse\n");
			return 	-EINVAL;
		}
		ucWriteEfuseFlag = ucWriteEfuseFlag * 10 + (SavaEfuse_p[i] - 0x30);
	}

	printk("channel:%d ucWriteEfuseFlag:%d\n",channel, ucWriteEfuseFlag);

	
	if((ucWriteEfuseFlag != 0) && (ucWriteEfuseFlag != 1)){
		printk("invalid WriteEfuseFlag\n");
		return -EINVAL;
	}
	
	if(channel <= 0 || channel > 14){
			printk("invalid channel!\n");
			return -EINVAL;
		}

	for(i=0;i<wrqu->data.length;i++){
		if(extra[i] == ',')
			extra[i] =' ';
	}
		
	if(Test_FreqOffset(hw_priv,&dcxo, &freqErrorHz, &rxs_s, channel)){
		printk("Test_FreqOffset Error\n");
		iResult = -1;
		goto FEEQ_ERR;
	}
	//tmp = DCXOCodeRead(hw_priv);printk("tmp %d\n"tmp);	
	if(ucWriteEfuseFlag)
	{
		printk("ucWriteEfuseFlag :%d\n",ucWriteEfuseFlag);
		wsm_get_efuse_data(hw_priv,(void *)&efuse_d,sizeof(struct efuse_headr));

		if(efuse_d.version == 0)
		{
			iResult = -3;
			goto FEEQ_ERR;
		}
		efuse_d.dcxo_trim = dcxo;
		/*
		*LMC_STATUS_CODE__EFUSE_VERSION_CHANGE	failed because efuse version change  
		*LMC_STATUS_CODE__EFUSE_FIRST_WRITE, 		failed because efuse by first write   
		*LMC_STATUS_CODE__EFUSE_PARSE_FAILED,		failed because efuse data wrong, cannot be parase
		*LMC_STATUS_CODE__EFUSE_FULL,				failed because efuse have be writen full
		*/
		ret = wsm_efuse_change_data_cmd(hw_priv, &efuse_d,0);
		if (ret == LMC_STATUS_CODE__EFUSE_FIRST_WRITE)
		{
			iResult = -3;
		}else if (ret == LMC_STATUS_CODE__EFUSE_PARSE_FAILED)
		{
			iResult = -4;
		}else if (ret == LMC_STATUS_CODE__EFUSE_FULL)
		{
			iResult = -5;
		}else if (ret == LMC_STATUS_CODE__EFUSE_VERSION_CHANGE)
		{
			iResult = -6;
		}else
		{
			iResult = 0;
		}

		frame_hexdump("efuse_d", (u8 *)&efuse_d, sizeof(struct efuse_headr));
		wsm_get_efuse_data(hw_priv,(void *)&efuse_bak, sizeof(struct efuse_headr));
		frame_hexdump("efuse_bak", (u8 *)&efuse_bak, sizeof(struct efuse_headr));
		
		if(memcmp((void *)&efuse_bak,(void *)&efuse_d, sizeof(struct efuse_headr)) !=0)
		{
			iResult = -2;
		}else
		{
			iResult = 0;
		}
		
	}

	
FEEQ_ERR:	
	
	sprintf(buff, "cfo:%d,evm:%d,gainImb:%d, phaseImb:%d,dcxo:%d,result:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change)",
	rxs_s.Cfo,
	rxs_s.evm,
	rxs_s.GainImb,
	rxs_s.PhaseImb,
	dcxo,
	iResult
	);

	if((ret = copy_to_user(wrqu->data.pointer, buff, strlen(buff))) != 0){
		return -EINVAL;
	}

	return ret;
}


const iw_handler atbm_private_handler[]={
	[0] = (iw_handler)atbm_ioctl_setmac,
	[1] = (iw_handler)atbm_ioctl_smartconfig_start,	
	[2] = (iw_handler)atbm_ioctl_start_tx,
	[3] = (iw_handler)atbm_ioctl_stop_tx,
	[4] = (iw_handler)atbm_ioctl_start_rx,
	[5] = (iw_handler)atbm_ioctl_stop_rx,
	[6] = (iw_handler)atbm_ioctl_fwdbg,
	[7] = (iw_handler)atbm_ioctl_command_help,
	[8] = (iw_handler)atbm_ioctl_fwcmd,
	[9] = (iw_handler)atbm_ioctl_set_rate,
	[10] = (iw_handler)atbm_ioctl_smartconfig_stop,
	[11] = (iw_handler)atbm_ioctl_set_rts,
	[12] = (iw_handler)atbm_ioctl_ctl_gi,
	[13] = (iw_handler)atbm_ioctl_getmac,
	[14] = (iw_handler)atbm_ioctl_start_wol,
	[15] = (iw_handler)atbm_ioctl_get_rx_stats,
	[16] = (iw_handler)atbm_ioctl_freqoffset,
	[17] = (iw_handler)atbm_ioctl_smartconfig_start_v2,
	[18] = (iw_handler)atbm_ioctl_etf_result_get,
	[19] = (iw_handler)atbm_ioctl_smartconfig_stop_v2,
	#ifdef ATBM_PRIVATE_IE
	/*Private IE for Scan*/
	[20] = (iw_handler)atbm_ioctl_ie_insert_user_data,
	[21] = (iw_handler)atbm_ioctl_ie_get_user_data,
	[22] = (iw_handler)atbm_ioctl_ie_send_msg,
	[23] = (iw_handler)atbm_ioctl_ie_recv_msg,
	[24] = (iw_handler)atbm_ioctl_ie_test,
	
	[25] = (iw_handler)atbm_ioctl_set_hidden_ssid,
	[26] = (iw_handler)atbm_ioctl_set_freq,
	[27] = (iw_handler)atbm_ioctl_set_txpw,
	[28] = (iw_handler)atbm_ioctl_ie_ipc_reset,
#ifndef ATBM_RATE
	[29] = (iw_handler)atbm_ioctl_get_rate,
#endif
	#else
	[20] = (iw_handler)atbm_ioctl_set_hidden_ssid,
	[21] = (iw_handler)atbm_ioctl_set_freq,
	[22] = (iw_handler)atbm_ioctl_set_txpw,
#ifndef ATBM_RATE
	[23] = (iw_handler)atbm_ioctl_get_rate,
#endif
	#endif
};


void register_wext_common(struct ieee80211_local *local){

#ifdef CONFIG_WIRELESS_EXT
#ifdef CONFIG_CFG80211_WEXT
	if(local->hw.wiphy->wext)
	{
	        atbm_handlers_def.standard = local->hw.wiphy->wext->standard;
	        atbm_handlers_def.num_standard = local->hw.wiphy->wext->num_standard;
		#if WIRELESS_EXT >= 17
	        atbm_handlers_def.get_wireless_stats = local->hw.wiphy->wext->get_wireless_stats;
		#endif
#endif
		#ifdef CONFIG_WEXT_PRIV
			atbm_handlers_def.num_private = sizeof(atbm_private_handler)/sizeof(atbm_private_handler[0]) ;
			atbm_handlers_def.private = atbm_private_handler ;
			atbm_handlers_def.num_private_args = sizeof(atbm_privtab)/sizeof(atbm_privtab[0]);
			atbm_handlers_def.private_args = (struct iw_priv_args *)atbm_privtab;
		#endif
#ifdef CONFIG_CFG80211_WEXT
	}
#endif
#endif

}





/**************************************************************************
**
** NAME         LMC_FM_GetATBMIe
**
** PARAMETERS:  pElements  -> Pointer to the Ie list
**              Length     -> Size of the Ie List
**              
** RETURNS:     Pointer to element if found or 0 otherwise.
**
** DESCRIPTION  Searches for ATBM test element  from a given IE list.
** 
**************************************************************************/
u8* LMC_FM_GetATBMIe(u8 *pElements,u16 Length)
{
  u8     ATBMIeOui[3]   = ATBM_OUI	;
  
  struct ATBM_TEST_IE  *Atbm_Ie;
	//dump_mem(pElements,Length);

   if(Length > sizeof(struct ATBM_TEST_IE)){
		pElements += Length-sizeof(struct ATBM_TEST_IE);
		Atbm_Ie =(struct ATBM_TEST_IE  *) pElements;
		/*
		DBG_Printf("Atbm_Ie->oui_type %x,Atbm_Ie->oui %x %x,size %x\n",
			Atbm_Ie->oui_type,
			Atbm_Ie->oui[2],
			ATBMIeOui[2],
			sizeof(struct ATBM_TEST_IE));
		
		dump_mem(pElements,16);*/

		 if(pElements[0]== D11_WIFI_ELT_ID){
			 if((memcmp(Atbm_Ie->oui,ATBMIeOui,3)==0)&&
			 	(Atbm_Ie->oui_type== WIFI_ATBM_IE_OUI_TYPE) ){
				return pElements;
			}
		 }
   }

  return (u8 *)NULL  ;
}//end LMC_FM_GetP2PIe()

void etf_v2_scan_end(struct atbm_common *hw_priv, struct ieee80211_vif *vif )
{
	int result = 0;//(0:OK; -1:FreqOffset Error; -2:Write efuse Failed;-3:efuse not write;-4:rx fail)
	u32 dcxo = 0;
	int freqErrorHz;
	
	etf_rx_status_get(hw_priv);
	msleep(10);

	if(atbm_test_rx_cnt <= 5){
#if CONFIG_ATBM_PRODUCT_TEST_NO_UART
		if((Atbm_Test_Success == 1) || (Atbm_Test_Success == -1)){
			gRxs_s.valid = 1;	
			Atbm_Test_Success = 0;
			atbm_test_rx_cnt = 0;
			ETF_bStartTx = 0;
			return;
		}
#endif
		printk("etf rx data[%d] less than 5 packet\n",atbm_test_rx_cnt);
		gRxs_s.result = -7;		

		gRxs_s.dcxo = dcxo;
		gRxs_s.valid = 1;	
		atbm_test_rx_cnt = 0;
		ETF_bStartTx = 0;
		return;
	}
	
	printk("Average: Cfo:%d,TxRSSI:%d,RxRSSI:%d,evm:%d\n",	
	gRxs_s.Cfo,
	gRxs_s.TxRSSI,
	gRxs_s.RxRSSI,
	gRxs_s.evm
	);
	
	result = Test_FreqOffset_v2(hw_priv,&dcxo,&freqErrorHz);
	
	if(result == 1)
	{
		//start next scan
		printk("start next scan\n");

		//mutex_lock(&hw_priv->conf_mutex);
		//wsm_stop_tx(hw_priv);
		//mutex_unlock(&hw_priv->conf_mutex);

		msleep(100);
		
		wsm_start_tx_v2(hw_priv,vif);
	}
	else  if((result == 0)
#if CONFIG_ATBM_PRODUCT_TEST_NO_UART
		&& (gRxs_s.evm < 400) && (gRxs_s.TxRSSI >= -84)
#endif
				)  //etf dcxo success
	{

		printk("etf dcxo success \n");
		gRxs_s.result = atbm_freqoffset_save_efuse(hw_priv,gRxs_s,dcxo);

		gRxs_s.dcxo = dcxo;
		gRxs_s.valid = 1;
		//del_timer_sync(&hw_priv->etf_expire_timer);
#if CONFIG_ATBM_PRODUCT_TEST_NO_UART
		Atbm_Test_Success = 1;
		wsm_send_result(hw_priv,vif);
#endif
		
	}else
	{
		gRxs_s.result = -1;
		gRxs_s.dcxo = dcxo;
		gRxs_s.valid = 1;
		printk("etf dcxo Fail \n");
		//del_timer_sync(&hw_priv->etf_expire_timer);
#if CONFIG_ATBM_PRODUCT_TEST_NO_UART
		Atbm_Test_Success = -1;
		wsm_send_result(hw_priv,vif);
#endif

	}

#if CONFIG_ATBM_PRODUCT_TEST_NO_UART
	//Atbm_Test_Success = 0;
#endif
	atbm_test_rx_cnt = 0;
	ETF_bStartTx = 0;
}

void etf_v2_scan_rx(struct atbm_common *hw_priv,struct sk_buff *skb,u8 rssi )
{

	s32 Cfo;
	s32  RSSI;
	s32 tmp;
	s16 txevm;
	struct ATBM_TEST_IE  *Atbm_Ie = NULL;
	u8 *data = (u8 *)skb->data + offsetof(struct atbm_ieee80211_mgmt, u.probe_resp.variable);
	int len = skb->len - offsetof(struct atbm_ieee80211_mgmt, u.probe_resp.variable);
	Atbm_Ie = (struct ATBM_TEST_IE  *)LMC_FM_GetATBMIe(data,len);
	if(Atbm_Ie)
	{
		tmp				= Atbm_Ie->result[1];
		tmp				= (s32)N_BIT_TO_SIGNED_32BIT(tmp, 16);
		Cfo = (s32)(((tmp*12207)/10));
		 
		txevm				= (s16)N_BIT_TO_SIGNED_32BIT(Atbm_Ie->result[2], 16);
		RSSI			= (s16)N_BIT_TO_SIGNED_32BIT(Atbm_Ie->result[3], 10);

		if( RSSI < -100)
		{
			printk("[%d]: Cfo:%d,TxRSSI:%d, rx dump packet,throw......\n",
			atbm_test_rx_cnt,	
			Cfo,
			RSSI
			);
			return;
		}
		
		if(atbm_test_rx_cnt == 0)
		{		
			gRxs_s.Cfo = Cfo;
			//gRxs_s.evm = evm;
			gRxs_s.TxRSSI = RSSI;
		}else
		{

			gRxs_s.Cfo = (gRxs_s.Cfo*3 + Cfo )/4;
			//gRxs_s.evm = evm;
			gRxs_s.TxRSSI = RSSI;
			//gRxs_s.TxRSSI = (gRxs_s.TxRSSI*3*10 + RSSI*10 +5)/40;

		}
	
		printk("[%d]: Cfo1:%d, Cfo:%d,TxRSSI:%d,txevm:%d\n",
		atbm_test_rx_cnt,
		tmp,
		Cfo,
		RSSI,txevm
		);

		//printk("etf_v2_scan_rx %d,cnt %d,[0x%x,0x%x,0x%x,0x%x,0x%x]\n",Atbm_Ie->test_type,atbm_test_rx_cnt,
		//	Atbm_Ie->result[0],Atbm_Ie->result[1],Atbm_Ie->result[2],Atbm_Ie->result[3],Atbm_Ie->result[3]);

		atbm_test_rx_cnt++;
		
	}

}

int atbm_ioctl_etf_result_get(struct net_device *dev, struct iw_request_info *info, union iwreq_data  *wrqu, char *extra)
{
	int ret = 0;
	
	u8 buff[512];

	sprintf(buff, "%dcfo:%d,evm:%d,dcxo:%d,txrssi:%d,rxrssi:%d,result:%d (0:OK; -1:FreqOffset Error; -2:efuse hard error;"
		" -3:efuse no written; -4:efuse anaysis failed; -5:efuse full; -6:efuse version change; -7:rx null)",
	gRxs_s.valid,
	gRxs_s.Cfo,
	gRxs_s.evm,
	gRxs_s.dcxo,
	gRxs_s.TxRSSI,
	gRxs_s.RxRSSI,
	gRxs_s.result
	);
#if 0
	printk( "%dcfo:%d,evm:%d,dcxo:%d,txrssi:%d,rxrssi:%d,result:%d (0:OK; -1:FreqOffset Error; -2:Write efuse Failed;-3:efuse not write;-4:rx null)",
	gRxs_s.valid,
	gRxs_s.Cfo,
	gRxs_s.evm,
	gRxs_s.dcxo,
	gRxs_s.TxRSSI,
	gRxs_s.RxRSSI,
	gRxs_s.result
	);
#endif
	if((ret = copy_to_user(wrqu->data.pointer, buff, strlen(buff))) != 0){
		return -EINVAL;
	}

	return ret;
}

void atbm_etf_test_expire_timer(unsigned long arg)
{
	//printk("++ETF Test time out!++\n");
	//Atbm_Test_Success = 3;
}


#endif //CONFIG_WIRELESS_EXT



/*
start return  1
stop return 0
*/
int ETF_Test_is_Start(void){
	if(ETF_bStartTx || ETF_bStartRx)
		return 1;
	return 0;
}

void ETF_Test_Init(void){
	ETF_bStartTx = 0;
	ETF_bStartRx = 0;
}

