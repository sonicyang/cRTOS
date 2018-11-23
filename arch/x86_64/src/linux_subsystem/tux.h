#ifndef __LINUX_SUBSYSTEM_TUX_H
#define __LINUX_SUBSYSTEM_TUX_H

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include "up_internal.h"

#define PAGE_SLOT_SIZE 0x1000000

#define FUTEX_WAIT 0x0
#define FUTEX_WAKE 0x1
#define FUTEX_PRIVATE_FLAG 0x80

struct rlimit {
  int rlim_cur;  /* Soft limit */
  int rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};

void*   find_free_slot(void);
uint64_t tux_delegate(unsigned long nbr, uintptr_t parm1, uintptr_t parm2,
                          uintptr_t parm3, uintptr_t parm4, uintptr_t parm5,
                          uintptr_t parm6);

int     tux_nanosleep   (unsigned long nbr, const struct timespec *rqtp, struct timespec *rmtp);

int     tux_clone       (unsigned long nbr, unsigned long flags, void *child_stack,
                         void *ptid, void *ctid, unsigned long tls);

void*   tux_mmap        (unsigned long nbr, void* addr, size_t length, int prot, int flags);
int     tux_munmap      (unsigned long nbr, void* addr, size_t length);

int     tux_getrlimit   (unsigned long nbr, int resource, struct rlimit *rlim);

int     tux_set_tid_address     (unsigned long nbr, int* tidptr);
void    tux_set_tid_callback    (int val, void* arg);

void*   tux_brk         (unsigned long nbr, void* brk);

int     tux_arch_prctl       (unsigned long nbr, int code, unsigned long addr);

int     tux_futex       (unsigned long nbr, uint32_t* uaddr, int opcode, uint32_t val);

static inline int tux_success_stub(void){
    return 0;
}

static inline int tux_fail_stub(void){
    return -1;
}

#endif//__LINUX_SUBSYSTEM_TUX_H
