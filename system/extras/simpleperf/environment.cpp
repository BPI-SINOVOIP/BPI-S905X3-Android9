/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "environment.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/utsname.h>

#include <limits>
#include <set>
#include <unordered_map>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <procinfo/process.h>

#if defined(__ANDROID__)
#include <android-base/properties.h>
#endif

#include "event_type.h"
#include "IOEventLoop.h"
#include "read_elf.h"
#include "thread_tree.h"
#include "utils.h"
#include "workload.h"

class LineReader {
 public:
  explicit LineReader(FILE* fp) : fp_(fp), buf_(nullptr), bufsize_(0) {
  }

  ~LineReader() {
    free(buf_);
    fclose(fp_);
  }

  char* ReadLine() {
    if (getline(&buf_, &bufsize_, fp_) != -1) {
      return buf_;
    }
    return nullptr;
  }

  size_t MaxLineSize() {
    return bufsize_;
  }

 private:
  FILE* fp_;
  char* buf_;
  size_t bufsize_;
};

std::vector<int> GetOnlineCpus() {
  std::vector<int> result;
  FILE* fp = fopen("/sys/devices/system/cpu/online", "re");
  if (fp == nullptr) {
    PLOG(ERROR) << "can't open online cpu information";
    return result;
  }

  LineReader reader(fp);
  char* line;
  if ((line = reader.ReadLine()) != nullptr) {
    result = GetCpusFromString(line);
  }
  CHECK(!result.empty()) << "can't get online cpu information";
  return result;
}

std::vector<int> GetCpusFromString(const std::string& s) {
  std::set<int> cpu_set;
  bool have_dash = false;
  const char* p = s.c_str();
  char* endp;
  int last_cpu;
  int cpu;
  // Parse line like: 0,1-3, 5, 7-8
  while ((cpu = static_cast<int>(strtol(p, &endp, 10))) != 0 || endp != p) {
    if (have_dash && !cpu_set.empty()) {
      for (int t = last_cpu + 1; t < cpu; ++t) {
        cpu_set.insert(t);
      }
    }
    have_dash = false;
    cpu_set.insert(cpu);
    last_cpu = cpu;
    p = endp;
    while (!isdigit(*p) && *p != '\0') {
      if (*p == '-') {
        have_dash = true;
      }
      ++p;
    }
  }
  return std::vector<int>(cpu_set.begin(), cpu_set.end());
}

static std::vector<KernelMmap> GetLoadedModules() {
  std::vector<KernelMmap> result;
  FILE* fp = fopen("/proc/modules", "re");
  if (fp == nullptr) {
    // There is no /proc/modules on Android devices, so we don't print error if failed to open it.
    PLOG(DEBUG) << "failed to open file /proc/modules";
    return result;
  }
  LineReader reader(fp);
  char* line;
  while ((line = reader.ReadLine()) != nullptr) {
    // Parse line like: nf_defrag_ipv6 34768 1 nf_conntrack_ipv6, Live 0xffffffffa0fe5000
    char name[reader.MaxLineSize()];
    uint64_t addr;
    uint64_t len;
    if (sscanf(line, "%s%" PRIu64 "%*u%*s%*s 0x%" PRIx64, name, &len, &addr) == 3) {
      KernelMmap map;
      map.name = name;
      map.start_addr = addr;
      map.len = len;
      result.push_back(map);
    }
  }
  bool all_zero = true;
  for (const auto& map : result) {
    if (map.start_addr != 0) {
      all_zero = false;
    }
  }
  if (all_zero) {
    LOG(DEBUG) << "addresses in /proc/modules are all zero, so ignore kernel modules";
    return std::vector<KernelMmap>();
  }
  return result;
}

static void GetAllModuleFiles(const std::string& path,
                              std::unordered_map<std::string, std::string>* module_file_map) {
  for (const auto& name : GetEntriesInDir(path)) {
    std::string entry_path = path + "/" + name;
    if (IsRegularFile(entry_path) && android::base::EndsWith(name, ".ko")) {
      std::string module_name = name.substr(0, name.size() - 3);
      std::replace(module_name.begin(), module_name.end(), '-', '_');
      module_file_map->insert(std::make_pair(module_name, entry_path));
    } else if (IsDir(entry_path)) {
      GetAllModuleFiles(entry_path, module_file_map);
    }
  }
}

static std::vector<KernelMmap> GetModulesInUse() {
  std::vector<KernelMmap> module_mmaps = GetLoadedModules();
  if (module_mmaps.empty()) {
    return std::vector<KernelMmap>();
  }
  std::unordered_map<std::string, std::string> module_file_map;
#if defined(__ANDROID__)
  // Search directories listed in "File locations" section in
  // https://source.android.com/devices/architecture/kernel/modular-kernels.
  for (const auto& path : {"/vendor/lib/modules", "/odm/lib/modules", "/lib/modules"}) {
    GetAllModuleFiles(path, &module_file_map);
  }
#else
  utsname uname_buf;
  if (TEMP_FAILURE_RETRY(uname(&uname_buf)) != 0) {
    PLOG(ERROR) << "uname() failed";
    return std::vector<KernelMmap>();
  }
  std::string linux_version = uname_buf.release;
  std::string module_dirpath = "/lib/modules/" + linux_version + "/kernel";
  GetAllModuleFiles(module_dirpath, &module_file_map);
#endif
  for (auto& module : module_mmaps) {
    auto it = module_file_map.find(module.name);
    if (it != module_file_map.end()) {
      module.filepath = it->second;
    }
  }
  return module_mmaps;
}

void GetKernelAndModuleMmaps(KernelMmap* kernel_mmap, std::vector<KernelMmap>* module_mmaps) {
  kernel_mmap->name = DEFAULT_KERNEL_MMAP_NAME;
  kernel_mmap->start_addr = 0;
  kernel_mmap->len = std::numeric_limits<uint64_t>::max();
  kernel_mmap->filepath = kernel_mmap->name;
  *module_mmaps = GetModulesInUse();
  for (auto& map : *module_mmaps) {
    if (map.filepath.empty()) {
      map.filepath = "[" + map.name + "]";
    }
  }
}

static bool ReadThreadNameAndPid(pid_t tid, std::string* comm, pid_t* pid) {
  android::procinfo::ProcessInfo procinfo;
  if (!android::procinfo::GetProcessInfo(tid, &procinfo)) {
    return false;
  }
  if (comm != nullptr) {
    *comm = procinfo.name;
  }
  if (pid != nullptr) {
    *pid = procinfo.pid;
  }
  return true;
}

std::vector<pid_t> GetThreadsInProcess(pid_t pid) {
  std::vector<pid_t> result;
  android::procinfo::GetProcessTids(pid, &result);
  return result;
}

bool IsThreadAlive(pid_t tid) {
  return IsDir(android::base::StringPrintf("/proc/%d", tid));
}

bool GetProcessForThread(pid_t tid, pid_t* pid) {
  return ReadThreadNameAndPid(tid, nullptr, pid);
}

bool GetThreadName(pid_t tid, std::string* name) {
  return ReadThreadNameAndPid(tid, name, nullptr);
}

std::vector<pid_t> GetAllProcesses() {
  std::vector<pid_t> result;
  std::vector<std::string> entries = GetEntriesInDir("/proc");
  for (const auto& entry : entries) {
    pid_t pid;
    if (!android::base::ParseInt(entry.c_str(), &pid, 0)) {
      continue;
    }
    result.push_back(pid);
  }
  return result;
}

bool GetThreadMmapsInProcess(pid_t pid, std::vector<ThreadMmap>* thread_mmaps) {
  std::string map_file = android::base::StringPrintf("/proc/%d/maps", pid);
  FILE* fp = fopen(map_file.c_str(), "re");
  if (fp == nullptr) {
    PLOG(DEBUG) << "can't open file " << map_file;
    return false;
  }
  thread_mmaps->clear();
  LineReader reader(fp);
  char* line;
  while ((line = reader.ReadLine()) != nullptr) {
    // Parse line like: 00400000-00409000 r-xp 00000000 fc:00 426998  /usr/lib/gvfs/gvfsd-http
    uint64_t start_addr, end_addr, pgoff;
    char type[reader.MaxLineSize()];
    char execname[reader.MaxLineSize()];
    strcpy(execname, "");
    if (sscanf(line, "%" PRIx64 "-%" PRIx64 " %s %" PRIx64 " %*x:%*x %*u %s\n", &start_addr,
               &end_addr, type, &pgoff, execname) < 4) {
      continue;
    }
    if (strcmp(execname, "") == 0) {
      strcpy(execname, DEFAULT_EXECNAME_FOR_THREAD_MMAP);
    }
    ThreadMmap thread;
    thread.start_addr = start_addr;
    thread.len = end_addr - start_addr;
    thread.pgoff = pgoff;
    thread.name = execname;
    thread.executable = (type[2] == 'x');
    thread_mmaps->push_back(thread);
  }
  return true;
}

bool GetKernelBuildId(BuildId* build_id) {
  ElfStatus result = GetBuildIdFromNoteFile("/sys/kernel/notes", build_id);
  if (result != ElfStatus::NO_ERROR) {
    LOG(DEBUG) << "failed to read /sys/kernel/notes: " << result;
  }
  return result == ElfStatus::NO_ERROR;
}

bool GetModuleBuildId(const std::string& module_name, BuildId* build_id) {
  std::string notefile = "/sys/module/" + module_name + "/notes/.note.gnu.build-id";
  return GetBuildIdFromNoteFile(notefile, build_id);
}

bool GetValidThreadsFromThreadString(const std::string& tid_str, std::set<pid_t>* tid_set) {
  std::vector<std::string> strs = android::base::Split(tid_str, ",");
  for (const auto& s : strs) {
    int tid;
    if (!android::base::ParseInt(s.c_str(), &tid, 0)) {
      LOG(ERROR) << "Invalid tid '" << s << "'";
      return false;
    }
    if (!IsDir(android::base::StringPrintf("/proc/%d", tid))) {
      LOG(ERROR) << "Non existing thread '" << tid << "'";
      return false;
    }
    tid_set->insert(tid);
  }
  return true;
}

/*
 * perf event paranoia level:
 *  -1 - not paranoid at all
 *   0 - disallow raw tracepoint access for unpriv
 *   1 - disallow cpu events for unpriv
 *   2 - disallow kernel profiling for unpriv
 *   3 - disallow user profiling for unpriv
 */
static bool ReadPerfEventParanoid(int* value) {
  std::string s;
  if (!android::base::ReadFileToString("/proc/sys/kernel/perf_event_paranoid", &s)) {
    PLOG(DEBUG) << "failed to read /proc/sys/kernel/perf_event_paranoid";
    return false;
  }
  s = android::base::Trim(s);
  if (!android::base::ParseInt(s.c_str(), value)) {
    PLOG(ERROR) << "failed to parse /proc/sys/kernel/perf_event_paranoid: " << s;
    return false;
  }
  return true;
}

bool CanRecordRawData() {
  int value;
  return IsRoot() || (ReadPerfEventParanoid(&value) && value == -1);
}

static const char* GetLimitLevelDescription(int limit_level) {
  switch (limit_level) {
    case -1: return "unlimited";
    case 0: return "disallowing raw tracepoint access for unpriv";
    case 1: return "disallowing cpu events for unpriv";
    case 2: return "disallowing kernel profiling for unpriv";
    case 3: return "disallowing user profiling for unpriv";
    default: return "unknown level";
  }
}

bool CheckPerfEventLimit() {
  // Root is not limited by /proc/sys/kernel/perf_event_paranoid. However, the monitored threads
  // may create child processes not running as root. To make sure the child processes have
  // enough permission to create inherited tracepoint events, write -1 to perf_event_paranoid.
  // See http://b/62230699.
  if (IsRoot() && android::base::WriteStringToFile("-1", "/proc/sys/kernel/perf_event_paranoid")) {
    return true;
  }
  int limit_level;
  bool can_read_paranoid = ReadPerfEventParanoid(&limit_level);
  if (can_read_paranoid && limit_level <= 1) {
    return true;
  }
#if defined(__ANDROID__)
  const std::string prop_name = "security.perf_harden";
  std::string prop_value = android::base::GetProperty(prop_name, "");
  if (prop_value.empty()) {
    // can't do anything if there is no such property.
    return true;
  }
  if (prop_value == "0") {
    return true;
  }
  // Try to enable perf_event_paranoid by setprop security.perf_harden=0.
  if (android::base::SetProperty(prop_name, "0")) {
    sleep(1);
    if (can_read_paranoid && ReadPerfEventParanoid(&limit_level) && limit_level <= 1) {
      return true;
    }
    if (android::base::GetProperty(prop_name, "") == "0") {
      return true;
    }
  }
  if (can_read_paranoid) {
    LOG(WARNING) << "/proc/sys/kernel/perf_event_paranoid is " << limit_level
        << ", " << GetLimitLevelDescription(limit_level) << ".";
  }
  LOG(WARNING) << "Try using `adb shell setprop security.perf_harden 0` to allow profiling.";
  return false;
#else
  if (can_read_paranoid) {
    LOG(WARNING) << "/proc/sys/kernel/perf_event_paranoid is " << limit_level
        << ", " << GetLimitLevelDescription(limit_level) << ".";
    return false;
  }
#endif
  return true;
}

bool GetMaxSampleFrequency(uint64_t* max_sample_freq) {
  std::string s;
  if (!android::base::ReadFileToString("/proc/sys/kernel/perf_event_max_sample_rate", &s)) {
    PLOG(DEBUG) << "failed to read /proc/sys/kernel/perf_event_max_sample_rate";
    return false;
  }
  s = android::base::Trim(s);
  if (!android::base::ParseUint(s.c_str(), max_sample_freq)) {
    LOG(ERROR) << "failed to parse /proc/sys/kernel/perf_event_max_sample_rate: " << s;
    return false;
  }
  return true;
}

bool CheckKernelSymbolAddresses() {
  const std::string kptr_restrict_file = "/proc/sys/kernel/kptr_restrict";
  std::string s;
  if (!android::base::ReadFileToString(kptr_restrict_file, &s)) {
    PLOG(DEBUG) << "failed to read " << kptr_restrict_file;
    return false;
  }
  s = android::base::Trim(s);
  int value;
  if (!android::base::ParseInt(s.c_str(), &value)) {
    LOG(ERROR) << "failed to parse " << kptr_restrict_file << ": " << s;
    return false;
  }
  // Accessible to everyone?
  if (value == 0) {
    return true;
  }
  // Accessible to root?
  if (value == 1 && IsRoot()) {
    return true;
  }
  // Can we make it accessible to us?
  if (IsRoot() && android::base::WriteStringToFile("1", kptr_restrict_file)) {
    return true;
  }
  LOG(WARNING) << "Access to kernel symbol addresses is restricted. If "
      << "possible, please do `echo 0 >/proc/sys/kernel/kptr_restrict` "
      << "to fix this.";
  return false;
}

ArchType GetMachineArch() {
  utsname uname_buf;
  if (TEMP_FAILURE_RETRY(uname(&uname_buf)) != 0) {
    PLOG(WARNING) << "uname() failed";
    return GetBuildArch();
  }
  ArchType arch = GetArchType(uname_buf.machine);
  if (arch != ARCH_UNSUPPORTED) {
    return arch;
  }
  return GetBuildArch();
}

void PrepareVdsoFile() {
  // vdso is an elf file in memory loaded in each process's user space by the kernel. To read
  // symbols from it and unwind through it, we need to dump it into a file in storage.
  // It doesn't affect much when failed to prepare vdso file, so there is no need to return values.
  std::vector<ThreadMmap> thread_mmaps;
  if (!GetThreadMmapsInProcess(getpid(), &thread_mmaps)) {
    return;
  }
  const ThreadMmap* vdso_map = nullptr;
  for (const auto& map : thread_mmaps) {
    if (map.name == "[vdso]") {
      vdso_map = &map;
      break;
    }
  }
  if (vdso_map == nullptr) {
    return;
  }
  std::string s(vdso_map->len, '\0');
  memcpy(&s[0], reinterpret_cast<void*>(static_cast<uintptr_t>(vdso_map->start_addr)),
         vdso_map->len);
  std::unique_ptr<TemporaryFile> tmpfile = ScopedTempFiles::CreateTempFile();
  if (!android::base::WriteStringToFd(s, tmpfile->release())) {
    return;
  }
  Dso::SetVdsoFile(tmpfile->path, sizeof(size_t) == sizeof(uint64_t));
}

static bool HasOpenedAppApkFile(int pid) {
  std::string fd_path = "/proc/" + std::to_string(pid) + "/fd/";
  std::vector<std::string> files = GetEntriesInDir(fd_path);
  for (const auto& file : files) {
    std::string real_path;
    if (!android::base::Readlink(fd_path + file, &real_path)) {
      continue;
    }
    if (real_path.find("app") != std::string::npos && real_path.find(".apk") != std::string::npos) {
      return true;
    }
  }
  return false;
}

std::set<pid_t> WaitForAppProcesses(const std::string& package_name) {
  std::set<pid_t> result;
  size_t loop_count = 0;
  while (true) {
    std::vector<pid_t> pids = GetAllProcesses();
    for (pid_t pid : pids) {
      std::string cmdline;
      if (!android::base::ReadFileToString("/proc/" + std::to_string(pid) + "/cmdline", &cmdline)) {
        // Maybe we don't have permission to read it.
        continue;
      }
      std::string process_name = android::base::Basename(cmdline);
      // The app may have multiple processes, with process name like
      // com.google.android.googlequicksearchbox:search.
      size_t split_pos = process_name.find(':');
      if (split_pos != std::string::npos) {
        process_name = process_name.substr(0, split_pos);
      }
      if (process_name != package_name) {
        continue;
      }
      // If a debuggable app with wrap.sh runs on Android O, the app will be started with
      // logwrapper as below:
      // 1. Zygote forks a child process, rename it to package_name.
      // 2. The child process execute sh, which starts a child process running
      //    /system/bin/logwrapper.
      // 3. logwrapper starts a child process running sh, which interprets wrap.sh.
      // 4. wrap.sh starts a child process running the app.
      // The problem here is we want to profile the process started in step 4, but sometimes we
      // run into the process started in step 1. To solve it, we can check if the process has
      // opened an apk file in some app dirs.
      if (!HasOpenedAppApkFile(pid)) {
        continue;
      }
      if (loop_count > 0u) {
        LOG(INFO) << "Got process " << pid << " for package " << package_name;
      }
      result.insert(pid);
    }
    if (!result.empty()) {
      return result;
    }
    if (++loop_count == 1u) {
      LOG(INFO) << "Waiting for process of app " << package_name;
    }
    usleep(1000);
  }
}

class ScopedFile {
 public:
  ScopedFile(const std::string& filepath, const std::string& app_package_name = "")
      : filepath_(filepath), app_package_name_(app_package_name) {}

  ~ScopedFile() {
    if (app_package_name_.empty()) {
      unlink(filepath_.c_str());
    } else {
      Workload::RunCmd({"run-as", app_package_name_, "rm", "-rf", filepath_});
    }
  }

 private:
  std::string filepath_;
  std::string app_package_name_;
};

bool RunInAppContext(const std::string& app_package_name, const std::string& cmd,
                     const std::vector<std::string>& args, size_t workload_args_size,
                     const std::string& output_filepath, bool need_tracepoint_events) {
  // 1. Test if the package exists.
  if (!Workload::RunCmd({"run-as", app_package_name, "echo", ">/dev/null"}, false)) {
    LOG(ERROR) << "Package " << app_package_name << " doesn't exist or isn't debuggable.";
    return false;
  }

  // 2. Copy simpleperf binary to the package. Create tracepoint_file if needed.
  std::string simpleperf_path;
  if (!android::base::Readlink("/proc/self/exe", &simpleperf_path)) {
    PLOG(ERROR) << "ReadLink failed";
    return false;
  }
  if (!Workload::RunCmd({"run-as", app_package_name, "cp", simpleperf_path, "simpleperf"})) {
    return false;
  }
  ScopedFile scoped_simpleperf("simpleperf", app_package_name);
  std::unique_ptr<ScopedFile> scoped_tracepoint_file;
  const std::string tracepoint_file = "/data/local/tmp/tracepoint_events";
  if (need_tracepoint_events) {
    // Since we can't read tracepoint events from tracefs in app's context, we need to prepare
    // them in tracepoint_file in shell's context, and pass the path of tracepoint_file to the
    // child process using --tracepoint-events option.
    if (!android::base::WriteStringToFile(GetTracepointEvents(), tracepoint_file)) {
      PLOG(ERROR) << "Failed to store tracepoint events";
      return false;
    }
    scoped_tracepoint_file.reset(new ScopedFile(tracepoint_file));
  }

  // 3. Prepare to start child process to profile.
  std::string output_basename = output_filepath.empty() ? "" :
                                    android::base::Basename(output_filepath);
  std::vector<std::string> new_args =
      {"run-as", app_package_name, "./simpleperf", cmd, "--in-app", "--log", GetLogSeverityName()};
  if (need_tracepoint_events) {
    new_args.push_back("--tracepoint-events");
    new_args.push_back(tracepoint_file);
  }
  for (size_t i = 0; i < args.size(); ++i) {
    if (i >= args.size() - workload_args_size || args[i] != "-o") {
      new_args.push_back(args[i]);
    } else {
      new_args.push_back(args[i++]);
      new_args.push_back(output_basename);
    }
  }
  std::unique_ptr<Workload> workload = Workload::CreateWorkload(new_args);
  if (!workload) {
    return false;
  }

  IOEventLoop loop;
  bool need_to_kill_child = false;
  if (!loop.AddSignalEvents({SIGINT, SIGTERM, SIGHUP},
                            [&]() { need_to_kill_child = true; return loop.ExitLoop(); })) {
    return false;
  }
  if (!loop.AddSignalEvent(SIGCHLD, [&]() { return loop.ExitLoop(); })) {
    return false;
  }

  // 4. Create child process to run run-as, and wait for the child process.
  if (!workload->Start()) {
    return false;
  }
  if (!loop.RunLoop()) {
    return false;
  }
  if (need_to_kill_child) {
    // The child process can exit before we kill it, so don't report kill errors.
    Workload::RunCmd({"run-as", app_package_name, "pkill", "simpleperf"}, false);
  }
  int exit_code;
  if (!workload->WaitChildProcess(&exit_code) || exit_code != 0) {
    return false;
  }

  // 5. If there is any output file, copy it from the app's directory.
  if (!output_filepath.empty()) {
    if (!Workload::RunCmd({"run-as", app_package_name, "cat", output_basename,
                           ">" + output_filepath})) {
      return false;
    }
    if (!Workload::RunCmd({"run-as", app_package_name, "rm", output_basename})) {
      return false;
    }
  }
  return true;
}

static std::string default_package_name;

void SetDefaultAppPackageName(const std::string& package_name) {
  default_package_name = package_name;
}

const std::string& GetDefaultAppPackageName() {
  return default_package_name;
}

void AllowMoreOpenedFiles() {
  // On Android <= O, the hard limit is 4096, and the soft limit is 1024.
  // On Android >= P, both the hard and soft limit are 32768.
  rlimit limit;
  if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
    limit.rlim_cur = limit.rlim_max;
    setrlimit(RLIMIT_NOFILE, &limit);
  }
}

std::string ScopedTempFiles::tmp_dir_;
std::vector<std::string> ScopedTempFiles::files_to_delete_;

ScopedTempFiles::ScopedTempFiles(const std::string& tmp_dir) {
  CHECK(tmp_dir_.empty());  // No other ScopedTempFiles.
  tmp_dir_ = tmp_dir;
}

ScopedTempFiles::~ScopedTempFiles() {
  tmp_dir_.clear();
  for (auto& file : files_to_delete_) {
    unlink(file.c_str());
  }
  files_to_delete_.clear();
}

std::unique_ptr<TemporaryFile> ScopedTempFiles::CreateTempFile(bool delete_in_destructor) {
  CHECK(!tmp_dir_.empty());
  std::unique_ptr<TemporaryFile> tmp_file(new TemporaryFile(tmp_dir_));
  CHECK_NE(tmp_file->fd, -1);
  if (delete_in_destructor) {
    files_to_delete_.push_back(tmp_file->path);
  }
  return tmp_file;
}

bool SignalIsIgnored(int signo) {
  struct sigaction act;
  if (sigaction(signo, nullptr, &act) != 0) {
    PLOG(FATAL) << "failed to query signal handler for signal " << signo;
  }

  if ((act.sa_flags & SA_SIGINFO)) {
    return false;
  }

  return act.sa_handler == SIG_IGN;
}
