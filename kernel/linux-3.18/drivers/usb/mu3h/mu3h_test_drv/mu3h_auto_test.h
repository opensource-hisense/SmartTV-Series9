#ifndef _MU3H_SYSFS_H
#define _MU3H_SYSFS_H

#include <linux/kobject.h>

#define CLI_MAX 30

#define mu3h_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

struct mu3h_test_item {
	char *name;
	char *command_line[CLI_MAX];
};

#endif

