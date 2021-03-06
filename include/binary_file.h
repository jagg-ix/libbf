/*
 * This file is part of libbf.
 *
 * libbf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libbf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with libbf.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file binary_file.h
 * @brief Definition and API of bin_file.
 * @details bin_file is the file abstraction provided by <b>libbf</b>. A
 * typical workflow with <b>libbf</b> is to initiate a bin_file object with
 * load_bin_file(), perform CFG generations and finally clean up with
 * close_bin_file(). An API will eventually be added to allow injection of
 * foreign code and patching of the original code.
 * @author Mike Kwan <michael.kwan08@imperial.ac.uk>
 */

#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bfd.h>
#include <dis-asm.h>
#include <libelf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <libiberty.h>

#include <libkern/htable.h>

#include "symbol.h"

#define IS_BF_ARCH_32(BF) (BF->bitiness == arch_32)

/**
 * @enum arch_bitiness
 * @brief Enumeration of architecture bitiness.
 * @details Since we support x86-32 and x86-64, having two members is
 * sufficient.
 */
enum arch_bitiness {
  /**
   * Enum value for x86-64.
   */
  arch_64,
  /**
   * Enum value for x86-32.
   */
  arch_32
};

/**
 * @enum insn_part_type
 * @brief Enumeration of the different instruction parts we expect.
 * @details The disassembler engine sets a combination of expected part types
 * as it disassembles. If it receives a type it was not expecting, it can
 * output this.
 */
enum insn_part_type {
  /**
   * Enum value for mnemonic.
   */
  insn_part_mnemonic = 1,
  /**
   * Enum value for secondary mnemonic.
   */
  insn_part_secondary_mnemonic = 2,
  /**
   * Enum value for operand.
   */
  insn_part_operand = 4,
  /**
   * Enum value for comma.
   */
  insn_part_comma = 8,
  /**
   * Enum value for comment indicator.
   */
  insn_part_comment_indicator = 16,
  /**
   * Enum value for comment contents.
   */
  insn_part_comment_contents = 32
};

/**
 * @internal
 * @struct disasm_context
 * @brief Internal context used by disassembler.
 * @details Allows us to pass in extra information to our custom fprintf
 * function.
 */
struct disasm_context {
  /**
   * @internal
   * @var insn
   * @brief bf_insn to hold information about the instruction being
   * disassembled.
   */
  struct bf_insn * insn;

  /**
   * @internal
   * @var part_counter
   * @brief A counter for how many times the fprintf function has been
   * called for the current instruction. Should be initialised to 0
   * before disassembly of each instruction.
   */
  int part_counter;

  /**
   * @internal
   * @var part_types_expected
   * @brief Holds a combination of the insn_part_type flags. Should be
   * initialised to insn_part_mnemonic before disassembly of each
   * instruction.
   */
  int part_types_expected;
};

/**
 * @struct bin_file
 * @brief The abstraction used for a binary file.
 * @details This structure encapsulates the information necessary to use
 * <b>libbf</b>. Primarily this is our way of wrapping and abstracting away
 * from BFD.
 */
struct bin_file {
  /**
   * @var abfd
   * @brief Wrapping the BFD object.
   * @note This is defined in bfd.h in the binutils distribution.
   */
  struct bfd * abfd;

  /**
   * @var output_path
   * @brief The filepath of the output.
   */
  char *       output_path;

  /**
   * @var bitiness
   * @brief Flag denoting the bitiness of the target.
   */
  enum arch_bitiness bitiness;

  /**
   * @internal
   * @var disassembler
   * @brief Holds the disassembler corresponding to the BFD object.
   * @note This is defined in dis-asm.h in the binutils distribution.
   */
  disassembler_ftype disassembler;

  /**
   * @internal
   * @var disasm_config
   * @brief Holds the configuration used by <b>libopcodes</b> for
   * disassembly.
   * @note This is defined in dis-asm.h in the binutils distribution.
   */
  struct disassemble_info disasm_config;

  /**
   * @internal
   * @var func_table
   * @brief Hashtable holding list of all the currently discovered bf_func
   * objects.
   * @details The implementation is that the address of a function is
   * its key.
   */
  struct htable func_table;

  /**
   * @internal
   * @var bb_table
   * @brief Hashtable holding list of all the currently discovered
   * bf_basic_blk objects.
   * @details The implementation is that the address of a basic block
   * is its key.
   */
  struct htable bb_table;

  /**
   * @internal
   * @var insn_table
   * @brief Hashtable holding list of all currently discovered bf_insn
   * objects.
   * @details The implementation is that the address of a instruction is
   * its key.
   */
  struct htable insn_table;

  /**
   * @internal
   * @var sym_table
   * @brief Hashtable holding list of all discovered bf_sym objects.
   * @details The implementation is that the address of a symbol is its
   * key.
   */
  struct symbol_table sym_table;

  /**
   * @internal
   * @var mem_table
   * @brief Hashtable holding mappings of sections mapped into memory by
   * the memory manager.
   * @details The implementation is that the address of a section is its
   * key.
   */
  struct htable mem_table;

  /**
   * @internal
   * @var context
   * @brief Internal disassembly state.
   */
  struct disasm_context context;
};

/**
 * @brief Loads a bin_file object.
 * @param target_path The location of the target to be loaded.
 * @param output_path The location of the output bin_file. Any changes made
 * by <b>libbf</b> will modify this file. If output_path is NULL,
 * <b>libbf</b> will directly modify the file pointed to by target_path.
 * @return NULL if a matching BFD backend could not be found. A bin_file
 * object associated with the target otherwise.
 * @note close_bin_file() must be called to allow the object to properly
 * clean up.
 */
extern struct bin_file * load_bin_file(char * target_path,
    char * output_path);

/**
 * @brief Closes a bin_file object.
 * @param bf The bin_file to be closed.
 * @return Returns TRUE if the close occurred successfully, FALSE otherwise.
 */
extern bool close_bin_file(struct bin_file * bf);

/**
 * @brief Builds a Control Flow Graph (CFG) using the entry point as the root.
 * @param bf The bin_file being analysed.
 * @return The first basic block of the generated CFG.
 * @details The bin_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once.
 */
extern struct bf_basic_blk * disasm_bin_file_entry(struct bin_file * bf);

/**
 * @brief Builds a Control Flow Graph (CFG) using the address of the symbol as
 * the root.
 * @param bf The bin_file being analysed.
 * @param sym The symbol to start analysis from. This can be obtained using 
 * bf_for_each_sym()
 * @param is_func A bool specifying whether the address of sym should be
 * treated as the start of a function.
 * @return The first basic block of the generated CFG.
 * @details The bin_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once. The reason is_func is required is because there is no
 * reliable heuristic to detect whether a bf_basic_blk represents the start of
 * a function other than it being a call target. Since we can not analyse
 * backwards, we need to be instructed how the root should be treated.
 */
extern struct bf_basic_blk * disasm_bin_file_sym(struct bin_file * bf,
		struct symbol * sym, bool is_func);

/**
 * @brief Builds a Control Flow Graph (CFG) sequentially disassembling every
 * symbol representing a function.
 * @param bf The bin_file being analysed.
 * @details The bin_file backend keeps track of all previously analysed
 * instructions. This means there is no need to generate a CFG from the same
 * root more than once.
 */
extern void disasm_all_func_sym(struct bin_file * bf);

#ifdef __cplusplus
}
#endif

#endif
