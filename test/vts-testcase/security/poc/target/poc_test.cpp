/*
 * Copyright 2016 The Android Open Source Project
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

#include "poc_test.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::string;

static struct option long_options[] = {
  {"device_model", required_argument, 0, 'd'},
  {"params", required_argument, 0, 'p'}
};

static DeviceModel TranslateDeviceModel(const char *model_name) {
  DeviceModel device_model;
  if (!strcmp("Nexus 5", model_name)) {
      device_model = NEXUS_5;
  } else if (!strcmp("Nexus 5X", model_name)) {
      device_model = NEXUS_5X;
  } else if (!strcmp("Nexus 6", model_name)) {
      device_model = NEXUS_6;
  } else if (!strcmp("Nexus 6P", model_name)) {
      device_model = NEXUS_6P;
  } else if (!strcmp("Pixel", model_name)) {
      device_model = PIXEL;
  } else if (!strcmp("Pixel XL", model_name)) {
      device_model = PIXEL_XL;
  } else {
      device_model = OTHER;
  }
  return device_model;
}

static map<string, string> ExtractParams(const char *test_params) {
  map<string, string> params;
  string input(test_params);
  std::istringstream iss(input);

  string key_value;
  while(std::getline(iss, key_value, ',')) {
    size_t delim = key_value.find('=');
    if (delim == string::npos) {
      cerr << "Missing '=' delimiter.\n";
      exit(POC_TEST_SKIP);
    }

    string key = key_value.substr(0, delim);
    string value = key_value.substr(delim + 1);

    params[key] = value;
  }

  return params;
}

VtsHostInput ParseVtsHostFlags(int argc, char *argv[]) {
  VtsHostInput host_input;
  int opt = 0;
  int index = 0;
  while ((opt = getopt_long_only(argc, argv, "", long_options, &index)) != -1) {
    switch(opt) {
      case 'd':
        host_input.device_model = TranslateDeviceModel(optarg);
        break;
      case 'p':
        host_input.params = ExtractParams(optarg);
        break;
      default:
        cerr << "Wrong parameters.\n";
        exit(POC_TEST_SKIP);
    }
  }
  return host_input;
}
