/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <fs_mgr.h>
#include "install.h"
#include "ui.h"
#include <dirent.h>
#include "bootloader_message/bootloader_message.h"
#include "recovery_amlogic.h"

#include "ubootenv/set_display_mode.h"

extern "C" {
#include "ubootenv/uboot_env.h"
}

#define LOGE(...) ui_print("E:" __VA_ARGS__)
#define LOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define LOGI(...) fprintf(stdout, "I:" __VA_ARGS__)

static const int MAX_ARGS = 100;
static const int MAX_ARG_LENGTH = 4096;
#define NUM_OF_BLKDEVICE_TO_ENUM    3
#define NUM_OF_PARTITION_TO_ENUM    6

static const char *UDISK_COMMAND_FILE = "/udisk/factory_update_param.aml";
static const char *SDCARD_COMMAND_FILE = "/sdcard/factory_update_param.aml";

void setup_cache_mounts() {
    int ret = 0;
    ret = ensure_path_mounted("/cache");
    if (ret != 0) {
        format_volume("/cache");
    }
}



static int mount_fs_rdonly(char *device_name, Volume *vol, const char *fs_type) {
    if (!mount(device_name, vol->mount_point, fs_type,
        MS_NOATIME | MS_NODEV | MS_NODIRATIME | MS_RDONLY, 0)) {
        LOGW("successful to mount %s on %s by read-only\n",
            device_name, vol->mount_point);
        return 0;
    } else {
        LOGE("failed to mount %s on %s by read-only (%s)\n",
            device_name, vol->mount_point, strerror(errno));
    }

    return -1;
}

int auto_mount_fs(char *device_name, Volume *vol) {
    if (access(device_name, F_OK)) {
        return -1;
    }

    if (!strcmp(vol->fs_type, "auto")) {
        if (!mount(device_name, vol->mount_point, "vfat",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, "")) {
            goto auto_mounted;
        } else {
            if (strstr(vol->mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, vol->mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, vol, "vfat")) {
                    goto auto_mounted;
                }
            }
        }

        if (!mount(device_name, vol->mount_point, "ntfs",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, "")) {
            goto auto_mounted;
        } else {
            if (strstr(vol->mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, vol->mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, vol, "ntfs")) {
                    goto auto_mounted;
                }
            }
        }

        if (!mount(device_name, vol->mount_point, "exfat",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, "")) {
            goto auto_mounted;
        } else {
            if (strstr(vol->mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, vol->mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, vol, "exfat")) {
                    goto auto_mounted;
                }
            }
        }
    } else {
        if(!mount(device_name, vol->mount_point, vol->fs_type,
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, "")) {
            goto auto_mounted;
        } else {
            if (strstr(vol->mount_point, "sdcard")) {
                LOGW("failed to mount %s on %s (%s).try read-only ...\n",
                    device_name, vol->mount_point, strerror(errno));
                if (!mount_fs_rdonly(device_name, vol, vol->fs_type)) {
                    goto auto_mounted;
                }
            }
        }
    }

    return -1;

auto_mounted:
    return 0;
}

int customize_smart_device_mounted(
    Volume *vol) {
    int i = 0, j = 0;
    int first_position = 0;
    int second_position = 0;
    char * tmp = NULL;
    char *mounted_device = NULL;
    char device_name[256] = {0};
    char device_boot[256] = {0};
    const char *usb_device = "/dev/block/sd";
    const char *sdcard_device = "/dev/block/mmcblk";

    if (vol->blk_device != NULL) {
        int num = 0;
        const char *blk_device = vol->blk_device;
        for (; *blk_device != '\0'; blk_device ++) {
            if (*blk_device == '#') {
                num ++;
            }
        }

        /*
        * Contain two '#' for blk_device name in recovery.fstab
        * such as /dev/block/sd## (udisk)
        * such as /dev/block/mmcblk#p# (sdcard)
        */
        if (num != 2) {
            return 1;   // Don't contain two '#'
        }

        if (access(vol->mount_point, F_OK)) {
            if (mkdir(vol->mount_point, 0755) == -1) {
                printf("mkdir %s failed!\n", vol->mount_point);
                return -1;
            }
        }

        // find '#' position
        if (strchr(vol->blk_device, '#')) {
            tmp = strchr(vol->blk_device, '#');
            first_position = tmp - vol->blk_device;
            if (strlen(tmp+1) > 0 && strchr(tmp+1, '#')) {
                tmp = strchr(tmp+1, '#');
                second_position = tmp - vol->blk_device;
            }
        }

        if (!first_position || !second_position) {
            LOGW("decompose blk_device error(%s) in recovery.fstab\n",
                vol->blk_device);
            return -1;
        }

        int copy_len = (strlen(vol->blk_device) < sizeof(device_name)) ?
            strlen(vol->blk_device) : sizeof(device_name);

        for (i = 0; i < NUM_OF_BLKDEVICE_TO_ENUM; i ++) {
            memset(device_name, '\0', sizeof(device_name));
            strncpy(device_name, vol->blk_device, copy_len);

            if (!strncmp(device_name, sdcard_device, strlen(sdcard_device))) {
                // start from '0' for mmcblk0p#
                device_name[first_position] = '0' + i;
            } else if (!strncmp(device_name, usb_device, strlen(usb_device))) {
                // start from 'a' for sda#
                device_name[first_position] = 'a' + i;
            }

            for (j = 1; j <= NUM_OF_PARTITION_TO_ENUM; j ++) {
                device_name[second_position] = '0' + j;
                if (!access(device_name, F_OK)) {
                    LOGW("try mount %s ...\n", device_name);
                    if (!auto_mount_fs(device_name, vol)) {
                        mounted_device = device_name;
                        LOGW("successful to mount %s\n", device_name);
                        goto mounted;
                    }
                }
            }

            if (!strncmp(device_name, sdcard_device, strlen(sdcard_device))) {
                // mmcblk0p1->mmcblk0
                device_name[strlen(device_name) - 2] = '\0';
                sprintf(device_boot, "%s%s", device_name, "boot0");
                // TODO: Here,need to distinguish between cards and flash at best
            } else if (!strncmp(device_name, usb_device, strlen(usb_device))) {
                // sda1->sda
                device_name[strlen(device_name) - 1] = '\0';
            }

            if (!access(device_name, F_OK)) {
                if (strlen(device_boot) && (!access(device_boot, F_OK))) {
                    continue;
                }

                LOGW("try mount %s ...\n", device_name);
                if (!auto_mount_fs(device_name, vol)) {
                    mounted_device = device_name;
                    LOGW("successful to mount %s\n", device_name);
                    goto mounted;
                }
            }
        }
    } else {
        LOGE("Can't get blk_device\n");
    }

    return -1;

mounted:
    return 0;
}

int smart_device_mounted(Volume *vol) {
    int i = 0, len = 0;
    char * tmp = NULL;
    char device_name[256] = {0};
    char *mounted_device = NULL;

    if (mkdir(vol->mount_point, 0755) == -1) {
        printf("mkdir %s failed!\n", vol->mount_point);
        //return -1;
    }

    if (vol->blk_device != NULL) {
        int ret = customize_smart_device_mounted(vol);
        if (ret <= 0) {
            return ret;
        }
    }

    if (vol->blk_device != NULL) {
        tmp = strchr(vol->blk_device, '#');
        len = tmp - vol->blk_device;
        if (tmp && len < 255) {
            strncpy(device_name, vol->blk_device, len);
            for (i = 1; i <= NUM_OF_PARTITION_TO_ENUM; i++) {
                device_name[len] = '0' + i;
                device_name[len + 1] = '\0';
                LOGW("try mount %s ...\n", device_name);
                if (!access(device_name, F_OK)) {
                    if (!auto_mount_fs(device_name, vol)) {
                        mounted_device = device_name;
                        LOGW("successful to mount %s\n", device_name);
                        goto mounted;
                    }
                }
            }

            const char *mmcblk = "/dev/block/mmcblk";
            if (!strncmp(device_name, mmcblk, strlen(mmcblk))) {
                device_name[len - 1] = '\0';
            } else {
                device_name[len] = '\0';
            }

            LOGW("try mount %s ...\n", device_name);
            if (!access(device_name, F_OK)) {
                if (!auto_mount_fs(device_name, vol)) {
                    mounted_device = device_name;
                    LOGW("successful to mount %s\n", device_name);
                    goto mounted;
                }
            }
        } else {
            LOGW("try mount %s ...\n", vol->blk_device);

            //check device name size < buffer size
            if (strlen(vol->blk_device) > 255) {
                printf("the device : %s is too long!\n", vol->blk_device);
                return -1;
            }
            strncpy(device_name, vol->blk_device, strlen(vol->blk_device));
            if (!access(device_name, F_OK)) {
                if (!auto_mount_fs(device_name, vol)) {
                    mounted_device = device_name;
                    LOGW("successful to mount %s\n", device_name);
                    goto mounted;
                }
            }
        }
    }

    return -1;

mounted:
    return 0;
}


//return value
// 0     mount OK
// -1   mount Faile
// 2    ignorel
int ensure_path_mounted_extra(Volume *v) {
    Volume* vUsb = volume_for_mount_point("/udisk");
    char tmp[128] = {0};

    if (strcmp(v->fs_type, "ext4") == 0) {
        if (strstr(v->mount_point, "system")) {
            if (!mount(v->blk_device, v->mount_point, v->fs_type,
                 MS_NOATIME | MS_NODEV | MS_NODIRATIME | MS_RDONLY, "")) {
                 return 0;
            }
        } else {
            if (!mount(v->blk_device, v->mount_point, v->fs_type,
                 MS_NOATIME | MS_NODEV | MS_NODIRATIME, "discard")) {
                 return 0;
            }
        }
        LOGE("failed to mount %s (%s)\n", v->mount_point, strerror(errno));
        return -1;
    } else if (strcmp(v->fs_type, "vfat") == 0 ||
               strcmp(v->fs_type, "auto") == 0 ) {
        if (strstr(v->mount_point, "sdcard") || strstr(v->mount_point, "udisk")) {
            int time_out = 2000000;
            while (time_out) {
                if (!smart_device_mounted(v)) {
                    return 0;
                }
                usleep(100000);
                time_out -= 100000;
            }
        } else {
            if (!mount(v->blk_device, v->mount_point, v->fs_type,
                MS_NOATIME | MS_NODEV | MS_NODIRATIME | MS_RDONLY, "")) {
                return 0;
            }
        }
        LOGE("failed to mount %s (%s)\n", v->mount_point, strerror(errno));
        return -1;
    } else {
        return 2;//not deal
    }
}

void* HdcpThreadLoop(void *) {
    set_display_mode("/etc/mesondisplay.cfg");
    return NULL;
}


// write /proc/sys/vm/watermark_scale_factor 30
//write /proc/sys/vm/min_free_kbytes 12288
int write_sys(const char *path, const char *value, int len) {
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", path);
        return -1;
    }

    int size = write(fd, value, len);
    if (size != len) {
        printf("write %s failed!\n", path);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}
void set_watermark_scale() {
    write_sys("/proc/sys/vm/watermark_scale_factor", "30", strlen("30"));
    write_sys("/proc/sys/vm/min_free_kbytes", "12288", strlen("12288"));
}

void amlogic_init() {
    set_watermark_scale();
    pthread_t input_thread_;
    pthread_create(&input_thread_, nullptr, HdcpThreadLoop, nullptr);
    sleep(1);
}

void amlogic_get_args(std::vector<std::string>& args) {

    if (args.size() == 1) {
        std::string content;
        if (ensure_path_mounted(UDISK_COMMAND_FILE) == 0 &&
            android::base::ReadFileToString(UDISK_COMMAND_FILE, &content)) {

            std::vector<std::string> tokens = android::base::Split(content, "\n");
            for (auto it = tokens.begin(); it != tokens.end(); it++) {
                // Skip empty and '\0'-filled tokens.
                if (!it->empty() && (*it)[0] != '\0') {
                    int size = it->size();
                    if ((*it)[size-1] == '\r') {
                        (*it)[size-1] = '\0';
                    }
                    args.push_back(std::move(*it));
                }
            }
            LOG(INFO) << "Got " << args.size() << " arguments from " << UDISK_COMMAND_FILE;
        }
    }

    if (args.size() == 1) {
        std::string content;
        if (ensure_path_mounted(SDCARD_COMMAND_FILE) == 0 &&
            android::base::ReadFileToString(SDCARD_COMMAND_FILE, &content)) {

            std::vector<std::string> tokens = android::base::Split(content, "\n");
            for (auto it = tokens.begin(); it != tokens.end(); it++) {
                // Skip empty and '\0'-filled tokens.
                if (!it->empty() && (*it)[0] != '\0') {
                    int size = it->size();
                    if ((*it)[size-1] == '\r') {
                        (*it)[size-1] = '\0';
                    }
                    args.push_back(std::move(*it));
                }
            }
            LOG(INFO) << "Got " << args.size() << " arguments from " << SDCARD_COMMAND_FILE;
        }
    }

    if (args.size() == 1) {
        args.push_back(std::move("--show_text"));
    }

}
