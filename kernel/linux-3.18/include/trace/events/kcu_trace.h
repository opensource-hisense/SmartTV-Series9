/*
 * If TRACE_SYSTEM is defined, that will be the directory created
 * in the ftrace directory under /sys/kernel/debug/tracing/events/<system>
 *
 * The define_trace.h below will also look for a file name of
 * TRACE_SYSTEM.h where TRACE_SYSTEM is what is defined here.
 * In this case, it would look for sample.h
 *
 * If the header name will be different than the system name
 * (as in this case), then you can override the header name that
 * define_trace.h will look up by defining TRACE_INCLUDE_FILE
 *
 * This file is called trace-events-sample.h but we want the system
 * to be called "sample". Therefore we must define the name of this
 * file:
 *
 * #define TRACE_INCLUDE_FILE trace-events-sample
 *
 * As we do an the bottom of this file.
 *
 * Notice that TRACE_SYSTEM should be defined outside of #if
 * protection, just like TRACE_INCLUDE_FILE.
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM kcu

/*
 * Notice that this file is not protected like a normal header.
 * We also must allow for rereading of this file. The
 *
 *  || defined(TRACE_HEADER_MULTI_READ)
 *
 * serves this purpose.
 */
#if !defined(_KCU_TRACE_EVENT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _KCU_TRACE_EVENT_H

#include <linux/sched.h>
/*
 * All trace headers should include tracepoint.h, until we finally
 * make it into a standard header.
 */
#include <linux/tracepoint.h>

/*
 * The TRACE_EVENT macro is broken up into 5 parts.
 *
 * name: name of the trace point. This is also how to enable the tracepoint.
 *   A function called trace_foo_bar() will be created.
 *
 * proto: the prototype of the function trace_foo_bar()
 *   Here it is trace_foo_bar(char *foo, int bar).
 *
 * args:  must match the arguments in the prototype.
 *    Here it is simply "foo, bar".
 *
 * struct:  This defines the way the data will be stored in the ring buffer.
 *    There are currently two types of elements. __field and __array.
 *    a __field is broken up into (type, name). Where type can be any
 *    type but an array.
 *    For an array. there are three fields. (type, name, size). The
 *    type of elements in the array, the name of the field and the size
 *    of the array.
 *
 *    __array( char, foo, 10) is the same as saying   char foo[10].
 *
 * fast_assign: This is a C like function that is used to store the items
 *    into the ring buffer.
 *
 * printk: This is a way to print out the data in pretty print. This is
 *    useful if the system crashes and you are logging via a serial line,
 *    the data can be printed to the console using this "printk" method.
 *
 * Note, that for both the assign and the printk, __entry is the handler
 * to the data structure in the ring buffer, and is defined by the
 * TP_STRUCT__entry.
 */
TRACE_EVENT(kcu_nfy_prepare,

	TP_PROTO(int nfy_id,unsigned char bsync),

	TP_ARGS(nfy_id,bsync),

	TP_STRUCT__entry(
		__field(	int,	nfy_id)
		__field(	unsigned char,bsync)
	),

	TP_fast_assign(
		__entry->nfy_id	= nfy_id;		
		__entry->bsync	= bsync;
	),

	TP_printk("%s notify id=%d",__entry->bsync?"sync":"async", __entry->nfy_id)
);
TRACE_EVENT(kcu_nfy_start,

	TP_PROTO(struct task_struct *to,int live_cnt,void *info),

	TP_ARGS(to,live_cnt,info),

	TP_STRUCT__entry(
		__field(	int,	live_cnt)
        __array(	char,	comm,	TASK_COMM_LEN	)
        __field(	pid_t,	pid			)
        __field(	void *,	info_ptr			)        
	),

	TP_fast_assign(
		__entry->live_cnt	= live_cnt;		
        memcpy(__entry->comm, to->comm, TASK_COMM_LEN);
		__entry->pid	= to->pid;
		__entry->info_ptr	= info;
	),

	TP_printk("info_ptr=%p comm=%s pid=%d live_cnt=%d" ,__entry->info_ptr,__entry->comm,__entry->pid,__entry->live_cnt)
);
TRACE_EVENT(kcu_nfy_process,

	TP_PROTO(int nfy_id,void *buf,void *info),

	TP_ARGS(nfy_id,buf,info),

	TP_STRUCT__entry(
		__field(	int,	nfy_id)
        __field(	void *,	buf			)
        __field(	void *,	info_ptr			)         
	),

	TP_fast_assign(
		__entry->nfy_id	= nfy_id;		
		__entry->buf	= buf;
		__entry->info_ptr	= info;
	),

	TP_printk("info_ptr=%p notify id=%d usr_buf=%p" ,__entry->info_ptr,__entry->nfy_id,__entry->buf)
);
TRACE_EVENT(kcu_nfy_done,

	TP_PROTO(void * ret_buf,int live_cnt),

	TP_ARGS(ret_buf,live_cnt),

	TP_STRUCT__entry(
		__field(	int,	live_cnt)
        __field(	void *,	ret_buf	)
	),

	TP_fast_assign(
		__entry->live_cnt	= live_cnt;		
		__entry->ret_buf	= ret_buf;       
	),

	TP_printk("live_cnt=%d ret_buf=%p" ,__entry->live_cnt,__entry->ret_buf)
);
TRACE_EVENT(kcu_nfy_ret,

	TP_PROTO(int nfy_id),

	TP_ARGS(nfy_id),

	TP_STRUCT__entry(
		__field(	int,	nfy_id)
	),

	TP_fast_assign(
		__entry->nfy_id	= nfy_id;
	),

	TP_printk("notify id=%d", __entry->nfy_id)
);
#endif

/***** NOTICE! The #if protection ends here. *****/


/*
 * There are several ways I could have done this. If I left out the
 * TRACE_INCLUDE_PATH, then it would default to the kernel source
 * include/trace/events directory.
 *
 * I could specify a path from the define_trace.h file back to this
 * file.
 *
 * #define TRACE_INCLUDE_PATH ../../samples/trace_events
 *
 * But the safest and easiest way to simply make it use the directory
 * that the file is in is to add in the Makefile:
 *
 * CFLAGS_trace-events-sample.o := -I$(src)
 *
 * This will make sure the current path is part of the include
 * structure for our file so that define_trace.h can find it.
 *
 * I could have made only the top level directory the include:
 *
 * CFLAGS_trace-events-sample.o := -I$(PWD)
 *
 * And then let the path to this directory be the TRACE_INCLUDE_PATH:
 *
 * #define TRACE_INCLUDE_PATH samples/trace_events
 *
 * But then if something defines "samples" or "trace_events" as a macro
 * then we could risk that being converted too, and give us an unexpected
 * result.
 */
//#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
//#define TRACE_INCLUDE_PATH .
/*
 * TRACE_INCLUDE_FILE is not needed if the filename and TRACE_SYSTEM are equal
 */
#define TRACE_INCLUDE_FILE kcu_trace
#include <trace/define_trace.h>
