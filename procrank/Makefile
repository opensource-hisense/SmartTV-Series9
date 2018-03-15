#
# Copyright (C) 2015 by Chris Simmonds <chris@2net.co.uk>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

#-----------------------------------------------------------------------------
# Copyright (c) 2010, MediaTek Inc.
# All rights reserved.
#
# Unauthorized use, practice, perform, copy, distribution, reproduction,
# or disclosure of this information in whole or in part is prohibited.
#-----------------------------------------------------------------------------
# $RCSfile:  $
# $Revision:
# $Date:
# $Author:  $
# $CCRevision:  $
# $SWAuthor:  $
# $MD5HEX:  $
#
# Description:
#        A set of files describing the framebuffer interface and core types.
#---------------------------------------------------------------------------*/

ifndef VM_LINUX_ROOT
VM_LINUX_ROOT := $(word 1, $(subst /apollo/,/apollo /, $(shell pwd)))
endif

# For MT5396 above (old)
#ENABLE_CA9 ?= true

# Select tool chain and library
#ifeq "$(ENABLE_VFP)" "true"
#    THIRDPARTY_LIB_ROOT=$(VM_LINUX_ROOT)/third_party/library/gnuarm-$(TOOL_CHAIN)_vfp
#else
#    THIRDPARTY_LIB_ROOT=$(VM_LINUX_ROOT)/third_party/library/gnuarm-$(TOOL_CHAIN)
#endif

include $(VM_LINUX_ROOT)/oss/source/mak/target.mak


PROGRAM = procrank
THIS_DIR := $(shell pwd)

# LOCAL_CFLAGS := -Wall -Wextra -Wformat=2 -Werror
LOCAL_CFLAGS := -Wall

libpagemap/libpagemap.a:
	make -C libpagemap

$(PROGRAM): $(PROGRAM).c libpagemap/libpagemap.a
	$(CC) $(LOCAL_CFLAGS) $(PROGRAM).c -Ilibpagemap/include -Llibpagemap -lpagemap -o procrank


.PHONY: all install clean

all: $(PROGRAM)
	@echo 'Create procrank library!'


clean:
	-rm -f $(PROGRAM)
	make -C libpagemap clean


install:
	echo "Install procrank . OSS_LIB_ROOT=$(OSS_LIB_ROOT) THIS_DIR=$(THIS_DIR)"
	rm -rf $(OSS_LIB_ROOT)/procrank
	mkdir -p $(OSS_LIB_ROOT)/procrank/$(PROCRANK_VERSION)
	cp -r $(THIS_DIR)/procrank $(OSS_LIB_ROOT)/procrank/$(PROCRANK_VERSION)
	@echo 'Release procrank library finished.'

