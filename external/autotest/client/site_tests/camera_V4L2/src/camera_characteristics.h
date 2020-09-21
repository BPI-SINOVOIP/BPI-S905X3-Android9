/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CAMERA_CHARACTERISTICS_H_
#define CAMERA_CHARACTERISTICS_H_

#include "common_types.h"

#include <unordered_map>

#include <base/macros.h>

// CameraCharacteristics reads the file /etc/camera/camera_characteristics.conf.
// There are several assumptions of the config file:
//  1. Each line should be at most 256 characters long.
//  2. camera id should be in ascending order (i.e., 0, 1, 2, ...).
//  3. usb_vid_pid should be the first subkey.
//  4. All configs of a module should be put together.
//
// Example of the config file:
//  camera0.lens_facing=0
//  camera0.sensor_orientation=0
//  camera0.module0.usb_vid_pid=0123:4567
//  camera0.module0.horizontal_view_angle=68.4
//  camera0.module0.lens_info_available_focal_lengths=1.64
//  camera0.module0.lens_info_minimum_focus_distance=0.22
//  camera0.module0.lens_info_optimal_focus_distance=0.5
//  camera0.module0.vertical_view_angle=41.6
//  camera0.module1.usb_vid_pid=89ab:cdef
//  camera0.module1.lens_info_available_focal_lengths=1.69,2
//  camera1.lens_facing=1
//  camera1.sensor_orientation=180
class CameraCharacteristics {
 public:
  CameraCharacteristics();
  ~CameraCharacteristics();

  // Parses /etc/camera/camera_characteristics.conf.
  // Returns DeviceInfos with default characteristics if the config file doesn't
  // exist.
  const DeviceInfos GetCharacteristicsFromFile(
      const std::unordered_map<std::string, std::string>& devices);

 private:
  void AddPerCameraCharacteristic(
      uint32_t camera_id, const char* characteristic, const char* value,
      DeviceInfos* device_infos);
  void AddPerModuleCharacteristic(
      uint32_t camera_id, const char* characteristic, const char* value,
      DeviceInfos* device_infos);
  void AddFloatValue(const char* value, const char* characteristic_name,
                     float* characteristic);

  DISALLOW_COPY_AND_ASSIGN(CameraCharacteristics);
};

#endif  // CAMERA_CHARACTERISTICS_H_
