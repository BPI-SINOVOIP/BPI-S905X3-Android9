#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DVB_TOP := $(call my-dir)

SUPPORT_CAS := n

ifeq (1,$(strip $(shell expr $(PLATFORM_SDK_VERSION) \>= 22)))
include $(DVB_TOP)/am_sysfs/Android.mk $(DVB_TOP)/am_adp/Android.mk $(DVB_TOP)/am_mw/Android.mk $(DVB_TOP)/am_ver/Android.mk
else
include $(DVB_TOP)/am_adp/Android.mk $(DVB_TOP)/am_mw/Android.mk $(DVB_TOP)/am_ver/Android.mk
endif

#include $(call all-makefiles-under,$(DVB_TOP)/test)
include $(DVB_TOP)/test/am_fend_test/Android.mk
include $(DVB_TOP)/test/am_dmx_test/Android.mk
include $(DVB_TOP)/test/am_av_test/Android.mk
include $(DVB_TOP)/test/am_dvr_test/Android.mk
include $(DVB_TOP)/test/am_dsc_test/Android.mk
include $(DVB_TOP)/test/am_userdata_test/Android.mk
include $(DVB_TOP)/test/am_timeshift_test/Android.mk
