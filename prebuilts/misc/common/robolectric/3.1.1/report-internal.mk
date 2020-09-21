# Defines a target named $(my_target) for generating a coverage report.

my_report_dir := $(my_coverage_dir)/reports
my_coverage_output := $(my_report_dir)/html

# Private variables.
$(my_coverage_output): \
		PRIVATE_MODULE := $(LOCAL_MODULE)
$(my_coverage_output): \
		PRIVATE_COVERAGE_FILE := $(my_coverage_file)
$(my_coverage_output): \
		PRIVATE_COVERAGE_SRCS_JARS := $(my_coverage_srcs_jars)
$(my_coverage_output): \
		PRIVATE_INSTRUMENT_SOURCE_DIRS := $(my_instrument_source_dirs)
$(my_coverage_output): \
		PRIVATE_COVERAGE_REPORT_CLASS := $(my_coverage_report_class)
$(my_coverage_output): \
		PRIVATE_COVERAGE_REPORT_JAR := $(my_coverage_report_jar)
$(my_coverage_output): \
		PRIVATE_REPORT_DIR := $(my_report_dir)

# Generate the coverage report.
$(my_coverage_output): $(my_collect_target) $(my_coverage_report_jar)
	$(hide) rm -rf $(PRIVATE_REPORT_DIR)
	$(hide) mkdir -p $(PRIVATE_REPORT_DIR)
	$(hide) java \
			-cp $(PRIVATE_COVERAGE_REPORT_JAR) \
			$(PRIVATE_COVERAGE_REPORT_CLASS) \
			--classpath $(strip $(call normalize-path-list, $(PRIVATE_COVERAGE_SRCS_JARS))) \
			--exec-file $(PRIVATE_COVERAGE_FILE) \
			--name $(PRIVATE_MODULE) \
			--report-dir $(PRIVATE_REPORT_DIR)/ \
			--srcs $(strip $(call normalize-path-list, $(PRIVATE_INSTRUMENT_SOURCE_DIRS))) \
			>$(PRIVATE_REPORT_DIR)/reporter.txt 2>&1
	$(hide) abs="$$(realpath "$(PRIVATE_REPORT_DIR)")"; \
			echo "Coverage report:" \
					"file://$$abs/index.html" \

# Generate a ZIP file of the coverage report.
my_coverage_output_zip := \
		$(my_coverage_dir)/report-$(my_coverage_report_format).zip

$(my_coverage_output_zip): \
		PRIVATE_REPORT_DIR := $(my_report_dir)
$(my_coverage_output_zip): $(my_coverage_output)
	$(hide) cd $(PRIVATE_REPORT_DIR) && zip -r $(PWD)/$@ .

# Add coverage report zip to dist files.
$(call dist-for-goals, $(my_target), \
		$(my_coverage_output_zip):robotests/$(my_coverage_report_dist_file))

# Running the coverage will always generate the report.
$(my_target): $(my_coverage_output)

# Reset local variables.
my_coverage_output :=
my_coverage_output_zip :=
my_report_dir :=
