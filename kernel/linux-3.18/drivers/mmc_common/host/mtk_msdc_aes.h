#ifndef _MTK_MSDC_AES_H_
#define _MTK_MSDC_AES_H_

#include <mtk_msdc.h>


void emmc_aes_init(struct msdc_host *host);
void emmc_aes_encrypt(struct msdc_host *host,struct mmc_command	*cmd,struct mmc_data *data);
void emmc_aes_decrypt(struct msdc_host *host,struct mmc_command	*cmd,struct mmc_data *data);
void emmc_aes_close(struct msdc_host *host);
int msdc_host_aes_dec_switch(struct msdc_host *host);
int msdc_host_aes_enc_switch(struct msdc_host *host);
#endif
