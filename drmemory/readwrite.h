/* **********************************************************
 * Copyright (c) 2010-2013 Google, Inc.  All rights reserved.
 * Copyright (c) 2008-2010 VMware, Inc.  All rights reserved.
 * **********************************************************/

/* Dr. Memory: the memory debugger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; 
 * version 2.1 of the License, and no later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/***************************************************************************
 * readwrite.h: Dr. Memory read/write instrumentation
 */

#ifndef _READWRITE_H_
#define _READWRITE_H_ 1

#include "fastpath.h"
#include "callstack.h" /* for app_loc_t */

/* there is no REG_EFLAGS so we use the REG_INVALID sentinel */
#define REG_EFLAGS REG_INVALID

/* we only need a little over 2 pages for whole_bb_spills_enabled(): could get
 * onto 2 pages by not emitting SPILL_REG_NONE.
 * -no_single_arg_slowpath needs only 10 pages.
 */
#define SHARED_SLOWPATH_SIZE (whole_bb_spills_enabled() ? PAGE_SIZE*11 : PAGE_SIZE*7)

void
instrument_init(void);

void
instrument_exit(void);

void
instrument_thread_init(void *drcontext);

void
instrument_thread_exit(void *drcontext);

byte *
generate_shared_slowpath(void *drcontext, instrlist_t *ilist, byte *pc);

void
update_stack_swap_threshold(void *drcontext, int new_threshold);

/* flags passed in to check_mem_opnd() and handle_mem_ref() */
enum {
    MEMREF_WRITE              = 0x001,
    MEMREF_PUSHPOP            = 0x002, /* the stack slot mem ref of push/pop */
    MEMREF_CHECK_DEFINEDNESS  = 0x004,
    MEMREF_USE_VALUES         = 0x008, /* for read, OUT; for write, IN */
    MEMREF_SINGLE_BYTE        = 0x010, /* keep using 1st byte in array */
    MEMREF_SINGLE_WORD        = 0x020, /* keep using 1st 2 bytes in array */
    MEMREF_SINGLE_DWORD       = 0x040, /* keep using 1st 4 bytes in array */
    MEMREF_MOVS               = 0x080, /* if a write, 1st entry in array holds base
                                        * of source shadow addr, which has already
                                        * been checked for addressability */
    MEMREF_CHECK_ADDRESSABLE  = 0x100, /* for pre-write */
    /* MEMREF_WRITE marks as defined, so for -no_check_uninitialized we
     * use MEMREF_CHECK_ADDRESSABLE for reads and writes.  This distinguishes.
     */
    MEMREF_IS_READ            = 0x200,
    /* i#556: for uninitialized random values passed as sizes to syscalls, we
     * don't want to walk huge sections of memory, so we stop checking after
     * the first erroneous word is found.
     */
    MEMREF_ABORT_AFTER_SIZE   = 4,
    MEMREF_ABORT_AFTER_UNADDR = 0x400,
};

#ifdef STATISTICS
/* per-opcode counts */
extern uint64 slowpath_count[OP_LAST+1];
extern uint64 slowpath_sz1;
extern uint64 slowpath_sz2;
extern uint64 slowpath_sz4;
extern uint64 slowpath_sz8;
extern uint64 slowpath_szOther;
/* FIXME: make generalized stats infrastructure */
extern uint slowpath_executions;
extern uint medpath_executions;
extern uint read_slowpath;
extern uint write_slowpath;
extern uint push_slowpath;
extern uint pop_slowpath;
extern uint read_fastpath;
extern uint write_fastpath;
extern uint push_fastpath;
extern uint pop_fastpath;
extern uint read4_fastpath;
extern uint write4_fastpath;
extern uint push4_fastpath;
extern uint pop4_fastpath;
extern uint slow_instead_of_fast;
extern uint heap_header_exception;
extern uint tls_exception;
extern uint alloca_exception;
extern uint strlen_exception;
extern uint strcpy_exception;
extern uint rawmemchr_exception;
extern uint strmem_unaddr_exception;
extern uint strrchr_exception;
extern uint andor_exception;
extern uint bitfield_const_exception;
extern uint bitfield_xor_exception;
extern uint loader_DRlib_exception;
extern uint cppexcept_DRlib_exception;
extern uint reg_dead;
extern uint reg_xchg;
extern uint reg_spill;
extern uint reg_spill_slow;
extern uint reg_spill_own;
extern uint reg_spill_used_in_bb;
extern uint reg_spill_unused_in_bb;
extern uint addressable_checks_elided;
extern uint aflags_saved_at_top;
extern uint num_faults;
extern uint num_slowpath_faults;
extern uint xl8_shared;
extern uint xl8_not_shared_reg_conflict;
extern uint xl8_not_shared_disp_too_big;
extern uint xl8_not_shared_unaligned;
extern uint xl8_not_shared_mem2mem;
extern uint xl8_not_shared_offs;
extern uint xl8_not_shared_slowpaths;
extern uint xl8_shared_slowpath_instrs;
extern uint xl8_shared_slowpath_count;
extern uint slowpath_unaligned;
extern uint slowpath_8_at_border;
extern uint alloc_stack_count;
extern uint delayed_free_bytes;
extern uint app_instrs_fastpath;
extern uint app_instrs_no_dup;
extern uint xl8_app_for_slowpath;
extern uint movs4_src_unaligned;
extern uint movs4_dst_unaligned;
extern uint movs4_src_undef;
extern uint movs4_med_fast;
extern uint cmps1_src_undef;
extern uint cmps1_med_fast;
extern uint num_bbs;
#endif

extern hashtable_t bb_table;

/* PR 493257: share shadow translation across multiple instrs */
extern hashtable_t xl8_sharing_table;

/* alloca handling in fastpath (i#91) */
extern hashtable_t ignore_unaddr_table;

extern reg_id_t seg_tls;

bool
opnd_uses_nonignorable_memory(opnd_t opnd);

bool
check_mem_opnd(uint opc, uint flags, app_loc_t *loc, opnd_t opnd, uint sz,
               dr_mcontext_t *mc, uint *shadow_vals);

bool
handle_mem_ref(uint flags, app_loc_t *loc, app_pc addr, size_t sz, dr_mcontext_t *mc,
               uint *shadow_vals);

bool
check_register_defined(void *drcontext, reg_id_t reg, app_loc_t *loc, size_t sz,
                       dr_mcontext_t *mc, instr_t *inst);

bool
is_in_gencode(byte *pc);

bool
event_restore_state(void *drcontext, bool restore_memory, dr_restore_state_info_t *info);

void
instrument_fragment_delete(void *drcontext, void *tag);

bool
instr_can_use_shared_slowpath(instr_t *inst, fastpath_info_t *mi);

void
instrument_slowpath(void *drcontext, instrlist_t *bb, instr_t *inst, fastpath_info_t *mi);

bool
slow_path_with_mc(void *drcontext, app_pc pc, app_pc decode_pc, dr_mcontext_t *mc);

size_t
instrument_persist_ro_size(void *drcontext, void *perscxt);

bool
instrument_persist_ro(void *drcontext, void *perscxt, file_t fd);

bool
instrument_resurrect_ro(void *drcontext, void *perscxt, byte **map INOUT);

void
bb_save_add_entry(app_pc key, bb_saved_info_t *save);

void
readwrite_module_load(void *drcontext, const module_data_t *mod, bool loaded);

void
readwrite_module_unload(void *drcontext, const module_data_t *mod);

/***************************************************************************
 * REGISTER SPILLING
 */

/* eflags eax and up-front save use this slot, and whole-bb spilling stores
 * eflags itself (lahf+seto) here
 */
#define SPILL_SLOT_EFLAGS_EAX SPILL_SLOT_3

int
spill_reg3_slot(bool eflags_dead, bool eax_dead, bool r1_dead, bool r2_dead);

void
spill_reg(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t reg,
          dr_spill_slot_t slot);

void
restore_reg(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t reg,
            dr_spill_slot_t slot);

opnd_t
spill_slot_opnd(void *drcontext, dr_spill_slot_t slot);

bool
is_spill_slot_opnd(void *drcontext, opnd_t op);

byte *
get_own_seg_base(void);

uint
num_own_spill_slots(void);

opnd_t
opnd_create_own_spill_slot(uint index);

ptr_uint_t
get_own_tls_value(uint index);

void
set_own_tls_value(uint index, ptr_uint_t val);

ptr_uint_t
get_thread_tls_value(void *drcontext, uint index);

void
set_thread_tls_value(void *drcontext, uint index, ptr_uint_t val);

ptr_uint_t
get_raw_tls_value(uint offset);

/***************************************************************************
 * ISA UTILITY ROUTINES
 */

#define REP_PREFIX    0xf3
#define REPNE_PREFIX  0xf2
#define MOVS_4_OPCODE 0xa5
#define CMPS_1_OPCODE 0xa6
#define LOOP_INSTR_OPCODE 0xe2
#define LOOP_INSTR_LENGTH 2
#define JNZ_SHORT_OPCODE    0x75
#define JNZ_SHORT_LENGTH    2
#define UD2A_LENGTH         2
#define CMP_OPCODE       0x80
#define CMP_BASE_IMM1_LENGTH  3
#define UD2A_OPCODE      0x0b0f
/* N.B.: other opcodes like ADD also use 0x81, and CMP with immed opnd may use 
 * other opcode value too. 
 */
#define CMP_IMMED_OPCODE 0x81
#define RET_NOIMM_OPCODE 0xc3

/* Avoid selfmod mangling for our "meta-instructions that can fault" (xref PR 472190).
 * Things would work without this (just lower performance, but on selfmod only)
 * except our short ctis don't reach w/ all the selfmod mangling: and we don't
 * have jmp_smart (i#56/PR 209710)!
 */
#define PREXL8M instrlist_meta_fault_preinsert

bool
reg_is_gpr(reg_id_t reg);

bool
reg_is_8bit(reg_id_t reg);

bool
reg_is_8bit_high(reg_id_t reg);

bool
reg_is_16bit(reg_id_t reg);

bool
reg_offs_in_dword(reg_id_t reg);

bool
opc_is_push(uint opc);

bool
opc_is_pop(uint opc);

bool
opc_is_stringop(uint opc);

bool
opc_is_stringop_loop(uint opc);

bool
opc_is_gpr_shift(uint opc);

bool
opc_is_jcc(uint opc);

bool
opc_is_cmovcc(uint opc);

bool
opc_is_fcmovcc(uint opc);

bool
opc_is_loopcc(uint opc);

/* can 2nd dst be treated as simply an extension of the 1st */
bool
opc_2nd_dst_is_extension(uint opc);

/* is xax is used after inst (including inst) */
bool
xax_is_used_subsequently(instr_t *inst);

uint
adjust_memop_push_offs(instr_t *inst);

opnd_t
adjust_memop(instr_t *inst, opnd_t opnd, bool write, uint *opsz, bool *pushpop_stackop);

bool
result_is_always_defined(instr_t *inst);

bool
always_check_definedness(instr_t *inst, int opnum);

bool
instr_check_definedness(instr_t *inst);

bool
instr_needs_all_srcs_and_vals(instr_t *inst);

int
num_true_srcs(instr_t *inst, dr_mcontext_t *mc);

int
num_true_dsts(instr_t *inst, dr_mcontext_t *mc);

#endif /* _READWRITE_H_ */
