##############################################################################
# No Warranty                                                                #
# Except as may be otherwise agreed to in writing, no warranties of any      #
# kind, whether express or implied, are given by MTK with respect to any MTK #
# Deliverables or any use thereof, and MTK Deliverables are provided on an   #
# "AS IS" basis.  MTK hereby expressly disclaims all such warranties,        #
# including any implied warranties of merchantability, non-infringement and  #
# fitness for a particular purpose and any warranties arising out of course  #
# of performance, course of dealing or usage of trade.  Parties further      #
# acknowledge that Company may, either presently and/or in the future,       #
# instruct MTK to assist it in the development and the implementation, in    #
# accordance with Company's designs, of certain softwares relating to        #
# Company's product(s) (the "Services").  Except as may be otherwise agreed  #
# to in writing, no warranties of any kind, whether express or implied, are  #
# given by MTK with respect to the Services provided, and the Services are   #
# provided on an "AS IS" basis.  Company further acknowledges that the       #
# Services may contain errors, that testing is important and Company is      #
# solely responsible for fully testing the Services and/or derivatives       #
# thereof before they are used, sublicensed or distributed.  Should there be #
# any third party action brought against MTK, arising out of or relating to  #
# the Services, Company agree to fully indemnify and hold MTK harmless.      #
# If the parties mutually agree to enter into or continue a business         #
# relationship or other arrangement, the terms and conditions set forth      #
# hereunder shall remain effective and, unless explicitly stated otherwise,  #
# shall prevail in the event of a conflict in the terms in any agreements    #
# entered into between the parties.                                          #
##############################################################################
###########################################################################
#
#  Copyright (c) 2004, CrystalMedia Technology, Inc.
#  All rights reserved
#
#  Unauthorized use, practice, perform, copy, distribution, reproduction,
#  or disclosure of this information in whole or in part is prohibited.
###########################################################################

# Copy from oss/source/mak/Makefile
VM_LINUX_ROOT ?= $(word 1, $(subst /vm_linux/,/vm_linux /, $(shell pwd)))
OSS_ROOT ?= $(VM_LINUX_ROOT)/oss

include $(OSS_ROOT)/../project_x/host.mak
include $(OSS_ROOT)/source/mak/target.mak
include $(OSS_ROOT)/source/mak/oss_version.mak

OSS_SRC_ROOT = $(OSS_ROOT)/source
LIB_SUB_ROOT  = library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)
OSS_LIB_ROOT = $(OSS_ROOT)/$(LIB_SUB_ROOT)

#
# Set initial values.
#
THIS_ROOT    := $(shell pwd)
PROJECT_ROOT := $(THIS_ROOT)

#
# Set warning error parse log/tools
#
export WARNING_LOG  = $(THIS_ROOT)/build.log
export ERROR_LOG    = $(THIS_ROOT)/build_fail.log
export CHK_ERR_WARN_SCRIPT = $(PROJECT_ROOT)/../mak/chk_warn_err.sh
export CHK_ERR_WARN_PARM = $(WARNING_LOG) $(ERROR_LOG)

#
# Set path to source obj library output path
#
export SOURCE_ROOT        ?= $(PROJECT_ROOT)/source
export SOURCE_LIB_ROOT    ?= $(PROJECT_ROOT)/lib
export OBJ_ROOT           ?= $(THIS_ROOT)/obj
export OUTPUT_ROOT        ?= $(THIS_ROOT)/output
export PREFIX              = $(OUTPUT_ROOT)
export FLPRJ_ROOT         := $(THIS_ROOT)

CROSS_HOST                ?= i686-pc-linux-gnu
CROSS_TARGET              ?= armv6z-mediatek-linux-gnueabi

ALSA_TAR_BALL             ?=alsa-lib-1.0.24.1.tar
ALSA_TAR_BALL_DIR          = alsa-lib-1.0.24.1
ALSA_UTIL_DIR              = alsa-utils-1.0.24.2

.PHONY : all src

all: init src install

util:
	cd $(FLPRJ_ROOT)/$(ALSA_UTIL_DIR) && \
	CC="$(CC)" CFLAGS="$(CC_FLAG)" LD=$(CROSS_COMPILE)ld AS="$(AS)" AR="$(AR)" STRIP="$(STRIP)" NM="$(NM)" RANLIB="$(RANLIB)" ./configure --prefix=$(PREFIX) --exec-prefix=$(PREFIX) --host=i686-pc-linux-gnu --target=armv6z-mediatek-linux-gnueabi --with-alsa-prefix=$(PREFIX)/lib --with-alsa-inc-prefix=$(PREFIX)/include --with-udev-rules-dir=$(PREFIX) --with-asound-state-dir=$(PREFIX) --disable-alsamixer --disable-python --disable-xmlto
	$(MAKE) -C $(FLPRJ_ROOT)/$(ALSA_UTIL_DIR) install; 
	
#CC="$(CC)" CFLAGS="$(CC_FLAG)" CXX="$(CXX)" CXXFLAGS="$(CC_FLAG)"  LD="$(LD)" CPP="$(CPP)" AS="$(AS)" AR="$(AR)" STRIP="$(STRIP)" NM="$(NM)" RANLIB="$(RANLIB)"
init:
	$(call if_dir_not_exist_fct,$(call path_cnvt,$(OUTPUT_ROOT)), $(MKDIR) $(MKDIR_FLAG) $(call path_cnvt,$(OUTPUT_ROOT));)
#	tar -xvf alsa-lib-1.0.24.1.tar
	cd $(FLPRJ_ROOT)/$(ALSA_TAR_BALL_DIR) && \
	CC="$(CC)" CFLAGS="$(CC_FLAG)" LD=$(CROSS_COMPILE)ld AS="$(AS)" AR="$(AR)" STRIP="$(STRIP)" NM="$(NM)" RANLIB="$(RANLIB)" ./configure --enable-shared=yes --enable-static=no --prefix=$(PREFIX) --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-configdir2=/basic/lib/alsa-lib-1.0.24.1/share/alsa --with-plugindir2=/basic/lib/alsa-lib-1.0.24.1/lib/alsa-lib --disable-python
#	cd $(FLPRJ_ROOT)/$(ALSA_TAR_BALL_DIR) && \
	CC="$(CC)" CFLAGS="$(CC_FLAG)" LD=$(CROSS_COMPILE)ld AS="$(AS)" AR="$(AR)" STRIP="$(STRIP)" NM="$(NM)" RANLIB="$(RANLIB)" ./configure --enable-shared=yes --enable-static=no --prefix=$(PREFIX) --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-configdir=/basic/lib/alsa-lib-1.0.24.1/share/alsa --with-plugindir=/basic/lib/alsa-lib-1.0.24.1/lib/alsa-lib
#	CC="$(CC)" CFLAGS="$(CC_FLAG)" CXX="$(CXX)" CXXFLAGS="$(CC_FLAG)" LD="$(CC)" AS="$(AS)" AR="$(AR)" STRIP="$(STRIP)" NM="$(NM)" RANLIB="$(RANLIB)" \
	./configure --enable-shared=yes --enable-static=no --prefix=$(PREFIX) --host=$(CROSS_HOST) --target=$(CROSS_TARGET)

src:
	$(MAKE) -C $(FLPRJ_ROOT)/$(ALSA_TAR_BALL_DIR) all;
	$(MAKE) -C $(FLPRJ_ROOT)/$(ALSA_TAR_BALL_DIR) install; 

install:
	@echo '@@@@@@@@Release $(ALSA_TAR_BALL_DIR) starting.@@@@@@@@'
	@mkdir -p $(OSS_LIB_ROOT)/alsa
	@mkdir -p $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)
	@echo 'cp -rf $(FLPRJ_ROOT)/output/* $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)'
	@cp -rf $(FLPRJ_ROOT)/output/* $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)
	@cp -rf $(FLPRJ_ROOT)/makefile.softlink  $(OSS_LIB_ROOT)/alsa/$(ALSA_VERSION)/lib/Makefile
	@echo '@@@@@@@@Release $(ALSA_TAR_BALL_DIR) finished.@@@@@@@@'

clean : clean_src

clean_src :
	$(RM) -r $(RM_FLAG) $(call path_cnvt,$(OUTPUT_ROOT))
	$(RM) -r $(RM_FLAG) $(call path_cnvt,$(SOURCE_LIB_ROOT))
	$(RM) -r $(RM_FLAG) $(call path_cnvt,$(OBJ_DIR)) build.log

help:
	@echo 'linuxq make BUILD_CFG=rel TOOL_CHAIN=4.5.1 ENABLE_CA9=true init src install 2>&1 |tee log'
	@echo '##################  debug variable #########################'
	@echo CC="$(CC)" 
	@echo CFLAGS="$(CC_FLAG)"
	@echo CXX="$(CXX)"
	@echo CXXFLAGS="$(CC_FLAG)" 
	@echo CPP="$(CPP)"
	@echo MAKE="$(MAKE)"
	@echo THIS_ROOT="$(THIS_ROOT)"
	@echo PROJECT_ROOT="$(PROJECT_ROOT)"
	@echo WARNING_LOG="$(WARNING_LOG)"
	@echo ERROR_LOG="$(ERROR_LOG)"
	@echo OUTPUT_ROOT="$(OUTPUT_ROOT)"
	@echo FLPRJ_ROOT="$(FLPRJ_ROOT)"
	@echo TOOL_CHAIN="$(TOOL_CHAIN)"
	@echo VFP_SUFFIX="$(VFP_SUFFIX)"
	@echo BUILD_CFG="$(BUILD_CFG)"
	@echo BUILD_DIR="$(BUILD_DIR)"
	@echo CROSS_COMPILE="$(CROSS_COMPILE)"
	@echo SOURCE_LIB_ROOT="$(SOURCE_LIB_ROOT)"
	@echo AS="$(AS)"
	@echo STRIP="$(STRIP)"
	@echo NM="$(NM)"
	@echo RANLIB="$(RANLIB)"
	@echo PREFIX="$(PREFIX)"
	@echo ALSA_UTIL_DIR="$(ALSA_UTIL_DIR)"
	@echo MAKE="$(MAKE)"
	@echo MKDIR="$(MKDIR)"
	@echo MKDIR_FLAG="$(MKDIR_FLAG)"
	@echo ALSA_TAR_BALL_DIR="$(ALSA_TAR_BALL_DIR)"
	@echo VM_LINUX_ROOT="$(VM_LINUX_ROOT)"
	@echo CHK_ERR_WARN_SCRIPT="$(CHK_ERR_WARN_SCRIPT)"
	@echo CHK_ERR_WARN_PARM="$(CHK_ERR_WARN_PARM)"
	@echo AR="$(AR)"
	@echo RM_FLAG="$(RM_FLAG)"
	@echo OBJ_DIR="$(OBJ_DIR)"
	@echo OSS_ROOT="$(OSS_ROOT)"
	@echo OSS_SRC_ROOT="$(OSS_SRC_ROOT)"
	@echo OSS_LIB_ROOT="$(OSS_LIB_ROOT)"