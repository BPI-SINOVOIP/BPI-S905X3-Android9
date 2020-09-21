/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2017/7/19
 *  @par function description:
 *  - 1 thermal HAL
 */

#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#define LOG_TAG "ThermalHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/thermal.h>

#define TEMPERATURE_TYPE_CPU        "CPU"
#define TEMPERATURE_TYPE_GPU        "GPU"

#define CPU_LABEL_0                 "CPU0"
#define CPU_LABEL_1                 "CPU1"
#define CPU_LABEL_2                 "CPU2"
#define CPU_LABEL_3                 "CPU3"
#define MAX_LENGTH                  100
#define VALUE_LENGTH                64

#define CPU_USAGE_FILE              "/proc/stat"
#define TEMPERATURE_DIR             "/sys/class/thermal"
#define THERMAL_DIR                 "thermal_zone"
#define THERMAL_CPU                 "thermal_zone0"
#define CPU_ONLINE_FILE_FORMAT      "/sys/devices/system/cpu/cpu%d/online"
#define UNKNOWN_LABEL               "UNKNOWN"

namespace android {
namespace amlogic {

/*---------------------------------------------------------------
* FUNCTION NAME: readSysValue
* DESCRIPTION:
*    			read value from sysfs
* ARGUMENTS:
*				const char *path:sysfs patch
* Return:
*    			-1:fail, >=0: success
* Note:
*
*---------------------------------------------------------------*/
static int readSysValue(const char *path) {
    int fd, len;
    char* end;
    char value[VALUE_LENGTH] = {0};

    if ((fd = open(path, O_RDONLY)) < 0) {
        ALOGE("readSysValue, open %s fail.", path);
        return -1;
    }

    len = read(fd, value, VALUE_LENGTH);
    if (len < 0) {
        ALOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);

    if ((0x0A == value[len-1]) || (0x0D == value[len-1])) {
        value[len-1] = '\0';
    }

    //ALOGI("readSysValue, path: %s, value: %s\n", path, value);
    int val = (int) strtol(value, &end, 0);
    if (end == value || *end != '\0' || val == INT_MAX || val == INT_MIN) {
        //ALOGI("readSysValue, end: %p, value: %p\n", end, value);
        return -1;
    }

    return val;
}

static ssize_t getTemperatures(thermal_module_t *module, temperature_t *list, size_t size) {
    char path[MAX_LENGTH];
    float temp;
    float throttlingThreshold;
    float shutdownThreshold;
    size_t idx = 0;
    DIR *dir;
    struct dirent *de;

    /** Read all available temperatures from
     * /sys/class/thermal/thermal_zone[0-9]+/temp files.
     * Don't guarantee that all temperatures are in Celsius. */
    dir = opendir(TEMPERATURE_DIR);
    if (dir == 0) {
        ALOGE("%s: failed to open directory %s: %s", __func__, TEMPERATURE_DIR, strerror(-errno));
        return -errno;
    }

    while ((de = readdir(dir))) {
        if (!strncmp(de->d_name, THERMAL_DIR, strlen(THERMAL_DIR))) {
            int val;
            const char *temp_name;
            temperature_type temp_temperature_type;

            if (!strncmp(de->d_name, THERMAL_CPU, strlen(THERMAL_CPU))) {
                temp_temperature_type = DEVICE_TEMPERATURE_CPU ;
                temp_name = TEMPERATURE_TYPE_CPU;
            }
            else{
                temp_temperature_type = DEVICE_TEMPERATURE_UNKNOWN ;
                temp_name = UNKNOWN_LABEL;
            }

            snprintf(path, MAX_LENGTH, "%s/%s/temp", TEMPERATURE_DIR, de->d_name);
            val = readSysValue(path);
            if (val < 0) {
                continue;
            }

            temp = (float)val/1000;
            if (list != NULL && idx < size) {
                snprintf(path, MAX_LENGTH, "%s/%s/trip_point_1_temp", TEMPERATURE_DIR, de->d_name);
                val = readSysValue(path);
                if (val >= 0) {
                    throttlingThreshold = (float)val/1000;
                }

                snprintf(path, MAX_LENGTH, "%s/%s/trip_point_3_temp", TEMPERATURE_DIR, de->d_name);
                val = readSysValue(path);
                if (val >= 0) {
                    shutdownThreshold = (float)val/1000;
                }

                list[idx] = (temperature_t) {
                    .name = temp_name,
                    .type = temp_temperature_type,
                    .current_value = temp,
                    .throttling_threshold = throttlingThreshold,
                    .shutdown_threshold = shutdownThreshold,
                    .vr_throttling_threshold = UNKNOWN_TEMPERATURE,
                };
            }

            idx++;
        }
    }
    closedir(dir);

    ALOGI("current temperature:%f, throttling_threshold:%f, shutdown_threshold:%f", temp, throttlingThreshold, shutdownThreshold);
    return idx;
}

static ssize_t getCpuUsages(thermal_module_t *module, cpu_usage_t *list) {
    int vals, cpu_num, online;
    ssize_t read;
    uint64_t user, nice, system, idle, active, total;
    char *line = NULL;
    size_t len = 0;
    size_t size = 0;
    char file_name[MAX_LENGTH];
    FILE *cpu_file;
    FILE *file = fopen(CPU_USAGE_FILE, "r");

    if (file == NULL) {
        ALOGE("%s: failed to open: %s", __func__, strerror(errno));
        return -errno;
    }

    /* sample:
    cpu  6604 1513 6140 156891 1251 0 9 0 0 0
    cpu0 1399 423 1667 38709 346 0 5 0 0 0
    cpu1 1521 325 1211 39586 157 0 3 0 0 0
    cpu2 1980 399 1931 38470 381 0 0 0 0 0
    cpu3 1704 366 1331 40126 367 0 1 0 0 0
    */
    while ((read = getline(&line, &len, file)) != -1) {
        // Skip non "cpu[0-9]" lines.
        if (strnlen(line, read) < 4 || strncmp(line, "cpu", 3) != 0 || !isdigit(line[3])) {
            free(line);
            line = NULL;
            len = 0;
            continue;
        }
        vals = sscanf(line, "cpu%d %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64, &cpu_num, &user,
                &nice, &system, &idle);

        free(line);
        line = NULL;
        len = 0;

        if (vals != 5) {
            ALOGE("%s: failed to read CPU information from file: %s", __func__, strerror(errno));
            fclose(file);
            return errno ? -errno : -EIO;
        }

        active = user + nice + system;
        total = active + idle;

        // Read online CPU information.
        snprintf(file_name, MAX_LENGTH, CPU_ONLINE_FILE_FORMAT, cpu_num);
        cpu_file = fopen(file_name, "r");
        online = 0;
        if (cpu_file == NULL) {
            ALOGE("%s: failed to open file: %s (%s)", __func__, file_name, strerror(errno));
            // /sys/devices/system/cpu/cpu0/online is missing on some systems, because cpu0 can't
            // be offline.
            online = cpu_num == 0;
        } else if (1 != fscanf(cpu_file, "%d", &online)) {
            ALOGE("%s: failed to read CPU online information from file: %s (%s)", __func__,
                    file_name, strerror(errno));
            fclose(file);
            fclose(cpu_file);
            return errno ? -errno : -EIO;
        } else {
            fclose(cpu_file);
        }

        if (list != NULL) {
            const char *cpuName = CPU_LABEL_0;
            if (0 == cpu_num)
                cpuName = CPU_LABEL_0;
            else if (1 == cpu_num)
                cpuName = CPU_LABEL_1;
            else if (2 == cpu_num)
                cpuName = CPU_LABEL_2;
            else if (3 == cpu_num)
                cpuName = CPU_LABEL_3;
            list[size] = (cpu_usage_t) {
                .name = cpuName,
                .active = active,
                .total = total,
                .is_online = (1 == online)?true:false
            };
        }

        size++;
    }

    //if (list != NULL) {
    //    for (size_t i = 0; i < size; ++i)
    //        ALOGI("index:%d: cpu name:%s", i, list[i].name);
    //}

    fclose(file);
    return size;
}

static ssize_t getCoolingDevices(thermal_module_t *module, cooling_device_t *list, size_t size) {
    return 0;
}

} // namespace amlogic
} // namespace android


static struct hw_module_methods_t thermal_module_methods = {
    .open = NULL,
};

thermal_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = THERMAL_HARDWARE_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = THERMAL_HARDWARE_MODULE_ID,
        .name = "AML Thermal HAL",
        .author = "aml",
        .methods = &thermal_module_methods,
    },

    .getTemperatures = android::amlogic::getTemperatures,
    .getCpuUsages = android::amlogic::getCpuUsages,
    .getCoolingDevices = android::amlogic::getCoolingDevices,
};
