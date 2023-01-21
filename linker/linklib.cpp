#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "linklib.h"

bool check_ehdr(Elf64_Ehdr* ehdr) {
    if (!IS_ELF(*ehdr)) {
        fprintf(stderr, "This is not ELF file.\n");
        return false;
    }
    // TODO: add other checker.
    return true;
}

void relocate_common_symbol(Elf64_Ehdr* ehdr) {
    auto bss = get_section(ehdr, ".bss");
    if (bss == nullptr) {
        fprintf(stderr, "cannnot find .bss section\n");
        return;
    }
    char* head = (char*)ehdr;
    auto shdrs = get_shdrs(ehdr);

    // get index of .bss section
    auto get_bss_idx = [&] () {
        for (unsigned int i = 0; i < shdrs.size(); i++) {
            if (shdrs[i] == bss) return (int)i;
        }
        return -1;
    };
    const int bss_idx = get_bss_idx();

    int bss_size = bss->sh_size;
    for (auto& shdr: shdrs) {
        if (shdr->sh_type != SHT_SYMTAB && shdr->sh_type != SHT_DYNSYM) continue;
        for (unsigned int i = 0; i < shdr->sh_size / shdr->sh_entsize; i++) {
            Elf64_Sym* sym = (Elf64_Sym*)(head + shdr->sh_offset + shdr->sh_entsize * i);
            if (sym->st_shndx != SHN_COMMON) continue;
            char* tgt_name = (char*)(head + shdrs[shdr->sh_link]->sh_offset + sym->st_name);
            printf("find COMMON symbol %s\n", tgt_name);

            bss_size = (bss_size + sym->st_value - 1) & ~(sym->st_value - 1);
            sym->st_value = bss_size;
            sym->st_shndx = bss_idx;
        }
    }

    bss_size = (bss_size + 15) & ~15;
    bss->sh_size = bss_size;
}

char* get_section_name(Elf64_Ehdr* ehdr, Elf64_Shdr* shdr) {
    char* head = (char*)ehdr;
    Elf64_Shdr *shstr = (Elf64_Shdr*)(head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
    char* name = (char*)(head + shstr->sh_offset + shdr->sh_name);
    return name;
}

Elf64_Shdr* get_section(Elf64_Ehdr* ehdr, const char* sh_name) {
    char* head = (char*)ehdr;
    for (int i = 0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr* shdr = (Elf64_Shdr*)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
        char* name = get_section_name(ehdr, shdr);
        if (!strcmp(name, sh_name)) return shdr;
    }
    return nullptr;
}

std::vector<Elf64_Shdr*> get_shdrs(Elf64_Ehdr* ehdr) {
    char* head = (char*)ehdr;
    std::vector<Elf64_Shdr*> shdrs(ehdr->e_shnum);
    for (int i = 0; i < ehdr->e_shnum; i++) {
        shdrs[i] = (Elf64_Shdr*)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
    }
    return shdrs;
}

Obj search_symbol(const std::vector<Obj>& objs, const char* name) {
    for (auto& obj: objs) {
        char* head = (char*)obj.address;
        Elf64_Shdr* symtab = get_section((Elf64_Ehdr*)obj.address, ".symtab");
        if (symtab == nullptr) continue;
        for (unsigned int i = 0; i < symtab->sh_size / symtab->sh_entsize; i++) {
            Elf64_Sym* sym = (Elf64_Sym*)(head + symtab->sh_offset + symtab->sh_entsize * i);
            if (sym->st_shndx == SHN_UNDEF) continue;
            if (sym->st_shndx == SHN_COMMON) continue;
            if (!sym->st_name) continue;
            auto shdrs = get_shdrs((Elf64_Ehdr*)obj.address);
            const char* sym_name = (const char*)(head + shdrs[symtab->sh_link]->sh_offset + sym->st_name);
            if (!strcmp(name, sym_name)) {
                char* addr = (char*)(head + shdrs[sym->st_shndx]->sh_offset + sym->st_value);
                Obj ret(addr, obj.filename);
                return ret;
            }
        }
    }

    Obj ret(nullptr, nullptr);
    return ret;
}

void link_objs(const std::vector<Obj>& objs) {
    for (auto& obj: objs) {
        printf("Fileanme: %s\n", obj.filename);
        char* head = (char*)obj.address;
        Elf64_Ehdr* ehdr = (Elf64_Ehdr*)head;
        auto shdrs = get_shdrs(ehdr);
        for (auto& shdr: shdrs) {
            if (shdr->sh_type != SHT_REL && shdr->sh_type != SHT_RELA) continue;
            Elf64_Shdr* symtab = shdrs[shdr->sh_link];
            for (unsigned int i = 0; i < shdr->sh_size / shdr->sh_entsize; i++) {
                Elf64_Rela* rela = (Elf64_Rela*)(head + shdr->sh_offset + shdr->sh_entsize * i);
                Elf64_Sym* sym = (Elf64_Sym*)(head + symtab->sh_offset + symtab->sh_entsize * ELF64_R_SYM(rela->r_info));
                char* tgt_name = nullptr;
                char* tgt_addr = nullptr;
                char* sym_addr = nullptr;
                if (sym->st_shndx == SHN_UNDEF) {
                    tgt_name = (char*)(head + shdrs[symtab->sh_link]->sh_offset + sym->st_name);
                    auto obj = search_symbol(objs, tgt_name);
                    if (obj.address == nullptr) continue;
                    sym_addr = obj.address;
                    tgt_addr = head + shdrs[shdr->sh_info]->sh_offset + rela->r_offset;
                } else if (sym->st_shndx == SHN_ABS || sym->st_shndx == SHN_COMMON) {
                    continue;
                } else {
                    auto tgt_shdr = shdrs[sym->st_shndx];
                    tgt_name = get_section_name(ehdr, tgt_shdr);
                    sym_addr = head + tgt_shdr->sh_offset + sym->st_value;
                    tgt_addr = head + shdrs[shdr->sh_info]->sh_offset + rela->r_offset;
                }

                if (tgt_name == nullptr || tgt_addr == nullptr || sym_addr == nullptr) continue;

                auto type = ELF64_R_TYPE(rela->r_info);
                if (type == R_X86_64_PC32) {
                    int wr_addr = sym_addr - tgt_addr + rela->r_addend;
                    memcpy(tgt_addr, &wr_addr, 4);
                    printf("relocate %s at address 0x%llx point to 0x%llx\n", tgt_name, (unsigned long long)tgt_addr, (unsigned long long)sym_addr);
                } else if (type == R_X86_64_PLT32) {
                    int wr_addr = sym_addr - tgt_addr + rela->r_addend;
                    memcpy(tgt_addr, &wr_addr, 4);
                    printf("relocate %s at address 0x%llx point to 0x%llx\n", tgt_name, (unsigned long long)tgt_addr, (unsigned long long)sym_addr);
                } else {
                    fprintf(stderr, "ignore %s\n", tgt_name);
                }
            }
        }
        printf("\n");
    }
}
