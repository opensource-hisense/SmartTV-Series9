#
# arch/arm64/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2012, ARM Ltd.
# Author: Will Deacon <will.deacon@arm.com>
#
# Based on the ia64 boot/Makefile.
#

include $(srctree)/arch/arm64/boot/dts/Makefile

UIMAGE_COMPRESSION = gzip
UIMAGE_ARCH := arm
# Note: the following conditions must always be true:
#   ZRELADDR == virt_to_phys(PAGE_OFFSET + TEXT_OFFSET)
#   PARAMS_PHYS must be within 4MB of ZRELADDR
#   INITRD_PHYS must be in RAM
# ZRELADDR    := $(zreladdr-y) # arm32 case, refer to arch/arm/mach-xxx/Makefile.boot
ifeq ($(CONFIG_ARCH_MT5893),y)
ZRELADDR    := 0x40080000
else
ZRELADDR    := 0x80000
endif
ZRELADDR0    := $(shell printf "%d" $(ZRELADDR))
ZRELADDR1    := $(shell expr $(ZRELADDR0) + 64)
ZRELADDRE    := $(shell printf "0x%.x" $(ZRELADDR1))

export ZRELADDR
targets := Image Image.gz uImage

DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif
DTB_OBJS := $(addprefix $(obj)/dts/,$(DTB_LIST))

$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)
	@$(kecho) '  Kernel: $@ is ready'

$(obj)/uImage: UIMAGE_ENTRYADDR=$(ZRELADDRE)
$(obj)/uImage-dtb: UIMAGE_ENTRYADDR=$(ZRELADDRE)

ifneq ($(LOADADDR),)
  UIMAGE_LOADADDR=$(LOADADDR)
else
  ifeq ($(CONFIG_ZBOOT_ROM),y)
    UIMAGE_LOADADDR=$(CONFIG_ZBOOT_ROM_TEXT)
  else
    UIMAGE_LOADADDR=$(ZRELADDR)
  endif
endif

check_for_multiple_loadaddr = \
if [ $(words $(UIMAGE_LOADADDR)) -ne 1 ]; then \
	echo 'multiple (or no) load addresses: $(UIMAGE_LOADADDR)'; \
	echo 'This is incompatible with uImages'; \
	echo 'Specify LOADADDR on the commandline to build an uImage'; \
	false; \
fi

$(obj)/Image.gz: $(obj)/Image FORCE
	$(call if_changed,gzip)

$(obj)/zImage:	$(obj)/Image.gz FORCE
	$(call if_changed,uimage)
	
$(obj)/Image.gz-dtb: $(obj)/Image.gz $(DTB_OBJS) FORCE
	$(call if_changed,cat)		
ifndef CONFIG_ANDROID
$(obj)/uImage:	$(obj)/zImage $(DTB_OBJS) FORCE
	$(call if_changed,cat)
	
$(obj)/uImage-dtb:	$(obj)/uImage FORCE
	$(call if_changed,uimage)
else
$(obj)/uImage-dtb:	$(obj)/zImage $(DTB_OBJS) FORCE
	$(call if_changed,cat)
endif

install: $(obj)/Image
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall: $(obj)/Image.gz
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image.gz System.map "$(INSTALL_PATH)"
