/*
 * linux/arch/arm/mach-mt53xx/perf_event.c
 *
 * watchpoint,breakpoint install/uninstall
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author: dtvbm11 $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */
#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/sched.h>
#include <linux/cpu_pm.h>
#include <linux/smp.h>
#include <linux/signal.h>
#include <linux/list.h>
#include <linux/slab.h>

#define attr_name(_attr) (_attr).attr.name

struct wbp_info
{
    struct list_head list;
    struct perf_event  * __percpu * wbp;
//    struct task_struct *task;
};
struct single_step_info
{
    struct perf_event   * wbp;
    u32 addr;
};
static LIST_HEAD(wbp_list);
static unsigned char single_trigger_mode=0;

static struct kobject *mt_perf_kobj=NULL;
static DEFINE_RWLOCK(access_wp_lock);
static struct wbp_info * get_global_event_info(struct perf_event *bp)
{
    struct wbp_info *new_wbp_info=NULL;
    struct perf_event   *wbp;
    
    read_lock(&access_wp_lock);
    list_for_each_entry(new_wbp_info,&wbp_list,list)
    {
        wbp=per_cpu(*(new_wbp_info->wbp), smp_processor_id());
        if(wbp==bp)
        {
            break;
        }
    }
    read_unlock(&access_wp_lock);
    if(wbp!=bp)
    {
        return NULL;
    }
    return new_wbp_info;
}
static void enable_single_step(struct perf_event *bp, u32 addr)
{
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);

	arch_uninstall_hw_breakpoint(bp);
    info->ctrl.type=ARM_BREAKPOINT_EXECUTE;
    info->address=addr;
	arch_install_hw_breakpoint(bp);
}
static void disable_single_step(struct perf_event *bp)
{
    struct arch_hw_breakpoint *info = counter_arch_bp(bp);
    
	arch_uninstall_hw_breakpoint(bp);
    switch (bp->attr.bp_type) {
	case HW_BREAKPOINT_X:
		info->ctrl.type = ARM_BREAKPOINT_EXECUTE;
		break;
	case HW_BREAKPOINT_R:
		info->ctrl.type = ARM_BREAKPOINT_LOAD;
		break;
	case HW_BREAKPOINT_W:
		info->ctrl.type = ARM_BREAKPOINT_STORE;
		break;
	case HW_BREAKPOINT_RW:
		info->ctrl.type = ARM_BREAKPOINT_LOAD | ARM_BREAKPOINT_STORE;
		break;
	default:
		info->ctrl.type=ARM_BREAKPOINT_STORE;
        break;
	}
	info->address=bp->attr.bp_addr;
	arch_install_hw_breakpoint(bp);
}
static void disable_single_step_on_cpu(void *info)
{
    disable_single_step((struct perf_event *)info);
}
static void disable_single_step_on_all_cpu(struct perf_event *bp)
{
    struct wbp_info *new_wbp_info=NULL;
    int cpu;
    
    new_wbp_info=get_global_event_info(bp);
    if(!new_wbp_info)
        return;
    
    get_online_cpus();
	for_each_online_cpu(cpu)
    {
        smp_call_function_single((int)cpu, disable_single_step_on_cpu, per_cpu(*(new_wbp_info->wbp), cpu), 1);
    }
    put_online_cpus();
}
static void  enable_single_step_on_cpu(void *info)
{
    struct single_step_info *ss_info=(struct single_step_info *)info;
    enable_single_step(ss_info->wbp,ss_info->addr);
}
static void enable_single_step_on_all_cpu(struct perf_event *bp,u32 addr)
{
    struct wbp_info *new_wbp_info=NULL;
    int cpu;
    struct single_step_info ss_info;
    
    new_wbp_info=get_global_event_info(bp);
    if(!new_wbp_info)
        return;
    
    get_online_cpus();
	for_each_online_cpu(cpu)
    {
        ss_info.wbp=per_cpu(*(new_wbp_info->wbp), cpu);
        ss_info.addr=addr;
        smp_call_function_single((int)cpu, enable_single_step_on_cpu,&ss_info, 1);
    }
    put_online_cpus();
}
static void uninstall_wide_hw_breakpoint(void *info)
{
    arch_uninstall_hw_breakpoint((struct perf_event  *)info);
    counter_arch_bp((struct perf_event  *)info)->ctrl.enabled=0;
}
static void uninstall_wide_hw_breakpoint_on_all_cpu(struct perf_event *bp)
{
    struct wbp_info *new_wbp_info=NULL;
    int cpu=0;
    
    new_wbp_info=get_global_event_info(bp);
    if(!new_wbp_info)
        return;
    
    get_online_cpus();
	for_each_online_cpu(cpu)
    {
        smp_call_function_single((int)cpu, uninstall_wide_hw_breakpoint,per_cpu(*(new_wbp_info->wbp),cpu), 1);
    }
    put_online_cpus();
    
}
static void wbp_handler(struct perf_event *wbp,struct perf_sample_data *data,struct pt_regs *regs)
{
    struct siginfo siginfo;
    struct sighand_struct *sighand;
    struct k_sigaction *ka;
    unsigned char bsend_signal=0;
    struct arch_hw_breakpoint *info;
    
    info=counter_arch_bp(wbp);
    
	printk("wbp:%p trigger:%x \n",wbp,info->trigger);
    
    if(wbp->attach_state & PERF_ATTACH_TASK)
    {
        printk("wbp is attached to task\n");
    }
    
    if (info->address==wbp->attr.bp_addr)
    {
        dump_stack();
        if((!in_interrupt()) && current->mm && (current->mm!=&init_mm))
        {
            sighand = current->sighand;
            spin_lock_irq(&sighand->siglock);
            ka = &sighand->action[SIGTRAP-1];
            if ((ka->sa.sa_handler != SIG_IGN) && (ka->sa.sa_handler != SIG_DFL))
            {
                bsend_signal=1;
            }
            spin_unlock_irq(&sighand->siglock);//we accept the race condition of changing signal handler to IGN or DFL between this gap,but we must release spin lock since force_sig_info will acquire the spin lock  
            if(bsend_signal)
            {
                printk("send signal to user thread %s\n",current->comm);
                siginfo.si_signo = SIGTRAP;
                siginfo.si_errno = !single_trigger_mode;
                siginfo.si_code  = TRAP_HWBKPT;
                siginfo.si_addr  = (void __user *)(counter_arch_bp(wbp)->trigger);
    
                force_sig_info(siginfo.si_signo,&siginfo, current);
            }
        }
        if(single_trigger_mode)
        {
            if(in_interrupt())
            {
                arch_uninstall_hw_breakpoint(wbp);
                info->ctrl.enabled=0;            
            }
            else if(wbp->attach_state & PERF_ATTACH_TASK)
            {
                perf_event_disable(wbp);
                printk("disable %p for user bp\n",wbp);
            }
            else
            {
                uninstall_wide_hw_breakpoint_on_all_cpu(wbp);        
            }
            return;
        }
    }
    if (info->address!=wbp->attr.bp_addr)
	{
        printk("disable step control\n");
        if(in_interrupt() || (wbp->attach_state & PERF_ATTACH_TASK))
        {
            disable_single_step(wbp);
        }
        else
        {
            disable_single_step_on_all_cpu(wbp);
        }
        printk("disable step control end\n");
	}
    else
    {
        printk("enable step control\n");
        if(in_interrupt() || (wbp->attach_state & PERF_ATTACH_TASK))
        {
            enable_single_step(wbp,regs->ARM_pc+(thumb_mode(regs) ? 2 : 4));
        }
        else
        {
            enable_single_step_on_all_cpu(wbp,regs->ARM_pc+(thumb_mode(regs) ? 2 : 4));
        }
        printk("enable step control end\n");
    }
    
}

static ssize_t watchpoint_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
    unsigned long wp_addr;
	struct perf_event * __percpu * wbp;
    struct perf_event_attr wp_attr;
    int ret=0;
    struct wbp_info *new_wbp_info=NULL;
    struct perf_event * user_wbp;
    const char * name=attr?attr_name(*attr):NULL;
    unsigned char bwp=1;
    
    
	if (kstrtoul(buf, 0, &wp_addr))
		return -EINVAL;

    if(name && !strncmp(name,"hwbrp",5))
    {
        bwp=0;
    }
    hw_breakpoint_init(&wp_attr);
	
    wp_attr.bp_addr = wp_addr;
    wp_attr.bp_len = (sizeof(unsigned long)==4)?HW_BREAKPOINT_LEN_4:HW_BREAKPOINT_LEN_8;
    wp_attr.bp_type = bwp?HW_BREAKPOINT_W:HW_BREAKPOINT_X;
    
    printk("%s-%d-%d:Enable %s: 0x%lx\n",current->comm,current->pid,current->tgid,name?name:"unknown",wp_addr);	
    
    if(wp_addr<TASK_SIZE)//user space address
    {
        user_wbp=register_user_hw_breakpoint(&wp_attr,wbp_handler, NULL,current);
        if (IS_ERR((void __force *)user_wbp)) {
            ret = PTR_ERR((void __force *)user_wbp);
            printk(KERN_INFO "registration Fail %d\n",ret);
            return ret;
        }
        return count;
    }
    
    wbp = register_wide_hw_breakpoint(&wp_attr,wbp_handler, NULL);

    if (IS_ERR((void __force *)wbp)) {
        ret = PTR_ERR((void __force *)wbp);
        printk(KERN_INFO "registration Fail %d\n",ret);
		return ret;
    }
    
    new_wbp_info=(struct wbp_info *)kmalloc(sizeof(struct wbp_info),GFP_KERNEL);
    if(!new_wbp_info)return -ENOMEM;
    INIT_LIST_HEAD(&new_wbp_info->list);
    new_wbp_info->wbp=wbp;
    
    write_lock(&access_wp_lock);
    list_add_tail(&new_wbp_info->list,&wbp_list);
    write_unlock(&access_wp_lock);
    
    printk(KERN_INFO "%s registration OK\n",name?name:"unknown");
    return count;
}
static ssize_t hwbrp_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
    return watchpoint_store(kobj,attr,buf,count);
}
static ssize_t watchpoint_show(struct kobject *kobj,struct kobj_attribute *pattr, char *buf)
{
    ssize_t count = 0;
    struct perf_event   * wbp;
    struct wbp_info *new_wbp_info=NULL;
    char *type_str;
    
    read_lock(&access_wp_lock);

    list_for_each_entry(new_wbp_info,&wbp_list,list)
    {
        wbp=per_cpu(*(new_wbp_info->wbp), smp_processor_id());
        if(wbp->attr.bp_type==HW_BREAKPOINT_X)type_str="x";
        else if(wbp->attr.bp_type==HW_BREAKPOINT_R)type_str="r";
        else if(wbp->attr.bp_type==HW_BREAKPOINT_W)type_str="w";
        else if(wbp->attr.bp_type==HW_BREAKPOINT_RW)type_str="rw";
        count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "%s:0x%x\n",type_str,counter_arch_bp(wbp)->address);
    }
    read_unlock(&access_wp_lock);   
    return count;
}
static ssize_t uninstall_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
    unsigned long wp_addr;
	struct perf_event  * wbp;
    struct wbp_info *new_wbp_info=NULL;
    struct wbp_info *tmp=NULL;
    unsigned char found=0;
    
	if (kstrtoul(buf, 0, &wp_addr))
		return -EINVAL;
        
    write_lock(&access_wp_lock);
    list_for_each_entry_safe(new_wbp_info,tmp,&wbp_list,list)
    {
        wbp=per_cpu(*(new_wbp_info->wbp), smp_processor_id());
        if(counter_arch_bp(wbp)->address==wp_addr)
        {
            list_del(&new_wbp_info->list);
            found=1;
            break;
        }
    }
    write_unlock(&access_wp_lock);
    if(found)
    {
        unregister_wide_hw_breakpoint(new_wbp_info->wbp);
        kfree(new_wbp_info);
    }
    return count;    
}
static ssize_t trigger_mode_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
    if(!strncmp(buf,"repeat",min(count,strlen("repeat"))))
    {
        single_trigger_mode=0;
    }
    else if(!strncmp(buf,"single",min(count,strlen("single"))))
    {
        single_trigger_mode=1;
    }
    else
    {
        return -EINVAL;
    }
    return count;
}
static ssize_t trigger_mode_show(struct kobject *kobj,struct kobj_attribute *pattr, char *buf)
{
    ssize_t count = 0;
    char *msg;
    
    msg=single_trigger_mode?"single":"repeat";
    count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "current trigger mode:%s(choice:repeat/single)\n",msg);        
    return count;
}
static void reinstall_all_wide_hw_breakpoint_on_cpu(void *info)
{
    struct wbp_info *new_wbp_info=NULL;
    struct perf_event  * wbp;
        
    read_lock(&access_wp_lock);
    list_for_each_entry(new_wbp_info,&wbp_list,list)
    {
        wbp=per_cpu(*(new_wbp_info->wbp), smp_processor_id());
        if(arch_install_hw_breakpoint(wbp))
        {
            printk("reinstall %p fails\n",wbp);
        }
    }
    read_unlock(&access_wp_lock);
}
static void uninstall_all_wide_hw_breakpoint_on_cpu(void)
{
    struct wbp_info *new_wbp_info=NULL;
        
    read_lock(&access_wp_lock);
    list_for_each_entry(new_wbp_info,&wbp_list,list)
    {
        arch_uninstall_hw_breakpoint(per_cpu(*(new_wbp_info->wbp),smp_processor_id()));
    }
    read_unlock(&access_wp_lock);    
}

#ifdef CONFIG_PM
static int cpu_pm_notify(struct notifier_block *self, unsigned long action,
			     void *v)
{
	if (action == CPU_PM_EXIT)//for boot cpu resume
	{
        reinstall_all_wide_hw_breakpoint_on_cpu(NULL);
        printk(KERN_ALERT "perf_event:CPU_PM_EXIT\n");
	}
    else if(action==CPU_PM_ENTER)//for boot cpu suspend
    {
        uninstall_all_wide_hw_breakpoint_on_cpu();
        printk(KERN_ALERT "perf_event:CPU_PM_ENTER\n");        
    }
	return NOTIFY_OK;
}
static struct notifier_block cpu_pm_nb = {
	.notifier_call = cpu_pm_notify,
};
#endif
#ifdef CONFIG_HOTPLUG_CPU
static int cpu_hotplug_notify(struct notifier_block *self,
				      unsigned long action, void *cpu)
{
	if ((action & ~CPU_TASKS_FROZEN) == CPU_ONLINE)
	{
        printk(KERN_ALERT "perf_event:cpu_online\n");
        smp_call_function_single((int)cpu, reinstall_all_wide_hw_breakpoint_on_cpu, NULL, 1);
	}

	return NOTIFY_OK;
}

static struct notifier_block cpu_hotplug_nb = {
	.notifier_call = cpu_hotplug_notify,
};
#endif
static struct kobj_attribute wp_attr=__ATTR_RW(watchpoint);
static struct kobj_attribute uninstall_attr=__ATTR_WO(uninstall);
static struct kobj_attribute hp_attr=__ATTR_WO(hwbrp);
static struct kobj_attribute trig_mode_attr=__ATTR_RW(trigger_mode);

static const struct attribute *mt_perf_event_attrs[] = {
	&wp_attr.attr,
	&uninstall_attr.attr,
	&hp_attr.attr,
	&trig_mode_attr.attr,    
	NULL,
};

static int __init mt_perf_event_init(void)
{
    int error=0;
    
    mt_perf_kobj=kobject_create_and_add("mt_perf", kernel_kobj);
    if(!mt_perf_kobj)
    {
        return -ENOMEM;
    }
    error = sysfs_create_files(mt_perf_kobj, mt_perf_event_attrs);
    if (error)
    {
        kobject_put(mt_perf_kobj);
        return error;
    }
    #ifdef CONFIG_PM
	cpu_pm_register_notifier(&cpu_pm_nb);//care PM_EXIT
    #endif
    #ifdef CONFIG_HOTPLUG_CPU
	register_cpu_notifier(&cpu_hotplug_nb); //care online
    #endif
    return 0;
}
device_initcall(mt_perf_event_init);
