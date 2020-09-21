#define LOG_TAG "bt_hwcfg_usb"
#define RTKBT_RELEASE_NAME "20190311_BT_ANDROID_9.0"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_rtk.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"
#include <unistd.h>
#include <endian.h>
#include <byteswap.h>
#include <unistd.h>

#include "bt_vendor_lib.h"
#include "hardware.h"
#include "rtk_common.h"

/******************************************************************************
**  Constants &  Macros
******************************************************************************/

extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];
extern bool rtkbt_auto_restart;
void hw_usb_config_cback(void *p_evt_buf);
extern bt_hw_cfg_cb_t hw_cfg_cb;
extern int getmacaddr(unsigned char * addr);
extern struct rtk_epatch_entry *rtk_get_patch_entry(bt_hw_cfg_cb_t *cfg_cb);
extern int rtk_get_bt_firmware(uint8_t** fw_buf, char* fw_short_name);
extern uint8_t rtk_get_fw_project_id(uint8_t *p_buf);

#define EXTRA_CONFIG_FILE "/vendor/etc/bluetooth/rtk_btconfig.txt"
static struct rtk_bt_vendor_config_entry *extra_extry;
static struct rtk_bt_vendor_config_entry *extra_entry_inx = NULL;

/******************************************************************************
**  Static variables
******************************************************************************/
//static bt_hw_cfg_cb_t hw_cfg_cb;

typedef struct {
    uint16_t    vid;
    uint16_t    pid;
    uint16_t    lmp_sub_default;
    uint16_t    lmp_sub;
    uint16_t    eversion;
    char        *mp_patch_name;
    char        *patch_name;
    char        *config_name;
    uint8_t     *fw_cache;
    int         fw_len;
    uint16_t    mac_offset;
    uint32_t    max_patch_size;
} usb_patch_info;

static usb_patch_info usb_fw_patch_table[] = {
/* { vid, pid, lmp_sub_default, lmp_sub, everion, mp_fw_name, fw_name, config_name, fw_cache, fw_len, mac_offset } */
{ 0x0BDA, 0x1724, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723A */
{ 0x0BDA, 0x8723, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AE */
{ 0x0BDA, 0xA723, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AE for LI */
{ 0x0BDA, 0x0723, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AE */
{ 0x13D3, 0x3394, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AE for Azurewave*/

{ 0x0BDA, 0x0724, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AU */
{ 0x0BDA, 0x8725, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AU */
{ 0x0BDA, 0x872A, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AU */
{ 0x0BDA, 0x872B, 0x1200, 0, 0, "mp_rtl8723a_fw", "rtl8723a_fw", "rtl8723a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* 8723AU */

{ 0x0BDA, 0xb720, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723bu_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BU */
{ 0x0BDA, 0xb72A, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723bu_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BU */
{ 0x0BDA, 0xb728, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for LC */
{ 0x0BDA, 0xb723, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE */
{ 0x0BDA, 0xb72B, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE */
{ 0x0BDA, 0xb001, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for HP */
{ 0x0BDA, 0xb002, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE */
{ 0x0BDA, 0xb003, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE */
{ 0x0BDA, 0xb004, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE */
{ 0x0BDA, 0xb005, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE */

{ 0x13D3, 0x3410, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for Azurewave */
{ 0x13D3, 0x3416, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for Azurewave */
{ 0x13D3, 0x3459, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for Azurewave */
{ 0x0489, 0xE085, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for Foxconn */
{ 0x0489, 0xE08B, 0x8723, 0, 0, "mp_rtl8723b_fw", "rtl8723b_fw", "rtl8723b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8723BE for Foxconn */

{ 0x0BDA, 0x2850, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761au_fw", "rtl8761a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AU */
{ 0x0BDA, 0xA761, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761au_fw", "rtl8761a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AU only */
{ 0x0BDA, 0x818B, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761aw8192eu_fw", "rtl8761aw8192eu_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AW + 8192EU */
{ 0x0BDA, 0x818C, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761aw8192eu_fw", "rtl8761aw8192eu_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AW + 8192EU */
{ 0x0BDA, 0x8760, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761au8192ee_fw", "rtl8761a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AU + 8192EE */
{ 0x0BDA, 0xB761, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761au_fw", "rtl8761a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AUV only */
{ 0x0BDA, 0x8761, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761au8192ee_fw", "rtl8761a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AU + 8192EE for LI */
{ 0x0BDA, 0x8A60, 0x8761, 0, 0, "mp_rtl8761a_fw", "rtl8761au8812ae_fw", "rtl8761a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8761AU + 8812AE */
{ 0x0BDA, 0x8771, 0x8761, 0, 0, "mp_rtl8761b_fw", "rtl8761b_fw", "rtl8761b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_4PLUS, MAX_PATCH_SIZE_40K}, /* RTL8761BU */

{ 0x0BDA, 0x8821, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AE */
{ 0x0BDA, 0x0821, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AE */
{ 0x0BDA, 0x0823, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AU */
{ 0x13D3, 0x3414, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AE */
{ 0x13D3, 0x3458, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AE */
{ 0x13D3, 0x3461, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AE */
{ 0x13D3, 0x3462, 0x8821, 0, 0, "mp_rtl8821a_fw", "rtl8821a_fw", "rtl8821a_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_1_2, MAX_PATCH_SIZE_24K}, /* RTL8821AE */

{ 0x0BDA, 0xB822, 0x8822, 0, 0, "mp_rtl8822b_fw", "rtl8822b_fw", "rtl8822b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_24K}, /* RTL8822BE */
{ 0x0BDA, 0xB82C, 0x8822, 0, 0, "mp_rtl8822b_fw", "rtl8822b_fw", "rtl8822b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_24K}, /* RTL8822BU */
{ 0x0BDA, 0xB023, 0x8822, 0, 0, "mp_rtl8822b_fw", "rtl8822b_fw", "rtl8822b_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_24K}, /* RTL8822BE */
{ 0x0BDA, 0xB703, 0x8703, 0, 0, "mp_rtl8723c_fw", "rtl8723c_fw", "rtl8723c_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_24K}, /* RTL8723CU */
{ 0x0BDA, 0xC82C, 0x8822, 0, 0, "mp_rtl8822c_fw", "rtl8822c_fw", "rtl8822c_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_4PLUS, MAX_PATCH_SIZE_40K}, /* RTL8822CU */
{ 0x0BDA, 0xC822, 0x8822, 0, 0, "mp_rtl8822c_fw", "rtl8822c_fw", "rtl8822c_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_4PLUS, MAX_PATCH_SIZE_40K}, /* RTL8822CE */
/* todo: RTL8703BU */

{ 0x0BDA, 0xD723, 0x8723, 0, 0, "mp_rtl8723d_fw", "rtl8723d_fw", "rtl8723d_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_40K}, /* RTL8723DU */
{ 0x0BDA, 0xD720, 0x8723, 0, 0, "mp_rtl8723d_fw", "rtl8723d_fw", "rtl8723d_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_40K}, /* RTL8723DE */
{ 0x0BDA, 0xB820, 0x8821, 0, 0, "mp_rtl8821c_fw", "rtl8821c_fw", "rtl8821c_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_40K}, /* RTL8821CU */
{ 0x0BDA, 0xC820, 0x8821, 0, 0, "mp_rtl8821c_fw", "rtl8821c_fw", "rtl8821c_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_40K}, /* RTL8821CU */
{ 0x0BDA, 0xC821, 0x8821, 0, 0, "mp_rtl8821c_fw", "rtl8821c_fw", "rtl8821c_config", NULL, 0 ,CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_40K}, /* RTL8821CE */
/* todo: RTL8703CU */

/* NOTE: must append patch entries above the null entry */
{ 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0 }
};


uint16_t usb_project_id[] = {
    ROM_LMP_8723a,
    ROM_LMP_8723b,
    ROM_LMP_8821a,
    ROM_LMP_8761a,
    ROM_LMP_8703a,
    ROM_LMP_8763a,
    ROM_LMP_8703b,
    ROM_LMP_8723c,
    ROM_LMP_8822b,
    ROM_LMP_8723d,
    ROM_LMP_8821c,
    ROM_LMP_NONE,
    ROM_LMP_NONE,
    ROM_LMP_8822c,
    ROM_LMP_8761b,
    ROM_LMP_NONE
};
//signature: realtech
static const uint8_t RTK_EPATCH_SIGNATURE[8]={0x52,0x65,0x61,0x6C,0x74,0x65,0x63,0x68};
//Extension Section IGNATURE:0x77FD0451
static const uint8_t EXTENSION_SECTION_SIGNATURE[4]={0x51,0x04,0xFD,0x77};

static void usb_line_process(char *buf, unsigned short *offset, int *t)
{
    char *head = buf;
    char *ptr = buf;
    char *argv[32];
    int argc = 0;
    unsigned char len = 0;
    int i = 0;
    static int alt_size = 0;

    if(buf[0] == '\0' || buf[0] == '#' || buf[0] == '[')
        return;
    if(alt_size > MAX_ALT_CONFIG_SIZE-4)
    {
        ALOGW("Extra Config file is too large");
        return;
    }
    if(extra_entry_inx == NULL)
        extra_entry_inx = extra_extry;
    ALOGI("line_process:%s", buf);
    while((ptr = strsep(&head, ", \t")) != NULL)
    {
        if(!ptr[0])
            continue;
        argv[argc++] = ptr;
        if(argc >= 32) {
            ALOGW("Config item is too long");
            break;
        }
    }

    if(argc <4) {
        ALOGE("Invalid Config item, ignore");
        return;
    }

    offset[(*t)] = (unsigned short)((strtoul(argv[0], NULL, 16)) | (strtoul(argv[1], NULL, 16) << 8));
    ALOGI("Extra Config offset %04x", offset[(*t)]);
    extra_entry_inx->offset = offset[(*t)];
    (*t)++;
    len = (unsigned char)strtoul(argv[2], NULL, 16);
    if(len != (unsigned char)(argc - 3)) {
        ALOGE("Extra Config item len %d is not match, we assume the actual len is %d", len, (argc-3));
        len = argc -3;
    }
    extra_entry_inx->entry_len = len;

    alt_size += len + sizeof(struct rtk_bt_vendor_config_entry);
    if(alt_size > MAX_ALT_CONFIG_SIZE)
    {
        ALOGW("Extra Config file is too large");
        extra_entry_inx->offset = 0;
        extra_entry_inx->entry_len = 0;
        alt_size -= (len + sizeof(struct rtk_bt_vendor_config_entry));
        return;
    }
    for(i = 0; i < len; i++)
    {
        extra_entry_inx->entry_data[i] = (uint8_t)strtoul(argv[3+i], NULL, 16);
        ALOGI("data[%d]:%02x", i, extra_entry_inx->entry_data[i]);
    }
    extra_entry_inx = (struct rtk_bt_vendor_config_entry *)((uint8_t *)extra_entry_inx + len + sizeof(struct rtk_bt_vendor_config_entry));
}

static void usb_parse_extra_config(const char *path, usb_patch_info *patch_entry, unsigned short *offset, int *t)
{
    int fd, ret;
    unsigned char buf[1024];

    fd = open(path, O_RDONLY);
    if(fd == -1) {
        ALOGI("Couldn't open extra config %s, err:%s", path, strerror(errno));
        return;
    }

    ret = read(fd, buf, sizeof(buf));
    if(ret == -1) {
        ALOGE("Couldn't read %s, err:%s", path, strerror(errno));
        close(fd);
        return;
    }
    else if(ret == 0) {
        ALOGE("%s is empty", path);
        close(fd);
        return;
    }

    if(ret > 1022) {
        ALOGE("Extra config file is too big");
        close(fd);
        return;
    }
    buf[ret++] = '\n';
    buf[ret++] = '\0';
    close(fd);
    char *head = (void *)buf;
    char *ptr = (void *)buf;
    ptr = strsep(&head, "\n\r");
    if(strncmp(ptr, patch_entry->config_name, strlen(ptr)))
    {
        ALOGW("Extra config file not set for %s, ignore", patch_entry->config_name);
        return;
    }
    while((ptr = strsep(&head, "\n\r")) != NULL)
    {
        if(!ptr[0])
            continue;
        usb_line_process(ptr, offset, t);
    }
}

static inline int getUsbAltSettings(usb_patch_info *patch_entry, unsigned short *offset)//(patch_info *patch_entry, unsigned short *offset, int max_group_cnt)
{
    int n = 0;
    if(patch_entry)
        offset[n++] = patch_entry->mac_offset;
/*
//sample code, add special settings

    offset[n++] = 0x15B;
*/
    if(extra_extry)
        usb_parse_extra_config(EXTRA_CONFIG_FILE, patch_entry, offset, &n);

    return n;
}

static inline int getUsbAltSettingVal(usb_patch_info *patch_entry, unsigned short offset, unsigned char * val)
{
    int res = 0;

    int i = 0;
    struct rtk_bt_vendor_config_entry *ptr = extra_extry;

    while(ptr->offset)
    {
        if(ptr->offset == offset)
        {
            if(offset != patch_entry->mac_offset)
            {
                memcpy(val, ptr->entry_data, ptr->entry_len);
                res = ptr->entry_len;
                ALOGI("Get Extra offset:%04x, val:", offset);
                for(i = 0; i < ptr->entry_len; i++)
                    ALOGI("%02x", ptr->entry_data[i]);
            }
            break;
        }
        ptr = (struct rtk_bt_vendor_config_entry *)((uint8_t *)ptr + ptr->entry_len + sizeof(struct rtk_bt_vendor_config_entry));
    }

/*    switch(offset)
    {
//sample code, add special settings
        case 0x15B:
            val[0] = 0x0B;
            val[1] = 0x0B;
            val[2] = 0x0B;
            val[3] = 0x0B;
            res = 4;
            break;

        default:
            res = 0;
            break;
    }
*/
    if((patch_entry)&&(offset == patch_entry->mac_offset)&&(res == 0))
    {
        if(getmacaddr(val) == 0){
            ALOGI("MAC: %02x:%02x:%02x:%02x:%02x:%02x", val[5], val[4], val[3], val[2], val[1], val[0]);
            res = 6;
        }
    }
    return res;
}

static void rtk_usb_update_altsettings(usb_patch_info *patch_entry, unsigned char* config_buf_ptr, size_t *config_len_ptr)
{
    unsigned short offset[256], data_len;
    unsigned char val[256];

    struct rtk_bt_vendor_config* config = (struct rtk_bt_vendor_config*) config_buf_ptr;
    struct rtk_bt_vendor_config_entry* entry = config->entry;
    size_t config_len = *config_len_ptr;
    unsigned int  i = 0;
    int count = 0,temp = 0, j;

    if((extra_extry = (struct rtk_bt_vendor_config_entry *)malloc(MAX_ALT_CONFIG_SIZE)) == NULL)
    {
        ALOGE("malloc buffer for extra_extry failed");
    }
    else
        memset(extra_extry, 0, MAX_ALT_CONFIG_SIZE);

    ALOGI("ORG Config len=%08zx:\n", config_len);
    for(i = 0; i <= config_len; i+= 0x10)
    {
        ALOGI("%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", i, \
            config_buf_ptr[i], config_buf_ptr[i+1], config_buf_ptr[i+2], config_buf_ptr[i+3], config_buf_ptr[i+4], config_buf_ptr[i+5], config_buf_ptr[i+6], config_buf_ptr[i+7], \
            config_buf_ptr[i+8], config_buf_ptr[i+9], config_buf_ptr[i+10], config_buf_ptr[i+11], config_buf_ptr[i+12], config_buf_ptr[i+13], config_buf_ptr[i+14], config_buf_ptr[i+15]);
    }

    memset(offset, 0, sizeof(offset));
    memset(val, 0, sizeof(val));
    data_len = le16_to_cpu(config->data_len);

    count = getUsbAltSettings(patch_entry, offset);//getAltSettings(patch_entry, offset, sizeof(offset)/sizeof(unsigned short));
    if(count <= 0){
        ALOGI("rtk_update_altsettings: No AltSettings");
        return;
    }else{
        ALOGI("rtk_update_altsettings: %d AltSettings", count);
    }

    if (data_len != config_len - sizeof(struct rtk_bt_vendor_config))
    {
        ALOGE("rtk_update_altsettings: config len(%x) is not right(%lx)", data_len, (unsigned long)(config_len-sizeof(struct rtk_bt_vendor_config)));
        return;
    }

    for (i = 0; i < data_len;)
    {
        for(j = 0; j < count;j++)
        {
            if(le16_to_cpu(entry->offset) == offset[j]) {
                if(offset[j] == patch_entry->mac_offset)
                    offset[j] = 0;
                else
                {
                    struct rtk_bt_vendor_config_entry *t = extra_extry;
                    while(t->offset) {
                        if(t->offset == le16_to_cpu(entry->offset))
                        {
                            if(t->entry_len == entry->entry_len)
                                offset[j] = 0;
                            break;
                        }
                        t = (struct rtk_bt_vendor_config_entry *)((uint8_t *)t + t->entry_len + sizeof(struct rtk_bt_vendor_config_entry));
                    }
                }
            }
        }
        if(getUsbAltSettingVal(patch_entry, le16_to_cpu(entry->offset), val) == entry->entry_len){
            ALOGI("rtk_update_altsettings: replace %04x[%02x]", le16_to_cpu(entry->offset), entry->entry_len);
            memcpy(entry->entry_data, val, entry->entry_len);
        }
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry*)((uint8_t*)entry + temp);
    }

    for(j = 0; j < count;j++){
        if(offset[j] == 0)
            continue;
        entry->entry_len = getUsbAltSettingVal(patch_entry, offset[j], val);
        if(entry->entry_len <= 0)
            continue;
        entry->offset = cpu_to_le16(offset[j]);
        memcpy(entry->entry_data, val, entry->entry_len);
        ALOGI("rtk_update_altsettings: add %04x[%02x]", le16_to_cpu(entry->offset), entry->entry_len);
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry*)((uint8_t*)entry + temp);
    }
    config->data_len = cpu_to_le16(i);
    *config_len_ptr = i + sizeof(struct rtk_bt_vendor_config);

    if(extra_extry)
    {
        free(extra_extry);
        extra_extry = NULL;
        extra_entry_inx = NULL;
    }

    ALOGI("NEW Config len=%08zx:\n", *config_len_ptr);
    for(i = 0; i<= (*config_len_ptr); i+= 0x10)
    {
        ALOGI("%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", i, \
            config_buf_ptr[i], config_buf_ptr[i+1], config_buf_ptr[i+2], config_buf_ptr[i+3], config_buf_ptr[i+4], config_buf_ptr[i+5], config_buf_ptr[i+6], config_buf_ptr[i+7], \
            config_buf_ptr[i+8], config_buf_ptr[i+9], config_buf_ptr[i+10], config_buf_ptr[i+11], config_buf_ptr[i+12], config_buf_ptr[i+13], config_buf_ptr[i+14], config_buf_ptr[i+15]);
    }
    return;
}


static void rtk_usb_parse_config_file(unsigned char** config_buf, size_t* filelen, uint8_t bt_addr[6], uint16_t mac_offset)
{
    struct rtk_bt_vendor_config* config = (struct rtk_bt_vendor_config*) *config_buf;
    uint16_t config_len = le16_to_cpu(config->data_len), temp = 0;
    struct rtk_bt_vendor_config_entry* entry = config->entry;
    unsigned int i = 0;
    uint8_t  heartbeat_buf = 0;
    //uint32_t config_has_bdaddr = 0;
    uint8_t *p;

    ALOGD("bt_addr = %x", bt_addr[0]);
    if (le32_to_cpu(config->signature) != RTK_VENDOR_CONFIG_MAGIC)
    {
        ALOGE("config signature magic number(0x%x) is not set to RTK_VENDOR_CONFIG_MAGIC", config->signature);
        return;
    }

    if (config_len != *filelen - sizeof(struct rtk_bt_vendor_config))
    {
        ALOGE("config len(0x%x) is not right(0x%zx)", config_len, *filelen-sizeof(struct rtk_bt_vendor_config));
        return;
    }

    hw_cfg_cb.heartbeat = 0;
    for (i = 0; i < config_len;)
    {
        switch(le16_to_cpu(entry->offset))
        {
            case 0x017a:
            {
                if(mac_offset == CONFIG_MAC_OFFSET_GEN_1_2)
                {
                    p = (uint8_t *)entry->entry_data;
                    STREAM_TO_UINT8(heartbeat_buf, p);
                    if((heartbeat_buf & 0x02) && (heartbeat_buf & 0x10))
                        hw_cfg_cb.heartbeat = 1;
                    else
                        hw_cfg_cb.heartbeat = 0;

                    ALOGI("config 0x017a heartbeat = %d",hw_cfg_cb.heartbeat);
                }
                break;
            }
            case 0x01be:
            {
                if(mac_offset == CONFIG_MAC_OFFSET_GEN_3PLUS || mac_offset == CONFIG_MAC_OFFSET_GEN_4PLUS)
                {
                    p = (uint8_t *)entry->entry_data;
                    STREAM_TO_UINT8(heartbeat_buf, p);
                    if((heartbeat_buf & 0x02) && (heartbeat_buf & 0x10))
                        hw_cfg_cb.heartbeat = 1;
                    else
                        hw_cfg_cb.heartbeat = 0;

                    ALOGI("config 0x01be heartbeat = %d",hw_cfg_cb.heartbeat);
                }
                break;
            }
            default:
                ALOGI("config offset(0x%x),length(0x%x)", entry->offset, entry->entry_len);
                break;
        }
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry*)((uint8_t*)entry + temp);
    }

    return;
}

static uint32_t rtk_usb_get_bt_config(unsigned char** config_buf,
        char * config_file_short_name, uint16_t mac_offset)
{
    char bt_config_file_name[PATH_MAX] = {0};
    struct stat st;
    size_t filelen;
    int fd;
    //FILE* file = NULL;

    sprintf(bt_config_file_name, BT_CONFIG_DIRECTORY, config_file_short_name);
    ALOGI("BT config file: %s", bt_config_file_name);

    if (stat(bt_config_file_name, &st) < 0)
    {
        ALOGE("can't access bt config file:%s, errno:%d\n", bt_config_file_name, errno);
        return 0;
    }

    filelen = st.st_size;
    if(filelen > MAX_ORG_CONFIG_SIZE)
    {
        ALOGE("bt config file is too large(>0x%04x)", MAX_ORG_CONFIG_SIZE);
        return 0;
    }

    if ((fd = open(bt_config_file_name, O_RDONLY)) < 0)
    {
        ALOGE("Can't open bt config file");
        return 0;
    }

    if ((*config_buf = malloc(MAX_ORG_CONFIG_SIZE + MAX_ALT_CONFIG_SIZE)) == NULL)
    {
        ALOGE("malloc buffer for config file fail(0x%zx)\n", filelen);
        close(fd);
        return 0;
    }

    if (read(fd, *config_buf, filelen) < (ssize_t)filelen)
    {
        ALOGE("Can't load bt config file");
        free(*config_buf);
        close(fd);
        return 0;
    }

    rtk_usb_parse_config_file(config_buf, &filelen, vnd_local_bd_addr, mac_offset);

    close(fd);
    return filelen;
}

static usb_patch_info *rtk_usb_get_fw_table_entry(uint16_t vid, uint16_t pid)
{
    usb_patch_info *patch_entry = usb_fw_patch_table;

    uint32_t entry_size = sizeof(usb_fw_patch_table) / sizeof(usb_fw_patch_table[0]);
    uint32_t i;

    for (i = 0; i < entry_size; i++, patch_entry++) {
        if ((vid == patch_entry->vid)&&(pid == patch_entry->pid))
            break;
    }

    if (i == entry_size) {
        ALOGE("%s: No fw table entry found", __func__);
        return NULL;
    }

    return patch_entry;
}

static void rtk_usb_get_bt_final_patch(bt_hw_cfg_cb_t* cfg_cb)
{
    uint8_t proj_id = 0;
    struct rtk_epatch_entry* entry = NULL;
    struct rtk_epatch *patch = (struct rtk_epatch *)cfg_cb->fw_buf;
    //int iBtCalLen = 0;

    if(cfg_cb->lmp_subversion == LMPSUBVERSION_8723a)
    {
        if(memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, 8) == 0)
        {
            ALOGE("8723as check signature error!");
            cfg_cb->dl_fw_flag = 0;
            goto free_buf;
        }
        else
        {
            cfg_cb->total_len = cfg_cb->fw_len + cfg_cb->config_len;
            if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len)))
            {
                ALOGE("can't alloc memory for fw&config, errno:%d", errno);
                cfg_cb->dl_fw_flag = 0;
                goto free_buf;
            }
            else
            {
                ALOGI("8723as, fw copy direct");
                memcpy(cfg_cb->total_buf, cfg_cb->fw_buf, cfg_cb->fw_len);
                memcpy(cfg_cb->total_buf+cfg_cb->fw_len, cfg_cb->config_buf, cfg_cb->config_len);
                //cfg_cb->lmp_sub_current = *(uint16_t *)(cfg_cb->total_buf + cfg_cb->total_len - cfg_cb->config_len - 4);
                cfg_cb->dl_fw_flag = 1;
                goto free_buf;
            }
        }
    }

    if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, 8))
    {
        ALOGE("check signature error");
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    /* check the extension section signature */
    if (memcmp(cfg_cb->fw_buf + cfg_cb->fw_len - 4, EXTENSION_SECTION_SIGNATURE, 4))
    {
        ALOGE("check extension section signature error");
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    proj_id = rtk_get_fw_project_id(cfg_cb->fw_buf + cfg_cb->fw_len - 5);
    if(usb_project_id[proj_id] != hw_cfg_cb.lmp_subversion_default)
    {
        ALOGE("usb_project_id is 0x%08x, fw project_id is %d, does not match!!!",
                        usb_project_id[proj_id], hw_cfg_cb.lmp_subversion);
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    entry = rtk_get_patch_entry(cfg_cb);
    if (entry)
    {
        cfg_cb->total_len = entry->patch_length + cfg_cb->config_len;
    }
    else
    {
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }

    ALOGI("total_len = 0x%x", cfg_cb->total_len);

    if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len)))
    {
        ALOGE("Can't alloc memory for multi fw&config, errno:%d", errno);
        cfg_cb->dl_fw_flag = 0;
        goto free_buf;
    }
    else
    {
        memcpy(cfg_cb->total_buf, cfg_cb->fw_buf + entry->patch_offset, entry->patch_length);
        memcpy(cfg_cb->total_buf + entry->patch_length - 4, &patch->fw_version, 4);
        memcpy(&entry->svn_version, cfg_cb->total_buf + entry->patch_length - 8, 4);
        memcpy(&entry->coex_version, cfg_cb->total_buf + entry->patch_length - 12, 4);

        ALOGI("BTCOEX:20%06d-%04x svn_version:%d lmp_subversion:0x%x hci_version:0x%x hci_revision:0x%x chip_type:%d Cut:%d libbt-vendor_uart version:%s, patch->fw_version = %x\n",
        ((entry->coex_version >> 16) & 0x7ff) + ((entry->coex_version >> 27) * 10000),
        (entry->coex_version & 0xffff), entry->svn_version, cfg_cb->lmp_subversion, cfg_cb->hci_version, cfg_cb->hci_revision, cfg_cb->chip_type, cfg_cb->eversion+1, RTK_VERSION, patch->fw_version);
    }

    if (cfg_cb->config_len)
    {
        memcpy(cfg_cb->total_buf+entry->patch_length, cfg_cb->config_buf, cfg_cb->config_len);
    }

    cfg_cb->dl_fw_flag = 1;
    ALOGI("Fw:%s exists, config file:%s exists", (cfg_cb->fw_len>0)?"":"not", (cfg_cb->config_len>0)?"":"not");

free_buf:
    if (cfg_cb->fw_len > 0)
    {
        free(cfg_cb->fw_buf);
        cfg_cb->fw_len = 0;
    }

    if (cfg_cb->config_len > 0)
    {
        free(cfg_cb->config_buf);
        cfg_cb->config_len = 0;
    }

    if(entry)
    {
        free(entry);
    }
}

static int usb_hci_download_patch_h4(HC_BT_HDR *p_buf, int index, uint8_t *data, int len)
{
    uint8_t retval = FALSE;
    uint8_t *p = (uint8_t *) (p_buf + 1);

    UINT16_TO_STREAM(p, HCI_VSC_DOWNLOAD_FW_PATCH);
    *p++ = 1 + len;  /* parameter length */
    *p++ = index;
    memcpy(p, data, len);
    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1+len;

    hw_cfg_cb.state = HW_CFG_DL_FW_PATCH;

    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_DOWNLOAD_FW_PATCH, p_buf, hw_usb_config_cback);
    return retval;
}

static void rtk_usb_get_fw_version(bt_hw_cfg_cb_t* cfg_cb)
{
    struct rtk_epatch *patch = (struct rtk_epatch *)cfg_cb->fw_buf;

    if(cfg_cb->lmp_subversion == LMPSUBVERSION_8723a)
    {
        cfg_cb->lmp_sub_current = 0;
    }
    else
    {
        cfg_cb->lmp_sub_current = (uint16_t)patch->fw_version;
    }

}
/*******************************************************************************
**
** Function         hw_usb_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_usb_config_cback(void *p_mem)
{
    HC_BT_HDR   *p_evt_buf = NULL;
    uint8_t     *p = NULL;//, *pp=NULL;
    uint8_t     status = 0;
    uint16_t    opcode = 0;
    HC_BT_HDR   *p_buf = NULL;
    uint8_t     is_proceeding = FALSE;
    //int         i = 0;
    uint8_t     iIndexRx = 0;
    //patch_info* prtk_patch_file_info = NULL;
    usb_patch_info* prtk_usb_patch_file_info = NULL;
    //uint32_t    host_baudrate = 0;

#if (USE_CONTROLLER_BDADDR == TRUE)
    //const uint8_t null_bdaddr[BD_ADDR_LEN] = {0,0,0,0,0,0};
#endif

    if(p_mem != NULL)
    {
        p_evt_buf = (HC_BT_HDR *) p_mem;
        status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET);
        p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE_OFFSET;
        STREAM_TO_UINT16(opcode,p);
    }

    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    /*a cut fc6d status==1*/
    if (((status == 0) ||(opcode == HCI_VSC_READ_ROM_VERSION)) && bt_vendor_cbacks)
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_MAX_LEN);

    if (p_buf != NULL)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        BTVNDDBG("hw_cfg_cb.state = %i", hw_cfg_cb.state);
        switch (hw_cfg_cb.state)
        {
            case HW_CFG_RESET_CHANNEL_CONTROLLER:
            {
                usleep(300000);
                hw_cfg_cb.state = HW_CFG_READ_LOCAL_VER;
                p = (uint8_t *) (p_buf + 1);
                UINT16_TO_STREAM(p, HCI_READ_LMP_VERSION);
                *p++ = 0;
                p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_READ_LMP_VERSION, p_buf, hw_usb_config_cback);
                break;
            }
            case HW_CFG_READ_LOCAL_VER:
            {
                if (status == 0)
                {
                    p = ((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OP1001_HCI_VERSION_OFFSET);
                    STREAM_TO_UINT16(hw_cfg_cb.hci_version, p);
                    p = ((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OP1001_HCI_REVISION_OFFSET);
                    STREAM_TO_UINT16(hw_cfg_cb.hci_revision, p);
                    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OP1001_LMP_SUBVERSION_OFFSET;
                    STREAM_TO_UINT16(hw_cfg_cb.lmp_subversion, p);

                    prtk_usb_patch_file_info = rtk_usb_get_fw_table_entry(hw_cfg_cb.vid, hw_cfg_cb.pid);
                    if((prtk_usb_patch_file_info == NULL) || (prtk_usb_patch_file_info->lmp_sub_default == 0))
                    {
                        ALOGE("get patch entry error");
                        is_proceeding = FALSE;
                        break;
                    }
                    hw_cfg_cb.config_len = rtk_usb_get_bt_config(&hw_cfg_cb.config_buf, prtk_usb_patch_file_info->config_name, prtk_usb_patch_file_info->mac_offset);
                    hw_cfg_cb.fw_len = rtk_get_bt_firmware(&hw_cfg_cb.fw_buf, prtk_usb_patch_file_info->patch_name);
                    rtk_usb_get_fw_version(&hw_cfg_cb);

                    hw_cfg_cb.lmp_subversion_default = prtk_usb_patch_file_info->lmp_sub_default;
                    BTVNDDBG("lmp_subversion = 0x%x hw_cfg_cb.hci_version = 0x%x hw_cfg_cb.hci_revision = 0x%x, hw_cfg_cb.lmp_sub_current = 0x%x", hw_cfg_cb.lmp_subversion, hw_cfg_cb.hci_version, hw_cfg_cb.hci_revision, hw_cfg_cb.lmp_sub_current);

                    if(prtk_usb_patch_file_info->lmp_sub_default == hw_cfg_cb.lmp_subversion)
                    {
                        BTVNDDBG("%s: Cold BT controller startup", __func__);
                        //hw_cfg_cb.state = HW_CFG_START;
                        //goto CFG_USB_START;
                        hw_cfg_cb.state = HW_CFG_READ_ECO_VER;
                        p = (uint8_t *) (p_buf + 1);
                        UINT16_TO_STREAM(p, HCI_VSC_READ_ROM_VERSION);
                        *p++ = 0;
                        p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                        is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_VSC_READ_ROM_VERSION, p_buf, hw_usb_config_cback);
                    }
                    else if (hw_cfg_cb.lmp_subversion != hw_cfg_cb.lmp_sub_current)
                    {
                        BTVNDDBG("%s: Warm BT controller startup with updated lmp", __func__);
                        goto RESET_HW_CONTROLLER;
                    }
                    else
                    {
                        BTVNDDBG("%s: Warm BT controller startup with same lmp", __func__);
                        userial_vendor_usb_ioctl(DWFW_CMPLT, NULL);
                        free(hw_cfg_cb.total_buf);
                        hw_cfg_cb.total_len = 0;

                        bt_vendor_cbacks->dealloc(p_buf);
                        bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                        hw_cfg_cb.state = 0;
                        is_proceeding = TRUE;
                    }

 /*                   if(hw_cfg_cb.lmp_subversion == LMPSUBVERSION_8723a)
                    {
                        hw_cfg_cb.state = HW_CFG_START;
                        goto CFG_USB_START;
                    }
                    else
                    {
                        hw_cfg_cb.state = HW_CFG_READ_ECO_VER;
                        p = (uint8_t *) (p_buf + 1);
                        UINT16_TO_STREAM(p, HCI_VSC_READ_ROM_VERSION);
                        *p++ = 0;
                        p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                        is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_VSC_READ_ROM_VERSION, p_buf, hw_usb_config_cback);
                    }*/
                }
                break;
            }
RESET_HW_CONTROLLER:
            case HW_RESET_CONTROLLER:
            {
                if (status == 0)
                {
                    //usleep(300000);//300ms
                    userial_vendor_usb_ioctl(RESET_CONTROLLER, NULL);//reset controller
                    hw_cfg_cb.state = HW_CFG_READ_ECO_VER;
                    p = (uint8_t *) (p_buf + 1);
                    UINT16_TO_STREAM(p, HCI_VSC_READ_ROM_VERSION);
                    *p++ = 0;
                    p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                    is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_VSC_READ_ROM_VERSION, p_buf, hw_usb_config_cback);
                }
                break;
            }
            case HW_CFG_READ_ECO_VER:
            {
                if(status == 0)
                {
                    hw_cfg_cb.eversion = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPFC6D_EVERSION_OFFSET);
                    BTVNDDBG("hw_usb_config_cback chip_id of the IC:%d", hw_cfg_cb.eversion+1);
                }
                else if(1 == status)
                {
                    hw_cfg_cb.eversion = 0;
                }
                else
                {
                    is_proceeding = FALSE;
                    break;
                }

                hw_cfg_cb.state = HW_CFG_START;
                    goto CFG_USB_START;

            }
CFG_USB_START:
            case HW_CFG_START:
            {
                //get efuse config file and patch code file
                prtk_usb_patch_file_info = rtk_usb_get_fw_table_entry(hw_cfg_cb.vid, hw_cfg_cb.pid);

                if((prtk_usb_patch_file_info == NULL) || (prtk_usb_patch_file_info->lmp_sub_default == 0))
                {
                    ALOGE("get patch entry error");
                    is_proceeding = FALSE;
                    break;
                }
                hw_cfg_cb.max_patch_size = prtk_usb_patch_file_info->max_patch_size;
                hw_cfg_cb.config_len = rtk_usb_get_bt_config(&hw_cfg_cb.config_buf, prtk_usb_patch_file_info->config_name, prtk_usb_patch_file_info->mac_offset);
                if (hw_cfg_cb.config_len)
                {
                    ALOGE("update altsettings");
                    rtk_usb_update_altsettings(prtk_usb_patch_file_info, hw_cfg_cb.config_buf, &(hw_cfg_cb.config_len));
                }

                hw_cfg_cb.fw_len = rtk_get_bt_firmware(&hw_cfg_cb.fw_buf, prtk_usb_patch_file_info->patch_name);
                if (hw_cfg_cb.fw_len < 0)
                {
                    ALOGE("Get BT firmware fail");
                    hw_cfg_cb.fw_len = 0;
                    is_proceeding = FALSE;
                    break;
                }
                else{
                    //hw_cfg_cb.project_id_mask = prtk_usb_patch_file_info->project_id_mask;
                    rtk_usb_get_bt_final_patch(&hw_cfg_cb);
                }

                BTVNDDBG("Check total_len(0x%08x) max_patch_size(0x%08x)", hw_cfg_cb.total_len, hw_cfg_cb.max_patch_size);
                if (hw_cfg_cb.total_len > hw_cfg_cb.max_patch_size)
                {
                    ALOGE("total length of fw&config(0x%08x) larger than max_patch_size(0x%08x)", hw_cfg_cb.total_len, hw_cfg_cb.max_patch_size);
                    is_proceeding = FALSE;
                    break;
                }

                if ((hw_cfg_cb.total_len > 0) && hw_cfg_cb.dl_fw_flag)
                {
                    hw_cfg_cb.patch_frag_cnt = hw_cfg_cb.total_len / PATCH_DATA_FIELD_MAX_SIZE;
                    hw_cfg_cb.patch_frag_tail = hw_cfg_cb.total_len % PATCH_DATA_FIELD_MAX_SIZE;
                    if (hw_cfg_cb.patch_frag_tail)
                        hw_cfg_cb.patch_frag_cnt += 1;
                    else
                        hw_cfg_cb.patch_frag_tail = PATCH_DATA_FIELD_MAX_SIZE;
                    BTVNDDBG("patch fragment count %d, tail len %d", hw_cfg_cb.patch_frag_cnt, hw_cfg_cb.patch_frag_tail);
                }
                else
                {
                    is_proceeding = FALSE;
                    break;
                }

                goto DOWNLOAD_USB_FW;

            }
            /* fall through intentionally */

DOWNLOAD_USB_FW:
            case HW_CFG_DL_FW_PATCH:
                BTVNDDBG("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, opcode:0x%x", status, opcode);

                //recv command complete event for patch code download command
                if(opcode == HCI_VSC_DOWNLOAD_FW_PATCH)
                {
                    iIndexRx = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET + 1);
                    BTVNDDBG("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, iIndexRx:%i", status, iIndexRx);
                    hw_cfg_cb.patch_frag_idx++;

                    if(iIndexRx&0x80)
                    {
                        BTVNDDBG("vendor lib fwcfg completed");
                        userial_vendor_usb_ioctl(DWFW_CMPLT, NULL);
                        free(hw_cfg_cb.total_buf);
                        hw_cfg_cb.total_len = 0;

                        bt_vendor_cbacks->dealloc(p_buf);
                        bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                        hw_cfg_cb.state = 0;
                        is_proceeding = TRUE;
                        break;
                    }
                }

                if (hw_cfg_cb.patch_frag_idx < hw_cfg_cb.patch_frag_cnt)
                {
                    iIndexRx = hw_cfg_cb.patch_frag_idx?((hw_cfg_cb.patch_frag_idx-1)%0x7f+1):0;
                    if (hw_cfg_cb.patch_frag_idx == hw_cfg_cb.patch_frag_cnt - 1)
                    {
                        BTVNDDBG("HW_CFG_DL_FW_PATCH: send last fw fragment");
                        iIndexRx |= 0x80;
                        hw_cfg_cb.patch_frag_len = hw_cfg_cb.patch_frag_tail;
                    }
                    else
                    {
                        iIndexRx &= 0x7F;
                        hw_cfg_cb.patch_frag_len = PATCH_DATA_FIELD_MAX_SIZE;
                    }
                }

                is_proceeding = usb_hci_download_patch_h4(p_buf, iIndexRx,
                                    hw_cfg_cb.total_buf+(hw_cfg_cb.patch_frag_idx*PATCH_DATA_FIELD_MAX_SIZE),
                                    hw_cfg_cb.patch_frag_len);
                break;
            default:
                break;
        } // switch(hw_cfg_cb.state)
    } // if (p_buf != NULL)

    /* Free the RX event buffer */
    if ((bt_vendor_cbacks) && (p_evt_buf != NULL))
        bt_vendor_cbacks->dealloc(p_evt_buf);

    if (is_proceeding == FALSE)
    {
        ALOGE("vendor lib fwcfg aborted!!!");
        if (bt_vendor_cbacks)
        {
            if (p_buf != NULL)
                bt_vendor_cbacks->dealloc(p_buf);

            userial_vendor_usb_ioctl(DWFW_CMPLT, NULL);
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }

        if(hw_cfg_cb.config_len)
        {
            free(hw_cfg_cb.config_buf);
            hw_cfg_cb.config_len = 0;
        }

        if(hw_cfg_cb.fw_len)
        {
            free(hw_cfg_cb.fw_buf);
            hw_cfg_cb.fw_len= 0;
        }

        if(hw_cfg_cb.total_len)
        {
            free(hw_cfg_cb.total_buf);
            hw_cfg_cb.total_len = 0;
        }
        hw_cfg_cb.state = 0;
    }
}

/*******************************************************************************
**
** Function        hw__usb_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_usb_config_start(char transtype, uint32_t usb_id)
{
    RTK_UNUSED(transtype);
    memset(&hw_cfg_cb, 0, sizeof(bt_hw_cfg_cb_t));
    hw_cfg_cb.dl_fw_flag = 1;
    hw_cfg_cb.chip_type = CHIPTYPE_NONE;
    hw_cfg_cb.pid = usb_id & 0x0000ffff;
    hw_cfg_cb.vid = (usb_id>>16) & 0x0000ffff;
    BTVNDDBG("RTKBT_RELEASE_NAME: %s",RTKBT_RELEASE_NAME);
    BTVNDDBG("\nRealtek libbt-vendor_usb Version %s \n",RTK_VERSION);
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    BTVNDDBG("hw_usb_config_start, transtype = 0x%x, pid = 0x%04x, vid = 0x%04x \n", transtype, hw_cfg_cb.pid, hw_cfg_cb.vid);

    if (bt_vendor_cbacks)
    {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_PREAMBLE_SIZE);
        if(p_buf)
        {
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            p = (uint8_t *) (p_buf + 1);

            p = (uint8_t *)(p_buf + 1);
            UINT16_TO_STREAM(p, HCI_VENDOR_RESET);
            *p++ = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            hw_cfg_cb.state = HW_CFG_RESET_CHANNEL_CONTROLLER;
            bt_vendor_cbacks->xmit_cb(HCI_VENDOR_RESET, p_buf, hw_usb_config_cback);
        }
        else {
            ALOGE("%s buffer alloc fail!", __func__);
        }
    }
    else
        ALOGE("%s call back is null", __func__);
}

