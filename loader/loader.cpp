#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

const uint64_t PAGE_SIZE = 4096;

#define IS_ELF(ehdr) (                   \
    ((ehdr).e_ident[EI_MAG0] == ELFMAG0) && \
    ((ehdr).e_ident[EI_MAG1] == ELFMAG1) && \
    ((ehdr).e_ident[EI_MAG2] == ELFMAG2) && \
    ((ehdr).e_ident[EI_MAG3] == ELFMAG3))

template<typename T>
T round_up(T n, T m) {
    return (n + (m-1)) & ~(m-1);
}

template<typename T>
T round_down(T n, T m) {
    return n & ~(m-1);
}

char* get_interp_name(char* head) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)head;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        if (phdr->p_type == PT_INTERP) {
            char* name = head + phdr->p_offset;
            return name;
        }
    }
    return nullptr;
}

char* load_files(char* head) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)head;
    if (!IS_ELF(*ehdr)) {
        fprintf(stderr, "This is not ELF file.\n");
        return nullptr;
    }

    //if (ehdr->e_type != ET_DYN) {
    //    fprintf(stderr, "only support PIE files.\n");
    //    return nullptr;
    //}

    uint64_t base_addr = -1;
    size_t top_addr = 0;
    // analyze required map size
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr*)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        if (phdr->p_type != PT_LOAD) continue;
        uint64_t vaddr = static_cast<uint64_t>(phdr->p_vaddr);
        size_t msize = phdr->p_memsz;
        if (base_addr > vaddr) base_addr = vaddr;
        if (top_addr < vaddr + msize) top_addr = vaddr + msize;
    }
    uint64_t base_addr_align = round_down(base_addr, PAGE_SIZE);
    uint64_t top_addr_align = round_up(top_addr, PAGE_SIZE);
    //uint64_t base_offset = base_addr - base_addr_align;

    // TODO: fix prot (use mprotect for each ph?)
    size_t map_size = top_addr_align - base_addr_align;
    char* map_head = (char*)mmap(&base_addr_align, map_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
    if (map_head == MAP_FAILED) {
        perror("mmap");
        return nullptr;
    }
    printf("Map head addr: 0x%lx, size: 0x%lx\n", (uint64_t)map_head, map_size);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr*)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        if (phdr->p_type != PT_LOAD) continue;
        printf("load Segment %d\n", i);
        uint64_t vaddr = static_cast<uint64_t>(phdr->p_vaddr);
        size_t fsize = phdr->p_filesz;
        printf("v_addr = 0x%lx, filesz = 0x%lx\n", vaddr, fsize);
        // TODO: support prot with mprotect
        int prot = 0;
        if (phdr->p_flags & PROT_READ) prot |= PROT_READ;
        if (phdr->p_flags & PROT_WRITE) prot |= PROT_WRITE;
        if (phdr->p_flags & PROT_EXEC) prot |= PROT_EXEC;
        memcpy(map_head + (vaddr - base_addr), head + phdr->p_offset, fsize);
        printf("copy to 0x%lx\n", (uint64_t)(map_head + (vaddr - base_addr)));
    }

    char* entry = map_head + (ehdr->e_entry - base_addr);
    return entry;
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

    char* entry = load_files(head);
    if (entry == nullptr) {
        return -1;
    }
    printf("%s Entry point: 0x%lx\n", argv[1], (uint64_t)entry);

    char* interp = get_interp_name(head);
    char* interp_entry = nullptr;
    if (interp != nullptr) {
        printf("find interpreter %s.\n", interp);

        int fd = open(interp, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "cannot open %s\n", interp);
            return -1;
        }
        struct stat sb;
        fstat(fd, &sb);
        char* elf_ld_head = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
        char* interp_entry = load_files(elf_ld_head);
        if (interp_entry == nullptr) {
            return -1;
        }
        printf("%s Entry point: 0x%lx\n", interp, (uint64_t)entry);
    }

    munmap(head, sb.st_size);
    close(fd);

    //TODO: stack, arguments, aux vec

    using ep = int(*)(int, char**, char**);
    ep func = (interp_entry != nullptr) ? (ep)interp_entry : (ep)entry;
    char* env[1] = {nullptr};
    func(argc, argv, env);

    return 0;
}