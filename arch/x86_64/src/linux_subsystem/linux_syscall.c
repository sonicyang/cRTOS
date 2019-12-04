/****************************************************************************
 *  arch/x86_64/src/broadwell/broadwell_linux_subsystem.c
 *
 *   Copyright (C) 2011-2012, 2014-2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <nuttx/arch.h>
#include <nuttx/sched.h>
#include <nuttx/board.h>
#include <nuttx/irq.h>
#include <arch/io.h>
#include <syscall.h>
#include <semaphore.h>
#include <errno.h>

#include "up_internal.h"
#include "sched/sched.h"

#include "tux.h"
#include "tux_syscall_table.h"

syscall_t linux_syscall_action_table[500] = {
    tux_file_delegate, // SYS_read
    tux_file_delegate, // SYS_write
    tux_open_delegate, // SYS_open
    tux_file_delegate, // SYS_close
    tux_delegate, // SYS_stat
    tux_file_delegate, // SYS_fstat
    tux_delegate, // sys_lstat
    (syscall_t)tux_poll, // SYS_poll,
    tux_file_delegate, // SYS_lseek,
    (syscall_t)tux_mmap,
    (syscall_t)tux_success_stub, // XXX: SYS_mprotect Missing logic, how to protect flat memory space
    (syscall_t)tux_munmap,
    (syscall_t)tux_brk,
    (syscall_t)tux_rt_sigaction, // SYS_re_sigaction
    tux_local,
    tux_no_impl, // SYS_sigreturn,
    tux_file_delegate, // SYS_ioctl,
    tux_file_delegate, // SYS_pread,
    tux_file_delegate, // SYS_pwrite,
    tux_file_delegate, // sys_readv
    tux_file_delegate, // sys_writev
    tux_delegate, // sys_access
    (syscall_t)tux_pipe, // sys_pipe
    (syscall_t)tux_select, // sys_select
    tux_local,
    (syscall_t)tux_mremap, // sys_mremap
    (syscall_t)tux_success_stub, // sys_msync
    (syscall_t)tux_success_stub, // sys_mincore
    (syscall_t)tux_success_stub, // sys_madvise
    (syscall_t)tux_shmget, // SYS_shmget,
    (syscall_t)tux_shmat, // SYS_shmat,
    (syscall_t)tux_shmctl, // SYS_shmctl,
    tux_file_delegate, // SYS_dup,
    tux_dup2_delegate, // SYS_dup2,
    tux_no_impl, // SYS_pause,
    (syscall_t)tux_nanosleep,
    tux_no_impl, // SYS_getitimer,
    (syscall_t)tux_alarm, // SYS_alarm,
    tux_no_impl, // SYS_setitimer,
    (syscall_t)tux_getpid, // SYS_getpid,
    tux_file_delegate, // SYS_sendfile,
    tux_open_delegate, // SYS_socket,
    tux_file_delegate, // SYS_connect,
    tux_file_delegate, // SYS_accept,
    tux_file_delegate, // SYS_sendto,
    tux_file_delegate, // SYS_recvfrom,
    tux_file_delegate, // SYS_sendmsg,
    tux_file_delegate, // SYS_recvmsg,
    tux_delegate, // SYS_shutdown,
    tux_file_delegate, // SYS_bind,
    tux_file_delegate, // SYS_listen,
    tux_file_delegate, // SYS_getsockname,
    tux_file_delegate, // SYS_getpeername,
    tux_delegate, // SYS_socketpair,
    tux_file_delegate, // SYS_setsockopt,
    tux_file_delegate, // SYS_getsockopt,
    (syscall_t)tux_clone,
    (syscall_t)tux_fork, // sys_fork
    (syscall_t)tux_fork, // sys_vfork
    (syscall_t)tux_exec, // sys_execve
    (syscall_t)tux_exit, // SYS_exit,
    (syscall_t)tux_waithook, // sys_wait4
    (syscall_t)tux_pidhook, // SYS_kill,
    tux_delegate, // SYS_uname,
    (syscall_t)tux_semget, // SYS_semget,
    (syscall_t)tux_semop, // SYS_semop,
    (syscall_t)tux_semctl, // SYS_semctl,
    (syscall_t)tux_shmdt, // SYS_shmdt,
    tux_no_impl, // SYS_msgget,
    tux_no_impl, // SYS_msgsnd,
    tux_no_impl, // SYS_msgrcv,
    tux_no_impl, // SYS_msgctl,
    tux_file_delegate, // SYS_fcntl,
    tux_delegate, // SYS_flock,
    tux_file_delegate, // SYS_fsync,
    tux_delegate, // SYS_fdatasync,
    tux_delegate, // SYS_truncate,
    tux_file_delegate, // SYS_ftruncate,
    tux_delegate, // SYS_getdents,
    tux_delegate, // SYS_getcwd,
    tux_delegate, // SYS_chdir,
    tux_delegate, // SYS_fchdir,
    tux_delegate, // SYS_rename,
    tux_delegate, // SYS_mkdir,
    tux_delegate, // SYS_rmdir,
    tux_delegate, // SYS_creat,
    tux_delegate, // SYS_link, Only peusdo pilesystem are supported, not useful, disabled for now
    tux_delegate, // SYS_unlink,
    tux_delegate, // SYS_symlink,
    tux_delegate, // SYS_readlink,
    tux_delegate, // SYS_chmod,
    tux_delegate, // SYS_fchmod,
    tux_delegate, // SYS_chown,
    tux_delegate, // SYS_fchown,
    tux_delegate, // SYS_lchown,
    tux_delegate, // SYS_umask,
    (syscall_t)tux_gettimeofday, // SYS_gettimeofday,
    (syscall_t)tux_getrlimit,
    tux_delegate, // SYS_getrusage,
    tux_delegate, // SYS_sysinfo // XXX: should fill in the memory slots using nuttx's parameter
    (syscall_t)tux_success_stub, // SYS_times,
    tux_no_impl, // SYS_ptrace,
    tux_delegate, // SYS_getuid,
    tux_no_impl, // SYS_syslog,
    tux_delegate, // SYS_getgid,
    tux_delegate, // SYS_setuid,
    tux_delegate, // SYS_setgid,
    tux_delegate, // SYS_geteuid,
    tux_delegate, // SYS_getegid,
    tux_delegate, // SYS_setpgid,
    (syscall_t)tux_getppid, // SYS_getppid,
    tux_delegate, // SYS_getpgrp,
    tux_delegate, // SYS_setsid,
    tux_delegate, // SYS_setreuid,
    tux_delegate, // SYS_setregid,
    tux_delegate, // SYS_getgroups,
    tux_delegate, // SYS_setgroups,
    tux_delegate, // SYS_setresuid,
    tux_delegate, // SYS_getresuid,
    tux_delegate, // SYS_setresgid,
    tux_delegate, // SYS_getresgid,
    tux_delegate, // SYS_getpgid,
    tux_delegate, // SYS_setfsuid,
    tux_delegate, // SYS_setfsgid,
    tux_delegate, // SYS_getsid,
    tux_delegate, // SYS_capget,
    tux_delegate, // SYS_capset,
    tux_local, // SYS_sigpending,
    tux_local, // SYS_sigtimedwait,
    tux_no_impl, // SYS_rt_sigqueueinfo,
    tux_local, // SYS_sigsuspend,
    (syscall_t)tux_sigaltstack, // SYS_sigaltstack,
    tux_no_impl, // SYS_utime,
    tux_delegate, // SYS_mknod,
    tux_no_impl, // SYS_uselib,
    tux_no_impl, // SYS_personality,
    tux_no_impl, // SYS_ustat,
    tux_delegate, // SYS_statfs,
    tux_file_delegate, // SYS_fstatfs,
    tux_delegate, // SYS_sysfs,
    tux_no_impl, // SYS_getpriority,
    tux_no_impl, // SYS_setpriority,
    (syscall_t)tux_pidhook, // SYS_sched_setparam,
    (syscall_t)tux_pidhook, // SYS_sched_getparam,
    (syscall_t)tux_success_stub, // SYS_sched_setscheduler,
    tux_local, // SYS_sched_getscheduler,
    (syscall_t)tux_sched_get_priority_max, // SYS_sched_get_priority_max,
    (syscall_t)tux_sched_get_priority_min, // SYS_sched_get_priority_min,
    (syscall_t)tux_pidhook, // SYS_sched_rr_get_interval,
    (syscall_t)tux_success_stub, // SYS_mlock,
    (syscall_t)tux_success_stub, // SYS_munlock,
    (syscall_t)tux_success_stub, // SYS_mlockall,
    (syscall_t)tux_success_stub, // SYS_munlockall,
    tux_no_impl, // SYS_vhangup,
    tux_no_impl, // SYS_modify_ldt,
    tux_no_impl, // SYS_pivot_root,
    tux_no_impl, // SYS__sysctl,
    (syscall_t)tux_success_stub, // SYS_prctl,
    (syscall_t)tux_arch_prctl,
    tux_no_impl, // SYS_adjtimex,
    (syscall_t)tux_success_stub, // SYS_setrlimit,
    tux_no_impl, // SYS_chroot,
    tux_no_impl, // SYS_sync,
    tux_no_impl, // SYS_acct,
    tux_no_impl, // SYS_settimeofday,
    tux_file_delegate, // SYS_mount,
    tux_file_delegate, // SYS_umount2,
    tux_no_impl, // SYS_swapon,
    tux_no_impl, // SYS_swapoff,
    tux_no_impl, // SYS_reboot,
    tux_no_impl, // SYS_sethostname,
    tux_no_impl, // SYS_setdomainname,
    tux_no_impl, // SYS_iopl,
    tux_no_impl, // SYS_ioperm,
    tux_no_impl, // SYS_create_module,
    tux_no_impl, // SYS_init_module,
    tux_no_impl, // SYS_delete_module,
    tux_no_impl, // SYS_get_kernel_syms,
    tux_no_impl, // SYS_query_module,
    tux_no_impl, // SYS_quotactl,
    tux_no_impl, // SYS_nfsservctl,
    tux_no_impl, // SYS_getpmsg,
    tux_no_impl, // SYS_putpmsg,
    tux_no_impl, // SYS_afs_syscall,
    tux_no_impl, // SYS_tuxcall,
    tux_no_impl, // SYS_security,
    (syscall_t)tux_gettid, // SYS_gettid
    tux_delegate, // SYS_readahead,
    tux_delegate, // SYS_setxattr,
    tux_delegate, // SYS_lsetxattr,
    tux_delegate, // SYS_fsetxattr,
    tux_delegate, // SYS_getxattr,
    tux_delegate, // SYS_lgetxattr,
    tux_delegate, // SYS_fgetxattr,
    tux_delegate, // SYS_listxattr,
    tux_delegate, // SYS_llistxattr,
    tux_delegate, // SYS_flistxattr,
    tux_delegate, // SYS_removexattr,
    tux_delegate, // SYS_lremovexattr,
    tux_delegate, // SYS_fremovexattr,
    tux_no_impl, // SYS_tkill,
    tux_delegate,//tux_no_impl, // SYS_time,
    (syscall_t)tux_futex,
    (syscall_t)tux_success_stub, // SYS_sched_setaffinity, // Only if we expend to SMP
    (syscall_t)tux_sched_getaffinity, // SYS_sched_getaffinity,
    tux_no_impl, // SYS_set_thread_area,
    tux_no_impl, // SYS_io_setup,
    tux_no_impl, // SYS_io_destroy,
    tux_no_impl, // SYS_io_getevents,
    tux_no_impl, // SYS_io_submit,
    tux_no_impl, // SYS_io_cancel,
    tux_no_impl, // SYS_get_thread_area,
    tux_no_impl, // SYS_lookup_dcookie,
    tux_delegate, // SYS_epoll_create,
    tux_delegate, // SYS_epoll_ctl_old,
    tux_delegate, // SYS_epoll_wait_old,
    tux_no_impl, // SYS_remap_file_pages,
    tux_delegate, // SYS_getdents64,
    (syscall_t)tux_set_tid_address,
    tux_no_impl, // SYS_restart_syscall,
    (syscall_t)tux_semtimedop, // SYS_semtimedop,
    (syscall_t)tux_success_stub, // SYS_fadvise64,
    tux_local, // SYS_timer_create,
    tux_local, // SYS_timer_settime,
    tux_local, // SYS_timer_gettime,
    tux_local, // SYS_timer_getoverrun,
    tux_local, // SYS_timer_delete,
    tux_local, // SYS_clock_settime,
    tux_local, // SYS_clock_gettime,
    tux_local, // SYS_clock_getres,
    tux_local, // SYS_clock_nanosleep,
    (syscall_t)tux_exit, //sys_exit_group
    tux_delegate, // SYS_epoll_wait,
    tux_delegate, // SYS_epoll_ctl,
    tux_no_impl, // SYS_tgkill,
    tux_no_impl, // SYS_utimes,
    tux_no_impl, // SYS_vserver,
    tux_no_impl, // SYS_mbind,
    tux_no_impl, // SYS_set_mempolicy,
    tux_no_impl, // SYS_get_mempolicy,
    tux_local, // SYS_mq_open,
    tux_local, // SYS_mq_unlink,
    tux_local, // SYS_mq_timedsend,
    tux_local, // SYS_mq_timedreceive,
    tux_local, // SYS_mq_notify,
    tux_no_impl, // SYS_mq_getsetattr, // Maybe we should glue one out?
    tux_no_impl, // SYS_kexec_load,
    tux_no_impl, // SYS_waitid,
    tux_no_impl, // SYS_add_key,
    tux_no_impl, // SYS_request_key,
    tux_no_impl, // SYS_keyctl,
    tux_no_impl, // SYS_ioprio_set,
    tux_no_impl, // SYS_ioprio_get,
    tux_delegate, // SYS_inotify_init,
    tux_delegate, // SYS_inotify_add_watch,
    tux_delegate, // SYS_inotify_rm_watch,
    tux_no_impl, // SYS_migrate_pages,
    tux_delegate, // SYS_openat,
    tux_delegate, // SYS_mkdirat,
    tux_delegate, // SYS_mknodat,
    tux_delegate, // SYS_fchownat,
    tux_delegate, // SYS_futimesat,
    tux_delegate, // SYS_newfstatat,
    tux_delegate, // SYS_unlinkat,
    tux_delegate, // SYS_renameat,
    tux_delegate, // SYS_linkat,
    tux_delegate, // SYS_symlinkat,
    tux_delegate, // SYS_readlinkat,
    tux_delegate, // SYS_fchmodat,
    tux_delegate, // SYS_faccessat,
    tux_delegate, // SYS_pselect6,
    tux_delegate, // SYS_ppoll,
    tux_no_impl, // SYS_unshare,
    (syscall_t)tux_success_stub, // SYS_set_robust_list,
    tux_no_impl, // SYS_get_robust_list,
    tux_no_impl, // SYS_splice,
    tux_no_impl, // SYS_tee,
    tux_no_impl, // SYS_sync_file_range,
    tux_no_impl, // SYS_vmsplice,
    tux_no_impl, // SYS_move_pages,
    tux_no_impl, // SYS_utimensat,
    tux_delegate, // SYS_epoll_pwait,
    tux_no_impl, // SYS_signalfd,
    tux_no_impl, // SYS_timerfd_create,
    tux_delegate, // SYS_eventfd,
    tux_delegate, // SYS_fallocate,
    tux_no_impl, // SYS_timerfd_settime,
    tux_no_impl, // SYS_timerfd_gettime,
    tux_delegate, // SYS_accept4,
    tux_no_impl, // SYS_signalfd4,
    tux_delegate, // SYS_eventfd2,
    tux_delegate, // SYS_epoll_create1,
    tux_delegate, // SYS_dup3,
    (syscall_t)tux_pipe, // SYS_pipe2,
    tux_delegate, // SYS_inotify_init1,
    tux_delegate, // SYS_preadv,
    tux_delegate, // SYS_pwritev,
    tux_no_impl, // SYS_rt_tgsigqueueinfo,
    tux_no_impl, // SYS_perf_event_open,
    tux_delegate, // SYS_recvmmsg,
    tux_delegate, // SYS_fanotify_init,
    tux_delegate, // SYS_fanotify_mark,
    tux_no_impl, // SYS_prlimit64,
    tux_delegate, // SYS_name_to_handle_at,
    tux_delegate, // SYS_open_by_handle_at,
    tux_no_impl, // SYS_clock_adjtime,
    tux_delegate, // SYS_syncfs,
    tux_delegate, // SYS_sendmmsg,
    tux_no_impl, // SYS_setns,
    (syscall_t)tux_getcpu, // SYS_getcpu,
    tux_no_impl, // SYS_process_vm_readv,
    tux_no_impl, // SYS_process_vm_writev,
    tux_no_impl, // SYS_kcmp,
    tux_no_impl, // SYS_finit_module,
    tux_no_impl, // SYS_sched_setattr,
    tux_no_impl, // SYS_sched_getattr,
    tux_no_impl, // SYS_renameat2,
    tux_no_impl, // SYS_seccomp,
    tux_delegate,   // SYS_getrandom,
    tux_no_impl, // SYS_memfd_create,
    tux_no_impl, // SYS_kexec_file_load,
    tux_no_impl, // SYS_bpf,
    tux_no_impl, // SYS_execveat,
    tux_no_impl, // SYS_userfaultfd,
    tux_no_impl, // SYS_membarrier,
    (syscall_t)tux_success_stub, // SYS_mlock2,
    tux_no_impl, // SYS_copy_file_range,
    tux_delegate, // SYS_preadv2,
    tux_delegate, // SYS_pwritev2,
    tux_delegate, // SYS_pkey_mprotect,
    tux_delegate, // SYS_pkey_alloc,
    tux_delegate, // SYS_pkey_free,
};

uint64_t linux_syscall_number_table[500] = {
    SYS_read,
    SYS_write,
    SYS_open,
    SYS_close,
    SYS_stat,
    SYS_fstat,
    -1, // SYS_lstat
    SYS_poll,
    SYS_lseek,
    -1,
    -1,
    -1,
    -1,
    SYS_sigaction,
    SYS_sigprocmask,
    -1, // SYS_sigreturn,
    SYS_ioctl,
    -1, // SYS_pread,
    -1, // SYS_pwrite,
    -1, // sys_readv
    -1, // sys_writev
    -1, // sys_access
    SYS_pipe2,
    SYS_select,
    SYS_sched_yield,
    -1, // sys_mremap
    -1, // sys_msync
    -1, // sys_mincore
    -1, // sys_madvise
    -1, // SYS_shmget,
    -1, // SYS_shmat,
    -1, // SYS_shmctl,
    SYS_dup,
    SYS_dup2,
    -1, // SYS_pause,
    -1,
    -1, // SYS_getitimer,
    -1, // SYS_alarm,
    -1, // SYS_setitimer,
    SYS_getpid,
    -1,
    SYS_socket,
    SYS_connect,
    SYS_accept,
    -1, // SYS_sendto,
    -1, // SYS_recvfrom,
    -1, // SYS_sendmsg,
    -1, // SYS_recvmsg,
    -1, // SYS_shutdown,
    SYS_bind,
    SYS_listen,
    SYS_getsockname,
    SYS_getpeername,
    -1,
    SYS_setsockopt,
    SYS_getsockopt,
    -1,
    -1, // sys_fork
    -1, // sys_vfrok
    -1, // sys_execve
    SYS_exit,
    SYS_waitpid, // sys_wait4
    SYS_kill,
    SYS_uname,
    -1, // SYS_semget,
    -1, // SYS_semop,
    -1, // SYS_semctl,
    -1, // SYS_shmdt,
    -1, // SYS_msgget,
    -1, // SYS_msgsnd,
    -1, // SYS_msgrcv,
    -1, // SYS_msgctl,
    SYS_fcntl,
    -1,
    SYS_fsync,
    -1, // SYS_fdatasync,
    -1, // SYS_truncate,
    SYS_ftruncate,
    -1, // SYS_getdents,
    -1, // SYS_getcwd,
    -1, // SYS_chdir,
    -1, // SYS_fchdir,
    -1, // SYS_rename,
    -1, // SYS_mkdir,
    -1, // SYS_rmdir,
    -1, // SYS_creat,
    -1, // SYS_link, Only peusdo pilesystem are supported, not useful, disabled for now
    -1, // SYS_unlink,
    -1, // SYS_symlink,
    -1, // SYS_readlink,
    -1, // SYS_chmod,
    -1, // SYS_fchmod,
    -1, // SYS_chown,
    -1, // SYS_fchown,
    -1, // SYS_lchown,
    -1, // SYS_umask,
    -1, // SYS_gettimeofday,
    -1,
    -1, // SYS_getrusage,
    -1, // SYS_sysinfo,
    -1, // SYS_times,
    -1, // SYS_ptrace,
    -1, // SYS_getuid,
    -1, // SYS_syslog,
    -1, // SYS_getgid,
    -1, // SYS_setuid,
    -1, // SYS_setgid,
    -1, // SYS_geteuid,
    -1, // SYS_getegid,
    -1, // SYS_setpgid,
    SYS_getpid, // SYS_getppid,
    -1, // SYS_getpgrp,
    -1, // SYS_setsid,
    -1, // SYS_setreuid,
    -1, // SYS_setregid,
    -1, // SYS_getgroups,
    -1, // SYS_setgroups,
    -1, // SYS_setresuid,
    -1, // SYS_getresuid,
    -1, // SYS_setresgid,
    -1, // SYS_getresgid,
    -1, // SYS_getpgid,
    -1, // SYS_setfsuid,
    -1, // SYS_setfsgid,
    -1, // SYS_getsid,
    -1, // SYS_capget,
    -1, // SYS_capset,
    SYS_sigpending,
    SYS_sigtimedwait,
    -1, // SYS_rt_sigqueueinfo,
    SYS_sigsuspend,
    -1, // SYS_sigaltstack,
    -1, // SYS_utime,
    -1, // SYS_mknod,
    -1, // SYS_uselib,
    -1, // SYS_personality,
    -1, // SYS_ustat,
    SYS_statfs,
    SYS_fstatfs,
    -1, // SYS_sysfs,
    -1, // SYS_getpriority,
    -1, // SYS_setpriority,
    SYS_sched_setparam,
    SYS_sched_getparam,
    SYS_sched_setscheduler,
    SYS_sched_getscheduler,
    -1, // SYS_sched_get_priority_max,
    -1, // SYS_sched_get_priority_min,
    SYS_sched_rr_get_interval,
    -1, // SYS_mlock,
    -1, // SYS_munlock,
    -1, // SYS_mlockall,
    -1, // SYS_munlockall,
    -1, // SYS_vhangup,
    -1, // SYS_modify_ldt,
    -1, // SYS_pivot_root,
    -1, // SYS__sysctl,
    -1, // SYS_prctl,
    -1,
    -1, // SYS_adjtimex,
    -1, // SYS_setrlimit,
    -1, // SYS_chroot,
    -1, // SYS_sync,
    -1, // SYS_acct,
    -1, // SYS_settimeofday,
    SYS_mount,
    SYS_umount2,
    -1, // SYS_swapon,
    -1, // SYS_swapoff,
    -1, // SYS_reboot,
    -1, // SYS_sethostname,
    -1, // SYS_setdomainname,
    -1, // SYS_iopl,
    -1, // SYS_ioperm,
    -1, // SYS_create_module,
    -1, // SYS_init_module,
    -1, // SYS_delete_module,
    -1, // SYS_get_kernel_syms,
    -1, // SYS_query_module,
    -1, // SYS_quotactl,
    -1, // SYS_nfsservctl,
    -1, // SYS_getpmsg,
    -1, // SYS_putpmsg,
    -1, // SYS_afs_syscall,
    -1, // SYS_tuxcall,
    -1, // SYS_security,
    SYS_getpid, // Fake get tid to get pid, not a different in our world heh?
    -1, // SYS_readahead,
    -1, // SYS_setxattr,
    -1, // SYS_lsetxattr,
    -1, // SYS_fsetxattr,
    -1, // SYS_getxattr,
    -1, // SYS_lgetxattr,
    -1, // SYS_fgetxattr,
    -1, // SYS_listxattr,
    -1, // SYS_llistxattr,
    -1, // SYS_flistxattr,
    -1, // SYS_removexattr,
    -1, // SYS_lremovexattr,
    -1, // SYS_fremovexattr,
    -1, // SYS_tkill,
    -1, // SYS_time,
    -1,
    -1, // SYS_sched_setaffinity, // Only if we expend to SMP
    -1, // SYS_sched_getaffinity,
    -1, // SYS_set_thread_area,
    -1, // SYS_io_setup,
    -1, // SYS_io_destroy,
    -1, // SYS_io_getevents,
    -1, // SYS_io_submit,
    -1, // SYS_io_cancel,
    -1, // SYS_get_thread_area,
    -1, // SYS_lookup_dcookie,
    -1, // SYS_epoll_create,
    -1, // SYS_epoll_ctl_old,
    -1, // SYS_epoll_wait_old,
    -1, // SYS_remap_file_pages,
    -1, // SYS_getdents64,
    -1,
    -1, // SYS_restart_syscall,
    -1, // SYS_semtimedop,
    -1, // SYS_fadvise64,
    SYS_timer_create,
    SYS_timer_settime,
    SYS_timer_gettime,
    SYS_timer_getoverrun,
    SYS_timer_delete,
    SYS_clock_settime,
    SYS_clock_gettime,
    SYS_clock_getres,
    SYS_clock_nanosleep,
    SYS_exit,
    -1, // SYS_epoll_wait,
    -1, // SYS_epoll_ctl,
    -1, // SYS_tgkill,
    -1, // SYS_utimes,
    -1, // SYS_vserver,
    -1, // SYS_mbind,
    -1, // SYS_set_mempolicy,
    -1, // SYS_get_mempolicy,
    SYS_mq_open,
    SYS_mq_unlink,
    SYS_mq_timedsend,
    SYS_mq_timedreceive,
    SYS_mq_notify,
    -1, // SYS_mq_getsetattr, // Maybe we should glue one out?
    -1, // SYS_kexec_load,
    -1, // SYS_waitid
    -1, // SYS_add_key,
    -1, // SYS_request_key,
    -1, // SYS_keyctl,
    -1, // SYS_ioprio_set,
    -1, // SYS_ioprio_get,
    -1, // SYS_inotify_init,
    -1, // SYS_inotify_add_watch,
    -1, // SYS_inotify_rm_watch,
    -1, // SYS_migrate_pages,
    -1, // SYS_openat,
    -1, // SYS_mkdirat,
    -1, // SYS_mknodat,
    -1, // SYS_fchownat,
    -1, // SYS_futimesat,
    -1, // SYS_newfstatat,
    -1, // SYS_unlinkat,
    -1, // SYS_renameat,
    -1, // SYS_linkat,
    -1, // SYS_symlinkat,
    -1, // SYS_readlinkat,
    -1, // SYS_fchmodat,
    -1, // SYS_faccessat,
    -1, // SYS_pselect6,
    -1, // SYS_ppoll,
    -1, // SYS_unshare,
    -1, // SYS_set_robust_list,
    -1, // SYS_get_robust_list,
    -1, // SYS_splice,
    -1, // SYS_tee,
    -1, // SYS_sync_file_range,
    -1, // SYS_vmsplice,
    -1, // SYS_move_pages,
    -1, // SYS_utimensat,
    -1, // SYS_epoll_pwait,
    -1, // SYS_signalfd,
    -1, // SYS_timerfd_create,
    -1, // SYS_eventfd,
    -1, // SYS_fallocate,
    -1, // SYS_timerfd_settime,
    -1, // SYS_timerfd_gettime,
    -1, // SYS_accept4,
    -1, // SYS_signalfd4,
    -1, // SYS_eventfd2,
    -1, // SYS_epoll_create1,
    -1, // SYS_dup3,
    SYS_pipe2,
    -1, // SYS_inotify_init1,
    -1, // SYS_preadv,
    -1, // SYS_pwritev,
    -1, // SYS_rt_tgsigqueueinfo,
    -1, // SYS_perf_event_open,
    -1, // SYS_recvmmsg,
    -1, // SYS_fanotify_init,
    -1, // SYS_fanotify_mark,
    -1, // SYS_prlimit64,
    -1, // SYS_name_to_handle_at,
    -1, // SYS_open_by_handle_at,
    -1, // SYS_clock_adjtime,
    -1, // SYS_syncfs,
    -1, // SYS_sendmmsg,
    -1, // SYS_setns,
    -1, // SYS_getcpu,
    -1, // SYS_process_vm_readv,
    -1, // SYS_process_vm_writev,
    -1, // SYS_kcmp,
    -1, // SYS_finit_module,
    -1, // SYS_sched_setattr,
    -1, // SYS_sched_getattr,
    -1, // SYS_renameat2,
    -1, // SYS_seccomp,
    SYS_getrandom,
    -1, // SYS_memfd_create,
    -1, // SYS_kexec_file_load,
    -1, // SYS_bpf,
    -1, // SYS_execveat,
    -1, // SYS_userfaultfd,
    -1, // SYS_membarrier,
    -1, // SYS_mlock2,
    -1, // SYS_copy_file_range,
    -1, // SYS_preadv2,
    -1, // SYS_pwritev2,
    -1, // SYS_pkey_mprotect,
    -1, // SYS_pkey_alloc,
    -1, // SYS_pkey_free,
    -1, // SYS_statx,
    -1, // SYS_io_pgetevents,
    -1 // SYS_rseq,
};


#ifdef CONFIG_LIB_SYSCALL

/****************************************************************************
 * Private Functions
 ****************************************************************************/

uint64_t __attribute__ ((noinline))
linux_interface(unsigned long nbr, uintptr_t parm1, uintptr_t parm2,
                          uintptr_t parm3, uintptr_t parm4, uintptr_t parm5,
                          uintptr_t parm6)
{
  uint64_t ret;

  svcinfo("Linux Subsystem call: %d\n", nbr);

  /* Call syscall from table. */
  ret = linux_syscall_action_table[nbr](nbr, parm1, parm2, parm3, parm4, parm5, parm6);

  svcinfo("ret = %llx\n", ret);

  return ret;
}


#endif
