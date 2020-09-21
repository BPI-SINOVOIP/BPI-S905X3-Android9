/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "camera_characteristics.h"

#include <ctype.h>

#include <ios>
#include <sstream>

#include <base/files/file_util.h>
#include <base/logging.h>

// /etc/camera/camera_characteristics.conf contains camera information which
// driver cannot provide.
static const char kCameraCharacteristicsConfigFile[] =
    "/etc/camera/camera_characteristics.conf";

/* Common parameters */
static const char kLensFacing[] = "lens_facing";
static const char kSensorOrientation[] = "sensor_orientation";
static const char kUsbVidPid[] = "usb_vid_pid";
static const char kLensInfoAvailableFocalLengths[] =
    "lens_info_available_focal_lengths";
static const char kLensInfoMinimumFocusDistance[] =
    "lens_info_minimum_focus_distance";
static const char kLensInfoOptimalFocusDistance[] =
    "lens_info_optimal_focus_distance";

/* HAL v1 parameters */
static const char kHorizontalViewAngle_16_9[] = "horizontal_view_angle_16_9";
static const char kHorizontalViewAngle_4_3[] = "horizontal_view_angle_4_3";
static const char kVerticalViewAngle_16_9[] = "vertical_view_angle_16_9";
static const char kVerticalViewAngle_4_3[] = "vertical_view_angle_4_3";

/* HAL v3 parameters */
static const char kLensInfoAvailableApertures[] =
    "lens_info_available_apertures";
static const char kSensorInfoPhysicalSize[] = "sensor_info_physical_size";
static const char kSensorInfoPixelArraySize[] = "sensor_info_pixel_array_size";

/* Special parameters */
static const char kFramesToSkipAfterStreamon[] =
    "frames_to_skip_after_streamon";
static const char kResolution1280x960Unsupported[] =
    "resolution_1280x960_unsupported";
static const char kResolution1600x1200Unsupported[] =
    "resolution_1600x1200_unsupported";
static const char kConstantFramerateUnsupported[] =
    "constant_framerate_unsupported";

static const struct DeviceInfo kDefaultCharacteristics = {
  "",     // device_path
  "",     // usb_vid
  "",     // usb_pid
  0,      // lens_facing
  0,      // sensor_orientation
  0,      // frames_to_skip_after_streamon
  66.5,   // horizontal_view_angle_16_9
  0.0,    // horizontal_view_angle_4_3
  {1.6},  // lens_info_available_focal_lengths
  0.3,    // lens_info_minimum_focus_distance
  0.5,    // lens_info_optimal_focus_distance
  42.5,   // vertical_view_angle_16_9
  0.0,    // vertical_view_angle_4_3
  false,  // resolution_1280x960_unsupported
  false,  // resolution_1600x1200_unsupported
  false,  // constant_framerate_unsupported
  0,      // sensor_info_pixel_array_size_width
  0       // sensor_info_pixel_array_size_height
};

CameraCharacteristics::CameraCharacteristics() {
}

CameraCharacteristics::~CameraCharacteristics() {
}

const DeviceInfos CameraCharacteristics::GetCharacteristicsFromFile(
    const std::unordered_map<std::string, std::string>& devices) {
  const base::FilePath path(kCameraCharacteristicsConfigFile);
  FILE* file = base::OpenFile(path, "r");
  if (!file) {
    LOG(INFO) << __func__ << ": Can't open file "
              << kCameraCharacteristicsConfigFile;
    return DeviceInfos();
  }

  DeviceInfos tmp_device_infos;
  char buffer[256], key[256], value[256];
  uint32_t camera_id;
  uint32_t module_id = -1;
  std::string vid, pid;
  while (fgets(buffer, sizeof(buffer), file)) {
    // Skip comments and empty lines.
    if (buffer[0] == '#' || buffer[0] == '\n') {
      continue;
    }

    if (sscanf(buffer, "%[^=]=%s", key, value) != 2) {
      LOG(ERROR) << __func__ << ": Illegal format: " << buffer;
      continue;
    }
    std::vector<char*> sub_keys;
    char* sub_key = strtok(key, ".");
    while (sub_key) {
      sub_keys.push_back(sub_key);
      sub_key = strtok(NULL, ".");
    }

    if (sscanf(sub_keys[0], "camera%u", &camera_id) != 1) {
      LOG(ERROR) << __func__ << ": Illegal format: " << sub_keys[0];
      continue;
    }
    if (camera_id > tmp_device_infos.size()) {
      // Camera id should be ascending by one.
      LOG(ERROR) << __func__ << ": Invalid camera id: " << camera_id;
      continue;
    } else if (camera_id == tmp_device_infos.size()) {
      tmp_device_infos.push_back(kDefaultCharacteristics);
    }

    uint32_t tmp_module_id;
    // Convert value to lower case.
    for (char* p = value; *p; ++p) *p = tolower(*p);

    if (sscanf(sub_keys[1], "module%u", &tmp_module_id) != 1) {
      AddPerCameraCharacteristic(
          camera_id, sub_keys[1], value, &tmp_device_infos);
    } else {
      if (tmp_module_id != module_id) {
        vid.clear();
        pid.clear();
        module_id = tmp_module_id;
      }
      if (strcmp(sub_keys[2], kUsbVidPid) == 0) {
        char tmp_vid[256], tmp_pid[256];
        if (sscanf(value, "%[0-9a-z]:%[0-9a-z]", tmp_vid, tmp_pid) != 2) {
          LOG(ERROR) << __func__ << ": Invalid format: " << sub_keys[2];
          continue;
        }
        vid = tmp_vid;
        pid = tmp_pid;
        const auto& device = devices.find(value);
        if (device != devices.end()) {
          tmp_device_infos[camera_id].usb_vid = vid;
          tmp_device_infos[camera_id].usb_pid = pid;
          tmp_device_infos[camera_id].device_path = device->second;
        }

        VLOG(1) << __func__ << ": Camera" << camera_id << " "
                << kUsbVidPid << ": " << value;
      } else if (!vid.empty() && !pid.empty()) {
        // Some characteristics are module-specific, so only matched ones are
        // selected.
        if (tmp_device_infos[camera_id].usb_vid != vid ||
            tmp_device_infos[camera_id].usb_pid != pid) {
          VLOG(1) << __func__ << ": Mismatched module: "
                  << "vid: " << vid << " pid: " << pid;
          continue;
        }
        AddPerModuleCharacteristic(
            camera_id, sub_keys[2], value, &tmp_device_infos);
      } else {
        // Characteristic usb_vid_pid should come before other module-specific
        // characteristics.
        LOG(ERROR) << __func__ << ": Illegal format."
                   << " usb_vid_pid should come before: " << buffer;
      }
    }
  }
  base::CloseFile(file);

  DeviceInfos device_infos;
  // Some devices use the same camera_characteristics.conf and have different
  // number of cameras.
  for (size_t id = 0; id < tmp_device_infos.size(); ++id) {
    if (tmp_device_infos[id].device_path.empty()) {
      LOG(INFO) << __func__ << ": No matching module for camera" << id;
    } else {
      device_infos.push_back(tmp_device_infos[id]);
    }
  }

  // Check sensor array size to decide supported resolutions.
  for (size_t id = 0; id < device_infos.size(); ++id) {
    if (device_infos[id].sensor_info_pixel_array_size_width < 1280 ||
        device_infos[id].sensor_info_pixel_array_size_height < 960) {
      device_infos[id].resolution_1280x960_unsupported = true;
    }
    if (device_infos[id].sensor_info_pixel_array_size_width < 1600 ||
        device_infos[id].sensor_info_pixel_array_size_height < 1200) {
      device_infos[id].resolution_1600x1200_unsupported = true;
    }
  }
  return device_infos;
}

void CameraCharacteristics::AddPerCameraCharacteristic(
    uint32_t camera_id, const char* characteristic, const char* value,
    DeviceInfos* device_infos) {
  VLOG(1) << __func__ << ": " << characteristic << ": " << value;
  if (strcmp(characteristic, kLensFacing) == 0) {
    (*device_infos)[camera_id].lens_facing = strtol(value, NULL, 10);
  } else if (strcmp(characteristic, kSensorOrientation) == 0) {
    (*device_infos)[camera_id].sensor_orientation = strtol(value, NULL, 10);
  } else {
    LOG(ERROR) << __func__ << ": Unknown characteristic: " << characteristic
               << " value: " << value;
  }
}

void CameraCharacteristics::AddPerModuleCharacteristic(
    uint32_t camera_id, const char* characteristic, const char* value,
    DeviceInfos* device_infos) {
  if (strcmp(characteristic, kFramesToSkipAfterStreamon) == 0) {
    VLOG(1) << __func__ << ": " << characteristic << ": " << value;
    (*device_infos)[camera_id].frames_to_skip_after_streamon =
        strtol(value, NULL, 10);
  } else if (strcmp(characteristic, kHorizontalViewAngle_16_9) == 0) {
    AddFloatValue(value, kHorizontalViewAngle_16_9,
                  &(*device_infos)[camera_id].horizontal_view_angle_16_9);
  } else if (strcmp(characteristic, kHorizontalViewAngle_4_3) == 0) {
    AddFloatValue(value, kHorizontalViewAngle_4_3,
                  &(*device_infos)[camera_id].horizontal_view_angle_4_3);
  } else if (strcmp(characteristic, kLensInfoAvailableFocalLengths) == 0) {
    (*device_infos)[camera_id].lens_info_available_focal_lengths.clear();
    char tmp_value[256];
    strcpy(tmp_value, value);
    char* focal_length = strtok(tmp_value, ",");
    while (focal_length) {
      float tmp_focal_length = strtof(focal_length, NULL);
      if (tmp_focal_length != 0.0) {
        VLOG(1) << __func__ << ": " << characteristic << ": "
                << tmp_focal_length;
        (*device_infos)[camera_id].lens_info_available_focal_lengths.push_back(
            tmp_focal_length);
      } else {
        LOG(ERROR) << __func__ << ": Invalid " << characteristic << ": "
                   << value;
        (*device_infos)[camera_id].lens_info_available_focal_lengths.clear();
        (*device_infos)[camera_id].lens_info_available_focal_lengths.push_back(
            kDefaultCharacteristics.lens_info_available_focal_lengths[0]);
        break;
      }
      focal_length = strtok(NULL, ",");
    }
  } else if (strcmp(characteristic, kLensInfoMinimumFocusDistance) == 0) {
    AddFloatValue(value, kLensInfoMinimumFocusDistance,
                  &(*device_infos)[camera_id].lens_info_minimum_focus_distance);
  } else if (strcmp(characteristic, kLensInfoOptimalFocusDistance) == 0) {
    AddFloatValue(value, kLensInfoOptimalFocusDistance,
                  &(*device_infos)[camera_id].lens_info_optimal_focus_distance);
  } else if (strcmp(characteristic, kVerticalViewAngle_16_9) == 0) {
    AddFloatValue(value, kVerticalViewAngle_16_9,
                  &(*device_infos)[camera_id].vertical_view_angle_16_9);
  } else if (strcmp(characteristic, kVerticalViewAngle_4_3) == 0) {
    AddFloatValue(value, kVerticalViewAngle_4_3,
                  &(*device_infos)[camera_id].vertical_view_angle_4_3);
  } else if (strcmp(characteristic, kLensInfoAvailableApertures) == 0) {
    /* Do nothing. This is for hal v3 */
  } else if (strcmp(characteristic, kSensorInfoPhysicalSize) == 0) {
    /* Do nothing. This is for hal v3 */
  } else if (strcmp(characteristic, kSensorInfoPixelArraySize) == 0) {
    int width, height;
    if (sscanf(value, "%dx%d", &width, &height) != 2) {
      LOG(ERROR) << __func__ << ": Illegal array size format: " << value;
      return;
    }
    VLOG(1) << __func__ << ": " << characteristic << ": " << width
            << "x" << height;
    (*device_infos)[camera_id].sensor_info_pixel_array_size_width = width;
    (*device_infos)[camera_id].sensor_info_pixel_array_size_height = height;
  } else if (strcmp(characteristic, kResolution1280x960Unsupported) == 0) {
    VLOG(1) << __func__ << ": " << characteristic << ": " << value;
    std::istringstream is(value);
    is >> std::boolalpha
       >> (*device_infos)[camera_id].resolution_1280x960_unsupported;
  } else if (strcmp(characteristic, kResolution1600x1200Unsupported) == 0) {
    VLOG(1) << __func__ << ": " << characteristic << ": " << value;
    std::istringstream is(value);
    is >> std::boolalpha
       >> (*device_infos)[camera_id].resolution_1600x1200_unsupported;
  } else if (strcmp(characteristic, kConstantFramerateUnsupported) == 0) {
    VLOG(1) << __func__ << ": " << characteristic << ": " << value;
    std::istringstream is(value);
    is >> std::boolalpha
       >> (*device_infos)[camera_id].constant_framerate_unsupported;
  } else {
    LOG(ERROR) << __func__ << ": Unknown characteristic: " << characteristic
               << " value: " << value;
  }
}

void CameraCharacteristics::AddFloatValue(const char* value,
                                          const char* characteristic_name,
                                          float* characteristic) {
  float tmp_value = strtof(value, NULL);
  if (tmp_value != 0.0) {
    VLOG(1) << __func__ << ": " << characteristic_name << ": " << value;
    *characteristic = tmp_value;
  } else {
    LOG(ERROR) << __func__ << ": Invalid " << characteristic_name
               << ": " << value;
  }
}
