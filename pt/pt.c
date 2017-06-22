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

#include "pt.h"

#define MAXTHREAD 0x08
#define PTEN 0x02


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


typedef struct pt_manager_struct{
	pid_t proxy_pid; //process id of proxy

	char target_path[PATH_MAX]; //path of target program

	pid_t fserver_pid;  //process id of fork server

	pid_t tgid; //thread group id of the fuzzed process (the pid of the master thread)

	target_thread_t targets[MAXTHREAD]; //array for threads in the fuzzed process

	int target_num; //how many threads are in running
	
	//todo list: create a set of locks for synchronizations

}pt_manager_t;


//Data structure for PT capability
typedef struct pt_cap_struct{
	bool has_pt;  // if pt is included  
	bool has_topa;	// if pt supports ToPA
	bool cr3_match; // if pt can filter based on CR3
	
 	int topa_num;	//number of ToPA entried	
	int addr_range_num; //number of address ranges for filtering	
	int psb_freq_mask; //psb packets frequency mast (psb->boundary packets)
}pt_cap_t; 



pt_cap_t pt_cap = {
	.has_pt = false,
	.has_topa = false,
	.cr3_match = false,

	.topa_num = DEF_TOPA_NUM,
	.addr_range_num = 0,
	.psb_freq_mask = 0
};


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


#define NETLINK_USER 31
struct sock *nl_sk = NULL;

static void hello_nl_recv_msg(struct sk_buff *skb) {

	struct nlmsghdr *nlh;
	int pid;
	struct sk_buff *skb_out;
	int msg_size;
	char *msg="Hello from kernel";
	int res;

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	msg_size=strlen(msg);

	nlh=(struct nlmsghdr*)skb->data;
	printk(KERN_INFO "Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
	pid = nlh->nlmsg_pid; /*pid of sending process */

	skb_out = nlmsg_new(msg_size,0);

	if(!skb_out)
	{

		printk(KERN_ERR "Failed to allocate new skb\n");
		return;

	} 
	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);  
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(nlh),msg,msg_size);

	res=nlmsg_unicast(nl_sk,skb_out,pid);

	if(res<0)
		printk(KERN_INFO "Error while sending bak to user\n");
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


	struct netlink_kernel_cfg cfg = {
		.input = hello_nl_recv_msg,
	};

	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

	return 0;
}


static void __exit pt_exit(void){
	printk(KERN_INFO "end test pt test\n");
	netlink_kernel_release(nl_sk);
}

module_init(pt_init);
module_exit(pt_exit);


