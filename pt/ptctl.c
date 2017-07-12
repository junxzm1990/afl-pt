#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <linux/ctype.h>
#include <asm/errno.h>
#include <linux/tracepoint.h>
#include <linux/sched.h>
#include <asm/processor.h>	
#include <asm/mman.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include "pt.h"

#define MSR_IA32_RTIT_CTL		0x00000570
#define RTIT_CTL_TRACEEN		BIT(0)
#define RTIT_CTL_OS			BIT(2)
#define RTIT_CTL_USR			BIT(3)
#define RTIT_CTL_CR3EN			BIT(7)
#define RTIT_CTL_TOPA			BIT(8)
#define RTIT_CTL_TSC_EN			BIT(10)
#define RTIT_CTL_DISRETC		BIT(11)
#define RTIT_CTL_BRANCH_EN		BIT(13)
#define MSR_IA32_RTIT_STATUS		0x00000571
#define RTIT_STATUS_CONTEXTEN		BIT(1)
#define RTIT_STATUS_TRIGGEREN		BIT(2)
#define RTIT_STATUS_ERROR		BIT(4)
#define RTIT_STATUS_STOPPED		BIT(5)
#define MSR_IA32_RTIT_CR3_MATCH		0x00000572
#define MSR_IA32_RTIT_OUTPUT_BASE	0x00000560
#define MSR_IA32_RTIT_OUTPUT_MASK	0x00000561

#define pt_enabled() (native_read_msr(MSR_IA32_RTIT_CTL) & RTIT_CTL_TRACEEN)

//For the following, please refer to Intel mannual
#define pt_topa_base() native_read_msr(MSR_IA32_RTIT_OUTPUT_BASE)
#define pt_topa_index() ((native_read_msr(MSR_IA32_RTIT_OUTPUT_MASK) \
			& 0xffffffff) >> 7)
#define pt_topa_offset() (native_read_msr(MSR_IA32_RTIT_OUTPUT_MASK) >> 32)

#define pt_topa_mask() native_read_msr(MSR_IA32_RTIT_OUTPUT_MASK)

static void pt_pause(void) {
		wrmsrl(MSR_IA32_RTIT_CTL, native_read_msr(MSR_IA32_RTIT_CTL) & ~RTIT_CTL_TRACEEN);
}

static void pt_setup_msr(topa_t *topa, u64 mask)
{
	wrmsrl(MSR_IA32_RTIT_STATUS, 0);
	wrmsrl(MSR_IA32_RTIT_OUTPUT_BASE, virt_to_phys(topa));
	wrmsrl(MSR_IA32_RTIT_OUTPUT_MASK, mask);
	wrmsrl(MSR_IA32_RTIT_CTL, 
			(RTIT_CTL_TRACEEN | RTIT_CTL_TOPA
			| RTIT_CTL_BRANCH_EN | RTIT_CTL_USR)
			& (~(TSC_EN | CYC_EN | MTC_EN)));
}

void record_pt(int tx){
	unsigned index; 
	unsigned offset;
	u64 mask; 

	if(pt_enabled())
		pt_pause();	

	index = pt_topa_index();
	offset = pt_topa_offset();
	mask = pt_topa_mask();	

	ptm->targets[tx].offset = index * (1 << TOPA_ENTRY_UNIT_SIZE) * PAGE_SIZE + offset;

	ptm->targets[tx].outmask = mask;
}

void restart_pt(int tx){

	if(pt_enabled())
		pt_pause();

	pt_setup_msr(ptm->targets[tx].topa, 0);
}


void resume_pt(int tx){

	if(pt_enabled())
		pt_pause();

	pt_setup_msr(ptm->targets[tx].topa, ptm->targets[tx].outmask);
}

