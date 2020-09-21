ifeq "$(findstring darwin,$(TARGET_DEVICE))" "darwin"
  include $(call all-subdir-makefiles)
endif
