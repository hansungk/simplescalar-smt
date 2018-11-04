/* syscall.c - proxy system call handler routines */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 2000-2001 by The Regents of The University of Michigan.
 * Copyright (C) 1994-2001 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/uio.h>
#
/* only enable a minimal set of systen call proxies if on limited
   hosts or if in cross endian live execution mode */
#ifndef MIN_SYSCALL_MODE
#if defined(_MSC_VER) || defined(__CYGWIN32__) || defined(MD_CROSS_ENDIAN)
#define MIN_SYSCALL_MODE
#endif
#endif /* !MIN_SYSCALL_MODE */

/* live execution only support on same-endian hosts... */
#ifdef _MSC_VER
#include <io.h>
#else /* !_MSC_VER */
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <sys/param.h>
#endif
#include <errno.h>
#include <time.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#ifndef _MSC_VER
#include <sys/resource.h>
#endif
#include <signal.h>
#ifndef _MSC_VER
#include <sys/file.h>
#endif
#include <sys/stat.h>
#ifndef _MSC_VER
#include <sys/uio.h>
#endif
#include <setjmp.h>
#ifndef _MSC_VER
#include <sys/times.h>
#endif
#include <limits.h>
#ifndef _MSC_VER
#include <sys/ioctl.h>
#endif
#if defined(linux)
#include <utime.h>
#include <dirent.h>
#include <sys/vfs.h>
#endif
#if defined(_AIX)
#include <sys/statfs.h>
#else /* !_AIX */
#ifndef _MSC_VER
#include <sys/mount.h>
#endif
#endif /* !_AIX */
#if !defined(linux) && !defined(sparc) && !defined(hpux) && !defined(__hpux) && !defined(__CYGWIN32__) && !defined(ultrix)
#ifndef _MSC_VER
#include <sys/select.h>
#endif
#endif
#ifdef linux
#include <sgtty.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#endif /* linux */

#if defined(__svr4__)
#include <sys/dirent.h>
#include <sys/filio.h>
#elif defined(__osf__)
#include <dirent.h>
/* -- For some weird reason, getdirentries() is not declared in any
 * -- header file under /usr/include on the Alpha boxen that I tried
 * -- SS-Alpha on. But the function exists in the libraries.
 */
int getdirentries(int fd, char *buf, int nbytes, long *basep);
#endif

#if defined(__svr4__) || defined(__osf__)
#include <sys/statvfs.h>
#define statfs statvfs
#include <sys/time.h>
#include <utime.h>
#include <sgtty.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#if defined(sparc) && defined(__unix__)
#if defined(__svr4__) || defined(__USLC__)
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

/* dorks */
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef TAB2
#undef XTABS
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#undef FLUSHO
#undef PENDIN
#endif

#if defined(hpux) || defined(__hpux)
#undef CR0
#endif

#ifdef __FreeBSD__
#include <sys/ioctl_compat.h>
#else
#ifndef _MSC_VER
#include <termio.h>
#endif
#endif

#if defined(hpux) || defined(__hpux)
/* et tu, dorks! */
#undef HUPCL
#undef ECHO
#undef B50
#undef B75
#undef B110
#undef B134
#undef B150
#undef B200
#undef B300
#undef B600
#undef B1200
#undef B1800
#undef B2400
#undef B4800
#undef B9600
#undef B19200
#undef B38400
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef EXTA
#undef EXTB
#undef B900
#undef B3600
#undef B7200
#undef XTABS
#include <sgtty.h>
#include <utime.h>
#endif

#ifdef __CYGWIN32__
#include <sys/unistd.h>
#include <sys/vfs.h>
#endif

#include <sys/socket.h>
#include <sys/poll.h>

#ifdef _MSC_VER
#define access		_access
#define chmod		_chmod
#define chdir		_chdir
#define unlink		_unlink
#define open		_open
#define creat		_creat
#define pipe		_pipe
#define dup		_dup
#define dup2		_dup2
#define stat		_stat
#define fstat		_fstat
#define lseek		_lseek
#define read		_read
#define write		_write
#define close		_close
#define getpid		_getpid
#define utime		_utime
#include <sys/utime.h>
#endif /* _MSC_VER */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "sim.h"
#include "endian.h"
#include "eio.h"
#include "syscall.h"

#define OSF_SYS_syscall     0
/* OSF_SYS_exit moved to alpha.h */
#define OSF_SYS_fork        2
#define OSF_SYS_read        3
/* OSF_SYS_write moved to alpha.h */
#define OSF_SYS_old_open    5       /* 5 is old open */
#define OSF_SYS_close       6
#define OSF_SYS_wait4       7
#define OSF_SYS_old_creat   8       /* 8 is old creat */
#define OSF_SYS_link        9
#define OSF_SYS_unlink      10
#define OSF_SYS_execv       11
#define OSF_SYS_chdir       12
#define OSF_SYS_fchdir      13
#define OSF_SYS_mknod       14
#define OSF_SYS_chmod       15
#define OSF_SYS_chown       16
#define OSF_SYS_obreak      17
#define OSF_SYS_getfsstat   18
#define OSF_SYS_lseek       19
#define OSF_SYS_getpid      20
#define OSF_SYS_mount       21
#define OSF_SYS_unmount     22
#define OSF_SYS_setuid      23
#define OSF_SYS_getuid      24
#define OSF_SYS_exec_with_loader    25
#define OSF_SYS_ptrace      26
#ifdef  COMPAT_43
#define OSF_SYS_nrecvmsg    27
#define OSF_SYS_nsendmsg    28
#define OSF_SYS_nrecvfrom   29
#define OSF_SYS_naccept     30
#define OSF_SYS_ngetpeername        31
#define OSF_SYS_ngetsockname        32
#else
#define OSF_SYS_recvmsg     27
#define OSF_SYS_sendmsg     28
#define OSF_SYS_recvfrom    29
#define OSF_SYS_accept      30
#define OSF_SYS_getpeername 31
#define OSF_SYS_getsockname 32
#endif
#define OSF_SYS_access      33
#define OSF_SYS_chflags     34
#define OSF_SYS_fchflags    35
#define OSF_SYS_sync        36
#define OSF_SYS_kill        37
#define OSF_SYS_old_stat    38      /* 38 is old stat */
#define OSF_SYS_setpgid     39
#define OSF_SYS_old_lstat   40      /* 40 is old lstat */
#define OSF_SYS_dup 41
#define OSF_SYS_pipe        42
#define OSF_SYS_set_program_attributes      43
#define OSF_SYS_profil      44
#define OSF_SYS_open        45
                                /* 46 is obsolete osigaction */
#define OSF_SYS_getgid      47
#define OSF_SYS_sigprocmask 48
#define OSF_SYS_getlogin    49
#define OSF_SYS_setlogin    50
#define OSF_SYS_acct        51
#define OSF_SYS_sigpending  52
#define OSF_SYS_ioctl       54
#define OSF_SYS_reboot      55
#define OSF_SYS_revoke      56
#define OSF_SYS_symlink     57
#define OSF_SYS_readlink    58
#define OSF_SYS_execve      59
#define OSF_SYS_umask       60
#define OSF_SYS_chroot      61
#define OSF_SYS_old_fstat   62      /* 62 is old fstat */
#define OSF_SYS_getpgrp     63
#define OSF_SYS_getpagesize 64
#define OSF_SYS_mremap      65
#define OSF_SYS_vfork       66
#define OSF_SYS_stat        67
#define OSF_SYS_lstat       68
#define OSF_SYS_sbrk        69
#define OSF_SYS_sstk        70
#define OSF_SYS_mmap        71
#define OSF_SYS_ovadvise    72
#define OSF_SYS_munmap      73
#define OSF_SYS_mprotect    74
#define OSF_SYS_madvise     75
#define OSF_SYS_old_vhangup 76      /* 76 is old vhangup */
#define OSF_SYS_kmodcall    77
#define OSF_SYS_mincore     78
#define OSF_SYS_getgroups   79
#define OSF_SYS_setgroups   80
#define OSF_SYS_old_getpgrp 81      /* 81 is old getpgrp */
#define OSF_SYS_setpgrp     82
#define OSF_SYS_setitimer   83
#define OSF_SYS_old_wait    84      /* 84 is old wait */
#define OSF_SYS_table       85
#define OSF_SYS_getitimer   86
#define OSF_SYS_gethostname 87
#define OSF_SYS_sethostname 88
#define OSF_SYS_getdtablesize       89
#define OSF_SYS_dup2        90
#define OSF_SYS_fstat       91
#define OSF_SYS_fcntl       92
#define OSF_SYS_select      93
#define OSF_SYS_poll        94
#define OSF_SYS_fsync       95
#define OSF_SYS_setpriority 96
#define OSF_SYS_socket      97
#define OSF_SYS_connect     98
#ifdef  COMPAT_43
#define OSF_SYS_accept      99
#else
#define OSF_SYS_old_accept  99      /* 99 is old accept */
#endif
#define OSF_SYS_getpriority 100
#ifdef  COMPAT_43
#define OSF_SYS_send        101
#define OSF_SYS_recv        102
#else
#define OSF_SYS_old_send    101     /* 101 is old send */
#define OSF_SYS_old_recv    102     /* 102 is old recv */
#endif
#define OSF_SYS_sigreturn   103
#define OSF_SYS_bind        104
#define OSF_SYS_setsockopt  105
#define OSF_SYS_listen      106
#define OSF_SYS_plock       107
#define OSF_SYS_old_sigvec  108     /* 108 is old sigvec */
#define OSF_SYS_old_sigblock        109     /* 109 is old sigblock */
#define OSF_SYS_old_sigsetmask      110     /* 110 is old sigsetmask */
#define OSF_SYS_sigsuspend  111
#define OSF_SYS_sigstack    112
#ifdef  COMPAT_43
#define OSF_SYS_recvmsg     113
#define OSF_SYS_sendmsg     114
#else
#define OSF_SYS_old_recvmsg 113     /* 113 is old recvmsg */
#define OSF_SYS_old_sendmsg 114     /* 114 is old sendmsg */
#endif
                                /* 115 is obsolete vtrace */
#define OSF_SYS_gettimeofday        116
#define OSF_SYS_getrusage   117
#define OSF_SYS_getsockopt  118
#define OSF_SYS_readv       120
#define OSF_SYS_writev      121
#define OSF_SYS_settimeofday        122
#define OSF_SYS_fchown      123
#define OSF_SYS_fchmod      124
#ifdef  COMPAT_43
#define OSF_SYS_recvfrom    125
#else
#define OSF_SYS_old_recvfrom        125     /* 125 is old recvfrom */
#endif
#define OSF_SYS_setreuid    126
#define OSF_SYS_setregid    127
#define OSF_SYS_rename      128
#define OSF_SYS_truncate    129
#define OSF_SYS_ftruncate   130
#define OSF_SYS_flock       131
#define OSF_SYS_setgid      132
#define OSF_SYS_sendto      133
#define OSF_SYS_shutdown    134
#define OSF_SYS_socketpair  135
#define OSF_SYS_mkdir       136
#define OSF_SYS_rmdir       137
#define OSF_SYS_utimes      138
                                /* 139 is obsolete 4.2 sigreturn */
#define OSF_SYS_adjtime     140
#ifdef  COMPAT_43
#define OSF_SYS_getpeername 141
#else
#define OSF_SYS_old_getpeername     141     /* 141 is old getpeername */
#endif
#define OSF_SYS_gethostid   142
#define OSF_SYS_sethostid   143
#define OSF_SYS_getrlimit   144
#define OSF_SYS_setrlimit   145
#define OSF_SYS_old_killpg  146     /* 146 is old killpg */
#define OSF_SYS_setsid      147
#define OSF_SYS_quotactl    148
#define OSF_SYS_oldquota    149
#ifdef  COMPAT_43
#define OSF_SYS_getsockname 150
#else
#define OSF_SYS_old_getsockname     150     /* 150 is old getsockname */
#endif
#define OSF_SYS_pid_block   153
#define OSF_SYS_pid_unblock 154
#define OSF_SYS_sigaction   156
#define OSF_SYS_sigwaitprim 157
#define OSF_SYS_nfssvc      158
#define OSF_SYS_getdirentries       159
#define OSF_SYS_statfs      160
#define OSF_SYS_fstatfs     161
#define OSF_SYS_async_daemon        163
#define OSF_SYS_getfh       164
#define OSF_SYS_getdomainname       165
#define OSF_SYS_setdomainname       166
#define OSF_SYS_exportfs    169
#define OSF_SYS_alt_plock   181     /* 181 is alternate plock */
#define OSF_SYS_getmnt      184
#define OSF_SYS_alt_sigpending      187     /* 187 is alternate sigpending */
#define OSF_SYS_alt_setsid  188     /* 188 is alternate setsid */
#define OSF_SYS_swapon      199
#define OSF_SYS_msgctl      200
#define OSF_SYS_msgget      201
#define OSF_SYS_msgrcv      202
#define OSF_SYS_msgsnd      203
#define OSF_SYS_semctl      204
#define OSF_SYS_semget      205
#define OSF_SYS_semop       206
#define OSF_SYS_uname       207
#define OSF_SYS_lchown      208
#define OSF_SYS_shmat       209
#define OSF_SYS_shmctl      210
#define OSF_SYS_shmdt       211
#define OSF_SYS_shmget      212
#define OSF_SYS_mvalid      213
#define OSF_SYS_getaddressconf      214
#define OSF_SYS_msleep      215
#define OSF_SYS_mwakeup     216
#define OSF_SYS_msync       217
#define OSF_SYS_signal      218
#define OSF_SYS_utc_gettime 219
#define OSF_SYS_utc_adjtime 220
#define OSF_SYS_security    222
#define OSF_SYS_kloadcall   223
#define OSF_SYS_getpgid     233
#define OSF_SYS_getsid      234
#define OSF_SYS_sigaltstack 235
#define OSF_SYS_waitid      236
#define OSF_SYS_priocntlset 237
#define OSF_SYS_sigsendset  238
#define OSF_SYS_set_speculative     239
#define OSF_SYS_msfs_syscall        240
#define OSF_SYS_sysinfo     241
#define OSF_SYS_uadmin      242
#define OSF_SYS_fuser       243
#define OSF_SYS_proplist_syscall    244
#define OSF_SYS_ntp_adjtime 245
#define OSF_SYS_ntp_gettime 246
#define OSF_SYS_pathconf    247
#define OSF_SYS_fpathconf   248
#define OSF_SYS_uswitch     250
#define OSF_SYS_usleep_thread       251
#define OSF_SYS_audcntl     252
#define OSF_SYS_audgen      253
#define OSF_SYS_sysfs       254
#define OSF_SYS_subOSF_SYS_info 255
#define OSF_SYS_getsysinfo  256
#define OSF_SYS_setsysinfo  257
#define OSF_SYS_afs_syscall 258
#define OSF_SYS_swapctl     259
#define OSF_SYS_memcntl     260
#define OSF_SYS_fdatasync   261


///////////////////////////////////////////////////////////////////////////
// alpha linux syscall specific definitions
///////////////////////////////////////////////////////////////////////////

#define PAGE_SIZE 8192 // 8192 bytes in a virtual memory page

// alpha open flags
static const int ALPHA_O_RDONLY       = 00000000;
static const int ALPHA_O_WRONLY       = 00000001;
static const int ALPHA_O_RDWR         = 00000002;
static const int ALPHA_O_NONBLOCK     = 00000004;
static const int ALPHA_O_APPEND       = 00000010;
static const int ALPHA_O_CREAT        = 00001000;
static const int ALPHA_O_TRUNC        = 00002000;
static const int ALPHA_O_EXCL         = 00004000;
static const int ALPHA_O_NOCTTY       = 00010000;
static const int ALPHA_O_SYNC         = 00040000;
static const int ALPHA_O_DRD          = 00100000;
static const int ALPHA_O_DIRECTIO     = 00200000;
static const int ALPHA_O_CACHE        = 00400000;
static const int ALPHA_O_DSYNC        = 02000000;
static const int ALPHA_O_RSYNC        = 04000000;

// alpha ioctl commands
static const unsigned ALPHA_IOCTL_TIOCGETP   = 0x40067408;
static const unsigned ALPHA_IOCTL_TIOCSETP   = 0x80067409;
static const unsigned ALPHA_IOCTL_TIOCSETN   = 0x8006740a;
static const unsigned ALPHA_IOCTL_TIOCSETC   = 0x80067411;
static const unsigned ALPHA_IOCTL_TIOCGETC   = 0x40067412;
static const unsigned ALPHA_IOCTL_FIONREAD   = 0x4004667f;
static const unsigned ALPHA_IOCTL_TIOCISATTY = 0x2000745e;
static const unsigned ALPHA_IOCTL_TIOCGETS   = 0x402c7413;
static const unsigned ALPHA_IOCTL_TIOCGETA   = 0x40127417;

// rlimit resource ids
static const unsigned ALPHA_RLIMIT_CPU     =  0; 
static const unsigned ALPHA_RLIMIT_FSIZE   =  1;
static const unsigned ALPHA_RLIMIT_DATA    =  2;
static const unsigned ALPHA_RLIMIT_STACK   =  3;
static const unsigned ALPHA_RLIMIT_CORE    =  4;
static const unsigned ALPHA_RLIMIT_RSS     =  5;
static const unsigned ALPHA_RLIMIT_NOFILE  =  6;
static const unsigned ALPHA_RLIMIT_AS      =  7;
static const unsigned ALPHA_RLIMIT_VMEM    =  7;
static const unsigned ALPHA_RLIMIT_NPROC   =  8;
static const unsigned ALPHA_RLIMIT_MEMLOCK =  9;
static const unsigned ALPHA_RLIMIT_LOCKS   = 10;

/////////////////////////////////////////////////////////////////////////////
// system call structures -- pulled from the cross-compiler include dir
/////////////////////////////////////////////////////////////////////////////

typedef struct
{
  // 65 is a hard define in the include tree
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
} alpha_utsname;

typedef struct
{
  uint64_t rlim_cur; // soft limit
  uint64_t rlim_max; // hard limit
} alpha_rlimit;

typedef struct
{
  uint32_t  st_dev;
  uint32_t  st_ino;
  uint32_t  st_mode;
  uint32_t  st_nlink;
  uint32_t  st_uid;
  uint32_t  st_gid;
  uint32_t  st_rdev;
  int32_t   __pad0;
  int64_t   st_size;
  uint64_t  st_atimeX; // compiler chokes on st_atime
  uint64_t  st_mtimeX; // compiler chokes on st_mtime
  uint64_t  st_ctimeX; // compiler chokes on st_ctime
  uint32_t  st_blksize;
  int32_t   st_blocks;
  uint32_t  st_flags;
  uint32_t  st_gen;
} alpha_stat;

typedef struct
{
  uint64_t st_dev;
  uint64_t st_ino;
  uint64_t st_rdev;
  uint64_t st_size;
  uint64_t st_blocks;

  uint32_t st_mode;
  uint32_t st_uid;
  uint32_t st_gid;
  uint32_t st_blksize;
  uint32_t st_nlink;
  uint32_t __pad0;

  uint64_t st_atimeX; // compiler chokes on st_atime
  uint64_t st_atime_nsec;
  uint64_t st_mtimeX; // compiler chokes on st_mtime
  uint64_t st_mtime_nsec;
  uint64_t st_ctimeX; // compiler chokes on st_ctime
  uint64_t st_ctime_nsec;
  int64_t  __unused[3];
} alpha_stat64;

typedef struct
{
  int64_t tv_sec;  // seconds
  int64_t tv_usec; // microseconds
} alpha_timeval;

typedef struct
{
  int64_t tz_minuteswest;
  int64_t tz_dsttime;
} alpha_timezone;

typedef struct
{
  uint64_t iov_base; // aka void *iov_base; // Starting address
  uint64_t iov_len;  // aka size_t iov_len; // Number of bytes to transfer
} alpha_iovec;




/* translate system call arguments */
struct xlate_table_t
{
  int target_val;
  int host_val;
};

int
xlate_arg(int target_val, struct xlate_table_t *map, int map_sz, char *name)
{
  int i;

  for (i=0; i < map_sz; i++)
    {
      if (target_val == map[i].target_val)
	return map[i].host_val;
    }

  /* not found, issue warning and return target_val */
  warn("could not translate argument for `%s': %d", name, target_val);
  return target_val;
}

/* internal system call buffer size, used primarily for file name arguments,
   argument larger than this will be truncated */
#define MAXBUFSIZE 		1024

/* total bytes to copy from a valid pointer argument for ioctl() calls,
   syscall.c does not decode ioctl() calls to determine the size of the
   arguments that reside in memory, instead, the ioctl() proxy simply copies
   NUM_IOCTL_BYTES bytes from the pointer argument to host memory */
#define NUM_IOCTL_BYTES		128
#define SYSCALL_BUFFER_SIZE 1024 // for places a byte array is needed

/* OSF ioctl() requests */
#define OSF_TIOCGETP		0x40067408
#define OSF_FIONREAD		0x4004667f

/* target stat() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct  osf_statbuf
{
  word_t osf_st_dev;
  word_t osf_st_ino;
  word_t osf_st_mode;
  half_t osf_st_nlink;
  half_t pad0;			/* to match Alpha/AXP padding... */
  word_t osf_st_uid;
  word_t osf_st_gid;
  word_t osf_st_rdev;
  word_t pad1;			/* to match Alpha/AXP padding... */
  qword_t osf_st_size;
  word_t osf_st_atime;
  word_t osf_st_spare1;
  word_t osf_st_mtime;
  word_t osf_st_spare2;
  word_t osf_st_ctime;
  word_t osf_st_spare3;
  word_t osf_st_blksize;
  word_t osf_st_blocks;
  word_t osf_st_gennum;
  word_t osf_st_spare4;
};

struct osf_sgttyb {
  byte_t sg_ispeed;	/* input speed */
  byte_t sg_ospeed;	/* output speed */
  byte_t sg_erase;	/* erase character */
  byte_t sg_kill;	/* kill character */
  shalf_t sg_flags;	/* mode flags */
};

#define OSF_NSIG		32

#define OSF_SIG_BLOCK		1
#define OSF_SIG_UNBLOCK		2
#define OSF_SIG_SETMASK		3

struct osf_sigcontext {
  qword_t sc_onstack;              /* sigstack state to restore */
  qword_t sc_mask;                 /* signal mask to restore */
  qword_t sc_pc;                   /* pc at time of signal */
  qword_t sc_ps;                   /* psl to retore */
  qword_t sc_regs[32];             /* processor regs 0 to 31 */
  qword_t sc_ownedfp;              /* fp has been used */
  qword_t sc_fpregs[32];           /* fp regs 0 to 31 */
  qword_t sc_fpcr;                 /* floating point control register */
  qword_t sc_fp_control;           /* software fpcr */
};

struct osf_statfs {
  shalf_t f_type;		/* type of filesystem (see below) */
  shalf_t f_flags;		/* copy of mount flags */
  word_t f_fsize;		/* fundamental filesystem block size */
  word_t f_bsize;		/* optimal transfer block size */
  word_t f_blocks;		/* total data blocks in file system, */
  /* note: may not represent fs size. */
  word_t f_bfree;		/* free blocks in fs */
  word_t f_bavail;		/* free blocks avail to non-su */
  word_t f_files;		/* total file nodes in file system */
  word_t f_ffree;		/* free file nodes in fs */
  qword_t f_fsid;		/* file system id */
  word_t f_spare[9];		/* spare for later */
};

struct osf_timeval
{
  sword_t osf_tv_sec;		/* seconds */
  sword_t osf_tv_usec;		/* microseconds */
};

struct osf_timezone
{
  sword_t osf_tz_minuteswest;	/* minutes west of Greenwich */
  sword_t osf_tz_dsttime;	/* type of dst correction */
};

/* target getrusage() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct osf_rusage
{
  struct osf_timeval osf_ru_utime;
  struct osf_timeval osf_ru_stime;
  sword_t osf_ru_maxrss;
  sword_t osf_ru_ixrss;
  sword_t osf_ru_idrss;
  sword_t osf_ru_isrss;
  sword_t osf_ru_minflt;
  sword_t osf_ru_majflt;
  sword_t osf_ru_nswap;
  sword_t osf_ru_inblock;
  sword_t osf_ru_oublock;
  sword_t osf_ru_msgsnd;
  sword_t osf_ru_msgrcv;
  sword_t osf_ru_nsignals;
  sword_t osf_ru_nvcsw;
  sword_t osf_ru_nivcsw;
};

struct osf_rlimit
{
  qword_t osf_rlim_cur;		/* current (soft) limit */
  qword_t osf_rlim_max;		/* maximum value for rlim_cur */
};

struct osf_sockaddr
{
  half_t sa_family;		/* address family, AF_xxx */
  byte_t sa_data[24];		/* 14 bytes of protocol address */
};

struct osf_iovec
{
  md_addr_t iov_base;		/* starting address */
  word_t iov_len;		/* length in bytes */
  word_t pad;
};

/* returns size of DIRENT structure */
#define OSF_DIRENT_SZ(STR)						\
  (sizeof(word_t) + 2*sizeof(half_t) + (((strlen(STR) + 1) + 3)/4)*4)

struct osf_dirent
{
  word_t d_ino;			/* file number of entry */
  half_t d_reclen;		/* length of this record */
  half_t d_namlen;		/* length of string in d_name */
  char d_name[256];		/* DUMMY NAME LENGTH */
				/* the real maximum length is */
				/* returned by pathconf() */
                                /* At this time, this MUST */
                                /* be 256 -- the kernel */
                                /* requires it */
};

/* open(2) flags for Alpha/AXP OSF target, syscall.c automagically maps
   between these codes to/from host open(2) flags */
#define OSF_O_RDONLY		0x0000
#define OSF_O_WRONLY		0x0001
#define OSF_O_RDWR		0x0002
#define OSF_O_NONBLOCK		0x0004
#define OSF_O_APPEND		0x0008
#define OSF_O_CREAT		0x0200
#define OSF_O_TRUNC		0x0400
#define OSF_O_EXCL		0x0800
#define OSF_O_NOCTTY		0x1000
#define OSF_O_SYNC		0x4000

/* open(2) flags translation table for SimpleScalar target */
struct {
  int osf_flag;
  int local_flag;
} osf_flag_table[] = {
  /* target flag */	/* host flag */
#ifdef _MSC_VER
  { OSF_O_RDONLY,	_O_RDONLY },
  { OSF_O_WRONLY,	_O_WRONLY },
  { OSF_O_RDWR,		_O_RDWR },
  { OSF_O_APPEND,	_O_APPEND },
  { OSF_O_CREAT,	_O_CREAT },
  { OSF_O_TRUNC,	_O_TRUNC },
  { OSF_O_EXCL,		_O_EXCL },
#ifdef _O_NONBLOCK
  { OSF_O_NONBLOCK,	_O_NONBLOCK },
#endif
#ifdef _O_NOCTTY
  { OSF_O_NOCTTY,	_O_NOCTTY },
#endif
#ifdef _O_SYNC
  { OSF_O_SYNC,		_O_SYNC },
#endif
#else /* !_MSC_VER */
  { OSF_O_RDONLY,	O_RDONLY },
  { OSF_O_WRONLY,	O_WRONLY },
  { OSF_O_RDWR,		O_RDWR },
  { OSF_O_APPEND,	O_APPEND },
  { OSF_O_CREAT,	O_CREAT },
  { OSF_O_TRUNC,	O_TRUNC },
  { OSF_O_EXCL,		O_EXCL },
  { OSF_O_NONBLOCK,	O_NONBLOCK },
  { OSF_O_NOCTTY,	O_NOCTTY },
#ifdef O_SYNC
  { OSF_O_SYNC,		O_SYNC },
#endif
#endif /* _MSC_VER */
};
#define OSF_NFLAGS	(sizeof(osf_flag_table)/sizeof(osf_flag_table[0]))

qword_t sigmask = 0;

qword_t sigaction_array[OSF_NSIG] =
 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* setsockopt option names */
#define OSF_SO_DEBUG		0x0001
#define OSF_SO_ACCEPTCONN	0x0002
#define OSF_SO_REUSEADDR	0x0004
#define OSF_SO_KEEPALIVE	0x0008
#define OSF_SO_DONTROUTE	0x0010
#define OSF_SO_BROADCAST	0x0020
#define OSF_SO_USELOOPBACK	0x0040
#define OSF_SO_LINGER		0x0080
#define OSF_SO_OOBINLINE	0x0100
#define OSF_SO_REUSEPORT	0x0200

struct xlate_table_t sockopt_map[] =
{
  { OSF_SO_DEBUG,	SO_DEBUG },
#ifdef SO_ACCEPTCONN
  { OSF_SO_ACCEPTCONN,	SO_ACCEPTCONN },
#endif
  { OSF_SO_REUSEADDR,	SO_REUSEADDR },
  { OSF_SO_KEEPALIVE,	SO_KEEPALIVE },
  { OSF_SO_DONTROUTE,	SO_DONTROUTE },
  { OSF_SO_BROADCAST,	SO_BROADCAST },
#ifdef SO_USELOOPBACK
  { OSF_SO_USELOOPBACK,	SO_USELOOPBACK },
#endif
  { OSF_SO_LINGER,	SO_LINGER },
  { OSF_SO_OOBINLINE,	SO_OOBINLINE },
#ifdef SO_REUSEPORT
  { OSF_SO_REUSEPORT,	SO_REUSEPORT }
#endif
};

/* setsockopt TCP options */
#define OSF_TCP_NODELAY		0x01 /* don't delay send to coalesce packets */
#define OSF_TCP_MAXSEG		0x02 /* maximum segment size */
#define OSF_TCP_RPTR2RXT	0x03 /* set repeat count for R2 RXT timer */
#define OSF_TCP_KEEPIDLE	0x04 /* secs before initial keepalive probe */
#define OSF_TCP_KEEPINTVL	0x05 /* seconds between keepalive probes */
#define OSF_TCP_KEEPCNT		0x06 /* num of keepalive probes before drop */
#define OSF_TCP_KEEPINIT	0x07 /* initial connect timeout (seconds) */
#define OSF_TCP_PUSH		0x08 /* set push bit in outbnd data packets */
#define OSF_TCP_NODELACK	0x09 /* don't delay send to coalesce packets */

struct xlate_table_t tcpopt_map[] =
{
  { OSF_TCP_NODELAY,	TCP_NODELAY },
  { OSF_TCP_MAXSEG,	TCP_MAXSEG },
#ifdef TCP_RPTR2RXT
  { OSF_TCP_RPTR2RXT,	TCP_RPTR2RXT },
#endif
#ifdef TCP_KEEPIDLE
  { OSF_TCP_KEEPIDLE,	TCP_KEEPIDLE },
#endif
#ifdef TCP_KEEPINTVL
  { OSF_TCP_KEEPINTVL,	TCP_KEEPINTVL },
#endif
#ifdef TCP_KEEPCNT
  { OSF_TCP_KEEPCNT,	TCP_KEEPCNT },
#endif
#ifdef TCP_KEEPINIT
  { OSF_TCP_KEEPINIT,	TCP_KEEPINIT },
#endif
#ifdef TCP_PUSH
  { OSF_TCP_PUSH,	TCP_PUSH },
#endif
#ifdef TCP_NODELACK
  { OSF_TCP_NODELACK,	TCP_NODELACK }
#endif
};

/* setsockopt level names */
#define OSF_SOL_SOCKET		0xffff	/* options for socket level */
#define OSF_SOL_IP		0	/* dummy for IP */
#define OSF_SOL_TCP		6	/* tcp */
#define OSF_SOL_UDP		17	/* user datagram protocol */

struct xlate_table_t socklevel_map[] =
{
#if defined(__svr4__) || defined(__osf__)
  { OSF_SOL_SOCKET,	SOL_SOCKET },
  { OSF_SOL_IP,		IPPROTO_IP },
  { OSF_SOL_TCP,	IPPROTO_TCP },
  { OSF_SOL_UDP,	IPPROTO_UDP }
#else
  { OSF_SOL_SOCKET,	SOL_SOCKET },
  { OSF_SOL_IP,		SOL_IP },
  { OSF_SOL_TCP,	SOL_TCP },
  { OSF_SOL_UDP,	SOL_UDP }
#endif
};

/* socket() address families */
#define OSF_AF_UNSPEC		0
#define OSF_AF_UNIX		1	/* Unix domain sockets */
#define OSF_AF_INET		2	/* internet IP protocol */
#define OSF_AF_IMPLINK		3	/* arpanet imp addresses */
#define OSF_AF_PUP		4	/* pup protocols: e.g. BSP */
#define OSF_AF_CHAOS		5	/* mit CHAOS protocols */
#define OSF_AF_NS		6	/* XEROX NS protocols */
#define OSF_AF_ISO		7	/* ISO protocols */

struct xlate_table_t family_map[] =
{
  { OSF_AF_UNSPEC,	AF_UNSPEC },
  { OSF_AF_UNIX,	AF_UNIX },
  { OSF_AF_INET,	AF_INET },
#ifdef AF_IMPLINK
  { OSF_AF_IMPLINK,	AF_IMPLINK },
#endif
#ifdef AF_PUP
  { OSF_AF_PUP,		AF_PUP },
#endif
#ifdef AF_CHAOS
  { OSF_AF_CHAOS,	AF_CHAOS },
#endif
#ifdef AF_NS
  { OSF_AF_NS,		AF_NS },
#endif
#ifdef AF_ISO
  { OSF_AF_ISO,		AF_ISO }
#endif
};

/* socket() socket types */
#define OSF_SOCK_STREAM		1	/* stream (connection) socket */
#define OSF_SOCK_DGRAM		2	/* datagram (conn.less) socket */
#define OSF_SOCK_RAW		3	/* raw socket */
#define OSF_SOCK_RDM		4	/* reliably-delivered message */
#define OSF_SOCK_SEQPACKET	5	/* sequential packet socket */

struct xlate_table_t socktype_map[] =
{
  { OSF_SOCK_STREAM,	SOCK_STREAM },
  { OSF_SOCK_DGRAM,	SOCK_DGRAM },
  { OSF_SOCK_RAW,	SOCK_RAW },
  { OSF_SOCK_RDM,	SOCK_RDM },
  { OSF_SOCK_SEQPACKET,	SOCK_SEQPACKET }
};

/* OSF table() call. Right now, we only support TBL_SYSINFO queries */
#define OSF_TBL_SYSINFO		12
struct osf_tbl_sysinfo 
{
  long si_user;		/* user time */
  long si_nice;		/* nice time */
  long si_sys;		/* system time */
  long si_idle;		/* idle time */
  long si_hz;
  long si_phz;
  long si_boottime;	/* boot time in seconds */
  long wait;		/* wait time */
};


/* OSF SYSCALL -- standard system call sequence
   the kernel expects arguments to be passed with the normal C calling
   sequence; v0 should contain the system call number; on return from the
   kernel mode, a3 will be 0 to indicate no error and non-zero to indicate an
   error; if an error occurred v0 will contain an errno; if the kernel return
   an error, setup a valid gp and jmp to _cerror */

/* syscall proxy handler, architect registers and memory are assumed to be
   precise when this function is called, register and memory are updated with
   the results of the sustem call */

typedef enum {
  /* 0  r0 */  R_V0,    // return value
  /* 1  r1 */  R_T0,    // temporary register
  /* 2  r2 */  R_T1,    // temporary register
  /* 3  r3 */  R_T2,    // temporary register
  /* 4  r4 */  R_T3,    // temporary register
  /* 5  r5 */  R_T4,    // temporary register
  /* 6  r6 */  R_T5,    // temporary register
  /* 7  r7 */  R_T6,    // temporary register
  /* 8  r8 */  R_T7,    // temporary register
  /* 9  r9 */  R_S0,    // saved register
  /* 10 r10 */ R_S1,    // saved register
  /* 11 r11 */ R_S2,    // saved register
  /* 12 r12 */ R_S3,    // saved register
  /* 13 r13 */ R_S4,    // saved register
  /* 14 r14 */ R_S5,    // saved register
  /* 15 r15 */ R_FP,    // frame pointer
  /* 16 r16 */ R_A0,    // function argument
  /* 17 r17 */ R_A1,    // function argument
  /* 18 r18 */ R_A2,    // function argument
  /* 19 r19 */ R_A3,    // function argument
  /* 20 r20 */ R_A4,    // function argument
  /* 21 r21 */ R_A5,    // function argument
  /* 22 r22 */ R_T8,    // temporary register
  /* 23 r23 */ R_T9,    // temporary register
  /* 24 r24 */ R_T10,   // temporary register
  /* 25 r25 */ R_T11,   // temporary register
  /* 26 r26 */ R_RA,    // function return address
  /* 27 r27 */ R_PV,    // procedure value
  /* 28 r28 */ R_AT,    // reserved for assembler
  /* 29 r29 */ R_GP,    // global pointer
  /* 30 r30 */ R_SP,    // stack pointer
  /* 31 r31 */ R_IZERO, // integer zero
  /* 32 f0  */ R_F0,    // floating point register
  /* 33 f1  */ R_F1,    // floating point register
  /* 34 f2  */ R_F2,    // floating point register
  /* 35 f3  */ R_F3,    // floating point register
  /* 36 f4  */ R_F4,    // floating point register
  /* 37 f5  */ R_F5,    // floating point register
  /* 38 f6  */ R_F6,    // floating point register
  /* 39 f7  */ R_F7,    // floating point register
  /* 40 f8  */ R_F8,    // floating point register
  /* 41 f9  */ R_F9,    // floating point register
  /* 42 f10 */ R_F10,   // floating point register
  /* 43 f11 */ R_F11,   // floating point register
  /* 44 f12 */ R_F12,   // floating point register
  /* 45 f13 */ R_F13,   // floating point register
  /* 46 f14 */ R_F14,   // floating point register
  /* 47 f15 */ R_F15,   // floating point register
  /* 48 f16 */ R_F16,   // floating point register
  /* 49 f17 */ R_F17,   // floating point register
  /* 50 f18 */ R_F18,   // floating point register
  /* 51 f19 */ R_F19,   // floating point register
  /* 52 f20 */ R_F20,   // floating point register
  /* 53 f21 */ R_F21,   // floating point register
  /* 54 f22 */ R_F22,   // floating point register
  /* 55 f23 */ R_F23,   // floating point register
  /* 56 f24 */ R_F24,   // floating point register
  /* 57 f25 */ R_F25,   // floating point register
  /* 58 f26 */ R_F26,   // floating point register
  /* 59 f27 */ R_F27,   // floating point register
  /* 60 f28 */ R_F28,   // floating point register
  /* 61 f29 */ R_F29,   // floating point register
  /* 62 f30 */ R_F30,   // floating point register
  /* 63 f31 */ R_FZERO, // floating point zero
  /* 64     */ R_FPCR,  // floating point condition register
  /* 65     */ R_UNIQ,  // process unique register
  /* 66     */ NUM_REGS // number of registers, not an actual register
} RegNames;

typedef enum {
	false,
	true
} bool; 

typedef enum 
{
  // OSF system call codes //////////////////////////////////////////////////
  /*   0 */  SYSCALL_OSF_SYSCALL,
  /*   1 */  SYSCALL_EXIT,
  /*   2 */  SYSCALL_FORK,
  /*   3 */  SYSCALL_READ,
  /*   4 */  SYSCALL_WRITE,
  /*   5 */  SYSCALL_OSF_OLD_OPEN,
  /*   6 */  SYSCALL_CLOSE,
  /*   7 */  SYSCALL_OSF_WAIT4,
  /*   8 */  SYSCALL_OSF_OLD_CREAT,
  /*   9 */  SYSCALL_LINK,
  /*  10 */  SYSCALL_UNLINK,
  /*  11 */  SYSCALL_OSF_EXECVE,
  /*  12 */  SYSCALL_CHDIR,
  /*  13 */  SYSCALL_FCHDIR,
  /*  14 */  SYSCALL_MKNOD,
  /*  15 */  SYSCALL_CHMOD,
  /*  16 */  SYSCALL_CHOWN,
  /*  17 */  SYSCALL_BRK,
  /*  18 */  SYSCALL_OSF_GETFSSTAT,
  /*  19 */  SYSCALL_LSEEK,
  /*  20 */  SYSCALL_GETXPID,
  /*  21 */  SYSCALL_OSF_MOUNT,
  /*  22 */  SYSCALL_UMOUNT,
  /*  23 */  SYSCALL_SETUID,
  /*  24 */  SYSCALL_GETXUID,
  /*  25 */  SYSCALL_EXEC_WITH_LOADER,
  /*  26 */  SYSCALL_OSF_PTRACE,
  /*  27 */  SYSCALL_OSF_NRECVMSG,
  /*  28 */  SYSCALL_OSF_NSENDMSG,
  /*  29 */  SYSCALL_OSF_NRECVFROM,
  /*  30 */  SYSCALL_OSF_NACCEPT,
  /*  31 */  SYSCALL_OSF_NGETPEERNAME,
  /*  32 */  SYSCALL_OSF_NGETSOCKNAME,
  /*  33 */  SYSCALL_ACCESS,
  /*  34 */  SYSCALL_OSF_CHFLAGS,
  /*  35 */  SYSCALL_OSF_FCHFLAGS,
  /*  36 */  SYSCALL_SYNC,
  /*  37 */  SYSCALL_KILL,
  /*  38 */  SYSCALL_OSF_OLD_STAT,
  /*  39 */  SYSCALL_SETPGID,
  /*  40 */  SYSCALL_OSF_OLD_LSTAT,
  /*  41 */  SYSCALL_DUP,
  /*  42 */  SYSCALL_PIPE,
  /*  43 */  SYSCALL_OSF_SET_PROGRAM_ATTRIBUTES,
  /*  44 */  SYSCALL_OSF_PROFIL,
  /*  45 */  SYSCALL_OPEN,
  /*  46 */  SYSCALL_OSF_OLD_SIGACTION,
  /*  47 */  SYSCALL_GETXGID,
  /*  48 */  SYSCALL_OSF_SIGPROCMASK,
  /*  49 */  SYSCALL_OSF_GETLOGIN,
  /*  50 */  SYSCALL_OSF_SETLOGIN,
  /*  51 */  SYSCALL_ACCT,
  /*  52 */  SYSCALL_SIGPENDING,
  /*  53 */  SYSCALL_OSF_CLASSCNTL,
  /*  54 */  SYSCALL_IOCTL,
  /*  55 */  SYSCALL_OSF_REBOOT,
  /*  56 */  SYSCALL_OSF_REVOKE,
  /*  57 */  SYSCALL_SYMLINK,
  /*  58 */  SYSCALL_READLINK,
  /*  59 */  SYSCALL_EXECVE,
  /*  60 */  SYSCALL_UMASK,
  /*  61 */  SYSCALL_CHROOT,
  /*  62 */  SYSCALL_OSF_OLD_FSTAT,
  /*  63 */  SYSCALL_GETPGRP,
  /*  64 */  SYSCALL_GETPAGESIZE,
  /*  65 */  SYSCALL_OSF_MREMAP,
  /*  66 */  SYSCALL_VFORK,
  /*  67 */  SYSCALL_STAT,
  /*  68 */  SYSCALL_LSTAT,
  /*  69 */  SYSCALL_OSF_SBRK,
  /*  70 */  SYSCALL_OSF_SSTK,
  /*  71 */  SYSCALL_MMAP,
  /*  72 */  SYSCALL_OSF_OLD_VADVISE,
  /*  73 */  SYSCALL_MUNMAP,
  /*  74 */  SYSCALL_MPROTECT,
  /*  75 */  SYSCALL_MADVISE,
  /*  76 */  SYSCALL_VHANGUP,
  /*  77 */  SYSCALL_OSF_KMODCALL,
  /*  78 */  SYSCALL_OSF_MINCORE,
  /*  79 */  SYSCALL_GETGROUPS,
  /*  80 */  SYSCALL_SETGROUPS,
  /*  81 */  SYSCALL_OSF_OLD_GETPGRP,
  /*  82 */  SYSCALL_SETPGRP,
  /*  83 */  SYSCALL_OSF_SETITIMER,
  /*  84 */  SYSCALL_OSF_OLD_WAIT,
  /*  85 */  SYSCALL_OSF_TABLE,
  /*  86 */  SYSCALL_OSF_GETITIMER,
  /*  87 */  SYSCALL_GETHOSTNAME,
  /*  88 */  SYSCALL_SETHOSTNAME,
  /*  89 */  SYSCALL_GETDTABLESIZE,
  /*  90 */  SYSCALL_DUP2,
  /*  91 */  SYSCALL_FSTAT,
  /*  92 */  SYSCALL_FCNTL,
  /*  93 */  SYSCALL_OSF_SELECT,
  /*  94 */  SYSCALL_POLL,
  /*  95 */  SYSCALL_FSYNC,
  /*  96 */  SYSCALL_SETPRIORITY,
  /*  97 */  SYSCALL_SOCKET,
  /*  98 */  SYSCALL_CONNECT,
  /*  99 */  SYSCALL_ACCEPT,
  /* 100 */  SYSCALL_GETPRIORITY,
  /* 101 */  SYSCALL_SEND,
  /* 102 */  SYSCALL_RECV,
  /* 103 */  SYSCALL_SIGRETURN,
  /* 104 */  SYSCALL_BIND,
  /* 105 */  SYSCALL_SETSOCKOPT,
  /* 106 */  SYSCALL_LISTEN,
  /* 107 */  SYSCALL_OSF_PLOCK,
  /* 108 */  SYSCALL_OSF_OLD_SIGVEC,
  /* 109 */  SYSCALL_OSF_OLD_SIGBLOCK,
  /* 110 */  SYSCALL_OSF_OLD_SIGSETMASK,
  /* 111 */  SYSCALL_SIGSUSPEND,
  /* 112 */  SYSCALL_OSF_SIGSTACK,
  /* 113 */  SYSCALL_RECVMSG,
  /* 114 */  SYSCALL_SENDMSG,
  /* 115 */  SYSCALL_OSF_OLD_VTRACE,
  /* 116 */  SYSCALL_OSF_GETTIMEOFDAY,
  /* 117 */  SYSCALL_OSF_GETRUSAGE,
  /* 118 */  SYSCALL_GETSOCKOPT,
  /* 119 */  SYSCALL_NUMA_SYSCALLS,
  /* 120 */  SYSCALL_READV,
  /* 121 */  SYSCALL_WRITEV,
  /* 122 */  SYSCALL_OSF_SETTIMEOFDAY,
  /* 123 */  SYSCALL_FCHOWN,
  /* 124 */  SYSCALL_FCHMOD,
  /* 125 */  SYSCALL_RECVFROM,
  /* 126 */  SYSCALL_SETREUID,
  /* 127 */  SYSCALL_SETREGID,
  /* 128 */  SYSCALL_RENAME,
  /* 129 */  SYSCALL_TRUNCATE,
  /* 130 */  SYSCALL_FTRUNCATE,
  /* 131 */  SYSCALL_FLOCK,
  /* 132 */  SYSCALL_SETGID,
  /* 133 */  SYSCALL_SENDTO,
  /* 134 */  SYSCALL_SHUTDOWN,
  /* 135 */  SYSCALL_SOCKETPAIR,
  /* 136 */  SYSCALL_MKDIR,
  /* 137 */  SYSCALL_RMDIR,
  /* 138 */  SYSCALL_OSF_UTIMES,
  /* 139 */  SYSCALL_OSF_OLD_SIGRETURN,
  /* 140 */  SYSCALL_OSF_ADJTIME,
  /* 141 */  SYSCALL_GETPEERNAME,
  /* 142 */  SYSCALL_OSF_GETHOSTID,
  /* 143 */  SYSCALL_OSF_SETHOSTID,
  /* 144 */  SYSCALL_GETRLIMIT,
  /* 145 */  SYSCALL_SETRLIMIT,
  /* 146 */  SYSCALL_OSF_OLD_KILLPG,
  /* 147 */  SYSCALL_SETSID,
  /* 148 */  SYSCALL_QUOTACTL,
  /* 149 */  SYSCALL_OSF_OLDQUOTA,
  /* 150 */  SYSCALL_GETSOCKNAME,
  /* 151 */  SYSCALL_OSF_PREAD,
  /* 152 */  SYSCALL_OSF_PWRITE,
  /* 153 */  SYSCALL_OSF_PID_BLOCK,
  /* 154 */  SYSCALL_OSF_PID_UNBLOCK,
  /* 155 */  SYSCALL_OSF_SIGNAL_URTI,
  /* 156 */  SYSCALL_SIGACTION,
  /* 157 */  SYSCALL_OSF_SIGWAITPRIM,
  /* 158 */  SYSCALL_OSF_NFSSVC,
  /* 159 */  SYSCALL_OSF_GETDIRENTRIES,
  /* 160 */  SYSCALL_OSF_STATFS,
  /* 161 */  SYSCALL_OSF_FSTATFS,
  /* 162 */  SYSCALL_UNKNOWN_162,
  /* 163 */  SYSCALL_OSF_ASYNC_DAEMON,
  /* 164 */  SYSCALL_OSF_GETFH,
  /* 165 */  SYSCALL_OSF_GETDOMAINNAME,
  /* 166 */  SYSCALL_SETDOMAINNAME,
  /* 167 */  SYSCALL_UNKNOWN_167,
  /* 168 */  SYSCALL_UNKNOWN_168,
  /* 169 */  SYSCALL_OSF_EXPORTFS,
  /* 170 */  SYSCALL_UNKNOWN_170,
  /* 171 */  SYSCALL_UNKNOWN_171,
  /* 172 */  SYSCALL_UNKNOWN_172,
  /* 173 */  SYSCALL_UNKNOWN_173,
  /* 174 */  SYSCALL_UNKNOWN_174,
  /* 175 */  SYSCALL_UNKNOWN_175,
  /* 176 */  SYSCALL_UNKNOWN_176,
  /* 177 */  SYSCALL_UNKNOWN_177,
  /* 178 */  SYSCALL_UNKNOWN_178,
  /* 179 */  SYSCALL_UNKNOWN_179,
  /* 180 */  SYSCALL_UNKNOWN_180,
  /* 181 */  SYSCALL_OSF_ALT_PLOCK,
  /* 182 */  SYSCALL_UNKNOWN_182,
  /* 183 */  SYSCALL_UNKNOWN_183,
  /* 184 */  SYSCALL_OSF_GETMNT,
  /* 185 */  SYSCALL_UNKNOWN_185,
  /* 186 */  SYSCALL_UNKNOWN_186,
  /* 187 */  SYSCALL_OSF_ALT_SIGPENDING,
  /* 188 */  SYSCALL_OSF_ALT_SETSID,
  /* 189 */  SYSCALL_UNKNOWN_189,
  /* 190 */  SYSCALL_UNKNOWN_190,
  /* 191 */  SYSCALL_UNKNOWN_191,
  /* 192 */  SYSCALL_UNKNOWN_192,
  /* 193 */  SYSCALL_UNKNOWN_193,
  /* 194 */  SYSCALL_UNKNOWN_194,
  /* 195 */  SYSCALL_UNKNOWN_195,
  /* 196 */  SYSCALL_UNKNOWN_196,
  /* 197 */  SYSCALL_UNKNOWN_197,
  /* 198 */  SYSCALL_UNKNOWN_198,
  /* 199 */  SYSCALL_OSF_SWAPON,
  /* 200 */  SYSCALL_MSGCTL,
  /* 201 */  SYSCALL_MSGGET,
  /* 202 */  SYSCALL_MSGRCV,
  /* 203 */  SYSCALL_MSGSND,
  /* 204 */  SYSCALL_SEMCTL,
  /* 205 */  SYSCALL_SEMGET,
  /* 206 */  SYSCALL_SEMOP,
  /* 207 */  SYSCALL_OSF_UTSNAME,
  /* 208 */  SYSCALL_LCHOWN,
  /* 209 */  SYSCALL_OSF_SHMAT,
  /* 210 */  SYSCALL_SHMCTL,
  /* 211 */  SYSCALL_SHMDT,
  /* 212 */  SYSCALL_SHMGET,
  /* 213 */  SYSCALL_OSF_MVALID,
  /* 214 */  SYSCALL_OSF_GETADDRESSCONF,
  /* 215 */  SYSCALL_OSF_MSLEEP,
  /* 216 */  SYSCALL_OSF_MWAKEUP,
  /* 217 */  SYSCALL_MSYNC,
  /* 218 */  SYSCALL_OSF_SIGNAL,
  /* 219 */  SYSCALL_OSF_UTC_GETTIME,
  /* 220 */  SYSCALL_OSF_UTC_ADJTIME,
  /* 221 */  SYSCALL_UNKNOWN_221,
  /* 222 */  SYSCALL_OSF_SECURITY,
  /* 223 */  SYSCALL_OSF_KLOADCALL,
  /* 224 */  SYSCALL_UNKNOWN_224,
  /* 225 */  SYSCALL_UNKNOWN_225,
  /* 226 */  SYSCALL_UNKNOWN_226,
  /* 227 */  SYSCALL_UNKNOWN_227,
  /* 228 */  SYSCALL_UNKNOWN_228,
  /* 229 */  SYSCALL_UNKNOWN_229,
  /* 230 */  SYSCALL_UNKNOWN_230,
  /* 231 */  SYSCALL_UNKNOWN_231,
  /* 232 */  SYSCALL_UNKNOWN_232,
  /* 233 */  SYSCALL_GETPGID,
  /* 234 */  SYSCALL_GETSID,
  /* 235 */  SYSCALL_SIGALTSTACK,
  /* 236 */  SYSCALL_OSF_WAITID,
  /* 237 */  SYSCALL_OSF_PRIOCNTLSET,
  /* 238 */  SYSCALL_OSF_SIGSENDSET,
  /* 239 */  SYSCALL_OSF_SET_SPECULATIVE,
  /* 240 */  SYSCALL_OSF_MSFS_SYSCALL,
  /* 241 */  SYSCALL_OSF_SYSINFO,
  /* 242 */  SYSCALL_OSF_UADMIN,
  /* 243 */  SYSCALL_OSF_FUSER,
  /* 244 */  SYSCALL_OSF_PROPLIST_SYSCALL,
  /* 245 */  SYSCALL_OSF_NTP_ADJTIME,
  /* 246 */  SYSCALL_OSF_NTP_GETTIME,
  /* 247 */  SYSCALL_OSF_PATHCONF,
  /* 248 */  SYSCALL_OSF_FPATHCONF,
  /* 249 */  SYSCALL_UNKNOWN_249,
  /* 250 */  SYSCALL_OSF_USWITCH,
  /* 251 */  SYSCALL_OSF_USLEEP_THREAD,
  /* 252 */  SYSCALL_OSF_AUDCNTL,
  /* 253 */  SYSCALL_OSF_AUDGEN,
  /* 254 */  SYSCALL_SYSFS,
  /* 255 */  SYSCALL_OSF_SUBSYS_INFO,
  /* 256 */  SYSCALL_OSF_GETSYSINFO,
  /* 257 */  SYSCALL_OSF_SETSYSINFO,
  /* 258 */  SYSCALL_OSF_AFS_SYSCALL,
  /* 259 */  SYSCALL_OSF_SWAPCTL,
  /* 260 */  SYSCALL_OSF_MEMCNTL,
  /* 261 */  SYSCALL_OSF_FDATASYNC,
  /* 262 */  SYSCALL_UNKNOWN_262,
  /* 263 */  SYSCALL_UNKNOWN_263,
  /* 264 */  SYSCALL_UNKNOWN_264,
  /* 265 */  SYSCALL_UNKNOWN_265,
  /* 266 */  SYSCALL_UNKNOWN_266,
  /* 267 */  SYSCALL_UNKNOWN_267,
  /* 268 */  SYSCALL_UNKNOWN_268,
  /* 269 */  SYSCALL_UNKNOWN_269,
  /* 270 */  SYSCALL_UNKNOWN_270,
  /* 271 */  SYSCALL_UNKNOWN_271,
  /* 272 */  SYSCALL_UNKNOWN_272,
  /* 273 */  SYSCALL_UNKNOWN_273,
  /* 274 */  SYSCALL_UNKNOWN_274,
  /* 275 */  SYSCALL_UNKNOWN_275,
  /* 276 */  SYSCALL_UNKNOWN_276,
  /* 277 */  SYSCALL_UNKNOWN_277,
  /* 278 */  SYSCALL_UNKNOWN_278,
  /* 279 */  SYSCALL_UNKNOWN_279,
  /* 280 */  SYSCALL_UNKNOWN_280,
  /* 281 */  SYSCALL_UNKNOWN_281,
  /* 282 */  SYSCALL_UNKNOWN_282,
  /* 283 */  SYSCALL_UNKNOWN_283,
  /* 284 */  SYSCALL_UNKNOWN_284,
  /* 285 */  SYSCALL_UNKNOWN_285,
  /* 286 */  SYSCALL_UNKNOWN_286,
  /* 287 */  SYSCALL_UNKNOWN_287,
  /* 288 */  SYSCALL_UNKNOWN_288,
  /* 289 */  SYSCALL_UNKNOWN_289,
  /* 290 */  SYSCALL_UNKNOWN_290,
  /* 291 */  SYSCALL_UNKNOWN_291,
  /* 292 */  SYSCALL_UNKNOWN_292,
  /* 293 */  SYSCALL_UNKNOWN_293,
  /* 294 */  SYSCALL_UNKNOWN_294,
  /* 295 */  SYSCALL_UNKNOWN_295,
  /* 296 */  SYSCALL_UNKNOWN_296,
  /* 297 */  SYSCALL_UNKNOWN_297,
  /* 298 */  SYSCALL_UNKNOWN_298,
  /* 299 */  SYSCALL_UNKNOWN_299,

  // Linux-specific system call codes ///////////////////////////////////////
  /* 300 */  SYSCALL_BDFLUSH,
  /* 301 */  SYSCALL_SETHAE,
  /* 302 */  SYSCALL_MOUNT,
  /* 303 */  SYSCALL_OLD_ADJTIMEX,
  /* 304 */  SYSCALL_SWAPOFF,
  /* 305 */  SYSCALL_GETDENTS,
  /* 306 */  SYSCALL_CREATE_MODULE,
  /* 307 */  SYSCALL_INIT_MODULE,
  /* 308 */  SYSCALL_DELETE_MODULE,
  /* 309 */  SYSCALL_GET_KERNEL_SYMS,
  /* 310 */  SYSCALL_SYSLOG,
  /* 311 */  SYSCALL_REBOOT,
  /* 312 */  SYSCALL_CLONE,
  /* 313 */  SYSCALL_USELIB,
  /* 314 */  SYSCALL_MLOCK,
  /* 315 */  SYSCALL_MUNLOCK,
  /* 316 */  SYSCALL_MLOCKALL,
  /* 317 */  SYSCALL_MUNLOCKALL,
  /* 318 */  SYSCALL_SYSINFO,
  /* 319 */  SYSCALL__SYSCTL,
  /* 320 */  SYSCALL_WAS_SYS_IDLE,
  /* 321 */  SYSCALL_OLDUMOUNT,
  /* 322 */  SYSCALL_SWAPON,
  /* 323 */  SYSCALL_TIMES,
  /* 324 */  SYSCALL_PERSONALITY,
  /* 325 */  SYSCALL_SETFSUID,
  /* 326 */  SYSCALL_SETFSGID,
  /* 327 */  SYSCALL_USTAT,
  /* 328 */  SYSCALL_STATFS,
  /* 329 */  SYSCALL_FSTATFS,
  /* 330 */  SYSCALL_SCHED_SETPARAM,
  /* 331 */  SYSCALL_SCHED_GETPARAM,
  /* 332 */  SYSCALL_SCHED_SETSCHEDULER,
  /* 333 */  SYSCALL_SCHED_GETSCHEDULER,
  /* 334 */  SYSCALL_SCHED_YIELD,
  /* 335 */  SYSCALL_SCHED_GET_PRIORITY_MAX,
  /* 336 */  SYSCALL_SCHED_GET_PRIORITY_MIN,
  /* 337 */  SYSCALL_SCHED_RR_GET_INTERVAL,
  /* 338 */  SYSCALL_AFS_SYSCALL,
  /* 339 */  SYSCALL_UNAME,
  /* 340 */  SYSCALL_NANOSLEEP,
  /* 341 */  SYSCALL_MREMAP,
  /* 342 */  SYSCALL_NFSSERVCTL,
  /* 343 */  SYSCALL_SETRESUID,
  /* 344 */  SYSCALL_GETRESUID,
  /* 345 */  SYSCALL_PCICONFIG_READ,
  /* 346 */  SYSCALL_PCICONFIG_WRITE,
  /* 347 */  SYSCALL_QUERY_MODULE,
  /* 348 */  SYSCALL_PRCTL,
  /* 349 */  SYSCALL_PREAD,
  /* 350 */  SYSCALL_PWRITE,
  /* 351 */  SYSCALL_RT_SIGRETURN,
  /* 352 */  SYSCALL_RT_SIGACTION,
  /* 353 */  SYSCALL_RT_SIGPROCMASK,
  /* 354 */  SYSCALL_RT_SIGPENDING,
  /* 355 */  SYSCALL_RT_SIGTIMEDWAIT,
  /* 356 */  SYSCALL_RT_SIGQUEUEINFO,
  /* 357 */  SYSCALL_RT_SIGSUSPEND,
  /* 358 */  SYSCALL_SELECT,
  /* 359 */  SYSCALL_GETTIMEOFDAY,
  /* 360 */  SYSCALL_SETTIMEOFDAY,
  /* 361 */  SYSCALL_GETITIMER,
  /* 362 */  SYSCALL_SETITIMER,
  /* 363 */  SYSCALL_UTIMES,
  /* 364 */  SYSCALL_GETRUSAGE,
  /* 365 */  SYSCALL_WAIT4,
  /* 366 */  SYSCALL_ADJTIMEX,
  /* 367 */  SYSCALL_GETCWD,
  /* 368 */  SYSCALL_CAPGET,
  /* 369 */  SYSCALL_CAPSET,
  /* 370 */  SYSCALL_SENDFILE,
  /* 371 */  SYSCALL_SETRESGID,
  /* 372 */  SYSCALL_GETRESGID,
  /* 373 */  SYSCALL_DIPC,
  /* 374 */  SYSCALL_PIVOT_ROOT,
  /* 375 */  SYSCALL_MINCORE,
  /* 376 */  SYSCALL_PCICONFIG_IOBASE,
  /* 377 */  SYSCALL_GETDENTS64,
  /* 378 */  SYSCALL_GETTID,
  /* 379 */  SYSCALL_READAHEAD,
  /* 380 */  SYSCALL_SECURITY,
  /* 381 */  SYSCALL_TKILL,
  /* 382 */  SYSCALL_SETXATTR,
  /* 383 */  SYSCALL_LSETXATTR,
  /* 384 */  SYSCALL_FSETXATTR,
  /* 385 */  SYSCALL_GETXATTR,
  /* 386 */  SYSCALL_LGETXATTR,
  /* 387 */  SYSCALL_FGETXATTR,
  /* 388 */  SYSCALL_LISTXATTR,
  /* 389 */  SYSCALL_LLISTXATTR,
  /* 390 */  SYSCALL_FLISTXATTR,
  /* 391 */  SYSCALL_REMOVEXATTR,
  /* 392 */  SYSCALL_LREMOVEXATTR,
  /* 393 */  SYSCALL_FREMOVEXATTR,
  /* 394 */  SYSCALL_FUTEX,
  /* 395 */  SYSCALL_SCHED_SETAFFINITY,
  /* 396 */  SYSCALL_SCHED_GETAFFINITY,
  /* 397 */  SYSCALL_TUXCALL,
  /* 398 */  SYSCALL_IO_SETUP,
  /* 399 */  SYSCALL_IO_DESTROY,
  /* 400 */  SYSCALL_IO_GETEVENTS,
  /* 401 */  SYSCALL_IO_SUBMIT,
  /* 402 */  SYSCALL_IO_CANCEL,
  /* 403 */  SYSCALL_UNKNOWN_403,
  /* 404 */  SYSCALL_UNKNOWN_404,
  /* 405 */  SYSCALL_EXIT_GROUP,
  /* 406 */  SYSCALL_LOOKUP_DCOOKIE,
  /* 407 */  SYSCALL_SYS_EPOLL_CREATE,
  /* 408 */  SYSCALL_SYS_EPOLL_CTL,
  /* 409 */  SYSCALL_SYS_EPOLL_WAIT,
  /* 410 */  SYSCALL_REMAP_FILE_PAGES,
  /* 411 */  SYSCALL_SET_TID_ADDRESS,
  /* 412 */  SYSCALL_RESTART_SYSCALL,
  /* 413 */  SYSCALL_FADVISE64,
  /* 414 */  SYSCALL_TIMER_CREATE,
  /* 415 */  SYSCALL_TIMER_SETTIME,
  /* 416 */  SYSCALL_TIMER_GETTIME,
  /* 417 */  SYSCALL_TIMER_GETOVERRUN,
  /* 418 */  SYSCALL_TIMER_DELETE,
  /* 419 */  SYSCALL_CLOCK_SETTIME,
  /* 420 */  SYSCALL_CLOCK_GETTIME,
  /* 421 */  SYSCALL_CLOCK_GETRES,
  /* 422 */  SYSCALL_CLOCK_NANOSLEEP,
  /* 423 */  SYSCALL_SEMTIMEDOP,
  /* 424 */  SYSCALL_TGKILL,
  /* 425 */  SYSCALL_STAT64,
  /* 426 */  SYSCALL_LSTAT64,
  /* 427 */  SYSCALL_FSTAT64,
  /* 428 */  SYSCALL_VSERVER,
  /* 429 */  SYSCALL_MBIND,
  /* 430 */  SYSCALL_GET_MEMPOLICY,
  /* 431 */  SYSCALL_SET_MEMPOLICY,
  /* 432 */  SYSCALL_MQ_OPEN,
  /* 433 */  SYSCALL_MQ_UNLINK,
  /* 434 */  SYSCALL_MQ_TIMEDSEND,
  /* 435 */  SYSCALL_MQ_TIMEDRECEIVE,
  /* 436 */  SYSCALL_MQ_NOTIFY,
  /* 437 */  SYSCALL_MQ_GETSETATTR,
  /* 438 */  SYSCALL_WAITID,
  /* 439 */  SYSCALL_ADD_KEY,
  /* 440 */  SYSCALL_REQUEST_KEY,
  /* 441 */  SYSCALL_KEYCTL
} AlphaLinuxSyscalls;

void translate_stat_buf(alpha_stat *t_buf, struct stat *h_buf)
{
  // translate host stat buf to target stat buf
  t_buf->st_dev     = h_buf->st_dev;
  t_buf->st_ino     = h_buf->st_ino;
  t_buf->st_mode    = h_buf->st_mode;
  t_buf->st_nlink   = h_buf->st_nlink;
  t_buf->st_uid     = h_buf->st_uid;
  t_buf->st_gid     = h_buf->st_gid;
  t_buf->st_rdev    = h_buf->st_rdev;
  t_buf->st_size    = h_buf->st_size;
  t_buf->st_atimeX  = h_buf->st_atime;
  t_buf->st_mtimeX  = h_buf->st_mtime;
  t_buf->st_ctimeX  = h_buf->st_ctime;
  t_buf->st_blksize = h_buf->st_blksize;
  t_buf->st_blocks  = h_buf->st_blocks;
  t_buf->st_flags   = 0;
  t_buf->st_gen     = 0;
}

/*void translate_stat64_buf( alpha_stat64* t_buf, struct stat64 *h_buf)
{
  // translate host stat64 buf to target stat64 buf
  t_buf->st_dev     = h_buf->st_dev;
  t_buf->st_ino     = h_buf->st_ino;
  t_buf->st_rdev    = h_buf->st_rdev;
  t_buf->st_size    = h_buf->st_size;
  t_buf->st_blocks  = h_buf->st_blocks;
  
  t_buf->st_mode    = h_buf->st_mode;
  t_buf->st_uid     = h_buf->st_uid;
  t_buf->st_gid     = h_buf->st_gid;
  t_buf->st_blksize = h_buf->st_blksize;
  t_buf->st_nlink   = h_buf->st_nlink;

  t_buf->st_atimeX   = h_buf->st_atime;
  t_buf->st_atime_nsec = 0;
  t_buf->st_mtimeX   = h_buf->st_mtime;
  t_buf->st_mtime_nsec = 0;
  t_buf->st_ctimeX   = h_buf->st_ctime;
  t_buf->st_ctime_nsec = 0;
}*/

void
sys_syscall(struct regs_t *regs,	/* registers to access */
	    mem_access_fn mem_fn,	/* generic memory accessor */
	    struct mem_t *mem,		/* memory space to access */
	    md_inst_t inst,		/* system call inst */
	    int traceable)		/* traceable system call? */
{
  qword_t syscode = regs->regs_R[MD_REG_V0];

  /* fix for syscall() which uses CALL_PAL CALLSYS for making system calls */
  if (syscode == SYSCALL_OSF_SYSCALL)
    syscode = regs->regs_R[MD_REG_A0];

  /* first, check if an EIO trace is being consumed... */
  if (traceable && sim_eio_fd != NULL)
    {
      eio_read_trace(sim_eio_fd, sim_num_insn, regs, mem_fn, mem, inst);

      /* kludge fix for sigreturn(), it modifies all registers */
      if (syscode == SYSCALL_SIGRETURN)
	{
	  int i;
	  struct osf_sigcontext sc;
	  md_addr_t sc_addr = regs->regs_R[MD_REG_A0];

	  mem_bcopy(mem_fn, mem, Read, sc_addr, 
		    &sc, sizeof(struct osf_sigcontext));
	  regs->regs_NPC = sc.sc_pc;
	  for (i=0; i < 32; ++i)
	    regs->regs_R[i] = sc.sc_regs[i];
	  for (i=0; i < 32; ++i)
	    regs->regs_F.q[i] = sc.sc_fpregs[i];
	  regs->regs_C.fpcr = sc.sc_fpcr;
          program_complete = 1;
	}

      /* fini... */
      return;
    }

  /* no, OK execute the live system call... */
 
//  printf("REACHED A SYSCALL %d\n", syscode);

  // integer register 0 contains the system call code on alpha
  uint64_t syscall_code = regs->regs_R[R_V0];
  // default return value of 0
  int64_t return_value = 0;
  // default success value of false
  bool success = false;

  switch(syscode)
  {
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_READ: // 3
      {
        int64_t fd = regs->regs_R[R_A0];
        uint64_t vaddr = regs->regs_R[R_A1];
        int64_t count = regs->regs_R[R_A2];
        int64_t len;
        char *buf;

        // alllocate a buffer
        buf = (char *)calloc(count, sizeof(char));
        if (!buf) fatal("ArchLib:  syscall read ran out of memory");

        // perform the read
        len = read(fd, buf, count);

        // copy the result to the simulator
        if (len > 0)
        {
 	  mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, buf, /*nread*/len);
//          memory->bcopy(true, vaddr, buf, len);
          return_value = len;
          success = true;
        }
        else if (len == 0) // eof
        {
          return_value = 0;
          success = true;
        }
        else
        {
          return_value = errno;
          warn("error reading %d", errno);
        }

        // free the buffer
        free(buf);
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_WRITE: // 4
      {
        int64_t fd = regs->regs_R[R_A0];
        uint64_t vaddr = regs->regs_R[R_A1];
        int64_t count = regs->regs_R[R_A2];
        int64_t len;
        char *buf;

        // allocate a buffer
        buf = (char *)calloc(count, sizeof(char));
        if (!buf) fatal("ArchLib:  syscall read ran out of memory");

        // copy the buffer from the simulator
 	mem_bcopy(mem_fn, mem, Read,
		  /*buf*/vaddr, buf, /*nread*/count);
//        memory->bcopy(false, vaddr, buf, count);

        // write the buffer out to the fd
        len = write(fd, buf, count);
        fsync(fd);

        // set the result
        if (len >= 0)
        {
          return_value = len;
          success = true;
        }
        else
          return_value = errno;

        // free the buffer
        free(buf);
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_CLOSE: // 6
      {
        int res;

        // close file descriptors > 2 (sim uses 0, 1, and 2)
        if (regs->regs_R[R_A0] > 2)
        {
          res = close(regs->regs_R[R_A0]);
          if (res != (int64_t)-1)
          {
            return_value = 0;
            success = true;
          }
          else
            return_value = errno;
        }
        else
        {
          return_value = 0;
          success = true;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_UNLINK: // 10
      {
        char pathname[SYSCALL_BUFFER_SIZE];
        uint64_t pathname_ptr = regs->regs_R[R_A0];
        int64_t result;

        // read pathname out of virtual memory
	mem_strcpy(mem_fn, mem, Read, /*fname*/pathname_ptr, pathname);
//        memory->strcpy(false, pathname_ptr, pathname);

        result = unlink(pathname);
        if (result == -1)
          return_value = errno;
        else
        {
          return_value = 0;
          success = true;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_BRK: // 17
      {

	sqword_t delta;
	md_addr_t addr;

	delta = regs->regs_R[MD_REG_A0];
//	addr = ld_brk_point + delta;
	if (!delta)
		addr = _system.brk_point;
	else 
		addr = delta;

	if (verbose)
	  myfprintf(stderr, "SYS_sbrk: delta: 0x%012p (%ld)\n", delta, delta);

	ld_brk_point = addr;
	_system.brk_point = addr;
	regs->regs_R[MD_REG_V0] = ld_brk_point;
	regs->regs_R[MD_REG_A3] = 0;

	if (verbose)
	  myfprintf(stderr, "ld_brk_point: 0x%012p\n", ld_brk_point);

	return_value = addr;
	success = true;


#if 0
        md_addr_t addr;

        /* round the new heap pointer to the its page boundary */
#if 0
        addr = ROUND_UP(/*base*/regs->regs_R[MD_REG_A0], MD_PAGE_SIZE);
#endif
        addr = /*base*/regs->regs_R[MD_REG_A0];

	if (verbose)
	  myfprintf(stderr, "SYS_obreak: addr: 0x%012p\n", addr);

	ld_brk_point = addr;
	regs->regs_R[MD_REG_V0] = ld_brk_point;
	regs->regs_R[MD_REG_A3] = 0;

	if (verbose)
	  myfprintf(stderr, "ld_brk_point: 0x%012p\n", ld_brk_point);
*/
#endif


/*	if (regs->regs_R[R_A0] == 0)
        {
          return_value = _system.brk_point;
          ld_brk_point = return_value;
	  success = true;
        }
        else
        {
          //ld_brk_point
          _system.brk_point = regs->regs_R[R_A0];
          return_value = _system.brk_point;
          ld_brk_point = return_value;
          success = true;
        }*/
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_LSEEK: // 19
      {
        int64_t fd = regs->regs_R[R_A0];
        uint64_t offset = regs->regs_R[R_A1];
        int64_t whence = regs->regs_R[R_A2];
        off_t result;

        result = lseek(fd, offset, whence);

        if (result == (off_t)-1)
          return_value = errno;
        else
        {
          return_value = result;
          success = true;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_GETXPID: // 20
      {
        // get our pid and write it to the result register
        // this function is always successful
        return_value = getpid();
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_GETXUID: // 24
      {
        // uid is returned, euid goes in r20 (R_A4)
        regs->regs_R[R_A4] = (uint64_t)geteuid();
        return_value = (uint64_t)getuid();
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_KILL: // 37
      {
        // this means something has gone wrong, almost always
        warn("syscall_kill pid %i\n", regs[R_A0]);
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_OPEN: // 45
      {
        char pathname[SYSCALL_BUFFER_SIZE];
        uint64_t pathname_ptr = regs->regs_R[R_A0];
        uint64_t sim_flags = regs->regs_R[R_A1];
        uint64_t mode = regs->regs_R[R_A2];
        int local_flags;
        int fd;
        
        // read pathname out of virtual memory
	mem_strcpy(mem_fn, mem, Read, /*fname*/pathname_ptr, pathname);
//        memory->strcpy(false, pathname_ptr, pathname);

        // decode sim open flags to local open flags
        local_flags = 0;
        if (sim_flags & ALPHA_O_RDONLY)   local_flags |= O_RDONLY;
        if (sim_flags & ALPHA_O_WRONLY)   local_flags |= O_WRONLY;
        if (sim_flags & ALPHA_O_RDWR)     local_flags |= O_RDWR;
        if (sim_flags & ALPHA_O_NONBLOCK) local_flags |= O_NONBLOCK;
        if (sim_flags & ALPHA_O_APPEND)   local_flags |= O_APPEND;
        if (sim_flags & ALPHA_O_CREAT)    local_flags |= O_CREAT;
        if (sim_flags & ALPHA_O_TRUNC)    local_flags |= O_TRUNC;
        if (sim_flags & ALPHA_O_EXCL)     local_flags |= O_EXCL;
        if (sim_flags & ALPHA_O_NOCTTY)   local_flags |= O_NOCTTY;
        if (sim_flags & ALPHA_O_SYNC)     local_flags |= O_SYNC;
        if (sim_flags & ALPHA_O_DSYNC)    local_flags |= O_DSYNC;
        if (sim_flags & ALPHA_O_RSYNC)    local_flags |= O_RSYNC;

        // open the file
        fd = open(pathname, local_flags, mode);

        if (fd == -1) // return error condition
          return_value = errno;
        else // return file descriptor
        {
          return_value = fd;
          success = true;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_GETXGID: // 47
      {
        // gid is returned, egid goes in r20 (R_A4)
        regs->regs_R[R_A4] = (uint64_t)getegid();
        return_value = (uint64_t)getgid();
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_OSF_SIGPROCMASK: // 48
      {
        // ignore this syscall
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_IOCTL: // 54
      {
        // this is normally a program trying to determine if stdout is a tty
        // if so, tell the program that it's not a tty, so the program does
        // block buffering
        int64_t fd = regs->regs_R[R_A0];
        int64_t request = regs->regs_R[R_A1];

        if (fd < 0)
          return_value = EBADF;
        else
        {
          switch (request)
          {
            case 0x40067408: //ALPHA_IOCTL_TIOCGETP:
            case 0x80067409: //ALPHA_IOCTL_TIOCSETP:
            case 0x8006740a: //ALPHA_IOCTL_TIOCSETN:
            case 0x80067411: //ALPHA_IOCTL_TIOCSETC:
            case 0x40067412: //ALPHA_IOCTL_TIOCGETC:
            case 0x2000745e: //ALPHA_IOCTL_TIOCISATTY:
            case 0x402c7413: //ALPHA_IOCTL_TIOCGETS:
            case 0x40127417: //ALPHA_IOCTL_TIOCGETA:
              return_value = ENOTTY;
              break;

            default:
              fatal("unsupported ioctl call: %x on fd: %d pc: 0x%llx",
                  request, fd, 0);
          }
        }
      }
      break;
      /////////////////////////////////////////////////////////////////////////
    case SYSCALL_MMAP: // 71
      {
        uint64_t addr = regs->regs_R[R_A0];
        uint64_t length = regs->regs_R[R_A1];
        int64_t flags = regs->regs_R[R_A3];
        int64_t fd = regs->regs_R[R_A4];

        // verify addr & length are both page aligned
        if (((addr % PAGE_SIZE) != 0) ||
            ((length % PAGE_SIZE) != 0))
          return_value = EINVAL;
        else
        {
//          warn("mmap ignorning suggested map address");
          if (addr != 0)
            warn("mmap ignorning suggested map address");

          addr = _system.mmap_end;
          _system.mmap_end += length;

          if (!(flags & 0x10))
            warn("mmapping fd %d.  this is bad if not /dev/zero", fd); 
          return_value = addr;
          success = true;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_MUNMAP: // 73
      {
        // since we barely support mmap, we just pretend like whatever they
        // want to munmap is fine
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_MPROTECT: // 74
      {
        // since all threads are isolated, this is never an issue
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_FCNTL: // 92
      {
        int64_t fd = regs->regs_R[R_A0];
        int64_t cmd = regs->regs_R[R_A1];

        if (fd < 0)
          return_value = EBADF;
        else
        {
          switch (cmd)
          {
            case 0: // F_DUPFD
              return_value = EMFILE;
            case 1: // F_GETFD (get close-on-exec flag)
            case 2: // F_SETFD (set close-on-exec flag)
              return_value = 0;
              success = true;
              break;

            case 3: // F_GETFL (get file flags)
            case 4: // F_SETFL (set file flags)
              return_value = fcntl(fd, cmd);
              if (return_value != -1)
                success = true;
              else
                return_value = errno;
              break;

            case 7: // F_GETLK (get lock)
            case 8: // F_SETLK (set lock)
            case 9: // F_SETLKW (set lock and wait)
            default:
              // pretend that these all worked
              warn("ignored fcntl command %d on fd %d", cmd, fd);
              return_value = 0;
              success = true;
              break;
          }
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
/*     case SYSCALL_SOCKET: // 97
      {
        // 
        return_value = 0;
        success = true;
      }
      break;
     /////////////////////////////////////////////////////////////////////////
     case SYSCALL_CONNECT: // 98
      {
        // 
        return_value = 0;
        success = true;
      }
      break;
     /////////////////////////////////////////////////////////////////////////
     case SYSCALL_SEND: // 101
      {
        // 
        return_value = 0;
        success = true;
      }
      break;
      /////////////////////////////////////////////////////////////////////////
     case SYSCALL_RECV: // 102
      {
        // 
        return_value = 0;
        success = true;
      }
      break;*/
    /////////////////////////////////////////////////////////////////////////
     case SYSCALL_WRITEV: // 121
      {
        int64_t fd = regs->regs_R[R_A0];
        uint64_t iovptr = regs->regs_R[R_A1];
        int64_t iovcnt = regs->regs_R[R_A2];
        struct iovec *iov;
        alpha_iovec a_iov;
        int i, res;

        if (fd < 0)
          return_value = EBADF;
        else
        {
	  iov = malloc(/* len */ sizeof(struct iovec)*iovcnt);
          //iov = new struct iovec[iovcnt];
          // for each io vector entry
          for (i = 0; i < iovcnt; i++)
          {
            // read a_iov out of simulator memory
	    mem_bcopy(mem_fn, mem, Read,
		  /*buf*/iovptr + i*sizeof(a_iov), &a_iov, /*nread*/sizeof(a_iov));
/*            memory->bcopy(false, 
                iovptr + i*sizeof(a_iov), 
                &a_iov, 
                sizeof(a_iov));
*/
            // copy into local iov
            iov[i].iov_len = a_iov.iov_len;
            // allocate local buffer
	    iov[i].iov_base = malloc(/* len */ sizeof(char)*(iov[i].iov_len));
            //iov[i].iov_base = new char [iov[i].iov_len];
            // copy into local buffer
 	    mem_bcopy(mem_fn, mem, Read,
		  /*buf*/a_iov.iov_base, iov[i].iov_base, /*nread*/a_iov.iov_len);
/*            memory->bcopy(false, 
                a_iov.iov_base, 
                iov[i].iov_base, 
                a_iov.iov_len);
*/          }

          // perform writev
          res = writev(fd, iov, iovcnt);

          if (res < 0)
            return_value = errno;
          else
          {
            return_value = 0;
            success = true;
          }

          // cleanup
          for (i = 0; i < iovcnt; i++)
	    free(iov[i].iov_base);
            //delete[] (char*) iov[i].iov_base;
          //delete[] iov;
	  free(iov);
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_FTRUNCATE: // 130
      {
        int fd = regs->regs_R[R_A0];
        off_t length = regs->regs_R[R_A1];

        if (fd < 0)
          return_value = EBADF;
        else
        {
          return_value = ftruncate(fd, length);
          if (return_value == -1)
            return_value = errno;
          else
            success = true;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_GETRLIMIT: // 144
      {
        uint64_t resource = regs->regs_R[R_A0];
        uint64_t vaddr = regs->regs_R[R_A1];
        alpha_rlimit arlm;

        switch (resource)
        {
          case 3: //ALPHA_RLIMIT_STACK:
            arlm.rlim_cur = 16 * 1024 * 1024; // 16MB, arbitrary
            arlm.rlim_max = arlm.rlim_cur;
            return_value = 0;
            success = true;
            break;

          case 2: //ALPHA_RLIMIT_DATA:
            arlm.rlim_cur = 1024 * 1024 * 1024; // 1GB, arbitrary
            arlm.rlim_max = arlm.rlim_cur;
            return_value = 0;
            success = true;
            break;

          default:
            warn("unimplemented rlimit resource %u ... failing ...", resource);
            break;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_SETRLIMIT: // 145
      {
        // ignore this syscall
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_OSF_GETSYSINFO: // 256
      {
        uint64_t op = regs->regs_R[R_A0];
        uint64_t vaddr;

        switch(op)
        {
          case 45:
            // This is a bad idea and was just copied from the m5 sim.
            //regs->regs_R[R_FPCR] = 0;
            vaddr = regs->regs_R[R_A1];

 	    mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, &regs->regs_R[R_FPCR], /*nread*/sizeof(uint64_t));
//            memory->bcopy(true, vaddr, &regs->regs_R[R_FPCR], sizeof(uint64_t));
            return_value = 0;
            success = true;
            break;

          default:
            fatal("unsupported operation %u on system call OSF_GETSYSINFO",
                op);
            break;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_OSF_SETSYSINFO: // 257
      {
        uint64_t op = regs->regs_R[R_A0];
        uint64_t vaddr;

        switch(op)
        {
          case 14:
            // This is a bad idea and was just copied from the m5 sim.
            vaddr = regs->regs_R[R_A1];

 	    mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, &(regs->regs_R[R_FPCR]), /*nread*/sizeof(uint64_t));
//            memory->bcopy(false, vaddr, &regs->regs_R[R_FPCR], sizeof(uint64_t));
            return_value = 0;
            success = true;
            break;

          default:
            fatal("unsupported operation %u on system call OSF_GETSYSINFO",
                op);
            break;
        }
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_TIMES: // 323
      {
        // ignoring
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_UNAME: // 339
      {
        alpha_utsname buf;
        uint64_t vaddr;

        strcpy(buf.sysname,  "Linux");             // modeling linux OS
        strcpy(buf.nodename, "mjdechen");          // mark's laptop
        strcpy(buf.release,  "2.6.27.5-117.fc10"); // fedora 10, why not ...
        strcpy(buf.version,  "Mon Dec  8 21:18:29 PST 2008"); // why not
        strcpy(buf.machine,  "alpha");             // modeling alpha isa
        strcpy(buf.domainname, "");                // my laptop had none

        // get buffer address from first arg reg
        vaddr = regs->regs_R[R_A0];

        // copy the buffer into virtual memory
 	mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, &buf, /*nread*/sizeof(buf));
//        memory->bcopy(true, vaddr, &buf, sizeof(buf));
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_MREMAP: // 341
      {
        unsigned long int addr = regs->regs_R[R_A0];
        uint64_t old_length = regs->regs_R[R_A1];
        uint64_t new_length = regs->regs_R[R_A2];
        uint64_t flags = regs->regs_R[R_A3];

	if (new_length > old_length) {
		if ((addr + old_length) == _system.mmap_end) {
			unsigned long int diff = new_length - old_length;
			_system.mmap_end += diff;
			return_value = addr;
			success = true;
			return;
		} else {
//			if (!(flags & 1)) {
        		char *buf;
        		buf = (char *)calloc(old_length, sizeof(char));
		 	mem_bcopy(mem_fn, mem, Read,
		  		/*buf*/addr, buf, /*nread*/old_length);
		 	mem_bcopy(mem_fn, mem, Write,
		  		/*buf*/_system.mmap_end, buf, /*nread*/old_length);
			return_value = _system.mmap_end;
			_system.mmap_end += new_length;			
			//warn("can't remap here and MREMAP_MAYMOVE flag not set\n");
			success = true;
        		free(buf);
			return;
	
//			} else {
		}				
	} else {
		return_value = addr;
		success = true;
		return;
	}	
	
         // this syscall tries to expand / shrink an existing memory mapping,
        // yet we don't really have a proper memory mapping table.  so, we
        // won't handle it at all all.
        return_value = 0;
        success = false;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_RT_SIGACTION: // 352
      {
        // ignoring
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_GETTIMEOFDAY: // 359
      {
        // this is a little wonky.  if the benchmark really needs the time
        // to do something useful, then the simulated processor will appear
        // to be running slow (~1MHz).  keeping this in mind, if it becomes
        // necessary to have processors really know something useful about
        // time, the simulator should record time at the beginning of the sim
        // and then fudge all future syscalls to gettimeofday, to something
        // like (orig time + (num_insts / 6.4B))

        struct timeval tv;
        struct timezone tz;
        uint64_t tvaddr = regs->regs_R[R_A0];
        uint64_t tzaddr = regs->regs_R[R_A1];
        alpha_timeval atv;
        alpha_timezone atz;
        int64_t result;

        if (tzaddr)
          result = gettimeofday(&tv, &tz);
        else
          result = gettimeofday(&tv, 0);

        if (result == 0)
        {
          atv.tv_sec = tv.tv_sec;
          atv.tv_usec = tv.tv_usec;
 	  mem_bcopy(mem_fn, mem, Write,
		  /*buf*/tvaddr, &atv, /*nread*/sizeof(atv));
//          memory->bcopy(true, tvaddr, &atv, sizeof(atv));
          
          if (tzaddr)
          {
            atz.tz_minuteswest = tz.tz_minuteswest;
            atz.tz_dsttime = tz.tz_dsttime;
 	    mem_bcopy(mem_fn, mem, Write,
		  /*buf*/tzaddr, &atz, /*nread*/sizeof(atz));
//            memory->bcopy(true, tzaddr, &atz, sizeof(atz));
          }

          return_value = 0;
          success = true;
        }
        else
          return_value = errno;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_GETCWD: // 367
      {
        char *buf;
        uint64_t vaddr = regs->regs_R[R_A0];
        uint64_t size = regs->regs_R[R_A1];
        char *res;

        // allocate a buffer
        buf = (char *)calloc(size, sizeof(char));

        res = getcwd(buf, size);

        if (res < 0)
          return_value = errno;
        else
        {
          return_value = strlen(res);
          success = true;
	  mem_strcpy(mem_fn, mem, Write, /*fname*/vaddr, buf);
//          memory->strcpy(true, vaddr, buf);
        }

        // clean up after ourselves
        free(buf);
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_EXIT_GROUP: // 405
      {
        // set program complete flag
        program_complete = true;
        return_value = 0;
        success = true;
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_STAT64: // 425
      {
//printf("SYSCALL_STAT64\n");
#if 1 
        char pathname[SYSCALL_BUFFER_SIZE];
        uint64_t pathname_ptr = regs->regs_R[R_A0];
        uint64_t vaddr = regs->regs_R[R_A1];
        //struct stat64 buf;
//        alpha_stat64 buf;
        alpha_stat64 alpha_buf;
        int result;

        // read path out of simulator memory
	mem_strcpy(mem_fn, mem, Read, /*fname*/pathname_ptr, pathname);
//        memory->strcpy(false, pathname_ptr, pathname);

        // perform stat
        result = stat64(pathname, &alpha_buf);
//	printf("\ndoing a stat64 on \"%s\"\n", pathname);

        if (result < 0)
          return_value = errno;
        else
        {
//          translate_stat64_buf(&alpha_buf, &buf);
 	  mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, &alpha_buf, /*nread*/sizeof(alpha_buf));
//          memory->bcopy(true, vaddr, &alpha_buf, sizeof(alpha_buf));
          return_value = 0;
          success = true;
	  printf("\nNO ERROR\n");
        }
#endif
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_LSTAT64: // 426
      {
//printf("SYSCALL_LSTAT64\n");
#if 1
        char pathname[SYSCALL_BUFFER_SIZE];
        uint64_t pathname_ptr = regs->regs_R[R_A0];
        uint64_t vaddr = regs->regs_R[R_A1];
//        alpha_stat64 buf;
        //struct stat64 buf;
        alpha_stat64 alpha_buf;
        int result;

        // read path out of simulator memory
	mem_strcpy(mem_fn, mem, Read, /*fname*/pathname_ptr, pathname);
//        memory->strcpy(false, pathname_ptr, pathname);

        // perform lstat
        result = lstat64(pathname, &alpha_buf);
//	printf("\ndoing a lstat64 on \"%s\"i\n", pathname);

        if (result < 0)
          return_value = errno;
        else
        {
//          translate_stat64_buf(&alpha_buf, &buf);
 	  mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, &alpha_buf, /*nread*/sizeof(alpha_buf));
//          memory->bcopy(true, vaddr, &alpha_buf, sizeof(alpha_buf));
          return_value = 0;
          success = true;
        }
#endif
      }
      break;
    /////////////////////////////////////////////////////////////////////////
    case SYSCALL_FSTAT64: // 427
      {

#if 1
//printf("SYSCALL_FSTAT64\n");
        int fd = regs->regs_R[R_A0];
        uint64_t vaddr = regs->regs_R[R_A1];
        //struct stat64 buf;
//        alpha_stat64 buf;
        alpha_stat64 alpha_buf;
        int result;
        
        if (fd < 0)
          regs->regs_R[R_V0] = EBADF;
        else
        {
          result = fstat64(fd, &alpha_buf);

          if (result < 0)
            return_value = errno;
          else
          {
//            translate_stat64_buf(&alpha_buf, &buf);
 	    mem_bcopy(mem_fn, mem, Write,
		  /*buf*/vaddr, &alpha_buf, /*nread*/sizeof(alpha_buf));
//            memory->bcopy(true, vaddr, &alpha_buf, sizeof(alpha_buf));
            return_value = 0;
            success = true;
          }
        }
#endif

      }
      break;
    /////////////////////////////////////////////////////////////////////////
    default: // panic
      {
//        if (knobs::debug_syscalls)
//          warn("unimplemented syscall code: %d", syscall_code);
        // success defaults to false
        // return value defaults to 0
      }
      break;
    /////////////////////////////////////////////////////////////////////////
  }
  ///////////////////////////////////////////////////////////////////////////
 
  // write result and value into register file 
  if (success)
    regs->regs_R[R_A3] = 0;
  else
    regs->regs_R[R_A3] = -1;
  regs->regs_R[R_V0] = return_value;

  //printf(" %d\n", return_value);



#if 0
  switch (syscode)
    {
    case OSF_SYS_exit:
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf,
	      /* exitcode + fudge */(regs->regs_R[MD_REG_A0] & 0xff) + 1);
      break;

    case OSF_SYS_read:
      {
	char *buf;

	/* allocate same-sized input buffer in host memory */
	if (!(buf =
	      (char *)calloc(/*nbytes*/regs->regs_R[MD_REG_A2], sizeof(char))))
	  fatal("out of memory in SYS_read");

	/* read data from file */
	do {
	  /*nread*/regs->regs_R[MD_REG_V0] =
	    read(/*fd*/regs->regs_R[MD_REG_A0], buf,
	         /*nbytes*/regs->regs_R[MD_REG_A2]);
	} while (/*nread*/regs->regs_R[MD_REG_V0] == -1
	         && errno == EAGAIN);

	/* check for error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy results back into host memory */
	mem_bcopy(mem_fn, mem, Write,
		  /*buf*/regs->regs_R[MD_REG_A1], buf, /*nread*/regs->regs_R[MD_REG_A2]);

	/* done with input buffer */
	free(buf);
      }
      break;

    case OSF_SYS_write:
      {
	char *buf;

	/* allocate same-sized output buffer in host memory */
	if (!(buf =
	      (char *)calloc(/*nbytes*/regs->regs_R[MD_REG_A2], sizeof(char))))
	  fatal("out of memory in SYS_write");

	/* copy inputs into host memory */
	mem_bcopy(mem_fn, mem, Read, /*buf*/regs->regs_R[MD_REG_A1], buf,
		  /*nbytes*/regs->regs_R[MD_REG_A2]);

	/* write data to file */
	if (sim_progfd && MD_OUTPUT_SYSCALL(regs))
	  {
	    /* redirect program output to file */

	    /*nwritten*/regs->regs_R[MD_REG_V0] =
	      fwrite(buf, 1, /*nbytes*/regs->regs_R[MD_REG_A2], sim_progfd);
	  }
	else
	  {
	    /* perform program output request */

	    do {
	      /*nwritten*/regs->regs_R[MD_REG_V0] =
	        write(/*fd*/regs->regs_R[MD_REG_A0],
		      buf, /*nbytes*/regs->regs_R[MD_REG_A2]);
	    } while (/*nwritten*/regs->regs_R[MD_REG_V0] == -1
		     && errno == EAGAIN);
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] == regs->regs_R[MD_REG_A2])
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* done with output buffer */
	free(buf);
      }
      break;

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_getdomainname:
      /* get program scheduling priority */
      {
	char *buf;

	buf = malloc(/* len */(size_t)regs->regs_R[MD_REG_A1]);
	if (!buf)
	  fatal("out of virtual memory in gethostname()");

	/* result */regs->regs_R[MD_REG_V0] =
	  getdomainname(/* name */buf,
		      /* len */(size_t)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy string back to simulated memory */
	mem_bcopy(mem_fn, mem, Write,
		  /* name */regs->regs_R[MD_REG_A0],
		  buf, /* len */regs->regs_R[MD_REG_A1]);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_flock:
      /* get flock() information on the file */
      {
	regs->regs_R[MD_REG_V0] =
	  flock(/*fd*/(int)regs->regs_R[MD_REG_A0],
		/*cmd*/(int)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_bind:
      {
	const struct sockaddr a_sock;

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  &a_sock, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

      regs->regs_R[MD_REG_V0] =
	bind((int) regs->regs_R[MD_REG_A0],
	     &a_sock,(int) regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_sendto:
      {
	char *buf = NULL;
	struct sockaddr d_sock;
	int buf_len = 0;

	buf_len = regs->regs_R[MD_REG_A2];

	if (buf_len > 0)
	  buf = (char *) malloc(buf_len*sizeof(char));

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  buf, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	if (regs->regs_R[MD_REG_A5] > 0) 
	  mem_bcopy(mem_fn, mem, Read, regs->regs_R[MD_REG_A4],
		    &d_sock, (int)regs->regs_R[MD_REG_A5]);

	regs->regs_R[MD_REG_V0] =
	  sendto((int) regs->regs_R[MD_REG_A0],
		 buf,(int) regs->regs_R[MD_REG_A2],
		 (int) regs->regs_R[MD_REG_A3],
		 &d_sock,(int) regs->regs_R[MD_REG_A5]);

	mem_bcopy(mem_fn, mem, Write, /* serv_addr */regs->regs_R[MD_REG_A1],
		  buf, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	/* maybe copy back whole size of sockaddr */
	if (regs->regs_R[MD_REG_A5] > 0)
	  mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A4],
		    &d_sock, (int)regs->regs_R[MD_REG_A5]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	if (buf != NULL) 
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
      /* ADDED BY CALDER 10/27/99 */
    case OSF_SYS_old_recvfrom:
    case OSF_SYS_recvfrom:
      {
	int addr_len;
	char *buf;
	struct sockaddr *a_sock;
      
	buf = (char *) malloc(sizeof(char)*regs->regs_R[MD_REG_A2]);

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  buf, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A5],
		  &addr_len, sizeof(int));

	a_sock = (struct sockaddr *)malloc(addr_len);

	mem_bcopy(mem_fn, mem, Read, regs->regs_R[MD_REG_A4],
		  a_sock, addr_len);

	regs->regs_R[MD_REG_V0] =
	  recvfrom((int) regs->regs_R[MD_REG_A0],
		   buf,(int) regs->regs_R[MD_REG_A2],
		   (int) regs->regs_R[MD_REG_A3], a_sock,&addr_len);

	mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A1],
		  buf, (int) regs->regs_R[MD_REG_V0]);

	mem_bcopy(mem_fn, mem, Write, /* serv_addr */regs->regs_R[MD_REG_A5],
		  &addr_len, sizeof(int));

	mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A4],
		  a_sock, addr_len);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

    case OSF_SYS_open:
      {
	char buf[MAXBUFSIZE];
	unsigned int i;
	int osf_flags = regs->regs_R[MD_REG_A1], local_flags = 0;

	/* translate open(2) flags */
	for (i=0; i < OSF_NFLAGS; i++)
	  {
	    if (osf_flags & osf_flag_table[i].osf_flag)
	      {
		osf_flags &= ~osf_flag_table[i].osf_flag;
		local_flags |= osf_flag_table[i].local_flag;
	      }
	  }
	/* any target flags left? */
	if (osf_flags != 0)
	  fatal("syscall: open: cannot decode flags: 0x%08x", osf_flags);

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* open the file */
#ifdef __CYGWIN32__
	/*fd*/regs->regs_R[MD_REG_V0] =
	  open(buf, local_flags|O_BINARY, /*mode*/regs->regs_R[MD_REG_A2]);
#else /* !__CYGWIN32__ */
	/*fd*/regs->regs_R[MD_REG_V0] =
	  open(buf, local_flags, /*mode*/regs->regs_R[MD_REG_A2]);
#endif /* __CYGWIN32__ */
	
	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_close:
      /* don't close stdin, stdout, or stderr as this messes up sim logs */
      if (/*fd*/regs->regs_R[MD_REG_A0] == 0
	  || /*fd*/regs->regs_R[MD_REG_A0] == 1
	  || /*fd*/regs->regs_R[MD_REG_A0] == 2)
	{
	  regs->regs_R[MD_REG_A3] = 0;
	  break;
	}

      /* close the file */
      regs->regs_R[MD_REG_V0] = close(/*fd*/regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

#if 0
    case OSF_SYS_creat:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* create the file */
	/*fd*/regs->regs_R[MD_REG_V0] =
	  creat(buf, /*mode*/regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

    case OSF_SYS_unlink:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* delete the file */
	/*result*/regs->regs_R[MD_REG_V0] = unlink(buf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_chdir:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* change the working directory */
	/*result*/regs->regs_R[MD_REG_V0] = chdir(buf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_chmod:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* chmod the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  chmod(buf, /*mode*/regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_chown:
#ifdef _MSC_VER
      warn("syscall chown() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem,Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* chown the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  chown(buf, /*owner*/regs->regs_R[MD_REG_A1],
		/*group*/regs->regs_R[MD_REG_A2]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
#endif /* _MSC_VER */
      break;

    case OSF_SYS_sbrk:
      {
	sqword_t delta;
	md_addr_t addr;

	delta = regs->regs_R[MD_REG_A0];
	addr = ld_brk_point + delta;

	if (verbose)
	  myfprintf(stderr, "SYS_sbrk: delta: 0x%012p (%ld)\n", delta, delta);

	ld_brk_point = addr;
	regs->regs_R[MD_REG_V0] = ld_brk_point;
	regs->regs_R[MD_REG_A3] = 0;

	if (verbose)
	  myfprintf(stderr, "ld_brk_point: 0x%012p\n", ld_brk_point);

#if 0
	/* check whether heap area has merged with stack area */
	if (/* addr >= ld_brk_point && */ addr < regs->regs_R[MD_REG_SP])
	  {
	    regs->regs_R[MD_REG_A3] = 0;
	    ld_brk_point = addr;
	  }
	else
	  {
	    /* out of address space, indicate error */
	    regs->regs_R[MD_REG_A3] = -1;
	  }
#endif
      }
      break;

    case OSF_SYS_obreak:
      {
        md_addr_t addr;

        /* round the new heap pointer to the its page boundary */
#if 0
        addr = ROUND_UP(/*base*/regs->regs_R[MD_REG_A0], MD_PAGE_SIZE);
#endif
        addr = /*base*/regs->regs_R[MD_REG_A0];

	if (verbose)
	  myfprintf(stderr, "SYS_obreak: addr: 0x%012p\n", addr);

	ld_brk_point = addr;
	regs->regs_R[MD_REG_V0] = ld_brk_point;
	regs->regs_R[MD_REG_A3] = 0;

	if (verbose)
	  myfprintf(stderr, "ld_brk_point: 0x%012p\n", ld_brk_point);
      break;

    case OSF_SYS_lseek:
      /* seek into file */
      regs->regs_R[MD_REG_V0] =
	lseek(/*fd*/regs->regs_R[MD_REG_A0],
	      /*off*/regs->regs_R[MD_REG_A1], /*dir*/regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_getpid:
      /* get the simulator process id */
      /*result*/regs->regs_R[MD_REG_V0] = getpid();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_getuid:
#ifdef _MSC_VER
      warn("syscall getuid() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      /* get current user id */
      /*first result*/regs->regs_R[MD_REG_V0] = getuid();
      /*second result*/regs->regs_R[MD_REG_A4] = geteuid();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#endif /* _MSC_VER */
      break;

    case OSF_SYS_access:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[MD_REG_A0], buf);

	/* check access on the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  access(buf, /*mode*/regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_stat:
    case OSF_SYS_lstat:
      {
	char buf[MAXBUFSIZE];
	struct osf_statbuf osf_sbuf;
#ifdef _MSC_VER
	struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[MD_REG_A0], buf);

	/* stat() the file */
	if (syscode == OSF_SYS_stat)
	  /*result*/regs->regs_R[MD_REG_V0] = stat(buf, &sbuf);
	else /* syscode == OSF_SYS_lstat */
	  {
#ifdef _MSC_VER
            warn("syscall lstat() not yet implemented for MSC...");
            regs->regs_R[MD_REG_A3] = 0;
            break;
#else /* !_MSC_VER */
	    /*result*/regs->regs_R[MD_REG_V0] = lstat(buf, &sbuf);
#endif /* _MSC_VER */
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* translate from host stat structure to target format */
	osf_sbuf.osf_st_dev = MD_SWAPW(sbuf.st_dev);
	osf_sbuf.osf_st_ino = MD_SWAPW(sbuf.st_ino);
	osf_sbuf.osf_st_mode = MD_SWAPW(sbuf.st_mode);
	osf_sbuf.osf_st_nlink = MD_SWAPH(sbuf.st_nlink);
	osf_sbuf.osf_st_uid = MD_SWAPW(sbuf.st_uid);
	osf_sbuf.osf_st_gid = MD_SWAPW(sbuf.st_gid);
	osf_sbuf.osf_st_rdev = MD_SWAPW(sbuf.st_rdev);
	osf_sbuf.osf_st_size = MD_SWAPQ(sbuf.st_size);
	osf_sbuf.osf_st_atime = MD_SWAPW(sbuf.st_atime);
	osf_sbuf.osf_st_mtime = MD_SWAPW(sbuf.st_mtime);
	osf_sbuf.osf_st_ctime = MD_SWAPW(sbuf.st_ctime);
#ifndef _MSC_VER
	osf_sbuf.osf_st_blksize = MD_SWAPW(sbuf.st_blksize);
	osf_sbuf.osf_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */

	/* copy stat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
		  &osf_sbuf, sizeof(struct osf_statbuf));
      }
      break;

    case OSF_SYS_dup:
      /* dup() the file descriptor */
      /*fd*/regs->regs_R[MD_REG_V0] = dup(/*fd*/regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

#if 0
    case OSF_SYS_pipe:
      {
	int fd[2];

	/* copy pipe descriptors to host memory */;
	mem_bcopy(mem_fn, mem, Read, /*fd's*/regs->regs_R[MD_REG_A0],
		  fd, sizeof(fd));

	/* create a pipe */
	/*result*/regs->regs_R[7] = pipe(fd);

	/* copy descriptor results to result registers */
	/*pipe1*/regs->regs_R[MD_REG_V0] = fd[0];
	/*pipe 2*/regs->regs_R[3] = fd[1];

	/* check for an error condition */
	if (regs->regs_R[7] == (qword_t)-1)
	  {
	    regs->regs_R[MD_REG_V0] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;
#endif

    case OSF_SYS_getgid:
#ifdef _MSC_VER
      warn("syscall getgid() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      /* get current group id */
      /*first result*/regs->regs_R[MD_REG_V0] = getgid();
      /*second result*/regs->regs_R[MD_REG_A4] = getegid();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#endif /* _MSC_VER */
      break;

    case OSF_SYS_ioctl:
      switch (/* req */regs->regs_R[MD_REG_A1])
	{
#if !defined(TIOCGETP) && defined(linux)
	case OSF_TIOCGETP: /* <Out,TIOCGETP,6> */
	  {
	    struct termios lbuf;
	    struct osf_sgttyb buf;

	    /* result */regs->regs_R[MD_REG_V0] =
			  tcgetattr(/* fd */(int)regs->regs_R[MD_REG_A0],
				    &lbuf);

	    /* translate results */
	    buf.sg_ispeed = lbuf.c_ispeed;
	    buf.sg_ospeed = lbuf.c_ospeed;
	    buf.sg_erase = lbuf.c_cc[VERASE];
	    buf.sg_kill = lbuf.c_cc[VKILL];
	    buf.sg_flags = 0;	/* FIXME: this is wrong... */

	    mem_bcopy(mem_fn, mem, Write,
		      /* buf */regs->regs_R[MD_REG_A2], &buf,
		      sizeof(struct osf_sgttyb));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* probably not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
#ifdef TIOCGETP
	case OSF_TIOCGETP: /* <Out,TIOCGETP,6> */
	  {
	    struct sgttyb lbuf;
	    struct osf_sgttyb buf;

	    /* result */regs->regs_R[MD_REG_V0] =
	      ioctl(/* fd */(int)regs->regs_R[MD_REG_A0],
		    /* req */TIOCGETP,
		    &lbuf);

	    /* translate results */
	    buf.sg_ispeed = lbuf.sg_ispeed;
	    buf.sg_ospeed = lbuf.sg_ospeed;
	    buf.sg_erase = lbuf.sg_erase;
	    buf.sg_kill = lbuf.sg_kill;
	    buf.sg_flags = lbuf.sg_flags;
	    mem_bcopy(mem_fn, mem, Write,
		      /* buf */regs->regs_R[MD_REG_A2], &buf,
		      sizeof(struct osf_sgttyb));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
#ifdef FIONREAD
	case OSF_FIONREAD:
	  {
	    int nread;

	    /* result */regs->regs_R[MD_REG_V0] =
	      ioctl(/* fd */(int)regs->regs_R[MD_REG_A0],
		    /* req */FIONREAD,
		    /* arg */&nread);

	    mem_bcopy(mem_fn, mem, Write,
		      /* arg */regs->regs_R[MD_REG_A2],
		      &nread, sizeof(nread));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
#ifdef FIONBIO
	case /*FIXME*/FIONBIO:
	  {
	    int arg = 0;

	    if (regs->regs_R[MD_REG_A2])
	      mem_bcopy(mem_fn, mem, Read,
		      /* arg */regs->regs_R[MD_REG_A2],
		      &arg, sizeof(arg));

#ifdef NOTNOW
	    fprintf(stderr, "FIONBIO: %d, %d\n",
		    (int)regs->regs_R[MD_REG_A0],
		    arg);
#endif
	    /* result */regs->regs_R[MD_REG_V0] =
	      ioctl(/* fd */(int)regs->regs_R[MD_REG_A0],
		    /* req */FIONBIO,
		    /* arg */&arg);

	    if (regs->regs_R[MD_REG_A2])
	      mem_bcopy(mem_fn, mem, Write,
		      /* arg */regs->regs_R[MD_REG_A2],
		      &arg, sizeof(arg));

	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[MD_REG_A3] = 0;
	    else /* not a typewriter, return details */
	      {
		regs->regs_R[MD_REG_A3] = -1;
		regs->regs_R[MD_REG_V0] = errno;
	      }
	  }
	  break;
#endif
	default:
	  warn("unsupported ioctl call: ioctl(%ld, ...)",
	       regs->regs_R[MD_REG_A1]);
	  regs->regs_R[MD_REG_A3] = 0;
	  break;
	}
      break;

#if 0
      {
	char buf[NUM_IOCTL_BYTES];
	int local_req = 0;

	/* convert target ioctl() request to host ioctl() request values */
	switch (/*req*/regs->regs_R[MD_REG_A1]) {
/* #if !defined(__CYGWIN32__) */
	case SS_IOCTL_TIOCGETP:
	  local_req = TIOCGETP;
	  break;
	case SS_IOCTL_TIOCSETP:
	  local_req = TIOCSETP;
	  break;
	case SS_IOCTL_TCGETP:
	  local_req = TIOCGETP;
	  break;
/* #endif */
#ifdef TCGETA
	case SS_IOCTL_TCGETA:
	  local_req = TCGETA;
	  break;
#endif
#ifdef TIOCGLTC
	case SS_IOCTL_TIOCGLTC:
	  local_req = TIOCGLTC;
	  break;
#endif
#ifdef TIOCSLTC
	case SS_IOCTL_TIOCSLTC:
	  local_req = TIOCSLTC;
	  break;
#endif
	case SS_IOCTL_TIOCGWINSZ:
	  local_req = TIOCGWINSZ;
	  break;
#ifdef TCSETAW
	case SS_IOCTL_TCSETAW:
	  local_req = TCSETAW;
	  break;
#endif
#ifdef TIOCGETC
	case SS_IOCTL_TIOCGETC:
	  local_req = TIOCGETC;
	  break;
#endif
#ifdef TIOCSETC
	case SS_IOCTL_TIOCSETC:
	  local_req = TIOCSETC;
	  break;
#endif
#ifdef TIOCLBIC
	case SS_IOCTL_TIOCLBIC:
	  local_req = TIOCLBIC;
	  break;
#endif
#ifdef TIOCLBIS
	case SS_IOCTL_TIOCLBIS:
	  local_req = TIOCLBIS;
	  break;
#endif
#ifdef TIOCLGET
	case SS_IOCTL_TIOCLGET:
	  local_req = TIOCLGET;
	  break;
#endif
#ifdef TIOCLSET
	case SS_IOCTL_TIOCLSET:
	  local_req = TIOCLSET;
	  break;
#endif
	}

	if (!local_req)
	  {
	    /* FIXME: could not translate the ioctl() request, just warn user
	       and ignore the request */
	    warn("syscall: ioctl: ioctl code not supported d=%d, req=%d",
		 regs->regs_R[MD_REG_A0], regs->regs_R[MD_REG_A1]);
	    regs->regs_R[MD_REG_V0] = 0;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
	    /* ioctl() code was successfully translated to a host code */

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes to host mem */
	    if (/*argp*/regs->regs_R[MD_REG_A2] != 0)
	      mem_bcopy(mem_fn, mem, Read, /*argp*/regs->regs_R[MD_REG_A2],
			buf, NUM_IOCTL_BYTES);

	    /* perform the ioctl() call */
	    /*result*/regs->regs_R[MD_REG_V0] =
	      ioctl(/*fd*/regs->regs_R[MD_REG_A0], local_req, buf);

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes from host mem */
	    if (/*argp*/regs->regs_R[MD_REG_A2] != 0)
	      mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A2],
			buf, NUM_IOCTL_BYTES);

	    /* check for an error condition */
	    if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	      regs->regs_R[7] = 0;
	    else
	      {	
		/* got an error, return details */
		regs->regs_R[MD_REG_V0] = errno;
		regs->regs_R[7] = 1;
	      }
	  }
      }
      break;
#endif

    case OSF_SYS_fstat:
      {
	struct osf_statbuf osf_sbuf;
#ifdef _MSC_VER
        struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* fstat() the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  fstat(/*fd*/regs->regs_R[MD_REG_A0], &sbuf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* translate the stat structure to host format */
	osf_sbuf.osf_st_dev = MD_SWAPW(sbuf.st_dev);
	osf_sbuf.osf_st_ino = MD_SWAPW(sbuf.st_ino);
	osf_sbuf.osf_st_mode = MD_SWAPW(sbuf.st_mode);
	osf_sbuf.osf_st_nlink = MD_SWAPH(sbuf.st_nlink);
	osf_sbuf.osf_st_uid = MD_SWAPW(sbuf.st_uid);
	osf_sbuf.osf_st_gid = MD_SWAPW(sbuf.st_gid);
	osf_sbuf.osf_st_rdev = MD_SWAPW(sbuf.st_rdev);
	osf_sbuf.osf_st_size = MD_SWAPQ(sbuf.st_size);
	osf_sbuf.osf_st_atime = MD_SWAPW(sbuf.st_atime);
	osf_sbuf.osf_st_mtime = MD_SWAPW(sbuf.st_mtime);
	osf_sbuf.osf_st_ctime = MD_SWAPW(sbuf.st_ctime);
#ifndef _MSC_VER
	osf_sbuf.osf_st_blksize = MD_SWAPW(sbuf.st_blksize);
	osf_sbuf.osf_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */

	/* copy fstat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
		  &osf_sbuf, sizeof(struct osf_statbuf));
      }
      break;

    case OSF_SYS_getpagesize:
      /* get target pagesize */
      regs->regs_R[MD_REG_V0] = /* was: getpagesize() */MD_PAGE_SIZE;

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_setitimer:
      /* FIXME: the sigvec system call is ignored */
      warn("syscall: setitimer ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;

    case OSF_SYS_table:
      {
	qword_t table_id, table_index, buf_addr, num_elem, size_elem;
	struct osf_tbl_sysinfo sysinfo;
	
	table_id = regs->regs_R[MD_REG_A1];
	table_index = regs->regs_R[MD_REG_A2];
	buf_addr = regs->regs_R[MD_REG_A3];
	num_elem = regs->regs_R[MD_REG_A4];
	size_elem = regs->regs_R[MD_REG_A5];
	
	switch(table_id)
	{
	case OSF_TBL_SYSINFO:
	  if (table_index != 0)
	    {
	      panic("table: table id TBL_SYSINFO requires 0 index, got %08d",
		    table_index );
	    }
	  else if (num_elem != 1)
	    {
	      panic("table: table id TBL_SYSINFO requires 1 elts, got %08d",
		    num_elem );
	    }
	  else
	    {
	      struct rusage rusage_info;
	      
	      /* use getrusage() to determine user & system time */
	      if (getrusage(RUSAGE_SELF, &rusage_info) < 0)
		{
		  /* abort the system call */
		  regs->regs_R[MD_REG_A3] = -1;
		  /* not kosher to pass off errno of getrusage() as errno
		     of table(), but what the heck... */
		  regs->regs_R[MD_REG_V0] = errno;
		  break;
		}
	      
	      /* use sysconf() to determine clock tick frequency */
	      sysinfo.si_hz = sysconf(_SC_CLK_TCK);

	      /* convert user and system time into clock ticks */
	      sysinfo.si_user = rusage_info.ru_utime.tv_sec * sysinfo.si_hz + 
		(rusage_info.ru_utime.tv_usec * sysinfo.si_hz) / 1000000UL;
	      sysinfo.si_sys = rusage_info.ru_stime.tv_sec * sysinfo.si_hz + 
		(rusage_info.ru_stime.tv_usec * sysinfo.si_hz) / 1000000UL;

	      /* following can't be determined in a portable manner and
		 are ignored */
	      sysinfo.si_nice = 0;
	      sysinfo.si_idle = 0;
	      sysinfo.si_phz = 0;
	      sysinfo.si_boottime = 0;
	      sysinfo.wait = 0;

	      /* copy structure into simulator memory */
	      mem_bcopy(mem_fn, mem, Write, buf_addr,
			&sysinfo, sizeof(struct osf_tbl_sysinfo));

	      /* return success */
	      regs->regs_R[MD_REG_A3] = 0;
	    }
	  break;

	default:
	  warn("table: unsupported table id %d requested, ignored", table_id);
	  regs->regs_R[MD_REG_A3] = 0;
	}
      }
      break;

    case OSF_SYS_getdtablesize:
#if defined(_AIX) || defined(__alpha)
      /* get descriptor table size */
      regs->regs_R[MD_REG_V0] = getdtablesize();

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#elif defined(ultrix)
      {
	/* no comparable system call found, try some reasonable defaults */
	warn("syscall: called getdtablesize\n");
	regs->regs_R[MD_REG_V0] = 16;
	regs->regs_R[MD_REG_A3] = 0;
      }
#elif defined(MIN_SYSCALL_MODE)
      {
	/* no comparable system call found, try some reasonable defaults */
	warn("syscall: called getdtablesize\n");
	regs->regs_R[MD_REG_V0] = 16;
	regs->regs_R[MD_REG_A3] = 0;
      }
#else
      {
	struct rlimit rl;

	/* get descriptor table size in rlimit structure */
	if (getrlimit(RLIMIT_NOFILE, &rl) != (qword_t)-1)
	  {
	    regs->regs_R[MD_REG_V0] = rl.rlim_cur;
	    regs->regs_R[MD_REG_A3] = 0;
	  }
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
#endif
      break;

    case OSF_SYS_dup2:
      /* dup2() the file descriptor */
      regs->regs_R[MD_REG_V0] =
	dup2(/*fd1*/regs->regs_R[MD_REG_A0], /*fd2*/regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;

    case OSF_SYS_fcntl:
#ifdef _MSC_VER
      warn("syscall fcntl() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* !_MSC_VER */
      /* get fcntl() information on the file */
      regs->regs_R[MD_REG_V0] =
	fcntl(/*fd*/regs->regs_R[MD_REG_A0],
	      /*cmd*/regs->regs_R[MD_REG_A1], /*arg*/regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
#endif /* _MSC_VER */
      break;

#if 0
    case OSF_SYS_sigvec:
      /* FIXME: the sigvec system call is ignored */
      warn("syscall: sigvec ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;
#endif

#if 0
    case OSF_SYS_sigblock:
      /* FIXME: the sigblock system call is ignored */
      warn("syscall: sigblock ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;
#endif

#if 0
    case OSF_SYS_sigsetmask:
      /* FIXME: the sigsetmask system call is ignored */
      warn("syscall: sigsetmask ignored");
      regs->regs_R[MD_REG_A3] = 0;
      break;
#endif

    case OSF_SYS_gettimeofday:
#ifdef _MSC_VER
      warn("syscall gettimeofday() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#else /* _MSC_VER */
      {
	struct osf_timeval osf_tv;
	struct timeval tv, *tvp;
	struct osf_timezone osf_tz;
	struct timezone tz, *tzp;

	if (/*timeval*/regs->regs_R[MD_REG_A0] != 0)
	  {
	    /* copy timeval into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeval*/regs->regs_R[MD_REG_A0],
		      &osf_tv, sizeof(struct osf_timeval));

	    /* convert target timeval structure to host format */
	    tv.tv_sec = MD_SWAPW(osf_tv.osf_tv_sec);
	    tv.tv_usec = MD_SWAPW(osf_tv.osf_tv_usec);
	    tvp = &tv;
	  }
	else
	  tvp = NULL;

	if (/*timezone*/regs->regs_R[MD_REG_A1] != 0)
	  {
	    /* copy timezone into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timezone*/regs->regs_R[MD_REG_A1],
		      &osf_tz, sizeof(struct osf_timezone));

	    /* convert target timezone structure to host format */
	    tz.tz_minuteswest = MD_SWAPW(osf_tz.osf_tz_minuteswest);
	    tz.tz_dsttime = MD_SWAPW(osf_tz.osf_tz_dsttime);
	    tzp = &tz;
	  }
	else
	  tzp = NULL;

	/* get time of day */
	/*result*/regs->regs_R[MD_REG_V0] = gettimeofday(tvp, tzp);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	if (/*timeval*/regs->regs_R[MD_REG_A0] != 0)
	  {
	    /* convert host timeval structure to target format */
	    osf_tv.osf_tv_sec = MD_SWAPW(tv.tv_sec);
	    osf_tv.osf_tv_usec = MD_SWAPW(tv.tv_usec);

	    /* copy timeval to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timeval*/regs->regs_R[MD_REG_A0],
		      &osf_tv, sizeof(struct osf_timeval));
	  }

	if (/*timezone*/regs->regs_R[MD_REG_A1] != 0)
	  {
	    /* convert host timezone structure to target format */
	    osf_tz.osf_tz_minuteswest = MD_SWAPW(tz.tz_minuteswest);
	    osf_tz.osf_tz_dsttime = MD_SWAPW(tz.tz_dsttime);

	    /* copy timezone to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timezone*/regs->regs_R[MD_REG_A1],
		      &osf_tz, sizeof(struct osf_timezone));
	  }
      }
#endif /* !_MSC_VER */
      break;

    case OSF_SYS_getrusage:
#if defined(__svr4__) || defined(__USLC__) || defined(hpux) || defined(__hpux) || defined(_AIX)
      {
	struct tms tms_buf;
	struct osf_rusage rusage;

	/* get user and system times */
	if (times(&tms_buf) != (qword_t)-1)
	  {
	    /* no error */
	    regs->regs_R[MD_REG_V0] = 0;
	    regs->regs_R[MD_REG_A3] = 0;
	  }
	else /* got an error, indicate result */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* initialize target rusage result structure */
#if defined(__svr4__)
	memset(&rusage, '\0', sizeof(struct osf_rusage));
#else /* !defined(__svr4__) */
	bzero(&rusage, sizeof(struct osf_rusage));
#endif

	/* convert from host rusage structure to target format */
	rusage.osf_ru_utime.osf_tv_sec = MD_SWAPW(tms_buf.tms_utime/CLK_TCK);
	rusage.osf_ru_utime.osf_tv_sec =
	  MD_SWAPW(rusage.osf_ru_utime.osf_tv_sec);
	rusage.osf_ru_utime.osf_tv_usec = 0;
	rusage.osf_ru_stime.osf_tv_sec = MD_SWAPW(tms_buf.tms_stime/CLK_TCK);
	rusage.osf_ru_stime.osf_tv_sec =
	  MD_SWAPW(rusage.osf_ru_stime.osf_tv_sec);
	rusage.osf_ru_stime.osf_tv_usec = 0;

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[MD_REG_A1],
		  &rusage, sizeof(struct osf_rusage));
      }
#elif defined(__unix__)
      {
	struct rusage local_rusage;
	struct osf_rusage rusage;

	/* get rusage information */
	/*result*/regs->regs_R[MD_REG_V0] =
	  getrusage(/*who*/regs->regs_R[MD_REG_A0], &local_rusage);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* convert from host rusage structure to target format */
	rusage.osf_ru_utime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_utime.tv_sec);
	rusage.osf_ru_utime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_utime.tv_usec);
	rusage.osf_ru_utime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_utime.tv_sec);
	rusage.osf_ru_utime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_utime.tv_usec);
	rusage.osf_ru_stime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_stime.tv_sec);
	rusage.osf_ru_stime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_stime.tv_usec);
	rusage.osf_ru_stime.osf_tv_sec =
	  MD_SWAPW(local_rusage.ru_stime.tv_sec);
	rusage.osf_ru_stime.osf_tv_usec =
	  MD_SWAPW(local_rusage.ru_stime.tv_usec);
	rusage.osf_ru_maxrss = MD_SWAPW(local_rusage.ru_maxrss);
	rusage.osf_ru_ixrss = MD_SWAPW(local_rusage.ru_ixrss);
	rusage.osf_ru_idrss = MD_SWAPW(local_rusage.ru_idrss);
	rusage.osf_ru_isrss = MD_SWAPW(local_rusage.ru_isrss);
	rusage.osf_ru_minflt = MD_SWAPW(local_rusage.ru_minflt);
	rusage.osf_ru_majflt = MD_SWAPW(local_rusage.ru_majflt);
	rusage.osf_ru_nswap = MD_SWAPW(local_rusage.ru_nswap);
	rusage.osf_ru_inblock = MD_SWAPW(local_rusage.ru_inblock);
	rusage.osf_ru_oublock = MD_SWAPW(local_rusage.ru_oublock);
	rusage.osf_ru_msgsnd = MD_SWAPW(local_rusage.ru_msgsnd);
	rusage.osf_ru_msgrcv = MD_SWAPW(local_rusage.ru_msgrcv);
	rusage.osf_ru_nsignals = MD_SWAPW(local_rusage.ru_nsignals);
	rusage.osf_ru_nvcsw = MD_SWAPW(local_rusage.ru_nvcsw);
	rusage.osf_ru_nivcsw = MD_SWAPW(local_rusage.ru_nivcsw);

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[MD_REG_A1],
		  &rusage, sizeof(struct osf_rusage));
      }
#elif defined(__CYGWIN32__) || defined(_MSC_VER)
	    warn("syscall: called getrusage\n");
            regs->regs_R[7] = 0;
#else
#error No getrusage() implementation!
#endif
      break;

    case OSF_SYS_utimes:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	if (/*timeval*/regs->regs_R[MD_REG_A1] == 0)
	  {
#if defined(hpux) || defined(__hpux) || defined(__i386__)
	    /* no utimes() in hpux, use utime() instead */
	    /*result*/regs->regs_R[MD_REG_V0] = utime(buf, NULL);
#elif defined(_MSC_VER)
            /* no utimes() in MSC, use utime() instead */
	    /*result*/regs->regs_R[MD_REG_V0] = utime(buf, NULL);
#elif defined(__svr4__) || defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /*result*/regs->regs_R[MD_REG_V0] = utimes(buf, NULL);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes\n");
#else
#error No utimes() implementation!
#endif
	  }
	else
	  {
	    struct osf_timeval osf_tval[2];
#ifndef _MSC_VER
	    struct timeval tval[2];
#endif /* !_MSC_VER */

	    /* copy timeval structure to host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeout*/regs->regs_R[MD_REG_A1],
		      osf_tval, 2*sizeof(struct osf_timeval));

#ifndef _MSC_VER
	    /* convert timeval structure to host format */
	    tval[0].tv_sec = MD_SWAPW(osf_tval[0].osf_tv_sec);
	    tval[0].tv_usec = MD_SWAPW(osf_tval[0].osf_tv_usec);
	    tval[1].tv_sec = MD_SWAPW(osf_tval[1].osf_tv_sec);
	    tval[1].tv_usec = MD_SWAPW(osf_tval[1].osf_tv_usec);
#endif /* !_MSC_VER */

#if defined(hpux) || defined(__hpux) || defined(__svr4__)
	    /* no utimes() in hpux, use utime() instead */
	    {
	      struct utimbuf ubuf;

	      ubuf.actime = MD_SWAPW(tval[0].tv_sec);
	      ubuf.modtime = MD_SWAPW(tval[1].tv_sec);

	      /* result */regs->regs_R[MD_REG_V0] = utime(buf, &ubuf);
	    }
#elif defined(_MSC_VER)
            /* no utimes() in hpux, use utime() instead */
            {
              struct _utimbuf ubuf;

              ubuf.actime = MD_SWAPW(osf_tval[0].osf_tv_sec);
              ubuf.modtime = MD_SWAPW(osf_tval[1].osf_tv_sec);

              /* result */regs->regs_R[MD_REG_V0] = utime(buf, &ubuf);
            }
#elif defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /* result */regs->regs_R[MD_REG_V0] = utimes(buf, tval);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes\n");
#else
#error No utimes() implementation!
#endif
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;

    case OSF_SYS_getrlimit:
    case OSF_SYS_setrlimit:
#ifdef _MSC_VER
      warn("syscall get/setrlimit() not yet implemented for MSC...");
      regs->regs_R[MD_REG_A3] = 0;
#elif defined(__CYGWIN32__)
      {
	warn("syscall: called get/setrlimit\n");
	regs->regs_R[MD_REG_A3] = 0;
      }
#else
      {
	struct osf_rlimit osf_rl;
	struct rlimit rl;

	/* copy rlimit structure to host memory */
	mem_bcopy(mem_fn, mem, Read, /*rlimit*/regs->regs_R[MD_REG_A1],
		  &osf_rl, sizeof(struct osf_rlimit));

	/* convert rlimit structure to host format */
	rl.rlim_cur = MD_SWAPQ(osf_rl.osf_rlim_cur);
	rl.rlim_max = MD_SWAPQ(osf_rl.osf_rlim_max);

	/* get rlimit information */
	if (syscode == OSF_SYS_getrlimit)
	  /*result*/regs->regs_R[MD_REG_V0] =
	    getrlimit(regs->regs_R[MD_REG_A0], &rl);
	else /* syscode == OSF_SYS_setrlimit */
	  /*result*/regs->regs_R[MD_REG_V0] =
	    setrlimit(regs->regs_R[MD_REG_A0], &rl);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* convert rlimit structure to target format */
	osf_rl.osf_rlim_cur = MD_SWAPQ(rl.rlim_cur);
	osf_rl.osf_rlim_max = MD_SWAPQ(rl.rlim_max);

	/* copy rlimit structure to target memory */
	mem_bcopy(mem_fn, mem, Write, /*rlimit*/regs->regs_R[MD_REG_A1],
		  &osf_rl, sizeof(struct osf_rlimit));
      }
#endif
      break;

    case OSF_SYS_sigprocmask:
      {
	static int first = TRUE;

	if (first)
	  {
	    warn("partially supported sigprocmask() call...");
	    first = FALSE;
	  }

	/* from klauser@cs.colorado.edu: there are a couple bugs in the
	   sigprocmask implementation; here is a fix: the problem comes from an
	   impedance mismatch between application/libc interface and
	   libc/syscall interface, the former of which you read about in the
	   manpage, the latter of which you actually intercept here.  The
	   following is mostly correct, but does not capture some minor
	   details, which you only get right if you really let the kernel
	   handle it. (e.g. you can't really ever block sigkill etc.) */

        regs->regs_R[MD_REG_V0] = sigmask;
        regs->regs_R[MD_REG_A3] = 0;

        switch (regs->regs_R[MD_REG_A0])
	  {
          case OSF_SIG_BLOCK:
            sigmask |= regs->regs_R[MD_REG_A1];
            break;
          case OSF_SIG_UNBLOCK:
            sigmask &= ~regs->regs_R[MD_REG_A1];
            break;
          case OSF_SIG_SETMASK:
            sigmask = regs->regs_R[MD_REG_A1];
            break;
          default:
            regs->regs_R[MD_REG_V0] = EINVAL;
            regs->regs_R[MD_REG_A3] = 1;
	  }

#if 0 /* FIXME: obsolete... */
	if (regs->regs_R[MD_REG_A2] > /* FIXME: why? */0x10000000)
	  mem_bcopy(mem_fn, mem, Write, regs->regs_R[MD_REG_A2],
		    &sigmask, sizeof(sigmask));

	if (regs->regs_R[MD_REG_A1] != 0)
	  {
	    switch (regs->regs_R[MD_REG_A0])
	      {
	      case OSF_SIG_BLOCK:
		sigmask |= regs->regs_R[MD_REG_A1];
		break;
	      case OSF_SIG_UNBLOCK:
		sigmask &= regs->regs_R[MD_REG_A1]; /* I think */
	      break;
	      case OSF_SIG_SETMASK:
		sigmask = regs->regs_R[MD_REG_A1]; /* I think */
		break;
	      default:
		panic("illegal how value to sigprocmask()");
	      }
	  }
	regs->regs_R[MD_REG_V0] = 0;
	regs->regs_R[MD_REG_A3] = 0;
#endif
      }
      break;

    case OSF_SYS_sigaction:
      {
	int signum;
	static int first = TRUE;

	if (first)
	  {
	    warn("partially supported sigaction() call...");
	    first = FALSE;
	  }

	signum = regs->regs_R[MD_REG_A0];
	if (regs->regs_R[MD_REG_A1] != 0)
	  sigaction_array[signum] = regs->regs_R[MD_REG_A1];

	if (regs->regs_R[MD_REG_A2])
	  regs->regs_R[MD_REG_A2] = sigaction_array[signum];

	regs->regs_R[MD_REG_V0] = 0;

	/* for some reason, __sigaction expects A3 to have a 0 return value */
	regs->regs_R[MD_REG_A3] = 0;
  
	/* FIXME: still need to add code so that on a signal, the 
	   correct action is actually taken. */

	/* FIXME: still need to add support for returning the correct
	   error messages (EFAULT, EINVAL) */
      }
      break;

    case OSF_SYS_sigstack:
      warn("unsupported sigstack() call...");
      regs->regs_R[MD_REG_A3] = 0;
      break;

    case OSF_SYS_sigreturn:
      {
	int i;
	struct osf_sigcontext sc;
	static int first = TRUE;

	if (first)
	  {
	    warn("partially supported sigreturn() call...");
	    first = FALSE;
	  }

	mem_bcopy(mem_fn, mem, Read, /* sc */regs->regs_R[MD_REG_A0],
		  &sc, sizeof(struct osf_sigcontext));

	sigmask = MD_SWAPQ(sc.sc_mask); /* was: prog_sigmask */
	regs->regs_NPC = MD_SWAPQ(sc.sc_pc);

	/* FIXME: should check for the branch delay bit */
	/* FIXME: current->nextpc = current->pc + 4; not sure about this... */
	for (i=0; i < 32; ++i)
	  regs->regs_R[i] = sc.sc_regs[i];
	for (i=0; i < 32; ++i)
	  regs->regs_F.q[i] = sc.sc_fpregs[i];
	regs->regs_C.fpcr = sc.sc_fpcr;
      }
      break;

    case OSF_SYS_uswitch:
      warn("unsupported uswitch() call...");
      regs->regs_R[MD_REG_V0] = regs->regs_R[MD_REG_A1]; 
      break;

    case OSF_SYS_setsysinfo:
      warn("unsupported setsysinfo() call...");
      regs->regs_R[MD_REG_V0] = 0; 
      break;

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_getdirentries:
      {
	int i, cnt, osf_cnt;
	struct dirent *p;
	sword_t fd = regs->regs_R[MD_REG_A0];
	md_addr_t osf_buf = regs->regs_R[MD_REG_A1];
	char *buf;
	sword_t osf_nbytes = regs->regs_R[MD_REG_A2];
	md_addr_t osf_pbase = regs->regs_R[MD_REG_A3];
	sqword_t osf_base;
	long base = 0;

	/* number of entries in simulated memory */
	if (!osf_nbytes)
	  warn("attempting to get 0 directory entries...");

	/* allocate local memory, whatever fits */
	buf = calloc(1, osf_nbytes);
	if (!buf)
	  fatal("out of virtual memory");

	/* get directory entries */
#if defined(__svr4__)
	base = lseek ((int)fd, (off_t)0, SEEK_CUR);
	regs->regs_R[MD_REG_V0] =
	  getdents((int)fd, (struct dirent *)buf, (size_t)osf_nbytes);
#else /* !__svr4__ */
	regs->regs_R[MD_REG_V0] =
	  getdirentries((int)fd, buf, (size_t)osf_nbytes, &base);
#endif

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  {
	    regs->regs_R[MD_REG_A3] = 0;

	    /* anything to copy back? */
	    if (regs->regs_R[MD_REG_V0] > 0)
	      {
		/* copy all possible results to simulated space */
		for (i=0, cnt=0, osf_cnt=0, p=(struct dirent *)buf;
		     cnt < regs->regs_R[MD_REG_V0] && p->d_reclen > 0;
		     i++, cnt += p->d_reclen, p=(struct dirent *)(buf+cnt))
		  {
		    struct osf_dirent osf_dirent;

		    osf_dirent.d_ino = MD_SWAPW(p->d_ino);
		    osf_dirent.d_namlen = MD_SWAPH(strlen(p->d_name));
		    strcpy(osf_dirent.d_name, p->d_name);
		    osf_dirent.d_reclen = MD_SWAPH(OSF_DIRENT_SZ(p->d_name));

		    mem_bcopy(mem_fn, mem, Write,
			      osf_buf + osf_cnt,
			      &osf_dirent, OSF_DIRENT_SZ(p->d_name));
		    osf_cnt += OSF_DIRENT_SZ(p->d_name);
		  }

		if (osf_pbase != 0)
		  {
		    osf_base = (sqword_t)base;
		    mem_bcopy(mem_fn, mem, Write, osf_pbase,
			      &osf_base, sizeof(osf_base));
		  }

		/* update V0 to indicate translated read length */
		regs->regs_R[MD_REG_V0] = osf_cnt;
	      }
	  }
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_truncate:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[MD_REG_A0], buf);

	/* truncate the file */
	/*result*/regs->regs_R[MD_REG_V0] =
	  truncate(buf, /* length */(size_t)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

#if !defined(__CYGWIN32__) && !defined(_MSC_VER)
    case OSF_SYS_ftruncate:
      /* truncate the file */
      /*result*/regs->regs_R[MD_REG_V0] =
	ftruncate(/* fd */(int)regs->regs_R[MD_REG_A0],
		 /* length */(size_t)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_statfs:
      {
	char buf[MAXBUFSIZE];
	struct osf_statfs osf_sbuf;
	struct statfs sbuf;

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[MD_REG_A0], buf);

	/* statfs() the fs */
	/*result*/regs->regs_R[MD_REG_V0] = statfs(buf, &sbuf);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* translate from host stat structure to target format */
#if defined(__svr4__) || defined(__osf__)
	osf_sbuf.f_type = MD_SWAPH(0x6969) /* NFS, whatever... */;
#else /* !__svr4__ */
	osf_sbuf.f_type = MD_SWAPH(sbuf.f_type);
#endif
	osf_sbuf.f_fsize = MD_SWAPW(sbuf.f_bsize);
	osf_sbuf.f_blocks = MD_SWAPW(sbuf.f_blocks);
	osf_sbuf.f_bfree = MD_SWAPW(sbuf.f_bfree);
	osf_sbuf.f_bavail = MD_SWAPW(sbuf.f_bavail);
	osf_sbuf.f_files = MD_SWAPW(sbuf.f_files);
	osf_sbuf.f_ffree = MD_SWAPW(sbuf.f_ffree);
	/* osf_sbuf.f_fsid = MD_SWAPW(sbuf.f_fsid); */

	/* copy stat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[MD_REG_A1],
		  &osf_sbuf, sizeof(struct osf_statbuf));
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setregid:
      /* set real and effective group ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setregid(/* rgid */(gid_t)regs->regs_R[MD_REG_A0],
		 /* egid */(gid_t)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setreuid:
      /* set real and effective user ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setreuid(/* ruid */(uid_t)regs->regs_R[MD_REG_A0],
		 /* euid */(uid_t)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_socket:
      /* create an endpoint for communication */

      /* result */regs->regs_R[MD_REG_V0] =
	socket(/* domain */xlate_arg((int)regs->regs_R[MD_REG_A0],
				     family_map, N_ELT(family_map),
				     "socket(family)"),
	       /* type */xlate_arg((int)regs->regs_R[MD_REG_A1],
				   socktype_map, N_ELT(socktype_map),
				   "socket(type)"),
	       /* protocol */xlate_arg((int)regs->regs_R[MD_REG_A2],
				       family_map, N_ELT(family_map),
				       "socket(proto)"));

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_connect:
      {
	struct osf_sockaddr osf_sa;

	/* initiate a connection on a socket */

	/* get the socket address */
	if (regs->regs_R[MD_REG_A2] > sizeof(struct osf_sockaddr))
	  {
	    fatal("sockaddr size overflow: addrlen = %d",
		  regs->regs_R[MD_REG_A2]);
	  }
	/* copy sockaddr structure to host memory */
	mem_bcopy(mem_fn, mem, Read, /* serv_addr */regs->regs_R[MD_REG_A1],
		  &osf_sa, /* addrlen */(int)regs->regs_R[MD_REG_A2]);
#if 0
	int i;
	sa.sa_family = osf_sa.sa_family;
	for (i=0; i < regs->regs_R[MD_REG_A2]; i++)
	  sa.sa_data[i] = osf_sa.sa_data[i];
#endif
	/* result */regs->regs_R[MD_REG_V0] =
	  connect(/* sockfd */(int)regs->regs_R[MD_REG_A0],
		  (void *)&osf_sa, /* addrlen */(int)regs->regs_R[MD_REG_A2]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_uname:
      /* get name and information about current kernel */

      regs->regs_R[MD_REG_A3] = -1;
      regs->regs_R[MD_REG_V0] = EPERM;
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_writev:
      {
	int i;
	char *buf;
	struct iovec *iov;

	/* allocate host side I/O vectors */
	iov =
	  (struct iovec *)malloc(/* iovcnt */regs->regs_R[MD_REG_A2]
				 * sizeof(struct iovec));
	if (!iov)
	  fatal("out of virtual memory in SYS_writev");

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    struct osf_iovec osf_iov;

	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      (/*iov*/regs->regs_R[MD_REG_A1]
		       + i*sizeof(struct osf_iovec)),
		      &osf_iov, sizeof(struct osf_iovec));

	    iov[i].iov_len = MD_SWAPW(osf_iov.iov_len);
	    if (osf_iov.iov_base != 0 && osf_iov.iov_len != 0)
	      {
		buf = (char *)calloc(MD_SWAPW(osf_iov.iov_len), sizeof(char));
		if (!buf)
		  fatal("out of virtual memory in SYS_writev");
		mem_bcopy(mem_fn, mem, Read, MD_SWAPQ(osf_iov.iov_base),
			  buf, MD_SWAPW(osf_iov.iov_len));
		iov[i].iov_base = buf;
	      }
	    else
	      iov[i].iov_base = NULL;
	  }

	/* perform the vector'ed write */
	do {
	  /*result*/regs->regs_R[MD_REG_V0] =
	    writev(/* fd */(int)regs->regs_R[MD_REG_A0], iov,
		   /* iovcnt */(size_t)regs->regs_R[MD_REG_A2]);
	} while (/*result*/regs->regs_R[MD_REG_V0] == -1
	         && errno == EAGAIN);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* free all the allocated memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    if (iov[i].iov_base)
	      {
		free(iov[i].iov_base);
		iov[i].iov_base = NULL;
	      }
	  }
	free(iov);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_readv:
      {
	int i;
	char *buf = NULL;
	struct osf_iovec *osf_iov;
	struct iovec *iov;

	/* allocate host side I/O vectors */
	osf_iov =
	  calloc(/* iovcnt */regs->regs_R[MD_REG_A2],
		 sizeof(struct osf_iovec));
	if (!osf_iov)
	  fatal("out of virtual memory in SYS_readv");

	iov =
	  calloc(/* iovcnt */regs->regs_R[MD_REG_A2], sizeof(struct iovec));
	if (!iov)
	  fatal("out of virtual memory in SYS_readv");

	/* copy host side I/O vector buffers */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      (/*iov*/regs->regs_R[MD_REG_A1]
		       + i*sizeof(struct osf_iovec)),
		      &osf_iov[i], sizeof(struct osf_iovec));

	    iov[i].iov_len = MD_SWAPW(osf_iov[i].iov_len);
	    if (osf_iov[i].iov_base != 0 && osf_iov[i].iov_len != 0)
	      {
		buf =
		  (char *)calloc(MD_SWAPW(osf_iov[i].iov_len), sizeof(char));
		if (!buf)
		  fatal("out of virtual memory in SYS_readv");
		iov[i].iov_base = buf;
	      }
	    else
	      iov[i].iov_base = NULL;
	  }

	/* perform the vector'ed read */
	do {
	  /*result*/regs->regs_R[MD_REG_V0] =
	    readv(/* fd */(int)regs->regs_R[MD_REG_A0], iov,
		  /* iovcnt */(size_t)regs->regs_R[MD_REG_A2]);
	} while (/*result*/regs->regs_R[MD_REG_V0] == -1
		 && errno == EAGAIN);

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    if (osf_iov[i].iov_base != 0)
	      {
		mem_bcopy(mem_fn, mem, Write, MD_SWAPQ(osf_iov[i].iov_base),
			  iov[i].iov_base, MD_SWAPW(osf_iov[i].iov_len));
	      }
	  }

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* free all the allocated memory */
	for (i=0; i < /* iovcnt */regs->regs_R[MD_REG_A2]; i++)
	  {
	    if (iov[i].iov_base)
	      {
		free(iov[i].iov_base);
		iov[i].iov_base = NULL;
	      }
	  }

	if (osf_iov)
	  free(osf_iov);
	if (iov)
	  free(iov);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setsockopt:
      {
	char *buf;
	struct xlate_table_t *map;
	int nmap;

 	/* set options on sockets */

	/* copy optval to host memory */
	if (/* optval */regs->regs_R[MD_REG_A3] != 0
	    && /* optlen */regs->regs_R[MD_REG_A4] != 0)
	  {
	    buf = calloc(1, /* optlen */(size_t)regs->regs_R[MD_REG_A4]);
	    if (!buf)
	      fatal("cannot allocate memory in OSF_SYS_setsockopt");
	    
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      /* optval */regs->regs_R[MD_REG_A3],
		      buf, /* optlen */(int)regs->regs_R[MD_REG_A4]);
	  }
	else
	  buf = NULL;

	/* pick the correct translation table */
	if ((int)regs->regs_R[MD_REG_A1] == OSF_SOL_SOCKET)
	  {
	    map = sockopt_map;
	    nmap = N_ELT(sockopt_map);
	  }
	else if ((int)regs->regs_R[MD_REG_A1] == OSF_SOL_TCP)
	  {
	    map = tcpopt_map;
	    nmap = N_ELT(tcpopt_map);
	  }
	else
	  {
	    warn("no translation map available for `setsockopt()': %d",
		 (int)regs->regs_R[MD_REG_A1]);
	    map = sockopt_map;
	    nmap = N_ELT(sockopt_map);
	  }

	/* result */regs->regs_R[MD_REG_V0] =
	  setsockopt(/* sock */(int)regs->regs_R[MD_REG_A0],
		     /* level */xlate_arg((int)regs->regs_R[MD_REG_A1],
					  socklevel_map, N_ELT(socklevel_map),
					  "setsockopt(level)"),
		     /* optname */xlate_arg((int)regs->regs_R[MD_REG_A2],
					    map, nmap,
					    "setsockopt(opt)"),
		     /* optval */buf,
		     /* optlen */regs->regs_R[MD_REG_A4]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_old_getsockname:
      {
	/* get socket name */
	char *buf;
	word_t osf_addrlen;
	int addrlen;

	/* get simulator memory parameters to host memory */
	mem_bcopy(mem_fn, mem, Read,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));
	addrlen = (int)osf_addrlen;
	if (addrlen != 0)
	  {
	    buf = calloc(1, addrlen);
	    if (!buf)
	      fatal("cannot allocate memory in OSF_SYS_old_getsockname");
	  }
	else
	  buf = NULL;
	
	/* result */regs->regs_R[MD_REG_V0] =
	  getsockname(/* sock */(int)regs->regs_R[MD_REG_A0],
		      /* name */(struct sockaddr *)buf,
		      /* namelen */&addrlen);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy results to simulator memory */
	if (addrlen != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* addr */regs->regs_R[MD_REG_A1],
		    buf, addrlen);
	osf_addrlen = (qword_t)addrlen;
	mem_bcopy(mem_fn, mem, Write,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));

	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_old_getpeername:
      {
	/* get socket name */
	char *buf;
	word_t osf_addrlen;
	int addrlen;

	/* get simulator memory parameters to host memory */
	mem_bcopy(mem_fn, mem, Read,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));
	addrlen = (int)osf_addrlen;
	if (addrlen != 0)
	  {
	    buf = calloc(1, addrlen);
	    if (!buf)
	      fatal("cannot allocate memory in OSF_SYS_old_getsockname");
	  }
	else
	  buf = NULL;
	
	/* result */regs->regs_R[MD_REG_V0] =
	  getpeername(/* sock */(int)regs->regs_R[MD_REG_A0],
		      /* name */(struct sockaddr *)buf,
		      /* namelen */&addrlen);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy results to simulator memory */
	if (addrlen != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* addr */regs->regs_R[MD_REG_A1],
		    buf, addrlen);
	osf_addrlen = (qword_t)addrlen;
	mem_bcopy(mem_fn, mem, Write,
		  /* paddrlen */regs->regs_R[MD_REG_A2],
		  &osf_addrlen, sizeof(osf_addrlen));

	if (buf != NULL)
	  free(buf);
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setgid:
      /* set group ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setgid(/* gid */(gid_t)regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setuid:
      /* set user ID */

      /*result*/regs->regs_R[MD_REG_V0] =
	setuid(/* uid */(uid_t)regs->regs_R[MD_REG_A0]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_getpriority:
      /* get program scheduling priority */

      /*result*/regs->regs_R[MD_REG_V0] =
	getpriority(/* which */(int)regs->regs_R[MD_REG_A0],
		    /* who */(int)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_setpriority:
      /* set program scheduling priority */

      /*result*/regs->regs_R[MD_REG_V0] =
	setpriority(/* which */(int)regs->regs_R[MD_REG_A0],
		    /* who */(int)regs->regs_R[MD_REG_A1],
		    /* prio */(int)regs->regs_R[MD_REG_A2]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_select:
      {
	fd_set readfd, writefd, exceptfd;
	fd_set *readfdp, *writefdp, *exceptfdp;
	struct timeval timeout, *timeoutp;

	/* copy read file descriptor set into host memory */
	if (/* readfds */regs->regs_R[MD_REG_A1] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* readfds */regs->regs_R[MD_REG_A1],
		      &readfd, sizeof(fd_set));
	    readfdp = &readfd;
	  }
	else
	  readfdp = NULL;

	/* copy write file descriptor set into host memory */
	if (/* writefds */regs->regs_R[MD_REG_A2] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* writefds */regs->regs_R[MD_REG_A2],
		      &writefd, sizeof(fd_set));
	    writefdp = &writefd;
	  }
	else
	  writefdp = NULL;

	/* copy exception file descriptor set into host memory */
	if (/* exceptfds */regs->regs_R[MD_REG_A3] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* exceptfds */regs->regs_R[MD_REG_A3],
		      &exceptfd, sizeof(fd_set));
	    exceptfdp = &exceptfd;
	  }
	else
	  exceptfdp = NULL;

	/* copy timeout value into host memory */
	if (/* timeout */regs->regs_R[MD_REG_A4] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read,
		      /* timeout */regs->regs_R[MD_REG_A4],
		      &timeout, sizeof(struct timeval));
	    timeoutp = &timeout;
	  }
	else
	  timeoutp = NULL;

#if defined(hpux) || defined(__hpux)
	/* select() on the specified file descriptors */
	/* result */regs->regs_R[MD_REG_V0] =
	  select(/* nfds */regs->regs_R[MD_REG_A0],
		 (int *)readfdp, (int *)writefdp, (int *)exceptfdp, timeoutp);
#else
	/* select() on the specified file descriptors */
	/* result */regs->regs_R[MD_REG_V0] =
	  select(/* nfds */regs->regs_R[MD_REG_A0],
		 readfdp, writefdp, exceptfdp, timeoutp);
#endif

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy read file descriptor set to target memory */
	if (/* readfds */regs->regs_R[MD_REG_A1] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* readfds */regs->regs_R[MD_REG_A1],
		    &readfd, sizeof(fd_set));

	/* copy write file descriptor set to target memory */
	if (/* writefds */regs->regs_R[MD_REG_A2] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* writefds */regs->regs_R[MD_REG_A2],
		    &writefd, sizeof(fd_set));

	/* copy exception file descriptor set to target memory */
	if (/* exceptfds */regs->regs_R[MD_REG_A3] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* exceptfds */regs->regs_R[MD_REG_A3],
		    &exceptfd, sizeof(fd_set));

	/* copy timeout value result to target memory */
	if (/* timeout */regs->regs_R[MD_REG_A4] != 0)
	  mem_bcopy(mem_fn, mem, Write,
		    /* timeout */regs->regs_R[MD_REG_A4],
		    &timeout, sizeof(struct timeval));
      }
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_shutdown:
      /* shuts down socket send and receive operations */

      /*result*/regs->regs_R[MD_REG_V0] =
	shutdown(/* sock */(int)regs->regs_R[MD_REG_A0],
		 /* how */(int)regs->regs_R[MD_REG_A1]);

      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
	{
	  regs->regs_R[MD_REG_A3] = -1;
	  regs->regs_R[MD_REG_V0] = errno;
	}
      break;
#endif

#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_poll:
      {
	int i;
	struct pollfd *fds;

	/* allocate host side I/O vectors */
	fds = calloc(/* nfds */regs->regs_R[MD_REG_A1], sizeof(struct pollfd));
	if (!fds)
	  fatal("out of virtual memory in SYS_poll");

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* nfds */regs->regs_R[MD_REG_A1]; i++)
	  {
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Read,
		      (/* fds */regs->regs_R[MD_REG_A0]
		       + i*sizeof(struct pollfd)),
		      &fds[i], sizeof(struct pollfd));
	  }

	/* perform the vector'ed write */
	/* result */regs->regs_R[MD_REG_V0] =
	  poll(/* fds */fds,
	       /* nfds */(unsigned long)regs->regs_R[MD_REG_A1],
	       /* timeout */(int)regs->regs_R[MD_REG_A2]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /* nfds */regs->regs_R[MD_REG_A1]; i++)
	  {
	    /* copy target side pointer data into host side vector */
	    mem_bcopy(mem_fn, mem, Write,
		      (/* fds */regs->regs_R[MD_REG_A0]
		       + i*sizeof(struct pollfd)),
		      &fds[i], sizeof(struct pollfd));
	  }

	/* free all the allocated memory */
	free(fds);
      }
      break;
#endif

    case OSF_SYS_usleep_thread:
#if 0
      fprintf(stderr, "usleep(%d)\n", (unsigned int)regs->regs_R[MD_REG_A0]);
#endif
#ifdef alpha
      regs->regs_R[MD_REG_V0] = usleep((unsigned int)regs->regs_R[MD_REG_A0]);
#else
      usleep((unsigned int)regs->regs_R[MD_REG_A0]);
      regs->regs_R[MD_REG_V0] = 0;
#endif
      /* check for an error condition */
      if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
        regs->regs_R[MD_REG_A3] = 0;
      else /* got an error, return details */
        {
          regs->regs_R[MD_REG_A3] = -1;
          regs->regs_R[MD_REG_V0] = errno;
        }
#if 0
      warn("unsupported usleep_thread() call...");
      regs->regs_R[MD_REG_V0] = 0; 
#endif
      break;
      
#if !defined(MIN_SYSCALL_MODE)
    case OSF_SYS_gethostname:
      /* get program scheduling priority */
      {
	char *buf;

	buf = malloc(/* len */(size_t)regs->regs_R[MD_REG_A1]);
	if (!buf)
	  fatal("out of virtual memory in gethostname()");

	/* result */regs->regs_R[MD_REG_V0] =
	  gethostname(/* name */buf,
		      /* len */(size_t)regs->regs_R[MD_REG_A1]);

	/* check for an error condition */
	if (regs->regs_R[MD_REG_V0] != (qword_t)-1)
	  regs->regs_R[MD_REG_A3] = 0;
	else /* got an error, return details */
	  {
	    regs->regs_R[MD_REG_A3] = -1;
	    regs->regs_R[MD_REG_V0] = errno;
	  }

	/* copy string back to simulated memory */
	mem_bcopy(mem_fn, mem, Write,
		  /* name */regs->regs_R[MD_REG_A0],
		  buf, /* len */regs->regs_R[MD_REG_A1]);
      }
      break;
#endif

    case OSF_SYS_madvise:
      warn("unsupported madvise() call ignored...");
      regs->regs_R[MD_REG_V0] = 0;
      break;

    default:
      warn("invalid/unimplemented syscall %ld, PC=0x%08p, RA=0x%08p, winging it",
	   syscode, regs->regs_PC, regs->regs_R[MD_REG_RA]);
      regs->regs_R[MD_REG_A3] = -1;
      regs->regs_R[MD_REG_V0] = 0;
#if 0
      fatal("invalid/unimplemented system call encountered, code %d", syscode);
#endif
    }
#endif
    
  if (verbose)
    fprintf(stderr, "syscall(%d): returned %d:%d...\n",
            (int)syscode, (int)regs->regs_R[MD_REG_A3],
            (int)regs->regs_R[MD_REG_V0]);
}
