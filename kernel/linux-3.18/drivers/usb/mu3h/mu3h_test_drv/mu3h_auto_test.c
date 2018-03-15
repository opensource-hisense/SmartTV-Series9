
#include <linux/kobject.h>

#include "mu3h_auto_test.h"
#include "mtk-test.h"
#include "mtk-test-lib.h"



struct mu3h_test_item mu3h_auto_test_items[] = {
#if 0

	{ "connect device", { "dev.init",
		                  NULL,
		                }
	},
		
    { "evaluate context", 
    	{ "slt.evalctx 256 512 1 1 0 0",
          "lb.config bulk out 1024 0 1 1 0 0",
          "slt.evalctx 32 512 0 1 0 0",
	      "hcd.cleanup",
	      NULL,
	    }
	},
#endif	
	{ "config endpoint using (1)DC,(2)add/drop endpoint concurrently",
		{ "dev.init",
	      "dev.configep 1 tx bulk 1024 0",
          "dev.configep 1 rx bulk 1024 0",
          "lb.config intr out 8 0 1 0",
          "lb.config intr in 8 0 1 1",
          "lb.deconfig all",
          "dbg.dbgslt",
          "lb.config isoc out 8 0 1 0",
          "lb.config isoc in 8 0 1 1",
          "lb.deconfig out 1 0",
          "lb.deconfig in 1 0",
          "lb.config bulk out 1024 0 1 0",
          "lb.config bulk in 1024 0 1 1",
          "lb.loop 65535 0 1 1 1",
          NULL,
        }
    },
	

	{"bulk transfer", 
		{ "auto.lbscan ss bulk",
		  NULL,
    	}
	},
	
	{"interrupt transfer",
	    { "auto.lbscan ss intr",
	      NULL,
		}
	},

	 
	{"isochronous transfer", 
		{ "auto.lbscan ss bulk",
		   NULL,
		}
	},
	 
	{"control transfer",
		{ "auto.lbctrl 0 0 ss",
		   NULL,
		}
	},
	{"Bulk TD(ISP+IOC)(loopback)",
		{ "auto.lbsgscan ss bulk",
		   NULL,
		}
	},
	{"Interrupt TD(ISP+IOC)(loopback)",
		{ "auto.lbsgscan ss intr",
		   NULL,
		}
	},
	{"High bandwidth Interrupt transfer (loopback)",
		{ "auto.lbscan ss intr true",
		   NULL,
		}
	},
	{"High bandwidth Isochronous transfer (loopback)",
		{ "auto.lbscan ss isoc true",
		   NULL,
		}
	},
	 
    {NULL,},
};
		
static ssize_t manu_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
//	return sprintf(buf, "%d\n", force_suspend);
    return 1;
}

static ssize_t manu_store(struct kobject *kobj, struct kobj_attribute *attr,
	        const char *buf, size_t n)
{
    int retval;
	char *ch;
	ch = kmalloc(n, GFP_KERNEL);
	memcpy(ch, buf, n);
	if (ch[n-1] == '\n')
		ch[n-1] = '\0';
	
	retval = 0;

	retval = call_function_mu3h(ch);
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
	kfree(ch);
	printk(KERN_ERR "%s", buf);
	return n ;
}

mu3h_attr(manu);	

static ssize_t auto_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
//	return sprintf(buf, "%d\n", force_suspend);
    return 1;
}

static ssize_t auto_store(struct kobject *kobj, struct kobj_attribute *attr,
	       const char *buf, size_t n)
{
    int retval;
	int i;
	struct mu3h_test_item *item;
	char *temp;
	char *ch;
	ch = (char *)buf;
	if (ch[n-1] == '\n')
		ch[n-1] = '\0';
	
	retval = 0;

	item = mu3h_auto_test_items;
	
    for (; item->name != NULL; item++){
		printk(KERN_ERR "\n########################################################################\n");
		printk(KERN_ERR " TEST ITEM:  %s :\n",item->name);
		printk(KERN_ERR "\n########################################################################\n");
		for (i=0; i< CLI_MAX; i++){			
			temp = item->command_line[i];
			if (temp == NULL){
				break;
			}
			printk(KERN_ERR "\n%s \n",temp);
		    retval = call_function_mu3h(temp);
		}		
		if (retval < 0){
			printk(KERN_ERR "\n************************************************************************\n");
			printk(KERN_ERR " FAILED: %s :\n",item->name);
			printk(KERN_ERR "\n************************************************************************\n\n");
			return -EINVAL;;
		}
		else{
			printk(KERN_ERR "\n************************************************************************\n");
			printk(KERN_ERR " SUCCESSFUL: %s :\n",item->name);
			printk(KERN_ERR "\n************************************************************************\n\n");
			}
			
	}

	return n ;
}

mu3h_attr(auto);



static struct attribute * mu3h_test[] = {
	&manu_attr.attr,
	&auto_attr.attr,
	NULL,
};

static struct attribute_group mu3h_attr_group = {
	.attrs = mu3h_test,
};



static int __init mu3h_creat_sysfs(void)
{
    int retval = 0;
	
	struct kobject *mu3h_kobj;
	
	mu3h_kobj = kobject_create_and_add("mu3h", NULL);
	if (!mu3h_kobj)
		return -ENOMEM;
	
	retval = sysfs_create_group(mu3h_kobj, &mu3h_attr_group);
	
	return retval;

}

module_init(mu3h_creat_sysfs);



