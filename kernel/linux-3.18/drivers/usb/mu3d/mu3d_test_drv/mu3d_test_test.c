#include <linux/init.h>
#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include <linux/kernel.h>       /* printk() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
//#include <linux/pci.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>
#include <linux/usb/ch9.h>
#include <asm/uaccess.h>
#include "mu3d_test_test.h"
#include "mu3d_test_unified.h"

#include <linux/usb.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/usb/hcd.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>



struct file_operations mu3d_test_fops;


////////////////////////////////////////////////////////////////////////////

#define MU3D_CLI_MAGIC	12345
#define IOCTL_READ _IOR(MU3D_CLI_MAGIC, 0, int)
#define IOCTL_WRITE _IOW(MU3D_CLI_MAGIC, 1, int)

#define BUF_SIZE 200
#define MAX_ARG_SIZE 20

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	char name[256];
	int (*cb_func)(int argc, char** argv);
} CMD_TBL_T;

CMD_TBL_T _arPCmdTbl_mu3d[]=
{
	{"auto.dev", &TS_AUTO_TEST},
	{"auto.u3i", &u3init},
	{"auto.u3w", &u3w},
	{"auto.u3r", &u3r},
	{"auto.u3d", &U3D_Phy_Cfg_Cmd},
	{"auto.link", &u3d_linkup},
	{"auto.eyeinit", &dbg_phy_eyeinit},	
	{"auto.eyescan", &dbg_phy_eyescan},
	#ifdef SUPPORT_OTG
	{"auto.stop", &TS_AUTO_TEST_STOP},	
	{"auto.otg", &otg_top},
	#endif
	{"", NULL}
};

////////////////////////////////////////////////////////////////////////////

char wr_buf_mu3d[BUF_SIZE];
char rd_buf_mu3d[BUF_SIZE] = "this is a test";

////////////////////////////////////////////////////////////////////////////

int call_function_mu3d(char *buf)
{
	int i;
	int argc;
	char *argv[MAX_ARG_SIZE];

	argc = 0;
	do
	{
		argv[argc] = strsep(&buf, " ");
		printk(KERN_DEBUG "[%d] %s\r\n", argc, argv[argc]);
		argc++;
	} while (buf);

	for (i = 0; i < sizeof(_arPCmdTbl_mu3d)/sizeof(CMD_TBL_T); i++)
	{
		if ((!strcmp(_arPCmdTbl_mu3d[i].name, argv[0])) && (_arPCmdTbl_mu3d[i].cb_func != NULL))
			return _arPCmdTbl_mu3d[i].cb_func(argc, argv);
	}

	return -1;
}


struct dentry *mu3d_debug_root;
EXPORT_SYMBOL_GPL(mu3d_debug_root);

static struct dentry *mu3d_test;

static int mu3d_debugfs_init(void)
{
	mu3d_debug_root = debugfs_create_dir("mu3d", NULL);
	if (!mu3d_debug_root)
		return -ENOENT;

	mu3d_test = debugfs_create_file("mu3d_test", 0644,
						mu3d_debug_root, NULL,
						&mu3d_test_fops);
	if (!mu3d_test) {
		debugfs_remove(mu3d_debug_root);
		mu3d_debug_root = NULL;
		return -ENOENT;
	}

	return 0;
}

static void usb_debugfs_cleanup(void)
{
	debugfs_remove(mu3d_test);
	debugfs_remove(mu3d_debug_root);
}	

static int mu3d_test_open(struct inode *inode, struct file *file)
{

    printk(KERN_DEBUG "xhci_mtk_test open: successful\n");
    return 0;
}

static int mu3d_test_release(struct inode *inode, struct file *file)
{

    printk(KERN_DEBUG "xhci_mtk_test release: successful\n");
    return 0;
}

static ssize_t mu3d_test_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{

    printk(KERN_DEBUG "xhci_mtk_test read: returning zero bytes\n");
    return 0;
}

static ssize_t mu3d_test_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{
	int retval;
	memcpy(wr_buf_mu3d, buf, count);
	if (wr_buf_mu3d[count-1] == '\n')
		wr_buf_mu3d[count-1] = '\0';
	
	retval = 0;

	retval = call_function_mu3d(wr_buf_mu3d);
	if (retval == -ENOSYS){
		printk(KERN_ERR "TEST NON_EXIST\n");
		return -1;
	} else if (retval == RET_FAIL) {
		printk(KERN_ERR "TEST FAIL\n");
	}
	else if (retval == RET_SUCCESS)
		printk(KERN_ERR "TEST PASS\n");
	else 
		printk(KERN_ERR "TEST FAIL: INVALID RET %d\n", retval);
	printk(KERN_ERR "%s", wr_buf_mu3d);
	return count ;
}


long mu3d_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    int len = BUF_SIZE;

	switch (cmd) {
		case IOCTL_READ:
			len = copy_to_user((char *) arg, rd_buf_mu3d, len);
			printk("IOCTL_READ: %s\r\n", rd_buf_mu3d);
			break;
		case IOCTL_WRITE:
			len = copy_from_user(wr_buf_mu3d, (char *) arg, len);
			printk("IOCTL_WRITE: %s\r\n", wr_buf_mu3d);

			//invoke function
			return call_function_mu3d(wr_buf_mu3d);
			break;
		default:
			return -ENOTTY;
	}

	return len;
}

struct file_operations mu3d_test_fops = {
    owner:   THIS_MODULE,
    read:    mu3d_test_read,
    write:   mu3d_test_write,
    unlocked_ioctl:   mu3d_test_ioctl,
    open:    mu3d_test_open,
    release: mu3d_test_release,
};


static int __init mu3d_test_init(void)
{
	int retval = 0;
	printk(KERN_DEBUG "xchi_mtk_test Init\n");
	retval = register_chrdev(MU3D_TEST_MAJOR, DEVICE_NAME, &mu3d_test_fops);
	if(retval < 0)
	{
		printk(KERN_DEBUG "xchi_mtk_test Init failed, %d\n", retval);
		goto fail;
	}
	retval = mu3d_debugfs_init();
	if(retval < 0)
	{
		printk(KERN_DEBUG "mu3h_debugfs init failed, %d\n", retval);
		goto fail;
	}
	
	return 0;
	fail:
		return retval;
}
module_init(mu3d_test_init);

static void __exit mu3d_test_cleanup(void)
{
	printk(KERN_DEBUG "mu3d_test End\n");
	unregister_chrdev(MU3D_TEST_MAJOR, DEVICE_NAME);
	usb_debugfs_cleanup();
	
}
module_exit(mu3d_test_cleanup);

MODULE_LICENSE("GPL");
