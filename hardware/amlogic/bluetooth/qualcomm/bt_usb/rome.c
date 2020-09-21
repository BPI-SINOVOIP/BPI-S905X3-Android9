/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/string.h>  /* for memcpy() */
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/usb.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

struct ath3k_version {
    unsigned int	rom_version;
    unsigned int	build_version;
    unsigned int	ram_version;
    unsigned char	ref_clock;
    unsigned char	reserved[0x07];
};
struct __packed rome1_1_version {
    u8  type;
    u8  length[3];
    u8  sign_ver;
    u8  sign_algo;
    u8  resv1[2];
    u16 product_id;
    u16 build_ver;
    u16 patch_ver;
    u8  resv2[2];
    u32 entry_addr;
};
struct __packed rome2_1_version {
    u8  type;
    u8  length[3];
    u32 total_len;
    u32 patch_len;
    u8  sign_ver;
    u8  sign_algo;
    u8  resv1[2];
    u16 product_id;
    u16 build_ver;
    u16 patch_ver;
    u8  resv2[2];
    u32 entry_addr;
};

#define BTUSB_TEST(fmt, ...) \
    printk(KERN_ERR "Bluetooth: [BTUSB]: " fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, arg...)    printk(KERN_ERR "Bluetooth: " fmt "\n" , ## arg)
#define BT_ERR(fmt, arg...)     printk(KERN_ERR "%s: " fmt "\n" , __func__ , ## arg)
#define BT_DBG                  BTUSB_TEST

#define ROME1_1_USB_CHIP_VERSION        0x101
#define ROME2_1_USB_CHIP_VERSION        0x200
#define ROME3_0_USB_CHIP_VERSION        0x300
#define ROME3_2_USB_CHIP_VERSION        0x302
#define NPL1_0_USB_CHIP_VERSION         0xc0100

#define ROME1_1_USB_RAMPATCH_FILE   "ar3k/rampatch_1.1.img"
#define ROME1_1_USB_NVM_FILE        "ar3k/nvm_tlv_usb_1.1.bin"

#define ROME2_1_USB_RAMPATCH_FILE   "ar3k/rampatch_tlv_usb_2.1.tlv"
#define ROME2_1_USB_NVM_FILE        "ar3k/nvm_tlv_usb_2.1.bin"

#define ROME3_0_USB_RAMPATCH_FILE   "ar3k/rampatch_tlv_usb_3.0.tlv"
#define ROME3_0_USB_NVM_FILE        "ar3k/nvm_tlv_usb_3.0.bin"

#define TF1_1_USB_RAMPATCH_FILE     "ar3k/rampatch_tlv_usb_tf_1.1.tlv"
#define TF1_1_USB_NVM_FILE          "ar3k/nvm_tlv_usb_tf_1.1.bin"

#define NPL1_0_USB_RAMPATCH_FILE    "qca9379/ar3k/rampatch_tlv_usb_npl_1.0.tlv"
#define NPL1_0_USB_NVM_FILE         "qca9379/ar3k/nvm_tlv_usb_npl_1.0.bin"

#define ROME2_1_USB_RAMPATCH_HEADER sizeof(struct rome2_1_version)
#define ROME1_1_USB_RAMPATCH_HEADER sizeof(struct rome1_1_version)

#define ROME1_1_USB_NVM_HEADER      0x04
#define ROME2_1_USB_NVM_HEADER      0x04

#define TF1_1_USB_PRODUCT_ID        0xe500
#define ATH3K_DNLOAD                0x01
#define ATH3K_GETSTATE              0x05
#define ATH3K_GETVERSION            0x09



/* 0x20 : NO FW DOWNLOADED */

#define ATH3K_XTAL_FREQ_26M     0x00
#define ATH3K_XTAL_FREQ_40M     0x01
#define ATH3K_XTAL_FREQ_19P2    0x02
#define ATH3K_NAME_LEN          0xFF
#define ATH3K_PATH_LEN          ( ATH3K_NAME_LEN + 15 /* for absolute path + extra null character */)


#define BULK_SIZE               4096
#define FW_HDR_SIZE             20

#define PATCH_UPDATE_MASK       0x80
#define SYSCFG_UPDATE_MASK      0x40


extern int request_firmware(const struct firmware **firmware_p, const char *name, struct device *device);
extern void release_firmware(const struct firmware *fw);

static bool file_exists(const char *fname)
{
    bool exists = true;
    mm_segment_t old_fs;
    struct file* fp   = NULL;

    old_fs = get_fs();
    set_fs(get_ds());
    fp = filp_open(fname, O_RDONLY, 0);
    set_fs(old_fs);

    if(IS_ERR(fp))
    {
        exists = false;
        BT_ERR(" File [%s]  Error (%ld)\n", fname, PTR_ERR(fp));
    }
    else
    {
        filp_close(fp, NULL);
        BT_INFO(" File [%s]  exists \n", fname);
    }

    return exists;
}

static int ath3k_get_version(struct usb_device *udev,
            struct ath3k_version *version)
{
    int ret, pipe = 0;
    struct ath3k_version *buf;
    const int size = sizeof(*buf);

    buf = kmalloc(size, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    pipe = usb_rcvctrlpipe(udev, 0);
    ret = usb_control_msg(udev, pipe, ATH3K_GETVERSION,
              USB_TYPE_VENDOR | USB_DIR_IN, 0, 0,
              buf, size, USB_CTRL_SET_TIMEOUT);

    memcpy(version, buf, size);
    kfree(buf);

    return ret;
}


int get_rome_version(struct usb_device *udev, struct ath3k_version *version)
{
    struct ath3k_version fw_version;
    int ret = -1;

    if (!version) {
        BT_ERR("NULL output parameters");
        return ret;
    }

    ret = ath3k_get_version(udev, &fw_version);
    if (ret < 0) {
        BT_ERR("Failed to get Rome Firmware version");
        return ret;
    }

    memcpy(version, &fw_version, sizeof(struct ath3k_version));
    return 0;
}

static int ath3k_get_state(struct usb_device *udev, unsigned char *state)
{
    int ret, pipe = 0;
    char *buf;

    buf = kmalloc(sizeof(*buf), GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    pipe = usb_rcvctrlpipe(udev, 0);
    ret = usb_control_msg(udev, pipe, ATH3K_GETSTATE,
                  USB_TYPE_VENDOR | USB_DIR_IN, 0, 0,
                  buf, sizeof(*buf), USB_CTRL_SET_TIMEOUT);

    *state = *buf;
    kfree(buf);

    return ret;
}

static int ath3k_load_fwfile(struct usb_device *udev,
        const struct firmware *firmware, int header_h)
{
    u8 *send_buf;
    int err, pipe, len, size, count, sent = 0;
    int ret;

    count = firmware->size;

    send_buf = kmalloc(BULK_SIZE, GFP_KERNEL);
    if (!send_buf) {
        BT_ERR("Can't allocate memory chunk for firmware");
        return -ENOMEM;
    }

    size = min_t(uint, count, header_h);
    memcpy(send_buf, firmware->data, size);

    pipe = usb_sndctrlpipe(udev, 0);
    ret = usb_control_msg(udev, pipe, ATH3K_DNLOAD,
            USB_TYPE_VENDOR, 0, 0, send_buf,
            size, USB_CTRL_SET_TIMEOUT);
    if (ret < 0) {
        BT_ERR("Can't change to loading configuration err");
        kfree(send_buf);
        return ret;
    }

    sent += size;
    count -= size;

    while (count) {
        size = min_t(uint, count, BULK_SIZE);
        pipe = usb_sndbulkpipe(udev, 0x02);

        memcpy(send_buf, firmware->data + sent, size);

        err = usb_bulk_msg(udev, pipe, send_buf, size,
                    &len, 3000);
        if (err || (len != size)) {
            BT_ERR("Error in firmware loading err = %d,"
                "len = %d, size = %d", err, len, size);
            kfree(send_buf);
            return err;
        }
        sent  += size;
        count -= size;
    }

    kfree(send_buf);
    return 0;
}



static int ath3k_load_patch(struct usb_device *udev,
                        struct ath3k_version *version)
{
    unsigned char fw_state;
    char filename[ATH3K_NAME_LEN] = {0};
    const struct firmware *firmware;
    struct ath3k_version pt_version;
    struct rome2_1_version *rome2_1_version;
    struct rome1_1_version *rome1_1_version;
    int ret;
    char file_path[ATH3K_PATH_LEN] = {0};

    BT_INFO("%s: Get FW STATE prior to downloading the RAMAPCTH\n", __func__);
    printk(KERN_ERR "%s: Get FW STATE prior to downloading the RAMAPCTH\n", __func__);
    ret = ath3k_get_state(udev, &fw_state);
    if (ret < 0) {
        BT_ERR("Can't get state to change to load ram patch err");
        printk(KERN_ERR "%s: Can't get state to change to load ram patch err\n", __func__);
        return ret;
    }

    if (fw_state & PATCH_UPDATE_MASK) {
            BT_INFO("%s: Patch already downloaded(fw_state: %d)", __func__,
            fw_state);
            printk(KERN_ERR "%s: Patch already downloaded(fw_state: %d)\n", __func__, fw_state);
            return 0;
    }
    else
    {
        BT_INFO("%s: Downloading RamPatch(fw_state: %d)\n", __func__,
            fw_state);
        printk(KERN_ERR "%s: Downloading RamPatch(fw_state: %d)\n", __func__, fw_state);
    }

    switch (version->rom_version) {

    case ROME1_1_USB_CHIP_VERSION:
        BT_DBG("Chip Detected as ROME1.1");
        snprintf(filename, ATH3K_NAME_LEN, ROME1_1_USB_RAMPATCH_FILE);
        break;

    case ROME2_1_USB_CHIP_VERSION:
        BT_DBG("Chip Detected as ROME2.1");
        snprintf(filename, ATH3K_NAME_LEN, ROME2_1_USB_RAMPATCH_FILE);
        break;

    case ROME3_0_USB_CHIP_VERSION:
        BT_DBG("Chip Detected as ROME3.0");
        printk(KERN_ERR "%s: Chip Detected as ROME3.0\n", __func__);
        snprintf(filename, ATH3K_NAME_LEN, ROME3_0_USB_RAMPATCH_FILE);
        break;

    case ROME3_2_USB_CHIP_VERSION:
        if (udev->descriptor.idProduct == TF1_1_USB_PRODUCT_ID) {
            BT_DBG("Chip Detected as TF1.1");
            snprintf(filename, ATH3K_NAME_LEN,
                TF1_1_USB_RAMPATCH_FILE);
        }
        else
        {
            BT_INFO("Unsupported Chip");
            return -ENODEV;
        }
        break;

    case NPL1_0_USB_CHIP_VERSION:
        BT_DBG("Chip Detected as Naples1.0");
        snprintf(filename, ATH3K_NAME_LEN, NPL1_0_USB_RAMPATCH_FILE);
        break;

    default:
        BT_DBG("Chip Detected as Ath3k");
        snprintf(filename, ATH3K_NAME_LEN, "ar3k/AthrBT_0x%08x.dfu",
        version->rom_version);
        break;
    }

    snprintf(file_path,ATH3K_PATH_LEN,"/vendor/firmware/%s", filename);

    if ( false == file_exists(file_path) )
    {
        ret = -EEXIST;
        BT_ERR("File(%s) Error \n", file_path);
        return ret;
    }

    ret = request_firmware(&firmware, filename, &udev->dev);

    if (ret < 0) {
        BT_ERR("Patch file not found %s", filename);
        printk(KERN_ERR "%s: Patch file not found %s\n", __func__, filename);
        return ret;
    }
    else
    {
        printk(KERN_ERR "%s: Find Patch file %s\n", __func__, filename);
    }

    if ((version->rom_version == ROME2_1_USB_CHIP_VERSION) ||
        (version->rom_version == ROME3_0_USB_CHIP_VERSION) ||
        (version->rom_version == ROME3_2_USB_CHIP_VERSION) ||
        (version->rom_version == NPL1_0_USB_CHIP_VERSION)) {
        rome2_1_version = (struct rome2_1_version *) firmware->data;
        pt_version.rom_version = rome2_1_version->build_ver;
        pt_version.build_version = rome2_1_version->patch_ver;
        BT_DBG("pt_ver.rome_ver : 0x%x\n", pt_version.rom_version);
        BT_DBG("pt_ver.build_ver: 0x%x\n", pt_version.build_version);
        BT_DBG("fw_ver.rom_ver: 0x%x\n", version->rom_version);
        BT_DBG("fw_ver.build_ver: 0x%x\n", version->build_version);
    } else if (version->rom_version == ROME1_1_USB_CHIP_VERSION) {
        rome1_1_version = (struct rome1_1_version *) firmware->data;
        pt_version.build_version = rome1_1_version->build_ver;
        pt_version.rom_version = rome1_1_version->patch_ver;
        BT_DBG("pt_ver.rom1.1_ver : 0x%x", pt_version.rom_version);
        BT_DBG("pt_ver.build1.1_ver: 0x%x", pt_version.build_version);
        BT_DBG("fw_ver.rom1.1_ver: 0x%x", version->rom_version);
        BT_DBG("fw_ver.build1.1_ver: 0x%x", version->build_version);
    } else {
    pt_version.rom_version = *(int *)(firmware->data + firmware->size - 8);
    pt_version.build_version = *(int *)(firmware->data + firmware->size - 4);
    }

    if ((pt_version.rom_version != (version->rom_version & 0xffff)) ||
        (pt_version.build_version <= version->build_version)) {
        BT_ERR("Patch file version did not match with firmware\n");
        release_firmware(firmware);
        return -EINVAL;
    }

    if ((version->rom_version == ROME2_1_USB_CHIP_VERSION) ||
        (version->rom_version == ROME3_0_USB_CHIP_VERSION) ||
        (version->rom_version == ROME3_2_USB_CHIP_VERSION) ||
        (version->rom_version == NPL1_0_USB_CHIP_VERSION)) {
        BT_ERR("%s: Loading RAMPATCH...", __func__);
        ret = ath3k_load_fwfile(udev, firmware,
                        ROME2_1_USB_RAMPATCH_HEADER);
        }
    else if (version->rom_version == ROME1_1_USB_CHIP_VERSION)
        ret = ath3k_load_fwfile(udev, firmware, ROME1_1_USB_RAMPATCH_HEADER);
    else
        ret = ath3k_load_fwfile(udev, firmware, FW_HDR_SIZE);

    release_firmware(firmware);

    BT_DBG("%s: DONE Downloading RamPatch", __func__);

    return ret;
}

static int ath3k_load_syscfg(struct usb_device *udev,
                        struct ath3k_version *version)
{
    unsigned char fw_state;
    char filename[ATH3K_NAME_LEN] = {0};
    const struct firmware *firmware;
    int clk_value, ret;
    char file_path[ATH3K_PATH_LEN] = {0};

    BT_INFO("%s: Get FW STATE prior to downloading the NVM\n", __func__);
    ret = ath3k_get_state(udev, &fw_state);
    if (ret < 0) {
        BT_ERR("Can't get state to change to load configuration err");
        return -EBUSY;
    }

    if (fw_state & SYSCFG_UPDATE_MASK) {
        BT_INFO("%s: NVM already downloaded(fw_state: %d)\n", __func__,
            fw_state);
        return 0;
    } else
        BT_INFO("%s: Downloading NVM(fw_state: %d)\n", __func__, fw_state);

    switch (version->ref_clock) {
    case ATH3K_XTAL_FREQ_26M:
        clk_value = 26;
        break;
    case ATH3K_XTAL_FREQ_40M:
        clk_value = 40;
        break;
    case ATH3K_XTAL_FREQ_19P2:
        clk_value = 19;
        break;
    default:
        clk_value = 0;
        break;
    }

    if (version->rom_version == ROME2_1_USB_CHIP_VERSION)
        snprintf(filename, ATH3K_NAME_LEN, ROME2_1_USB_NVM_FILE);
    else if (version->rom_version == ROME3_0_USB_CHIP_VERSION)
        snprintf(filename, ATH3K_NAME_LEN, ROME3_0_USB_NVM_FILE);
    else if (version->rom_version == ROME3_2_USB_CHIP_VERSION)
    {
        if (udev->descriptor.idProduct == TF1_1_USB_PRODUCT_ID)
            snprintf(filename, ATH3K_NAME_LEN, TF1_1_USB_NVM_FILE);
        else
        {
            BT_INFO("Unsupported Chip");
            return -ENODEV;
        }
    }
    else if (version->rom_version == ROME1_1_USB_CHIP_VERSION)
        snprintf(filename, ATH3K_NAME_LEN, ROME1_1_USB_NVM_FILE);
    else if (version->rom_version == NPL1_0_USB_CHIP_VERSION)
        snprintf(filename, ATH3K_NAME_LEN, NPL1_0_USB_NVM_FILE);
    else
        snprintf(filename, ATH3K_NAME_LEN, "ar3k/ramps_0x%08x_%d%s",
            version->rom_version, clk_value, ".dfu");

    snprintf(file_path,ATH3K_PATH_LEN,"/vendor/firmware/%s", filename);

    if ( false == file_exists(file_path) )
    {
        ret = -EEXIST;
        BT_ERR("File(%s) Error \n", file_path);
        return ret;
    }

    ret = request_firmware(&firmware, filename, &udev->dev);
    if (ret < 0) {
        BT_ERR("Configuration file not found %s", filename);
        return ret;
    }
    else
    {
        printk(KERN_ERR "%s: Configuration file found %s\n", __func__, filename);
    }

    if ((version->rom_version == ROME2_1_USB_CHIP_VERSION) ||
        (version->rom_version == ROME3_0_USB_CHIP_VERSION) ||
        (version->rom_version == ROME3_2_USB_CHIP_VERSION) ||
        (version->rom_version == NPL1_0_USB_CHIP_VERSION)) {
        BT_ERR("%s: Loading NVM...", __func__);
        ret = ath3k_load_fwfile(udev, firmware, ROME2_1_USB_NVM_HEADER);
        }
    else if (version->rom_version == ROME1_1_USB_CHIP_VERSION)
        ret = ath3k_load_fwfile(udev, firmware, ROME1_1_USB_NVM_HEADER);
    else
        ret = ath3k_load_fwfile(udev, firmware, FW_HDR_SIZE);

    release_firmware(firmware);

    BT_DBG("%s: DONE: Downloading NVM\n", __func__);

    return ret;
}


int rome_download(struct usb_device *udev, struct ath3k_version *version)
{
    int ret;

    ret = ath3k_load_patch(udev, version);
    if (ret < 0) {
        BT_ERR("Loading patch file failed");
        printk(KERN_ERR "%s: Loading patch file failed\n", __func__);
        return ret;
    }
    else
    {
        printk(KERN_ERR "%s: Succeed Loading patch file\n", __func__);
    }

    ret = ath3k_load_syscfg(udev, version);

    if (ret < 0) {
        BT_ERR("Loading sysconfig file failed");
        printk(KERN_ERR "%s: Loading sysconfig file failed\n", __func__);
        return ret;
    }
    else
    {
        printk(KERN_ERR "%s: Succeed Loading sysconfig file\n", __func__);
    }

    return 0;
}

void btusb_firmware_upgrade(struct usb_device *udev)
{

    struct ath3k_version version;
    int err;

    printk(KERN_ERR "%s: Start Downloading Rampatch and NVM\n", __func__);
    err = get_rome_version(udev, &version);
    if (err < 0) {
        BT_ERR("Failed to get ROME USB version");
        printk(KERN_ERR "%s: Failed to get ROME USB version\n", __func__);
    }
    BT_INFO("Rome Version: 0x%x\n", version.rom_version);
    err = rome_download(udev, &version);
    if (err < 0) {
        BT_ERR("Failed to download ROME firmware");
        printk(KERN_ERR "%s: \n", __func__);
        printk(KERN_ERR "%s: Failed to download ROME firmware\n", __func__);
    }
    else
    {
        printk(KERN_ERR "%s: Succeed Downloading Rampatch and NVM\n", __func__);
    }
}
