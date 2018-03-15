#ifndef KCU_DATA_H
#define KCU_DATA_H 1

#include <linux/list.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <linux/soc/mediatek/kcu.h>
#include <linux/stat.h>

#include <linux/spinlock.h>

static unsigned char kcu_dbgen=0;

#define KCU_DEBUG_MEM 1

#define CB_ATTR_RW(_name) static struct kobj_attribute _name ##_attr = __ATTR(_name, S_IRUSR|S_IWUSR|S_IRGRP, _name ##_show, _name ##_store)
#define CB_ATTR_RO(_name) static struct kobj_attribute _name ##_attr =__ATTR(_name, S_IRUSR|S_IRGRP, _name ##_show, NULL)
#define CB_BIN_ATTR_WO(_name,arg_size) static struct bin_attribute _name ##_attr = { \
	.attr = { .name = __stringify(_name), .mode = S_IWUSR |S_IRUSR | S_IRGRP },\
	.size=arg_size,\
	.write = _name,\
	}
#define KCU_LOG(arg...) \
    if(kcu_dbgen)printk(arg);

#define list_for_each_entry_fn(pos, head, member,fn,arg)    \
({ \
    int ret=0;\
    for (pos = list_entry_rcu((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry_rcu(pos->member.next, typeof(*pos), member)){ \
        if((ret=fn(pos,arg)))break;\
        }\
    ret;\
})        
#ifdef KCU_DEBUG_MEM
#define kcu_malloc(size,flags) \
({ \
    void *pret=kzalloc(size,flags);\
    KCU_LOG("kcu_mem:allocate %p,line=%d\n",pret,__LINE__);\
    pret; \
})
#define kcu_free(addr) \
({ \
    kfree(addr);\
    KCU_LOG("kcu_mem:free %p,line=%d\n",addr,__LINE__);\
})
#else
#define kcu_malloc  kmalloc
#define kcu_free    kfree
#endif   

#define ASYNC_NFY_TASK_MASK 0x01
#define SYNC_NFY_TASK_MASK 0x02

struct registered_task
{
	struct list_head list;
	struct task_struct *tsk; 
    struct notifier_data *rcu_data;//save to do rcu free
    struct rcu_head rcu;    
};
struct tgid_entry
{
	struct list_head list;
	pid_t tgid;
    struct rcu_head rcu;
    long mask;//this entry is for async or sync notify,or both 
};

struct nfyid_to_tgid_head
{
	struct list_head list;
	spinlock_t lock;
    struct rcu_head rcu; 
};

struct registered_task_head
{
	struct list_head list;
	spinlock_t lock;//lock for add/del entry into task head list
    spinlock_t nfy_lock;//lock for select the suitable task to notify(enqueue signal count is the least)
    struct rcu_head rcu; 
    unsigned long nfy_mask[(NFY_USR_MAX/BITS_PER_LONG)+(((NFY_USR_MAX%BITS_PER_LONG)==0)?0:1)];    
    struct registered_task *ru_task;//recently used task
#ifdef CONFIG_DEBUG_MEDIATEK_KCU
    struct kobj_attribute * debug_attr;
#endif    
};
    
typedef struct nfy_usr_info
{
	atomic_t usage;
//    unsigned long pad;
    struct nfy_usr_header nfy_header;
}NFY_USR_INFO_T;

static inline unsigned char nfy_mask_isset(unsigned long * set,unsigned long mask)
{
		return ((set[mask/BITS_PER_LONG] & (1UL << (mask % BITS_PER_LONG)))!=0);
}
static inline void nfy_mask_add(unsigned long * set,unsigned long mask)
{
		set[mask/BITS_PER_LONG] |= 1UL << (mask % BITS_PER_LONG);
}

static inline void nfy_mask_del(unsigned long * set,unsigned long mask)
{
		set[mask/BITS_PER_LONG] &= ~(1UL << (mask % BITS_PER_LONG));   
}

static inline int find_tgid_entry(struct tgid_entry *entry,pid_t tgid)
{
    if(entry->tgid==tgid)
    {
        return 1;
    }
    return 0;
}

static inline struct registered_task * find_entry_in_task_head(struct registered_task_head *head,struct task_struct *task)
{
    struct registered_task *tmp=NULL;
    list_for_each_entry_rcu(tmp, &head->list, list)
    {
        if(tmp->tsk==task)break;
    }
    
    return (tmp->tsk==task)?tmp:NULL;
}
static inline struct registered_task * del_entry_in_task_head(struct registered_task_head *head,struct task_struct *task)
{
    struct registered_task *tmp=NULL;
    
    spin_lock(&head->lock);
    list_for_each_entry_rcu(tmp, &head->list, list)
    {
        if(tmp->tsk==task)
        {
            list_del_rcu(&tmp->list);
            break;
        }
    }
    spin_unlock(&head->lock);
    
    return (tmp->tsk==task)?tmp:NULL;
}

static inline int sigmask2signo(sigset_t * mask)
{
        int signo=0;
        
        if (_NSIG_WORDS > 1)
        {
            signo=(_NSIG_BPW==64)?fls64(mask->sig[1]):fls(mask->sig[1]);
            signo=(signo==0)?0:(signo+BITS_PER_LONG);   
        }
        if(signo==0)
        {
            signo=(_NSIG_BPW==64)?fls64(mask->sig[0]):fls(mask->sig[0]);        
        }
        return signo;
}
static inline void ref_dec_and_free(void *ptr,atomic_t * usage)
{
	if (atomic_dec_and_test(usage))
	{
        kcu_free(ptr);
	}
}
#if KCU_CHK_SUM
static inline unsigned long calsum(unsigned char *data,size_t size)
{
    unsigned long sum=0;
    size_t index=0;
    
    while(index<size)
    {
        sum+=data[index];
        index++;
    }
    return sum;
}
#endif
#endif
