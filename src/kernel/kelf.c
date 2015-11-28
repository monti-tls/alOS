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

#include "kernel/kelf.h"
#include "kernel/elf32.h"
#include "kernel/kmalloc.h"
#include "kernel/ksymbols.h"
#include <string.h>

///////////////////////////
//// Module parameters ////
///////////////////////////

// N/A

////////////////////////////////
//// Module's sanity checks ////
////////////////////////////////

// N/A

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! An ELF32 binary blob representation
//!   for the kernel.
struct kelf
{
    union
    {
        //! A raw pointer to the beginning of
        //!   the elf blob
        void* raw;
        //! A pointer to the header of the elf blob
        elf32_header* header;
    };

    //! The section header string table header
    elf32_shdr* shstrtab;
    //! The symbol table header
    elf32_shdr* symtab;
    //! The symbol's name string table header
    elf32_shdr* symstrtab;

    //! An array containing all sections header
    //!   indexes with the SHF_ALLOC flag set,
    //!   thus directly contributing to the process image
    elf32_word* allocsh;
    //! Size of the allocsh array
    elf32_word allocshnum;

    //! Start address of sections in the program's
    //!   memory image
    elf32_off* progmem_shoff;

    //! Size in bytes of the program memory image
    elf32_word progmem_size;
    //! Program's memory image
    char* progmem;
};

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

static int header_check(struct kelf* elf);
static int find_shstrtab(struct kelf* elf);
static int find_symtab(struct kelf* elf);
static int find_symstrtab(struct kelf* elf);
static int find_allocsh(struct kelf* elf);
static elf32_shdr* section(struct kelf* elf, elf32_word id);
static const char* section_name(struct kelf* elf, elf32_shdr* shdr);
static elf32_sym* symbol(struct kelf* elf, elf32_word id);
static const char* symbol_name(struct kelf* elf, elf32_sym* sym);
static int alloc_progmem(struct kelf* elf);
static int load_progmem_section(struct kelf* elf, elf32_word id);

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

// N/A

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! Check the header of an elf binary blob.
//! \param elf The elf blob to check
//! \return 0 if header is correct, -1 otherwise
static int header_check(struct kelf* elf)
{
    // Magic check
    for(int i = EI_MAG0; i <= EI_MAG3; ++i)
        if(elf->header->e_ident[i] != elf32_magic[i - EI_MAG0])
            return -1;

    // Type check
    if(elf->header->e_type != ET_REL)
        return -1;

    // Machine check
    if(elf->header->e_machine != EM_ARM)
        return -1;

    // Version check
    if(elf->header->e_version != EV_CURRENT)
        return -1;

    // Consistency size check
    if(elf->header->e_ehsize != EEH_SIZE)
        return -1;

    return 0;
}

//! Find and store the section header name table's header.
//! \param The elf blob to work on
//! \return the section header's index if succeeded,
//!         SHN_UNDEF upon failure
static int find_shstrtab(struct kelf* elf)
{
    if(!elf)
        return SHN_UNDEF;

    // Get the string section header index
    if(elf->header->e_shstrndx == SHN_UNDEF)
        return SHN_UNDEF;

    elf->shstrtab = section(elf, elf->header->e_shstrndx);
    if(!elf->shstrtab)
        return SHN_UNDEF;

    return elf->header->e_shstrndx;
}

//! Find and store the symbol table's index.
//! \param elf The elf blob to work on
//! \return the section header's index on success,
//!         SHN_UNDEF otherwise
static int find_symtab(struct kelf* elf)
{
    if(!elf)
        return SHN_UNDEF;

    for(int i = 0; i < elf->header->e_shnum; ++i)
    {
        elf32_shdr* shdr = section(elf, i);
        if(!shdr)
            break;

        if(shdr->sh_type == SHT_SYMTAB)
        {
            elf->symtab = shdr;
            return i;
        }
    }

    return SHN_UNDEF;
}

//! Find and store the symbol string table's header.
//! \param elf The elf blob to work on
//! \return the section header's index on success,
//!         SHN_UNDEF otherwise
static int find_symstrtab(struct kelf* elf)
{
    if(!elf || !elf->symtab)
        return SHN_UNDEF;

    elf->symstrtab = section(elf, elf->symtab->sh_link);
    if(!elf->symstrtab)
        return -1;

    return elf->symtab->sh_link;
}

//! Find and store all sections presenting the
//!   SHF_ALLOC flag
//! \param elf The elf blob to work on
//! \return The number of found sections, -1 on error
static int find_allocsh(struct kelf* elf)
{
    if(!elf)
        return -1;

    // Get the count of sections
    elf32_word count = 0;
    for(elf32_word i = 0; i < elf->header->e_shnum; ++i)
    {
        elf32_shdr* shdr = section(elf, i);
        if(!shdr)
            return -1;

        if(shdr->sh_flags & SHF_ALLOC)
            ++count;
    }

    if(!count)
        return 0;

    // Allocate the array
    elf->allocshnum = count;
    elf->allocsh = kmalloc(elf->allocshnum * sizeof(elf32_word));
    if(!elf->allocsh)
        return -1;

    // Fill the array
    count = 0;
    for(elf32_word i = 0; i < elf->header->e_shnum; ++i)
    {
        elf32_shdr* shdr = section(elf, i);
        if(!shdr)
            return -1;

        if(shdr->sh_flags & SHF_ALLOC)
            elf->allocsh[count++] = i;
    }

    return elf->allocshnum;
}

//! Get the id-th section header.
//! \param elf The elf blob to work on
//! \param id The identifier of the section header to retrieve
//! \return A pointer to the section header, 0 if error(s) occured
static elf32_shdr* section(struct kelf* elf, elf32_word id)
{
    if(!elf || id >= elf->header->e_shnum)
        return 0;

    void* shbase = elf->raw + elf->header->e_shoff;
    void* section = shbase + id * elf->header->e_shentsize;

    return (elf32_shdr*)section;
}

//! Get a section name from the string table section.
//! \param elf The elf blob to work on
//! \param shdr The section's header
//! \return The address of the string, 0 upon failure
static const char* section_name(struct kelf* elf, elf32_shdr* shdr)
{
    if(!elf || !elf->shstrtab)
        return 0;

    return (const char*)(elf->raw + elf->shstrtab->sh_offset + shdr->sh_name);
}

//! Get a symbol entry from the symbol table.
//! \param elf The elf blob to work on
//! \param id The symbol's identifier
//! \return A pointer to the symbol, 0 upon failure
static elf32_sym* symbol(struct kelf* elf, elf32_word id)
{
    if(!elf || !elf->symtab)
        return 0;

    return (elf32_sym*)(elf->raw + elf->symtab->sh_offset + id * elf->symtab->sh_entsize);
}

//! Get a symbol's name.
//! \param elf The elf blob to work on
//! \param sym The symbol from which to retrieve the name
//! \return The symbol's name, 0 if error(s) occured
static const char* symbol_name(struct kelf* elf, elf32_sym* sym)
{
    if(!elf || !sym || !elf->symstrtab)
        return 0;

    elf32_word st_type = ELF32_ST_TYPE(sym->st_info);

    if(st_type == STT_SECTION)
    {
        elf32_shdr* shdr = section(elf, sym->st_shndx);
        return section_name(elf, shdr);
    }
    else
    {
        if(sym->st_name > elf->symstrtab->sh_size)
            return 0;

        return (const char*)(elf->raw + elf->symstrtab->sh_offset + sym->st_name);
    }

    return 0;
}

//! Allocate memory for the program's memory image, and
//!   compute the loaded sections' offsets
//! \param elf The elf blob to work on
//! \return 0 if succeeded, -1 if failed
static int alloc_progmem(struct kelf* elf)
{
    if(!elf || !elf->allocsh)
        return -1;

    // Allocate the section offsets table
    elf->progmem_shoff = kmalloc(elf->allocshnum * sizeof(elf32_off));
    if(!elf->progmem_shoff)
        return -1;

    // Compute properly aligned section offsets
    elf32_off off = 0;
    for(elf32_word i = 0; i < elf->allocshnum; ++i)
    {
        // Get the required alignment for this section
        elf32_shdr* shdr = section(elf, elf->allocsh[i]);
        if(!shdr)
            return -1;
        elf32_word align = shdr->sh_addralign;

        // If the required alignment is not even
        //   ensured by kmalloc, fuck off
        if(align > KMALLOC_ALIGNMENT)
            return -1;

        // Properly align initial address (we know 0 offset
        //   is already aligned by kmalloc)
        elf32_off r = align ? off % align : 0;
        off = r ? off + (align - r) : off;

        // Save the computed offset
        elf->progmem_shoff[i] = off;

        // Increment by the size of the section,
        //   after the last iteration off contains the
        //   total required size for the program's memory
        //   image
        off += shdr->sh_size;
    }

    // Allocate the required amout of program memory
    elf->progmem_size = off;
    elf->progmem = kmalloc(elf->progmem_size);
    if(!elf->progmem)
        return -1;

    return 0;
}

//! Load a specific section into the program memory
//! \param elf The elf blob to work on
//! \param id The identifier of the section to initialize,
//!           *relative* to the elf->allocsh array
//! \return 0 if succeeded, -1 if failed
static int load_progmem_section(struct kelf* elf, elf32_word id)
{
    if(!elf || !elf->progmem || id >= elf->allocshnum)
        return -1;

    elf32_shdr* shdr = section(elf, elf->allocsh[id]);
    if(!shdr)
        return -1;
    elf32_off off = elf->progmem_shoff[id];

    // Usually this will be the .bss section, those
    //   are meant to be initialized with zeroes
    if(shdr->sh_type == SHT_NOBITS)
    {
        for(elf32_off i = 0; i < shdr->sh_size; ++i)
        {
            elf->progmem[off + i] = 0;
        }
    }
    // Those are .text, .data and .rodata sections
    // Here we don't mind about read-only sections
    else if(shdr->sh_type == SHT_PROGBITS)
    {
        char* sdata = (char*)(elf->raw + shdr->sh_offset);

        for(elf32_off i = 0; i < shdr->sh_size; ++i)
        {
            elf->progmem[off + i] = sdata[i];
        }
    }
    // Those should not be here !
    else
    {
        return -1;
    }

    return 0;
}

//! Load all sections from elf->allocsh in program memory
//! \param elf The elf blob to work on
//! \return 0 on success, -1 upon failure
static int load_progmem(struct kelf* elf)
{
    if(!elf || !elf->progmem)
        return -1;

    for(elf32_word i = 0; i < elf->allocshnum; ++i)
    {
        if(load_progmem_section(elf, i) < 0)
            return -1;
    }

    return 0;
}

//! Get the address of a given symbol in program's memory image
//! \param elf The elf blob to work on
//! \param id The id of the symbol to retrieve
//! \return The address of the symbol if found, 0 instead
static elf32_addr symbol_addr(struct kelf* elf, elf32_sym* sym)
{
    if(!elf || !sym || !elf->progmem)
        return 0;

    // Get symbol type
    elf32_word st_type = ELF32_ST_TYPE(sym->st_info);

    // Get the symbol's section address
    elf32_addr st_shaddr = 0x00;
    for(elf32_word i = 0; i < elf->allocshnum; ++i)
    {
        if(elf->allocsh[i] == sym->st_shndx)
            st_shaddr = ((elf32_addr)elf->progmem) + elf->progmem_shoff[i];
    }

    // S = symbol value
    elf32_addr S;

    // For data or code symbols, S = section offset + value
    if(st_type == STT_OBJECT || st_type == STT_FUNC)
    {
        if(!st_shaddr)
            return 0;

        S = (st_shaddr + sym->st_value) & ~0x01;
    }
    // For sections, S = section_offset
    else if(st_type == STT_SECTION)
    {
        if(!st_shaddr)
            return 0;

        S = st_shaddr & ~0x01;
    }
    // For other symbols (externs, ...), attempt to resolve them
    //   them from kernel symbols
    else if(st_type == STT_NOTYPE)
    {
        const char* name = symbol_name(elf, sym);
        if(!name)
            return 0;

        return (elf32_addr)ksymbol(name);
    }

    return S;
}

//! Apply a relocation to the process' image
//! \param elf The elf blob to work on
//! \param shdr The relocation's section header
//! \param rel The relocation to apply
//! \return 0 on success, -1 otherwise
static int do_rel_for_section(struct kelf* elf, elf32_shdr* shdr, elf32_rel* rel)
{
    if(!elf || !shdr || shdr->sh_type != SHT_REL || !elf->progmem || !rel)
        return -1;

    // Get relocation parameters
    elf32_word r_sym = ELF32_R_SYM(rel->r_info);
    elf32_word r_type = ELF32_R_TYPE(rel->r_info);

    // Get the relocation's section address
    elf32_addr r_shaddr = 0x00;
    for(elf32_word i = 0; i < elf->allocshnum; ++i)
    {
        if(elf->allocsh[i] == shdr->sh_info)
            r_shaddr = ((elf32_addr)elf->progmem) + elf->progmem_shoff[i];
    }

    elf32_sym* sym = symbol(elf, r_sym);
    if(!sym)
        return -1;

    elf32_word T = ELF32_ST_TYPE(sym->st_info) == STT_FUNC ? 0x01 : 0x00;

    elf32_addr S = symbol_addr(elf, sym);
    if(!S)
        return -1;

    // Location to relocate
    elf32_word* P = (elf32_word*)(r_shaddr + rel->r_offset);

    // Simple 32-bit word relocation
    if(r_type == R_ARM_ABS32)
    {
        elf32_word A = *P;
        *P = (S + A) | T;
    }
    // Thumb call relocation
    else if(r_type == R_ARM_THM_CALL)
    {
        elf32_half upper_insn = ((elf32_half*)P)[0];
        elf32_half lower_insn = ((elf32_half*)P)[1];

        elf32_word s = (upper_insn >> 10) & 1;
        elf32_word j1 = (lower_insn >> 13) & 1;
        elf32_word j2 = (lower_insn >> 11) & 1;

        elf32_sword off = (s << 24) | ((~(j1 ^ s) & 1) << 23) | ((~(j2 ^ s) & 1) << 22) |
                          ((upper_insn & 0x03ff) << 12) | ((lower_insn & 0x07ff) << 1);

        if(off & 0x01000000)
            off -= 0x02000000;

        off += S - (elf32_word)P;

        s = (off >> 24) & 1;
        j1 = s ^ (~(off >> 23) & 1);
        j2 = s ^ (~(off >> 22) & 1);

        upper_insn = ((upper_insn & 0xf800) | (s << 10) | ((off >> 12) & 0x03ff));
        lower_insn = ((lower_insn & 0xd000) | (j1 << 13) | (j1 << 11) | ((off >> 1) & 0x07ff));

        ((elf32_half*)P)[0] = upper_insn;
        ((elf32_half*)P)[1] = lower_insn;
    }
    else
        return -1;

    return 0;
}

//! Apply all relocations for a given relocation section
//! \param elf The elf blob to work on
//! \param shdr The relocation section header
//! \return 0 if suceeded, -1 otherwise
static int do_rels_for_section(struct kelf* elf, elf32_shdr* shdr)
{
    if(!elf || !shdr || shdr->sh_type != SHT_REL || !elf->progmem)
        return -1;

    for(elf32_word i = 0; i < shdr->sh_size / shdr->sh_entsize; ++i)
    {
        elf32_rel* rel = (elf32_rel*)(elf->raw + shdr->sh_offset + i * shdr->sh_entsize);

        if(do_rel_for_section(elf, shdr, rel) < 0)
            return -1;
    }

    return 0;
}

//! Apply all relocations for the given elf blob
//! \param elf The elf blob to work on
//! \return 0 on success, -1 otherwise
static int do_rels(struct kelf* elf)
{
    if(!elf || !elf->progmem)
        return -1;

    for(elf32_word i = 0; i < elf->header->e_shnum; ++i)
    {
        elf32_shdr* shdr = section(elf, i);
        if(shdr->sh_type != SHT_REL)
            continue;

        if(do_rels_for_section(elf, shdr) < 0)
            return -1;
    }

    return 0;
}

//! Perform the whole elf loading process :
//!   - check it for defects
//!   - find relevant sections
//!   - allocate process image
//!   - apply relocations
//! \param elf The elf blob to load
//! \return 0 on success, -1 otherwise
static int load(struct kelf* elf)
{
    if(!elf)
        return -1;

    if(header_check(elf) < 0)
        return -1;
    if(find_shstrtab(elf) == SHN_UNDEF)
        return -1;
    if(find_symtab(elf) == SHN_UNDEF)
        return -1;
    if(find_symstrtab(elf) == SHN_UNDEF)
        return -1;
    if(find_allocsh(elf) < 0)
        return -1;
    if(alloc_progmem(elf) < 0)
        return -1;
    if(load_progmem(elf) < 0)
        return -1;
    if(do_rels(elf) < 0)
        return -1;

    return 0;
}

//! Unload an elf blob from memory (free its internal
//!   resources)
//! \param elf The elf blob to unload
//! \return 0 if suceeded, -1 otherwise
static int unload(struct kelf* elf)
{
    if(!elf)
        return -1;

    kfree(elf->progmem);
    kfree(elf->progmem_shoff);
    kfree(elf->allocsh);

    elf->shstrtab = 0;
    elf->symtab = 0;
    elf->symstrtab = 0;
    elf->allocsh = 0;
    elf->allocshnum = 0;
    elf->progmem_shoff = 0;
    elf->progmem_size = 0;
    elf->progmem = 0;

    return 0;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

struct kelf* kelf_load(void* raw)
{
    struct kelf* elf = kmalloc(sizeof(struct kelf));
    elf->raw = raw;

    if(load(elf) < 0)
    {
        kfree(elf->raw);
        kfree(elf);
        return 0;
    }

    return elf;
}

void kelf_unload(struct kelf* elf)
{
    if(!elf)
        return;

    unload(elf);

    kfree(elf);
}

void* kelf_symbol(struct kelf* elf, const char* name)
{
    if(!elf || !name)
        return 0;

    for(elf32_word i = 0; i < elf->symtab->sh_size / elf->symtab->sh_entsize; ++i)
    {
        elf32_sym* sym = symbol(elf, i);
        if(!sym)
            return 0;

        if(strcmp(symbol_name(elf, sym), name) == 0)
        {
            elf32_word addr = symbol_addr(elf, sym);
            if(addr)
            {
                if(ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
                    addr |= 1;
            }
            return (void*)addr;
        }
    }

    return 0;
}
