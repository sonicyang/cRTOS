#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>
#include <nuttx/sched.h>

#include <errno.h>

#include "tux.h"
#include "up_internal.h"
#include "sched/sched.h"

#include <arch/irq.h>
#include <sys/mman.h>

#define STR(x) #x
#define XSTR(s) STR(s)

# define CSIGNAL       0x000000ff /* Signal mask to be sent at exit.  */
# define CLONE_VM      0x00000100 /* Set if VM shared between processes.  */
# define CLONE_FS      0x00000200 /* Set if fs info shared between processes.  */
# define CLONE_FILES   0x00000400 /* Set if open files shared between processes.  */
# define CLONE_SIGHAND 0x00000800 /* Set if signal handlers shared.  */
# define CLONE_THREAD  0x00010000 /* Set to add to same thread group.  */
# define CLONE_SETTLS  0x00080000 /* Set TLS info.  */
# define CLONE_PARENT_SETTID 0x00100000 /* Store TID in userlevel buffer
					   before MM copy.  */
# define CLONE_CHILD_CLEARTID 0x00200000 /* Register exit futex and memory
					    location to clear.  */
# define CLONE_CHILD_SETTID 0x01000000 /* Store TID in userlevel buffer in
					  the child.  */

static inline void* new_memory_block(uint64_t size, void** virt) {
    void* ret;
    ret = gran_alloc(tux_mm_hnd, size);
    *virt = temp_map_at_0xc0000000(ret, ret + size);
    memset(*virt, 0, size);
    return ret;
}

void clone_trampoline(void* regs) {
    uint64_t regs_on_stack[16];
    svcinfo("Entering Clone Trampoline\n");

    struct tcb_s *rtcb = this_task();
    struct vma_s *ptr;
    for(ptr = rtcb->xcp.vma; ptr; ptr = ptr->next){
        tux_delegate(9, (((uint64_t)ptr->pa_start) << 32) | (uint64_t)(ptr->va_start), VMA_SIZE(ptr),
                     0, MAP_ANONYMOUS, 0, 0);
    }

    // Move the regs onto the stack and free the memory
    memcpy(regs_on_stack, regs, sizeof(uint64_t) * 16);
    kmm_free(regs);

    // Call a stub to jump to the actual entry point
    // It loads the same registers as the original task
    fork_kickstart(regs_on_stack);

    _exit(255); // We should never end up here
}

int tux_clone(unsigned long nbr, unsigned long flags, void *child_stack,
              void *ptid, void *ctid,
              unsigned long tls){

  int ret;
  struct task_tcb_s *tcb;
  struct tcb_s *rtcb = this_task();
  void* stack;
  struct vma_s *ptr, *ptr2, *pptr, *pda_ptr;
  uint64_t i;
  void* virt_mem;
  uint64_t *regs;

  int* orig_child_tid_ptr;
  int orig_tid;

  tcb = (FAR struct task_tcb_s *)kmm_zalloc(sizeof(struct task_tcb_s));
  if (!tcb)
    return -1;

  stack = kmm_zalloc(0x8000); //Kernel stack
  if(!stack)
    return -1;

  ret = task_init((FAR struct tcb_s *)tcb, "clone_thread", rtcb->init_priority,
                  (uint32_t*)stack, 0x8000, NULL, NULL);
  if (ret < 0)
  {
    ret = -get_errno();
    berr("task_init() failed: %d\n", ret);
    goto errout_with_tcb;
  }

  /* Check the flags */
  /* XXX: Ignore CLONE_FS */
  /* XXX: Ignore CLONE_FILES */
  /* XXX: Ignore CLONE_SIGHAND */
  /* Ignore CLONE_VSEM, not sure how to properly handle this */

  if(flags & CLONE_SETTLS){
    tcb->cmn.xcp.fs_base_set = 1;
    tcb->cmn.xcp.fs_base = (uint64_t)tls;
  }

  if(flags & CLONE_CHILD_SETTID){
    orig_tid = *(uint32_t*)(ctid);
    *(uint32_t*)(ctid) = tcb->cmn.pid;
  }

  if(flags & CLONE_CHILD_CLEARTID){
    orig_child_tid_ptr = _tux_set_tid_address((struct tcb_s*)tcb, (int*)(ctid));
  }

  /* Clone the VM */
  if(flags & CLONE_VM){
      tcb->cmn.xcp.vma = rtcb->xcp.vma;
      tcb->cmn.xcp.pda = rtcb->xcp.pda;

      /* manual set the stack pointer */
      tcb->cmn.xcp.regs[REG_RSP] = (uint64_t)child_stack;

      /* manual set the instruction pointer */
      tcb->cmn.xcp.regs[REG_RIP] = *((uint64_t*)(get_kernel_stack_ptr()) - 2); // Directly leaves the syscall
  }else{

    /* copy our mapped memory, including stack, to the new process */

    struct vma_s* mapping = NULL;
    struct vma_s* curr;

    for(ptr = rtcb->xcp.vma; ptr; ptr = ptr->next){
        if(ptr->pa_start != 0xffffffff) {
            curr = kmm_zalloc(sizeof(struct vma_s));
            curr->next = mapping;
            mapping = curr;
        }
    }

    tcb->cmn.xcp.vma = mapping;

    svcinfo("Copy mappings\n");
    for(ptr = rtcb->xcp.vma, curr = tcb->cmn.xcp.vma; ptr; ptr = ptr->next){
        if(ptr->pa_start == 0xffffffff){
            continue;
        }

        curr->va_start = ptr->va_start;
        curr->va_end = ptr->va_end;
        curr->proto = ptr->proto;

        curr->_backing = kmm_zalloc(strlen(ptr->_backing) + 1);
        strcpy(curr->_backing, ptr->_backing);

        curr->pa_start = new_memory_block(VMA_SIZE(ptr), &virt_mem);
        memcpy(virt_mem, ptr->va_start, VMA_SIZE(ptr));

        svcinfo("Mapping: %llx - %llx: %llx %s\n", ptr->va_start, ptr->va_end, curr->pa_start, curr->_backing);

        curr = curr->next;
    }

    svcinfo("Copy pdas\n");
    tcb->cmn.xcp.pda = pda_ptr = kmm_zalloc(sizeof(struct vma_s));

    for(ptr = tcb->cmn.xcp.vma; ptr;){
        // Scan hole with continuous addressing and same proto
        for(pptr = ptr, ptr2 = ptr->next; ptr2 && (((pptr->va_end + HUGE_PAGE_SIZE - 1) & HUGE_PAGE_MASK) >= (ptr2->va_start & HUGE_PAGE_MASK)) && (pptr->proto == ptr2->proto); pptr = ptr2, ptr2 = ptr2->next){
            svcinfo("Boundary: %llx and %llx\n", ((pptr->va_end + HUGE_PAGE_SIZE - 1) & HUGE_PAGE_MASK), (ptr2->va_start & HUGE_PAGE_MASK));
            svcinfo("Merge: %llx - %llx and %llx - %llx\n", pptr->va_start, pptr->va_end, ptr2->va_start, ptr2->va_end);

        }

        svcinfo("PDA Mapping: %llx - %llx\n", ptr->va_start, pptr->va_end);

        pda_ptr->va_start = ptr->va_start & HUGE_PAGE_MASK;
        pda_ptr->va_end = (pptr->va_end + HUGE_PAGE_SIZE - 1) & HUGE_PAGE_MASK; //Align up
        pda_ptr->proto = ptr->proto; // All proto are the same
        pda_ptr->_backing = "";

        pda_ptr->pa_start = new_memory_block(VMA_SIZE(pda_ptr) / HUGE_PAGE_SIZE * PAGE_SIZE, &virt_mem);
        do{
            for(i = ptr->va_start; i < ptr->va_end; i += PAGE_SIZE){
                ((uint64_t*)(virt_mem))[((i - pda_ptr->va_start) >> 12) & 0x3ffff] = (ptr->pa_start + i - ptr->va_start) | ptr->proto;
            }
            ptr = ptr->next;
        }while(ptr != pptr->next);

        if(ptr){
            pda_ptr->next = kmm_zalloc(sizeof(struct vma_s));
            pda_ptr = pda_ptr->next;
        }
    }

    svcinfo("All mapped\n");

    // set brk
    tcb->cmn.xcp.__min_brk = rtcb->xcp.__min_brk;
    tcb->cmn.xcp.__brk = rtcb->xcp.__brk;

    if(!(flags & CLONE_SETTLS)) {
        tcb->cmn.xcp.fs_base_set = rtcb->xcp.fs_base_set;
        tcb->cmn.xcp.fs_base = rtcb->xcp.fs_base;
    }

    /* manual set the instruction pointer */
    regs = kmm_zalloc(sizeof(uint64_t) * 16);
    memcpy(regs, (uint64_t*)(get_kernel_stack_ptr()) - 16, sizeof(uint64_t) * 16);
    tcb->cmn.xcp.regs[REG_RDI] = regs;
    tcb->cmn.xcp.regs[REG_RIP] = clone_trampoline; // We need to manage the memory mapping
    /* stack is the new kernel stack */

    tcb->cmn.xcp.linux_tcb = tux_delegate(57, 0, 0, 0, 0, 0, 0); // Get a new shadow process
    tcb->cmn.xcp.is_linux = 2; /* This is the head of threads, responsible to scrap the addrenv */
    nxsem_init(&tcb->cmn.xcp.syscall_lock, 1, 0);
    nxsem_setprotocol(&tcb->cmn.xcp.syscall_lock, SEM_PRIO_NONE);

    add_remote_on_exit((struct tcb_s*)tcb, tux_on_exit, NULL);
  }

  /* set it after copying the memory to child */
  if(flags & CLONE_PARENT_SETTID){
    *(uint32_t*)(ptid) = rtcb->pid;
  }

  /* restore the parent memory */
  if((flags & CLONE_CHILD_SETTID) && !(flags & CLONE_VM)){
    *(uint32_t*)(ctid) = orig_tid;
  }

  if((flags & CLONE_CHILD_CLEARTID) && !(flags & CLONE_VM)){
    _tux_set_tid_address((struct tcb_s*)tcb, orig_child_tid_ptr);
  }

  svcinfo("Cloned a task with RIP=0x%llx, RSP=0x%llx, kstack=0x%llx\n",
          tcb->cmn.xcp.regs[REG_RIP],
          tcb->cmn.xcp.regs[REG_RSP],
          stack);

  /* clone return 0 to child */
  tcb->cmn.xcp.regs[REG_RAX] = 0;

  sinfo("activate: new task=%d\n", tcb->cmn.pid);
  /* Then activate the task at the provided priority */
  ret = task_activate((FAR struct tcb_s *)tcb);
  if (ret < 0)
  {
    ret = -get_errno();
    berr("task_activate() failed: %d\n", ret);
    goto errout_with_tcbinit;
  }

  return tcb->cmn.pid;

errout_with_tcbinit:
    sched_releasetcb(&tcb->cmn, TCB_FLAG_TTYPE_TASK);
    return -1;

errout_with_tcb:
    kmm_free(tcb);
    kmm_free(stack);
    return -1;
}

