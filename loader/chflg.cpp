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

void change_flags(char* head) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)head;
    if (!IS_ELF(*ehdr)) {
        fprintf(stderr, "This is not ELF file.");
        return;
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *phdr = (Elf64_Phdr*)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        const int nex_flags = PF_X | PF_R | PF_W;
        printf("[%d] change flag from %d to %d\n", i, phdr->p_flags, nex_flags);
        phdr->p_flags = nex_flags;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s obj-file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return -1;
    }
    struct stat sb;
    fstat(fd, &sb);
    char* head = (char*)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (head == MAP_FAILED) {
        fprintf(stderr, "cannot map fd\n");
        return -1;
    }

    change_flags(head);

    munmap(head, sb.st_size);
    close(fd);

    return 0;
}