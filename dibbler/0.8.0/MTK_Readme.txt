dibbler 0.8.2 How to build
1. tar xvfz dibbler-0.8.2.tar.gz
2. cd dibbler-0.8.2
3. vi configure
4. goto line 16417, chagne "ac_cv_func_malloc_0_nonnull=no" to "ac_cv_func_malloc_0_nonnull=yes"
5. goto line 16484, change "ac_cv_func_realloc_0_nonnull=no" to "ac_cv_func_realloc_0_nonnull=yes"
6. cd bison++
7. vi configure
8. goto line 4865, change "ac_cv_func_malloc_0_nonnull=no" to "ac_cv_func_malloc_0_nonnull=yes"
9. cd ..
10. vi ./Port-linux/interface.c

11. move the following three lines in front of line "#include <linux/sockios.h>", and save the file "interface.c"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

12. use following command to configure, default will build 4.5.1 ca9 version
LD=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-g++ CXX=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-c++ CC=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-gcc AR=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-ar RANLIB=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-ranlib ./configure --host=arm-linux

13. "make" or "make -j 24"

14. "dibbler-client" will be generated in current directory






dibbler 0.8.0 How to build
1. tar xvfz dibbler-0.8.0-src.tar.gz
2. cd dibbler-0.8.0
3. vi ./Port-linux/interface.c
4. move the following three lines in front of line "#include <linux/sockios.h>", and save the file "interface.c"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

5 add the following two lines in fron of line "#ifdef MINGWBUILD",and save the file " ClntIfaceMgr/ClntIfaceIface.cpp"
#include<unistd.h>
#include<stdio.h>

6. use following command to configure, default will build 4.5.1 ca9 version
CHOST=arm-linux CXXLD=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-g++ CXX=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-c++ CC=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-gcc AR=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-ar RANLIB=/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-ranlib make client

7. "dibbler-client" will be generated in current directory




Following is test command
cd /
cp -a /var/ /tmp
cp -a /etc/ /tmp
mount /tmp/var /var
mount /tmp/etc /etc
cd /var
mkdir lib
cd lib
mkdir dibbler
cd /etc
mkdir dibbler
cd /mnt/usb/sda1/dhcp
cp client.conf /etc/dibbler
./dibbler-client run &
