#ifndef __PT_H__
#define __PT_H__
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/list.h>

#define MSR_IA32_RTIT_OUTPUT_BASE	0x00000560
#define MSR_IA32_RTIT_OUTPUT_MASK_PTRS	0x00000561
#define MSR_IA32_RTIT_CTL		0x00000570

#define TRACE_EN	BIT_ULL(0)
#define CYC_EN		BIT_ULL(1)
#define CTL_OS		BIT_ULL(2)
#define CTL_USER	BIT_ULL(3)
#define PT_ERROR	BIT_ULL(4)
#define CR3_FILTER	BIT_ULL(7)
#define TO_PA		BIT_ULL(8)
#define MTC_EN		BIT_ULL(9)
#define TSC_EN		BIT_ULL(10)
#define DIS_RETC	BIT_ULL(11)
#define BRANCH_EN	BIT_ULL(13)
#define MTC_MASK	(0xf << 14)
#define CYC_MASK	(0xf << 19)
#define PSB_MASK	(0xf << 24)
#define ADDR0_SHIFT	32
#define ADDR1_SHIFT	36
#define ADDR0_MASK	(0xfULL << ADDR0_SHIFT)
#define ADDR1_MASK	(0xfULL << ADDR1_SHIFT)
#define ADDR0_CFG	((u64)1 << ADDR0_SHIFT)

#define MSR_IA32_RTIT_STATUS		0x00000571
#define MSR_IA32_CR3_MATCH		0x00000572

#define TOPA_STOP	BIT_ULL(4)
#define TOPA_INT	BIT_ULL(2)
#define TOPA_END	BIT_ULL(0)

#define TOPA_SIZE_SHIFT 6
#define MSR_IA32_ADDR0_START		0x00000580
#define MSR_IA32_ADDR0_END		0x00000581
#define MSR_IA32_ADDR1_START		0x00000582
#define MSR_IA32_ADDR1_END		0x00000583

#define DEF_TOPA_NUM			36

#define MAXTHREAD 0x08

#define PTEN 0x41 //64 entries for ToPA // 256 MB in total
#define PTINT PTEN-3

#define TOPANROM 0x1e
#define TOPABACK 0x2
#define TOPAEND 0x1

#define MEGNUM 0x08
#define MAX_MSG 256

#define NETLINK_USER 31 
#define DEM ":"

#define TOPA_ENTRY_SIZE_4K 0
#define TOPA_ENTRY_SIZE_8K 1
#define TOPA_ENTRY_SIZE_16K 2
#define TOPA_ENTRY_SIZE_32K 3
#define TOPA_ENTRY_SIZE_64K 4
#define TOPA_ENTRY_SIZE_128K 5
#define TOPA_ENTRY_SIZE_256K 6
#define TOPA_ENTRY_SIZE_512K 7
#define TOPA_ENTRY_SIZE_1M 8
#define TOPA_ENTRY_SIZE_2M 9
#define TOPA_ENTRY_SIZE_4M 10
#define TOPA_ENTRY_SIZE_8M 11
#define TOPA_ENTRY_SIZE_16M 12
#define TOPA_ENTRY_SIZE_32M 13
#define TOPA_ENTRY_SIZE_64M 14
#define TOPA_ENTRY_SIZE_128M 15
#define TOPA_ENTRY_UNIT_SIZE TOPA_ENTRY_SIZE_4M	
#define TOPA_ENTRY_SIZE_BASE TOPA_ENTRY_SIZE_2M
#define TOPA_ENTRY_SIZE_ALL TOPA_ENTRY_SIZE_4M

#define TOPA_BUFFER_SIZE (1 << (12 + TOPA_ENTRY_SIZE_BASE))
#define TOPA_T_SIZE TOPA_ENTRY_SIZE_8K

#define VMA_SZ (1 << TOPA_ENTRY_UNIT_SIZE) * (PTEN - 1) * PAGE_SIZE

enum proxy_status {
	PSLEEP = 0,
	PSTART,
	PFS,
	PTARGET,
	PFUZZ,
	UNKNOWN
};

enum msg_etype{
	START = 0,
	TARGET,
	ERROR,
	NEXT
};

enum tar_status{
	TSTART = 0,
	TRUN,
	TINT,
	TEXIT
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

//Data structure for PT capability
typedef struct pt_cap_struct{
	bool has_pt;  // if pt is included  
	bool has_topa;	// if pt supports ToPA
	bool cr3_match; // if pt can filter based on CR3
	
 	int topa_num;	//number of ToPA entried	
	int addr_range_num; //number of address ranges for filtering	
	int psb_freq_mask; //psb packets frequency mast (psb->boundary packets)
}pt_cap_t; 

//data structure for a thread in the fuzzed process
typedef struct target_thread_struct{
	pid_t pid; 
	struct task_struct *task; 
	topa_t  *topa; 
	enum tar_status status; 

	u64 pva; //virtual address for proxy to access	
	u64 offset; //offset in the PT buffer 		
	u64 run_cnt; 
	u64 outmask;

	//proxy offset address	
	u64 poa; 
	u64 pca;
	
	//start address of executable .text
	u64 addr_range_a; 
	//end address of executbale .text
	u64 addr_range_b;



}target_thread_t;

typedef struct pt_manager_struct{

  struct list_head next_ptm;//pointer for next ptm

	enum proxy_status p_stat;

	pid_t proxy_pid; //process id of proxy
	struct task_struct* proxy_task; 


	char target_path[PATH_MAX]; //path of target program

	pid_t fserver_pid;  //process id of fork server
	pid_t tgid; //thread group id of the fuzzed process (the pid of the master thread)

	target_thread_t targets[MAXTHREAD]; //array for threads in the fuzzed process
	int target_num; //how many threads are in running
	u64 run_cnt; 	
	

  u64 timer_interval;

	//if filtering based on address is enabled
	bool addr_filter; 
}pt_manager_t;


// typedef struct reverse_map_st{
//   pid_t target;

// }

//global singleton for mantaining pt module
typedef struct pt_facotry_struct{
  struct list_head ptm_list;//TODO: lock
  bool trace_point_init; //TODO: lock
  u64 ptm_num; //TODO: lock
}pt_factory_t;

typedef struct netlink_struct{
	struct sock *nl_sk;
	struct netlink_kernel_cfg cfg;  
}netlink_t;

extern pt_factory_t *pt_factory;

extern unsigned long (*ksyms_func)(const char *name);

unsigned long proxy_unmapped_area(struct task_struct *task, struct file * file, unsigned long addr, unsigned long len, unsigned long pgoff, unsigned long flags);

struct vm_area_struct *proxy_special_mapping(
	struct mm_struct *mm,
	unsigned long addr, unsigned long len,
	unsigned long vm_flags);


void * proxy_find_symbol(char * name);
void record_pt(pt_manager_t *, int tx);
void resume_pt(pt_manager_t *, int tx);

typedef int (*trace_probe_ptr_ty)(struct tracepoint *tp, void *probe, void *data);
typedef int (*trace_release_ptr_ty)(struct tracepoint *tp, void *probe, void *data); 
typedef unsigned long (*ksyms_func_ptr_ty)(const char *name);

#endif

