#ifndef __BTUT_GAP_IF_H__
#define __BTUT_GAP_IF_H__

#define CMD_KEY_GAP     "GAP"

extern int default_scan_mode;

int btut_gap_init();
int btut_gap_deinit();
int btut_gap_deinit_profiles();

#endif /* __BTUT_GAP_IF_H__ */
