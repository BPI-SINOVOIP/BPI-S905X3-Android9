/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#ifndef SYSTEM_CONTROL_H
#define SYSTEM_CONTROL_H
#include <errno.h>
#include <BasicTypes.h>

int32_t sc_get_hdmitx_mode_list(std::vector<std::string>& edidlist);
int32_t sc_get_hdmitx_hdcp_state(bool & val);
int32_t sc_get_display_mode(std::string &dispmode);
int32_t sc_set_display_mode(std::string &dispmode);
int32_t sc_get_osd_position(std::string &dispmode, int *position);

int32_t sc_write_sysfs(const char * path, std::string & val);
int32_t sc_read_sysfs(const char * path, std::string & val);

int32_t sc_read_bootenv(const char * key, std::string & val);
int32_t sc_set_property(const char *prop, const char *val);

bool sc_get_pref_display_mode(std::string & dispmode);
#endif/*SYSTEM_CONTROL_H*/
