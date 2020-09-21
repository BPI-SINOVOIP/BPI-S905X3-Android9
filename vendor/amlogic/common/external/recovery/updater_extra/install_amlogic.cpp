/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <selinux/selinux.h>
#include <ftw.h>
#include <sys/capability.h>
#include <sys/xattr.h>
#include <linux/xattr.h>
#include <inttypes.h>
#include <ziparchive/zip_archive.h>

#include <memory>
#include <vector>

#include "bootloader.h"
#include "cutils/android_reboot.h"
#include "cutils/properties.h"
#include "edify/expr.h"
#include "error_code.h"
#include "updater/updater.h"
#include "check/dtbcheck.h"
#include "ubootenv/uboot_env.h"

#include "roots.h"
#include <bootloader_message/bootloader_message.h>
#include <fs_mgr.h>

#include "common.h"
#include "device.h"

#include "ui.h"
#include <dirent.h>

#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#define ARRAY_SIZE(x)  sizeof(x)/sizeof(x[0])
#define EMMC_USER_PARTITION        "bootloader"
#define EMMC_BLK0BOOT0_PARTITION   "mmcblk0boot0"
#define EMMC_BLK0BOOT1_PARTITION   "mmcblk0boot1"
#define EMMC_BLK1BOOT0_PARTITION   "mmcblk1boot0"
#define EMMC_BLK1BOOT1_PARTITION   "mmcblk1boot1"
#define COMMAND_FILE "/cache/recovery/command"
#define CACHE_ROOT "/cache"

#define RECOVERY_IMG "recovery.img"
#define UNCRYPT_FILE "/cache/recovery/uncrypt_file"
#define RECOVERY_BACKUP "/cache/recovery/recovery.img"
#define UPDATE_TMP_FILE "/cache/update_tmp.zip"
#define DEV_DATA                "/dev/block/data"
#define DEV_EMMC                "/dev/block/mmcblk0"

#define PATH_KEY_CDEV		  "/dev/unifykeys"
#define PARAM_FIRMWARE         "/param/firmware.le"
#define UNIFYKEY_HDCP_FW             "hdcp22_rx_fw"
#define UNIFYKEY_HDCP_PRIVATE    "hdcp22_rx_private"
#define KEY_UNIFY_NAME_LEN	         (48)
#define KEY_HDCP_OFFSET	  (10*1024)
#define KEY_HDCP_SIZE	         (2080)
/*do not use those anymore*/
#define KEYUNIFY_ATTACH         _IO('f', 0x60)
#define KEYUNIFY_GET_INFO     _IO('f', 0x62)

enum emmcPartition {
    USER = 0,
    BLK0BOOT0,
    BLK0BOOT1,
    BLK1BOOT0,
    BLK1BOOT1,
};

static int sEmmcPartionIndex = -1;
static const char *sEmmcPartionName[] = {
    EMMC_USER_PARTITION,
    EMMC_BLK0BOOT0_PARTITION,
    EMMC_BLK0BOOT1_PARTITION,
    EMMC_BLK1BOOT0_PARTITION,
    EMMC_BLK1BOOT1_PARTITION,
};

int wipe_flag = 0;

/* for ioctrl transfer paramters. */
struct key_item_info_t {
    unsigned int id;
    char name[KEY_UNIFY_NAME_LEN];
    unsigned int size;
    unsigned int permit;
    unsigned int flag;        /*bit 0: 1 exsit, 0-none;*/
    unsigned int reserve;
};

int RecoverySecureCheck(const ZipArchiveHandle zipArchive);
int RecoveryDtbCheck(const ZipArchiveHandle zipArchive);
int GetEnvPartitionOffset(const ZipArchiveHandle za);
/*
 * return value: 0 if no error; 1 if path not existed, -1 if access failed
 *
 */
static int read_sysfs_val(const char* path, char* rBuf, const unsigned bufSz, int * readCnt)
{
    int ret = 0;
    int fd  = -1;
    int count = 0;

    if (access(path, F_OK)) {
            printf("path[%s] not existed\n", path);
            return 1;
    }
    if (access(path, R_OK)) {
            printf("path[%s] cannot read\n", path);
            return -1;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
            printf("fail in open[%s] in O_RDONLY\n", path);
            goto _exit;
    }

    count = read(fd, rBuf, bufSz);
    if (count <= 0) {
            printf("read %s failed (count:%d)\n",
                            path, count);
            close(fd);
            return -1;
    }
    *readCnt = count;

    ret = 0;
_exit:
    if (fd >= 0) close(fd);
    return ret;
}

static int getBootloaderOffset(int* bootloaderOffset)
{
    const char* PathBlOff = "/sys/class/aml_store/bl_off_bytes" ;
    int             iret  = 0;
    char  buf[16]         = { 0 };
    int           readCnt = 0;

    iret = read_sysfs_val(PathBlOff, buf, 15, &readCnt);
    if (iret < 0) {
            printf("fail when read path[%s]\n", PathBlOff);
            return __LINE__;
    }
    buf[readCnt] = 0;
    *bootloaderOffset = atoi(buf);
    printf("bootloaderOffset is %s\n", buf);

    return 0;
}

static int _mmcblOffBytes = 0;


static int write_data(int fd, const char *data, ssize_t len)
{
    ssize_t size = len;
    char *verify = NULL;

    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos == -1) {
        fprintf(stderr, "lseek failed to get cur offset (%s)\n",  strerror(errno));
        return -1;
    }

    fprintf(stderr, "data len = %d, pos = %ld\n", len, pos);
    if (write(fd, data, len) != len) {
        fprintf(stderr, " write error at 0x%08lx (%s)\n",pos, strerror(errno));
        return -1;
    }

    verify = (char *)malloc(size);
    if (verify == NULL) {
        fprintf(stderr, "block: failed to malloc size=%u (%s)\n", size, strerror(errno));
        return -1;
    }

    if ((lseek(fd, pos, SEEK_SET) != pos) ||(read(fd, verify, size) != size)) {
        fprintf(stderr, "block: re-read error at 0x%08lx (%s)\n",pos, strerror(errno));
        if (verify) {
            free(verify);
        }
        return -1;
    }

    if (memcmp(data, verify, size) != 0) {
        fprintf(stderr, "block: verification error at 0x%08lx (%s)\n",pos, strerror(errno));
        if (verify) {
            free(verify);
        }
        return -1;
    }

    fprintf(stderr, "successfully wrote data at %ld\n", pos);
    if (verify) {
        free(verify);
    }

    return len;
}


int do_cache_sync(void) {

    int fd = 0;
    int ret = 0;

    //umount /cache
    ret = umount("/cache");
    if (ret != 0) {
        fprintf(stderr, "umount cache failed\n", strerror(errno));
    }

    fd = open("/dev/block/cache", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s failed (%s)\n","/dev/block/cache", strerror(errno));
        return -1;
    }

    FILE *fp = fdopen(fd, "r+");
    if (fp == NULL) {
        printf("fdopen failed!\n");
        close(fd);
        return -1;
    }

    fflush(fp);
    fsync(fd);
    fclose(fp);

    ret = mount("/dev/block/cache", "/cache", "ext4",\
        MS_NOATIME | MS_NODEV | MS_NODIRATIME,"discard");
    if (ret < 0 ) {
        fprintf(stderr, "mount cache failed (%s)\n","/dev/block/cache", strerror(errno));
    }

    return 0;
}

//return value
// -1  :   failed
//  0   :   success
static int backup_partition_data(const char *name,const char *dir, long offset) {
    int ret = 0;
    int fd = 0;
    FILE *fp = NULL;
    int sor_fd = -1;
    int dst_fd = -1;
    ssize_t wrote = 0;
    ssize_t readed = 0;
    char devpath[128] = {0};
    char dstpath[128] = {0};
    const int BUFFER_MAX =  32*1024*1024;   //Max support 32*M
    printf("backup partition name:%s, to dir:%s\n", name, dir);

    if ((name == NULL) || (dir == NULL)) {
        fprintf(stderr, "name(%s) or dir(%s) is NULL!\n", name, dir);
        return -1;
    }

    if (!strcmp(name, "dtb")) {//dtb is char device
        sprintf(devpath, "/dev/%s", name);
    } else {
        sprintf(devpath, "/dev/block/%s", name);
    }

    sprintf(dstpath, "%s%s.img", dir, name);

    sor_fd = open(devpath, O_RDONLY);
    if (sor_fd < 0) {
        fprintf(stderr, "open %s failed (%s)\n",devpath, strerror(errno));
        return -1;
    }

    dst_fd = open(dstpath, O_WRONLY | O_CREAT, 00777);
    if (dst_fd < 0) {
        fprintf(stderr, "open %s failed (%s)\n",dstpath, strerror(errno));
        close(sor_fd);
        return -1;
    }

    char* buffer = (char *)malloc(BUFFER_MAX);
    if (buffer == NULL) {
        fprintf(stderr, "can't malloc %d buffer!\n", BUFFER_MAX);
        goto err_out;
    }

    if (strcmp(name, "dtb")) {
        ret = lseek(sor_fd, offset, SEEK_SET);
        if (ret == -1) {
            printf("failed to lseek %d\n", offset);
            goto err_out;
        }
    }

    readed = read(sor_fd, buffer, BUFFER_MAX);
    if (readed <= 0) {
        fprintf(stderr, "read failed read:%d!\n", readed);
        goto err_out;
    }

    wrote = write(dst_fd, buffer, readed);
    if (wrote != readed) {
        fprintf(stderr, "write %s failed (%s)\n",dstpath, strerror(errno));
        goto err_out;
    }

    close(dst_fd);
    close(sor_fd);
    free(buffer);
    buffer = NULL;

    ret = do_cache_sync();
    if (ret < 0) {
        printf("do sync for cache partition failed!\n");
    }

    return 0;


err_out:
    if (sor_fd >= 0) {
        close(sor_fd);
    }

    if (dst_fd >= 0) {
        close(dst_fd);
    }

    if (buffer) {
        free(buffer);
        buffer = NULL;
    }

    return -1;

}


static ssize_t write_chrdev_data(const char *dev, const char *data, const ssize_t size)
{
    int fd = -1;
    ssize_t wrote = 0;
    ssize_t readed = 0;
    char *verify = NULL;

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s failed (%s)\n", dev, strerror(errno));
        return -1;
    }

    fprintf(stderr, "data len = %d\n", size);
    if ((wrote = write(fd, data, size)) != size) {
        fprintf(stderr, "wrote error, count %d (%s)\n",wrote, strerror(errno));
        goto err;
    }

    fsync(fd);
    close(fd);
    sync();

    fd = open(dev, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s failed after wrote success (%s)\n", dev, strerror(errno));
        return -1;
    }

    verify = (char *)malloc(256*1024);
    if (verify == NULL) {
        fprintf(stderr, "failed to malloc size=%d (%s)\n", size, strerror(errno));
        goto err;
    }

    memset(verify, 0, 256*1024);

    if ((readed = read(fd, verify, size)) != size) {
        fprintf(stderr, "readed error, count %d (%s)\n", readed, strerror(errno));
        if (verify != NULL) {
            free(verify);
        }
        goto err;
    }

    if (memcmp(data, verify, size) != 0) {
        fprintf(stderr, "verification error, wrote != readed\n");
        if (verify != NULL) {
            free(verify);
        }
        goto err;
    }

    fprintf(stderr, " successfully wrote data\n");
    if (verify != NULL) {
        free(verify);
    }

    close(fd);
    return wrote;

err:
    close(fd);
    return -1;
}

int block_write_data( const std::string& args, off_t offset) {
    int fd = -1;
    int result = 0;
    bool success = false;
    char * tmp_name = NULL;
    char devname[64] = {0};

    memset(devname, 0, sizeof(devname));
    sprintf(devname, "/dev/%s", "bootloader");  //nand partition
    fd = open(devname, O_RDWR);
    if (fd < 0) {
        memset(devname, 0, sizeof(devname));
        // emmc user, boot0, boot1 partition
        sprintf(devname, "/dev/block/%s", sEmmcPartionName[sEmmcPartionIndex]);
        fd = open(devname, O_RDWR);
        if (fd < 0) {
            tmp_name = "mtdblock0";
            memset(devname, 0, sizeof(devname));
            sprintf(devname, "/dev/block/%s", tmp_name); //spi partition
            fd = open(devname, O_RDWR);
            if (fd < 0) {
                printf("failed to open %s\n", devname);
                return -1;
            }
        }

        printf("start to write bootloader to %s...\n", devname);
        result =lseek(fd, offset, SEEK_SET);//seek to skip mmc area since gxl
        if (result == -1) {
            printf("failed to lseek %d\n", offset);
            goto done;
        }
        ssize_t wrote = write_data(fd, args.c_str(), args.size());
        success = (wrote == args.size());


        if (!success) {
            fprintf(stderr, "write_data to %s partition failed: %s\n", devname, strerror(errno));
        } else {
            printf("write_data to %s partition successful\n", devname);
        }
    } else {
        printf("start to write bootloader to %s...\n", devname);
        success = true;
        int size =  args.size();
        result = lseek(fd, offset, SEEK_SET);//need seek one sector to skip MBR area since gxl
        if (result == -1) {
            printf("failed to lseek %d\n", offset);
            goto done;
        }

        fprintf(stderr, "size = %d offset = %ld\n", size, offset);
        if (write(fd, args.c_str(), size) != size) {
            fprintf(stderr, " write error at offset :%ld (%s)\n",offset, strerror(errno));
            success = false;
        }

        if (!success) {
            fprintf(stderr, "write_data to %s partition failed: %s\n", devname, strerror(errno));
        } else {
            printf("write_data to %s partition successful\n", devname);
        }
    }
    close(fd);

    result = success ? 0 : -1;
    return result;

done:
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    return -1;
}


Value* WriteBootloaderImageFn(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    int iRet = 0;

    if (argv.size() != 1) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects at least 1 arg", name);
    }

    std::vector<std::unique_ptr<Value>> args;
    if (!ReadValueArgs(state, argv, &args)) {
        return nullptr;
    }

    if ((args[0]->type == VAL_STRING || (args[0]->data.size())) == 0) {
        ErrorAbort(state, kArgsParsingFailure, "file argument to %s can't be empty", name);
        return nullptr;
    }

    iRet = getBootloaderOffset(&_mmcblOffBytes);
    if (iRet) {
        printf("Fail in getBootloaderOffset, ret=%d\n", iRet);
        return StringValue("bootloader err");
    }

    unsigned int i;
    char emmcPartitionPath[128];
    for (i = BLK0BOOT0; i < ARRAY_SIZE(sEmmcPartionName); i ++) {
        memset(emmcPartitionPath, 0, sizeof(emmcPartitionPath));
        sprintf(emmcPartitionPath, "/dev/block/%s", sEmmcPartionName[i]);
        if (!access(emmcPartitionPath, F_OK)) {
            sEmmcPartionIndex = i;
            iRet = block_write_data(args[0]->data, _mmcblOffBytes);
            if (iRet == 0) {
                printf("Write Uboot Image to %s successful!\n\n", sEmmcPartionName[sEmmcPartionIndex]);
            } else {
                printf("Write Uboot Image to %s failed!\n\n", sEmmcPartionName[sEmmcPartionIndex]);
                printf("iRet= %d, exit !!!\n", iRet);
                return ErrorAbort(state, kFwriteFailure, "%s() update bootloader", name);
            }
        }
    }

    sEmmcPartionIndex = USER;
    iRet = block_write_data(args[0]->data, _mmcblOffBytes);
    if (iRet == 0) {
        printf("Write Uboot Image successful!\n\n");
    } else {
        printf("Write Uboot Image failed!\n\n");
        printf("iRet= %d, exit !!!\n", iRet);
        return ErrorAbort(state, kFwriteFailure, "%s() update bootloader", name);
    }

    return StringValue("bootloader");
}

Value* WriteDtbImageFn(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    bool success = false;
    const char *DTB_DEV=  "/dev/dtb";
    const int DTB_DATA_MAX =  256*1024;// write 256K dtb datas to dtb device maximum,kernel limit

    if (argv.size() != 1) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects at least 1 arg", name);
    }

    std::vector<std::unique_ptr<Value>> args;

    if (!ReadValueArgs(state, argv, &args)) {
        return nullptr;
    }

    if (args[0]->type == VAL_INVALID) {
        return StringValue("");
    }

    fprintf(stderr, "\nstart to write dtb.img to %s...\n", DTB_DEV);
    if (args[0]->type == VAL_BLOB) {
        fprintf(stderr, "contents type: VAL_BLOB\ncontents size: %d\n", args[0]->data.size());
        if (!args[0]->data.c_str() || -1 == args[0]->data.size()) {
            fprintf(stderr, "#ERR:BLOb Data extracted FAILED for dtb\n");
            success = false;
        } else {
            if (args[0]->data.size() > DTB_DATA_MAX) {
                fprintf(stderr, "data size(%d) out of range size(max:%d)\n", args[0]->data.size(), DTB_DATA_MAX);
                return StringValue("");
            }
            ssize_t wrote = write_chrdev_data(DTB_DEV, args[0]->data.c_str(), args[0]->data.size());
            success = (wrote == args[0]->data.size());
        }
    }

    if (!success) {
        return ErrorAbort(state, kFwriteFailure, "%s() update dtb failed", name);
    } else {
        return StringValue("dtb");
    }
}

Value* SetBootloaderEnvFn(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    int ret = 0;
    if (argv.size() != 2) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects 2 args, got %zu", name,argv.size());
    }

    std::vector<std::string> args;
    if (!ReadArgs(state, argv, &args)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed to parse the argument(s)", name);
    }

    const std::string& env_name = args[0];
    const std::string& env_val = args[1];

    if ((env_name.size() == 0) || (env_val.size() == 0)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed, one of the argument(s) is null ", name);
    }

    ret = set_bootloader_env(env_name.c_str(), env_val.c_str());
    printf("setenv %s %s %s.(%d)\n", env_name.c_str(), env_val.c_str(), (ret < 0) ? "failed" : "successful", ret);
    if (!ret) {
        return StringValue("success");
    } else {
        return StringValue("");
    }
}


Value* OtaZipCheck(const char* name, State* state,
                           const std::vector<std::unique_ptr<Expr>>&argv) {
    int check = 0;
    ZipArchiveHandle za = static_cast<UpdaterInfo*>(state->cookie)->package_zip;

    printf("\n-- Secure Check...\n");

    check = RecoverySecureCheck(za);
    if (check <= 0) {
        return ErrorAbort(state, kArgsParsingFailure, "Secure check failed. %s\n\n", !check ? "(Not match)" : "");
    } else if (check == 1) {
        printf("Secure check complete.\n\n");
    }

#ifndef RECOVERY_DISABLE_DTB_CHECK
    printf("\n-- Dtb Check...\n");

    check = RecoveryDtbCheck(za);
    if (check != 0) {
        if (check > 1) {
            if (check == 3) {
                printf("data offset changed, need wipe_data\n\n");
                wipe_flag = 1;

                FILE *pf = fopen("/cache/recovery/stage", "w+");
                if (pf != NULL) {
                    printf("write /cache/recovery/stage \n\n");
                    int len = fwrite("2", 1, strlen("2"), pf);
                    printf("stage write len:%d, 2\n", len);
                    fflush(pf);
                    fclose(pf);
                }
            }
            printf("dtb check not match, but can upgrade by two step.\n\n");
            return StringValue(strdup("1"));
        }
        return ErrorAbort(state, kArgsParsingFailure, "Dtb check failed. Not match\n");
    } else {
        printf("dtb check complete.\n\n");
    }
#endif
    return StringValue(strdup("0"));
}

Value* BackupDataCache(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    int ret = 0;
    if (argv.size() != 2) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects 2 args, got %zu", name, argv.size());
    }

    std::vector<std::string> args;
    if (!ReadArgs(state, argv, &args)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed to parse the argument(s)", name);
    }

    const std::string& partition = args[0];
    const std::string& destination = args[1];

    ret = backup_partition_data(partition.c_str(), destination.c_str(), 0);
    if (ret != 0) {
        printf("backup %s to %s , failed!\n", partition.c_str(), destination.c_str());
    } else {
        printf("backup %s to %s , success!\n", partition.c_str(), destination.c_str());
    }

    return StringValue("backup");
}

Value* RebootRecovery(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    struct bootloader_message boot {};
    std::string err;

    read_bootloader_message(&boot,  &err);

    printf("boot.command: %s\n", boot.command);
    printf("boot.recovery: %s\n", boot.recovery);

    if (strstr(boot.recovery, "--update_package=")) {
        printf("check bootloader_message \n");
        if (wipe_flag == 1) {
            printf("need wipe data \n");
            strlcat(boot.recovery, "--wipe_data\n", sizeof(boot.recovery));
        }
    }

    printf("boot.command: %s\n", boot.command);
    printf("boot.recovery: %s\n", boot.recovery);

    printf("write_bootloader_message \n");
    if (!write_bootloader_message(boot, &err)) {
        printf("%s\n", err.c_str());
        printf("write_bootloader_message failed!\n");
    }


    if (!strstr(boot.recovery, "--update_package=")) {
        std::string content;
        printf("bootloader_message is null \n");
        if (android::base::ReadFileToString(COMMAND_FILE, &content)) {
            printf("recovery command: %s\n", content.c_str());
            if (wipe_flag == 1) {
                printf("need wipe data \n");
                std::string content2 = content + "--wipe_data\n";
                printf("recovery command 2: %s\n", content2.c_str());

                if (!android::base::WriteStringToFile(content2, COMMAND_FILE)) {
                    printf("failed to write %s\n", COMMAND_FILE);
                }
            }
        }
        printf("read %s error\n", COMMAND_FILE);
    }

    property_set(ANDROID_RB_PROPERTY, "reboot,recovery");
    sleep(5);

    return ErrorAbort(state, kRebootFailure, "reboot to recovery failed!\n");
}


Value* Reboot(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {

    property_set(ANDROID_RB_PROPERTY, "reboot");
    sleep(5);

    return ErrorAbort(state, kRebootFailure, "reboot failed!\n");
}


Value* SetUpdateStage(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    if (argv.size() != 1) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects 1 args, got %zu", name, argv.size());
    }

    std::vector<std::string> args;
    if (!ReadArgs(state, argv, &args)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed to parse the argument(s)", name);
    }

    const std::string& stage_step = args[0];

    FILE *pf = fopen("/cache/recovery/stage", "w+");
    if (pf == NULL) {
        return ErrorAbort(state, kFileOpenFailure, "fopen stage failed!\n");
    }

    int len = fwrite(stage_step.c_str(), 1, strlen(stage_step.c_str()), pf);
    printf("stage write len:%d, %s\n", len, stage_step.c_str());
    fflush(pf);
    fclose(pf);

    return StringValue("done");
}

Value* GetUpdateStage(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    char buff[128] = {0};

    FILE *pf = fopen("/cache/recovery/stage", "r");
    if (pf == NULL) {
        return StringValue("0");
    }

    int len = fread(buff, 1, 128, pf);
    printf("stage fread len:%d, %s\n", len, buff);
    fclose(pf);

    return StringValue(buff);
}

Value* BackupEnvPartition(const char* name, State* state,
                           const std::vector<std::unique_ptr<Expr>>&argv) {
    int offset = 0;
    char tmpbuf[32] = {0};
    ZipArchiveHandle za = static_cast<UpdaterInfo*>(state->cookie)->package_zip;

    offset = GetEnvPartitionOffset(za);
    if (offset <= 0) {
        return ErrorAbort(state, kArgsParsingFailure, "get env partition offset failed!\n");
    }

    offset = offset/(1024*1024);

    sprintf(tmpbuf, "%s%d", "seek=", offset);
    char *args2[7] = {"/sbin/busybox", "dd", "if=/dev/block/env", "of=/dev/block/mmcblk0", "bs=1M"};
    args2[5] = &tmpbuf[0];
    args2[6] = nullptr;
    pid_t child = fork();
    if (child == 0) {
        execv("/sbin/busybox", args2);
        printf("execv failed\n");
        _exit(EXIT_FAILURE);
    }

    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            ErrorAbort(state,kArgsParsingFailure, "child exited with status:%d\n", WEXITSTATUS(status));
        }
    } else if (WIFSIGNALED(status)) {
        ErrorAbort(state,kArgsParsingFailure, "child terminated by signal :%d\n", WTERMSIG(status));
    }

    return StringValue(strdup("0"));
}

#define RESIZE2FS_INFO  "/cache/recovery/datainfo"
#define  DATA_DEVICE      "/dev/block/data"
#define  COPY_SIZE           (10*1024*1024)
#define  RESERVED_SIZE           (50*1024*1024)

int UserData_Backup(int fd, unsigned long long soffset, unsigned long long doffset, unsigned long long size, unsigned char *buf) {
    signed long long ret = 0;
    doffset= doffset * (-1);
    printf("soffset:%lld, doffset:%lld, size:%lld\n", soffset, doffset, size);
    ret = lseek(fd, soffset, SEEK_SET);
    if (ret < 0) {
        printf("lseek soffset failed! ret : %lld\n", ret);
        return -1;
    }

    ret = read(fd, buf, size);
    if (ret != size) {
        printf("read size %lldfailed!\n", size);
        return -1;
    }

    ret = lseek(fd, doffset, SEEK_END);
    if (ret < 0) {
        printf("lseek doffset failed! ret = %lld\n", ret);
        return -1;
    }

    ret = write(fd, buf, size);
     if (ret != size) {
        printf("write size %lld failed!\n", size);
        return -1;
    }

     return 0;
}

int UserData_Move(int fd, int fd1, unsigned long long emmc_offset, unsigned long long offset, unsigned long long size, unsigned char *buf) {
    signed long long ret = 0;
    printf("e_offset:%llu offset:%llu, size:%lld\n", emmc_offset+offset, offset, size);
    ret = lseek(fd, emmc_offset+offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek offset failed! ret : %lld\n", ret);
        return -1;
    }

    ret = read(fd, buf, size);
    if (ret != size) {
        printf("read size %lldfailed!\n", size);
        return -1;
    }

    ret = lseek(fd1, offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek doffset failed! ret = %lld\n", ret);
        return -1;
    }

    ret = write(fd1, buf, size);
     if (ret != size) {
        printf("write size %lld failed!\n", size);
        return -1;
    }

     return 0;
}

int UserData_Recovery(int fd, unsigned long long soffset, unsigned long long doffset, unsigned long long size, unsigned char *buf) {
    long long ret = 0;
    soffset = soffset * (-1);
    printf("soffset:%lld, doffset:%lld, size:%lld\n", soffset, doffset, size);
    ret = lseek(fd, soffset, SEEK_END);
    if (ret < 0) {
        printf("lseek soffset failed!ret = %lld\n", ret);
        return -1;
    }

    ret = read(fd, buf, size);
    if (ret != size) {
        printf("read size %lld failed!\n", size);
        return -1;
    }

    ret = lseek(fd, doffset, SEEK_SET);
    if (ret < 0) {
        printf("lseek doffset failed! ret = %lld\n", ret);
        return -1;
    }

    ret = write(fd, buf, size);
     if (ret != size) {
        printf("write size %lld failed!\n", size);
        return -1;
    }

     return 0;
}

int Recovery_Resize2fs_Data(void) {
    int ret = 0;
    char buf[256] = {0};
    int matches = 0;
    unsigned resize_4k = 0;
    unsigned long long data_resize = 0;
    unsigned long long data_old_offset = 0;

    //open datainfo
    FILE *pf = fopen(RESIZE2FS_INFO, "r");
    if (pf == NULL) {
        printf("fopen %s failed!\n", RESIZE2FS_INFO);
        return 1;
    }

    //fread data from datainfo
    int len = fread(buf, 1, 255, pf);

    //close datainfo
    fclose(pf);
    if (len <= 0) {
        printf("fread %s failed!\n", RESIZE2FS_INFO);
        return -1;
    }

    //parse datainfo
    matches = sscanf(buf, "%u %llu", &resize_4k, &data_old_offset);
    if (matches != 2) {
        printf("fread %s error data!\n", RESIZE2FS_INFO);
        return -1;
    }

    //blocks to real size
    data_resize = (unsigned long long)resize_4k*4*1024;
    printf("backup resize2fs %s (%llu) \n",DATA_DEVICE, data_resize);

    //open /dev/block/data for get fd
    int fd = open(DEV_EMMC, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", DEV_EMMC);
        return -1;
    }

    //open /dev/block/data for get fd
    int fd1 = open(DATA_DEVICE, O_RDWR);
    if (fd1 < 0) {
        printf("open %s failed!\n", DATA_DEVICE);
        close(fd);
        return -1;
    }

    //malloc 10^M buffer
    unsigned char *tmp = (unsigned char *)malloc(COPY_SIZE);
    if (tmp == NULL) {
        printf("malloc 10M failed!\n");
        close(fd);
        close(fd1);
        return -1;
    }

    unsigned long long tmp_size = 0;
    unsigned long long left = data_resize;
    printf("start to backup the resize2fs data(%llu) emmc_offset(%llu) partition to head!\n", data_resize, data_old_offset);
#if 1
    //while read and write
    while (left > 0) {
        if (left > COPY_SIZE) {
            tmp_size = COPY_SIZE;
        } else {
            tmp_size = left;
        }

        printf("-----read_size(%llu), left(%llu)------\n", tmp_size, left);
        ret = UserData_Move(fd, fd1, data_old_offset, left-tmp_size, tmp_size, tmp);
        if (ret < 0) {
            printf("userdata move failed!\n");
            break;
        }
        left -= tmp_size;
    }
#else
    //while read and write
    while (left > 0) {
        if (left > COPY_SIZE) {
            tmp_size = COPY_SIZE;
        } else {
            tmp_size = left;
        }

        printf("-----read_size(%llu), left(%llu)------\n", tmp_size, left);
        ret = UserData_Recovery(fd, left, data_resize-left, tmp_size, tmp);
        if (ret < 0) {
            printf("userdata move failed!\n");
            break;
        }
        left -= tmp_size;
    }
#endif
    close(fd);

    //fdopen for ffush and fsync
    FILE *fps = fdopen(fd1, "r+");
    if (fps == NULL) {
        printf("fdopen failed!\n");
        close(fd1);
    } else {
        fflush(fps);
        fsync(fd1);
        fclose(fps);
    }

    //free buffer
    free(tmp);
    tmp = NULL;

    return ret;
}

static unsigned long long get_block_device_size(int fd)
{
    unsigned long long size = 0;
    int ret;

    ret = ioctl(fd, BLKGETSIZE64, &size);

    if (ret)
            return 0;

    return size;
}

static int check_partition_data_resize(unsigned long long data_size,
    unsigned long long check_size) {


    char *args2[4] = {"/sbin/resize2fs", "-P", "/dev/block/data"};
    args2[3] = nullptr;
    pid_t child = fork();
    if (child == 0) {
        execv("/sbin/resize2fs", args2);
        printf("execv failed\n");
        _exit(EXIT_FAILURE);
    }

    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            printf("child exited with status:%d\n", WEXITSTATUS(status));
            return -1;
        }
    } else if (WIFSIGNALED(status)) {
        printf("child terminated by signal :%d\n", WTERMSIG(status));
        return -1;
    }

    char buf[256] = {0};
    int matches = 0;
    unsigned long long data_resize = 0;

    //open datainfo
    FILE *pf = fopen(RESIZE2FS_INFO, "r");
    if (pf == NULL) {
        printf("fopen %s failed!\n", RESIZE2FS_INFO);
        return -1;
    }

    //fread data from datainfo
    int len = fread(buf, 1, 256, pf);

    //close datainfo
    fclose(pf);
    if (len <= 0) {
        printf("fread %s failed!\n", RESIZE2FS_INFO);
        return -1;
    }

    //parse datainfo
    matches = sscanf(buf, "%llu", &data_resize);
    if (matches != 1) {
        printf("fread %s error data!\n", RESIZE2FS_INFO);
        return -1;
    }

    //blocks to real size
    data_resize = data_resize*4*1024;
    printf("backup resize2fs %s (%llu -> %llu) \n",DATA_DEVICE, data_size, data_resize);

    //calc the check size by bytes
    check_size = check_size*1024*1024;
    if (data_size - data_resize < check_size) {
        printf("data partition has no enough memory(%llu < %llu) for resize2fs!\n", data_size - data_resize, check_size);
        unlink(RESIZE2FS_INFO);
        return -1;
    }

    printf("data partition has enough memory for resize2fs!\n");
    return 0;
}

int Backup_Data_check( unsigned long check_size) {
    int fd = 0;
    unsigned long long data_size = 0;

    fd = open(DEV_DATA, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", DEV_DATA);
        return -1;
    }

    data_size = get_block_device_size(fd);
    if (data_size == 0) {
        printf("get %s size failed!\n", DEV_DATA);
        close(fd);
        return -1;
    }
    close(fd);

#if 0
    char buf[256] = {0};
    int matches = 0;
    unsigned long long data_resize = 0;
    unsigned long long emmc_size = 0;
    unsigned long long data_offset = 0;
    unsigned long long data_diff = 0;

    fd = open(DEV_EMMC, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", DEV_EMMC);
        return -1;
    }
    emmc_size = get_block_device_size(fd);
    if (emmc_size == 0) {
        printf("get %s size failed!\n", DEV_EMMC);
        close(fd);
        return -1;
    }
    close(fd);

    data_offset = emmc_size - data_size;
    printf("data partition offset: %llu\n", data_offset);
#endif

    return check_partition_data_resize(data_size, check_size);
}

int Backup_Resize2fs_Data( void ) {
#if 1
    int fd = 0;
    char buf[256] = {0};
    unsigned long long emmc_size = 0;
    unsigned long long data_offset = 0;
    unsigned long long data_size = 0;

    fd = open(DEV_DATA, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", DEV_DATA);
        return -1;
    }

    data_size = get_block_device_size(fd);
    if (data_size == 0) {
        printf("get %s size failed!\n", DEV_DATA);
        close(fd);
        return -1;
    }
    close(fd);

    fd = open(DEV_EMMC, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", DEV_EMMC);
        return -1;
    }
    emmc_size = get_block_device_size(fd);
    if (emmc_size == 0) {
        printf("get %s size failed!\n", DEV_EMMC);
        close(fd);
        return -1;
    }
    close(fd);

    data_offset = emmc_size - data_size;
    printf("data partition offset: %llu\n", data_offset);

     //open datainfo
    FILE *pf = fopen(RESIZE2FS_INFO, "a+");
    if (pf == NULL) {
        printf("fopen %s failed!\n", RESIZE2FS_INFO);
        return -1;
    }

    //fread data from datainfo
    sprintf(buf, " %llu", data_offset);
    int len = fwrite(buf, 1, 256, pf );
    fflush(pf);
    fclose(pf);
    return 0;

#else
    int ret = 0;
    char buf[256] = {0};
    int matches = 0;
    unsigned long long data_resize = 0;

    //open datainfo
    FILE *pf = fopen(RESIZE2FS_INFO, "r");
    if (pf == NULL) {
        printf("fopen %s failed!\n", RESIZE2FS_INFO);
        return -1;
    }

    //fread data from datainfo
    int len = fread(buf, 1, 256, pf);

    //close datainfo
    fclose(pf);
    if (len <= 0) {
        printf("fread %s failed!\n", RESIZE2FS_INFO);
        return -1;
    }

    //parse datainfo
    matches = sscanf(buf, "%llu", &data_resize);
    if (matches != 1) {
        printf("fread %s error data!\n", RESIZE2FS_INFO);
        return -1;
    }

    //blocks to real size
    data_resize = data_resize*4*1024;
    printf("backup resize2fs %s (%llu) \n",DATA_DEVICE, data_resize);

    //open /dev/block/data for get fd
    int fd = open(DATA_DEVICE, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", DATA_DEVICE);
        return -1;
    }

    //malloc 10^M buffer
    unsigned char *tmp = (unsigned char *)malloc(COPY_SIZE);
    if (tmp == NULL) {
        printf("malloc 10M failed!\n");
        close(fd);
        return -1;
    }

    unsigned long long tmp_size = 0;
    unsigned long long left = data_resize;
    printf("start to backup the resize2fs data(%llu) partition to end!\n", data_resize);

    //while read and write
    while (left > 0) {
        if (left > COPY_SIZE) {
            tmp_size = COPY_SIZE;
        } else {
            tmp_size = left;
        }

        left -= tmp_size;
        printf("-----read_size(%llu), left(%llu)------\n", tmp_size, left);
        ret = UserData_Backup(fd, left, data_resize-left, tmp_size, tmp);
        if (ret < 0) {
            printf("userdata move failed!\n");
            break;
        }
    }

    //fdopen for ffush and fsync
    FILE *fps = fdopen(fd, "r+");
    if (fps == NULL) {
        printf("fdopen failed!\n");
        close(fd);
    } else {
        fflush(fps);
        fsync(fd);
        fclose(fps);
    }

    //free buffer
    free(tmp);
    tmp = NULL;

    return ret;
 #endif
}


Value* BackupDataPartitionCheck(const char* name, State* state,
                           const std::vector<std::unique_ptr<Expr>>&argv) {
   int ret = 0;
   if (argv.size() != 1) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects 1 args, got %zu", name, argv.size());
    }

    std::vector<std::string> args;
    if (!ReadArgs(state, argv, &args)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed to parse the argument(s)", name);
    }

    const std::string& expect_size = args[0];
    unsigned long resize_diff_size = strtoul(expect_size.c_str(), NULL, 10);

    ret = Backup_Data_check(resize_diff_size);

    if (ret < 0) {
        return ErrorAbort(state, kArgsParsingFailure, "data can not do resize2fs!\n");
    }
    return StringValue(strdup("0"));
}

Value* BackupDataPartition(const char* name, State* state,
                           const std::vector<std::unique_ptr<Expr>>&argv) {

    char *args2[4] = {"/sbin/resize2fs", "-fMp", "/dev/block/data"};
    args2[3] = nullptr;
    pid_t child = fork();
    if (child == 0) {
        execv("/sbin/resize2fs", args2);
        printf("execv failed\n");
        _exit(EXIT_FAILURE);
    }

    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            ErrorAbort(state,kArgsParsingFailure, "child exited with status:%d\n", WEXITSTATUS(status));
        }
    } else if (WIFSIGNALED(status)) {
        ErrorAbort(state,kArgsParsingFailure, "child terminated by signal :%d\n", WTERMSIG(status));
    }

    //no need to format data partition
    wipe_flag = 0;
    int ret = Backup_Resize2fs_Data();

    if (ret < 0) {
        ErrorAbort(state,kArgsParsingFailure, "backup resize2fs data failed!\n");
    }
    return StringValue(strdup("0"));
}

Value* RecoveryDataPartition(const char* name, State* state,
                           const std::vector<std::unique_ptr<Expr>>&argv) {

    int ret = Recovery_Resize2fs_Data();
    if (ret == 1) {
        printf("no file datainfo for recovery data partition!\n");
        return StringValue(strdup("0"));
    } else if (ret < 0) {
        ErrorAbort(state,kArgsParsingFailure, "recovery data partition failed!\n");
    }

    char *args2[4] = {"/sbin/resize2fs", "-f", "/dev/block/data"};
    args2[3] = nullptr;
    pid_t child = fork();
    if (child == 0) {
        execv("/sbin/resize2fs", args2);
        printf("execv failed\n");
        _exit(EXIT_FAILURE);
    }

    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            ErrorAbort(state,kArgsParsingFailure, "child exited with status:%d\n", WEXITSTATUS(status));
        }
    } else if (WIFSIGNALED(status)) {
        ErrorAbort(state,kArgsParsingFailure, "child terminated by signal :%d\n", WTERMSIG(status));
    }

    unlink(RESIZE2FS_INFO);

    return StringValue(strdup("0"));
}

static int get_partition_free_size(const char *path, unsigned long long package_size) {

    struct statfs buf;
    int ret = statfs(path, &buf);
    if (ret < 0) {
        printf("statfs failed!\n");
        return 0;
    }

    printf("buf.f_bsize = %d \n", buf.f_bsize);
    printf("buf.f_bavail = %llu \n", buf.f_bavail);
    printf("cache free size:%llu , update_size:%zu \n", buf.f_bsize*buf.f_bavail, package_size);
    if (buf.f_bsize*buf.f_bavail < package_size + RESERVED_SIZE) {
        printf("cache partition is not enough to backup, backup to /dev/block/mmcblk0!\n");
        return 0;
    }
    printf("cache partition is enough to backup update zip!\n");
    return 1;
}

Value* BackupUpdatePackage(const char* name, State* state,
                           const std::vector<std::unique_ptr<Expr>>&argv) {
    int ret = 0;
    int backup_cache_flag = 0;
    if (argv.size() != 2) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects 2 args, got %zu", name, argv.size());
    }

    std::vector<std::string> args;
    if (!ReadArgs(state, argv, &args)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed to parse the argument(s)", name);
    }

    const std::string& partition = args[0];
    const std::string& offset = args[1];

    std::string err;
    char buf[256] = {0};
    char tmp[256] = {0};
    char path[256] = {0};
    struct bootloader_message boot {};

    //read from bcb data of misc
    //printf("start to backup update zip to cache partition\n");
    read_bootloader_message(&boot,  &err);
    printf("boot.command: %s\n", boot.command);
    printf("boot.recovery: %s\n", boot.recovery);

    if (strstr(boot.recovery, "--update_package=")) {
        printf("check bootloader_message \n");

        //check for strcpy not out of memory
        if (strlen(strstr(boot.recovery, "--update_package=")) > 255) {
            return ErrorAbort(state, kArgsParsingFailure, "boot.recovery is too long!\n");
        }

        strcpy(tmp, strstr(boot.recovery, "--update_package="));
        char *p = strtok(tmp, "\n");

        //check for strcpy not out of memory
        if (strlen(tmp+strlen("--update_package=")) > 255) {
            return ErrorAbort(state, kArgsParsingFailure, "boot.recovery is too long!\n");
        }
        strcpy(path, tmp+strlen("--update_package="));
    }

    //if path is NULL, than read from /cache/recovery/command
    if (!strstr(boot.recovery, "--update_package=")) {
        std::string content;
        printf("bootloader_message is null \n");
        if (android::base::ReadFileToString(COMMAND_FILE, &content)) {
            printf("recovery command: %s\n", content.c_str());
            if (strstr(content.c_str(), "--update_package=")) {
                strcpy(tmp, strstr(content.c_str(), "--update_package="));
                char *p = strtok(tmp, "\n");
                strcpy(path, tmp+strlen("--update_package="));
            } else {
                ret = -1;
            }
        }
    }

    //check cache partition whether has enough space store update package.
    backup_cache_flag = get_partition_free_size("/cache", static_cast<UpdaterInfo*>(state->cookie)->package_zip_len);
    printf("update package path: %s\n", path);

    //if update_package=@/cache/recovery/block.map
    //need to find the real data of package and backup to /dev/block/mmcblk0
    //and save the size and name of package to /cache/recovery/zipinfo
    if (path[0] == '@') {

        //backup update zip to /dev/block/mmcblk0
        if (backup_cache_flag == 0) {
            std::string content;
            if (android::base::ReadFileToString(UNCRYPT_FILE, &content)) {
                printf("recovery uncrypt: %s\n", content.c_str());

                //check for strcpy not out of memory
                if (strlen(content.c_str()) > 255) {
                    return ErrorAbort(state, kArgsParsingFailure, "recovery uncrypt data size > 255!\n");
                }
                strcpy(path, content.c_str());
            }

            printf("%s %s %d", path, offset.c_str(), static_cast<UpdaterInfo*>(state->cookie)->package_zip_len);
            sprintf(buf, "%s %s %d", path, offset.c_str(), static_cast<UpdaterInfo*>(state->cookie)->package_zip_len);

            FILE *pf = fopen("/cache/recovery/zipinfo", "w+");
            if (pf == NULL) {
                return ErrorAbort(state, kArgsParsingFailure, "fopen zipinfo failed!\n");
            }

            int len = fwrite(buf, 1, strlen(buf), pf);
            printf("zipinfo write len:%d, %s\n", len, buf);
            fflush(pf);
            fclose(pf);

            int fd = open(partition.c_str(), O_RDWR);
            if (fd < 0) {
                printf("open %s failed!\n", partition.c_str());
                return ErrorAbort(state, kFileOpenFailure, "open mmcblk failed!\n");
            }

            int offset_w = strtoul(offset.c_str(), NULL, 10);

            int result = lseek(fd, offset_w*1024*1024, SEEK_SET);
            if (result == -1) {
                printf("lseek %s failed!\n", partition.c_str());
                close(fd);
                return ErrorAbort(state, kLseekFailure, "lseek mmcblk failed!\n");
            }

            size_t len_w = write(fd, static_cast<UpdaterInfo*>(state->cookie)->package_zip_addr,static_cast<UpdaterInfo*>(state->cookie)->package_zip_len);
            if (len_w != static_cast<UpdaterInfo*>(state->cookie)->package_zip_len) {
                printf("write %s failed!\n", partition.c_str());
                close(fd);
                return ErrorAbort(state, kFwriteFailure, "write mmcblk failed!\n");
            }

            FILE *fp = NULL;
            fp = fdopen(fd, "r+");
            if (fp == NULL) {
                printf("fdopen failed!\n");
                close(fd);
                return ErrorAbort(state, kFileOpenFailure, "close mmcblk failed!\n");
            }

            fflush(fp);
            fsync(fd);
            fclose(fp);
        } else {
            //backup update zip to /cache
            int fd = open(UPDATE_TMP_FILE, O_RDWR| O_CREAT, 00777);
            if (fd < 0) {
                printf("open %s failed!\n", UPDATE_TMP_FILE);
                return ErrorAbort(state, kFileOpenFailure, "open %s failed!\n", UPDATE_TMP_FILE);
            }

            size_t len_w = write(fd, static_cast<UpdaterInfo*>(state->cookie)->package_zip_addr,static_cast<UpdaterInfo*>(state->cookie)->package_zip_len);
            if (len_w != static_cast<UpdaterInfo*>(state->cookie)->package_zip_len) {
                printf("write %s failed!\n", UPDATE_TMP_FILE);
                close(fd);
                return ErrorAbort(state, kFwriteFailure, "write %s failed!\n", UPDATE_TMP_FILE);
            }

            close(fd);
            ret = do_cache_sync();
            if (ret < 0) {
                printf("do sync for cache partition failed!\n");
            }

            //write the misc for upgrade from cache next update
            memset(boot.recovery, 0, 768);
            strncpy(boot.recovery, "recovery\n--update_package=/cache/update_tmp.zip\n", sizeof("recovery\n--update_package=/cache/update_tmp.zip"));
            write_bootloader_message(boot,  &err);
        }
    }

    if (ret == 0) {
        return StringValue(strdup("0"));
    } else {
        return ErrorAbort(state, kArgsParsingFailure, "backup update to /dev/block/mmcblk* failed!\n");
    }
}

Value* DeleteFileByName(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    if (argv.size() != 1) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() expects 1 args, got %zu", name, argv.size());
    }

    std::vector<std::string> args;
    if (!ReadArgs(state, argv, &args)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() Failed to parse the argument(s)", name);
    }

    const std::string& file = args[0];

    struct stat st;
    if (stat(file.c_str(), &st) == 0) {
        unlink(file.c_str());
    } else {
        printf("%s not exist, can not delete it!\n", file.c_str());
    }

    return StringValue("done");
}

int init_unifykey(const char *path, const char *keyname)
{
    int fp = -1;
    int ret = -1;
    struct key_item_info_t key_item_info;

    if (strlen(keyname) > KEY_UNIFY_NAME_LEN) {
        printf("keyname:%s size is too long\n", keyname);
        return -1;
    }

    fp  = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }
    strcpy(key_item_info.name, keyname);

    ret =ioctl(fp, KEYUNIFY_ATTACH, &key_item_info);
    close(fp);
    return ret;
}

int get_info_unifykey(const char *path, struct key_item_info_t *info)
{
    int fp = -1;
    int ret = -1;

    fp  = open(path, O_RDWR);
    if (fp < 0) {
        printf("no %s found\n", path);
        return -1;
    }

    ret =ioctl(fp, KEYUNIFY_GET_INFO, &info);
    close(fp);
    return ret;
}



int hdcp_write_key(const char * node, char *buff, const char *name)
{
    int fd;
    int ret = 0;
    unsigned long ppos;
    size_t writesize;
    struct key_item_info_t key_item_info;

    if ((NULL == node) || (NULL == buff) || (NULL == name))
    {
        printf("invalid param!\n");
        return -1;
    }

    if (strlen(name) > KEY_UNIFY_NAME_LEN) {
        printf("name:%s size is too long\n", name);
        return -1;
    }

    fd  = open(node, O_RDWR);
    if (fd < 0) {
        printf("no %s found\n", node);
        return -1;
    }

    /* seek the key index need operate. */
    strcpy(key_item_info.name, name);
    ret = ioctl(fd, KEYUNIFY_GET_INFO, &key_item_info);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    ppos = key_item_info.id;
    ret = lseek(fd, ppos, SEEK_SET);
    if (ret == -1) {
        printf("failed to lseek %d\n", ppos);
        close(fd);
        return -1;
    }

    writesize = write(fd, buff, KEY_HDCP_SIZE);
    if (writesize != KEY_HDCP_SIZE) {
        printf("write %s failed!\n", key_item_info.name);
        close(fd);
        return -1;
    }
    printf("write %s(%zu) down!\n", key_item_info.name, writesize);
    close(fd);
    return 0;
}


Value* WriteHdcp22RxFwFn(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {

    int ret = -1;

    //check if /param/firmware.le exist
    if (access(PARAM_FIRMWARE, F_OK)) {
        printf("can't find %s, skip!\n", PARAM_FIRMWARE);
        return StringValue("OK");
    }

    //init unifykey hdcp22_rx_private
    ret = init_unifykey(PATH_KEY_CDEV, UNIFYKEY_HDCP_PRIVATE);
    if (ret < 0) {
        printf("can't init unifykey:%s\n", UNIFYKEY_HDCP_PRIVATE);
        return StringValue("OK");
    }

    //init unifykey hdcp22_rx_fw
    ret = init_unifykey(PATH_KEY_CDEV, UNIFYKEY_HDCP_FW);
    if (ret < 0) {
        printf("can't init unifykey:%s\n", UNIFYKEY_HDCP_FW);
        return StringValue("OK");
    }

    //get info of hdcp22_rx_fw
    struct key_item_info_t info;
    strcpy(info.name, UNIFYKEY_HDCP_FW);
    ret = get_info_unifykey(PATH_KEY_CDEV, &info);
    if (ret < 0) {
        printf("get info unifykey:%s failed\n", UNIFYKEY_HDCP_FW);
        return StringValue("OK");
    }

    //check size of hdcp22_rx_fw
    printf("%s size is %u\n", UNIFYKEY_HDCP_FW, info.size);
    if (info.size == 2080) {
        printf("%s size is right, no need write!\n", UNIFYKEY_HDCP_FW);
        return StringValue("OK");
    }

    //open /param/firmware.le
    int fd = open(PARAM_FIRMWARE, O_RDONLY);
    if (fd < 0) {
        return ErrorAbort(state, kFileOpenFailure, "open file %s failed!\n", PARAM_FIRMWARE);
    }

    //seek 10k
    ret = lseek(fd, KEY_HDCP_OFFSET, SEEK_SET);
    if (ret < 0) {
        close(fd);
        return ErrorAbort(state, kLseekFailure, "lseek file %s(%d) failed!\n", PARAM_FIRMWARE, KEY_HDCP_OFFSET);
    }

    //malloc buffer
    char *tmpbuf = (char *)malloc(KEY_HDCP_SIZE);
    if (tmpbuf == NULL) {
        close(fd);
        return ErrorAbort(state, kArgsParsingFailure, "malloc tmpbuf (%d) failed!\n", KEY_HDCP_SIZE);
    }

    //memeset and read
    memset(tmpbuf, 0, KEY_HDCP_SIZE);
    int len = read(fd, tmpbuf, KEY_HDCP_SIZE);
    if (len != KEY_HDCP_SIZE) {
        close(fd);
        free(tmpbuf);
        return ErrorAbort(state, kFreadFailure, "read %s (%d) failed!\n", PARAM_FIRMWARE,KEY_HDCP_SIZE);
    }
    close(fd);

    //write hdcp_rx_fw key
    ret = hdcp_write_key(PATH_KEY_CDEV, tmpbuf, UNIFYKEY_HDCP_FW);
    if (ret < 0) {
        free(tmpbuf);
        return ErrorAbort(state, kFwriteFailure, "write %s  failed!\n", UNIFYKEY_HDCP_FW);
    }
    free(tmpbuf);
    tmpbuf = NULL;

    return StringValue("OK");
}

Value* RecoveryBackupExist(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {

    ZipArchiveHandle za = static_cast<UpdaterInfo*>(state->cookie)->package_zip;
    ZipString zip_string_path(RECOVERY_IMG);
    ZipEntry entry;

    //get recovery.img entry
    if (FindEntry(za, zip_string_path, &entry) != 0) {
        printf("no %s in package, no need backup.\n", zip_string_path.name);
        return StringValue(strdup("1"));
    }

    //recovery.img length
    printf("entry.uncompressed_length:%d\n", entry.uncompressed_length);

    //check /cache/recovery/recovery.img exist
    struct stat st;
    if (stat(RECOVERY_BACKUP, &st) == 0) {
        if (st.st_size >= entry.uncompressed_length) {
            //has recovery.img backup, no need backup again
            printf("%s is ok,no need backup.\n", RECOVERY_BACKUP);
            return StringValue(strdup("1"));
        } else {
            //recovery.img not full, need backup
            printf("%s not full, need backup.\n", RECOVERY_BACKUP);
            return StringValue(strdup("0"));
        }
    } else {
        printf("%s not exist, need backup.\n", RECOVERY_BACKUP);
        return StringValue(strdup("0"));
    }
}

Value* WipeDdrParameter(const char* name, State* state, const std::vector<std::unique_ptr<Expr>>& argv) {
    int fd = -1;
    int len = 0;
    int ddr_size = 2*1024*1024;
    const char *ddr_device = "/dev/ddr_parameter";

    fd = open(ddr_device, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s failed (%s)\n", ddr_device, strerror(errno));
        return StringValue(strdup("0"));
    }

   char *buf = (char *)malloc(ddr_size);
   if (buf == NULL) {
        fprintf(stderr, "malloc buf failed\n");
        close(fd);
        return StringValue(strdup("0"));
   }

   memset(buf, 0, ddr_size );

    len = write(fd, buf, ddr_size);
    if (len != ddr_size) {
        printf("write ddr_parameter failed\n");
    }

    free(buf);
    fsync(fd);
    close(fd);

    return StringValue(strdup("0"));
}

void Register_libinstall_amlogic() {
    RegisterFunction("write_dtb_image", WriteDtbImageFn);
    RegisterFunction("write_bootloader_image", WriteBootloaderImageFn);
    RegisterFunction("reboot_recovery", RebootRecovery);
    RegisterFunction("reboot", Reboot);
    RegisterFunction("backup_data_cache", BackupDataCache);
    RegisterFunction("set_bootloader_env", SetBootloaderEnvFn);
    RegisterFunction("ota_zip_check", OtaZipCheck);
    RegisterFunction("get_update_stage", GetUpdateStage);
    RegisterFunction("set_update_stage", SetUpdateStage);
    RegisterFunction("write_hdcp_22rxfw", WriteHdcp22RxFwFn);
    RegisterFunction("backup_env_partition", BackupEnvPartition);
    RegisterFunction("backup_update_package", BackupUpdatePackage);
    RegisterFunction("backup_data_partition_check", BackupDataPartitionCheck);
    RegisterFunction("backup_data_partition", BackupDataPartition);
    RegisterFunction("recovery_data_partition", RecoveryDataPartition);

    RegisterFunction("delete_file", DeleteFileByName);
    RegisterFunction("recovery_backup_exist", RecoveryBackupExist);
    RegisterFunction("wipe_ddr_parameter", WipeDdrParameter);
}
