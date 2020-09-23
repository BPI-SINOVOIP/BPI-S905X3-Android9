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

#include "Disk.h"
#include "PublicVolume.h"
#include "Utils.h"
#include "VolumeBase.h"
#include "VolumeManager.h"
#include "ResponseCode.h"

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include <vector>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/sysmacros.h>

using android::base::ReadFileToString;
using android::base::WriteStringToFile;
using android::base::StringPrintf;

namespace android {
namespace droidvold {

static const char* kSgdiskPath = "/system/bin/sgdisk";
static const char* kSgdiskToken = " \t\n";

static const char* kSysfsMmcMaxMinors = "/sys/module/mmcblk/parameters/perdev_minors";

static const unsigned int kMajorBlockScsiA = 8;
static const unsigned int kMajorBlockSr = 11;
static const unsigned int kMajorBlockScsiB = 65;
static const unsigned int kMajorBlockScsiC = 66;
static const unsigned int kMajorBlockScsiD = 67;
static const unsigned int kMajorBlockScsiE = 68;
static const unsigned int kMajorBlockScsiF = 69;
static const unsigned int kMajorBlockScsiG = 70;
static const unsigned int kMajorBlockScsiH = 71;
static const unsigned int kMajorBlockScsiI = 128;
static const unsigned int kMajorBlockScsiJ = 129;
static const unsigned int kMajorBlockScsiK = 130;
static const unsigned int kMajorBlockScsiL = 131;
static const unsigned int kMajorBlockScsiM = 132;
static const unsigned int kMajorBlockScsiN = 133;
static const unsigned int kMajorBlockScsiO = 134;
static const unsigned int kMajorBlockScsiP = 135;
static const unsigned int kMajorBlockMmc = 179;
static const unsigned int kMajorBlockExperimentalMin = 240;
static const unsigned int kMajorBlockExperimentalMax = 254;

static const char* kGptBasicData = "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7";
static const char* kGptAndroidMeta = "19A710A2-B3CA-11E4-B026-10604B889DCF";
static const char* kGptAndroidExpand = "193D1EA4-B3CA-11E4-B075-10604B889DCF";

enum class Table {
    kUnknown,
    kMbr,
    kGpt,
};

static bool isVirtioBlkDevice(unsigned int major) {
    /*
     * The new emulator's "ranchu" virtual board no longer includes a goldfish
     * MMC-based SD card device; instead, it emulates SD cards with virtio-blk,
     * which has been supported by upstream kernel and QEMU for quite a while.
     * Unfortunately, the virtio-blk block device driver does not use a fixed
     * major number, but relies on the kernel to assign one from a specific
     * range of block majors, which are allocated for "LOCAL/EXPERIMENAL USE"
     * per Documentation/devices.txt. This is true even for the latest Linux
     * kernel (4.4; see init() in drivers/block/virtio_blk.c).
     *
     * This makes it difficult for vold to detect a virtio-blk based SD card.
     * The current solution checks two conditions (both must be met):
     *
     *  a) If the running environment is the emulator;
     *  b) If the major number is an experimental block device major number (for
     *     x86/x86_64 3.10 ranchu kernels, virtio-blk always gets major number
     *     253, but it is safer to match the range than just one value).
     *
     * Other conditions could be used, too, e.g. the hardware name should be
     * "ranchu", the device's sysfs path should end with "/block/vd[d-z]", etc.
     * But just having a) and b) is enough for now.
     */
    return IsRunningInEmulator() && major >= kMajorBlockExperimentalMin
            && major <= kMajorBlockExperimentalMax;
}

Disk::Disk(const std::string& eventPath, dev_t device,
        const std::string& nickname, const std::string& eventName, int flags):
        mDevice(device), mSize(-1), mNickname(nickname), mFlags(flags), mCreated(
                false), mJustPartitioned(false) {
    mId = StringPrintf("disk:%u,%u", major(device), minor(device));
    mEventPath = eventPath;
    mDevName = eventName;
    mSysPath = StringPrintf("/sys/%s", eventPath.c_str());
    mDevPath = StringPrintf("/dev/block/%s", mDevName.c_str());

    mSrdisk = (!strncmp(nickname.c_str(), "sr", 2)) ? true : false;
}

Disk::~Disk() {
    CHECK(!mCreated);
}

std::shared_ptr<VolumeBase> Disk::findVolume(const std::string& id) {
    for (auto vol : mVolumes) {
        if (vol->getId() == id) {
            return vol;
        }
        auto stackedVol = vol->findVolume(id);
        if (stackedVol != nullptr) {
            return stackedVol;
        }
    }
    return nullptr;
}

std::shared_ptr<VolumeBase> Disk::findVolumeByPath(const std::string& path) {
    for (auto vol : mVolumes) {
        if (vol->getPath() == path) {
            return vol;
        }
    }
    return nullptr;
}

void Disk::listVolumes(VolumeBase::Type type, std::list<std::string>& list) {
    for (auto vol : mVolumes) {
        if (vol->getType() == type) {
            list.push_back(vol->getId());
        }
        // TODO: consider looking at stacked volumes
    }
}

status_t Disk::create() {
    CHECK(!mCreated);
    mCreated = true;
    notifyEvent(ResponseCode::DiskCreated, StringPrintf("%d", mFlags));

    // do nothing when srdisk is created
    if (mSrdisk)
        return OK;

    readDiskMetadata();
    // sleep 10ms
    usleep(10000);

    std::string fsType;
    std::string unused;

    // if no partition, handle here
    if (ReadPartMetadata(mDevPath, fsType, unused, unused) == OK) {
        if (!fsType.empty()) {
            if (VolumeManager::Instance()->getDebug())
                LOG(DEBUG) << "treat entire disk as partition, devPath=" << mDevPath;
            createPublicVolume(mDevName, true, 0);
        }
    }

    return OK;
}

status_t Disk::reset() {
    CHECK(!mCreated);
    mCreated = true;
    notifyEvent(ResponseCode::DiskCreated, StringPrintf("%d", mFlags));

    // do nothing when srdisk is created
    if (mSrdisk)
        return OK;

    readDiskMetadata();

    if (mPartNo.size() == 0) {
        createPublicVolume(mDevName, true, 0);
    } else {
        std::string partDevName;
        for (auto part : mPartNo) {
            if (mFlags & Flags::kUsb)
                partDevName = StringPrintf("%s%d", mDevName.c_str(), part);
            else if (mFlags & Flags::kSd)
                partDevName = StringPrintf("%sp%d", mDevName.c_str(), part);

            createPublicVolume(partDevName, false, part);
        }
    }


    return OK;
}

status_t Disk::destroy() {
    CHECK(mCreated);
    destroyAllVolumes();
    notifyEvent(ResponseCode::DiskDestroyed);
    mCreated = false;
    return OK;
}

void Disk::handleJustPublicPhysicalDevice(
    const std::string& physicalDevName) {
    auto vol = std::shared_ptr<VolumeBase>(new PublicVolume(physicalDevName, true));
    if (mJustPartitioned) {
        LOG(DEBUG) << "Device just partitioned; silently formatting";
        vol->setSilent(true);
        vol->create();
        vol->format("auto");
        vol->destroy();
        vol->setSilent(false);
    }

    mVolumes.push_back(vol);
    vol->setDiskId(getId());
    vol->setSysPath(getSysPath());
    vol->create();
    //vol->mount();
}

void Disk::createPublicVolume(const std::string& partDevName,
        const bool isPhysical, int part) {
    auto vol = std::shared_ptr<VolumeBase>(new PublicVolume(partDevName, isPhysical));
    if (mJustPartitioned) {
        LOG(DEBUG) << "Device just partitioned; silently formatting";
        vol->setSilent(true);
        vol->create();
        vol->format("auto");
        vol->destroy();
        vol->setSilent(false);
    }

    mVolumes.push_back(vol);
    vol->setDiskId(getId());
    vol->setSysPath(getSysPath());
    vol->setDiskFlags(mFlags);
    vol->setPartNo(part);

    vol->create();
    //vol->mount();
}

void Disk::destroyAllVolumes() {
    for (auto vol : mVolumes) {
        vol->destroy();
    }
    mVolumes.clear();
}

status_t Disk::readDiskMetadata() {
    mSize = -1;
    mLabel.clear();

    int fd = open(mDevPath.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd != -1) {
        if (ioctl(fd, BLKGETSIZE64, &mSize)) {
            mSize = -1;
        }
        close(fd);
    }

    unsigned int majorId = major(mDevice);
    switch (majorId) {
    case kMajorBlockSr:
    case kMajorBlockScsiA: case kMajorBlockScsiB: case kMajorBlockScsiC: case kMajorBlockScsiD:
    case kMajorBlockScsiE: case kMajorBlockScsiF: case kMajorBlockScsiG: case kMajorBlockScsiH:
    case kMajorBlockScsiI: case kMajorBlockScsiJ: case kMajorBlockScsiK: case kMajorBlockScsiL:
    case kMajorBlockScsiM: case kMajorBlockScsiN: case kMajorBlockScsiO: case kMajorBlockScsiP: {
        std::string path(mSysPath + "/device/vendor");
        std::string tmp;
        if (!ReadFileToString(path, &tmp)) {
            PLOG(WARNING) << "Failed to read vendor from " << path;
            return -errno;
        }
        mLabel = tmp;
        break;
    }
    case kMajorBlockMmc: {
        std::string path(mSysPath + "/device/manfid");
        std::string tmp;
        if (!ReadFileToString(path, &tmp)) {
            PLOG(WARNING) << "Failed to read manufacturer from " << path;
            return -errno;
        }
        uint64_t manfid = strtoll(tmp.c_str(), nullptr, 16);
        // Our goal here is to give the user a meaningful label, ideally
        // matching whatever is silk-screened on the card.  To reduce
        // user confusion, this list doesn't contain white-label manfid.
        switch (manfid) {
        case 0x000003: mLabel = "SanDisk"; break;
        case 0x00001b: mLabel = "Samsung"; break;
        case 0x000028: mLabel = "Lexar"; break;
        case 0x000074: mLabel = "Transcend"; break;
        }
        break;
    }
    default: {
        if (isVirtioBlkDevice(majorId)) {
            LOG(DEBUG) << "Recognized experimental block major ID " << majorId
                    << " as virtio-blk (emulator's virtual SD card device)";
            mLabel = "Virtual";
            break;
        }
        LOG(WARNING) << "Unsupported block major type " << majorId;
        return -ENOTSUP;
    }
    }

    notifyEvent(ResponseCode::DiskSizeChanged, StringPrintf("%" PRIu64, mSize));
    notifyEvent(ResponseCode::DiskLabelChanged, mLabel);
    notifyEvent(ResponseCode::DiskSysPathChanged, mSysPath);
    return OK;
}

void Disk::handleBlockEvent(NetlinkEvent *evt) {
    std::string eventPath(evt->findParam("DEVPATH")?evt->findParam("DEVPATH"):"");
    std::string devName(evt->findParam("DEVNAME")?evt->findParam("DEVNAME"):"");
    std::string devType(evt->findParam("DEVTYPE")?evt->findParam("DEVTYPE"):"");

    // can we handle this event
    if (eventPath.find(mEventPath) == std::string::npos) {
        LOG(DEBUG) << "evt will handle by other disk " << mEventPath;
        return;
    }

    if (devType != "partition") {
        LOG(DEBUG) << "evt type is not partition " << devType;
        evt->dump();
        return;
    }

    std::string partDevName;
    switch (evt->getAction()) {
    case NetlinkEvent::Action::kAdd: {
        int part = atoi(evt->findParam("PARTN"));

        mPartNo.push_back(part);
        if (mFlags & Flags::kUsb)
            partDevName = StringPrintf("%s%d", mDevName.c_str(), part);
        else if (mFlags & Flags::kSd)
            partDevName = StringPrintf("%sp%d", mDevName.c_str(), part);

        LOG(INFO) << " partDevName =" << partDevName;
        createPublicVolume(partDevName, false, part);
        break;
    }
    case NetlinkEvent::Action::kChange: {
        // ignore
        LOG(DEBUG) << "Disk at " << mDevPath << " changed";
        break;
    }
    case NetlinkEvent::Action::kRemove: {
        // will handle by vm
        break;
    }
    default: {
        LOG(WARNING) << "Unexpected block event action " << (int) evt->getAction();
        break;
    }
    }
}

status_t Disk::unmountAll() {
    for (auto vol : mVolumes) {
        vol->unmount();
    }
    return OK;
}

void Disk::notifyEvent(int event) {
    VolumeManager::Instance()->getBroadcaster()->sendBroadcast(event,
            getId());
}

void Disk::notifyEvent(int event, const std::string& value) {
    VolumeManager::Instance()->getBroadcaster()->sendBroadcast(event,
            StringPrintf("%s %s", getId().c_str(), value.c_str()));
}


bool Disk::isSrdiskMounted() {
    if (!mSrdisk) {
        return false;
    }

    for (auto vol : mVolumes) {
        return vol->isSrdiskMounted();
    }

    return false;
}

int Disk::getMaxMinors() {
    // Figure out maximum partition devices supported
    unsigned int majorId = major(mDevice);
    switch (majorId) {
    case kMajorBlockScsiA: case kMajorBlockScsiB: case kMajorBlockScsiC: case kMajorBlockScsiD:
    case kMajorBlockScsiE: case kMajorBlockScsiF: case kMajorBlockScsiG: case kMajorBlockScsiH:
    case kMajorBlockScsiI: case kMajorBlockScsiJ: case kMajorBlockScsiK: case kMajorBlockScsiL:
    case kMajorBlockScsiM: case kMajorBlockScsiN: case kMajorBlockScsiO: case kMajorBlockScsiP: {
        // Per Documentation/devices.txt this is static
        return 31;
    }
    case kMajorBlockMmc: {
        // Per Documentation/devices.txt this is dynamic
        std::string tmp;
        if (!ReadFileToString(kSysfsMmcMaxMinors, &tmp)) {
            LOG(ERROR) << "Failed to read max minors";
            return -errno;
        }
        return atoi(tmp.c_str());
    }
    default: {
        if (isVirtioBlkDevice(majorId)) {
            // drivers/block/virtio_blk.c has "#define PART_BITS 4", so max is
            // 2^4 - 1 = 15
            return 15;
        }
    }
    }

    LOG(ERROR) << "Unsupported block major type " << majorId;
    return -ENOTSUP;
}

}  // namespace vold
}  // namespace android
