1. make .so, need assign variable VM_LINUX_ROOT & DTV_LINUX_MAK_ROOT
	 make VM_LINUX_ROOT=/proj/mtk40117/git_view_edb_sqlite/apollo DTV_LINUX_MAK_ROOT=/proj/mtk40117/git_view_edb_sqlite/apollo/linux_mts/mak
	 
2. new folder in oss\library\gnuarm-4.8.2_neon_ca9\sqlite\3.17.0

3. oss/source/mak/oss_version.mak
	 change correct version string


vim ./dtv_lib/customer_p2.mak
../../../oss/library/gnuarm-4.8.2_neon_ca9/sqlite/3.17.0/include/sqlite3.h

