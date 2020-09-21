# Add compiled framework resources to a robolectric jar file
# Input variable:
#   robo_stub_module_name: the name of the make module that produces the Java library; the jar file should have been generated to
#                  $(TARGET_OUT_COMMON_INTERMEDIATES)/JAVA_LIBRARIES/$(robo_stub_module_name)_intermediates.
# Output variable:
#   robo_full_target: the built classes-with-res.jar
#
# Depends on framework-res.apk, in order to pull the
# resource files out of there.
#
robo_intermediates := $(call intermediates-dir-for,JAVA_LIBRARIES,$(robo_stub_module_name),,COMMON)
robo_full_target := $(robo_intermediates)/classes-with-res.jar
robo_input_jar := $(robo_intermediates)/classes.jar
robo_classes_dir := $(robo_intermediates)/classes
framework_res_package := $(call intermediates-dir-for,APPS,framework-res,,COMMON)/package-export.apk

$(robo_full_target): PRIVATE_INTERMEDIATES_DIR := $(robo_intermediates)
$(robo_full_target): PRIVATE_FRAMEWORK_RES_PACKAGE := $(framework_res_package)

$(robo_full_target): PRIVATE_CLASS_INTERMEDIATES_DIR := $(robo_classes_dir)

.KATI_RESTAT: $(robo_full_target)
$(robo_full_target): $(framework_res_package) $(robo_input_jar) $(ZIPTIME)
	@echo Adding framework resources: $@
	$(hide) rm -rf $(PRIVATE_CLASS_INTERMEDIATES_DIR)
	$(hide) mkdir -p $(PRIVATE_CLASS_INTERMEDIATES_DIR)
	$(hide) if [ ! -f $(robo_input_jar) ]; then \
		echo Missing file $(robo_input_jar); \
		rm -rf $(PRIVATE_CLASS_INTERMEDIATES_DIR); \
		exit 1; \
	fi;
	$(hide) unzip -qo $(robo_input_jar) -d $(PRIVATE_CLASS_INTERMEDIATES_DIR)
	# Move the raw/uncompiled resources into raw-res/
	# This logic can be removed once the transition to binary resources is complete
	$(hide) mkdir -p  $(PRIVATE_CLASS_INTERMEDIATES_DIR)/raw-res
	$(hide) mv $(PRIVATE_CLASS_INTERMEDIATES_DIR)/res $(PRIVATE_CLASS_INTERMEDIATES_DIR)/raw-res/
	$(hide) mv $(PRIVATE_CLASS_INTERMEDIATES_DIR)/assets $(PRIVATE_CLASS_INTERMEDIATES_DIR)/raw-res/
	$(hide) if [ ! -f $(PRIVATE_FRAMEWORK_RES_PACKAGE) ]; then \
		echo Missing file $(PRIVATE_FRAMEWORK_RES_PACKAGE); \
		rm -rf $(PRIVATE_CLASS_INTERMEDIATES_DIR); \
		exit 1; \
	fi;
	$(hide) unzip -qo $(PRIVATE_FRAMEWORK_RES_PACKAGE) -d $(PRIVATE_CLASS_INTERMEDIATES_DIR)
	$(hide) (cd $(PRIVATE_CLASS_INTERMEDIATES_DIR) && rm -rf classes.dex META-INF)
	$(hide) mkdir -p $(dir $@)
	$(hide) jar -cf $@.tmp -C $(PRIVATE_CLASS_INTERMEDIATES_DIR) .
	$(hide) jar -u0f $@.tmp -C $(PRIVATE_CLASS_INTERMEDIATES_DIR) resources.arsc
	$(hide) $(ZIPTIME) $@.tmp
	$(hide) $(call commit-change-for-toc,$@)

