#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define IS_ELF(ehdr) (                   \
    ((ehdr).e_ident[EI_MAG0] == ELFMAG0) && \
    ((ehdr).e_ident[EI_MAG1] == ELFMAG1) && \
    ((ehdr).e_ident[EI_MAG2] == ELFMAG2) && \
    ((ehdr).e_ident[EI_MAG3] == ELFMAG3))

void elfdump(char* head) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)head;
    if (!IS_ELF(*ehdr)) {
        fprintf(stderr, "This is not ELF file.");
        return;
    }

    Elf64_Shdr *shstr = (Elf64_Shdr*)(head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
    std::vector<Elf64_Shdr*> shdrs(ehdr->e_shnum);
    printf("Section:\n");
    printf("idx: name offset size\n");
    for (int i = 0; i < ehdr->e_shnum; i++) {
        shdrs[i] = (Elf64_Shdr*)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
        char* sh_name = (char*)(head + shstr->sh_offset + shdrs[i]->sh_name);
        printf("%d: %s 0x%x 0x%x\n", i, sh_name, (int)shdrs[i]->sh_offset, (int)shdrs[i]->sh_size);
    }
    printf("\n\n");

    printf("Segment:\n");
    printf("idx: (type) include sections...\n");
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr*)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        const char* type = (phdr->p_type == PT_LOAD) ? "PT_LOAD" : 
                     (phdr->p_type == PT_DYNAMIC) ? "PT_DYNAMIC" :
                     (phdr->p_type == PT_PHDR) ? "PT_PHDR" : "UNKNOWN";
        //printf("%d: %s 0x%x 0x%x |", i, type, (int)phdr->p_offset, (int)phdr->p_filesz);
        printf("%d: (%s) ", i, type);
        for (int j = 0; j < ehdr->e_shnum; j++) {
            Elf64_Shdr *shdr = shdrs[j];
            int size = (shdr->sh_type != SHT_NOBITS) ? shdr->sh_size : 0;
            if (shdr->sh_offset < phdr->p_offset) continue;
            if (shdr->sh_offset + size > phdr->p_offset + phdr->p_filesz) continue;
            printf("%s ", (char*)(head + shstr->sh_offset + shdr->sh_name));
        }
        printf("\n");
    }
    printf("\n\n");

    printf("Symbols:\n");
    for (int i = 0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr* shdr = shdrs[i];
        if (shdr->sh_type != SHT_DYNSYM && shdr->sh_type != SHT_SYMTAB) continue;
        char* sh_name = (char*)(head + shstr->sh_offset + shdr->sh_name);
        printf("Section name: %s\n", sh_name);
        printf("sh_link: %d\n", shdr->sh_link);
        for (unsigned int j = 0; j < shdr->sh_size / shdr->sh_entsize; j++) {
            Elf64_Sym* sym = (Elf64_Sym*)(head + shdr->sh_offset + shdr->sh_entsize * j);
            const char* name = (const char*)(head + shdrs[shdr->sh_link]->sh_offset + sym->st_name);
            if (!sym->st_name) continue;
            printf("[%d] %s\n", j, name);
        }
        printf("\n");
    }
    printf("\n\n");

    printf("Relocation table:\n");
    for (int i = 0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr* shdr = shdrs[i];
        if (shdr->sh_type != SHT_REL && shdr->sh_type != SHT_RELA) continue;
        char* sh_name = (char*)(head + shstr->sh_offset + shdr->sh_name);
        printf("sh_name: %s\n", sh_name);
        for (unsigned int j = 0; j < shdr->sh_size / shdr->sh_entsize; j++) {
            int sym_offset = 0;
            if (shdr->sh_type == SHT_REL) {
                Elf64_Rel* rel = (Elf64_Rel*)(head + shdr->sh_offset + shdr->sh_entsize * j);
                sym_offset = ELF64_R_SYM(rel->r_info);
            } else if (shdr->sh_type == SHT_RELA) {
                Elf64_Rela* rela = (Elf64_Rela*)(head + shdr->sh_offset + shdr->sh_entsize * j);
                sym_offset = ELF64_R_SYM(rela->r_info);
            }
            Elf64_Shdr* symtab = shdrs[shdr->sh_link];
            Elf64_Sym* sym = (Elf64_Sym*)(head + symtab->sh_offset + symtab->sh_entsize * sym_offset);
            if (!sym->st_name) continue;
            const char* name = (const char*)(head + shdrs[symtab->sh_link]->sh_offset + sym->st_name);
            printf("[%d]: %s\n", j, name);
        }
        printf("\n");
    }
    printf("\n\n");
}

void load_files(char* head) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)head;
    if (!IS_ELF(*ehdr)) {
        fprintf(stderr, "This is not ELF file.");
        return;
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr*)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        if (phdr->p_type == PT_LOAD) {
            memcpy((char*)phdr->p_vaddr, head + phdr->p_offset, phdr->p_filesz);
            printf("Segment %d loaded\n", i);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s obj-file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return -1;
    }
    struct stat sb;
    fstat(fd, &sb);
    char* head = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    load_files(head);

    munmap(head, sb.st_size);
    close(fd);

    return 0;
}