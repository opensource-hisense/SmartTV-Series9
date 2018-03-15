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
#include <linux/soc/mediatek/platform.h>
#include <linux/vmalloc.h>

#define attr_name(_attr) (_attr).attr.name
#define MIN(x,y) (x>y)?(y):(x)

struct wbp_info
{
    struct list_head list;
    struct perf_event  * __percpu * wbp;
    bool once;//only trigger once?
    bool dump_cond;//dump stack by condition?
    unsigned long cond_mask;
    unsigned long cond_val;
    unsigned char enable_map;
//    struct task_struct *task;
};
struct wbp_option
{
    unsigned long addr;
    bool once;//only trigger once?
    bool dump_cond;//dump stack by condition?
    unsigned long cond_mask;
    unsigned long cond_val;    
};
struct single_step_info
{
    struct perf_event   * wbp;
    unsigned long addr;
};
static LIST_HEAD(wbp_list);
//static unsigned char single_trigger_mode=0;

static struct kobject *mt_perf_kobj=NULL;
static DEFINE_RWLOCK(access_wp_lock);
static int _parse_hwbp_user_option(const char *str,bool bwp,struct wbp_option *option)
{
    char * ptmp,*ptmp2;
    char tmp_buf[30]={0};
    //wp format:addr,once/repeat[,cond_mask][,cond_val]
    //brp format:addr,once/repeat
    
    memset(option,0,sizeof(*option));
    
    ptmp=strchr(str, ',');
    if(!ptmp)
        return -EINVAL;
    
    strncpy(tmp_buf,str,MIN(ptmp-str,sizeof(tmp_buf)-1));
    
    pr_debug("tmp_buf=%s\n",tmp_buf);
    
    
    if (kstrtoul(tmp_buf, 0, &option->addr))
	{
        pr_info("invalid @%d\n",__LINE__);
        return -EINVAL;
	}
    
    ptmp++;
    pr_debug("once? %s\n",ptmp);
    //once
    if(!strncmp(ptmp,"once",4))option->once=true;
    else if(!strncmp(ptmp,"repeat",6))option->once=false;    
    else return -EINVAL;
    
    if(!bwp)
    {
        option->dump_cond=false;
        return 0;
    }
    //cond
    ptmp=strchr(ptmp, ',');
    if(ptmp)
    {
        ptmp++;
        option->dump_cond=true;
        ptmp2=strchr(ptmp, ',');
        
        memset(tmp_buf,0,sizeof(tmp_buf));
        strncpy(tmp_buf,ptmp,MIN(ptmp2-ptmp,sizeof(tmp_buf)-1));
        
        pr_debug("cond_mask=%s\n",tmp_buf);
        
        if (kstrtoul(tmp_buf, 0, &option->cond_mask))
            return -EINVAL;
            
        ptmp2++;
        pr_debug("cond_val=%s\n",ptmp2);
        if (kstrtoul(ptmp2, 0, &option->cond_val))
                return -EINVAL;
    }
    else
    {
        option->dump_cond=false;
    }
    return 0;
}
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
static void enable_single_step(struct perf_event *bp, unsigned long addr)
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
static void enable_single_step_on_all_cpu(struct perf_event *bp,unsigned long addr)
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
static void _notify_hwbp_to_user(struct perf_event *wbp)
{
    struct siginfo siginfo;
    struct sighand_struct *sighand;
    struct k_sigaction *ka;
    unsigned char bsend_signal=0;
    
    if((!irqs_disabled()) && current->mm && (current->mm!=&init_mm))
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
            siginfo.si_errno = 1;
            siginfo.si_code  = TRAP_HWBKPT;
            siginfo.si_addr  = (void __user *)(counter_arch_bp(wbp)->trigger);
    
            force_sig_info(siginfo.si_signo,&siginfo, current);
        }
    }
}
static void wbp_handler(struct perf_event *wbp,struct perf_sample_data *data,struct pt_regs *regs)
{
    struct arch_hw_breakpoint *info;
    struct wbp_info *wbp_info=NULL;
    bool action=false;
    
    info=counter_arch_bp(wbp);
    
    if(interrupts_enabled(regs))
    {
        local_irq_enable();
    }
    
	//pr_info("wbp:%p trigger:%llx,address %llx orig addr %llx\n",wbp,info->trigger,info->address,wbp->attr.bp_addr);
    
    if(wbp->attach_state & PERF_ATTACH_TASK)
    {
        pr_debug("wbp is attached to task\n");
    }
    else
    {
        wbp_info=get_global_event_info(wbp);
    }
    if (info->address==wbp->attr.bp_addr)//first time trigger
    {
        if(!wbp_info || wbp_info->once)
        {
            if(wbp_info && (wbp_info->dump_cond))
            {
                    if(((*(volatile unsigned int *)wbp->attr.bp_addr) & wbp_info->cond_mask)
                        ==wbp_info->cond_val)
                    {
                        pr_info("%p=%x\n",(volatile unsigned int *)wbp->attr.bp_addr,*(volatile unsigned int *)wbp->attr.bp_addr);
                        action=true;
                    }
            }
            else
            {
                action=true;
            }
            if(action)
            {
                pr_info("touch %llx\n",info->address);
                dump_stack();
                _notify_hwbp_to_user(wbp);                
            }
            
            if(irqs_disabled())
            {
                arch_uninstall_hw_breakpoint(wbp);
                if(wbp_info)wbp_info->enable_map &=~(1U<<smp_processor_id());
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
                if(wbp_info)wbp_info->enable_map=0;    
            }
            goto exit;
        }
    }
    
    if (info->address!=wbp->attr.bp_addr)//second time,repeat mode
	{
        //Todo:set condition here to show something
        if(wbp_info && wbp_info->dump_cond)
        {
            if(((*(volatile unsigned int *)wbp->attr.bp_addr) & wbp_info->cond_mask)
                ==wbp_info->cond_val)
            {
                pr_info("%p=%x\n",(volatile unsigned int *)wbp->attr.bp_addr,*(volatile unsigned int *)wbp->attr.bp_addr);
                action=true;
            }
        }
        else
        {
            action=true;
        }
        if(action)
        {
            pr_info("touch %llx\n",info->address);
            dump_stack();
            _notify_hwbp_to_user(wbp);                
        }
        pr_debug("watchpoint/breakpoint over! %llx\n",wbp->attr.bp_addr);
        pr_debug("disable step control\n");
        if(irqs_disabled() || (wbp->attach_state & PERF_ATTACH_TASK))
        {
            disable_single_step(wbp);
        }
        else
        {
            disable_single_step_on_all_cpu(wbp);
        }
        pr_debug("disable step control end\n");
	}
    else
    {
        pr_debug("enable step control irq_disabled:%x\n",irqs_disabled());
        if(irqs_disabled() || (wbp->attach_state & PERF_ATTACH_TASK))
        {
            enable_single_step(wbp,instruction_pointer(regs)+4);
        }
        else
        {
            enable_single_step_on_all_cpu(wbp,instruction_pointer(regs)+4);
        }
        pr_debug("enable step control end\n");
    }
exit:    
    if(interrupts_enabled(regs))
    {
       local_irq_disable();
    }
}
static void * _register_wp_bp(unsigned char bwp,unsigned long wp_addr)
{
	struct perf_event * __percpu * wbp;
    struct perf_event_attr wp_attr;
    int ret=0;
    struct wbp_info *new_wbp_info=NULL;
    struct perf_event * user_wbp;
    int cpu;
    
    hw_breakpoint_init(&wp_attr);
    
    
    wp_attr.bp_addr = wp_addr;
    wp_attr.bp_len = ((sizeof(unsigned long)==4) || !bwp)?HW_BREAKPOINT_LEN_4:HW_BREAKPOINT_LEN_8;
    wp_attr.bp_type = bwp?HW_BREAKPOINT_W:HW_BREAKPOINT_X;
    
    if(bwp)wp_addr &= ~0x7UL;
    else wp_addr &= ~0x3UL;
    
    printk("%s-%d-%d:Enable %s: 0x%lx,len %llx\n",current->comm,current->pid,current->tgid,bwp?"hwwp":"hwbrp",wp_addr,wp_attr.bp_len);	
    
    if(wp_addr<TASK_SIZE)//user space address
    {
        user_wbp=register_user_hw_breakpoint(&wp_attr,wbp_handler, NULL,current);
        if (IS_ERR((void __force *)user_wbp)) {
            ret = PTR_ERR((void __force *)user_wbp);
            printk(KERN_INFO "registration Fail %d\n",ret);
            return user_wbp;
        }
        return NULL;
    }
    
    wbp = register_wide_hw_breakpoint(&wp_attr,wbp_handler, NULL);

    if (IS_ERR((void __force *)wbp)) {
        ret = PTR_ERR((void __force *)wbp);
        printk(KERN_INFO "registration Fail %d\n",ret);
		return wbp;
    }
    
    new_wbp_info=(struct wbp_info *)kzalloc(sizeof(struct wbp_info),GFP_KERNEL);
    if(!new_wbp_info)return ERR_PTR(-ENOMEM);
    INIT_LIST_HEAD(&new_wbp_info->list);
    new_wbp_info->wbp=wbp;
    
    get_online_cpus();
	for_each_online_cpu(cpu)
    {
       new_wbp_info->enable_map |=(1U<<cpu);
    }
    put_online_cpus();    
    
    write_lock(&access_wp_lock);
    list_add_tail(&new_wbp_info->list,&wbp_list);
    write_unlock(&access_wp_lock);
    
    return new_wbp_info;
}
static ssize_t watchpoint_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
    const char * name=attr?attr_name(*attr):NULL;
    unsigned char bwp=1;
    void *ret_ptr;
    struct wbp_info *wbp_info=NULL;
    struct wbp_option option;
    
    if(name && !strncmp(name,"hwbrp",5))
    {
        bwp=0;
    }
        
    if(_parse_hwbp_user_option(buf,bwp,&option))
    {
        return -EINVAL;
    }
    
    ret_ptr=_register_wp_bp(bwp,option.addr);
    if(!ret_ptr)goto done;
    if(IS_ERR(ret_ptr))return PTR_ERR(ret_ptr);
    wbp_info=(struct wbp_info *)ret_ptr;
    wbp_info->once=option.once;
    wbp_info->dump_cond=option.dump_cond;    
    wbp_info->cond_mask=option.cond_mask;
    wbp_info->cond_val=option.cond_val;
    
done:    
    pr_info("%s registration OK\n",name?name:"unknown");
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
        count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "%s:0x%llx,%s,enable_map %x ",type_str,counter_arch_bp(wbp)->address,new_wbp_info->once?"once":"repeat",new_wbp_info->enable_map);
        if(new_wbp_info->dump_cond)
        {
            count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "cond_msk:%lx cond_val:%lx",new_wbp_info->cond_mask,new_wbp_info->cond_val);    
        }
        count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\n");
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
				      unsigned long action, void *hcpu)
{
    int cpu = (long)hcpu;
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

static const struct attribute *mt_perf_event_attrs[] = {
	&wp_attr.attr,
	&uninstall_attr.attr,
	&hp_attr.attr,
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

enum
{
    WP_TYPE_NONE,
    WP_IO_PHYS,
    WP_DRAM_PHYS,
    WP_DRAM_VIRT,
};
static __initdata unsigned long  boot_wp_addr=ULONG_MAX;
static __initdata bool  boot_wp_trig_once;
static __initdata bool  boot_wp_dump_cond;
static __initdata unsigned long  boot_wp_cond_mask;
static __initdata unsigned long  boot_wp_cond_val;
static __initdata unsigned long  boot_brp_addr=ULONG_MAX;
static __initdata bool  boot_brp_trig_once;
static __initdata unsigned int   boot_wp_type=WP_TYPE_NONE;
extern void *tv_reg_base;

static int __init setup_hw_wp(char *str)
{
    char * ptmp;
    struct wbp_option option;
    
    if (*str++ != '=' || !*str)
        return -EINVAL;
    ptmp=strchr(str, ',');
    if(!ptmp)
        return -EINVAL;
    
    if(!strncmp(str,"io",ptmp-str))
        boot_wp_type=WP_IO_PHYS;
        
    if(!strncmp(str,"phys",ptmp-str))
        boot_wp_type=WP_DRAM_PHYS;
    
    if(!strncmp(str,"virt",ptmp-str))
        boot_wp_type=WP_DRAM_VIRT;
    
    ptmp++;
    if(_parse_hwbp_user_option(ptmp,true,&option))
    {
        return -EINVAL;
    }
    
    boot_wp_addr=option.addr;
    boot_wp_trig_once=option.once;
    boot_wp_dump_cond=option.dump_cond;
    boot_wp_cond_mask=option.cond_mask;
    boot_wp_cond_val=option.cond_val;    
    
    pr_debug("boot_wp_addr=%lx\n",boot_wp_addr);
    
    return 1;
}
static int __init setup_hw_brp(char *str)
{
    struct wbp_option option;
    
    if (*str++ != '=' || !*str)
        return -EINVAL;
 
    if(_parse_hwbp_user_option(str,false,&option))
    {
        return -EINVAL;
    }
    
    boot_brp_addr=option.addr;
    boot_brp_trig_once=option.once; 

    pr_debug("boot_brp_addr=%lx\n",boot_brp_addr);
    
    return 1;
}
static int __init init_boot_hw_brp(void)
{
    unsigned long addr;
    void *ret_ptr;
    struct wbp_info * wbp_info;
    
    if(boot_wp_addr!=ULONG_MAX)
    {
        if(boot_wp_type==WP_IO_PHYS)
        {
            addr=(unsigned long)tv_reg_base+(boot_wp_addr-__pfn_to_phys(vmalloc_to_pfn(tv_reg_base)));
        }
        else if(boot_wp_type==WP_DRAM_PHYS)
        {
            addr=(unsigned long)phys_to_virt(boot_wp_addr);
        }
        else
        {
            addr=boot_wp_addr;
        }
        ret_ptr=_register_wp_bp(true,addr);
        if(!ret_ptr)goto done;
        if(IS_ERR(ret_ptr))
        {
            pr_err("register wp addr %lx fails\n",addr);
            return PTR_ERR(ret_ptr);
        }
        wbp_info=(struct wbp_info *)ret_ptr;

        wbp_info->once=boot_wp_trig_once;
        wbp_info->dump_cond=boot_wp_dump_cond;    
        wbp_info->cond_mask=boot_wp_cond_mask;
        wbp_info->cond_val=boot_wp_cond_val;
    }
    if(boot_brp_addr!=ULONG_MAX)
    {
        ret_ptr=_register_wp_bp(true,boot_brp_addr);
        if(!ret_ptr)goto done;
        if(IS_ERR(ret_ptr))
        {
            pr_err("register wp addr %lx fails\n",addr);
            return PTR_ERR(ret_ptr);
        }
        wbp_info=(struct wbp_info *)ret_ptr;

        wbp_info->once=boot_brp_trig_once;
        wbp_info->dump_cond=false;
    }
done:    
    return 0;
}
__setup("hwwp", setup_hw_wp);
__setup("hwbrp", setup_hw_brp);
late_initcall(init_boot_hw_brp);
