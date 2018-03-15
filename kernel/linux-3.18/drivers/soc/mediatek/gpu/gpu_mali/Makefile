#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

ifeq "$(LINUX_GPU_SUPPORT)" "true"
	ifeq  ("$(findstring MALI450, $(GPU_SETTING_VER))", "MALI450")
	MTK_GPU_DIR=mali_utgard_linux/r4p1/
	else ifeq ("$(findstring MALI_G71, $(GPU_SETTING_VER))", "MALI_G71")
	MTK_GPU_DIR=mali_bifrost_linux/r3p0/drivers/
	else
	MTK_GPU_DIR=mali_midgard_linux/r7p0/drivers/
	endif
else
	ifeq  ("$(findstring MALI450, $(GPU_SETTING_VER))", "MALI450")
	MTK_GPU_DIR=mali_utgard_android/r6p1/
	else ifeq ("$(findstring MALI_G71, $(GPU_SETTING_VER))", "MALI_G71")
	MTK_GPU_DIR=mali_bifrost_android/r5p0/drivers/
	else
	MTK_GPU_DIR=mali_midgard_android/r14p0/drivers/
	endif
endif
export DDK_SRC_DIR=$(src)/$(MTK_GPU_DIR)
$(info *GPU_SETTING_VER 1 = $(GPU_SETTING_VER) '$(MTK_GPU_OS)' " $(subst -, ,$(GPU_SETTING_VER))" "$(findstring MALI450, $(GPU_SETTING_VER))" "$(MTK_GPU_DIR)")

$(info *GPU_SETTING_VER 1 = $(GPU_SETTING_VER) '$(MTK_GPU_OS)' " $(subst -, ,$(GPU_SETTING_VER))" "$(findstring MALI450, $(GPU_SETTING_VER))" "$(MTK_GPU_DIR)" "$(DDK_SRC_DIR)")
obj-y += $(MTK_GPU_DIR)
obj-$(CONFIG_GATOR) += gator/

