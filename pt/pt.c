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
#include <linux/debugfs.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <asm/processor.h>	
#include <linux/tracepoint.h>

#include "pt.h"

#define MAXTHREAD 0x08
#define PTEN 0x02
#define MEGNUM 0x08
#define MAX_MSG 1024

#define NETLINK_USER 31 //Only allow one proxy to be attached
#define DEM ":"

enum proxy_status {
	PSLEEP = 0,
	PSTART,
	PTOPA,
	PFUZZ,
	UNKNOWN
};

enum msg_etype{
	START = 0,
	TARGET,
	PTBUF,
	ERROR
};

struct topa_entry {
	u64 end:1;
	u64 rsvd0:1;
	u64 intr:1;
	u64 rsvd1:1;
	u64 stop:1;
	u64 rsvd2:1;
	u64 size:4;
	u64 rsvd3:2;
	u64 base:36;
	u64 rsvd4:16;
};

//data struct for ToPA
typedef struct topa_struct{
	struct topa_entry entries[PTEN];
}topa_t; 

//data structure for a thread in the fuzzed process
typedef struct target_thread_struct{
	pid_t pid; 
	struct task_struct *task; 
	topa_t  topa; 
}target_thread_t;


//Data structure for PT capability
typedef struct pt_cap_struct{
	bool has_pt;  // if pt is included  
	bool has_topa;	// if pt supports ToPA
	bool cr3_match; // if pt can filter based on CR3
	
 	int topa_num;	//number of ToPA entried	
	int addr_range_num; //number of address ranges for filtering	
	int psb_freq_mask; //psb packets frequency mast (psb->boundary packets)
}pt_cap_t; 

typedef struct pt_manager_struct{
	enum proxy_status p_stat;

	pid_t proxy_pid; //process id of proxy

	char target_path[PATH_MAX]; //path of target program

	pid_t fserver_pid;  //process id of fork server
	pid_t tgid; //thread group id of the fuzzed process (the pid of the master thread)

	target_thread_t targets[MAXTHREAD]; //array for threads in the fuzzed process
	int target_num; //how many threads are in running
}pt_manager_t;



typedef struct netlink_struct{
	struct sock *nl_sk;
	struct netlink_kernel_cfg cfg;  
}netlink_t;

static void pt_recv_msg(struct sk_buff *skb);

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

static enum msg_etype msg_type(char * msg){
	
	if(strstr(msg, "START"))
		return START;

	if(strstr(msg, "TARGET"))
		return TARGET;

	return ERROR; 	
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


static void probe_trace_exec(void * arg, struct task_struct *p, pid_t old_pid, struct linux_binprm *bprm){
	return;
}

static void probe_trace_switch(void *ignore, bool preempt, struct task_struct *prev, struct task_struct *next){
	return;
}


static void probe_trace_fork(void *ignore, struct task_struct *parent, struct task_struct * child){
	return;
}

static void probe_trace_exit(void * ignore, struct task_struct *tsk){
	return;
}

static bool set_trace_point(void){

	//trace on exec
	exec_tp = (struct tracepoint*) kallsyms_lookup_name("__tracepoint_sched_process_exec");
	
	if(!exec_tp)
		return false; 
	
	//trace fork of process
	fork_tp =  (struct tracepoint*) kallsyms_lookup_name("__tracepoint_sched_process_fork");	 
	
	if(!fork_tp)
		return false; 

	//trace process switch
	switch_tp = (struct tracepoint*) kallsyms_lookup_name("__tracepoint_sched_switch");
	
	if(!switch_tp)
		return false; 

	//trace exit of process
	exit_tp =  (struct tracepoint*) kallsyms_lookup_name("__tracepoint_sched_process_exit");
	
	if(!exit_tp)
		return false; 

	tracepoint_probe_register(exec_tp, probe_trace_exec, NULL); 
	tracepoint_probe_register(fork_tp, probe_trace_fork, NULL);
	tracepoint_probe_register(switch_tp, probe_trace_switch, NULL);
	tracepoint_probe_register(exit_tp, probe_trace_exit, NULL); 
	
	return true;
}


//all these communications are sequential. No lock needed. 
static void pt_recv_msg(struct sk_buff *skb) {

	struct nlmsghdr *nlh;
	int pid;
	char msg[MAX_MSG];

	//receive new data
	nlh=(struct nlmsghdr*)skb->data;
	strncpy(msg, (char*)nlmsg_data(nlh), MAX_MSG);
	pid = nlh->nlmsg_pid; /*pid of sending process */

	//check the message from proxy and then reply with the corresponding result
	switch(msg_type(msg)){
		case START:
			if (ptm.p_stat != PSLEEP){
				reply_msg("ERROR: Alread Started", pid); 	
				break; 	
			}	
			ptm.p_stat = PSTART;
			ptm.proxy_pid = pid;
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
		
			ptm.p_stat = PTOPA;
 	
			//Get the target binary path from the message
			strncpy(ptm.target_path, strstr(msg, DEM)+1, PATH_MAX);

			//set trace point on execv,fork,schedule,exit. 
			if(!set_trace_point()){
				ptm.p_stat = UNKNOWN;	
				reply_msg("ERROR: Cannot register trace point. Sorry", pid);
				break;
			}	

			reply_msg("TCONFIRM", pid);
			printk(KERN_INFO "Target %s\n", ptm.target_path);	
			break;

		//PT buf format: "BUF:0x........." (8 bytes, since we are on 64 bit machine)
		case PTBUF:
			if(ptm.p_stat != PTOPA){
				reply_msg("ERROR: PT Buffer Address Expected\n", pid);		
				break;

			}

			break;	

		case ERROR:
			reply_msg("ERROR: No such command!\n", pid); 
			break;
	}
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
		printk(KERN_INFO "The CPU does not meet requirement\n");
		return ENODEV;  	
	}
	//next step: enable PT?  

	nlt.nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &nlt.cfg);

	return 0;
}


static void __exit pt_exit(void){
	printk(KERN_INFO "end test pt test\n");
	netlink_kernel_release(nlt.nl_sk);
}

module_init(pt_init);
module_exit(pt_exit);
