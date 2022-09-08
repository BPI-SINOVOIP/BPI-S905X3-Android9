obj-m	+=	galcore.o
NPU_DIR := $(shell dirname $(lastword $(MAKEFILE_LIST)))
galcore-objs += os/linux/kernel/gc_hal_kernel_device.o
galcore-objs += os/linux/kernel/gc_hal_kernel_linux.o
galcore-objs += os/linux/kernel/gc_hal_kernel_math.o
galcore-objs += os/linux/kernel/gc_hal_kernel_os.o
galcore-objs += os/linux/kernel/gc_hal_kernel_debugfs.o
galcore-objs += os/linux/kernel/gc_hal_kernel_allocator.o
galcore-objs += os/linux/kernel/allocator/default/gc_hal_kernel_allocator_user_memory.o
galcore-objs += os/linux/kernel/allocator/default/gc_hal_kernel_allocator_dma.o
galcore-objs += os/linux/kernel/allocator/default/gc_hal_kernel_allocator_gfp.o
galcore-objs += os/linux/kernel/allocator/default/gc_hal_kernel_allocator_reserved_mem.o
galcore-objs += os/linux/kernel/gc_hal_kernel_driver.o
#galcore-objs += os/linux/kernel/platform/amlogic/gc_hal_kernel_platform_amlogic.o
galcore-objs += os/linux/kernel/allocator/default/gc_hal_kernel_allocator_dmabuf.o
galcore-objs += os/linux/kernel/gc_hal_kernel_iommu.o
galcore-objs += os/linux/kernel/gc_hal_kernel_drm.o
galcore-objs += os/linux/kernel/gc_hal_kernel_sync.o
galcore-objs += os/linux/kernel/gc_hal_kernel_security_channel_emulator.o
galcore-objs += kernel/gc_hal_kernel.o
galcore-objs += kernel/gc_hal_kernel_command.o
galcore-objs += kernel/gc_hal_kernel_db.o
galcore-objs += kernel/gc_hal_kernel_debug.o
galcore-objs += kernel/gc_hal_kernel_event.o
galcore-objs += kernel/gc_hal_kernel_heap.o
galcore-objs += kernel/gc_hal_kernel_mmu.o
galcore-objs += kernel/gc_hal_kernel_video_memory.o
galcore-objs += kernel/gc_hal_kernel_power.o
galcore-objs += kernel/gc_hal_kernel_security_v1.o
galcore-objs += kernel/gc_hal_kernel_preemption.o
galcore-objs += kernel/arch/gc_hal_kernel_context.o
galcore-objs += kernel/arch/gc_hal_kernel_hardware.o
galcore-objs += kernel/arch/gc_hal_kernel_hardware_async_fe.o
galcore-objs += kernel/arch/gc_hal_kernel_hardware_mc_fe.o
galcore-objs += kernel/arch/gc_hal_kernel_hardware_func.o
galcore-objs += kernel/arch/gc_hal_kernel_hardware_func_flop_reset.o
galcore-objs += kernel/arch/gc_hal_kernel_hardware_waitlink_fe.o
galcore-objs += kernel/arch/gc_hal_kernel_recorder.o
galcore-objs += security_v1/gc_hal_ta.o
galcore-objs += security_v1/gc_hal_ta_hardware.o
galcore-objs += security_v1/gc_hal_ta_mmu.o
galcore-objs += security_v1/os/emulator/gc_hal_ta_emulator.o

ifeq ($(PLATFORM_VERSION),9)
galcore-objs += os/linux/kernel/platform/amlogic/gc_hal_kernel_platform_amlogic_p.o
else
galcore-objs += os/linux/kernel/platform/amlogic/gc_hal_kernel_platform_amlogic_q.o
endif

EXTRA_CFLAGS = -DLINUX -DDRIVER
EXTRA_CFLAGS += -DDBG=0
EXTRA_CFLAGS += -DNO_DMA_COHERENT
EXTRA_CFLAGS += -DUSE_PLATFORM_DRIVER=0
EXTRA_CFLAGS += -DVIVANTE_PROFILER=1
EXTRA_CFLAGS += -DVIVANTE_PROFILER_CONTEXT=1
EXTRA_CFLAGS += -DENABLE_GPU_CLOCK_BY_DRIVER=1
EXTRA_CFLAGS += -DUSE_NEW_LINUX_SIGNAL=0
EXTRA_CFLAGS += -DUSE_LINUX_PCIE=0
EXTRA_CFLAGS += -DgcdCACHE_FUNCTION_UNIMPLEMENTED=0
EXTRA_CFLAGS += -DgcdENABLE_3D=1
EXTRA_CFLAGS += -DgcdENABLE_2D=0
EXTRA_CFLAGS += -DgcdENABLE_VG=0
EXTRA_CFLAGS += -DgcdFPGA_BUILD=0
EXTRA_CFLAGS += -DgcdENABLE_DRM=0
EXTRA_CFLAGS += -DgcdLINUX_SYNC_FILE=1
EXTRA_CFLAGS += -I$(NPU_DIR)/kernel/inc
EXTRA_CFLAGS += -I$(NPU_DIR)/kernel
EXTRA_CFLAGS += -I$(NPU_DIR)/kernel/arch
EXTRA_CFLAGS += -I$(NPU_DIR)/os/linux/kernel
EXTRA_CFLAGS += -I$(NPU_DIR)/os/linux/kernel/allocator/default/
EXTRA_CFLAGS += -I$(NPU_DIR)/security_v1/
EXTRA_CFLAGS += -Idrivers/staging/android
EXTRA_CFLAGS += -Iarch/arm/mm
