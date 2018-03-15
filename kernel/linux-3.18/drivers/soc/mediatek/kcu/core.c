#ifdef CONFIG_SYSFS
#include <linux/kobject.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/bitops.h>	
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/hardirq.h>
#include <linux/tracehook.h>
#include <linux/idr.h>
#define CREATE_TRACE_POINTS
#include <trace/events/kcu_trace.h>

#include  "kcu_data.h"
	
static DEFINE_MUTEX(reg_sig_thd_mutex);
static DEFINE_MUTEX(reg_nfy_func_mutex);

static DEFINE_IDR(tgid2task_idr);	//idr to convert tgid to registered task list
static DEFINE_RWLOCK(tgid2task_idr_lock);

static DEFINE_IDR(tgid2atask_idr);	//idr to convert tgid to async registered task list
static DEFINE_RWLOCK(tgid2atask_idr_lock);

static struct nfyid_to_tgid_head * nfyid2tgid_map[NFY_USR_MAX]={NULL};
static DEFINE_RWLOCK(nfyid2tgid_map_lock);

static DEFINE_SPINLOCK(free_task_lock);

static int sig_dequeue_notifier(void *priv);
static struct task_struct *_select_signal_thread(struct registered_task_head *head);
static struct task_struct *  _signal_to_idle_task(struct registered_task_head * head,struct siginfo * info,atomic_t * usage,unsigned long ret_size);
static struct nfyid_to_tgid_head * _get_tgid_head_by_nfyid(NFY_USR_ID nfyid);
static void _del_tgid_from_nfymap(pid_t pid,unsigned long *cb_mask,unsigned char bsync);
static void _del_task_head_by_pid(pid_t tgid,unsigned char bsync);
static struct registered_task_head * _get_task_head_by_pid(pid_t tgid,unsigned char bsync);
static int _prepare_notifier_data(struct signal_thread_info * reg_info);
static void _release_notifier_data(struct notifier_data *data);
static void _free_tgid_entry_rcu(struct rcu_head *head);
static void  _free_task_head_entry_rcu(struct rcu_head *head);
static void  _free_task_rcu(struct rcu_head *head);
static void _exit_signal_thread(struct task_struct *task);
static int task_free_notify_atomic(struct notifier_block *nb,unsigned long action, void *data);
static int _retrieve_signal_info(struct notifier_data * data);
static int sig_dequeue_notifier(void *priv);
static void  _free_tgid_head_rcu(struct rcu_head *head);
static struct registered_task_head *_allocate_task_head(void);
static int _assign_task_head(pid_t tgid,struct registered_task_head * task_head,unsigned char bsync);
static struct tgid_entry * _allocate_tgid_entry(pid_t tgid);
static void _assign_nfyid_head(NFY_USR_ID nfyid,struct nfyid_to_tgid_head * head);

#define attr_name(_attr) (_attr).attr.name
#ifdef CONFIG_DEBUG_MEDIATEK_KCU
static struct kobject *debug_dir_kobj=NULL;
static ssize_t reg_task_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
    ssize_t count = 0;
    const char * name=attr?attr_name(*attr):NULL;
    unsigned char bsync=0;
    pid_t tgid;
    struct registered_task_head *task_head;
    NFY_USR_ID nfy_id=0;
	struct registered_task *tmp=NULL;
	struct notifier_data *data;
    
    if(name)
    {
        bsync=(*name=='s');
        name++;
        name++;
        if (kstrtouint(name, 0, &tgid))
            return -EINVAL;
        task_head=_get_task_head_by_pid(tgid,bsync);
        if(IS_ERR(task_head))
        {
            rcu_read_unlock();
            count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "No Info\n");
        }
        else
        {
            count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "task head %p tgid %d:\n",task_head,tgid);
            count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0),"notify mask:\n\t");
            while(nfy_id<NFY_USR_MAX)
            {
                if(nfy_mask_isset(task_head->nfy_mask,nfy_id))
                {
                    count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0),"%2d ",nfy_id);
                    if(nfy_id && (nfy_id%8==0))count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0),"\n\t");
                }
                nfy_id++;
            }
            list_for_each_entry_rcu(tmp, &task_head->list, list)
            {
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\n\ttask %d-%s:\n",tmp->tsk->pid,tmp->tsk->comm);
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\t\tnotifier signal:%d\n",sigmask2signo(tmp->tsk->notifier_mask));
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\t\tnotifier data:\n");
                data=(struct notifier_data *)(tmp->tsk->notifier_data);  
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\t\t\t\tusr_buf:%p\n",data->usr_buf_addr);
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\t\t\t\tdone:%p\n",data->done);                                
            }
            rcu_read_unlock();
        }
    }
    return count;
}
static ssize_t debug_log_show(struct kobject *kobj,struct kobj_attribute *attr, char *buf)
{
    ssize_t count = 0;
    count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "%d\n",kcu_dbgen);
    return count;
}
static ssize_t debug_log_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
	if (kstrtou8(buf, 0, &kcu_dbgen))
		return -EINVAL;
    
    return count;
}
CB_ATTR_RW(debug_log);
static ssize_t notify_info_show(struct kobject *kobj,struct kobj_attribute *attr, char *buf)
{
    ssize_t count = 0;
    NFY_USR_ID nfy_id=0;
    struct nfyid_to_tgid_head *nfyid_head;
    struct tgid_entry * map_entry=NULL;
    unsigned long entry_cnt=0;
    
    while(nfy_id<NFY_USR_MAX)
    {
        nfyid_head=_get_tgid_head_by_nfyid(nfy_id);
        if(IS_ERR(nfyid_head))
        {
            rcu_read_unlock();
            nfy_id++;
            continue;
        }
        entry_cnt=0;
        count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\nnfy_id %d:\n\t",nfy_id);
        list_for_each_entry_rcu(map_entry, &nfyid_head->list, list)
        {
            if(map_entry->mask&SYNC_NFY_TASK_MASK)
            {
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "s-%d ",map_entry->tgid);
            }
            if(map_entry->mask&ASYNC_NFY_TASK_MASK)
            {
                count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "a-%d ",map_entry->tgid);
            }            
            if(entry_cnt && (entry_cnt%8==0))count += snprintf(buf+count,max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\n");
            entry_cnt++;
        }
        rcu_read_unlock();
        nfy_id++;
    }
    return count;
}
CB_ATTR_RO(notify_info);
static int reg_task_add_debug_entry(struct registered_task_head *task_head,pid_t tgid,unsigned char bsync)
{
    char *filename=NULL;
    struct kobj_attribute *kattr;
    size_t filenamelen=0;
    int error;
    pid_t tmp=tgid;
    
    while(tmp)
    {
        tmp/=10;
        filenamelen++;
    }
    filenamelen+=1+2;
    
    filename=(char *)kcu_malloc(filenamelen,GFP_KERNEL);
    if(!filename)return -ENOMEM;    
    
    snprintf(filename,filenamelen,"%c-%d",bsync?'s':'a',tgid);
    
    kattr=(struct kobj_attribute *)kcu_malloc(sizeof(struct kobj_attribute),GFP_KERNEL);
    if(!kattr)
    {
        kcu_free(filename);
        return -ENOMEM;
    }
    
    //memcpy(kattr,&debug_info_attr,sizeof(struct kobj_attribute));
    
    attr_name(*kattr)=filename;
    kattr->attr.mode=S_IRUSR|S_IRGRP;
    kattr->show=reg_task_show;
    
	error = sysfs_create_file(debug_dir_kobj,&kattr->attr);
	if (error)
	{
       kcu_free(kattr);
       kcu_free(filename);       
	}
    else
    {
        task_head->debug_attr=kattr;
    }
    return error;
}
#endif
static struct task_struct *_select_signal_thread(struct registered_task_head *task_head)
{
    struct registered_task *tmp=NULL;
    struct task_struct *task=NULL;
    unsigned long flags;
    atomic_t min_sigcnt=ATOMIC_INIT(INT_MAX);
    struct notifier_data *data;
       
    spin_lock_irqsave(&task_head->nfy_lock,flags);
    list_for_each_entry_rcu(tmp, &(task_head->ru_task->list), list)
    {
        if((unsigned long)tmp==(unsigned long)task_head)continue;
        data=(struct notifier_data *)(tmp->tsk->notifier_data);
        if(!atomic_read(&data->live_sigcnt))
        {
            task=tmp->tsk;
            break;
        }
        if(atomic_read(&data->live_sigcnt)<atomic_read(&min_sigcnt));
        {
            min_sigcnt=data->live_sigcnt;
            task=tmp->tsk;            
        }
    }
    if(task)
    {
        get_task_struct(task);
        data=(struct notifier_data *)(task->notifier_data); 
        atomic_inc(&data->live_sigcnt);
        task_head->ru_task=tmp;
    }
    spin_unlock_irqrestore(&task_head->nfy_lock,flags);
    return task;
}
static struct task_struct *  _signal_to_idle_task(struct registered_task_head * head,struct siginfo * info,atomic_t * usage,unsigned long ret_size)
{
    struct task_struct *tsk=NULL;
    sigset_t mask;
    int ret=0;
    struct notifier_data * data=NULL;
    
    tsk=_select_signal_thread(head);
    if(!tsk)
    {
        return ERR_PTR(-ESRCH);
    }
    data=(struct notifier_data *)(tsk->notifier_data);
    data->ret_size=ret_size;
    mask=*tsk->notifier_mask;
    info->si_signo=sigmask2signo(&mask);
    atomic_inc(usage);
    trace_kcu_nfy_start(tsk,atomic_read(&data->live_sigcnt),info->si_ptr);    
    ret=send_sig_info(info->si_signo,info,tsk);
    put_task_struct(tsk);
    if(ret<0)
    {
        printk(KERN_ALERT "send signal %d to task %d-%s fails,ret=%d\n",info->si_signo,tsk->pid,tsk->comm,ret);
        atomic_dec(usage);       
        return ERR_PTR(ret);
    }
    return tsk;
}
static struct tgid_entry * _allocate_tgid_entry(pid_t tgid)
{
    struct tgid_entry *map_entry;
    
    map_entry=(struct tgid_entry *)kcu_malloc(sizeof(struct tgid_entry),GFP_KERNEL);
    if(!map_entry)
    {
        return ERR_PTR(-ENOMEM);
    }
    INIT_LIST_HEAD(&map_entry->list);  
    map_entry->tgid=tgid; 
    map_entry->mask=0;
    return map_entry;
}
static struct nfyid_to_tgid_head * _allocate_nfyid_head(NFY_USR_ID nfyid)
{
    struct nfyid_to_tgid_head *nfyid_head;  
    
    nfyid_head=(struct nfyid_to_tgid_head *)kcu_malloc(sizeof(struct nfyid_to_tgid_head),GFP_KERNEL);
    if(!nfyid_head)return ERR_PTR(-ENOMEM);
    
    spin_lock_init(&nfyid_head->lock);
    INIT_LIST_HEAD(&nfyid_head->list); 
    
    return nfyid_head;
}
static void _assign_nfyid_head(NFY_USR_ID nfyid,struct nfyid_to_tgid_head * head)
{
    unsigned long flags;
    write_lock_irqsave(&nfyid2tgid_map_lock,flags);//need global spinlock    
    rcu_assign_pointer(nfyid2tgid_map[nfyid],head);
    write_unlock_irqrestore(&nfyid2tgid_map_lock,flags);    
}
static struct nfyid_to_tgid_head * _get_tgid_head_by_nfyid(NFY_USR_ID nfyid)
{
    struct nfyid_to_tgid_head *nfyid_head;
    
    read_lock(&nfyid2tgid_map_lock);
    nfyid_head=nfyid2tgid_map[nfyid];
    if(!nfyid_head)nfyid_head=ERR_PTR(-ESRCH);
    else rcu_read_lock();
    read_unlock(&nfyid2tgid_map_lock);    

    return nfyid_head;    
}
static void _del_tgid_head_by_nfyid(NFY_USR_ID nfyid)
{
    unsigned long flags=0;
    write_lock_irqsave(&nfyid2tgid_map_lock,flags);//need global spinlock
    rcu_assign_pointer(nfyid2tgid_map[nfyid],NULL);
    write_unlock_irqrestore(&nfyid2tgid_map_lock,flags);    
}
static void _del_tgid_from_nfymap(pid_t pid,unsigned long *nfy_mask,unsigned char bsync)
{
    struct nfyid_to_tgid_head *nfyid_head;
    struct tgid_entry * map_entry;
    NFY_USR_ID nfy_id=0;
    unsigned char bdelete=0;
    unsigned char bempty=0;
    unsigned char bfound=0;
    
    while(nfy_id<NFY_USR_MAX)
    {
        if(nfy_mask_isset(nfy_mask,nfy_id))
        {
           // printk("check nfyid %d\n",nfy_id);
            nfyid_head=_get_tgid_head_by_nfyid(nfy_id);
            if(IS_ERR(nfyid_head))
            {
                nfy_id++;
                continue;
            }
            spin_lock(&nfyid_head->lock);
            bfound=list_for_each_entry_fn(map_entry, &nfyid_head->list,list,find_tgid_entry,pid);            
            if(bfound)
            {
                clear_bit((bsync!=0),&map_entry->mask);
                bdelete=!map_entry->mask;
                if(bdelete)list_del_rcu(&map_entry->list);
            }
            bempty=list_empty(&nfyid_head->list);
            if(bempty)
            {
                //printk("delete tgid head %d\n",nfy_id);
                _del_tgid_head_by_nfyid(nfy_id);
            }
            if(bdelete)
            {
                call_rcu(&map_entry->rcu,_free_tgid_entry_rcu);
                if(bempty)
                {
                    call_rcu(&nfyid_head->rcu,_free_tgid_head_rcu); 
                }
            }
            spin_unlock(&nfyid_head->lock);
            rcu_read_unlock();
        }
        nfy_id++;
    }
}
static void _del_task_head_by_pid(pid_t tgid,unsigned char bsync)
{
    struct idr * idr_used;
    rwlock_t * idr_lock;
    unsigned long flags;
    struct registered_task_head *task_head;
    struct registered_task_head *tmp=NULL;
    
    idr_used=bsync?&tgid2task_idr:&tgid2atask_idr;
    idr_lock=bsync?&tgid2task_idr_lock:&tgid2atask_idr_lock;
    write_lock_irqsave(idr_lock,flags);
    task_head=(struct registered_task_head *)idr_find(idr_used,tgid);
    if(task_head)idr_remove(idr_used,tgid);
    tmp=task_head;            
    write_unlock_irqrestore(idr_lock,flags);
    
    if(tmp)call_rcu(&tmp->rcu,_free_task_head_entry_rcu);
}
static struct registered_task_head *_allocate_task_head(void)
{
    struct registered_task_head *task_head;
    
    task_head=(struct registered_task_head *)kcu_malloc(sizeof(struct registered_task_head),GFP_KERNEL);
    if(!task_head)return ERR_PTR(-ENOMEM);
    
    spin_lock_init(&task_head->lock);
    spin_lock_init(&task_head->nfy_lock);
    INIT_LIST_HEAD(&task_head->list);
    memset(task_head->nfy_mask,0,sizeof(task_head->nfy_mask));
    task_head->ru_task=NULL;
    
    return task_head;
}
static int _assign_task_head(pid_t tgid,struct registered_task_head * task_head,unsigned char bsync)
{
    struct idr * idr_used;
    rwlock_t * idr_lock;
    struct registered_task_head *tmp;
    int id;
    unsigned long flags=0;
    
    idr_used=bsync?&tgid2task_idr:&tgid2atask_idr;
    idr_lock=bsync?&tgid2task_idr_lock:&tgid2atask_idr_lock;
    
    rcu_assign_pointer(tmp,task_head);
    idr_preload(GFP_KERNEL);
    write_lock_irqsave(idr_lock,flags);
   // printk("pid %d:assign %s task head %p for tgid %d to idp %p\n",current->pid,bsync?"sync":"async",tmp,tgid,idr_used);
    id=idr_alloc(idr_used,tmp,tgid, 0, GFP_NOWAIT);
    write_unlock_irqrestore(idr_lock,flags);
    idr_preload_end();

    if(id!=tgid)
    {
        printk("pid %d:remove id %d from idr %p\n",current->pid,id,idr_used);
        idr_remove(idr_used,id);
        kcu_free(tmp);
        return -ENOSPC;
    }
    return 0;
}
static struct registered_task_head * _get_task_head_by_pid(pid_t tgid,unsigned char bsync)
{
    struct idr * idr_used;
    rwlock_t * idr_lock;
    struct registered_task_head *task_head;
    
    idr_used=bsync?&tgid2task_idr:&tgid2atask_idr;
    idr_lock=bsync?&tgid2task_idr_lock:&tgid2atask_idr_lock;
    read_lock(idr_lock);
    task_head=(struct registered_task_head *)idr_find(idr_used,tgid);
    if(!task_head)task_head=ERR_PTR(-ESRCH);
    else rcu_read_lock();
    read_unlock(idr_lock); 
    
    return task_head;
}
static int _prepare_notifier_data(struct signal_thread_info * reg_info)
{
    struct notifier_data *data;
    sigset_t * mask;
    int ret=0;
    
    data=(struct notifier_data *)kcu_malloc(sizeof(struct notifier_data)+sizeof(sigset_t),GFP_KERNEL);
	if(!data)
    {
        return -ENOMEM;    
    }
	data->usr_buf_addr=(void *)reg_info->usr_buf;
    data->done=NULL;
    data->ret_size=0;
    data->retbuf=NULL;
    atomic_set(&data->live_sigcnt,0);
	if(reg_info->bsync)
    {
        data->done=(struct completion *)kcu_malloc(sizeof(struct completion),GFP_KERNEL);
        if(!data->done)
        {
            ret=-ENOMEM;
            goto alloc_com_fail;
        }
        init_completion(data->done);
        
        data->retbuf=(unsigned char *)kcu_malloc(NFY_MAX_STRUCT_SIZE,GFP_KERNEL);
        if(!data->retbuf)
        {
            ret=-ENOMEM;
            goto alloc_ret_buf_fail;
        }     
    }
	mask=(sigset_t *)(((u8 *)data)+sizeof(struct notifier_data));
	sigemptyset(mask);
	sigaddset(mask,reg_info->signo);
	block_all_signals(sig_dequeue_notifier,data,mask);
    
    return 0;
alloc_ret_buf_fail:
     kcu_free(data->done);    
alloc_com_fail:
    kcu_free(data);
    
    return ret;
}
static void _release_notifier_data(struct notifier_data *data)
{
    if(data)
    {
        if(data->done)
        {
            if(!(completion_done(data->done)))//is threre any waiters?
            {
                complete_all(data->done);
            }
            kcu_free(data->done);
        }
        if(data->retbuf)
        {
            kcu_free(data->retbuf);        
        }
        kcu_free(data); 
    }
    unblock_all_signals();
}
static void _free_tgid_entry_rcu(struct rcu_head *head)
{
	struct tgid_entry *e = container_of(head,struct tgid_entry, rcu);
	kcu_free(e);
}
static void  _free_task_head_entry_rcu(struct rcu_head *head)
{
	struct registered_task_head *e = container_of(head,struct registered_task_head, rcu);
#ifdef CONFIG_DEBUG_MEDIATEK_KCU
    struct kobj_attribute *kattr=e->debug_attr;
    sysfs_remove_file(debug_dir_kobj,&kattr->attr);
    kcu_free(attr_name(*kattr));
    kcu_free(kattr);
#endif
    
	kcu_free(e);    
}
static void  _free_task_rcu(struct rcu_head *head)
{
	struct registered_task *e = container_of(head,struct registered_task, rcu);
    _release_notifier_data(e->rcu_data);
	kcu_free(e);    
}

static void  _free_tgid_head_rcu(struct rcu_head *head)
{
	struct nfyid_to_tgid_head *e = container_of(head,struct nfyid_to_tgid_head, rcu);
	kcu_free(e);      
}
static void _exit_signal_thread(struct task_struct *task)
{
    struct notifier_data *data;
    struct registered_task_head  *task_head; 
    struct registered_task *  reg_tsk;
    
    if(task->notifier_data)
    {
        data=(struct notifier_data *)task->notifier_data;
        spin_lock(&free_task_lock);
        printk("free task %d-%s\n",task->pid,task->comm);
        task_head=_get_task_head_by_pid(task->tgid,data->done!=NULL);
        
        if(!IS_ERR(task_head))
        {
           reg_tsk=del_entry_in_task_head(task_head,task);
           if(reg_tsk)call_rcu(&reg_tsk->rcu,_free_task_rcu);
           spin_lock(&task_head->lock);
           if(list_empty(&task_head->list))
           {
                _del_tgid_from_nfymap(task->tgid,task_head->nfy_mask,data->done!=NULL);
                spin_unlock(&task_head->lock);
                _del_task_head_by_pid(task->tgid,data->done!=NULL);
           }
           else
           {
                spin_unlock(&task_head->lock);
           }
           rcu_read_unlock();
        }
        spin_unlock(&free_task_lock);
    }
}
static int task_free_notify_atomic(struct notifier_block *nb,unsigned long action, void *data)
{
    struct task_struct *task=NULL;
    
	if((action==0) && (data))
	{
		task=(struct task_struct *)data;
        _exit_signal_thread(task);
    }    
    return 0;
}
static int _retrieve_signal_info(struct notifier_data * data)
{
	struct sigqueue *q=NULL;
    struct sigpending * list=&current->pending;
    int sig=sigmask2signo(current->notifier_mask);
    siginfo_t * info=NULL;
    struct nfy_usr_info * usr_info=NULL;
    int result=0;
#if 0    //here do not drop the event happends between the gap from reg to unreg,since it is hard to do __sigqueue_free if we do not modify kernel native code,it can be drpped by user space 
    unsigned long flags=0;
    unsigned char bnfy=0;
	/*
	 * Collect the siginfo appropriate to this signal.  Check if
	 * there is another siginfo for the same signal.
	*/
    list_for_each_entry_safe(q, tmp,&list->list, list) 
    {
        if (q->info.si_signo != sig)continue; 
        
        info=&q->info;
        
        if(info)
        {
            usr_info=(struct nfy_usr_info *)(info->si_ptr);
            raw_spin_lock_irqsave(&reg_task_lock, flags);
            bnfy=nfy_mask_isset(data->cb_mask,usr_info->nfy_header.eFctId);
            raw_spin_unlock_irqrestore(&reg_task_lock, flags);	
            if(bnfy)
            {
                if(copy_to_user(data->usr_buf_addr,&usr_info->nfy_header,usr_info->nfy_header.u4TagSize+sizeof(struct nfy_usr_header)));
                {
                    printk(KERN_ERR "copy_to_user fail!L %d!\n",__LINE__);
                    return -EFAULT;
                }
                ref_dec_and_free(usr_info,&usr_info->usage);
                info->si_ptr=data->usr_buf_addr;
                break;
            }
            else//not watched any more
            {
                ref_dec_and_free(usr_info,&usr_info->usage);
                list_del_init(&q->list);
                __sigqueue_free(q);
            }
        }
        else
        {
            printk(KERN_ERR "siginfo is null!\n");
            return -EINVAL;
        }
    }
#endif

    list_for_each_entry(q,&list->list, list) 
    {
        if (q->info.si_signo == sig)
        {
            info=&q->info;
            break; 
        }
    }
    if(info)
    {
      usr_info=(struct nfy_usr_info *)(info->si_ptr);
      if(copy_to_user(data->usr_buf_addr,&usr_info->nfy_header,usr_info->nfy_header.u4TagSize+sizeof(struct nfy_usr_header)))
      {
        printk(KERN_ERR "copy_to_user fail!L %d!\n",__LINE__);
        result=-EFAULT;
      }
      else
      {
        ref_dec_and_free(usr_info,&usr_info->usage);       
      }
    }
    else
    {
        printk(KERN_ERR "siginfo is null!\n");
        result= -EINVAL;
    }
    if(!result)
    {
        if(usr_info)
        {
            trace_kcu_nfy_process(usr_info->nfy_header.eFctId,data->usr_buf_addr,info->si_ptr);   
        }
        info->si_ptr=data->usr_buf_addr; 
    }
    return result;
}

static int sig_dequeue_notifier(void *priv)
{
    struct notifier_data *data=(struct notifier_data *)priv;
    
    if(0!=_retrieve_signal_info(data))
    {
        printk(KERN_ERR "process signal fails\n");
    }
    else
    {
        if(!data->done)//mark the async task as finish processing this signal after signal dequeue since we can not know when the user space has finished process the signal
        {
            atomic_dec(&data->live_sigcnt);
        }
    }
    /*always return 1,otherwise SIGPENDING thread flag will be cleared.do it as original signal deliver flow*/
	return 1; 
}		  
static ssize_t kcu_register_signal_thread(struct file *filp, struct kobject *kobj,struct bin_attribute *bin_attr,char *buf, loff_t off, size_t count)
{
	struct signal_thread_info * info=NULL;
	struct registered_task * reg_task=NULL;
	unsigned long flags=0;
    struct registered_task_head *task_head=NULL;
    ssize_t ret=count;
    
	if((off!=0) || (count!=sizeof(struct signal_thread_info)))return -EINVAL;
	
	if(current->notifier_data)
	{
		return -EINVAL;
	}
	
	info=(struct signal_thread_info *)buf;
	
	if((info->signo<SIGRTMIN) || (info->signo>SIGRTMAX))
	{
		return -EINVAL;
	}
	if(!access_ok(VERIFY_WRITE, info->usr_buf,NFY_MAX_STRUCT_SIZE))
	{
		return -EINVAL;
	}
	
    ret=_prepare_notifier_data(info);
    if(ret<0)return ret;
    
	reg_task=(struct registered_task *)kcu_malloc(sizeof(struct registered_task),GFP_KERNEL);
	if(!reg_task)
    {
        ret=-ENOMEM;
        goto alloc_reg_task_fail;
    }
	reg_task->tsk=current;
    reg_task->rcu_data=current->notifier_data;

	INIT_LIST_HEAD(&reg_task->list); 
    
    mutex_lock(&reg_sig_thd_mutex);
    task_head=_get_task_head_by_pid(current->tgid,info->bsync);
    if(IS_ERR(task_head))
    {
        task_head=_allocate_task_head();
        if(!IS_ERR(task_head))
        {
            #ifdef CONFIG_DEBUG_MEDIATEK_KCU
            reg_task_add_debug_entry(task_head,current->tgid,info->bsync);
            #endif
            task_head->ru_task=reg_task;
            list_add_tail_rcu(&reg_task->list,&task_head->list);
            if((ret=_assign_task_head(current->tgid,task_head,info->bsync))<0)
            {
                goto alloc_reg_task_head_fail;
            }
        }
        else
        {
                ret=(ssize_t)PTR_RET(task_head);
                goto alloc_reg_task_head_fail;            
        }
    }
    else
    {

     	spin_lock_irqsave(&task_head->lock, flags);
       // printk("pid %d:add %p into task head %p\n",current->pid,reg_task,task_head);
        task_head->ru_task=reg_task;
        list_add_tail_rcu(&reg_task->list,&task_head->list);
        spin_unlock_irqrestore(&task_head->lock, flags);
        rcu_read_unlock();
    }
        
    mutex_unlock(&reg_sig_thd_mutex);    
    
    KCU_LOG("pid %d-%s:register %s signal thd into task head %p\n",current->pid,current->comm,info->bsync?"sync":"async",task_head);

    current->flags |= PF_NOFREEZE;//kcu thread need to be used when do suspend and resume
    
    return count;
alloc_reg_task_head_fail:
    mutex_unlock(&reg_sig_thd_mutex);
    kcu_free(reg_task); 
alloc_reg_task_fail:
    _release_notifier_data(current->notifier_data);

	return ret;
}
static ssize_t kcu_register_nfy_func(struct file *filp, struct kobject *kobj,struct bin_attribute *bin_attr,char *buf, loff_t off, size_t count)
{
	struct nfy_func_info * pset_info;
	unsigned long flags=0;
    struct registered_task_head *task_head; 
    ssize_t ret=count;
    struct tgid_entry * map_entry,*tmp_entry;
    int bfound=0,bempty=0,bdelete=0;
    struct nfyid_to_tgid_head *nfyid_head;
    NFY_USR_ID nfy_id;
    
	if((off!=0) || (count!=sizeof(struct nfy_func_info)))return -EINVAL;
	
	pset_info=(struct nfy_func_info *)buf;
    nfy_id=pset_info->eFctId;
    
	if(nfy_id>=NFY_USR_MAX)
    {
        printk("invalid funcid:%d %d\n",nfy_id,NFY_USR_MAX);
        return -EINVAL;
    }
    mutex_lock(&reg_nfy_func_mutex);
    //check whether the process has any thread registered as signal thread
    task_head=_get_task_head_by_pid(current->tgid,pset_info->bsync);
    if(IS_ERR(task_head))
    {
        printk("can not find task for tgid %d\n",current->tgid);
        ret=(ssize_t)PTR_RET(task_head);
        goto unlock;          
    }
    if(pset_info->nfy_en==nfy_mask_isset(task_head->nfy_mask,nfy_id))
    {
        rcu_read_unlock();
        ret=-EINVAL;
        goto unlock;    
    }
    else
    {
        if(pset_info->nfy_en)nfy_mask_add(task_head->nfy_mask,nfy_id);
        else nfy_mask_del(task_head->nfy_mask,nfy_id);
    }
    rcu_read_unlock();    
    
    if(pset_info->nfy_en)
    {
        map_entry=_allocate_tgid_entry(current->tgid);
        if(IS_ERR(map_entry))
        {
            ret=(ssize_t)PTR_RET(map_entry);
            goto unlock;        
        }       
    }
    nfyid_head=_get_tgid_head_by_nfyid(nfy_id);
    KCU_LOG("pid %d-%s:%s notify id %d for %s task head %p\n",current->pid,current->comm,pset_info->nfy_en?"register":"unregister",nfy_id,pset_info->bsync?"sync":"async",task_head);
    if(IS_ERR(nfyid_head))
    {
        if(!pset_info->nfy_en)//unregister case,do not allow null nfyid head.that's mean it does not register this nfy id before
        {
            ret=-EINVAL;            
            goto unlock;
        }
        nfyid_head=_allocate_nfyid_head(nfy_id);
        if(IS_ERR(nfyid_head))
        {
            ret=(ssize_t)PTR_RET(nfyid_head);
            kcu_free(map_entry);
            goto unlock;
        }
        else
        {
            set_bit((pset_info->bsync!=0),&map_entry->mask);
            list_add_tail_rcu(&map_entry->list,&nfyid_head->list);
            _assign_nfyid_head(nfy_id,nfyid_head);
        }
    }
    else
    {
        if(pset_info->nfy_en)//register case
        {
            spin_lock_irqsave(&nfyid_head->lock,flags);
            bfound=list_for_each_entry_fn(tmp_entry, &nfyid_head->list,list,find_tgid_entry,current->tgid);
            if(!bfound)//first time
            {
                set_bit((pset_info->bsync!=0),&map_entry->mask);
                list_add_tail_rcu(&map_entry->list,&nfyid_head->list);                
            }
            else
            {
                set_bit((pset_info->bsync!=0),&tmp_entry->mask);                
            }
            spin_unlock_irqrestore(&nfyid_head->lock,flags);
            if(bfound)_free_tgid_entry_rcu(&map_entry->rcu);
        }
        else//unregister case
        {
            spin_lock_irqsave(&nfyid_head->lock,flags);
            bfound=list_for_each_entry_fn(tmp_entry, &nfyid_head->list,list,find_tgid_entry,current->tgid);            
            if(bfound)
            {
                clear_bit((pset_info->bsync!=0),&tmp_entry->mask);
                bdelete=!tmp_entry->mask;
                if(bdelete)list_del_rcu(&tmp_entry->list);
            }
            bempty=list_empty(&nfyid_head->list);
            if(bdelete)
            {
                call_rcu(&tmp_entry->rcu,_free_tgid_entry_rcu);
                if(bempty)
                {
                    _del_tgid_head_by_nfyid(nfy_id);
                    call_rcu(&nfyid_head->rcu,_free_tgid_head_rcu); 
                }
            }
            spin_unlock_irqrestore(&nfyid_head->lock,flags);           
        }
        rcu_read_unlock();
    }
unlock:    
    mutex_unlock(&reg_nfy_func_mutex); 
    
	return ret;
}
CB_BIN_ATTR_WO(kcu_register_signal_thread,sizeof(struct signal_thread_info));
CB_BIN_ATTR_WO(kcu_register_nfy_func,sizeof(struct nfy_func_info));
static struct kobject *kcu_kobj=NULL;
static struct notifier_block signal_task_free_nb = {
	.notifier_call = task_free_notify_atomic,
};
static struct bin_attribute * bin_attr_grp[]=
{
    &kcu_register_signal_thread_attr, 
    &kcu_register_nfy_func_attr,
    NULL,
};

static int __init kcu_sysfs_init(void)
{
	int error=0;
	struct bin_attribute ** bin_attr=bin_attr_grp;
    	
    kcu_kobj = kobject_create_and_add("kcu", kernel_kobj);	

	if (!kcu_kobj) {
		error = -ENOMEM;
		goto exit;
	}
#ifdef CONFIG_DEBUG_MEDIATEK_KCU
    debug_dir_kobj = kobject_create_and_add("debug", kcu_kobj);
    
	if (!debug_dir_kobj) {
		error = -ENOMEM;
		goto kobj_exit;
	} 
	error = sysfs_create_file(debug_dir_kobj, &debug_log_attr.attr);
	if (error)
		goto debug_dir_kobj_exit;
	error = sysfs_create_file(debug_dir_kobj, &notify_info_attr.attr);
	if (error)
		goto debug_dir_kobj_exit;
#endif
    while(*bin_attr)
	{
        error = sysfs_create_bin_file(kcu_kobj,*bin_attr);
        if (error)
		goto kobj_exit;
        bin_attr++;        
	}

	task_free_register(&signal_task_free_nb);
    
    if((sizeof(struct nfy_usr_info)-offsetof(struct nfy_usr_info,nfy_header))!=sizeof(struct nfy_usr_header))
    {
        printk(KERN_ALERT "pleasr check the alignment of struct nfy_usr_info\n");
        printk("sizeof(struct nfy_usr_info)=%zd struct nfy_usr_header=%zd\n",sizeof(struct nfy_usr_info),sizeof(struct nfy_usr_header));
        printk("sizeof(atomic_t)=%zd\n",sizeof(atomic_t));
        printk("offset of nfy_usr_header=%d\n",offsetof(struct nfy_usr_info,nfy_header));        
        BUG();
    }
	return 0;
#ifdef CONFIG_DEBUG_MEDIATEK_KCU
debug_dir_kobj_exit:
	kobject_put(debug_dir_kobj);
#endif
kobj_exit:
	kobject_put(kcu_kobj);		
exit:
	return error;	
}
device_initcall(kcu_sysfs_init);
#ifndef CONFIG_ARM64
//reset notifier data when do fork/clone
int  arch_dup_task_struct(struct task_struct *dst,
					       struct task_struct *src)
{
	*dst = *src;
    dst->notifier=NULL;
    dst->notifier_data=NULL;
    dst->notifier_mask=NULL;
	return 0;
}
#endif
static int kcu_notify_user(NFY_USR_ID nfy_id,void * pvArg,unsigned int u4TagSize,unsigned char bsync,void *ptr_ret,unsigned long ret_size)
{
    struct nfy_usr_info * p_nfy_info=NULL;
    gfp_t allocflags=0;
    struct siginfo info;
    int ret=-ESRCH;
    struct task_struct *tsk=NULL;
    struct notifier_data * data=NULL;
    struct tgid_entry * map_entry=NULL;
    struct registered_task_head *task_head;
    struct nfyid_to_tgid_head *  nfyid_head; 
    
    if(nfy_id>=NFY_USR_MAX)return -EINVAL;
    if(u4TagSize>NFY_MAX_STRUCT_SIZE)return -EINVAL;
    if(bsync && (ret_size>NFY_MAX_STRUCT_SIZE))return -EINVAL;//we set the maximum size as the max input arg
    
    trace_kcu_nfy_prepare(nfy_id,bsync);
    if (in_interrupt())
    {
        if(bsync)
        {
            printk("can not block when in interrupt\n");
            return -EINVAL;
        }
        allocflags = GFP_ATOMIC;
    }
    else
    {
        allocflags = GFP_KERNEL;
    }
    
    p_nfy_info=(struct nfy_usr_info *)kcu_malloc(sizeof(struct nfy_usr_info)+u4TagSize,allocflags);
    p_nfy_info->nfy_header.eFctId=nfy_id;
    p_nfy_info->nfy_header.u4TagSize=u4TagSize;
    #if KCU_MEASURE_TIME 
    do_gettimeofday(&(p_nfy_info->nfy_header.send_tm));
    #endif
    
    #if KCU_CHK_SUM
    p_nfy_info->nfy_header.sum=calsum(pvArg,u4TagSize);
    #endif
    atomic_set(&p_nfy_info->usage,1);
    
    if(u4TagSize)
    {
        memcpy((u8 *)p_nfy_info+sizeof(struct nfy_usr_info),pvArg,u4TagSize);
    }
    
    info.si_errno=0;
    info.si_code=SI_QUEUE; //use SI_QUEUE to copy all info to user space see copy_siginfo_to_user
    info.si_pid=current->pid;
    info.si_uid=0;   
    info.si_ptr=p_nfy_info;  
    
    nfyid_head=_get_tgid_head_by_nfyid(nfy_id);
    if(IS_ERR(nfyid_head))
    {
        rcu_read_unlock();
        ref_dec_and_free(p_nfy_info,&p_nfy_info->usage); 
        ret=PTR_RET(nfyid_head);
        return ret;
    }
    list_for_each_entry_rcu(map_entry, &nfyid_head->list, list)
    {
        if(!test_bit((bsync!=0),&map_entry->mask))continue;
        task_head=_get_task_head_by_pid(map_entry->tgid,bsync);
        if(IS_ERR(task_head))
        {
            rcu_read_unlock();
            continue;
        }
           tsk=_signal_to_idle_task(task_head,&info,&p_nfy_info->usage,ret_size);
           ret=PTR_RET(tsk);
        rcu_read_unlock();        
        if(bsync)break;//now only support to send synchronous call to only one process
    }
    
    if(bsync && (ret==0) && (nfy_id!=NFY_USR_THD_EXIT))
    {
        data=(struct notifier_data *)(tsk->notifier_data);
        rcu_read_unlock();
        wait_for_completion(data->done);
        trace_kcu_nfy_ret(nfy_id);
        if(ptr_ret && ret_size)
        {
            memcpy(ptr_ret,data->retbuf,ret_size);
        }
    }
    else
    {
        rcu_read_unlock();
    }
    
    ref_dec_and_free(p_nfy_info,&p_nfy_info->usage);
    return ret;
}
int kcu_notify_sync(NFY_USR_ID nfy_id,void * pvArg,unsigned int u4TagSize,void * ptr_ret,unsigned long ret_size)
{
    return kcu_notify_user(nfy_id,pvArg,u4TagSize,1,ptr_ret,ret_size);
}
int kcu_notify_async(NFY_USR_ID nfy_id,void * pvArg,unsigned int u4TagSize)
{
    return kcu_notify_user(nfy_id,pvArg,u4TagSize,0,NULL,0);
}
EXPORT_SYMBOL(kcu_notify_sync);
EXPORT_SYMBOL(kcu_notify_async);
#endif

