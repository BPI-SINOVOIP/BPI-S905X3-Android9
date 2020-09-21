#!/usr/bin/env python

import os

# Given a list of source files from the V8 gyp file, create a string that
# can be put into the LOCAL_SRC_FILES makefile variable. One per line, followed
# by a '\' and with the 'src' directory prepended.
def _makefileSources(src_list):
  result = ""
  for i in xrange(0, len(src_list)):
    result += '\tsrc/' + src_list[i]
    if i != len(src_list) - 1:
      result += ' \\'
    result += '\n'
  return result

# Return a string that contains the common header variable setup used in
# (most of) the V8 makefiles.
def _makefileCommonHeader(module_name):
  result = ""
  result += '### GENERATED, do not edit\n'
  result += '### for changes, see genmakefiles.py\n'
  result += 'LOCAL_PATH := $(call my-dir)\n'
  result += 'include $(CLEAR_VARS)\n'
  result += 'include $(LOCAL_PATH)/Android.v8common.mk\n'
  result += 'LOCAL_MODULE := ' + module_name + '\n'
  return result

# Write a makefile in the simpler format used by v8_libplatform and
# v8_libsampler
def _writeMakefile(filename, module_name, sources):
  if not sources:
    raise ValueError('No sources for ' + filename)

  with open(filename, 'w') as out:
    out.write(_makefileCommonHeader(module_name))
    out.write('LOCAL_MODULE_CLASS := STATIC_LIBRARIES\n')

    out.write('LOCAL_SRC_FILES := \\\n')

    out.write(_makefileSources(sources))

    out.write('LOCAL_C_INCLUDES := \\\n')
    out.write('\t$(LOCAL_PATH)/src \\\n')
    out.write('\t$(LOCAL_PATH)/include\n')

    out.write('include $(BUILD_STATIC_LIBRARY)\n')

def _writeMkpeepholeMakefile(target):
  if not target:
    raise ValueError('Must specify mkpeephole target properties')

  with open('Android.mkpeephole.mk', 'w') as out:
    out.write(_makefileCommonHeader('v8mkpeephole'))
    out.write('LOCAL_SRC_FILES := \\\n')
    sources = [x for x in target['sources'] if x.endswith('.cc')]
    sources.sort()

    out.write(_makefileSources(sources))

    out.write('LOCAL_STATIC_LIBRARIES += libv8base liblog\n')
    out.write('LOCAL_LDLIBS_linux += -lrt\n')
    out.write('include $(BUILD_HOST_EXECUTABLE)\n')

    out.write('include $(CLEAR_VARS)\n')
    out.write('include $(LOCAL_PATH)/Android.v8common.mk\n')
    out.write('LOCAL_MODULE := v8peephole\n')
    out.write('LOCAL_MODULE_CLASS := STATIC_LIBRARIES\n')
    out.write('generated_sources := $(call local-generated-sources-dir)\n')
    out.write('PEEPHOLE_TOOL := $(HOST_OUT_EXECUTABLES)/v8mkpeephole\n')
    out.write('PEEPHOLE_FILE := ' \
        '$(generated_sources)/bytecode-peephole-table.cc\n')
    out.write('$(PEEPHOLE_FILE): PRIVATE_CUSTOM_TOOL = ' \
        '$(PEEPHOLE_TOOL) $(PEEPHOLE_FILE)\n')
    out.write('$(PEEPHOLE_FILE): $(PEEPHOLE_TOOL)\n')
    out.write('\t$(transform-generated-source)\n')
    out.write('LOCAL_GENERATED_SOURCES += $(PEEPHOLE_FILE)\n')
    out.write('include $(BUILD_STATIC_LIBRARY)\n')

def _writeV8SrcMakefile(target):
  if not target:
    raise ValueError('Must specify v8_base target properties')

  with open('Android.v8.mk', 'w') as out:
    out.write(_makefileCommonHeader('libv8src'))
    out.write('LOCAL_MODULE_CLASS := STATIC_LIBRARIES\n')

    out.write('LOCAL_SRC_FILES := \\\n')

    sources = [x for x in target['sources'] if x.endswith('.cc')]
    sources.sort()

    out.write(_makefileSources(sources))

    arm_src = None
    arm64_src = None
    x86_src = None
    x86_64_src = None
    mips_src = None
    mips64_src = None
    for condition in target['conditions']:
      if condition[0] == 'v8_target_arch=="arm"':
        arm_src = [x for x in condition[1]['sources'] if x.endswith('.cc')]
      elif condition[0] == 'v8_target_arch=="arm64"':
        arm64_src = [x for x in condition[1]['sources'] if x.endswith('.cc')]
      elif condition[0] == 'v8_target_arch=="ia32"':
        x86_src = [x for x in condition[1]['sources'] if x.endswith('.cc')]
      elif condition[0] \
          == 'v8_target_arch=="mips" or v8_target_arch=="mipsel"':
        mips_src = [x for x in condition[1]['sources'] if x.endswith('.cc')]
      elif condition[0] \
          == 'v8_target_arch=="mips64" or v8_target_arch=="mips64el"':
        mips64_src = [x for x in condition[1]['sources'] if x.endswith('.cc')]
      elif condition[0] == 'v8_target_arch=="x64"':
        x86_64_src = [x for x in condition[1]['sources'] if x.endswith('.cc')]

    arm_src.sort()
    arm64_src.sort()
    x86_src.sort()
    x86_64_src.sort()
    mips_src.sort()
    mips64_src.sort()

    out.write('LOCAL_SRC_FILES_arm += \\\n')
    out.write(_makefileSources(arm_src))
    out.write('LOCAL_SRC_FILES_arm64 += \\\n')
    out.write(_makefileSources(arm64_src))
    out.write('LOCAL_SRC_FILES_mips += \\\n')
    out.write(_makefileSources(mips_src))
    out.write('LOCAL_SRC_FILES_mips64 += \\\n')
    out.write(_makefileSources(mips64_src))
    out.write('LOCAL_SRC_FILES_x86 += \\\n')
    out.write(_makefileSources(x86_src))
    out.write('LOCAL_SRC_FILES_x86_64 += \\\n')
    out.write(_makefileSources(x86_64_src))

    out.write('# Enable DEBUG option.\n')
    out.write('ifeq ($(DEBUG_V8),true)\n')
    out.write('LOCAL_SRC_FILES += \\\n')
    out.write('\tsrc/objects-debug.cc \\\n')
    out.write('\tsrc/ast/prettyprinter.cc \\\n')
    out.write('\tsrc/regexp/regexp-macro-assembler-tracer.cc\n')
    out.write('endif\n')
    out.write('LOCAL_C_INCLUDES := \\\n')
    out.write('\t$(LOCAL_PATH)/src \\\n')
    out.write('\texternal/icu/icu4c/source/common \\\n')
    out.write('\texternal/icu/icu4c/source/i18n\n')
    out.write('include $(BUILD_STATIC_LIBRARY)\n')

def _writeGeneratedFilesMakfile(target):
  if not target:
    raise ValueError('Must specify j2sc target properties')

  with open('Android.v8gen.mk', 'w') as out:
    out.write(_makefileCommonHeader('libv8gen'))
    out.write('LOCAL_MODULE_CLASS := STATIC_LIBRARIES\n')

    sources = target['variables']['library_files']
    out.write('V8_LOCAL_JS_LIBRARY_FILES := \\\n')
    out.write(_makefileSources(sources))

    sources = target['variables']['experimental_library_files']
    out.write('V8_LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES := \\\n')
    out.write(_makefileSources(sources))

    out.write('LOCAL_SRC_FILES += src/snapshot/snapshot-empty.cc\n')

    out.write('LOCAL_JS_LIBRARY_FILES := ' \
        '$(addprefix $(LOCAL_PATH)/, $(V8_LOCAL_JS_LIBRARY_FILES))\n')
    out.write('LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES := ' \
        '$(addprefix $(LOCAL_PATH)/, ' \
        '$(V8_LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES))\n')
    out.write('generated_sources := $(call local-generated-sources-dir)\n')
    out.write('')

    # Copy js2c.py to generated sources directory and invoke there to avoid
    # generating jsmin.pyc in the source directory
    out.write('JS2C_PY := ' \
        '$(generated_sources)/js2c.py $(generated_sources)/jsmin.py\n')
    out.write('$(JS2C_PY): ' \
        '$(generated_sources)/%.py : $(LOCAL_PATH)/tools/%.py | $(ACP)\n')
    out.write('\t@echo "Copying $@"\n')
    out.write('\t$(copy-file-to-target)\n')

    # Generate libraries.cc
    out.write('GEN1 := $(generated_sources)/libraries.cc\n')
    out.write('$(GEN1): SCRIPT := $(generated_sources)/js2c.py\n')
    out.write('$(GEN1): $(LOCAL_JS_LIBRARY_FILES) $(JS2C_PY)\n')
    out.write('\t@echo "Generating libraries.cc"\n')
    out.write('\t@mkdir -p $(dir $@)\n')
    out.write('\tpython $(SCRIPT) $@ CORE $(LOCAL_JS_LIBRARY_FILES)\n')
    out.write('V8_GENERATED_LIBRARIES := $(generated_sources)/libraries.cc\n')

    # Generate experimental-libraries.cc
    out.write('GEN2 := $(generated_sources)/experimental-libraries.cc\n')
    out.write('$(GEN2): SCRIPT := $(generated_sources)/js2c.py\n')
    out.write('$(GEN2): $(LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES) $(JS2C_PY)\n')
    out.write('\t@echo "Generating experimental-libraries.cc"\n')
    out.write('\t@mkdir -p $(dir $@)\n')
    out.write('\tpython $(SCRIPT) $@ EXPERIMENTAL ' \
        '$(LOCAL_JS_EXPERIMENTAL_LIBRARY_FILES)\n')
    out.write('V8_GENERATED_LIBRARIES ' \
        '+= $(generated_sources)/experimental-libraries.cc\n')

    # Generate extra-libraries.cc
    out.write('GEN3 := $(generated_sources)/extra-libraries.cc\n')
    out.write('$(GEN3): SCRIPT := $(generated_sources)/js2c.py\n')
    out.write('$(GEN3): $(JS2C_PY)\n')
    out.write('\t@echo "Generating extra-libraries.cc"\n')
    out.write('\t@mkdir -p $(dir $@)\n')
    out.write('\tpython $(SCRIPT) $@ EXTRAS\n')
    out.write('V8_GENERATED_LIBRARIES ' \
        '+= $(generated_sources)/extra-libraries.cc\n')

    # Generate experimental-extra-libraries.cc
    out.write('GEN4 := $(generated_sources)/experimental-extra-libraries.cc\n')
    out.write('$(GEN4): SCRIPT := $(generated_sources)/js2c.py\n')
    out.write('$(GEN4): $(JS2C_PY)\n')
    out.write('\t@echo "Generating experimental-extra-libraries.cc"\n')
    out.write('\t@mkdir -p $(dir $@)\n')
    out.write('\tpython $(SCRIPT) $@ EXPERIMENTAL_EXTRAS\n')
    out.write('V8_GENERATED_LIBRARIES ' \
        '+= $(generated_sources)/experimental-extra-libraries.cc\n')

    out.write('LOCAL_GENERATED_SOURCES += $(V8_GENERATED_LIBRARIES)\n')

    out.write('include $(BUILD_STATIC_LIBRARY)\n')

def _writeLibBaseMakefile(target):
  if not target:
    raise ValueError('Must specify v8_libbase target properties')

  with open('Android.base.mk', 'w') as out:
    out.write(_makefileCommonHeader('libv8base'))
    out.write('LOCAL_MODULE_CLASS := STATIC_LIBRARIES\n')

    out.write('LOCAL_SRC_FILES := \\\n')

    sources = [x for x in target['sources'] if x.endswith('.cc')]
    sources += ['base/platform/platform-posix.cc']
    sources.sort()

    out.write(_makefileSources(sources))
    out.write('LOCAL_SRC_FILES += \\\n')
    out.write('\tsrc/base/debug/stack_trace_android.cc \\\n')
    out.write('\tsrc/base/platform/platform-linux.cc\n')

    out.write('LOCAL_C_INCLUDES := $(LOCAL_PATH)/src\n')
    out.write('include $(BUILD_STATIC_LIBRARY)\n\n')

    out.write('include $(CLEAR_VARS)\n')

    out.write('include $(LOCAL_PATH)/Android.v8common.mk\n')

    # Set up the target identity
    out.write('LOCAL_MODULE := libv8base\n')
    out.write('LOCAL_MODULE_CLASS := STATIC_LIBRARIES\n')

    out.write('LOCAL_SRC_FILES := \\\n')
    out.write(_makefileSources(sources))

    # Host may be linux or darwin.
    out.write('ifeq ($(HOST_OS),linux)\n')
    out.write('LOCAL_SRC_FILES += \\\n')
    out.write('\tsrc/base/debug/stack_trace_posix.cc \\\n')
    out.write('\tsrc/base/platform/platform-linux.cc\n')
    out.write('endif\n')
    out.write('ifeq ($(HOST_OS),darwin)\n')
    out.write('LOCAL_SRC_FILES += \\\n')
    out.write('\tsrc/base/debug/stack_trace_posix.cc \\\n')
    out.write('\tsrc/base/platform/platform-macos.cc\n')
    out.write('endif\n')

    out.write('LOCAL_C_INCLUDES := $(LOCAL_PATH)/src\n')
    out.write('include $(BUILD_HOST_STATIC_LIBRARY)\n')

def GenerateMakefiles():
  # Slurp in the content of the V8 gyp file.
  with open(os.path.join(os.getcwd(), './src/v8.gyp'), 'r') as f:
    gyp = eval(f.read())

  # Find the targets that we're interested in and write out the makefiles.
  for target in gyp['targets']:
    name = target['target_name']
    sources = None
    if target.get('sources'):
      sources = [x for x in target['sources'] if x.endswith('.cc')]
      sources.sort()

    if name == 'v8_libplatform':
      _writeMakefile('Android.platform.mk', 'libv8platform', sources)
    elif name == 'v8_libsampler':
      _writeMakefile('Android.sampler.mk', 'libv8sampler', sources)
    elif name == 'v8_base':
      _writeV8SrcMakefile(target)
    elif name == 'mkpeephole':
      _writeMkpeepholeMakefile(target)
    elif name == 'js2c':
      _writeGeneratedFilesMakfile(target)
    elif name == 'v8_libbase':
      _writeLibBaseMakefile(target)

