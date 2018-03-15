##########################################################################
#
#  Copyright (c) 2004, CrystalMedia Technology, Inc.
#  All rights reserved
#
#  Unauthorized use, practice, perform, copy, distribution, reproduction,
#  or disclosure of this information in whole or in part is prohibited.
###########################################################################
# $RCSfile: $
# $Revision: #1 $
# $Date: 2012/06/19 $
# $Author:  $
# $MD5HEX:  $
#
# Description:
#
#############################################################################

CROSS_COMPILE =		/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-
BUILD_DIR          ?= $(shell pwd)

export CC     = gcc
export AR     = ar
export AS     = as
export STRIP  = strip
export RANLIB = ranlib

TAR         = /bin/tar
RM          = /bin/rm
RM_FLAG     = -fr
CP          = /bin/cp
CP_FLAG     = -f
CP_ALL_FLAG = -rf
MAKE        = make
MKDIR       = mkdir
MKDIR_FLAG  = -p 
MV          = /bin/mv

ifneq (,$(findstring $(TOOL_CHAIN), 4.9.1 4.9.2))
VFP_CFLAGS  =-mfloat-abi=hard -mfpu=vfp
else
VFP_CFLAGS  =-mfloat-abi=softfp -mfpu=vfp
endif

TOYBOX_VER               ?= 0.4.0
TOYBOX_FILE              ?= toybox-$(TOYBOX_VER).tar.bz2
TOYBOX_FOLDER            ?= toybox-$(TOYBOX_VER)
TOYBOX_DECOMPRESS_FLAG   ?= xjf
TOYBOX_CFG_FILE          ?= toybox-config-android-default
#TOYBOX_PATCH_FOLDER      ?= toybox-$(TOYBOX_VER)_patch
SUBARCH                   ?= arm

BUILD_TMPDIR = $(BUILD_DIR)/$(notdir $(TOYBOX_CFG_FILE)build)

define check_build_dir
if [ ! -d $(BUILD_TMPDIR) ]; then mkdir -p $(BUILD_TMPDIR); fi
endef

all_phony := toybox

.PHONY: $(all_phony) $(addprefix clean_,$(all_phony))

all: $(all_phony)

clean_all: $(addprefix clean_,$(all_phony))
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)
	$(RM) $(RM_FLAG) $(_PREFIX)
	
clean_TOYBOX:
	$(RM) $(RM_FLAG) $(BUILD_TMPDIR)

toybox:
ifeq "$(SUBARCH)" ""
	@echo "SUBARCH = NULL(could be i386/arm/sparc64/s390/parisc/powerpc/mips), e.g. 'make TOYBOX SUBARCH=arm', exit now!"
	@exit 1
endif
	@$(check_build_dir)
	$(TAR) $(TOYBOX_DECOMPRESS_FLAG) $(TOYBOX_FILE) -C $(BUILD_TMPDIR)
	$(CP) $(CP_FLAG) $(TOYBOX_CFG_FILE) $(BUILD_TMPDIR)/$(TOYBOX_FOLDER)/.config
#	$(CP) $(CP_ALL_FLAG) $(TOYBOX_PATCH_FOLDER)/* $(BUILD_TMPDIR)/$(TOYBOX_FOLDER)/
	$(if $(filter $(ENABLE_VFP),true),CPPFLAGS="$(VFP_CFLAGS)" CFLAGS="$(VFP_CFLAGS)",) \
	$(MAKE) -j 20 all     -C $(BUILD_TMPDIR)/$(TOYBOX_FOLDER) CFLAGS+="--static" SUBARCH=$(SUBARCH) CROSS_COMPILE=$(CROSS_COMPILE)

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/toybox_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/toybox_INFO.txt

