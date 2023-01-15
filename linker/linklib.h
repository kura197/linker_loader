#pragma once

#include <elf.h>
#include <vector>

#define IS_ELF(ehdr) (                   \
    ((ehdr).e_ident[EI_MAG0] == ELFMAG0) && \
    ((ehdr).e_ident[EI_MAG1] == ELFMAG1) && \
    ((ehdr).e_ident[EI_MAG2] == ELFMAG2) && \
    ((ehdr).e_ident[EI_MAG3] == ELFMAG3))

class Obj {
    public:
    char* address;
    char* filename;

    Obj(char* _address, char* _filename): address(_address), filename(_filename) {}

    Elf64_Ehdr* get_ehdr() { return (Elf64_Ehdr*)address; }
};

void elfdump(char* head);

bool check_ehdr(Elf64_Ehdr* ehdr);
void relocate_common_symbol(Elf64_Ehdr* ehdr);
char* get_section_name(Elf64_Ehdr* ehdr, Elf64_Shdr* shdr);
Elf64_Shdr* get_section(Elf64_Ehdr* ehdr, const char* sh_name);
std::vector<Elf64_Shdr*> get_shdrs(Elf64_Ehdr* ehdr);
Obj search_symbol(std::vector<Obj> objs, const char* name);