
#include <linux/kobject.h>

#include "mu3d_auto_test.h"
#include "mu3d_test_test.h"
#include "mu3d_test_unified.h"




struct mu3d_test_item mu3d_auto_test_items[] = {
	{ "mu3d_test driver",
		{ "auto.dev",
	      "auto.u3i",
          "auto.u3w",
          "auto.u3r",
          "auto.u3d",
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

	retval = call_function_mu3d(ch);
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

mu3d_attr(manu);	

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
	struct mu3d_test_item *item;
	char *temp;
	char *ch;
	ch = (char *)buf;
	if (ch[n-1] == '\n')
		ch[n-1] = '\0';
	
	retval = 0;

	item = mu3d_auto_test_items;
	
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
		    retval = call_function_mu3d(temp);
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

mu3d_attr(auto);



static struct attribute * mu3d_test[] = {
	&manu_attr.attr,
	&auto_attr.attr,
	NULL,
};

static struct attribute_group mu3d_attr_group = {
	.attrs = mu3d_test,
};



static int __init mu3d_creat_sysfs(void)
{
    int retval = 0;
	
	struct kobject *mu3d_kobj;
	
	mu3d_kobj = kobject_create_and_add("mu3d", NULL);
	if (!mu3d_kobj)
		return -ENOMEM;
	
	retval = sysfs_create_group(mu3d_kobj, &mu3d_attr_group);
	
	return retval;

}

module_init(mu3d_creat_sysfs);



