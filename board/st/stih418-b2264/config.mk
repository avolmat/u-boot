# SPDX-License-Identifier: GPL-2.0+
#
# Author: Alain VOLMAT <avolmat@me.com>

# Add link again libbtfmw library
#

ifeq ($(CONFIG_SPL_BUILD),y)
ifneq ($(CONFIG_TARGET_STIH418_B2264_LIBBTFMW_PATH),)
PLATFORM_LIBS += $(CONFIG_TARGET_STIH418_B2264_LIBBTFMW_PATH)/libbtfmw-h418.a

# libbtfmw is built with hard-float hence we need to remove -msoft-float
PLATFORM_RELFLAGS := $(filter-out $(call cc-option, -msoft-float), $(PLATFORM_RELFLAGS))
endif
endif
