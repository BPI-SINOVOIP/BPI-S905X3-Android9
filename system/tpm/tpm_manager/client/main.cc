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

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/memory/ptr_util.h>
#include <base/message_loop/message_loop.h>
#include <brillo/bind_lambda.h>
#if defined(USE_BINDER_IPC)
#include <brillo/binder_watcher.h>
#endif
#include <brillo/daemons/daemon.h>
#include <brillo/syslog_logging.h>
#include <crypto/sha2.h>

#if defined(USE_BINDER_IPC)
#include "tpm_manager/client/tpm_nvram_binder_proxy.h"
#include "tpm_manager/client/tpm_ownership_binder_proxy.h"
#else
#include "tpm_manager/client/tpm_nvram_dbus_proxy.h"
#include "tpm_manager/client/tpm_ownership_dbus_proxy.h"
#endif
#include "tpm_manager/common/print_tpm_manager_proto.h"
#include "tpm_manager/common/tpm_manager.pb.h"
#include "trunks/tpm_generated.h"

namespace tpm_manager {

constexpr char kGetTpmStatusCommand[] = "status";
constexpr char kTakeOwnershipCommand[] = "take_ownership";
constexpr char kRemoveOwnerDependencyCommand[] = "remove_dependency";
constexpr char kDefineSpaceCommand[] = "define_space";
constexpr char kDestroySpaceCommand[] = "destroy_space";
constexpr char kWriteSpaceCommand[] = "write_space";
constexpr char kReadSpaceCommand[] = "read_space";
constexpr char kLockSpaceCommand[] = "lock_space";
constexpr char kListSpacesCommand[] = "list_spaces";
constexpr char kGetSpaceInfoCommand[] = "get_space_info";

constexpr char kDependencySwitch[] = "dependency";
constexpr char kIndexSwitch[] = "index";
constexpr char kSizeSwitch[] = "size";
constexpr char kAttributesSwitch[] = "attributes";
constexpr char kPasswordSwitch[] = "password";
constexpr char kBindToPCR0Switch[] = "bind_to_pcr0";
constexpr char kFileSwitch[] = "file";
constexpr char kUseOwnerSwitch[] = "use_owner_authorization";
constexpr char kLockRead[] = "lock_read";
constexpr char kLockWrite[] = "lock_write";

constexpr char kUsage[] = R"(
Usage: tpm_manager_client <command> [<arguments>]
Commands:
  status
      Prints TPM status information.
  take_ownership
      Takes ownership of the Tpm with a random password.
  remove_dependency --dependency=<owner_dependency>
      Removes the named Tpm owner dependency. E.g. \"Nvram\" or \"Attestation\".
  define_space --index=<index> --size=<size> [--attributes=<attribute_list>]
               [--password=<password>] [--bind_to_pcr0]
      Defines an NV space. The attribute format is a '|' separated list of:
          PERSISTENT_WRITE_LOCK: Allow write lock; stay locked until destroyed.
          BOOT_WRITE_LOCK: Allow write lock; stay locked until next boot.
          BOOT_READ_LOCK: Allow read lock; stay locked until next boot.
          WRITE_AUTHORIZATION: Require authorization to write.
          READ_AUTHORIZATION: Require authorization to read.
          WRITE_EXTEND: Allow only extend operations, not direct writes.
          GLOBAL_LOCK: Engage write lock when the global lock is engaged.
          PLATFORM_WRITE: Allow write only with 'platform' authorization. This
                          is similar to the TPM 1.2 'physical presence' notion.
          OWNER_WRITE: Allow write only with TPM owner authorization.
          OWNER_READ: Allow read only with TPM owner authorization.
      This command requires that owner authorization is available. If a password
      is given it will be required only as specified by the attributes. E.g. if
      READ_AUTHORIZATION is not listed, then the password will not be required
      in order to read. Similarly, if the --bind_to_pcr0 option is given, the
      current PCR0 value will be required only as specified by the attributes.
  destroy_space --index=<index>
      Destroys an NV space. This command requires that owner authorization is
      available.
  write_space --index=<index> --file=<input_file> [--password=<password>]
              [--use_owner_authorization]
      Writes data from a file to an NV space. Any existing data will be
      overwritten.
  read_space --index=<index> --file=<output_file> [--password=<password>]
             [--use_owner_authorization]
      Reads the entire contents of an NV space to a file.
  lock_space --index=<index> [--lock_read] [--lock_write]
             [--password=<password>] [--use_owner_authorization]
      Locks an NV space for read and / or write.
  list_spaces
      Prints a list of all defined index values.
  get_space_info --index=<index>
      Prints public information about an NV space.
)";

constexpr char kKnownNVRAMSpaces[] = R"(
NVRAM Index Reference:
 TPM 1.2 (32-bit values)
  0x00001007 - Chrome OS Firmware Version Rollback Protection
  0x00001008 - Chrome OS Kernel Version Rollback Protection
  0x00001009 - Chrome OS Firmware Backup
  0x0000100A - Chrome OS Firmware Management Parameters
  0x20000004 - Chrome OS Install Attributes (aka LockBox)
  0x10000001 - Standard TPM_NV_INDEX_DIR (Permanent)
  0x1000F000 - Endorsement Certificate (Permanent)
  0x30000001 - Endorsement Authority Certificate (Permanent)
  0x0000F004 - Standard Test Index (for testing TPM_NV_DefineSpace)

 TPM 2.0 (24-bit values)
  0x400000 and following - Reserved for Firmware
  0x800000 and following - Reserved for Software
  0xC00000 and following - Endorsement Certificates
)";

bool ReadFileToString(const std::string& filename, std::string* data) {
  return base::ReadFileToString(base::FilePath(filename), data);
}

bool WriteStringToFile(const std::string& data, const std::string& filename) {
  int result =
      base::WriteFile(base::FilePath(filename), data.data(), data.size());
  return (result != -1 && static_cast<size_t>(result) == data.size());
}

uint32_t StringToUint32(const std::string& s) {
  return strtoul(s.c_str(), nullptr, 0);
}

uint32_t StringToNvramIndex(const std::string& s) {
  return trunks::HR_HANDLE_MASK & StringToUint32(s);
}

using ClientLoopBase = brillo::Daemon;
class ClientLoop : public ClientLoopBase {
 public:
  ClientLoop() = default;
  ~ClientLoop() override = default;

 protected:
  int OnInit() override {
    int exit_code = ClientLoopBase::OnInit();
    if (exit_code != EX_OK) {
      LOG(ERROR) << "Error initializing tpm_manager_client.";
      return exit_code;
    }
#if defined(USE_BINDER_IPC)
    if (!binder_watcher_.Init()) {
      LOG(ERROR) << "Error initializing binder watcher.";
      return EX_UNAVAILABLE;
    }
    std::unique_ptr<TpmNvramBinderProxy> nvram_proxy =
        base::MakeUnique<TpmNvramBinderProxy>();
    std::unique_ptr<TpmOwnershipBinderProxy> ownership_proxy =
        base::MakeUnique<TpmOwnershipBinderProxy>();
#else
    std::unique_ptr<TpmNvramDBusProxy> nvram_proxy =
        base::MakeUnique<TpmNvramDBusProxy>();
    std::unique_ptr<TpmOwnershipDBusProxy> ownership_proxy =
        base::MakeUnique<TpmOwnershipDBusProxy>();
#endif
    if (!nvram_proxy->Initialize()) {
      LOG(ERROR) << "Error initializing nvram proxy.";
      return EX_UNAVAILABLE;
    }
    if (!ownership_proxy->Initialize()) {
      LOG(ERROR) << "Error initializing ownership proxy.";
      return EX_UNAVAILABLE;
    }
    tpm_nvram_ = std::move(nvram_proxy);
    tpm_ownership_ = std::move(ownership_proxy);
    exit_code = ScheduleCommand();
    if (exit_code == EX_USAGE) {
      printf("%s%s", kUsage, kKnownNVRAMSpaces);
    }
    return exit_code;
  }

  void OnShutdown(int* exit_code) override {
    tpm_nvram_.reset();
    tpm_ownership_.reset();
    ClientLoopBase::OnShutdown(exit_code);
  }

 private:
  // Posts tasks on to the message loop based on command line flags.
  int ScheduleCommand() {
    base::Closure task;
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch("help") || command_line->HasSwitch("h") ||
        command_line->GetArgs().size() == 0) {
      return EX_USAGE;
    }
    std::string command = command_line->GetArgs()[0];
    if (command == kGetTpmStatusCommand) {
      task = base::Bind(&ClientLoop::HandleGetTpmStatus,
                        weak_factory_.GetWeakPtr());
    } else if (command == kTakeOwnershipCommand) {
      task = base::Bind(&ClientLoop::HandleTakeOwnership,
                        weak_factory_.GetWeakPtr());
    } else if (command == kRemoveOwnerDependencyCommand) {
      if (!command_line->HasSwitch(kDependencySwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(&ClientLoop::HandleRemoveOwnerDependency,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII(kDependencySwitch));
    } else if (command == kDefineSpaceCommand) {
      if (!command_line->HasSwitch(kIndexSwitch) ||
          !command_line->HasSwitch(kSizeSwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleDefineSpace, weak_factory_.GetWeakPtr(),
          StringToNvramIndex(command_line->GetSwitchValueASCII(kIndexSwitch)),
          StringToUint32(command_line->GetSwitchValueASCII(kSizeSwitch)),
          command_line->GetSwitchValueASCII(kAttributesSwitch),
          command_line->GetSwitchValueASCII(kPasswordSwitch),
          command_line->HasSwitch(kBindToPCR0Switch));
    } else if (command == kDestroySpaceCommand) {
      if (!command_line->HasSwitch(kIndexSwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleDestroySpace, weak_factory_.GetWeakPtr(),
          StringToNvramIndex(command_line->GetSwitchValueASCII(kIndexSwitch)));
    } else if (command == kWriteSpaceCommand) {
      if (!command_line->HasSwitch(kIndexSwitch) ||
          !command_line->HasSwitch(kFileSwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleWriteSpace, weak_factory_.GetWeakPtr(),
          StringToNvramIndex(command_line->GetSwitchValueASCII(kIndexSwitch)),
          command_line->GetSwitchValueASCII(kFileSwitch),
          command_line->GetSwitchValueASCII(kPasswordSwitch),
          command_line->HasSwitch(kUseOwnerSwitch));
    } else if (command == kReadSpaceCommand) {
      if (!command_line->HasSwitch(kIndexSwitch) ||
          !command_line->HasSwitch(kFileSwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleReadSpace, weak_factory_.GetWeakPtr(),
          StringToNvramIndex(command_line->GetSwitchValueASCII(kIndexSwitch)),
          command_line->GetSwitchValueASCII(kFileSwitch),
          command_line->GetSwitchValueASCII(kPasswordSwitch),
          command_line->HasSwitch(kUseOwnerSwitch));
    } else if (command == kLockSpaceCommand) {
      if (!command_line->HasSwitch(kIndexSwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleLockSpace, weak_factory_.GetWeakPtr(),
          StringToNvramIndex(command_line->GetSwitchValueASCII(kIndexSwitch)),
          command_line->HasSwitch(kLockRead),
          command_line->HasSwitch(kLockWrite),
          command_line->GetSwitchValueASCII(kPasswordSwitch),
          command_line->HasSwitch(kUseOwnerSwitch));
    } else if (command == kListSpacesCommand) {
      task =
          base::Bind(&ClientLoop::HandleListSpaces, weak_factory_.GetWeakPtr());
    } else if (command == kGetSpaceInfoCommand) {
      if (!command_line->HasSwitch(kIndexSwitch)) {
        return EX_USAGE;
      }
      task = base::Bind(
          &ClientLoop::HandleGetSpaceInfo, weak_factory_.GetWeakPtr(),
          StringToNvramIndex(command_line->GetSwitchValueASCII(kIndexSwitch)));
    } else {
      // Command line arguments did not match any valid commands.
      return EX_USAGE;
    }
    base::MessageLoop::current()->task_runner()->PostTask(FROM_HERE, task);
    return EX_OK;
  }

  // Template to print reply protobuf.
  template <typename ProtobufType>
  void PrintReplyAndQuit(const ProtobufType& reply) {
    LOG(INFO) << "Message Reply: " << GetProtoDebugString(reply);
    Quit();
  }

  void HandleGetTpmStatus() {
    GetTpmStatusRequest request;
    tpm_ownership_->GetTpmStatus(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<GetTpmStatusReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleTakeOwnership() {
    TakeOwnershipRequest request;
    tpm_ownership_->TakeOwnership(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<TakeOwnershipReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleRemoveOwnerDependency(const std::string& owner_dependency) {
    RemoveOwnerDependencyRequest request;
    request.set_owner_dependency(owner_dependency);
    tpm_ownership_->RemoveOwnerDependency(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<RemoveOwnerDependencyReply>,
                   weak_factory_.GetWeakPtr()));
  }

  bool DecodeAttribute(const std::string& attribute_str,
                       NvramSpaceAttribute* attribute) {
    if (attribute_str == "PERSISTENT_WRITE_LOCK") {
      *attribute = NVRAM_PERSISTENT_WRITE_LOCK;
      return true;
    }
    if (attribute_str == "BOOT_WRITE_LOCK") {
      *attribute = NVRAM_BOOT_WRITE_LOCK;
      return true;
    }
    if (attribute_str == "BOOT_READ_LOCK") {
      *attribute = NVRAM_BOOT_READ_LOCK;
      return true;
    }
    if (attribute_str == "WRITE_AUTHORIZATION") {
      *attribute = NVRAM_WRITE_AUTHORIZATION;
      return true;
    }
    if (attribute_str == "READ_AUTHORIZATION") {
      *attribute = NVRAM_READ_AUTHORIZATION;
      return true;
    }
    if (attribute_str == "WRITE_EXTEND") {
      *attribute = NVRAM_WRITE_EXTEND;
      return true;
    }
    if (attribute_str == "GLOBAL_LOCK") {
      *attribute = NVRAM_GLOBAL_LOCK;
      return true;
    }
    if (attribute_str == "PLATFORM_WRITE") {
      *attribute = NVRAM_PLATFORM_WRITE;
      return true;
    }
    if (attribute_str == "OWNER_WRITE") {
      *attribute = NVRAM_OWNER_WRITE;
      return true;
    }
    if (attribute_str == "OWNER_READ") {
      *attribute = NVRAM_OWNER_READ;
      return true;
    }
    LOG(ERROR) << "Unrecognized attribute: " << attribute_str;
    return false;
  }

  void HandleDefineSpace(uint32_t index,
                         size_t size,
                         const std::string& attributes,
                         const std::string& password,
                         bool bind_to_pcr0) {
    DefineSpaceRequest request;
    request.set_index(index);
    request.set_size(size);
    std::string::size_type pos = 0;
    std::string::size_type next_pos = 0;
    while (next_pos != std::string::npos) {
      next_pos = attributes.find('|', pos);
      std::string attribute_str;
      if (next_pos == std::string::npos) {
        attribute_str = attributes.substr(pos);
      } else {
        attribute_str = attributes.substr(pos, next_pos - pos);
      }
      if (!attribute_str.empty()) {
        NvramSpaceAttribute attribute;
        if (!DecodeAttribute(attribute_str, &attribute)) {
          Quit();
          return;
        }
        request.add_attributes(attribute);
      }
      pos = next_pos + 1;
    }
    request.set_authorization_value(crypto::SHA256HashString(password));
    request.set_policy(bind_to_pcr0 ? NVRAM_POLICY_PCR0 : NVRAM_POLICY_NONE);
    tpm_nvram_->DefineSpace(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<DefineSpaceReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleDestroySpace(uint32_t index) {
    DestroySpaceRequest request;
    request.set_index(index);
    tpm_nvram_->DestroySpace(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<DestroySpaceReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleWriteSpace(uint32_t index,
                        const std::string& input_file,
                        const std::string& password,
                        bool use_owner_authorization) {
    WriteSpaceRequest request;
    request.set_index(index);
    std::string data;
    if (!ReadFileToString(input_file, &data)) {
      LOG(ERROR) << "Failed to read input file.";
      Quit();
      return;
    }
    request.set_data(data);
    request.set_authorization_value(crypto::SHA256HashString(password));
    request.set_use_owner_authorization(use_owner_authorization);
    tpm_nvram_->WriteSpace(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<WriteSpaceReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleReadSpaceReply(const std::string& output_file,
                            const ReadSpaceReply& reply) {
    if (!WriteStringToFile(reply.data(), output_file)) {
      LOG(ERROR) << "Failed to write output file.";
    }
    LOG(INFO) << "Message Reply: " << GetProtoDebugString(reply);
    Quit();
  }

  void HandleReadSpace(uint32_t index,
                       const std::string& output_file,
                       const std::string& password,
                       bool use_owner_authorization) {
    ReadSpaceRequest request;
    request.set_index(index);
    request.set_authorization_value(crypto::SHA256HashString(password));
    request.set_use_owner_authorization(use_owner_authorization);
    tpm_nvram_->ReadSpace(request,
                          base::Bind(&ClientLoop::HandleReadSpaceReply,
                                     weak_factory_.GetWeakPtr(), output_file));
  }

  void HandleLockSpace(uint32_t index,
                       bool lock_read,
                       bool lock_write,
                       const std::string& password,
                       bool use_owner_authorization) {
    LockSpaceRequest request;
    request.set_index(index);
    request.set_lock_read(lock_read);
    request.set_lock_write(lock_write);
    request.set_authorization_value(crypto::SHA256HashString(password));
    request.set_use_owner_authorization(use_owner_authorization);
    tpm_nvram_->LockSpace(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<LockSpaceReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleListSpaces() {
    printf("%s\n", kKnownNVRAMSpaces);
    ListSpacesRequest request;
    tpm_nvram_->ListSpaces(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<ListSpacesReply>,
                            weak_factory_.GetWeakPtr()));
  }

  void HandleGetSpaceInfo(uint32_t index) {
    GetSpaceInfoRequest request;
    request.set_index(index);
    tpm_nvram_->GetSpaceInfo(
        request, base::Bind(&ClientLoop::PrintReplyAndQuit<GetSpaceInfoReply>,
                            weak_factory_.GetWeakPtr()));
  }

  // IPC proxy interfaces.
  std::unique_ptr<tpm_manager::TpmNvramInterface> tpm_nvram_;
  std::unique_ptr<tpm_manager::TpmOwnershipInterface> tpm_ownership_;

#if defined(USE_BINDER_IPC)
  brillo::BinderWatcher binder_watcher_;
#endif

  // Declared last so that weak pointers will be destroyed first.
  base::WeakPtrFactory<ClientLoop> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ClientLoop);
};

}  // namespace tpm_manager

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  brillo::InitLog(brillo::kLogToStderr);
  tpm_manager::ClientLoop loop;
  return loop.Run();
}
