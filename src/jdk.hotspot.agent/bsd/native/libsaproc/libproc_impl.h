/*
 * Copyright (c) 2003, 2020, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef _LIBPROC_IMPL_H_
#define _LIBPROC_IMPL_H_

#include <unistd.h>
#include <limits.h>
#include "libproc.h"
#include "symtab.h"

#ifdef __APPLE__
#include <inttypes.h>     // for PRIx64, 32, ...
#include <pthread.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>

#ifndef register_t
#define register_t uint64_t
#endif

/*** registers copied from bsd/amd64 */
typedef struct reg {
  register_t      r_r15;
  register_t      r_r14;
  register_t      r_r13;
  register_t      r_r12;
  register_t      r_r11;
  register_t      r_r10;
  register_t      r_r9;
  register_t      r_r8;
  register_t      r_rdi;
  register_t      r_rsi;
  register_t      r_rbp;
  register_t      r_rbx;
  register_t      r_rdx;
  register_t      r_rcx;
  register_t      r_rax;
  uint32_t        r_trapno;      // not used
  uint16_t        r_fs;
  uint16_t        r_gs;
  uint32_t        r_err;         // not used
  uint16_t        r_es;          // not used
  uint16_t        r_ds;          // not used
  register_t      r_rip;
  register_t      r_cs;
  register_t      r_rflags;
  register_t      r_rsp;
  register_t      r_ss;          // not used
} reg;

// convenient defs
typedef struct mach_header_64 mach_header_64;
typedef struct load_command load_command;
typedef struct segment_command_64 segment_command_64;
typedef struct thread_command thread_command;
typedef struct dylib_command dylib_command;
typedef struct symtab_command symtab_command;
typedef struct nlist_64 nlist_64;
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/ptrace.h>
#include "salibelf.h"
#endif //  __APPLE__

// data structures in this file mimic those of Solaris 8.0 - libproc's Pcontrol.h

#define BUF_SIZE     (PATH_MAX + NAME_MAX + 1)

// list of shared objects
typedef struct lib_info {
  char             name[BUF_SIZE];
  uintptr_t        base;
  struct symtab*   symtab;
  int              fd;        // file descriptor for lib
  struct lib_info* next;
} lib_info;

// list of threads
typedef struct sa_thread_info {
   lwpid_t                  lwp_id;     // same as pthread_t
   struct reg               regs;       // not for process, core uses for caching regset
   struct sa_thread_info*   next;
} sa_thread_info;

// list of virtual memory maps
typedef struct map_info {
   int              fd;       // file descriptor
   off_t            offset;   // file offset of this mapping
   uintptr_t        vaddr;    // starting virtual address
   size_t           memsz;    // size of the mapping
   uint32_t         flags;    // acces flags
   struct map_info* next;
} map_info;

// vtable for ps_prochandle
typedef struct ps_prochandle_ops {
   // "derived class" clean-up
   void (*release)(struct ps_prochandle* ph);
   // read from debuggee
   bool (*p_pread)(struct ps_prochandle *ph,
            uintptr_t addr, char *buf, size_t size);
   // write into debuggee
   bool (*p_pwrite)(struct ps_prochandle *ph,
            uintptr_t addr, const char *buf , size_t size);
   // get integer regset of a thread
   bool (*get_lwp_regs)(struct ps_prochandle* ph, lwpid_t lwp_id, struct reg* regs);
   // get info on thread
   bool (*get_lwp_info)(struct ps_prochandle *ph, lwpid_t lwp_id, void *linfo);
} ps_prochandle_ops;

// the ps_prochandle

struct core_data {
   int                core_fd;   // file descriptor of core file
   int                exec_fd;   // file descriptor of exec file
   int                interp_fd; // file descriptor of interpreter (ld-elf.so.1)
   // part of the class sharing workaround
   int                classes_jsa_fd; // file descriptor of class share archive
   uintptr_t          dynamic_addr;  // address of dynamic section of a.out
   uintptr_t          ld_base_addr;  // base address of ld.so
   size_t             num_maps;  // number of maps.
   map_info*          maps;      // maps in a linked list
   // part of the class sharing workaround
   map_info*          class_share_maps;// class share maps in a linked list
   map_info**         map_array; // sorted (by vaddr) array of map_info pointers
   char               exec_path[4096];  // file name java
};

struct ps_prochandle {
   ps_prochandle_ops* ops;       // vtable ptr
   pid_t              pid;
   int                num_libs;
   lib_info*          libs;      // head of lib list
   lib_info*          lib_tail;  // tail of lib list - to append at the end
   int                num_threads;
   sa_thread_info*    threads;   // head of thread list
   struct core_data*  core;      // data only used for core dumps, NULL for process
};

int pathmap_open(const char* name);
void print_debug(const char* format,...);
void print_error(const char* format,...);
bool is_debug();

typedef bool (*thread_info_callback)(struct ps_prochandle* ph, lwpid_t lwpid);

// reads thread info using libthread_db and calls above callback for each thread
bool read_thread_info(struct ps_prochandle* ph, thread_info_callback cb);

// deletes a thread from the thread list
void delete_thread_info(struct ps_prochandle* ph, sa_thread_info* thr);

// adds a new shared object to lib list, returns NULL on failure
lib_info* add_lib_info(struct ps_prochandle* ph, const char* libname, uintptr_t base);

// adds a new shared object to lib list, supply open lib file descriptor as well
lib_info* add_lib_info_fd(struct ps_prochandle* ph, const char* libname, int fd,
                          uintptr_t base);

sa_thread_info* add_thread_info(struct ps_prochandle* ph, lwpid_t lwp_id);
// a test for ELF signature without using libelf

#ifdef __APPLE__
// a test for Mach-O signature
bool is_macho_file(int fd);
// skip fat head to get image start offset of cpu_type_t
// return false if any error happens, else value in offset.
bool get_arch_off(int fd, cpu_type_t cputype, off_t *offset);
#else
bool is_elf_file(int fd);
#endif // __APPLE__

lwpid_t get_lwp_id(struct ps_prochandle* ph, int index);
bool set_lwp_id(struct ps_prochandle* ph, int index, lwpid_t lwpid);
bool get_nth_lwp_regs(struct ps_prochandle* ph, int index, struct reg* regs);

// ps_pglobal_lookup() looks up the symbol sym_name in the symbol table
// of the load object object_name in the target process identified by ph.
// It returns the symbol's value as an address in the target process in
// *sym_addr.

ps_err_e ps_pglobal_lookup(struct ps_prochandle *ph, const char *object_name,
                    const char *sym_name, psaddr_t *sym_addr);

// read "size" bytes info "buf" from address "addr"
ps_err_e ps_pread(struct ps_prochandle *ph, psaddr_t  addr,
                  void *buf, size_t size);

// write "size" bytes of data to debuggee at address "addr"
ps_err_e ps_pwrite(struct ps_prochandle *ph, psaddr_t addr,
                   const void *buf, size_t size);

// fill in ptrace_lwpinfo for lid
ps_err_e ps_linfo(struct ps_prochandle *ph, lwpid_t lwp_id, void *linfo);

// needed for when libthread_db is compiled with TD_DEBUG defined
void ps_plog (const char *format, ...);

// untility, tells the position in file
off_t ltell(int fd);
#endif //_LIBPROC_IMPL_H_
