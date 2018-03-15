
export TOOL_CHAIN           ?= 4.8.2
export ENABLE_VFP           ?= false
export ENABLE_CA9           ?= true

export VM_LINUX_ROOT        ?= $(word 1, $(subst /apollo/,/apollo /, $(PWD)))
export LINUX_ROOT           ?= $(VM_LINUX_ROOT)
export DTV_LINUX_MAK_ROOT   ?= $(VM_LINUX_ROOT)/linux_mts/build/mak
export OSS_ROOT      ?= $(VM_LINUX_ROOT)/oss
export OSS_SRC_ROOT  ?= $(OSS_ROOT)/source
export MAK_ROOT             := $(OSS_SRC_ROOT)/mak

include $(MAK_ROOT)/target.mak

export OSS_LIB_ROOT  ?= $(OSS_ROOT)/library/gnuarm-$(TOOL_CHAIN)$(VFP_SUFFIX)

ASFLAGS= -mips2

# The two "global" cross-tool variables
CC= $(CROSS_COMPILE)gcc
AR= $(CROSS_COMPILE)ar cr
INSTALL= install
MV= mv
CP= cp
CHMOD= chmod
CFLAGS= -fPIC -g

LIB_INSTALL_DIR= $(OSS_LIB_ROOT)/electric-fence/$(EFENCE_VERSION)
MAN_INSTALL_DIR= $(OSS_LIB_ROOT)/electric-fence/$(EFENCE_VERSION)

PACKAGE_SOURCE= README libefence.3 Makefile efence.h \
	efence.c page.c print.c eftest.c tstheap.c CHANGES COPYING

# Un-comment the following if you are running HP/UX.
# CFLAGS= -Aa -g -D_HPUX_SOURCE -DPAGE_PROTECTION_VIOLATED_SIGNAL=SIGBUS

# Un-comment the following if you are running AIX. This makes sure you won't
# get the shared-library malloc() rather than the Electric Fence malloc().
# COMPILE THE PROGRAMS YOU ARE DEBUGGING WITH THESE FLAGS, TOO.
# CFLAGS= -g -bnso -bnodelcsect -bI:/lib/syscalls.exp

# Un-comment the following if you are running SunOS 4.X
# Note the definition of PAGE_PROTECTION_VIOLATED_SIGNAL. This may vary
# depend on what version of Sun hardware you have.
# You'll probably have to link the program you are debugging with -Bstatic
# as well if using Sun's compiler, -static if using GCC.
# CFLAGS= -g -Bstatic -DPAGE_PROTECTION_VIOLATED_SIGNAL=SIGBUS

OBJECTS= efence.o page.o print.o

all:	libefence.a tstheap libefence.so


install: libefence.a libefence.3
	$(CP) libefence.a $(LIB_INSTALL_DIR)
	$(CHMOD) 644 $(LIB_INSTALL_DIR)/libefence.a
	$(INSTALL) libefence.3 $(MAN_INSTALL_DIR)/libefence.3
	$(CHMOD) 644 $(MAN_INSTALL_DIR)/libefence.3

clean:
	- rm -f $(OBJECTS) tstheap.o eftest.o tstheap eftest libefence.a \
	 libefence.cat ElectricFence.shar

roff:
	nroff -man < libefence.3 > libefence.cat


ElectricFence.shar: $(PACKAGE_SOURCE)
	shar $(PACKAGE_SOURCE) > ElectricFence.shar

shar: ElectricFence.shar

libefence.a: $(OBJECTS)
	- rm -f libefence.a
	$(AR) libefence.a $(OBJECTS)

libefence.so: $(OBJECTS)
	- rm -f libefence.so
	$(CXX) -shared $(OBJECTS) -o $@

tstheap: libefence.a tstheap.o
	- rm -f tstheap
	$(CC) $(CFLAGS) tstheap.o libefence.a -o tstheap -lpthread

eftest: libefence.a eftest.o
	- rm -f eftest
	$(CC) $(CFLAGS) eftest.o libefence.a -o eftest -lpthread

$(OBJECTS) tstheap.o eftest.o: efence.h
