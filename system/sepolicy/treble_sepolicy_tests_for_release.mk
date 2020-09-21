version := $(version_under_treble_tests)

include $(CLEAR_VARS)
# For Treble builds run tests verifying that processes are properly labeled and
# permissions granted do not violate the treble model.  Also ensure that treble
# compatibility guarantees are upheld between SELinux version bumps.
LOCAL_MODULE := treble_sepolicy_tests_$(version)
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := tests

include $(BUILD_SYSTEM)/base_rules.mk

# $(version)_plat - the platform policy shipped as part of the $(version) release.  This is
# built to enable us to determine the diff between the current policy and the
# $(version) policy, which will be used in tests to make sure that compatibility has
# been maintained by our mapping files.
$(version)_PLAT_PUBLIC_POLICY := $(LOCAL_PATH)/prebuilts/api/$(version)/public
$(version)_PLAT_PRIVATE_POLICY := $(LOCAL_PATH)/prebuilts/api/$(version)/private
$(version)_plat_policy.conf := $(intermediates)/$(version)_plat_policy.conf
$($(version)_plat_policy.conf): PRIVATE_MLS_SENS := $(MLS_SENS)
$($(version)_plat_policy.conf): PRIVATE_MLS_CATS := $(MLS_CATS)
$($(version)_plat_policy.conf): PRIVATE_TARGET_BUILD_VARIANT := user
$($(version)_plat_policy.conf): PRIVATE_TGT_ARCH := $(my_target_arch)
$($(version)_plat_policy.conf): PRIVATE_TGT_WITH_ASAN := $(with_asan)
$($(version)_plat_policy.conf): PRIVATE_ADDITIONAL_M4DEFS := $(LOCAL_ADDITIONAL_M4DEFS)
$($(version)_plat_policy.conf): PRIVATE_SEPOLICY_SPLIT := true
$($(version)_plat_policy.conf): $(call build_policy, $(sepolicy_build_files), \
$($(version)_PLAT_PUBLIC_POLICY) $($(version)_PLAT_PRIVATE_POLICY))
	$(transform-policy-to-conf)
	$(hide) sed '/dontaudit/d' $@ > $@.dontaudit


built_$(version)_plat_sepolicy := $(intermediates)/built_$(version)_plat_sepolicy
$(built_$(version)_plat_sepolicy): PRIVATE_ADDITIONAL_CIL_FILES := \
  $(call build_policy, technical_debt.cil , $($(version)_PLAT_PRIVATE_POLICY))
$(built_$(version)_plat_sepolicy): PRIVATE_NEVERALLOW_ARG := $(NEVERALLOW_ARG)
$(built_$(version)_plat_sepolicy): $($(version)_plat_policy.conf) $(HOST_OUT_EXECUTABLES)/checkpolicy \
  $(HOST_OUT_EXECUTABLES)/secilc \
  $(call build_policy, technical_debt.cil, $($(version)_PLAT_PRIVATE_POLICY)) \
  $(built_sepolicy_neverallows)
	@mkdir -p $(dir $@)
	$(hide) $(CHECKPOLICY_ASAN_OPTIONS) $(HOST_OUT_EXECUTABLES)/checkpolicy -M -C -c \
		$(POLICYVERS) -o $@ $<
	$(hide) cat $(PRIVATE_ADDITIONAL_CIL_FILES) >> $@
	$(hide) $(HOST_OUT_EXECUTABLES)/secilc -m -M true -G -c $(POLICYVERS) $(PRIVATE_NEVERALLOW_ARG) $@ -o $@ -f /dev/null

$(version)_plat_policy.conf :=


# $(version)_compat - the current plat_sepolicy.cil built with the compatibility file
# targeting the $(version) SELinux release.  This ensures that our policy will build
# when used on a device that has non-platform policy targetting the $(version) release.
$(version)_compat := $(intermediates)/$(version)_compat
$(version)_mapping.cil := $(LOCAL_PATH)/private/compat/$(version)/$(version).cil
$(version)_mapping.ignore.cil := $(LOCAL_PATH)/private/compat/$(version)/$(version).ignore.cil
$(version)_nonplat := $(LOCAL_PATH)/prebuilts/api/$(version)/nonplat_sepolicy.cil
$($(version)_compat): PRIVATE_CIL_FILES := \
$(built_plat_cil) $($(version)_mapping.cil) $($(version)_nonplat)
$($(version)_compat): $(HOST_OUT_EXECUTABLES)/secilc \
$(built_plat_cil) $($(version)_mapping.cil) $($(version)_nonplat)
	$(hide) $(HOST_OUT_EXECUTABLES)/secilc -m -M true -G -N -c $(POLICYVERS) \
		$(PRIVATE_CIL_FILES) -o $@ -f /dev/null

# $(version)_mapping.combined.cil - a combination of the mapping file used when
# combining the current platform policy with nonplatform policy based on the
# $(version) policy release and also a special ignored file that exists purely for
# these tests.
$(version)_mapping.combined.cil := $(intermediates)/$(version)_mapping.combined.cil
$($(version)_mapping.combined.cil): $($(version)_mapping.cil) $($(version)_mapping.ignore.cil)
	mkdir -p $(dir $@)
	cat $^ > $@

treble_sepolicy_tests_$(version) := $(intermediates)/treble_sepolicy_tests_$(version)
$(treble_sepolicy_tests_$(version)): ALL_FC_ARGS := $(all_fc_args)
$(treble_sepolicy_tests_$(version)): PRIVATE_SEPOLICY := $(built_sepolicy)
$(treble_sepolicy_tests_$(version)): PRIVATE_SEPOLICY_OLD := $(built_$(version)_plat_sepolicy)
$(treble_sepolicy_tests_$(version)): PRIVATE_COMBINED_MAPPING := $($(version)_mapping.combined.cil)
$(treble_sepolicy_tests_$(version)): PRIVATE_PLAT_SEPOLICY := $(built_plat_sepolicy)
ifeq ($(PRODUCT_FULL_TREBLE_OVERRIDE),true)
$(treble_sepolicy_tests_$(version)): PRIVATE_FAKE_TREBLE := --fake-treble
else
$(treble_sepolicy_tests_$(version)): PRIVATE_FAKE_TREBLE :=
endif
$(treble_sepolicy_tests_$(version)): $(HOST_OUT_EXECUTABLES)/treble_sepolicy_tests \
  $(all_fc_files) $(built_sepolicy) $(built_plat_sepolicy) \
  $(built_$(version)_plat_sepolicy) $($(version)_compat) $($(version)_mapping.combined.cil)
	@mkdir -p $(dir $@)
	$(hide) $(HOST_OUT_EXECUTABLES)/treble_sepolicy_tests -l \
		$(HOST_OUT)/lib64/libsepolwrap.$(SHAREDLIB_EXT) $(ALL_FC_ARGS) \
		-b $(PRIVATE_PLAT_SEPOLICY) -m $(PRIVATE_COMBINED_MAPPING) \
		-o $(PRIVATE_SEPOLICY_OLD) -p $(PRIVATE_SEPOLICY) \
		$(PRIVATE_FAKE_TREBLE)
	$(hide) touch $@

$(version)_PLAT_PUBLIC_POLICY :=
$(version)_PLAT_PRIVATE_POLICY :=
$(version)_compat :=
$(version)_mapping.cil :=
$(version)_mapping.combined.cil :=
$(version)_mapping.ignore.cil :=
$(version)_nonplat :=
built_$(version)_plat_sepolicy :=
version :=
version_under_treble_tests :=
