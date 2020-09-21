/*
 * Copyright (C) 2008 The Android Open Source Project
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
 */

#ifndef FRAMEWORK_NATIVE_CMD_DUMPSTATE_H_
#define FRAMEWORK_NATIVE_CMD_DUMPSTATE_H_

#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include <string>
#include <vector>

#include <android-base/macros.h>
#include <android-base/unique_fd.h>
#include <android/os/IDumpstateListener.h>
#include <utils/StrongPointer.h>
#include <ziparchive/zip_writer.h>

#include "DumpstateUtil.h"

// Workaround for const char *args[MAX_ARGS_ARRAY_SIZE] variables until they're converted to
// std::vector<std::string>
// TODO: remove once not used
#define MAX_ARGS_ARRAY_SIZE 1000

// TODO: move everything under this namespace
// TODO: and then remove explicitly android::os::dumpstate:: prefixes
namespace android {
namespace os {
namespace dumpstate {

class DumpstateTest;
class ProgressTest;

}  // namespace dumpstate
}  // namespace os
}  // namespace android

class ZipWriter;

// TODO: remove once moved to HAL
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Helper class used to report how long it takes for a section to finish.
 *
 * Typical usage:
 *
 *    DurationReporter duration_reporter(title);
 *
 */
class DurationReporter {
  public:
    DurationReporter(const std::string& title, bool log_only = false);

    ~DurationReporter();

  private:
    std::string title_;
    bool log_only_;
    uint64_t started_;

    DISALLOW_COPY_AND_ASSIGN(DurationReporter);
};

/*
 * Keeps track of current progress and estimated max, saving stats on file to tune up future runs.
 *
 * Each `dumpstate` section contributes to the total weight by an individual weight, so the overall
 * progress can be calculated by dividing the estimate max progress by the current progress.
 *
 * The estimated max progress is initially set to a value (`kDefaultMax) defined empirically, but
 * it's adjusted after each dumpstate run by storing the average duration in a file.
 *
 */
class Progress {
    friend class android::os::dumpstate::ProgressTest;
    friend class android::os::dumpstate::DumpstateTest;

  public:
    /*
     * Default estimation of the max duration of a bugreport generation.
     *
     * It does not need to match the exact sum of all sections, but ideally it should to be slight
     * more than such sum: a value too high will cause the bugreport to finish before the user
     * expected (for example, jumping from 70% to 100%), while a value too low will cause the
     * progress to get stuck at an almost-finished value (like 99%) for a while.
     *
     * This constant is only used when the average duration from previous runs cannot be used.
     */
    static const int kDefaultMax;

    Progress(const std::string& path = "");

    // Gets the current progress.
    int32_t Get() const;

    // Gets the current estimated max progress.
    int32_t GetMax() const;

    // Gets the initial estimated max progress.
    int32_t GetInitialMax() const;

    // Increments progress (ignored if not positive).
    // Returns `true` if the max progress increased as well.
    bool Inc(int32_t delta);

    // Persist the stats.
    void Save();

    void Dump(int fd, const std::string& prefix) const;

  private:
    Progress(int32_t initial_max, float growth_factor,
             const std::string& path = "");                                // Used by test cases.
    Progress(int32_t initial_max, int32_t progress, float growth_factor);  // Used by test cases.
    void Load();
    int32_t initial_max_;
    int32_t progress_;
    int32_t max_;
    float growth_factor_;
    int32_t n_runs_;
    int32_t average_max_;
    const std::string& path_;
};

/*
 * List of supported zip format versions.
 *
 * See bugreport-format.md for more info.
 */
static std::string VERSION_CURRENT = "2.0";

/*
 * Temporary version that adds a anr-traces.txt entry. Once tools support it, the current version
 * will be bumped to 3.0.
 */
static std::string VERSION_SPLIT_ANR = "3.0-dev-split-anr";

/*
 * "Alias" for the current version.
 */
static std::string VERSION_DEFAULT = "default";

/*
 * Structure that contains the information of an open dump file.
 */
struct DumpData {
    // Path of the file.
    std::string name;

    // Open file descriptor for the file.
    android::base::unique_fd fd;

    // Modification time of the file.
    time_t mtime;
};

/*
 * Main class driving a bugreport generation.
 *
 * Currently, it only contains variables that are accessed externally, but gradually the functions
 * that are spread accross utils.cpp and dumpstate.cpp will be moved to it.
 */
class Dumpstate {
    friend class DumpstateTest;

  public:
    static android::os::dumpstate::CommandOptions DEFAULT_DUMPSYS;

    static Dumpstate& GetInstance();

    /* Checkes whether dumpstate is generating a zipped bugreport. */
    bool IsZipping() const;

    /*
     * Forks a command, waits for it to finish, and returns its status.
     *
     * |title| description of the command printed on `stdout` (or empty to skip
     * description).
     * |full_command| array containing the command (first entry) and its arguments.
     * Must contain at least one element.
     * |options| optional argument defining the command's behavior.
     */
    int RunCommand(const std::string& title, const std::vector<std::string>& fullCommand,
                   const android::os::dumpstate::CommandOptions& options =
                       android::os::dumpstate::CommandOptions::DEFAULT);

    /*
     * Runs `dumpsys` with the given arguments, automatically setting its timeout
     * (`-T` argument)
     * according to the command options.
     *
     * |title| description of the command printed on `stdout` (or empty to skip
     * description).
     * |dumpsys_args| `dumpsys` arguments (except `-t`).
     * |options| optional argument defining the command's behavior.
     * |dumpsys_timeout| when > 0, defines the value passed to `dumpsys -T` (otherwise it uses the
     * timeout from `options`)
     */
    void RunDumpsys(const std::string& title, const std::vector<std::string>& dumpsys_args,
                    const android::os::dumpstate::CommandOptions& options = DEFAULT_DUMPSYS,
                    long dumpsys_timeout_ms = 0);

    /*
     * Prints the contents of a file.
     *
     * |title| description of the command printed on `stdout` (or empty to skip
     * description).
     * |path| location of the file to be dumped.
     */
    int DumpFile(const std::string& title, const std::string& path);

    /*
     * Adds a new entry to the existing zip file.
     * */
    bool AddZipEntry(const std::string& entry_name, const std::string& entry_path);

    /*
     * Adds a new entry to the existing zip file.
     *
     * |entry_name| destination path of the new entry.
     * |fd| file descriptor to read from.
     * |timeout| timeout to terminate the read if not completed. Set
     * value of 0s (default) to disable timeout.
     */
    android::status_t AddZipEntryFromFd(const std::string& entry_name, int fd,
                                        std::chrono::milliseconds timeout);

    /*
     * Adds a text entry entry to the existing zip file.
     */
    bool AddTextZipEntry(const std::string& entry_name, const std::string& content);

    /*
     * Adds all files from a directory to the zipped bugreport file.
     */
    void AddDir(const std::string& dir, bool recursive);

    /*
     * Takes a screenshot and save it to the given `path`.
     *
     * If `path` is empty, uses a standard path based on the bugreport name.
     */
    void TakeScreenshot(const std::string& path = "");

    /////////////////////////////////////////////////////////////////////
    // TODO: members below should be private once refactor is finished //
    /////////////////////////////////////////////////////////////////////

    // TODO: temporary method until Dumpstate object is properly set
    void SetProgress(std::unique_ptr<Progress> progress);

    void DumpstateBoard();

    /*
     * Updates the overall progress of the bugreport generation by the given weight increment.
     */
    void UpdateProgress(int32_t delta);

    /* Prints the dumpstate header on `stdout`. */
    void PrintHeader() const;

    /*
     * Adds the temporary report to the existing .zip file, closes the .zip file, and removes the
     * temporary file.
     */
    bool FinishZipFile();

    /* Gets the path of a bugreport file with the given suffix. */
    std::string GetPath(const std::string& suffix) const;

    /* Returns true if the current version supports priority dump feature. */
    bool CurrentVersionSupportsPriorityDumps() const;

    // TODO: initialize fields on constructor

    // dumpstate id - unique after each device reboot.
    uint32_t id_;

    // dumpstate pid
    pid_t pid_;

    // Whether progress updates should be published.
    bool update_progress_ = false;

    // How frequently the progess should be updated;the listener will only be notificated when the
    // delta from the previous update is more than the threshold.
    int32_t update_progress_threshold_ = 100;

    // Last progress that triggered a listener updated
    int32_t last_updated_progress_;

    // Whether it should take an screenshot earlier in the process.
    bool do_early_screenshot_ = false;

    std::unique_ptr<Progress> progress_;

    // When set, defines a socket file-descriptor use to report progress to bugreportz.
    int control_socket_fd_ = -1;

    // Bugreport format version;
    std::string version_ = VERSION_CURRENT;

    // Command-line arguments as string
    std::string args_;

    // Extra options passed as system property.
    std::string extra_options_;

    // Full path of the directory where the bugreport files will be written.
    std::string bugreport_dir_;

    // Full path of the temporary file containing the screenshot (when requested).
    std::string screenshot_path_;

    time_t now_;

    // Base name (without suffix or extensions) of the bugreport files, typically
    // `bugreport-BUILD_ID`.
    std::string base_name_;

    // Name is the suffix part of the bugreport files - it's typically the date (when invoked with
    // `-d`), but it could be changed by the user..
    std::string name_;

    // Full path of the temporary file containing the bugreport.
    std::string tmp_path_;

    // Full path of the file containing the dumpstate logs.
    std::string log_path_;

    // Pointer to the actual path, be it zip or text.
    std::string path_;

    // Pointer to the zipped file.
    std::unique_ptr<FILE, int (*)(FILE*)> zip_file{nullptr, fclose};

    // Pointer to the zip structure.
    std::unique_ptr<ZipWriter> zip_writer_;

    // Binder object listening to progress.
    android::sp<android::os::IDumpstateListener> listener_;
    std::string listener_name_;
    bool report_section_;

    // Notification title and description
    std::string notification_title;
    std::string notification_description;

    // List of open tombstone dump files.
    std::vector<DumpData> tombstone_data_;

    // List of open ANR dump files.
    std::vector<DumpData> anr_data_;

  private:
    // Used by GetInstance() only.
    Dumpstate(const std::string& version = VERSION_CURRENT);

    DISALLOW_COPY_AND_ASSIGN(Dumpstate);
};

// for_each_pid_func = void (*)(int, const char*);
// for_each_tid_func = void (*)(int, int, const char*);

typedef void(for_each_pid_func)(int, const char*);
typedef void(for_each_tid_func)(int, int, const char*);

/* saves the the contents of a file as a long */
int read_file_as_long(const char *path, long int *output);

/* prints the contents of the fd
 * fd must have been opened with the flag O_NONBLOCK.
 */
int dump_file_from_fd(const char *title, const char *path, int fd);

/* calls skip to gate calling dump_from_fd recursively
 * in the specified directory. dump_from_fd defaults to
 * dump_file_from_fd above when set to NULL. skip defaults
 * to false when set to NULL. dump_from_fd will always be
 * called with title NULL.
 */
int dump_files(const std::string& title, const char* dir, bool (*skip)(const char* path),
               int (*dump_from_fd)(const char* title, const char* path, int fd));

/** opens a socket and returns its file descriptor */
int open_socket(const char *service);

/* redirect output to a service control socket */
void redirect_to_socket(FILE *redirect, const char *service);

/* redirect output to a new file */
void redirect_to_file(FILE *redirect, char *path);

/* redirect output to an existing file */
void redirect_to_existing_file(FILE *redirect, char *path);

/* create leading directories, if necessary */
void create_parent_dirs(const char *path);

/* dump Dalvik and native stack traces, return the trace file location (NULL if none) */
const char *dump_traces();

/* for each process in the system, run the specified function */
void for_each_pid(for_each_pid_func func, const char *header);

/* for each thread in the system, run the specified function */
void for_each_tid(for_each_tid_func func, const char *header);

/* Displays a blocked processes in-kernel wait channel */
void show_wchan(int pid, int tid, const char *name);

/* Displays a processes times */
void show_showtime(int pid, const char *name);

/* Runs "showmap" for a process */
void do_showmap(int pid, const char *name);

/* Gets the dmesg output for the kernel */
void do_dmesg();

/* Prints the contents of all the routing tables, both IPv4 and IPv6. */
void dump_route_tables();

/* Play a sound via Stagefright */
void play_sound(const char *path);

/* Checks if a given path is a directory. */
bool is_dir(const char* pathname);

/** Gets the last modification time of a file, or default time if file is not found. */
time_t get_mtime(int fd, time_t default_mtime);

/* Dumps eMMC Extended CSD data. */
void dump_emmc_ecsd(const char *ext_csd_path);

/** Gets command-line arguments. */
void format_args(int argc, const char *argv[], std::string *args);

/** Main entry point for dumpstate. */
int run_main(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEWORK_NATIVE_CMD_DUMPSTATE_H_ */
