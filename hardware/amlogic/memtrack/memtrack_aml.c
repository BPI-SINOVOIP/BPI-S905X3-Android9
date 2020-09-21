/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *      used for memory track.
 */

#define LOG_TAG "memtrack"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <inttypes.h>
#include <dirent.h>
#include <stdint.h>

#include <hardware/memtrack.h>
#include <log/log.h>
#include <dirent.h>

#define DEBUG 0
#define IONPATH "/d/ion/heaps"
#define GPUT8X "/d/mali0/ctx"
#define MALI0 "/d/mali0"
#define MALI_DEVICE "/sys/devices/platform/ffe40000.bifrost"
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define CHAR_BUFFER_SIZE        1024
#define INIT_BUFS_ARRAY_SIZE    1024
#define PAGE_SIZE               4096

/**
 * Auxilary struct to keep information about single buffer.
 */
typedef struct __buf_info_t {
    uint64_t id;
    size_t size;
} buf_info_t;

/**
 * Auxilary struct to keep information about all buffers.
 */
typedef struct __bufs_array {
    size_t length;
    size_t capacity;
    buf_info_t* data;
} bufs_array_t;

/**
 * Preallocate memory for the array of buffer
 *
 * @param[in]     arr       Pointer to bufs_array_t struct defining the new array.
 * @param[in]     init_cap  Number of buffers for which memory should be preallocated.
 */
static void bufs_array_init(bufs_array_t* arr, size_t init_cap) {
    if (arr == NULL) return;
    arr->data = (buf_info_t *)malloc(init_cap * sizeof(buf_info_t));
    arr->capacity = init_cap;
    arr->length = 0;
}

/**
 * Add new buffer to existing array of buffers.
 *
 * @param[in]     arr       Pointer to bufs_array_t struct to which new buffer should be added.
 * @param[in]     buf_info  Pointer to the buf_info_t struct with info about the buffer to add.
 */
static void bufs_array_add(bufs_array_t* arr, buf_info_t* buf_info) {
    if (arr == NULL || buf_info == NULL) return;
    if (arr->capacity == arr->length) {
        arr->capacity *= 2;
        arr->data = (buf_info_t *)realloc(arr->data, arr->capacity * sizeof(buf_info_t));
    }
    arr->data[arr->length++] = *buf_info;
}

/**
 * Free memory used by array of buffers.
 *
 * @param[in]     arr       Poiner to bufs_array_t structure for which memory should be freed.
 */
static void bufs_array_free(bufs_array_t* arr) {
    if (arr == NULL) return;
    free(arr->data);
    arr->data = NULL;
    arr->capacity = arr->length = 0;
}

/**
 * Get total size of the buffers provided in the bufs param
 * using info from the bufs_info param. Assuming that all buffers are sorted.
 *
 * @param[in]     bufs_info     Buffers info array
 * @param[in]     bufs          Array of buffers to calculate total size of
 * @param[in]     bufs_size     The number of elements of bufs array
 *
 * @return total size of the buffers provided in the bufs param
 */
static int get_bufs_size(bufs_array_t* bufs_info, uint64_t* bufs, size_t bufs_size) {
    if (bufs_size == 0 || bufs_info == NULL || bufs == NULL || bufs_info->length == 0) return 0;
    size_t result = 0;
    size_t bufs_i = 0, bi_i = 0;
    while (bufs_i < bufs_size && bi_i < bufs_info->length) {
        if (bufs[bufs_i] < bufs_info->data[bi_i].id) {
            bufs_i++;
        } else if (bufs[bufs_i] > bufs_info->data[bi_i].id) {
            bi_i++;
        } else {
            result += bufs_info->data[bi_i].size;
            bufs_i++;
            bi_i++;
        }
    }
    return result;
}


static struct hw_module_methods_t memtrack_module_methods = {
    .open = NULL,
};

struct memtrack_record record_templates[] = {
    {
        .flags = MEMTRACK_FLAG_SMAPS_UNACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },

/*
    {
        .flags = MEMTRACK_FLAG_SMAPS_ACCOUNTED |
                 MEMTRACK_FLAG_PRIVATE |
                 MEMTRACK_FLAG_NONSECURE,
    },
*/
};

// just return 0
int aml_memtrack_init(const struct memtrack_module *module __unused)
{
    ALOGD("memtrack init");
    return 0;
}

/*
 * find the userid of process @pid
 * return the userid if success, or return -1 if not
 */
static int memtrack_find_userid(int pid)
{
    FILE *fp;
    char line[1024];
    char tmp[128];
    int userid;

    sprintf(tmp, "/proc/%d/status", pid);
    if ((fp=fopen(tmp, "r")) == NULL) {
        if (DEBUG) ALOGD("open file %s error %s", tmp, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "Uid: %d", &userid) == 1) {
            fclose(fp);
            return userid;
        }
    }

    // should never reach here
    fclose(fp);
    return -1;
}

static unsigned int memtrack_read_smaps(FILE *fp)
{
    char line[1024];
    unsigned int size, sum = 0;
    int skip, done = 0;

    uint64_t start;
    uint64_t end = 0;
    int len;
    char *name;
    int nameLen, name_pos;

    if(fgets(line, sizeof(line), fp) == 0) {
        return 0;
    }

    while (!done) {
        skip = 0;

        len = strlen(line);
        if (len < 1)
            return 0;

        line[--len] = 0;

        if (sscanf(line, "%"SCNx64 "-%"SCNx64 " %*s %*x %*x:%*x %*d%n", &start, &end, &name_pos) != 2) {
            skip = 1;
        } else {
            while (isspace(line[name_pos])) {
                name_pos += 1;
            }
            name = line + name_pos;
            nameLen = strlen(name);

            if (nameLen >= 8 &&
                    (!strncmp(name, "/dev/mali", 6) || !strncmp(name, "/dev/ump", 6))) {
                skip = 0;
            } else {
                skip = 1;
            }

        }

        while (1) {
            if (fgets(line, 1024, fp) == 0) {
                done = 1;
                break;
            }

            if(!skip) {
                if (line[0] == 'S' && sscanf(line, "Size: %d kB", &size) == 1) {
                    sum += size;
                }
            }

            if (sscanf(line, "%" SCNx64 "-%" SCNx64 " %*s %*x %*x:%*x %*d", &start, &end) == 2) {
                // looks like a new mapping
                // example: "10000000-10001000 ---p 10000000 00:00 0"
                break;
            }
        }
    }

    // converted into Bytes
    return (sum * 1024);
}

static size_t read_pid_egl_memory(pid_t pid)
{
    size_t unaccounted_size = 0;
    FILE *ion_fp;
    FILE *egl_fp;
    char tmp[CHAR_BUFFER_SIZE];
    char egl_ion_dir[] = IONPATH;
    struct dirent  *de;
    DIR *p_dir;

    p_dir = opendir( egl_ion_dir );
    if (!p_dir) {
        ALOGD("fail to open %s\n", egl_ion_dir);
        return 0;
    }


    while ((de = readdir(p_dir))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        snprintf(tmp, CHAR_BUFFER_SIZE, "%s/%s", egl_ion_dir, de->d_name);
        if ((ion_fp = fopen(tmp, "r")) == NULL) {
            ALOGD("open file %s error %s", tmp, strerror(errno));
            closedir(p_dir);
            return -errno;
        }
        if ((egl_fp = fopen(tmp, "r")) == NULL) {
            ALOGD("open file %s error %s", tmp, strerror(errno));
            fclose(ion_fp);
            closedir(p_dir);
            return -errno;
        }
        //Parse bufs. Entries appear as follows:
        //buf= ece0a300 heap_id= 4    size= 8486912    kmap= 0    dmap= 0
        int num_entries_found = 0;
        bufs_array_t bufs_array;
        bufs_array_init(&bufs_array, INIT_BUFS_ARRAY_SIZE);
        while (true) {
            char line[CHAR_BUFFER_SIZE];
            buf_info_t buf_info;
            int num_matched;
            if (fgets(line, sizeof(line), ion_fp) == NULL) {
                break;
            }
            num_matched = sscanf(line, "%*s %llx %*s %*u %*s %u %*s %*d %*s %*d",
                &buf_info.id, &buf_info.size);
            if (num_matched == 2) {
                // We've found an entry ...
                num_entries_found++;
                bufs_array_add(&bufs_array, &buf_info);
            } else {
                // Early termination: if we fail to parse a line after
                // hitting the correct section then we know we've finished.
                if (num_entries_found) {
                    break;
                }
            }
        }

        // Parse clients. Entries appear as follows:
        //client(ecfebd00)  composer@2.2-se      pid(3226)

        uint64_t* surface_flinger_bufs = NULL;
        size_t surface_flinger_bufs_size = 0;
        uint64_t* current_pids_bufs = NULL;
        size_t current_pids_bufs_size = 0;
        uint64_t* bufs;
        size_t* bufs_size;
        int need_to_rescan = 0;
        pid_t sf_pid = 0;
        while (true) {
            char     line[CHAR_BUFFER_SIZE];
            char     alloc_client[CHAR_BUFFER_SIZE];
            pid_t    alloc_pid;
            int      num_matched;
            if (need_to_rescan == 1) {
                need_to_rescan = 0;
            } else {
                if (fgets(line, sizeof(line), egl_fp) == NULL) {
                    break;
                }
            }
            num_matched = sscanf(line, "client(%*x) %1023s pid(%d)",
                                        alloc_client, &alloc_pid);
            if (num_matched == 2) {
                // We've found an entry ...
                if (alloc_pid == pid) {
                    if (current_pids_bufs == NULL) {
                        current_pids_bufs = (uint64_t*)malloc(bufs_array.length * sizeof(uint64_t));
                    }
                    bufs = current_pids_bufs;
                    bufs_size = &current_pids_bufs_size;
                } else if (strncmp(alloc_client, "surfaceflinger", sizeof(alloc_client)) == 0) {
                    sf_pid = alloc_pid;
                    if (surface_flinger_bufs == NULL) {
                        surface_flinger_bufs = (uint64_t*)malloc(bufs_array.length * sizeof(uint64_t));
                    }
                    bufs = surface_flinger_bufs;
                    bufs_size = &surface_flinger_bufs_size;
                } else {
                    bufs = NULL;
                }
                while (bufs) {
                    //Parse client's buffers lines, which look like this:
                    // handle= ecf9da00     buf= ecf1d600 heap_id= 4    size= 8486912

                    uint64_t buf_id;
                    if (fgets(line, sizeof(line), egl_fp) == NULL) {
                        break;
                    }
                    num_matched = sscanf(line, " handle= %*x buf= %llx %*s %*d %*s %*u", &buf_id);
                    if (num_matched == 1) {
                        // We've found an entry ...
                        bufs[(*bufs_size)++] = buf_id;
                    } else {
                        //We are out of entries, but most likely moved to the next client.
                        //Don't need to read another line from file.
                        need_to_rescan = 1;
                        break;
                    }
                }
            }
        }

        if (pid == sf_pid) {
            unaccounted_size += get_bufs_size(&bufs_array, surface_flinger_bufs,
                                              surface_flinger_bufs_size);
        } else if (current_pids_bufs_size > 0) {
            //Remove buffers from current_pids_bufs that are present in surface_flinger_bufs.
            //Assuming that both buf arrays are sorted.
            size_t pid_i_r = 0, pid_i_w = 0, sf_i = 0;
            while (pid_i_r < current_pids_bufs_size) {
                if (sf_i < surface_flinger_bufs_size) {
                    if (current_pids_bufs[pid_i_r] < surface_flinger_bufs[sf_i]) {
                        current_pids_bufs[pid_i_w] = current_pids_bufs[pid_i_r];
                        pid_i_r++;
                        pid_i_w++;
                    } else if (current_pids_bufs[pid_i_r] > surface_flinger_bufs[sf_i]) {
                        sf_i++;
                    } else {
                        pid_i_r++;
                        sf_i++;
                    }
                } else {
                    current_pids_bufs[pid_i_w] = current_pids_bufs[pid_i_r];
                    pid_i_r++;
                    pid_i_w++;
                }
            }
            current_pids_bufs_size = pid_i_w;
            unaccounted_size += get_bufs_size(&bufs_array, current_pids_bufs, current_pids_bufs_size);
        }


        //Free any memory used
        bufs_array_free(&bufs_array);
        if (current_pids_bufs != NULL) {
            free(current_pids_bufs);
            current_pids_bufs = NULL;
            current_pids_bufs_size = 0;
        }
        if (surface_flinger_bufs != NULL) {
            free(surface_flinger_bufs);
            surface_flinger_bufs = NULL;
            surface_flinger_bufs_size = 0;
        }
            fclose(ion_fp);
            fclose(egl_fp);
    }

    closedir(p_dir);

    return unaccounted_size;
}

static size_t read_egl_cached_memory(pid_t pid)
{
    size_t unaccounted_size = 0;
    FILE *ion_fp;
    char tmp[CHAR_BUFFER_SIZE];
    char line[CHAR_BUFFER_SIZE];
    char egl_ion_dir[] = IONPATH;

    snprintf(tmp, CHAR_BUFFER_SIZE, "%s/%s", egl_ion_dir, "vmalloc_ion");
    if ((ion_fp = fopen(tmp, "r")) == NULL) {
        ALOGD("open file %s error %s", tmp, strerror(errno));
        return -errno;
    }

    // Parse clients. Entries appear as follows:
    //client(edf39c80)  allocator@2.0-s      pid(3226)
    while (fgets(line, sizeof(line), ion_fp) != NULL) {
        char     alloc_client[CHAR_BUFFER_SIZE];
        pid_t    alloc_pid;
        int      num_matched;

        if (unaccounted_size > 0)
            break;

        num_matched = sscanf(line, "client(%*x) %1023s pid(%d)",
                                    alloc_client, &alloc_pid);
        if (num_matched == 2) {
            if (pid == alloc_pid &&
                strncmp(alloc_client, "allocator@2.0-s", sizeof(alloc_client)) == 0) {
                while (fgets(line, sizeof(line), ion_fp) != NULL) {
                    //Parse client's buffers lines, which look like this:
                    //     total cached         68481024
                    size_t buf_size;
                    num_matched = sscanf(line, " total cached  %zu", &buf_size);
                    if (num_matched == 1) {
                        // We've found an entry ...
                        unaccounted_size = buf_size;
                        break;
                    }
                }
            }
        }
    }

    //close file fd
    fclose(ion_fp);
    return unaccounted_size;
}

// mali t82x t83x
static int memtrack_get_gpuT8X(char *path)
{
    FILE *file;
    char line[1024];

    int gpu_size = 0;

    if ((file = fopen(path, "r")) == NULL) {
        if (DEBUG) ALOGD("open file %s error %s", path, strerror(errno));
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
            if (sscanf(line, "%d %*d", &gpu_size) != 1)
                continue;
            else
                break;
    }
    fclose(file);
    return gpu_size * PAGE_SIZE;
}

static size_t read_pid_gl_used_memory(pid_t pid)
{
    size_t unaccounted_size = 0;
    FILE *ion_fp;
    char tmp[CHAR_BUFFER_SIZE];
    char line[CHAR_BUFFER_SIZE];
    char gl_mem_dir[] = MALI0;

    snprintf(tmp, CHAR_BUFFER_SIZE, "%s/%s", gl_mem_dir, "gpu_memory");
    if ((ion_fp = fopen(tmp, "r")) == NULL) {
        ALOGD("open file %s error %s", tmp, strerror(errno));
        return -errno;
    }

    // Parse clients. Entries appear as follows:
    //kctx             pid              used_pages
    //----------------------------------------------------
    //f0cd5000       4370       1511
    while (fgets(line, sizeof(line), ion_fp) != NULL) {
        pid_t    alloc_pid;
        int      num_matched;
        int      ctx_used_mem;

        if (unaccounted_size > 0)
            break;
        num_matched = sscanf(line, "%*x %d %d", &alloc_pid, &ctx_used_mem);
        if (num_matched == 2) {
            if (pid == alloc_pid) {
                unaccounted_size = ctx_used_mem;
                fclose(ion_fp);
                return unaccounted_size * PAGE_SIZE;
            }
        }
    }
    //close file fd
    fclose(ion_fp);
    return unaccounted_size * PAGE_SIZE;
}

static size_t read_gl_device_cached_memory(pid_t pid)
{
    size_t unaccounted_size = 0;
    FILE *ion_fp = NULL;
    char tmp[CHAR_BUFFER_SIZE];
    char line[CHAR_BUFFER_SIZE];
    char gl_ion_dir[] = IONPATH;
    char gl_dev_dir[] = MALI_DEVICE;

    snprintf(tmp, CHAR_BUFFER_SIZE, "%s/%s", gl_ion_dir, "vmalloc_ion");
    if ((ion_fp = fopen(tmp, "r")) == NULL) {
        ALOGD("open file %s error %s", tmp, strerror(errno));
        return -errno;
    }

    // Parse clients. Entries appear as follows:
    //client(edf39c80)  allocator@2.0-s      pid(3226)
    while (fgets(line, sizeof(line), ion_fp) != NULL) {
        char     alloc_client[CHAR_BUFFER_SIZE];
        pid_t    alloc_pid;
        int      num_matched;
        FILE *fl_fp = NULL;

        if (unaccounted_size > 0)
            break;

        num_matched = sscanf(line, "client(%*x) %1023s pid(%d)",
                                    alloc_client, &alloc_pid);
        if (num_matched == 2) {
            if (pid == alloc_pid &&
                strncmp(alloc_client, "allocator@2.0-s", sizeof(alloc_client)) == 0) {
                snprintf(tmp, CHAR_BUFFER_SIZE, "%s/%s", gl_dev_dir, "mem_pool_size");
                if ((fl_fp = fopen(tmp, "r")) == NULL) {
                    return -errno;
                }
                // Entries appear as follows:
                // 7610 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
                size_t buf_size;
                while (fgets(line, sizeof(line), fl_fp) != NULL) {
                        if (sscanf(line, "%d %*d", &buf_size) != 1)
                            continue;
                        else {
                            unaccounted_size = buf_size * PAGE_SIZE;
                            fclose(fl_fp);
                            break;
                        }
                    }
                }
            }
        }

    //close file fd
    fclose(ion_fp);
    return unaccounted_size;
}

static unsigned int memtrack_get_gpuMem(int pid)
{
    FILE *fp;
    char *cp, tmp[CHAR_BUFFER_SIZE];
    unsigned int result = 0;

    DIR *gpudir;
    struct dirent *dir;
    int gpid = -1;

    gpudir = opendir(GPUT8X);

    if (!gpudir) {
        if (DEBUG)
            ALOGD("open %s error %s\n", GPUT8X, strerror(errno));
        sprintf(tmp, "/proc/%d/smaps", pid);
        fp = fopen(tmp, "r");
        if (fp == NULL) {
            if (DEBUG) ALOGD("open file %s error %s", tmp, strerror(errno));
            return 0;
        }
        result = memtrack_read_smaps(fp);

        fclose(fp);
        return result;
    } else {
        result = read_pid_gl_used_memory(pid);
        result += read_gl_device_cached_memory(pid);
        while ((dir = readdir(gpudir))) {
            strcpy(tmp, dir->d_name);
            if (DEBUG)
                ALOGD("gpudir name=%s\n", dir->d_name);
            if ((cp=strchr(tmp, '_'))) {
                *cp = '\0';
                gpid = atoi(tmp);
                if (DEBUG)
                    ALOGD("gpid=%d, pid=%d\n", gpid, pid);
                if (gpid == pid) {
                    sprintf(tmp, GPUT8X"/%s/%s", dir->d_name, "mem_pool_size");
                    result += memtrack_get_gpuT8X(tmp);
                    closedir(gpudir);
                    return result;
                }
            }
        }
        closedir(gpudir);
    }
    return result;
}

static int memtrack_get_memory(pid_t pid, enum memtrack_type type,
                             struct memtrack_record *records,
                             size_t *num_records)
{
    //FILE *fp;
    //FILE *ion_fp;
    //char line[1024];
    //char tmp[128];
    //unsigned int mali_inuse = 0;
    //unsigned int size;
    size_t unaccounted_size = 0;

    unsigned int gpu_size;

    size_t allocated_records =  ARRAY_SIZE(record_templates);
    *num_records = ARRAY_SIZE(record_templates);

    if (records == NULL) {
        return 0;
    }

    memcpy(records, record_templates, sizeof(struct memtrack_record) * allocated_records);

    if (type == MEMTRACK_TYPE_GL) {
        // find the user id of the process, only support calculate the non root process
        int ret = memtrack_find_userid(pid);
        if (ret <= 0) {
            return -1;
        }
        gpu_size = memtrack_get_gpuMem(pid);
        unaccounted_size += gpu_size;
    } else if (type == MEMTRACK_TYPE_GRAPHICS) {
        unaccounted_size += read_pid_egl_memory(pid);
    } else if (type == MEMTRACK_TYPE_OTHER) {
        unaccounted_size += read_egl_cached_memory(pid);
    }

    if (allocated_records > 0) {
        records[0].size_in_bytes = unaccounted_size;
        if (DEBUG)
            ALOGD("Graphics type:%d unaccounted_size:%u\n", type, unaccounted_size);
    }

    return 0;
}

int aml_memtrack_get_memory(const struct memtrack_module *module __unused,
                                pid_t pid,
                                int type,
                                struct memtrack_record *records,
                                size_t *num_records)
{
    if (pid <= 0)
        return -EINVAL;

    if (type == MEMTRACK_TYPE_GL || type == MEMTRACK_TYPE_GRAPHICS
        || type == MEMTRACK_TYPE_OTHER)
        return memtrack_get_memory(pid, type, records, num_records);
    else
        return -ENODEV;
}


struct memtrack_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = MEMTRACK_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = MEMTRACK_HARDWARE_MODULE_ID,
        .name = "aml Memory Tracker HAL",
        .author = "amlogic",
        .methods = &memtrack_module_methods,
    },

    .init = aml_memtrack_init,
    .getMemory = aml_memtrack_get_memory,
};
