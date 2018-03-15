
#include "star.h"
#include "star_regUtil.h"
#include "star_procfs.h"

static struct star_procfs star_proc;

static struct phyAdjustItem phyAdjustTbl [] = {
	{"slewRate", vStarSetSlewRate, "Adjust Rise/Fall Time,level is in 0~6"},
	{"100mAmp", vSet100Amp, "Adjust 100M Ampitude,level is in 0~15"},
	{"10mAmp", vStarSet10MAmp, "Adjust 10M Ampitude,level is in 0~3"},
	{"outputBias", vStarSetOutputBias, "Adjust Output Bias,level is in 0~3"},
	{"inputBias", vStarSetInputBias, "Adjust Input Bias,level is in 0~3"},
	{"FeedBackCap", vStarSetFeedBackCap, "Adjust Feed Back Cap,level is in 0~3"},
	{"DACAMP", vSetDACAmp, "Adjust DACAMP,level is in 0~15"},
};

static struct phyTestItem phyTestTbl [] = {
	{"100mtx", vStar100mTx, "100M TX"},
	{"10mlp", vStar10mLinkPulse, "10M Link Pulse"},
	{"100mrl", vStar100mReturnLoss, "100M Return Loss"},
	{"10mpr", vStar10mPseudoRandom, "10M Pseudo Random"},
	{"10mhm", vStar10mHarmonic, "10M Harmonic"},
};

static ssize_t proc_phy_read_ops(struct file *file, char __user *buf,
										size_t count, loff_t *ppos)
{
	int i = 0;

	STAR_MSG(STAR_ERR,
		"\nThe following is used to generate signals for PHY Compliance test\n\r");
	for (i = 0; i < (ARRAY_SIZE(phyTestTbl)); i++){
		STAR_MSG(STAR_ERR,
		"%s:\t\t\t echo test %s > phy \n\r",
			phyTestTbl[i].helpMsg, phyTestTbl[i].name);
	}

	STAR_MSG(STAR_ERR,
		"If PHY Compliance test fail, you can use the following cmd to adjust phy"
		" and find the best settings\n\r");
	for (i = 0; i < (ARRAY_SIZE(phyAdjustTbl)); i++){
		STAR_MSG(STAR_ERR,
			"%s:\t\t\t echo adjust %s [level] > phy \n\r",
			phyAdjustTbl[i].helpMsg, phyAdjustTbl[i].name);
	}

    return 0;
}
static bool str_cmp_seq(char **buf, const char * substr)
{
	size_t len = strlen(substr);
	if (!strncmp(*buf, substr, len)) {
		*buf += len + 1;
		return true;
	} else {
		return false;
	}
}

static bool str_cmp_get_val(char * buf,
		const char * substr, u32 *pvalue)
{
	size_t len = strlen(substr);

	return (!strncmp(buf, substr, len) && (1 == sscanf(buf+len, "%d", pvalue)));
}

static struct net_device *star_get_net_device(void)
{
	if (!star_proc.ndev)
		star_proc.ndev = dev_get_by_name(&init_net, "eth0");

	return star_proc.ndev;
}

static void star_put_net_device(void)
{
	if (!star_proc.ndev)
		return;

	dev_put(star_proc.ndev);
}

static ssize_t proc_phy_write_ops(struct file *file,
										const char __user *buffer,
					                  size_t count, loff_t *pos)
{
	char *buf, *tmp;
	int value = 0, i = 0;
	struct net_device *dev;
	StarPrivate *starPrv;
	StarDev *starDev;

	tmp = kmalloc(count + 1, GFP_KERNEL);
	buf = tmp;
	if (copy_from_user(buf, buffer, count))
	        return -EFAULT;
	buf[count] = '\0';

	dev = star_get_net_device();
	if (!dev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	if (str_cmp_seq(&buf, "test")) {
		for (i = 0; i < (ARRAY_SIZE(phyTestTbl)); i++){
			if (str_cmp_seq(&buf, phyTestTbl[i].name)){
				STAR_MSG(STAR_ERR ,"Test %s \n\r", phyTestTbl[i].name);
				phyTestTbl[i].func(starDev);
				break;
			}
		}
	} else if (str_cmp_seq(&buf, "adjust")){
		for (i = 0; i < (ARRAY_SIZE(phyAdjustTbl)); i++){
			if (str_cmp_get_val(buf, phyAdjustTbl[i].name, &value)){
				STAR_MSG(STAR_ERR ,"Set %s as %d\n\r",
						phyAdjustTbl[i].name, value);
				phyAdjustTbl[i].func(starDev, value);
				break;
			}
		}
	}

	kfree(tmp);
	return count;
}

static const struct file_operations star_phy_ops = {
    .read = proc_phy_read_ops,
    .write = proc_phy_write_ops,
};

static ssize_t proc_phy_reg_read(struct file *file, char __user *buf,
										size_t count, loff_t *ppos)
{
	STAR_MSG(STAR_ERR, "read phy register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo rp reg_addr > phyreg\n");

	STAR_MSG(STAR_ERR, "write phy register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo wp reg_addr value > phyreg\n");

    return 0;
}

static ssize_t proc_reg_write(struct file *file,
										const char __user *buffer,
										size_t count, loff_t *pos)
{
	char *buf, *tmp;
	u16 phy_val;
	u32 i, mac_val, len = 0, address = 0, value = 0;
	struct net_device *dev;
	StarPrivate *starPrv;
	StarDev *starDev;

	tmp = kmalloc(count + 1, GFP_KERNEL);
	buf = tmp;
	if (copy_from_user(buf, buffer, count))
	        return -EFAULT;
	buf[count] = '\0';

	dev = star_get_net_device();
	if (!dev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	if (str_cmp_seq(&buf, "rp")) {
		if (1 == sscanf(buf, "%x", &address)) {
			STAR_MSG(STAR_ERR, "address(0x%x):0x%x\n",
				address, StarMdcMdioRead(starDev,starPrv->phy_addr,address));
		} else {
			STAR_MSG(STAR_ERR,"sscanf rp(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "wp")){
		if (2 == sscanf(buf, "%x %x", &address, &value)) {
			phy_val = StarMdcMdioRead(starDev,starPrv->phy_addr,address);
			StarMdcMdioWrite(starDev, starPrv->phy_addr, address, (u16)value);
			STAR_MSG(STAR_ERR, "0x%x: 0x%x --> 0x%x!\n",
					address, phy_val,
					StarMdcMdioRead(starDev,starPrv->phy_addr,address));
		} else {
			STAR_MSG(STAR_ERR,"sscanf wp(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "rr")) {
		if (2 == sscanf(buf, "%x %x", &address, &len)) {
			for (i = 0; i < len / 4; i++) {
				STAR_MSG(STAR_ERR, "%p:\t%08x\t%08x\t%08x\t%08x\t\n",
							starDev->base + address + i * 16,
							StarGetReg(starDev->base + address + i * 16),
							StarGetReg(starDev->base + address + i * 16 + 4),
							StarGetReg(starDev->base + address + i * 16 + 8),
							StarGetReg(starDev->base + address + i * 16 + 12));
			}
		} else {
			STAR_MSG(STAR_ERR,"sscanf rr(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "wr")){
		if (2 == sscanf(buf, "%x %x", &address, &value)) {
			mac_val = StarGetReg(starDev->base + address);
			StarSetReg(starDev->base + address, value);
			STAR_MSG(STAR_ERR, "%p: %08x --> %08x!\n",
						starDev->base + address,
						mac_val,
						StarGetReg(starDev->base + address));
		} else {
			STAR_MSG(STAR_ERR,"sscanf wr(%s) error\n", buf);
		}
	} else {
		STAR_MSG(STAR_ERR,"wrong arg:%s\n", buf);
	}

	kfree(tmp);
	return count;
}

static const struct file_operations star_phy_reg_ops = {
    .read = proc_phy_reg_read,
    .write = proc_reg_write,
};

static ssize_t proc_mac_reg_read(struct file *file, char __user *buf,
										size_t count, loff_t *ppos)
{
	STAR_MSG(STAR_ERR, "read MAC register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo rr reg_addr len > macreg\n");

	STAR_MSG(STAR_ERR, "write MAC register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo wr reg_addr value > macreg\n");

    return 0;
}

static const struct file_operations star_mac_reg_ops = {
    .read = proc_mac_reg_read,
    .write = proc_reg_write,
};


static ssize_t proc_dump_net_stat(struct file *file,
					char __user *buf, size_t count, loff_t *ppos)
{
	struct net_device *ndev;
	StarPrivate *starPrv;
	StarDev *starDev;

	ndev = star_get_net_device();
	if (!ndev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	STAR_MSG(STAR_ERR, "\n");
	STAR_MSG(STAR_ERR, "rx_packets	=%lu  < total packets received	> \n",
			starDev->stats.rx_packets);
	STAR_MSG(STAR_ERR, "tx_packets	=%lu  < total packets transmitted	> \n",
			starDev->stats.tx_packets);
	STAR_MSG(STAR_ERR, "rx_bytes	=%lu  < total bytes received	> \n",
			starDev->stats.rx_bytes);
	STAR_MSG(STAR_ERR, "tx_bytes	=%lu  < total bytes transmitted > \n",
			starDev->stats.tx_bytes);
	STAR_MSG(STAR_ERR, "rx_errors;	=%lu  < bad packets received	> \n",
			starDev->stats.rx_errors);
	STAR_MSG(STAR_ERR, "tx_errors;	=%lu  < packet transmit problems > \n",
			starDev->stats.tx_errors);
	STAR_MSG(STAR_ERR, "rx_crc_errors =%lu  < recved pkt with crc error > \n",
			starDev->stats.rx_crc_errors);
	STAR_MSG(STAR_ERR, "\n");
	STAR_MSG(STAR_ERR, "Use 'cat /proc/driver/star/stat' to dump net info\n");
	STAR_MSG(STAR_ERR, "Use 'echo clear > /proc/driver/star/stat' to clear net info\n");

	return 0;
}

static ssize_t proc_clear_net_stat(struct file *file,
										const char __user *buffer,
					                  size_t count, loff_t *pos)
{
	char *buf;
	struct net_device *ndev;
	StarPrivate *starPrv;
	StarDev *starDev;

	buf = kmalloc(count + 1, GFP_KERNEL);
	if (copy_from_user(buf, buffer, count))
	        return -EFAULT;
	buf[count] = '\0';

	ndev = star_get_net_device();
	if (!ndev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	if (!strncmp(buf, "clear", count-1))
		memset(&starDev->stats, 0, sizeof(struct net_device_stats));
	else
		STAR_MSG(STAR_ERR, "fail to clear stat, buf:%s\n", buf);

	kfree(buf);

	return count;
}

static const struct file_operations star_net_status_ops = {
    .read = proc_dump_net_stat,
    .write = proc_clear_net_stat,
};

static struct star_proc_file star_file_tbl[] = {
	{"phy", &star_phy_ops},
	{"phyreg", &star_phy_reg_ops},
	{"macreg", &star_mac_reg_ops},
	{"stat", &star_net_status_ops},
};

int star_init_procfs(void)
{
	int i;

	STAR_MSG(STAR_ERR, "%s entered\n", __FUNCTION__);
	star_proc.root = proc_mkdir("driver/star", NULL);
	if(!star_proc.root) {
		STAR_MSG(STAR_ERR, "star_proc_dir create failed\n" );
		return -1;
	}

	star_proc.entry = kmalloc(ARRAY_SIZE(star_file_tbl) *
								sizeof(struct star_proc_file), GFP_KERNEL);
	for (i = 0 ; i < ARRAY_SIZE(star_file_tbl); i ++) {
		star_proc.entry[i] = proc_create(star_file_tbl[i].name,
			0755, star_proc.root, star_file_tbl[i].fops);
		if (!star_proc.entry[i]) {
			STAR_MSG(STAR_ERR, "%s create failed\n", star_file_tbl[i].name);
			return -1;
		}
	}

	return 0;
}

void star_exit_procfs(void)
{
	int i;

	STAR_MSG(STAR_ERR, "%s entered\n", __FUNCTION__);
	for (i = 0 ; i < ARRAY_SIZE(star_file_tbl); i ++)
		remove_proc_entry(star_file_tbl[i].name, star_proc.root);

	kfree(star_proc.entry);
	remove_proc_entry("driver/star", NULL); 
	star_put_net_device();
}

