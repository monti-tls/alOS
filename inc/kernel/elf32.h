/*
 * alOS
 * Copyright (C) 2015 Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ALOS_ELF32_H
#define ALOS_ELF32_H

#include <stdint.h>

/////////////////////////////////
//// ELF32 Types definitions ////
/////////////////////////////////

typedef uint32_t __attribute__((aligned(4))) elf32_addr;
typedef uint16_t __attribute__((aligned(2))) elf32_half;
typedef uint32_t __attribute__((aligned(4))) elf32_off;
typedef int32_t __attribute__((aligned(4))) elf32_sword;
typedef uint32_t __attribute__((aligned(4))) elf32_word;
typedef uint8_t elf32_char;

/////////////////////////////
//// ELF32 Global header ////
/////////////////////////////

//! elf32_header.e_ident size and byte offsets.
enum
{
    //!< Size of the e_ident field
    EI_NIDENT = 16,
    //!< First byte of the magic number
    EI_MAG0 = 0,
    //!< Last byte of the magic number
    EI_MAG3 = 3,
    //!< Class byte
    EI_CLASS = 4,
    //!< Data endianness byte
    EI_DATA = 5,
    //!< ELF Version byte
    EI_VERSION = 6,
    //!< Target OS ABI byte
    EI_OSABI = 7,
    //!< Target OS ABI version byte
    EI_ABIVERSION = 8
};

//! ELF32 magic number (e_ident[EI_MAG0 ... EI_MAG3]).
static elf32_char elf32_magic[] = "\x7f"
                                  "ELF";

//! elf32_header.e_ident values.
enum
{
    //!< The elf file is 32 bit
    EI_CLASS_32BIT = 0x01,
    //!< Data is in little-endian
    EI_DATA_LITTLE = 0x01,
};

//! elf32_header.e_type values.
enum
{
    //!< Invalid file type
    ET_NONE = 0x00,
    //!< Relocatable (object) file
    ET_REL = 0x01,
    //!< Executable file
    ET_EXEC = 0x02,
    //!< Shared object file
    ET_DYN = 0x03,
    //!< Core file
    ET_CORE = 0x04
};

//! elf32_header.e_machine values.
enum
{
    //!< ARM target machine
    EM_ARM = 0x28
};

//! elf32_header.e_version values.
enum
{
    //!< Invalid version
    EV_NONE = 0x00,
    //!< Current version
    EV_CURRENT = 0x01
};

//! elf32_header.e_ehsize values.
enum
{
    //!< Size in bytes of the elf32 header
    EEH_SIZE = 52
};

//! The ELF32 global header structure.
typedef struct __attribute__((packed))
{
    //!< Initial bytes mark
    elf32_char e_ident[EI_NIDENT];
    //!< Object file type (see ET_*)
    elf32_half e_type;
    //!< Target architecture (see EM_*)
    elf32_half e_machine;
    //!< Object file version (see EV_*)
    elf32_word e_version;
    //!< Virtual address of the object's entry point
    //!<   (if not applicable, holds 0)
    elf32_addr e_entry;
    //!< Program header table's file offset
    elf32_off e_phoff;
    //!< Section header table's file offset
    elf32_off e_shoff;
    //!< Target-dependent flags
    elf32_word e_flags;
    //!< This header's size (normally EEH_SIZE bytes)
    elf32_half e_ehsize;
    //!< Size of a program header table's entry
    elf32_half e_phentsize;
    //!< Number of entries in the program header table
    elf32_half e_phnum;
    //!< Size of a section header table's entry
    elf32_half e_shentsize;
    //!< Number of entries in the program section header table
    elf32_half e_shnum;
    //!< Index of the section header table that contains
    //!<   the section names
    elf32_half e_shstrndx;
} elf32_header;

///////////////////////////////
//// ELF32 Section headers ////
///////////////////////////////

//! ELF32 section header indexes special values
enum
{
    //!< Undefined or irrelevant index
    SHN_UNDEF = 0,
    //!< Symbols defined relative to this section
    //!<   are not affected by relocations and are
    //!<   absolutely defined
    SHN_ABS = 0xfff1,
    //!< Symbols defined relative to this section
    //!<   are common symbols
    SHN_COMMON = 0xfff2
};

//! elf32_shdr.sh_type values.
enum
{
    //!< Inactive section header entry
    SHT_NULL = 0x00,
    //!< Information defined by the program (such
    //!<   as code)
    SHT_PROGBITS = 0x01,
    //!< Link-time symbol table
    SHT_SYMTAB = 0x02,
    //!< String table section
    SHT_STRTAB = 0x03,
    //!< Relocation entries with explicit addends
    SHT_RELA = 0x04,
    //!< Symbol hash table
    SHT_HASH = 0x05,
    //!< Dynamic-linking information
    SHT_DYNAMIC = 0x06,
    //!< Note section
    SHT_NOTE = 0x07,
    //!< Same as SHT_PROGBITS, but occupies no file space
    SHT_NOBITS = 0x08,
    //!< Relocation entries without explicit addends
    SHT_REL = 0x09,
    //!< Unspecified semantics
    SHT_SHLIB = 0x0A,
    //!< Link-time symbol table
    SHT_DYNSYM = 0x0B
};

//! elf32_shdr.sh_flags values.
enum
{
    //!< Writable data during process execution
    SHF_WRITE = 0x01,
    //!< Occupies memory during execution of the program
    SHF_ALLOC = 0x02,
    //!< Contains executable machine instructions
    SHF_EXECINSTR = 0x04
};

//! The ELF32 section header structure.
typedef struct
{
    //!< Name of the section, as an index into
    //!<   the section header string
    elf32_word sh_name;
    //!< Section's contents and semantics (see SHT_*)
    elf32_word sh_type;
    //!< Miscellaneous attributes (see SHF_*)
    elf32_word sh_flags;
    //!< If the section appears in the memory
    //!<   image of a process, address at which
    //!<   the first byte of this section should
    //!<   reside
    elf32_addr sh_addr;
    //!< Byte offset from the beginning of the
    //!<   file to the first byte in this section
    elf32_off sh_offset;
    //!< Section size in bytes
    elf32_word sh_size;
    //!< Section-dependent section header table index link
    //!< For SHT_REL and SHT_RELA, the section header index
    //!<   of the associated symbol table
    elf32_word sh_link;
    //!< Section-dependent extra information
    //!< For SHT_REL and SHT_RELA, the section header index
    //!<   to which the relocation applies
    elf32_word sh_info;
    //!< Alignment constraints for the section
    //!<   (0 or 1 : no constraint)
    elf32_word sh_addralign;
    //!< If the section hold a table of fixed-size entries,
    //!<   holds the size in bytes of each entry (0 if N/A)
    elf32_word sh_entsize;
} elf32_shdr;

///////////////////////
//// ELF32 Symbols ////
///////////////////////

//! Get the bind attribute of a symbol.
#define ELF32_ST_BIND(st_info) ((st_info) >> 4)
//! Get the type attribute of a symbol.
#define ELF32_ST_TYPE(st_info) ((st_info)&0xf)

//! elf32_sym.info[bind] attributes values.
enum
{
    //!< Not visible outside this object file
    STB_LOCAL = 0x00,
    //!< Visible outside this object file
    STB_GLOBAL = 0x01,
    //!< Same as global, but w/ lower precedence
    STB_WEAK = 0x02
};

//! elf32_sym.info[type] attribute values.
enum
{
    //!< Not specified or invalid
    STT_NOTYPE = 0x00,
    //!< Associated with a data object
    STT_OBJECT = 0x01,
    //!< Associated with a function
    STT_FUNC = 0x02,
    //!< Associated with a section
    STT_SECTION = 0x03,
    //!< Associated with a file
    STT_FILE = 0x04
};

//! ELF32 Symbol table entry.
typedef struct
{
    //!< Index in the string symbol table
    //!<   for the symbol's name
    elf32_word st_name;
    //!< Value of the symbol, depending on
    //!<   the context
    //!< For st_shndx == SHN_COMMON, holds alignment
    //!<   constraints
    //!< For other values of st_shndx, holds the relative
    //!<   section offset
    elf32_addr st_value;
    //!< Size of the symbol (if N/A, holds 0)
    elf32_word st_size;
    //!< Symbol's type and binding attributes
    //!< See the ELF32_ST_* macros, STB_* and STT_*
    elf32_char st_info;
    //!< Holds 0
    elf32_char st_other;
    //!< Hold the section index from which this symbol
    //!<   is defined against
    elf32_half st_shndx;
} elf32_sym;

///////////////////////////
//// ELF32 Relocations ////
///////////////////////////

//! Get the symbol table index part of a relocation's
//!   info field.
#define ELF32_R_SYM(r_info) ((r_info) >> 8)
//! Get the type part of a relocation's info field.
#define ELF32_R_TYPE(r_info) ((elf32_char)(r_info))

enum
{
    //!< (S + A) | T
    R_ARM_ABS32 = 0x02,
    //!< ((S + A) | T) - P
    R_ARM_THM_CALL = 0x0A
};

//! An ELF32 relocation table entry.
typedef struct
{
    //!< The offset at which to apply the relocation
    //!<   action
    elf32_word r_offset;
    //!< Holds both the symbol table's index and the
    //!<   relocation type, see ELF32_R_* macros and
    //!<   R_ARM_*
    elf32_word r_info;
} elf32_rel;

#endif // ALOS_ELF32_H
