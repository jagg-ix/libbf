#ifndef BF_INSN_H
#define BF_INSN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "binary_file.h"
#include "bf_basic_blk.h"
#include "include/htable.h"

struct bf_insn_part {
	struct list_head list;
	char *		 str;
};

/*
 * Our abstraction of an instruction. A bf_insn consists of a list of parts
 * which hold the mnemonic and operands.
 */
struct bf_insn {
	/*
	 * Holds the address of the instruction.
	 */
	bfd_vma		      vma;

	/*
	 * Start of linked list of parts.
	 */
	struct list_head      part_list;

	/*
	 * Entry into the hashtable of binary_file.
	 */
	struct htable_entry   entry;

	/*
	 * The basic block containing the instruction.
	 */
	struct bf_basic_blk * bb;
};

/*
 * Returns a bf_insn object. bf_close_insn must be called to allow the object
 * to properly clean up.
 */
extern struct bf_insn * bf_init_insn(struct bf_basic_blk *, bfd_vma);

/*
 * Adds to the tail of the part list.
 */
extern void bf_add_insn_part(struct bf_insn *, char *);

/*
 * Prints the bf_insn to stdout.
 */
extern void bf_print_insn(struct bf_insn *);

/*
 * Prints the bf_insn to a FILE.
 */
extern void bf_print_insn_dot(FILE *, struct bf_insn *);

/*
 * Closes a bf_insn obtained from calling bf_init_insn.
 */
extern void bf_close_insn(struct bf_insn *);

/*
 * Adds a basic block to the insn_table of binary_file.
 */
extern void bf_add_insn(struct binary_file *, struct bf_insn *);

/*
 * Gets instruction for the starting VMA.
 */
extern struct bf_insn * bf_get_insn(struct binary_file *, bfd_vma);

/*
 * Checks for the existence of an instruction in the insn_table of binary_file.
 */
extern bool bf_exists_insn(struct binary_file *, bfd_vma);

/*
 * Releases memory for all instructions currently stored.
 */
extern void bf_close_insn_table(struct binary_file *);

#ifdef __cplusplus
}
#endif

#endif
