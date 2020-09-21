# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Disable HSP/HFP on Google WiFi (Gale) with UART-HCI Bluetooth
# which is incapable to handle SCO audio.
platform_name="$(mosys platform name)"
if [ "$platform_name" = "Gale" ]; then
     DISABLE_PROFILE="--disable_profile=hfp,hsp"
fi
# For board needs different device configs, check which config
# directory to use. Use that directory for both volume curves
# and dsp config.
if [ -f /etc/cras/get_device_config_dir ]; then
  device_config_dir="$(sh /etc/cras/get_device_config_dir)"
  DEVICE_CONFIG_DIR="--device_config_dir=${device_config_dir}"
  DSP_CONFIG="--dsp_config=${device_config_dir}/dsp.ini"
fi
if [ -f /etc/cras/get_internal_ucm_suffix ]; then
  internal_ucm_suffix="$(sh /etc/cras/get_internal_ucm_suffix)"
  INTERNAL_UCM_SUFFIX="--internal_ucm_suffix=${internal_ucm_suffix}"
fi
exec minijail0 -u cras -g cras -G -- /usr/bin/cras \
    ${DSP_CONFIG} ${DEVICE_CONFIG_DIR} ${DISABLE_PROFILE} \
    ${INTERNAL_UCM_SUFFIX}
