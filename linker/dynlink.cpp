#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "linklib.h"

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

    elfdump(head);

    munmap(head, sb.st_size);
    close(fd);

    return 0;
}