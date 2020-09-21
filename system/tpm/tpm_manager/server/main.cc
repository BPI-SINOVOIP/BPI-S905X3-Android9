//
// Copyright (C) 2015 The Android Open Source Project
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

#include <string>

#include <base/command_line.h>
#include <brillo/syslog_logging.h>
#if defined(USE_TPM2)
#include <trunks/trunks_factory_impl.h>
#endif

#include "tpm_manager/common/tpm_manager_constants.h"
#if defined(USE_BINDER_IPC)
#include "tpm_manager/server/binder_service.h"
#else
#include "tpm_manager/server/dbus_service.h"
#endif
#include "tpm_manager/server/local_data_store_impl.h"
#include "tpm_manager/server/tpm_manager_service.h"

#if defined(USE_TPM2)
#include "tpm_manager/server/tpm2_initializer_impl.h"
#include "tpm_manager/server/tpm2_nvram_impl.h"
#include "tpm_manager/server/tpm2_status_impl.h"
#else
#include "tpm_manager/server/tpm_initializer_impl.h"
#include "tpm_manager/server/tpm_nvram_impl.h"
#include "tpm_manager/server/tpm_status_impl.h"
#endif

namespace {

constexpr char kWaitForOwnershipTriggerSwitch[] = "wait_for_ownership_trigger";
constexpr char kLogToStderrSwitch[] = "log_to_stderr";

}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  int flags = brillo::kLogToSyslog;
  if (cl->HasSwitch(kLogToStderrSwitch)) {
    flags |= brillo::kLogToStderr;
  }
  brillo::InitLog(flags);

  tpm_manager::LocalDataStoreImpl local_data_store;
#if defined(USE_TPM2)
  trunks::TrunksFactoryImpl trunks_factory;
  // Tolerate some delay in trunksd being up and ready.
  constexpr int kTrunksDaemonTimeoutMS = 30000;  // 30 seconds
  int ms_waited = 0;
  while (!trunks_factory.Initialize() && ms_waited < kTrunksDaemonTimeoutMS) {
    usleep(300000);
    ms_waited += 300;
  }
  tpm_manager::Tpm2StatusImpl tpm_status(trunks_factory);
  tpm_manager::Tpm2InitializerImpl tpm_initializer(
      trunks_factory, &local_data_store, &tpm_status);
  tpm_manager::Tpm2NvramImpl tpm_nvram(trunks_factory, &local_data_store);
#else
  tpm_manager::TpmStatusImpl tpm_status;
  tpm_manager::TpmInitializerImpl tpm_initializer(&local_data_store,
                                                  &tpm_status);
  tpm_manager::TpmNvramImpl tpm_nvram(&local_data_store);
#endif
  tpm_manager::TpmManagerService tpm_manager_service(
      cl->HasSwitch(kWaitForOwnershipTriggerSwitch), &local_data_store,
      &tpm_status, &tpm_initializer, &tpm_nvram);

#if defined(USE_BINDER_IPC)
  tpm_manager::BinderService ipc_service(&tpm_manager_service,
                                         &tpm_manager_service);
#else
  tpm_manager::DBusService ipc_service(&tpm_manager_service,
                                       &tpm_manager_service);
#endif
  CHECK(tpm_manager_service.Initialize()) << "Failed to initialize service.";
  LOG(INFO) << "Starting TPM Manager...";
  return ipc_service.Run();
}
