#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <linux/dcache.h>
#include <linux/ctype.h>
#include <linux/syscore_ops.h>
#include <trace/events/sched.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/errno.h>
#include <linux/tracepoint.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <asm/processor.h>	
#include <asm/mman.h>
#include <linux/mman.h>
#include <linux/tracepoint.h>
#include "pt.h"

//kernel module parameters
static unsigned long kallsyms_lookup_name_ptr;
module_param(kallsyms_lookup_name_ptr, ulong, 0400);
MODULE_PARM_DESC(kallsyms_lookup_name_ptr, "Set address of function kallsyms_lookup_name_ptr (for kernels without CONFIG_KALLSYMS_ALL)");



unsigned long (*ksyms_func)(const char *name) = NULL;
static void pt_recv_msg(struct sk_buff *skb);

#define TOPA_ENTRY(_base, _size, _stop, _intr, _end) (struct topa_entry) { \
	.base = (_base) >> 12, \
	.size = (_size), \
	.stop = (_stop), \
	.intr = (_intr), \
	.end = (_end), \
}

#define INIT_TARGET(_pid, _task, _topa, _status, _pva, _offset, _mask) (target_thread_t) {\
	.pid = _pid, \
	.task = _task, \
	.topa = _topa, \
	.status = _status, \
	.pva = _pva, \
	.offset = _offset,\
	.outmask = _mask, \
}


pt_cap_t pt_cap = {
	.has_pt = false,
	.has_topa = false,
	.cr3_match = false,

	.topa_num = DEF_TOPA_NUM,
	.addr_range_num = 0,
	.psb_freq_mask = 0
};

//data struct for netlink management 
netlink_t nlt ={
	.nl_sk = NULL,
	.cfg = {
		.input = pt_recv_msg,
	},
};


pt_manager_t ptm = {
	.p_stat = PSLEEP,
	.target_num = 0,
};

static struct tracepoint *exec_tp; 
static struct tracepoint *switch_tp; 
static struct tracepoint *fork_tp;
static struct tracepoint *exit_tp; 

//query CPU ID to check capability of PT
static void query_pt_cap(void){
	
	unsigned a, b, c, d;
	unsigned a1, b1, c1, d1;
	//CPU id is not high enough to support PT?
	cpuid(0, &a, &b, &c, &d);
	if(a < 0x14){
		pt_cap.has_pt = false;
		return;
	}
	//CPU does not support PT?
	cpuid_count(0x07, 0, &a, &b, &c, &d);

	if (!(b & BIT(25))) {
		pt_cap.has_pt = false; 
		return;
	}
	pt_cap.has_pt = true; 

	//CPU has no ToPA for pt?
	cpuid_count(0x14, 0, &a, &b, &c, &d);
	pt_cap.has_topa = (c & BIT(0)) ? true : false;  

	//PT supports filtering  by CR3?
	pt_cap.cr3_match = (b & BIT(0)) ? true : false;

	if (!(c & BIT(1)))
		pt_cap.topa_num = 1;

	pt_cap.topa_num = min_t(unsigned, pt_cap.topa_num,
			       (PAGE_SIZE / 8) - 1);
	a1 = b1 = c1 = d1 = 0;
	if (a >= 1) cpuid_count(0x14, 1, &a1, &b1, &c1, &d1);
	if (b & BIT(1)) {
		pt_cap.psb_freq_mask= (b1 >> 16) & 0xffff;
		pt_cap.addr_range_num = a1 & 0x3;
	}
}

static bool check_pt(void){

	if(!pt_cap.has_pt){
		printk(KERN_INFO "No PT support\n");
		return false; 
	}

	if(!pt_cap.has_topa){
		printk(KERN_INFO "No ToPA support\n");
		return false; 
	}

	if(!pt_cap.cr3_match){
		printk(KERN_INFO "No filtering based on CR3\n");
		return false; 
	}	

	printk("The PT supports %d ToPA entries and %d address ranges for filtering\n", pt_cap.topa_num, pt_cap.addr_range_num);

	return true; 
}

static void reply_msg(char *msg, pid_t pid){

	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	char reply[MAX_MSG];
	int res;

	skb_out = nlmsg_new(MAX_MSG, 0);
		
	if(!skb_out){
		printk(KERN_ERR "Failed to allocate new skb\n");
		return;
	}
 
	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,MAX_MSG,0);  
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

	strncpy(reply, msg, MAX_MSG);
	strncpy(nlmsg_data(nlh),reply,MAX_MSG);

	res = nlmsg_unicast(nlt.nl_sk,skb_out,pid);

	if(res<0)
		printk(KERN_INFO "Error while sending bak to user\n");
}

//set up the topa table.
//each entry with 4MB space
static bool do_setup_topa(topa_t *topa)
{
	void *raw; 
	int index; 
	int it; 

	//create the first 30 entries with real space and no interrupt 
	for(index = 0; index < PTEN - 1; index++){
		raw =	(void*)__get_free_pages(GFP_KERNEL,TOPA_ENTRY_UNIT_SIZE);
		if(!raw) goto fail; 

		topa->entries[index] = TOPA_ENTRY(virt_to_phys(raw), TOPA_ENTRY_UNIT_SIZE, 0, 0, 0);
	}


	//create the 31th entry with real spce and interrupt
	raw = (void*)__get_free_pages(GFP_KERNEL,TOPA_ENTRY_UNIT_SIZE);
	if(!raw) goto fail; 

	topa->entries[index++] = TOPA_ENTRY(virt_to_phys(raw), TOPA_ENTRY_UNIT_SIZE, 0, 1, 0);


	//create the 32th entry as the backup area
	raw = (void*)__get_free_pages(GFP_KERNEL,TOPA_ENTRY_UNIT_SIZE);
	if(!raw) goto fail; 

	topa->entries[index++] = TOPA_ENTRY(virt_to_phys(raw), TOPA_ENTRY_UNIT_SIZE, 0, 1, 0);

	//Creat the last entry with end bit set
	//Init a circular buffer
	topa->entries[PTEN-TOPAEND] =  TOPA_ENTRY(virt_to_phys(topa), 0, 0, 0, 1);
	return true; 

//In case of failure, free all the pages		
fail: 	
	for(it = 0; it < index; it++)
		free_pages((long unsigned int)phys_to_virt(topa->entries[it].base),  TOPA_ENTRY_UNIT_SIZE);
	return false; 
}

//allocate the TOPA and the space for entries
static topa_t* pt_alloc_topa(void){

	topa_t *topa;
	topa = (topa_t *) __get_free_pages(GFP_KERNEL, TOPA_T_SIZE); 
	if(!topa)
		goto fail; 
	if(!do_setup_topa(topa))	
		goto free_topa; 
	return topa; 	
free_topa: 
	free_pages((unsigned long)topa, TOPA_T_SIZE);
fail:
	return NULL;
}


static  struct vm_area_struct* setup_proxy_vma(topa_t *topa){

	mm_segment_t fs; 
	struct vm_area_struct *vma;
	u64 mapaddr; 
	int index; 
	
	fs = get_fs();
	set_fs(get_ds());

	//Insert a vma into the Proxy address space
	vma = proxy_special_mapping(ptm.proxy_task->mm, 0, VMA_SZ, VM_READ);
	
	if(IS_ERR(vma))
		return NULL;

	mapaddr = vma->vm_start; 
	if(IS_ERR((void*)mapaddr))
		return NULL;
	set_fs(fs);	

	for(index = 0; index < PTEN - 1; index++)
		remap_pfn_range(vma, 
			mapaddr + index * (1 << TOPA_ENTRY_UNIT_SIZE) * PAGE_SIZE , 
			topa->entries[index].base, 
			(1 << TOPA_ENTRY_UNIT_SIZE ) *PAGE_SIZE, 
			vma->vm_page_prot);
	return vma; 
}

//A new target thread is started. 
//Set up the ToPA 
static bool setup_target_thread(struct task_struct *target){

	struct vm_area_struct *vma;
	topa_t *topa;

	topa = pt_alloc_topa();
	if(!topa) return false; 

	vma = setup_proxy_vma(topa);
	if(!vma) return false; 

	printk(KERN_INFO "Address of VMA for proxy %lx\n", vma->vm_start);
	//need a lock here when multiple target threads are running. 
	ptm.targets[ptm.target_num] = INIT_TARGET(target->pid, target, topa, TSTART, vma->vm_start, 0, 0); 
	ptm.target_num++;
	return true; 
}

//Check if the forkserver is started by matching the target path
static void probe_trace_exec(void * arg, struct task_struct *p, pid_t old_pid, struct linux_binprm *bprm){

	if( 0 == strncmp(bprm->filename, ptm.target_path, PATH_MAX)){
		if(ptm.p_stat != PFS)
			return;

		printk(KERN_INFO "Fork server path %s\n", bprm->filename);
		ptm.fserver_pid = p->pid;	
		ptm.p_stat = PTARGET; 		
	}
	return;
}
static void probe_trace_switch(void *ignore, bool preempt, struct task_struct *prev, struct task_struct *next){
	
	int tx; 

	if(preempt)
		preempt_disable();

	for(tx = 0; tx < ptm.target_num; tx++){
		if(ptm.targets[tx].pid == prev->pid){
			record_pt(tx);
			break;
		}
	}

	for(tx = 0; tx < ptm.target_num; tx++){
		if(ptm.targets[tx].pid == next->pid){
			resume_pt(tx);
		}		
	}

	if(preempt)
		preempt_enable();

	return;
}

//when the fork server forks, create a TOPA, and send it to the proxy server. 
//on context switch, tracing the target process
static void probe_trace_fork(void *ignore, struct task_struct *parent, struct task_struct * child){

	//the fork is invoked by the forkserver
	if(parent->pid == ptm.fserver_pid){
		if(ptm.p_stat != PTARGET)
			return;			
		//create_topa
		//register_topa
		//update ptm

		//Fork a thread? Does this really happen? 
		if(parent->mm == child->mm)
			return;
		
		if(!setup_target_thread(child)){
			reply_msg("ERROR:TOPA", ptm.proxy_pid);
			return; 
		}
		
		printk(KERN_INFO "Start Target\n");

		reply_msg("TOPA", ptm.proxy_pid);
		ptm.p_stat = PFUZZ;
	}		
	//check if the fork is from the target 

	return;
}

static void probe_trace_exit(void * ignore, struct task_struct *tsk){
	return;
}

static bool set_trace_point(void){

	int (*trace_probe_ptr)(struct tracepoint *tp, void *probe, void *data);
	if(!kallsyms_lookup_name_ptr){
		printk(KERN_INFO "Please specify the address of kallsyms_lookup_name_ptr");
	}

	//trace on exec
	exec_tp = (struct tracepoint*) ksyms_func("__tracepoint_sched_process_exec");	
	if(!exec_tp)
		return false; 
	
	//trace fork of process
	fork_tp =  (struct tracepoint*) ksyms_func("__tracepoint_sched_process_fork");	 
	if(!fork_tp)
		return false; 

	//trace process switch
	switch_tp = (struct tracepoint*) ksyms_func("__tracepoint_sched_switch");
	if(!switch_tp)
		return false; 

	//trace exit of process
	exit_tp =  (struct tracepoint*) ksyms_func("__tracepoint_sched_process_exit");
	if(!exit_tp)
		return false; 

       trace_probe_ptr = ksyms_func("tracepoint_probe_register");

	if(!trace_probe_ptr)
		return false;

	trace_probe_ptr(exec_tp, probe_trace_exec, NULL); 
	trace_probe_ptr(fork_tp, probe_trace_fork, NULL);
	trace_probe_ptr(switch_tp, probe_trace_switch, NULL);
	trace_probe_ptr(exit_tp, probe_trace_exit, NULL); 
	return true;
}

static void release_trace_point(void){
	
	int (*trace_release_ptr)(struct tracepoint *tp, void *probe, void *data); 

	if(!ksyms_func){
		printk(KERN_INFO "FFF Cannot unregister trace points!!!");
		return;	
	}

	trace_release_ptr = ksyms_func("tracepoint_probe_unregister"); 
	if(!trace_release_ptr){
		printk(KERN_INFO "Cannot unregister trace points!!!");
		return;
	}

	if(exec_tp)
		trace_release_ptr(exec_tp, (void*)probe_trace_exec, NULL);
	if(fork_tp)
		trace_release_ptr(fork_tp, (void*)probe_trace_fork, NULL);
	if(switch_tp)
		trace_release_ptr(switch_tp, (void*)probe_trace_switch, NULL);
	if(exit_tp)
		trace_release_ptr(exit_tp, (void*)probe_trace_exit, NULL);
}

static enum msg_etype msg_type(char * msg){
	
	if(strstr(msg, "START"))
		return START;

	if(strstr(msg, "TARGET"))
		return TARGET;

	if(strstr(msg, "TEST"))
		return TEST;

	return ERROR; 	
}

//all these communications are sequential. No lock needed. 
static void pt_recv_msg(struct sk_buff *skb) {

	struct nlmsghdr *nlh;
	int pid;
	char msg[MAX_MSG];
	struct task_struct* (*find_task_by_vpid)(pid_t nr);
	int tx;


	//receive new data
	nlh=(struct nlmsghdr*)skb->data;
	strncpy(msg, (char*)nlmsg_data(nlh), MAX_MSG);
	pid = nlh->nlmsg_pid; /*pid of sending process */

	find_task_by_vpid = proxy_find_symbol("find_task_by_vpid");

	if(!find_task_by_vpid){
		reply_msg("ERROR: Cannot get task of proxy", pid); 	
		return; 	
	}

	switch(msg_type(msg)){
		case START:
			if (ptm.p_stat != PSLEEP){
				reply_msg("ERROR: Alread Started", pid); 	
				break; 	
			}	
			ptm.p_stat = PSTART;
			ptm.proxy_pid = pid;

			ptm.proxy_task = find_task_by_vpid(pid); 
			//confirm start
			reply_msg("SCONFIRM", pid); 
			break;

		//Target menssage format: "TARGET:/path/to/bin" 
		case TARGET: 
			if(ptm.p_stat != PSTART){
				reply_msg("ERROR: Cannot attach target", pid);
				break;
			}
			if(!strstr(msg, DEM)){
				reply_msg("ERROR: Target format not correct", pid);
				break;
			}

			ptm.p_stat = PFS;
			//Get the target binary path from the message
			strncpy(ptm.target_path, strstr(msg, DEM)+1, PATH_MAX);
			//set trace point on execv,fork,schedule,exit. 

			if(!set_trace_point()){
				ptm.p_stat = UNKNOWN;	
				reply_msg("ERROR: Cannot register trace point. Sorry", pid);
				break;
			}	

			reply_msg("TCONFIRM", pid);
			break;

		case PTBUF:
			break;	
		
		case TEST:
			for(tx = 0; tx < ptm.target_num; tx++)
				printk(KERN_INFO "Offset of %d target %llx\n", tx, ptm.targets[tx].offset);	
			break;

		case ERROR:
			reply_msg("ERROR: No such command!\n", pid); 
			break;
	}
}

//Init pt 
//1. Check if pt is supported 
//2. Check capabilities
//3. Set up hooking point through Linux trace API
static int __init pt_init(void){

	//query pt_cap
	query_pt_cap();

	//check if PT meets basic requirements
	//1. Has PT
	//2. support ToPA
	//3. can do cr3-based filtering 
	if(!check_pt()){
		printk(KERN_INFO "The CPU has insufficient PT\n");
		return ENODEV;  	
	}
	//next step: enable PT?  

	if(!kallsyms_lookup_name_ptr){
		printk(KERN_INFO "Please specify the address to kallsyms_lookup_name\n");
	}

	ksyms_func = kallsyms_lookup_name_ptr; 
	nlt.nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &nlt.cfg);
	return 0;
}

static void __exit pt_exit(void){
	
	//release the netlink	
	netlink_kernel_release(nlt.nl_sk);
	//release the trace points
	if(ksyms_func);
		release_trace_point();
}

module_init(pt_init);
module_exit(pt_exit);
