#include <linux/module.h>
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
#include <asm/apic.h>
#include <asm/nmi.h>
#include "pt.h"

#define MSR_LVT 0x834
#define MSR_IA32_PERF_GLOBAL_STATUS 0x0000038e
#define MSR_IA32_PERF_GLOBAL_CTRL 	0x0000038f
#define MSR_GLOBAL_STATUS_RESET 0x390

#define read_global_status() native_read_msr(MSR_IA32_PERF_GLOBAL_STATUS)
#define read_global_status_reset() native_read_msr(MSR_GLOBAL_STATUS_RESET)
#define echo_pt_int() wrmsrl(MSR_GLOBAL_STATUS_RESET, BIT(55)) 

//kernel module parameters
static unsigned long kallsyms_lookup_name_ptr;
module_param(kallsyms_lookup_name_ptr, ulong, 0400);
MODULE_PARM_DESC(kallsyms_lookup_name_ptr, "Set address of function kallsyms_lookup_name_ptr (for kernels without CONFIG_KALLSYMS_ALL)");

/* unsigned long (*ksyms_func)(const char *name) = NULL; */
ksyms_func_ptr_ty ksyms_func = NULL;
static void pt_recv_msg(struct sk_buff *skb);
static void release_trace_point(void);

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

#define RESET_TARGET(tx) ptm.targets[tx].status = TEXIT

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

static struct tracepoint *exec_tp = NULL; 
static struct tracepoint *switch_tp= NULL; 
static struct tracepoint *fork_tp= NULL;
static struct tracepoint *exit_tp= NULL; 

static int get_target_tx(pid_t pid){
	int tx; 
	for(tx = 0; tx < ptm.target_num; tx++){
		if (ptm.targets[tx].pid == pid)
			return tx; 
	}
	return -1; 
}

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
	      printk(KERN_INFO "Error while sending bak [%s] to user [%d]\n", msg,pid);
}

//set up the topa table.
//each entry with 4MB space
static bool do_setup_topa(topa_t *topa)
{
	void *raw; 
	int index; 
	int it; 

	//create the first 30 entries with real space and no interrupt 
	for(index = 0; index < PTINT; index++){
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

	topa->entries[index++] = TOPA_ENTRY(virt_to_phys(raw), TOPA_ENTRY_UNIT_SIZE, 0, 0, 0);

	//Creat the last entry with end bit set
	//Init a circular buffer
	topa->entries[index] =  TOPA_ENTRY(virt_to_phys(topa), 0, 0, 0, 1);
	return true; 

//In case of failure, free all the pages		
fail: 	
	for(it = 0; it < index; it++)
		free_pages((long unsigned int)phys_to_virt(topa->entries[it].base << PAGE_SHIFT),  TOPA_ENTRY_UNIT_SIZE);
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

	//remap each topa to the address space of proxy
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

	int tx; 
	struct vm_area_struct *vma;
	topa_t *topa;
	
	//check if any target can be reused
	//only need to reset the pid, task, status, offset, and outmask
	for(tx = 0; tx < ptm.target_num; tx++){
		if(ptm.targets[tx].status == TEXIT){
			printk(KERN_INFO "Reuse ToPA for target %x\n", target->pid);
			ptm.targets[tx].pid = target->pid; 
			ptm.targets[tx].task = target;
			ptm.targets[tx].status = TSTART;
			ptm.targets[tx].offset = 0;
			ptm.targets[tx].outmask = 0;
			return true; 
		}		
	}

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

		printk(KERN_INFO "Fork server path %s and pid %d\n", bprm->filename, p->pid);
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

	char target_msg[MAX_MSG];	
	int tx;

	//the fork is invoked by the forkserver
	if(parent->pid == ptm.fserver_pid){
		if(ptm.p_stat != PTARGET && ptm.p_stat != PFUZZ)
			return;			

		//Fork a thread? Does this really happen? 
		if(parent->mm == child->mm)
			return;
		
		if(!setup_target_thread(child)){
			reply_msg("ERROR:TOPA", ptm.proxy_pid);
			return; 
		}
		
		printk(KERN_INFO "Start Target %d\n", child->pid);

		if(ptm.p_stat == PTARGET){
			tx = get_target_tx(child->pid);
			snprintf(target_msg, MAX_MSG, "TOPA:0x%lx:0x%lx", (long unsigned)ptm.targets[tx].pva, VMA_SZ);
			printk(KERN_INFO "TART_MESSGAE %s\n", target_msg);
			reply_msg(target_msg, ptm.proxy_pid);
		}
		ptm.p_stat = PFUZZ;
	}		
	return;
}

//register handler for process exit
//take care of the target process and the proxy process
static void probe_trace_exit(void * ignore, struct task_struct *tsk){

	int tx;
	int index;
	topa_t *topa;

	for(tx = 0; tx < ptm.target_num; tx++){

		//exit of a target thread
		if(ptm.targets[tx].pid == tsk->pid){
			//record the offset, as the thread may not have been switched out yet
			record_pt(tx);
			printk(KERN_INFO "Exit of target thread %x and offset %lx\n", tsk->pid, (unsigned long)ptm.targets[tx].offset);
			RESET_TARGET(tx);
		}	
	}

	//exit of the proxy process	
	if(tsk->pid == ptm.proxy_pid){

		printk(KERN_INFO "Exit of the proxy process\n");
		ptm.p_stat = PSLEEP;  
	
		for(tx = 0; tx < ptm.target_num; tx++){
			RESET_TARGET(tx);
			topa = ptm.targets[tx].topa;

			//free topa entries
			for(index = 0; index < PTEN - 1; index++)
				free_pages((long unsigned int)phys_to_virt(topa->entries[index].base << PAGE_SHIFT), TOPA_ENTRY_UNIT_SIZE);
			
			//free topa
			free_pages((unsigned long)topa, TOPA_T_SIZE);
			ptm.targets[tx].topa = NULL;
		}
		//reset target number
		ptm.target_num = 0;
		release_trace_point();

	}	
	return;
}


//set up the trace point to handle exec, fork, switch, and exit
static bool set_trace_point(void){

	trace_probe_ptr_ty trace_probe_ptr;

	//trace on exec
	exec_tp = (struct tracepoint*) ksyms_func("__tracepoint_sched_process_exec");	
	if(!exec_tp) return false; 

	//trace fork of process
	fork_tp =  (struct tracepoint*) ksyms_func("__tracepoint_sched_process_fork");	 
	if(!fork_tp) return false; 

	//trace process switch
	switch_tp = (struct tracepoint*) ksyms_func("__tracepoint_sched_switch");
	if(!switch_tp) return false; 

	//trace exit of process
	exit_tp =  (struct tracepoint*) ksyms_func("__tracepoint_sched_process_exit");
	if(!exit_tp) return false; 

	trace_probe_ptr = (trace_probe_ptr_ty)ksyms_func("tracepoint_probe_register");
	if(!trace_probe_ptr)
		return false;

	trace_probe_ptr(exec_tp, probe_trace_exec, NULL); 
	trace_probe_ptr(fork_tp, probe_trace_fork, NULL);
	trace_probe_ptr(switch_tp, probe_trace_switch, NULL);
	trace_probe_ptr(exit_tp, probe_trace_exit, NULL); 

	return true;
}

static void release_trace_point(void){


	trace_release_ptr_ty trace_release_ptr;

	printk(KERN_INFO "Release trace point\n");
	trace_release_ptr = (trace_release_ptr_ty)ksyms_func("tracepoint_probe_unregister"); 
	WARN_ON(!trace_release_ptr);

	if(exec_tp){
		trace_release_ptr(exec_tp, (void*)probe_trace_exec, NULL);
		exec_tp = NULL;
	}

	if(fork_tp){
		trace_release_ptr(fork_tp, (void*)probe_trace_fork, NULL);
		fork_tp = NULL; 
	}

	if(switch_tp){
		trace_release_ptr(switch_tp, (void*)probe_trace_switch, NULL);
		switch_tp = NULL;
	}

	if(exit_tp){
		trace_release_ptr(exit_tp, (void*)probe_trace_exit, NULL);		
		exit_tp = NULL;
	}

}

static enum msg_etype msg_type(char * msg){
	
	if(strstr(msg, "START"))
		return START;

	if(strstr(msg, "TARGET"))
		return TARGET;

	if(strstr(msg, "NEXT"))
		return NEXT;

	return ERROR; 	
}

static void process_next_msg(char *msg_recvd, char*msg_send){
    //get next boundary
    //check if it is under interupt, if so, deal with interrupt
    u64 coff; 	
    int tx; 
    siginfo_t sgt;
    int (*force_sig_info)(int sig, struct siginfo *info, struct task_struct *t);

    coff = 0;
    kstrtoull(strstr(msg_recvd, DEM)+1, 16, &coff);
    printk("Received next message %s and %llx\n", msg_recvd, coff);

    force_sig_info = proxy_find_symbol("force_sig_info");	
    if(!force_sig_info){
        printk("force_sig_info is null\n");
        return;
    }
	
    for(tx = 0; tx < ptm.target_num; tx++){
        if(ptm.targets[tx].status == TSTART || 
           ptm.targets[tx].status == TRUN){
            snprintf(msg_send, MAX_MSG, "NEXT:0x%lx", (unsigned long)ptm.targets[tx].offset);		
            printk("Sending next message on running %s\n", msg_send);
            return;
        }
        //process the interrupt status
        if(ptm.targets[tx].status == TINT){
            if(coff == ptm.targets[tx].offset){
                //continue the target
                ptm.targets[tx].offset = (u64)0;
                force_sig_info(SIGCONT, &sgt, ptm.targets[tx].task);
                ptm.targets[tx].status = TRUN;	
                snprintf(msg_send, MAX_MSG, "NEXT:0x%lx", (unsigned long)0);				
                printk("Sending next message on interrupt %s\n", msg_send);
            }else{
                snprintf(msg_send, MAX_MSG, "NEXT:0x%lx", (unsigned long)ptm.targets[tx].offset);		
                printk("Sending next message on interupt release %s\n", msg_send);
            }
            return;
        }
		
        if(ptm.targets[tx].status == TEXIT){
            snprintf(msg_send, MAX_MSG, "NEXT:0x%lx", (unsigned long)ptm.targets[tx].offset);			
            printk("Sending next message on exit %s\n", msg_send);
	    return;
        }
    }
    printk("Sending next message %s\n", msg_send);
    snprintf(msg_send, MAX_MSG, "ERROR:NO TARHET");
}


//all these communications are sequential. No lock needed. 
static void pt_recv_msg(struct sk_buff *skb) {

	struct nlmsghdr *nlh;
	int pid;
	char msg[MAX_MSG];
	char next_msg[MAX_MSG];
	struct task_struct* (*find_task_by_vpid)(pid_t nr);

	//receive new data
	nlh=(struct nlmsghdr*)skb->data;
	strncpy(msg, (char*)nlmsg_data(nlh), MAX_MSG);

	pid = nlh->nlmsg_pid; /*pid of sending process */
	find_task_by_vpid = proxy_find_symbol("find_task_by_vpid");
	WARN_ON(!find_task_by_vpid);

	switch(msg_type(msg)){
		case START:
			if (ptm.p_stat != PSLEEP){
				reply_msg("ERROR: Alread Started",pid); 				break; 	
			}
	
			ptm.p_stat = PSTART;
			ptm.proxy_pid = pid;
			ptm.proxy_task = find_task_by_vpid(pid); 

			printk(KERN_INFO "Proxy start with PID %d\n", pid);
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

			//Get the target binary path from the message
			strncpy(ptm.target_path, strstr(msg, DEM)+1, PATH_MAX);
			//set trace point on execv,fork,schedule,exit. 
			if(!set_trace_point()){
				ptm.p_stat = UNKNOWN;	
				reply_msg("ERROR: Cannot register trace point. Sorry", pid);
				break;
			}	

			ptm.p_stat = PFS;
			reply_msg("TCONFIRM", pid);
			break;

		//Recv: NEXT:0xprevboundary
		//Send: NEXT:0xnextboundary	
		case NEXT:
			process_next_msg(msg, next_msg);
			reply_msg(next_msg, pid); 
			break;

		case ERROR:
			reply_msg("ERROR: No such command!\n", pid); 
			break;
	}
}


//Process PMI interrupt when PT buffer is full
static int pt_nmi_handler(unsigned int cmd, struct pt_regs *regs)
{
	int tx; 
	u64 status;  
	siginfo_t sgt; 
	int (*force_sig_info)(int sig, struct siginfo *info, struct task_struct *t);

	//find the symbol for force_sig_info
	force_sig_info = proxy_find_symbol("force_sig_info");
	WARN_ON(!force_sig_info);

	//get the status MSR to distinguish PT PMI
	status = read_global_status();
	for(tx = 0; tx < ptm.target_num; tx++){
		if(ptm.targets[tx].pid == current->pid)
		{
			//the 55th bit is set
			if(status & BIT_ULL(55)){
				printk(KERN_INFO "NMI TRIGGERED %llx\n", status);
				//stop the target thread
				force_sig_info(SIGSTOP, &sgt, current);
			}
		}
	}
	return 0;
}

static int register_pmi_handler(void) {
	register_nmi_handler(NMI_LOCAL, pt_nmi_handler, NMI_FLAG_FIRST, "perf_pt");
	return 0;
}

void unregister_pmi_handler(void){
	void (*unregister_nmi_handler)(unsigned int type, const char *name);
	unregister_nmi_handler = proxy_find_symbol("unregister_nmi_handler");
	if (unregister_nmi_handler)
		unregister_nmi_handler(NMI_LOCAL, "perf_pt");
	else
		printk(KERN_INFO "unregister_nmi_handler is null\n");
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
	ksyms_func = (ksyms_func_ptr_ty)kallsyms_lookup_name_ptr; 

	//register the PMI handler	
	register_pmi_handler();
	//create a netlink server
	nlt.nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &nlt.cfg);
	return 0;
}

static void __exit pt_exit(void){

	unregister_pmi_handler();

	release_trace_point();
	//release the netlink	
	netlink_kernel_release(nlt.nl_sk);
}

module_init(pt_init);
module_exit(pt_exit);
MODULE_LICENSE("GPL");
