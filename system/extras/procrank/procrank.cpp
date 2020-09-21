//
// Copyright (C) 2008 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <pagemap/pagemap.h>

struct proc_info {
    pid_t pid;
    pm_memusage_t usage;
    uint64_t wss;
    int oomadj;
};

static void usage(char *myname);
static std::string getprocname(pid_t pid);
static int getoomadj(pid_t pid);
static bool getminfree(std::vector<uint64_t>* minfree, std::vector<int>* adj);
static int numcmp(uint64_t a, uint64_t b);

#define declare_sort(field) \
    static int sort_by_ ## field (const void *a, const void *b)

declare_sort(vss);
declare_sort(rss);
declare_sort(pss);
declare_sort(uss);
declare_sort(swap);
declare_sort(oomadj);

int (*compfn)(const void *a, const void *b);
static int order;

enum {
    MEMINFO_TOTAL,
    MEMINFO_FREE,
    MEMINFO_BUFFERS,
    MEMINFO_CACHED,
    MEMINFO_SHMEM,
    MEMINFO_SLAB,
    MEMINFO_SWAP_TOTAL,
    MEMINFO_SWAP_FREE,
    MEMINFO_ZRAM_TOTAL,
    MEMINFO_MAPPED,
    MEMINFO_VMALLOC_USED,
    MEMINFO_PAGE_TABLES,
    MEMINFO_KERNEL_STACK,
    MEMINFO_COUNT
};

void get_mem_info(uint64_t mem[]) {
    char buffer[1024];
    unsigned int numFound = 0;

    int fd = open("/proc/meminfo", O_RDONLY);

    if (fd < 0) {
        printf("Unable to open /proc/meminfo: %s\n", strerror(errno));
        return;
    }

    const int len = read(fd, buffer, sizeof(buffer)-1);
    close(fd);

    if (len < 0) {
        printf("Empty /proc/meminfo");
        return;
    }
    buffer[len] = 0;

    static const char* const tags[] = {
            "MemTotal:",
            "MemFree:",
            "Buffers:",
            "Cached:",
            "Shmem:",
            "Slab:",
            "SwapTotal:",
            "SwapFree:",
            "ZRam:",            // not read from meminfo but from /sys/block/zram0
            "Mapped:",
            "VmallocUsed:",
            "PageTables:",
            "KernelStack:",
            NULL
    };
    static const int tagsLen[] = {
            9,
            8,
            8,
            7,
            6,
            5,
            10,
            9,
            5,
            7,
            12,
            11,
            12,
            0
    };

    char* p = buffer;
    while (*p && (numFound < (sizeof(tagsLen) / sizeof(tagsLen[0])))) {
        int i = 0;
        while (tags[i]) {
            if (strncmp(p, tags[i], tagsLen[i]) == 0) {
                p += tagsLen[i];
                while (*p == ' ') p++;
                char* num = p;
                while (*p >= '0' && *p <= '9') p++;
                if (*p != 0) {
                    *p = 0;
                    p++;
                }
                mem[i] = atoll(num);
                numFound++;
                break;
            }
            i++;
        }
        while (*p && *p != '\n') {
            p++;
        }
        if (*p) p++;
    }
}

static uint64_t get_zram_mem_used() {
#define ZRAM_SYSFS "/sys/block/zram0/"
    FILE *f = fopen(ZRAM_SYSFS "mm_stat", "r");
    if (f) {
        uint64_t mem_used_total = 0;

        int matched = fscanf(f, "%*d %*d %" SCNu64 " %*d %*d %*d %*d", &mem_used_total);
        if (matched != 1)
            fprintf(stderr, "warning: failed to parse " ZRAM_SYSFS "mm_stat\n");

        fclose(f);
        return mem_used_total;
    }

    f = fopen(ZRAM_SYSFS "mem_used_total", "r");
    if (f) {
        uint64_t mem_used_total = 0;

        int matched = fscanf(f, "%" SCNu64, &mem_used_total);
        if (matched != 1)
            fprintf(stderr, "warning: failed to parse " ZRAM_SYSFS "mem_used_total\n");

        fclose(f);
        return mem_used_total;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    pm_kernel_t *ker;
    pm_process_t *proc;
    pid_t *pids;
    size_t num_procs;
    uint64_t total_pss;
    uint64_t total_uss;
    uint64_t total_swap;
    uint64_t total_pswap;
    uint64_t total_uswap;
    uint64_t total_zswap;
    int error;
    bool has_swap = false, has_zram = false;
    uint64_t required_flags = 0;
    uint64_t flags_mask = 0;

    int arg;
    size_t i;

    enum {
        WS_OFF,
        WS_ONLY,
        WS_RESET,
    } ws;

    uint64_t mem[MEMINFO_COUNT] = { };
    pm_proportional_swap_t *p_swap;
    float zram_cr = 0.0;

    signal(SIGPIPE, SIG_IGN);
    compfn = &sort_by_pss;
    order = -1;
    ws = WS_OFF;
    bool oomadj = false;

    for (arg = 1; arg < argc; arg++) {
        if (!strcmp(argv[arg], "-v")) { compfn = &sort_by_vss; continue; }
        if (!strcmp(argv[arg], "-r")) { compfn = &sort_by_rss; continue; }
        if (!strcmp(argv[arg], "-p")) { compfn = &sort_by_pss; continue; }
        if (!strcmp(argv[arg], "-u")) { compfn = &sort_by_uss; continue; }
        if (!strcmp(argv[arg], "-s")) { compfn = &sort_by_swap; continue; }
        if (!strcmp(argv[arg], "-o")) { compfn = &sort_by_oomadj; oomadj = true; continue; }
        if (!strcmp(argv[arg], "-c")) { required_flags = 0; flags_mask = KPF_SWAPBACKED; continue; }
        if (!strcmp(argv[arg], "-C")) { required_flags = flags_mask = KPF_SWAPBACKED; continue; }
        if (!strcmp(argv[arg], "-k")) { required_flags = flags_mask = KPF_KSM; continue; }
        if (!strcmp(argv[arg], "-w")) { ws = WS_ONLY; continue; }
        if (!strcmp(argv[arg], "-W")) { ws = WS_RESET; continue; }
        if (!strcmp(argv[arg], "-R")) { order *= -1; continue; }
        if (!strcmp(argv[arg], "-h")) { usage(argv[0]); exit(0); }
        fprintf(stderr, "Invalid argument \"%s\".\n", argv[arg]);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    get_mem_info(mem);
    p_swap = pm_memusage_pswap_create(mem[MEMINFO_SWAP_TOTAL] * 1024);

    error = pm_kernel_create(&ker);
    if (error) {
        fprintf(stderr, "Error creating kernel interface -- "
                        "does this kernel have pagemap?\n");
        exit(EXIT_FAILURE);
    }

    error = pm_kernel_pids(ker, &pids, &num_procs);
    if (error) {
        fprintf(stderr, "Error listing processes.\n");
        exit(EXIT_FAILURE);
    }

    std::vector<proc_info> procs(num_procs);
    for (i = 0; i < num_procs; i++) {
        procs[i].pid = pids[i];
        procs[i].oomadj = getoomadj(pids[i]);
        pm_memusage_zero(&procs[i].usage);
        pm_memusage_pswap_init_handle(&procs[i].usage, p_swap);
        error = pm_process_create(ker, pids[i], &proc);
        if (error) {
            fprintf(stderr, "warning: could not create process interface for %d\n", pids[i]);
            continue;
        }

        switch (ws) {
        case WS_OFF:
            error = pm_process_usage_flags(proc, &procs[i].usage, flags_mask,
                                           required_flags);
            break;
        case WS_ONLY:
            error = pm_process_workingset(proc, &procs[i].usage, 0);
            break;
        case WS_RESET:
            error = pm_process_workingset(proc, NULL, 1);
            break;
        }

        if (error) {
            fprintf(stderr, "warning: could not read usage for %d\n", pids[i]);
        }

        if (ws != WS_RESET && procs[i].usage.swap) {
            has_swap = true;
        }

        pm_process_destroy(proc);
    }

    free(pids);

    if (ws == WS_RESET) exit(0);

    procs.erase(std::remove_if(procs.begin(),
                               procs.end(),
                               [](auto proc){
                                   return proc.usage.vss == 0;
                               }),
                procs.end());

    qsort(procs.data(), procs.size(), sizeof(procs[0]), compfn);

    if (has_swap) {
        uint64_t zram_mem_used = get_zram_mem_used();
        if (zram_mem_used) {
            mem[MEMINFO_ZRAM_TOTAL] = zram_mem_used/1024;
            zram_cr = (float) mem[MEMINFO_ZRAM_TOTAL] /
                    (mem[MEMINFO_SWAP_TOTAL] - mem[MEMINFO_SWAP_FREE]);
            has_zram = true;
        }
    }

    printf("%5s  ", "PID");
    if (oomadj) {
        printf("%5s  ", "oom");
    }
    if (ws) {
        printf("%7s  %7s  %7s  ", "WRss", "WPss", "WUss");
        if (has_swap) {
            printf("%7s  %7s  %7s  ", "WSwap", "WPSwap", "WUSwap");
            if (has_zram) {
                printf("%7s  ", "WZSwap");
            }
        }
    } else {
        printf("%8s  %7s  %7s  %7s  ", "Vss", "Rss", "Pss", "Uss");
        if (has_swap) {
            printf("%7s  %7s  %7s  ", "Swap", "PSwap", "USwap");
            if (has_zram) {
                printf("%7s  ", "ZSwap");
            }
        }
    }

    printf("%s\n", "cmdline");

    total_pss = 0;
    total_uss = 0;
    total_swap = 0;
    total_pswap = 0;
    total_uswap = 0;
    total_zswap = 0;

    std::vector<uint64_t> lmk_minfree;
    std::vector<int> lmk_adj;
    if (oomadj) {
        getminfree(&lmk_minfree, &lmk_adj);
    }
    auto lmk_minfree_it = lmk_minfree.cbegin();
    auto lmk_adj_it = lmk_adj.cbegin();

    auto print_oomadj_totals = [&](int adj){
        for (; lmk_adj_it != lmk_adj.cend() && lmk_minfree_it != lmk_minfree.cend() &&
                 adj > *lmk_adj_it; lmk_adj_it++, lmk_minfree_it++) {
            // Print the cumulative total line
            printf("%5s  ", ""); // pid

            printf("%5s  ", ""); // oomadj

            if (ws) {
                printf("%7s  %6" PRIu64 "K  %6" PRIu64 "K  ",
                       "", total_pss / 1024, total_uss / 1024);
            } else {
                printf("%8s  %7s  %6" PRIu64 "K  %6" PRIu64 "K  ",
                       "", "", total_pss / 1024, total_uss / 1024);
            }

            if (has_swap) {
                printf("%6" PRIu64 "K  ", total_swap / 1024);
                printf("%6" PRIu64 "K  ", total_pswap / 1024);
                printf("%6" PRIu64 "K  ", total_uswap / 1024);
                if (has_zram) {
                    printf("%6" PRIu64 "K  ", total_zswap / 1024);
                }
            }

            printf("TOTAL for oomadj < %d (%6" PRIu64 "K)\n", *lmk_adj_it, *lmk_minfree_it / 1024);
        }
    };

    for (auto& proc: procs) {
        if (oomadj) {
            print_oomadj_totals(proc.oomadj);
        }

        std::string cmdline = getprocname(proc.pid);

        total_pss += proc.usage.pss;
        total_uss += proc.usage.uss;
        total_swap += proc.usage.swap;

        printf("%5d  ", proc.pid);

        if (oomadj) {
            printf("%5d  ", proc.oomadj);
        }

        if (ws) {
            printf("%6zuK  %6zuK  %6zuK  ",
                proc.usage.rss / 1024,
                proc.usage.pss / 1024,
                proc.usage.uss / 1024
            );
        } else {
            printf("%7zuK  %6zuK  %6zuK  %6zuK  ",
                proc.usage.vss / 1024,
                proc.usage.rss / 1024,
                proc.usage.pss / 1024,
                proc.usage.uss / 1024
            );
        }

        if (has_swap) {
            pm_swapusage_t su;

            pm_memusage_pswap_get_usage(&proc.usage, &su);
            printf("%6zuK  ", proc.usage.swap / 1024);
            printf("%6zuK  ", su.proportional / 1024);
            printf("%6zuK  ", su.unique / 1024);
            total_pswap += su.proportional;
            total_uswap += su.unique;
            pm_memusage_pswap_free(&proc.usage);
            if (has_zram) {
                size_t zpswap = su.proportional * zram_cr;
                printf("%6zuK  ", zpswap / 1024);
                total_zswap += zpswap;
            }
        }

        printf("%s\n", cmdline.c_str());
    }

    pm_memusage_pswap_destroy(p_swap);

    if (oomadj) {
        print_oomadj_totals(INT_MAX);
    }

    // Print the separator line
    printf("%5s  ", "");

    if (oomadj) {
        printf("%5s  ", "");
    }

    if (ws) {
        printf("%7s  %7s  %7s  ", "", "------", "------");
    } else {
        printf("%8s  %7s  %7s  %7s  ", "", "", "------", "------");
    }

    if (has_swap) {
        printf("%7s  %7s  %7s  ", "------", "------", "------");
        if (has_zram) {
            printf("%7s  ", "------");
        }
    }

    printf("%s\n", "------");

    // Print the total line
    printf("%5s  ", "");

    if (oomadj) {
        printf("%5s  ", "");
    }

    if (ws) {
        printf("%7s  %6" PRIu64 "K  %6" PRIu64 "K  ",
            "", total_pss / 1024, total_uss / 1024);
    } else {
        printf("%8s  %7s  %6" PRIu64 "K  %6" PRIu64 "K  ",
            "", "", total_pss / 1024, total_uss / 1024);
    }

    if (has_swap) {
        printf("%6" PRIu64 "K  ", total_swap / 1024);
        printf("%6" PRIu64 "K  ", total_pswap / 1024);
        printf("%6" PRIu64 "K  ", total_uswap / 1024);
        if (has_zram) {
            printf("%6" PRIu64 "K  ", total_zswap / 1024);
        }
    }

    printf("TOTAL\n");

    printf("\n");

    if (has_swap) {
        printf("ZRAM: %" PRIu64 "K physical used for %" PRIu64 "K in swap "
                "(%" PRIu64 "K total swap)\n",
                mem[MEMINFO_ZRAM_TOTAL], (mem[MEMINFO_SWAP_TOTAL] - mem[MEMINFO_SWAP_FREE]),
                mem[MEMINFO_SWAP_TOTAL]);
    }
    printf(" RAM: %" PRIu64 "K total, %" PRIu64 "K free, %" PRIu64 "K buffers, "
            "%" PRIu64 "K cached, %" PRIu64 "K shmem, %" PRIu64 "K slab\n",
            mem[MEMINFO_TOTAL], mem[MEMINFO_FREE], mem[MEMINFO_BUFFERS],
            mem[MEMINFO_CACHED], mem[MEMINFO_SHMEM], mem[MEMINFO_SLAB]);

    return 0;
}

static void usage(char *myname) {
    fprintf(stderr, "Usage: %s [ -W ] [ -v | -r | -p | -u | -s | -h ]\n"
                    "    -v  Sort by VSS.\n"
                    "    -r  Sort by RSS.\n"
                    "    -p  Sort by PSS.\n"
                    "    -u  Sort by USS.\n"
                    "    -s  Sort by swap.\n"
                    "        (Default sort order is PSS.)\n"
                    "    -R  Reverse sort order (default is descending).\n"
                    "    -c  Only show cached (storage backed) pages\n"
                    "    -C  Only show non-cached (ram/swap backed) pages\n"
                    "    -k  Only show pages collapsed by KSM\n"
                    "    -w  Display statistics for working set only.\n"
                    "    -W  Reset working set of all processes.\n"
                    "    -o  Show and sort by oom score against lowmemorykiller thresholds.\n"
                    "    -h  Display this help screen.\n",
    myname);
}

// Get the process name for a given PID.
static std::string getprocname(pid_t pid) {
    std::string filename = android::base::StringPrintf("/proc/%d/cmdline", pid);

    std::string procname;

    if (!android::base::ReadFileToString(filename, &procname)) {
        // The process went away before we could read its process name.
        procname = "<unknown>";
    }

    return procname;
}

static int getoomadj(pid_t pid) {
    std::string filename = android::base::StringPrintf("/proc/%d/oom_score_adj", pid);
    std::string oomadj;

    if (!android::base::ReadFileToString(filename, &oomadj)) {
        return -1001;
    }

    return strtol(oomadj.c_str(), NULL, 10);
}

static bool getminfree(std::vector<uint64_t>* minfree, std::vector<int>* adj) {
    std::string minfree_str;
    std::string adj_str;

    if (!android::base::ReadFileToString("/sys/module/lowmemorykiller/parameters/minfree", &minfree_str)) {
        return false;
    }

    if (!android::base::ReadFileToString("/sys/module/lowmemorykiller/parameters/adj", &adj_str)) {
        return false;
    }

    std::vector<std::string> minfree_vec = android::base::Split(minfree_str, ",");
    std::vector<std::string> adj_vec = android::base::Split(adj_str, ",");

    minfree->clear();
    minfree->resize(minfree_vec.size());
    adj->clear();
    adj->resize(adj_vec.size());

    std::transform(minfree_vec.begin(), minfree_vec.end(), minfree->begin(),
                   [](const std::string& s) -> uint64_t {
                       return strtoull(s.c_str(), NULL, 10) * PAGE_SIZE;
                   });

    std::transform(adj_vec.begin(), adj_vec.end(), adj->begin(),
                   [](const std::string& s) -> int {
                       return strtol(s.c_str(), NULL, 10);
                   });

    return true;
}

static int numcmp(uint64_t a, uint64_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int snumcmp(int64_t a, int64_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

#define create_sort(field, compfn) \
    static int sort_by_ ## field (const void *a, const void *b) { \
        return order * compfn( \
            ((struct proc_info*)(a))->usage.field, \
            ((struct proc_info*)(b))->usage.field  \
        ); \
    }

create_sort(vss, numcmp)
create_sort(rss, numcmp)
create_sort(pss, numcmp)
create_sort(uss, numcmp)
create_sort(swap, numcmp)

static int sort_by_oomadj (const void *a, const void *b) {
    // Negative oomadj is higher priority, reverse the sort order
    return -1 * order * snumcmp(
        ((struct proc_info*)a)->oomadj,
        ((struct proc_info*)b)->oomadj
        );
}
