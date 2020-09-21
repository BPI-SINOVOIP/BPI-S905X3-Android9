/*
 * Firmware I/O code for mac80211 altobeam APOLLO drivers
 * *
 * Copyright (c) 2016, altobeam
 * Author:
 *
 * Based on apollo code
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * Based on:
 * ST-Ericsson UMAC CW1200 driver which is
 * Copyright (c) 2010, ST-Ericsson
 * Author: Ajitpal Singh <ajitpal.singh@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/debugfs.h>

#include "apollo.h"
#include "fwio.h"
#include "hwio.h"
#include "sbus.h"
#include "debug.h"
#include "bh.h"
#include "dcxo_dpll.h"


static char *fw = FIRMWARE_DEFAULT_PATH;
module_param(fw, charp, 0644);
MODULE_PARM_DESC(fw, "Override platform_data firmware file");
#pragma message(FIRMWARE_DEFAULT_PATH)


struct firmware_headr {
	u32 flags; /*0x34353677*/
	u32 version;
	u32 iccm_len;
	u32 dccm_len;
	u32 reserve[3];
	u16 reserve2;
	u16 checksum;
};

struct firmware_altobeam {
	struct firmware_headr hdr;
	u8 *fw_iccm;
	u8 *fw_dccm;
};
static struct firmware_altobeam atbm_fw;

void atbm_release_firmware(void)
{
	printk("atbm_release_firmware\n");
	if(atbm_fw.fw_dccm)
	{
		atbm_kfree(atbm_fw.fw_dccm);
		atbm_fw.fw_dccm = NULL;
	}
	if(atbm_fw.fw_iccm)
	{
		atbm_kfree(atbm_fw.fw_iccm);
		atbm_fw.fw_iccm = NULL;
	}
}
int atbm_init_firmware(void)
{
	printk("atbm_init_firmware\n");
	memset(&atbm_fw,0,sizeof(struct firmware_altobeam));
	return 0;
}

int atbm_set_firmare(struct firmware_altobeam *fw)
{
#ifdef ATBM_USE_SAVED_FW
	if(!fw || (!fw->fw_dccm&&!fw->fw_iccm))
	{
		printk(KERN_ERR "fw is err\n");
		return -1;
	}

	if(atbm_fw.fw_dccm || atbm_fw.fw_iccm)
	{
		printk(KERN_ERR "atbm_fw has been set\n");
		return -1;
	}
	memcpy(&atbm_fw.hdr,&fw->hdr,sizeof(struct firmware_headr));
	
	if(atbm_fw.hdr.iccm_len)
	{
		atbm_fw.fw_iccm = atbm_kzalloc(atbm_fw.hdr.iccm_len,GFP_KERNEL);
		
		if(!atbm_fw.fw_iccm)
		{
			printk(KERN_ERR "alloc atbm_fw.fw_iccm err\n");
			goto err;
		}
		memcpy(atbm_fw.fw_iccm,fw->fw_iccm,atbm_fw.hdr.iccm_len);
	}

	if(atbm_fw.hdr.dccm_len)
	{
		atbm_fw.fw_dccm= atbm_kzalloc(atbm_fw.hdr.dccm_len,GFP_KERNEL);

		if(!atbm_fw.fw_dccm)
		{
			printk(KERN_ERR "alloc atbm_fw.fw_dccm err\n");
			goto err;
		}
		memcpy(atbm_fw.fw_dccm,fw->fw_dccm,atbm_fw.hdr.dccm_len);
	}
	return 0;
err:
	if(atbm_fw.fw_iccm)
	{
		atbm_kfree(atbm_fw.fw_iccm);
		atbm_fw.fw_iccm = NULL;
	}

	if(atbm_fw.fw_dccm)
	{
		atbm_kfree(atbm_fw.fw_dccm);
		atbm_fw.fw_dccm = NULL;
	}
#endif //#ifndef USB_BUS
	return -1;
}

#define FW_IS_READY	((atbm_fw.fw_dccm != NULL) || (atbm_fw.fw_iccm != NULL))
int atbm_get_fw(struct firmware_altobeam *fw)
{
	if(!FW_IS_READY)
	{
		return -1;
	}

	memcpy(&fw->hdr,&atbm_fw.hdr,sizeof(struct firmware_headr));
	fw->fw_iccm = atbm_fw.fw_iccm;
	fw->fw_dccm = atbm_fw.fw_dccm;
	printk("%s:get fw\n",__func__);
	return 0;
}


int atbm_get_hw_type(u32 config_reg_val, int *major_revision)
{
#if 0
	int hw_type = -1;
	u32 config_value = config_reg_val;
	//u32 silicon_type = (config_reg_val >> 24) & 0x3;
	u32 silicon_vers = (config_reg_val >> 31) & 0x1;

	/* Check if we have CW1200 or STLC9000 */

	hw_type = HIF_1601_CHIP;
#endif
	return HIF_1601_CHIP;
}

static int atbm_load_firmware_generic(struct atbm_common *priv, u8 *data,u32 size,u32 addr)
{

	int ret=0;
	u32 put = 0;
	u8 *buf = NULL;


	buf = atbm_kmalloc(DOWNLOAD_BLOCK_SIZE*2, GFP_KERNEL | GFP_DMA);
	if (!buf) {
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,
			"%s: can't allocate bootloader buffer.\n", __func__);
		ret = -ENOMEM;
		goto error;
	}
	
#ifndef HW_DOWN_FW
	if(priv->sbus_ops->bootloader_debug_config)
		priv->sbus_ops->bootloader_debug_config(priv->sbus_priv,0);
#endif //#ifndef HW_DOWN_FW

	/*  downloading loop */
	printk(KERN_ERR "%s: addr %x: len %x\n",__func__,addr,size);
	for (put = 0; put < size ;put += DOWNLOAD_BLOCK_SIZE) {
		u32 tx_size;


		/* calculate the block size */
		tx_size  = min((size - put),(u32)DOWNLOAD_BLOCK_SIZE);

		memcpy(buf, &data[put], tx_size);

		/* send the block to sram */
		ret = atbm_fw_write(priv,put+addr,buf, tx_size);
		if (ret < 0) {
			atbm_dbg(ATBM_APOLLO_DBG_ERROR,
				"%s: can't write block at line %d.\n",
				__func__, __LINE__);
			printk(KERN_ERR "%s:put = 0x%x\n",__func__,put);
			goto error;
		}
	} /* End of bootloader download loop */

error:
	atbm_kfree(buf);
	return ret;


}

char * atbm_HwGetChipFw(struct atbm_common *priv)
{
	u32 chipver = 0;
	char * strHwChipFw;

	if(fw)
	{
		printk(KERN_ERR "%s, use module_param fw [%s]\n",__func__, fw );
	 	return fw;
	}

	atbm_direct_read_reg_32(priv,0x0acc017c,&chipver);
	chipver&=0x3f;
	switch(chipver)
	{
		case 0x0:	
			strHwChipFw = ("ApolloC0.bin");		
			break;
		case 0x1:	
			strHwChipFw = ("ApolloC0_TC.bin");	
			break;
		case 0x3:	
			strHwChipFw = ("ApolloC1_TC.bin");	
			break;
		case 0xc:	
			strHwChipFw = ("ApolloD.bin");		
			break;
		case 0xd:	
			strHwChipFw = ("ApolloD_TC.bin");	
			break;
		case 0x10:	
			strHwChipFw = ("ApolloE.bin");		
			break;
		case 0x20:	
			strHwChipFw = ("AthenaA.bin");		
			break;
		case 0x14:	
			strHwChipFw = ("ApolloF.bin");		
			break;
		case 0x15:	
			strHwChipFw = ("ApolloF_TC.bin");	
			break;
		case 0x24:	
			strHwChipFw = ("AthenaB.bin");		
			break;
		case 0x25:	
			strHwChipFw = ("AthenaBX.bin");		
			break;
		case 0x18:	
			strHwChipFw = ("Apollo_FM.bin");		
			break;
		default:
			strHwChipFw = FIRMWARE_DEFAULT_PATH;		
		break;
	}

	printk("%s, chipver=0x%x, use fw [%s]\n",__func__, chipver,strHwChipFw );

	return strHwChipFw;
}

//#define TEST_DCXO_CONFIG move to makefile
#define USED_FW_FILE
#ifdef USED_FW_FILE

/*check if fw headr ok*/
static int atbm_fw_checksum(struct firmware_headr * hdr)
{
	return 1;
}
#else
#include "firmware.h"
#endif
static int atbm_start_load_firmware(struct atbm_common *priv)
{

	int ret;
#ifdef USED_FW_FILE
	const char *fw_path= atbm_HwGetChipFw(priv);
#endif//
	const struct firmware *firmware = NULL;
	struct firmware_altobeam fw_altobeam;
	
	memset(&fw_altobeam,0,sizeof(struct firmware_altobeam));
loadfw:
	//u32 testreg_uart;
#ifdef START_DCXO_CONFIG
	atbm_ahb_write_32(priv,0x18e00014,0x200);
	atbm_ahb_read_32(priv,0x18e00014,&val32_1);
	//atbm_ahb_read_32(priv,0x16400000,&testreg_uart);
	printk("0x18e000e4-->%08x %08x\n",val32_1);
#endif//TEST_DCXO_CONFIG
	if(!FW_IS_READY)
	{
#ifdef USED_FW_FILE
	    atbm_dbg(ATBM_APOLLO_DBG_MSG,"%s:FW FILE = %s\n",__func__,fw_path);
		ret = request_firmware(&firmware, fw_path, priv->pdev);
		if (ret) {
			atbm_dbg(ATBM_APOLLO_DBG_ERROR,
				"%s: can't load firmware file %s.\n",
				__func__, fw_path);
			goto error;
		}
		BUG_ON(!firmware->data);
		if(*(int *)firmware->data == ALTOBEAM_WIFI_HDR_FLAG){
			memcpy(&fw_altobeam.hdr,firmware->data,sizeof(struct firmware_headr));
			if(atbm_fw_checksum(&fw_altobeam.hdr)==0){
				ret = -1;
				 atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: atbm_fw_checksum fail 11\n", __func__);
				 goto error;
			}
			fw_altobeam.fw_iccm = (u8 *)firmware->data + sizeof(struct firmware_headr);
			fw_altobeam.fw_dccm = fw_altobeam.fw_iccm + fw_altobeam.hdr.iccm_len;
			atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: have header,lmac version(%d) iccm_len(%d) dccm_len(%d)\n", __func__,
				fw_altobeam.hdr.version,fw_altobeam.hdr.iccm_len,fw_altobeam.hdr.dccm_len);

			//frame_hexdump("fw_iccm ",fw_altobeam.fw_iccm,64);
			//frame_hexdump("fw_dccm ",fw_altobeam.fw_dccm,64);
		}
		else {
			fw_altobeam.hdr.version =  0x001;
			if(firmware->size > DOWNLOAD_ITCM_SIZE){
				fw_altobeam.hdr.iccm_len =  DOWNLOAD_ITCM_SIZE;
				fw_altobeam.hdr.dccm_len =  firmware->size - fw_altobeam.hdr.iccm_len;
				if(fw_altobeam.hdr.dccm_len > DOWNLOAD_DTCM_SIZE) {
					ret = -1;
				 	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: atbm_fw_checksum fail 22\n", __func__);
				 	goto error;
				}
				fw_altobeam.fw_iccm = (u8 *)firmware->data;
				fw_altobeam.fw_dccm = fw_altobeam.fw_iccm+fw_altobeam.hdr.iccm_len;
			}
			else {
				fw_altobeam.hdr.iccm_len = firmware->size;
				fw_altobeam.hdr.dccm_len = 0;
				fw_altobeam.fw_iccm = (u8 *)firmware->data;
				
			}

		}
#else //USED_FW_FILE
		{
		atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: used firmware.h=\n", __func__);
		fw_altobeam.hdr.iccm_len = sizeof(fw_code);
		fw_altobeam.hdr.dccm_len = sizeof(fw_data);
		
		fw_altobeam.fw_iccm = &fw_code[0];
		fw_altobeam.fw_dccm = &fw_data[0];
		}
#endif //USED_FW_FILE
		atbm_set_firmare(&fw_altobeam);
	}
	else
	{
		if((ret = atbm_get_fw(&fw_altobeam))<0)
		{
			goto error;
		}
	}
	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: START DOWNLOAD ICCM=========\n", __func__);

	ret = atbm_load_firmware_generic(priv,fw_altobeam.fw_iccm,fw_altobeam.hdr.iccm_len,DOWNLOAD_ITCM_ADDR);
	if(ret<0)
		goto error;
	#ifdef USB_BUS
	fw_altobeam.hdr.dccm_len = 0x8000;
	#else
	if(fw_altobeam.hdr.dccm_len > 0x9000)
	fw_altobeam.hdr.dccm_len = 0x9000;
	#endif
	atbm_dbg(ATBM_APOLLO_DBG_ERROR,"%s: START DOWNLOAD DCCM=========\n", __func__);
	ret = atbm_load_firmware_generic(priv,fw_altobeam.fw_dccm,fw_altobeam.hdr.dccm_len,DOWNLOAD_DTCM_ADDR);
	if(ret<0)
		goto error;

	atbm_dbg(ATBM_APOLLO_DBG_MSG, "%s: FIRMWARE DOWNLOAD SUCCESS\n",__func__);

error:
	if (ret<0){
		atbm_reset_lmc_cpu(priv);
		goto loadfw;
	}else if (firmware)
		release_firmware(firmware);
	return ret;


}


int atbm_load_firmware(struct atbm_common *hw_priv)
{
	int ret;

	printk("atbm_before_load_firmware++\n");
	ret = atbm_before_load_firmware(hw_priv);
	if(ret <0)
		goto out;
	printk("atbm_start_load_firmware++\n");
	ret = atbm_start_load_firmware(hw_priv);
	if(ret <0)
		goto out;
	printk("atbm_after_load_firmware++\n");
	ret = atbm_after_load_firmware(hw_priv);
	if(ret <0){
		goto out;
	}
	ret =0;
out:
	return ret;

}

