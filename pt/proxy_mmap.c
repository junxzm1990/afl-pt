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

void * proxy_find_symbol(char * name){
	if(!ksyms_func)
		return NULL;
	return (void*)ksyms_func(name);	
}

unsigned long pp_unmapped_area(struct task_struct *task, struct vm_unmapped_area_info *info)
{

	struct mm_struct *mm = task->mm;
	struct vm_area_struct *vma;
	unsigned long length, low_limit, high_limit, gap_start, gap_end;

	/* Adjust search length to account for worst case alignment overhead */
	length = info->length + info->align_mask;
	if (length < info->length)
		return -ENOMEM;

	/* Adjust search limits by the desired length */
	if (info->high_limit < length)
		return -ENOMEM;
	high_limit = info->high_limit - length;

	if (info->low_limit > high_limit)
		return -ENOMEM;
	low_limit = info->low_limit + length;

	/* Check if rbtree root looks promising */
	if (RB_EMPTY_ROOT(&mm->mm_rb))
		goto check_highest;
	vma = rb_entry(mm->mm_rb.rb_node, struct vm_area_struct, vm_rb);
	if (vma->rb_subtree_gap < length)
		goto check_highest;

	while (true) {
		/* Visit left subtree if it looks promising */
		gap_end = vma->vm_start;
		if (gap_end >= low_limit && vma->vm_rb.rb_left) {
			struct vm_area_struct *left =
				rb_entry(vma->vm_rb.rb_left,
					 struct vm_area_struct, vm_rb);
			if (left->rb_subtree_gap >= length) {
				vma = left;
				continue;
			}
		}

		gap_start = vma->vm_prev ? vma->vm_prev->vm_end : 0;
check_current:
		/* Check if current node has a suitable gap */
		if (gap_start > high_limit)
			return -ENOMEM;
		if (gap_end >= low_limit && gap_end - gap_start >= length)
			goto found;

		/* Visit right subtree if it looks promising */
		if (vma->vm_rb.rb_right) {
			struct vm_area_struct *right =
				rb_entry(vma->vm_rb.rb_right,
					 struct vm_area_struct, vm_rb);
			if (right->rb_subtree_gap >= length) {
				vma = right;
				continue;
			}
		}

		/* Go back up the rbtree to find next candidate node */
		while (true) {
			struct rb_node *prev = &vma->vm_rb;
			if (!rb_parent(prev))
				goto check_highest;
			vma = rb_entry(rb_parent(prev),
				       struct vm_area_struct, vm_rb);
			if (prev == vma->vm_rb.rb_left) {
				gap_start = vma->vm_prev->vm_end;
				gap_end = vma->vm_start;
				goto check_current;
			}
		}
	}

check_highest:
	/* Check highest gap, which does not precede any rbtree node */
	gap_start = mm->highest_vm_end;
	gap_end = ULONG_MAX;  /* Only for VM_BUG_ON below */
	if (gap_start > high_limit)
		return -ENOMEM;

found:
	/* We found a suitable gap. Clip it with the original low_limit. */
	if (gap_start < info->low_limit)
		gap_start = info->low_limit;

	/* Adjust gap address to the desired alignment */
	gap_start += (info->align_offset - gap_start) & info->align_mask;

	VM_BUG_ON(gap_start + info->length > info->high_limit);
	VM_BUG_ON(gap_start + info->length > gap_end);
	return gap_start;
}


unsigned long
proxy_get_unmapped_area(struct task_struct *task, struct file* file, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{

	struct mm_struct *mm = task->mm;
	struct vm_unmapped_area_info info;

	if (flags & MAP_FIXED)
		return addr;

	info.flags = 0;
	info.length = len;
	info.low_limit = mm->mmap_base;
	info.high_limit = TASK_SIZE;
	info.align_mask = 0;
	return pp_unmapped_area(task, &info);
}



unsigned long proxy_unmapped_area(struct task_struct *task, struct file * file, unsigned long addr, unsigned long len, unsigned long pgoff, unsigned long flags)
{

	/* Careful about overflows.. */
	if (len > TASK_SIZE)
		return -ENOMEM;

	addr = proxy_get_unmapped_area(task, file, addr, len, pgoff, flags);
	if (IS_ERR_VALUE(addr))
		return addr;
	if (addr > TASK_SIZE - len)
		return -ENOMEM;
	if (offset_in_page(addr))
		return -EINVAL;

	return addr;
}

struct vm_area_struct *proxy_special_mapping(
	struct mm_struct *mm,
	unsigned long addr, unsigned long len,
	unsigned long vm_flags)
{
	int ret;
	struct vm_area_struct *vma;
	struct kmem_cache *vm_area_cachep;
	int (*insert_vm_struct)(struct mm_struct * mm, struct vm_area_struct *vma);
		
	vm_area_cachep = *((struct kmem_cache **)proxy_find_symbol("vm_area_cachep"));
	if(!vm_area_cachep){
		printk(KERN_INFO "Cannot locate VMA cache\n");
		return ERR_PTR(-ENOMEM);
	}

	vma = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
	if (unlikely(vma == NULL)){
		printk(KERN_INFO "Cannot allocate VMA\n");
		return ERR_PTR(-ENOMEM);
	}

	INIT_LIST_HEAD(&vma->anon_vma_chain);
	vma->vm_mm = mm;

	if(!addr){
		addr = proxy_unmapped_area(mm->owner, NULL, 0, len, 0, vm_flags);
	}

	if(IS_ERR_VALUE(addr)){
		ret = addr; 
		goto out; 
	}
	
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_flags = vm_flags | mm->def_flags | VM_DONTEXPAND | VM_SOFTDIRTY;
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	insert_vm_struct = proxy_find_symbol("insert_vm_struct");
	
	if(!insert_vm_struct){
		printk(KERN_INFO "Cannot insert VMA\n");
		return ERR_PTR(-ENOMEM);
	}

	ret = insert_vm_struct(mm, vma);
	if (ret){
		printk("VMA is not inserted\n");
		goto out;
	}
	mm->total_vm += len >> PAGE_SHIFT;
	return vma;
out:
	kmem_cache_free(vm_area_cachep, vma);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(proxy_special_mapping);

