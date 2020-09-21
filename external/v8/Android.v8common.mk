LOCAL_CXX_STL := libc++

LOCAL_CPP_EXTENSION := cc

LOCAL_CFLAGS += \
	-Wall \
	-Werror \
	-Wno-endif-labels \
	-Wno-import \
	-Wno-format \
	-Wno-unused-parameter \
	-Wno-unused-private-field \
	-Wno-sign-compare \
	-Wno-missing-field-initializers \
	-Wno-ignored-qualifiers \
	-Wno-undefined-var-template \
	-fno-exceptions \
	-fvisibility=hidden \
	-DENABLE_DEBUGGER_SUPPORT \
	-DENABLE_LOGGING_AND_PROFILING \
	-DENABLE_VMSTATE_TRACKING \
	-DV8_NATIVE_REGEXP \
	-DV8_I18N_SUPPORT \
	-std=gnu++0x \
	-Os

LOCAL_CFLAGS_arm += -DV8_TARGET_ARCH_ARM
LOCAL_CFLAGS_arm64 += -DV8_TARGET_ARCH_ARM64

LOCAL_CFLAGS_mips += -DV8_TARGET_ARCH_MIPS \
	-Umips \
	-finline-limit=64 \
	-fno-strict-aliasing
LOCAL_CFLAGS_mips64 += -DV8_TARGET_ARCH_MIPS64 \
	-Umips \
	-finline-limit=64 \
	-fno-strict-aliasing

LOCAL_CFLAGS_x86 += -DV8_TARGET_ARCH_IA32
LOCAL_CFLAGS_x86_64 += -DV8_TARGET_ARCH_X64



