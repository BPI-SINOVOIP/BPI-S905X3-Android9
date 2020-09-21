/*
 * Common function shared by Linux WEXT, cfg80211 and p2p drivers
 *
 * Copyright (C) 1999-2015, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: wldev_common.c 537724 2015-02-27 10:26:14Z $
 */

#include <osl.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>

#include <wldev_common.h>
#include <bcmutils.h>
#include <dhd_config.h>

#if defined(IL_BIGENDIAN)
#include <bcmendian.h>
#define htod32(i) (bcmswap32(i))
#define htod16(i) (bcmswap16(i))
#define dtoh32(i) (bcmswap32(i))
#define dtoh16(i) (bcmswap16(i))
#define htodchanspec(i) htod16(i)
#define dtohchanspec(i) dtoh16(i)
#else
#define htod32(i) (i)
#define htod16(i) (i)
#define dtoh32(i) (i)
#define dtoh16(i) (i)
#define htodchanspec(i) (i)
#define dtohchanspec(i) (i)
#endif

#define	WLDEV_ERROR(args)						\
	do {										\
		printk(KERN_ERR "WLDEV-ERROR) %s : ", __func__);	\
		printk args;							\
	} while (0)

extern int dhd_ioctl_entry_local(struct net_device *net, wl_ioctl_t *ioc, int cmd);

s32 wldev_ioctl(
	struct net_device *dev, u32 cmd, void *arg, u32 len, u32 set)
{
	s32 ret = 0;
	struct wl_ioctl ioc;


	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = cmd;
	ioc.buf = arg;
	ioc.len = len;
	ioc.set = set;

	ret = dhd_ioctl_entry_local(dev, &ioc, cmd);

	return ret;
}

/* Format a iovar buffer, not bsscfg indexed. The bsscfg index will be
 * taken care of in dhd_ioctl_entry. Internal use only, not exposed to
 * wl_iw, wl_cfg80211 and wl_cfgp2p
 */
static s32 wldev_mkiovar(
	s8 *iovar_name, s8 *param, s32 paramlen,
	s8 *iovar_buf, u32 buflen)
{
	s32 iolen = 0;

	iolen = bcm_mkiovar(iovar_name, param, paramlen, iovar_buf, buflen);
	return iolen;
}

s32 wldev_iovar_getbuf(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, struct mutex* buf_sync)
{
	s32 ret = 0;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}
	wldev_mkiovar(iovar_name, param, paramlen, buf, buflen);
	ret = wldev_ioctl(dev, WLC_GET_VAR, buf, buflen, FALSE);
	if (buf_sync)
		mutex_unlock(buf_sync);
	return ret;
}


s32 wldev_iovar_setbuf(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, struct mutex* buf_sync)
{
	s32 ret = 0;
	s32 iovar_len;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}
	iovar_len = wldev_mkiovar(iovar_name, param, paramlen, buf, buflen);
	if (iovar_len > 0)
		ret = wldev_ioctl(dev, WLC_SET_VAR, buf, iovar_len, TRUE);
	else
		ret = BCME_BUFTOOSHORT;

	if (buf_sync)
		mutex_unlock(buf_sync);
	return ret;
}

s32 wldev_iovar_setint(
	struct net_device *dev, s8 *iovar, s32 val)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];

	val = htod32(val);
	memset(iovar_buf, 0, sizeof(iovar_buf));
	return wldev_iovar_setbuf(dev, iovar, &val, sizeof(val), iovar_buf,
		sizeof(iovar_buf), NULL);
}


s32 wldev_iovar_getint(
	struct net_device *dev, s8 *iovar, s32 *pval)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	s32 err;

	memset(iovar_buf, 0, sizeof(iovar_buf));
	err = wldev_iovar_getbuf(dev, iovar, pval, sizeof(*pval), iovar_buf,
		sizeof(iovar_buf), NULL);
	if (err == 0)
	{
		memcpy(pval, iovar_buf, sizeof(*pval));
		*pval = dtoh32(*pval);
	}
	return err;
}

/** Format a bsscfg indexed iovar buffer. The bsscfg index will be
 *  taken care of in dhd_ioctl_entry. Internal use only, not exposed to
 *  wl_iw, wl_cfg80211 and wl_cfgp2p
 */
s32 wldev_mkiovar_bsscfg(
	const s8 *iovar_name, s8 *param, s32 paramlen,
	s8 *iovar_buf, s32 buflen, s32 bssidx)
{
	const s8 *prefix = "bsscfg:";
	s8 *p;
	u32 prefixlen;
	u32 namelen;
	u32 iolen;

	if (bssidx == 0) {
		return wldev_mkiovar((s8*)iovar_name, (s8 *)param, paramlen,
			(s8 *) iovar_buf, buflen);
	}

	prefixlen = (u32) strlen(prefix); /* lengh of bsscfg prefix */
	namelen = (u32) strlen(iovar_name) + 1; /* lengh of iovar  name + null */
	iolen = prefixlen + namelen + sizeof(u32) + paramlen;

	if (buflen < 0 || iolen > (u32)buflen)
	{
		WLDEV_ERROR(("%s: buffer is too short\n", __FUNCTION__));
		return BCME_BUFTOOSHORT;
	}

	p = (s8 *)iovar_buf;

	/* copy prefix, no null */
	memcpy(p, prefix, prefixlen);
	p += prefixlen;

	/* copy iovar name including null */
	memcpy(p, iovar_name, namelen);
	p += namelen;

	/* bss config index as first param */
	bssidx = htod32(bssidx);
	memcpy(p, &bssidx, sizeof(u32));
	p += sizeof(u32);

	/* parameter buffer follows */
	if (paramlen)
		memcpy(p, param, paramlen);

	return iolen;

}

s32 wldev_iovar_getbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx, struct mutex* buf_sync)
{
	s32 ret = 0;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}

	wldev_mkiovar_bsscfg(iovar_name, param, paramlen, buf, buflen, bsscfg_idx);
	ret = wldev_ioctl(dev, WLC_GET_VAR, buf, buflen, FALSE);
	if (buf_sync) {
		mutex_unlock(buf_sync);
	}
	return ret;

}

s32 wldev_iovar_setbuf_bsscfg(
	struct net_device *dev, s8 *iovar_name,
	void *param, s32 paramlen, void *buf, s32 buflen, s32 bsscfg_idx, struct mutex* buf_sync)
{
	s32 ret = 0;
	s32 iovar_len;
	if (buf_sync) {
		mutex_lock(buf_sync);
	}
	iovar_len = wldev_mkiovar_bsscfg(iovar_name, param, paramlen, buf, buflen, bsscfg_idx);
	if (iovar_len > 0)
		ret = wldev_ioctl(dev, WLC_SET_VAR, buf, iovar_len, TRUE);
	else {
		ret = BCME_BUFTOOSHORT;
	}

	if (buf_sync) {
		mutex_unlock(buf_sync);
	}
	return ret;
}

s32 wldev_iovar_setint_bsscfg(
	struct net_device *dev, s8 *iovar, s32 val, s32 bssidx)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];

	val = htod32(val);
	memset(iovar_buf, 0, sizeof(iovar_buf));
	return wldev_iovar_setbuf_bsscfg(dev, iovar, &val, sizeof(val), iovar_buf,
		sizeof(iovar_buf), bssidx, NULL);
}


s32 wldev_iovar_getint_bsscfg(
	struct net_device *dev, s8 *iovar, s32 *pval, s32 bssidx)
{
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	s32 err;

	memset(iovar_buf, 0, sizeof(iovar_buf));
	err = wldev_iovar_getbuf_bsscfg(dev, iovar, pval, sizeof(*pval), iovar_buf,
		sizeof(iovar_buf), bssidx, NULL);
	if (err == 0)
	{
		memcpy(pval, iovar_buf, sizeof(*pval));
		*pval = dtoh32(*pval);
	}
	return err;
}

int wldev_get_link_speed(
	struct net_device *dev, int *plink_speed)
{
	int error;

	if (!plink_speed)
		return -ENOMEM;
	error = wldev_ioctl(dev, WLC_GET_RATE, plink_speed, sizeof(int), 0);
	if (unlikely(error))
		return error;

	/* Convert internal 500Kbps to Kbps */
	*plink_speed *= 500;
	return error;
}

int wldev_get_rssi(
	struct net_device *dev, scb_val_t *scb_val)
{
	int error;

	if (!scb_val)
		return -ENOMEM;

	error = wldev_ioctl(dev, WLC_GET_RSSI, scb_val, sizeof(scb_val_t), 0);
	if (unlikely(error))
		return error;

	return error;
}

int wldev_get_ssid(
	struct net_device *dev, wlc_ssid_t *pssid)
{
	int error;

	if (!pssid)
		return -ENOMEM;
	error = wldev_ioctl(dev, WLC_GET_SSID, pssid, sizeof(wlc_ssid_t), 0);
	if (unlikely(error))
		return error;
	pssid->SSID_len = dtoh32(pssid->SSID_len);
	return error;
}

int wldev_get_band(
	struct net_device *dev, uint *pband)
{
	int error;

	error = wldev_ioctl(dev, WLC_GET_BAND, pband, sizeof(uint), 0);
	return error;
}

int wldev_set_band(
	struct net_device *dev, uint band)
{
	int error = -1;

	if ((band == WLC_BAND_AUTO) || (band == WLC_BAND_5G) || (band == WLC_BAND_2G)) {
		error = wldev_ioctl(dev, WLC_SET_BAND, &band, sizeof(band), true);
		if (!error)
			dhd_bus_band_set(dev, band);
	}
	return error;
}

int wldev_get_datarate(struct net_device *dev, int *datarate)
{
	int error = 0;

	error = wldev_ioctl(dev, WLC_GET_RATE, datarate, sizeof(int), false);
	if (error) {
		return -1;
	} else {
		*datarate = dtoh32(*datarate);
	}

	return error;
}

extern chanspec_t
wl_chspec_driver_to_host(chanspec_t chanspec);
#define WL_EXTRA_BUF_MAX 2048
int wldev_get_mode(
	struct net_device *dev, uint8 *cap)
{
	int error = 0;
	int chanspec = 0;
	uint16 band = 0;
	uint16 bandwidth = 0;
	wl_bss_info_t *bss = NULL;
	char* buf = NULL;

	buf =  kmalloc(WL_EXTRA_BUF_MAX, GFP_KERNEL);
	if (!buf) {
		WLDEV_ERROR(("%s:NOMEM\n", __FUNCTION__));
		return -ENOMEM;
	}

	*(u32*) buf = htod32(WL_EXTRA_BUF_MAX);
	error = wldev_ioctl(dev, WLC_GET_BSS_INFO, (void*)buf, WL_EXTRA_BUF_MAX, false);
	if (error) {
		WLDEV_ERROR(("%s:failed:%d\n", __FUNCTION__, error));
		kfree(buf);
		buf = NULL;
		return error;
	}
	bss = (struct  wl_bss_info *)(buf + 4);
	chanspec = wl_chspec_driver_to_host(bss->chanspec);

	band = chanspec & WL_CHANSPEC_BAND_MASK;
	bandwidth = chanspec & WL_CHANSPEC_BW_MASK;

	if (band == WL_CHANSPEC_BAND_2G) {
		if (bss->n_cap)
			strcpy(cap, "n");
		else
			strcpy(cap, "bg");
	} else if (band == WL_CHANSPEC_BAND_5G) {
		if (bandwidth == WL_CHANSPEC_BW_80)
			strcpy(cap, "ac");
		else if ((bandwidth == WL_CHANSPEC_BW_40) || (bandwidth == WL_CHANSPEC_BW_20)) {
			if ((bss->nbss_cap & 0xf00) && (bss->n_cap))
				strcpy(cap, "n|ac");
			else if (bss->n_cap)
				strcpy(cap, "n");
			else if (bss->vht_cap)
				strcpy(cap, "ac");
			else
				strcpy(cap, "a");
		} else {
			WLDEV_ERROR(("%s:Mode get failed\n", __FUNCTION__));
			error = BCME_ERROR;
		}

	}
	kfree(buf);
	buf = NULL;
	return error;
}
int wldev_set_country(
	struct net_device *dev, char *country_code, bool notify, bool user_enforced)
{
	int error = -1;
	wl_country_t cspec = {{0}, 0, {0}};
	scb_val_t scbval;
	char smbuf[WLC_IOCTL_SMLEN];

	if (!country_code)
		return error;

	bzero(&scbval, sizeof(scb_val_t));
	error = wldev_iovar_getbuf(dev, "country", NULL, 0, &cspec, sizeof(cspec), NULL);
	if (error < 0) {
		WLDEV_ERROR(("%s: get country failed = %d\n", __FUNCTION__, error));
		return error;
	}

	if ((error < 0) ||
	    (strncmp(country_code, cspec.country_abbrev, WLC_CNTRY_BUF_SZ) != 0)) {

		if (user_enforced) {
			bzero(&scbval, sizeof(scb_val_t));
			error = wldev_ioctl(dev, WLC_DISASSOC, &scbval, sizeof(scb_val_t), true);
			if (error < 0) {
				WLDEV_ERROR(("%s: set country failed due to Disassoc error %d\n",
					__FUNCTION__, error));
				return error;
			}
		}

		cspec.rev = -1;
		memcpy(cspec.country_abbrev, country_code, WLC_CNTRY_BUF_SZ);
		memcpy(cspec.ccode, country_code, WLC_CNTRY_BUF_SZ);
		error = dhd_conf_get_country_from_config(dhd_get_pub(dev), &cspec);
		if (error)
			dhd_get_customized_country_code(dev, (char *)&cspec.country_abbrev, &cspec);
		error = wldev_iovar_setbuf(dev, "country", &cspec, sizeof(cspec),
			smbuf, sizeof(smbuf), NULL);
		if (error < 0) {
			WLDEV_ERROR(("%s: set country for %s as %s rev %d failed\n",
				__FUNCTION__, country_code, cspec.ccode, cspec.rev));
			return error;
		}
		dhd_conf_fix_country(dhd_get_pub(dev));
		dhd_conf_get_country(dhd_get_pub(dev), &cspec);
		dhd_bus_country_set(dev, &cspec, notify);
		printf("%s: set country for %s as %s rev %d\n",
			__FUNCTION__, country_code, cspec.ccode, cspec.rev);
	}
	return 0;
}

#if defined(CUSTOM_PLATFORM_NV_TEGRA)
/* tuning performance for miracast */
int wldev_miracast_tuning(
	struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;
	int ampdu_mpdu;
	int roam_off;
#ifdef VSDB_BW_ALLOCATE_ENABLE
	int mchan_algo;
	int mchan_bw;
#endif /* VSDB_BW_ALLOCATE_ENABLE */

	if (sscanf(command, "%*s %d", &mode) != 1) {
		WLDEV_ERROR(("Failed to get mode\n"));
		return -1;
	}

	WLDEV_ERROR(("mode: %d\n", mode));

	if (mode == 0) {
		/* Normal mode: restore everything to default */
		ampdu_mpdu = -1;	/* FW default */
#if defined(ROAM_ENABLE)
		roam_off = 0;	/* roam enable */
#elif defined(DISABLE_BUILTIN_ROAM)
		roam_off = 1;	/* roam disable */
#endif
#ifdef VSDB_BW_ALLOCATE_ENABLE
		mchan_algo = 0;	/* Default */
		mchan_bw = 50;	/* 50:50 */
#endif /* VSDB_BW_ALLOCATE_ENABLE */
	}
	else if (mode == 1) {
		/* Miracast source mode */
		ampdu_mpdu = 8;	/* for tx latency */
#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
		roam_off = 1; /* roam disable */
#endif
#ifdef VSDB_BW_ALLOCATE_ENABLE
		mchan_algo = 1;	/* BW based */
		mchan_bw = 25;	/* 25:75 */
#endif /* VSDB_BW_ALLOCATE_ENABLE */
	}
	else if (mode == 2) {
		/* Miracast sink/PC Gaming mode */
		ampdu_mpdu = -1;	/* FW default */
#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
		roam_off = 1; /* roam disable */
#endif
#ifdef VSDB_BW_ALLOCATE_ENABLE
		mchan_algo = 0;	/* Default */
		mchan_bw = 50;	/* 50:50 */
#endif /* VSDB_BW_ALLOCATE_ENABLE */
	}
	else {
		WLDEV_ERROR(("Unknown mode: %d\n", mode));
		return -1;
	}

	/* Update ampdu_mpdu */
	error = wldev_iovar_setint(dev, "ampdu_mpdu", ampdu_mpdu);
	if (error) {
		WLDEV_ERROR(("Failed to set ampdu_mpdu: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}

#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
	error = wldev_iovar_setint(dev, "roam_off", roam_off);
	if (error) {
		WLDEV_ERROR(("Failed to set roam_off: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}
#endif /* ROAM_ENABLE || DISABLE_BUILTIN_ROAM */

#ifdef VSDB_BW_ALLOCATE_ENABLE
	error = wldev_iovar_setint(dev, "mchan_algo", mchan_algo);
	if (error) {
		WLDEV_ERROR(("Failed to set mchan_algo: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}

	error = wldev_iovar_setint(dev, "mchan_bw", mchan_bw);
	if (error) {
		WLDEV_ERROR(("Failed to set mchan_bw: mode:%d, error:%d\n",
			mode, error));
		return -1;
	}
#endif /* VSDB_BW_ALLOCATE_ENABLE */

	return error;
}

int wldev_get_rx_rate_stats(
	struct net_device *dev, char *command, int total_len)
{
	wl_scb_rx_rate_stats_t *rstats;
	struct ether_addr ea;
	char smbuf[WLC_IOCTL_SMLEN];
	char eabuf[18] = {0, };
	int bytes_written = 0;
	int error;

	memcpy(eabuf, command+strlen("RXRATESTATS")+1, 17);

	if (!bcm_ether_atoe(eabuf, &ea)) {
		WLDEV_ERROR(("Invalid MAC Address\n"));
		return -1;
	}

	error = wldev_iovar_getbuf(dev, "rx_rate_stats",
		&ea, ETHER_ADDR_LEN, smbuf, sizeof(smbuf), NULL);
	if (error < 0) {
		WLDEV_ERROR(("get rx_rate_stats failed = %d\n", error));
		return -1;
	}

	rstats = (wl_scb_rx_rate_stats_t *)smbuf;
	bytes_written = sprintf(command, "1/%d/%d,",
		dtoh32(rstats->rx1mbps[0]), dtoh32(rstats->rx1mbps[1]));
	bytes_written += sprintf(command+bytes_written, "2/%d/%d,",
		dtoh32(rstats->rx2mbps[0]), dtoh32(rstats->rx2mbps[1]));
	bytes_written += sprintf(command+bytes_written, "5.5/%d/%d,",
		dtoh32(rstats->rx5mbps5[0]), dtoh32(rstats->rx5mbps5[1]));
	bytes_written += sprintf(command+bytes_written, "6/%d/%d,",
		dtoh32(rstats->rx6mbps[0]), dtoh32(rstats->rx6mbps[1]));
	bytes_written += sprintf(command+bytes_written, "9/%d/%d,",
		dtoh32(rstats->rx9mbps[0]), dtoh32(rstats->rx9mbps[1]));
	bytes_written += sprintf(command+bytes_written, "11/%d/%d,",
		dtoh32(rstats->rx11mbps[0]), dtoh32(rstats->rx11mbps[1]));
	bytes_written += sprintf(command+bytes_written, "12/%d/%d,",
		dtoh32(rstats->rx12mbps[0]), dtoh32(rstats->rx12mbps[1]));
	bytes_written += sprintf(command+bytes_written, "18/%d/%d,",
		dtoh32(rstats->rx18mbps[0]), dtoh32(rstats->rx18mbps[1]));
	bytes_written += sprintf(command+bytes_written, "24/%d/%d,",
		dtoh32(rstats->rx24mbps[0]), dtoh32(rstats->rx24mbps[1]));
	bytes_written += sprintf(command+bytes_written, "36/%d/%d,",
		dtoh32(rstats->rx36mbps[0]), dtoh32(rstats->rx36mbps[1]));
	bytes_written += sprintf(command+bytes_written, "48/%d/%d,",
		dtoh32(rstats->rx48mbps[0]), dtoh32(rstats->rx48mbps[1]));
	bytes_written += sprintf(command+bytes_written, "54/%d/%d",
		dtoh32(rstats->rx54mbps[0]), dtoh32(rstats->rx54mbps[1]));

	return bytes_written;
}

int wldev_get_assoc_resp_ie(
	struct net_device *dev, char *command, int total_len)
{
	wl_assoc_info_t *assoc_info;
	char smbuf[WLC_IOCTL_SMLEN];
	char bssid[6], null_bssid[6];
	int resp_ies_len = 0;
	int bytes_written = 0;
	int error, i;

	bzero(bssid, 6);
	bzero(null_bssid, 6);

	/* Check Association */
	error = wldev_ioctl(dev, WLC_GET_BSSID, &bssid, sizeof(bssid), 0);
	if (error == BCME_NOTASSOCIATED) {
		/* Not associated */
		bytes_written += snprintf(&command[bytes_written], total_len, "NA");
		goto done;
	}
	else if (error < 0) {
		WLDEV_ERROR(("WLC_GET_BSSID failed = %d\n", error));
		return -1;
	}
	else if (memcmp(bssid, null_bssid, ETHER_ADDR_LEN) == 0) {
		/*  Zero BSSID: Not associated */
		bytes_written += snprintf(&command[bytes_written], total_len, "NA");
		goto done;
	}

	/* Get assoc_info */
	bzero(smbuf, sizeof(smbuf));
	error = wldev_iovar_getbuf(dev, "assoc_info", NULL, 0, smbuf, sizeof(smbuf), NULL);
	if (error < 0) {
		WLDEV_ERROR(("get assoc_info failed = %d\n", error));
		return -1;
	}

	assoc_info = (wl_assoc_info_t *)smbuf;
	resp_ies_len = dtoh32(assoc_info->resp_len) - sizeof(struct dot11_assoc_resp);

	/* Retrieve assoc resp IEs */
	if (resp_ies_len) {
		error = wldev_iovar_getbuf(dev, "assoc_resp_ies", NULL, 0, smbuf, sizeof(smbuf),
			NULL);
		if (error < 0) {
			WLDEV_ERROR(("get assoc_resp_ies failed = %d\n", error));
			return -1;
		}

		/* Length */
		bytes_written += snprintf(&command[bytes_written], total_len, "%d,", resp_ies_len);

		/* IEs */
		if ((total_len - bytes_written) > resp_ies_len) {
			for (i = 0; i < resp_ies_len; i++) {
				bytes_written += sprintf(&command[bytes_written], "%02x", smbuf[i]);
			}
		} else {
			WLDEV_ERROR(("Not enough buffer\n"));
			return -1;
		}
	} else {
		WLDEV_ERROR(("Zero Length assoc resp ies = %d\n", resp_ies_len));
		return -1;
	}

done:

	return bytes_written;
}
#endif /* defined(CUSTOM_PLATFORM_NV_TEGRA) */
