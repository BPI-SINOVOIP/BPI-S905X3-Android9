#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:

 */
/**\file
 * \brief 配置文件模块测试程序
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-12: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_cfg.h>
#include <am_debug.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct
{
	AM_Bool_t           dhcp;
	struct in_addr      ip_addr;
	struct in_addr      gw_addr;
	struct in_addr      ip_mask;
	struct in_addr      server_ip;
	int                 server_port;
} AM_IPConfig_t;

typedef struct
{
	int                 service_id;
	int                 vid_pid;
	int                 aud_pid;
	int                 pcr_pid;
} AM_ServiceConfig_t;

#define MAX_SERVICE_COUNT (64)
typedef struct
{
	int                 freq;
	int                 baud;
	int                 qam;
	int                 service_count;
	int                 in_service_sec;
	AM_ServiceConfig_t  service[MAX_SERVICE_COUNT];
} AM_DVBConfig_t;

static AM_ErrorCode_t ip_key_cb (void *user_data, const char *key, const char *value)
{
	AM_IPConfig_t *conf=(AM_IPConfig_t*)user_data;
	
	if(!strcmp(key, "dhcp"))
	{
		return AM_CFG_Value2Bool(value, &conf->dhcp);
	}
	else if(!strcmp(key, "ip_addr"))
	{
		return AM_CFG_Value2IP(value, &conf->ip_addr);
	}
	else if(!strcmp(key, "ip_mask"))
	{
		return AM_CFG_Value2IP(value, &conf->ip_mask);
	}
	else if(!strcmp(key, "gw_addr"))
	{
		return AM_CFG_Value2IP(value, &conf->gw_addr);
	}
	else if(!strcmp(key, "server_ip"))
	{
		return AM_CFG_Value2IP(value, &conf->server_ip);
	}
	else if(!strcmp(key, "server_port"))
	{
		return AM_CFG_Value2Int(value, &conf->server_port);
	}
	return AM_CFG_ERR_UNKNOWN_TAG;
}

static AM_ErrorCode_t dvb_sec_begin_cb (void *user_data, const char *sec)
{
	AM_DVBConfig_t *conf=(AM_DVBConfig_t*)user_data;
	
	if(conf->in_service_sec)
	{
		AM_DEBUG(1, "unknown section");
		return AM_CFG_ERR_SYNTAX;
	}
	
	if(!strcmp(sec, "service"))
	{
		if(conf->service_count<MAX_SERVICE_COUNT)
		{
			conf->service_count++;
		}
		else
		{
			AM_DEBUG(1, "too many services");
		}
		conf->in_service_sec = 1;
		return AM_SUCCESS;
	}
	return AM_CFG_ERR_UNKNOWN_TAG;
}

static AM_ErrorCode_t dvb_sec_end_cb (void *user_data, const char *sec)
{
	AM_DVBConfig_t *conf=(AM_DVBConfig_t*)user_data;
	
	conf->in_service_sec = 0;
	return AM_SUCCESS;
}

static AM_ErrorCode_t dvb_key_cb (void *user_data, const char *key, const char *value)
{
	AM_DVBConfig_t *conf=(AM_DVBConfig_t*)user_data;
	
	if (conf->in_service_sec)
	{
		int id = conf->service_count-1;
		
		if(!strcmp(key, "service_id"))
		{
			return AM_CFG_Value2Int(value, &conf->service[id].service_id);
		}
		else if(!strcmp(key, "vid_pid"))
		{
			return AM_CFG_Value2Int(value, &conf->service[id].vid_pid);
		}
		else if(!strcmp(key, "aud_pid"))
		{
			return AM_CFG_Value2Int(value, &conf->service[id].aud_pid);
		}
		else if(!strcmp(key, "pcr_pid"))
		{
			return AM_CFG_Value2Int(value, &conf->service[id].pcr_pid);
		}
	}
	else
	{
		if(!strcmp(key, "freq"))
		{
			return AM_CFG_Value2Int(value, &conf->freq);
		}
		else if(!strcmp(key, "baud"))
		{
			return AM_CFG_Value2Int(value, &conf->baud);
		}
		else if(!strcmp(key, "qam"))
		{
			return AM_CFG_Value2Int(value, &conf->qam);
		}
	}
	
	return AM_CFG_ERR_UNKNOWN_TAG;
}

static void store_ip_conf(AM_IPConfig_t *conf)
{
	AM_CFG_OutputContext_t cx;
	
	AM_CFG_BeginOutput("ip.conf", &cx);
	AM_CFG_StoreBool(&cx, "dhcp", conf->dhcp);
	AM_CFG_StoreIP(&cx, "ip_addr", &conf->ip_addr);
	AM_CFG_StoreIP(&cx, "ip_mask", &conf->ip_mask);
	AM_CFG_StoreIP(&cx, "gw_addr", &conf->gw_addr);
	AM_CFG_StoreIP(&cx, "server_ip", &conf->server_ip);
	AM_CFG_StoreDec(&cx, "server_port", conf->server_port);
	
	AM_CFG_EndOutput(&cx);
}

static void store_dvb_conf(AM_DVBConfig_t *conf)
{
	AM_CFG_OutputContext_t cx;
	int i;
	
	AM_CFG_BeginOutput("dvb.conf", &cx);
	AM_CFG_StoreDec(&cx, "freq", conf->freq);
	AM_CFG_StoreDec(&cx, "baud", conf->baud);
	AM_CFG_StoreDec(&cx, "qam", conf->qam);
	
	for(i=0; i<conf->service_count; i++)
	{
		AM_CFG_BeginSection(&cx, "service");
		AM_CFG_StoreHex(&cx, "service_id", conf->service[i].service_id);
		AM_CFG_StoreHex(&cx, "vid_pid", conf->service[i].vid_pid);
		AM_CFG_StoreHex(&cx, "aud_pid", conf->service[i].aud_pid);
		AM_CFG_StoreHex(&cx, "pcr_pid", conf->service[i].pcr_pid);
		AM_CFG_EndSection(&cx);
	}
	
	AM_CFG_EndOutput(&cx);
}

int main(int argc, char **argv)
{
	AM_IPConfig_t ip_conf;
	AM_DVBConfig_t dvb_conf;
	int i;
	
	memset(&ip_conf, 0, sizeof(ip_conf));
	memset(&dvb_conf, 0, sizeof(dvb_conf));
	
	AM_CFG_Input("ip.conf", NULL, ip_key_cb, NULL, &ip_conf);
	AM_CFG_Input("dvb.conf", dvb_sec_begin_cb, dvb_key_cb, dvb_sec_end_cb, &dvb_conf);
	
	printf("DHCP: %d\n", ip_conf.dhcp);
	printf("IP: %s\n", inet_ntoa(ip_conf.ip_addr));
	printf("MASK: %s\n", inet_ntoa(ip_conf.ip_mask));
	printf("GATEWAY: %s\n", inet_ntoa(ip_conf.gw_addr));
	printf("SERVER IP: %s\n", inet_ntoa(ip_conf.server_ip));
	printf("SERVER PORT: %d\n", ip_conf.server_port);
	printf("FREQ: %d\n", dvb_conf.freq);
	printf("BUAD: %d\n", dvb_conf.baud);
	printf("QAM: %d\n", dvb_conf.qam);
	
	for(i=0; i<dvb_conf.service_count; i++)
	{
		printf("SERVICE %d\n", i);
		printf("\tSERVICE ID: 0x%x\n", dvb_conf.service[i].service_id);
		printf("\tVIDEO PID: 0x%x\n", dvb_conf.service[i].vid_pid);
		printf("\tAUDIO PID: 0x%x\n", dvb_conf.service[i].aud_pid);
		printf("\tPCR PID: 0x%x\n", dvb_conf.service[i].pcr_pid);
	}
	
	store_ip_conf(&ip_conf);
	store_dvb_conf(&dvb_conf);
	
	return 0;
}


