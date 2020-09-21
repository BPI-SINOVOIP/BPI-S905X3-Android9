
#ifndef _dhd_config_
#define _dhd_config_

#include <bcmdevs.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <wlioctl.h>
#include <proto/802.11.h>

#define FW_PATH_AUTO_SELECT 1
extern char firmware_path[MOD_PARAM_PATHLEN];

/* channel list */
typedef struct wl_channel_list {
	/* in - # of channels, out - # of entries */
	uint32 count;
	/* variable length channel list */
	uint32 channel[WL_NUMCHANNELS];
} wl_channel_list_t;

typedef struct wmes_param {
	int aifsn[AC_COUNT];
	int cwmin[AC_COUNT];
	int cwmax[AC_COUNT];
} wme_param_t;

#ifdef PKT_FILTER_SUPPORT
#define DHD_CONF_FILTER_MAX	8
/* filter list */
#define PKT_FILTER_LEN 300
typedef struct conf_pkt_filter_add {
	/* in - # of channels, out - # of entries */
	uint32 count;
	/* variable length filter list */
	char filter[DHD_CONF_FILTER_MAX][PKT_FILTER_LEN];
} conf_pkt_filter_add_t;

/* pkt_filter_del list */
typedef struct conf_pkt_filter_del {
	/* in - # of channels, out - # of entries */
	uint32 count;
	/* variable length filter list */
	uint32 id[DHD_CONF_FILTER_MAX];
} conf_pkt_filter_del_t;
#endif

#define CONFIG_COUNTRY_LIST_SIZE 100
/* country list */
typedef struct conf_country_list {
	uint32 count;
	wl_country_t cspec[CONFIG_COUNTRY_LIST_SIZE];
} conf_country_list_t;

typedef struct dhd_conf {
	uint	chip;			/* chip number */
	uint	chiprev;		/* chip revision */
	conf_country_list_t country_list; /* Country list */
	int band;			/* Band, b:2.4G only, otherwise for auto */
	int mimo_bw_cap;			/* Bandwidth, 0:HT20ALL, 1: HT40ALL, 2:HT20IN2G_HT40PIN5G */
	wl_country_t cspec;		/* Country */
	wl_channel_list_t channels;	/* Support channels */
	uint roam_off;		/* Roaming, 0:enable, 1:disable */
	uint roam_off_suspend;		/* Roaming in suspend, 0:enable, 1:disable */
	int roam_trigger[2];		/* The RSSI threshold to trigger roaming */
	int roam_scan_period[2];	/* Roaming scan period */
	int roam_delta[2];			/* Roaming candidate qualification delta */
	int fullroamperiod;			/* Full Roaming period */
	uint keep_alive_period;		/* The perioid in ms to send keep alive packet */
	int force_wme_ac;
	wme_param_t wme;	/* WME parameters */
	int stbc;			/* STBC for Tx/Rx */
#ifdef PKT_FILTER_SUPPORT
	conf_pkt_filter_add_t pkt_filter_add;		/* Packet filter add */
	conf_pkt_filter_del_t pkt_filter_del;		/* Packet filter add */
#endif
	int srl;	/* short retry limit */
	int lrl;	/* long retry limit */
	uint bcn_timeout;	/* beacon timeout */
	int spect;
	int txbf;
	int lpc;
	int ampdu_ba_wsize;
	int dpc_cpucore;
	int frameburst;
	bool deepsleep;
	int pm;
	int pktprio8021x;
} dhd_conf_t;

void dhd_conf_set_conf_path_by_fw_path(dhd_pub_t *dhd, char *conf_path, char *fw_path);
int dhd_conf_set_fw_int_cmd(dhd_pub_t *dhd, char *name, uint cmd, int val, int def, bool down);
int dhd_conf_set_fw_string_cmd(dhd_pub_t *dhd, char *cmd, int val, int def, bool down);
uint dhd_conf_get_band(dhd_pub_t *dhd);
int dhd_conf_set_country(dhd_pub_t *dhd);
int dhd_conf_get_country(dhd_pub_t *dhd, wl_country_t *cspec);
int dhd_conf_get_country_from_config(dhd_pub_t *dhd, wl_country_t *cspec);
int dhd_conf_fix_country(dhd_pub_t *dhd);
bool dhd_conf_match_channel(dhd_pub_t *dhd, uint32 channel);
int dhd_conf_set_roam(dhd_pub_t *dhd);
void dhd_conf_get_wme(dhd_pub_t *dhd, edcf_acparam_t *acp);
void dhd_conf_set_wme(dhd_pub_t *dhd);
void dhd_conf_add_pkt_filter(dhd_pub_t *dhd);
bool dhd_conf_del_pkt_filter(dhd_pub_t *dhd, uint32 id);
void dhd_conf_discard_pkt_filter(dhd_pub_t *dhd);
int dhd_conf_read_config(dhd_pub_t *dhd, char *conf_path);
int dhd_conf_set_chiprev(dhd_pub_t *dhd, uint chip, uint chiprev);
uint dhd_conf_get_chiprev(void *context);
int dhd_conf_get_pm(dhd_pub_t *dhd);
int dhd_conf_preinit(dhd_pub_t *dhd);
int dhd_conf_reset(dhd_pub_t *dhd);
int dhd_conf_attach(dhd_pub_t *dhd);
void dhd_conf_detach(dhd_pub_t *dhd);
void *dhd_get_pub(struct net_device *dev);

#endif /* _dhd_config_ */
