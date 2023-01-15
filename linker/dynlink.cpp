#include <cstdio>
#include <cstring>
#include <vector>
#include <functional>
#include <elf.h>
#include "linklib.h"

const int BUFSIZE = 64*1024;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s func-name obj-file0 [obj-file1, ...]\n", argv[0]);
        return -1;
    }

    const char* funcname = argv[1];
    static char buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);

    // align by 4K
    char* p = (char*)(((unsigned long long)buffer + 4095) & ~4095);

    std::vector<Obj> objs;

    for (int i = 2; i < argc; i++) {
        Obj obj(p, argv[i]);
        objs.push_back(std::move(obj));
        FILE* fp = fopen(argv[i], "rb");
        if (fp == NULL) {
            fprintf(stderr, "cannot open %s\n", argv[i]);
            continue;
        }

        printf("load %s at address 0x%llx\n", argv[i], (unsigned long long)p);
        int c;
        while((c = getc(fp)) != EOF) *(p++) = c;
        fclose(fp);

        p = (char*)(((unsigned long long)p + 15) & ~15);

        if(!check_ehdr(objs.back().get_ehdr())) {
            return -1;
        }

        //TODO:
        relocate_common_symbol(objs.back().get_ehdr());

        // prepare .bss space
        Elf64_Shdr* bss = get_section(objs.back().get_ehdr(), ".bss");
        if (bss != nullptr) {
            printf("find .bss section and locate at address 0x%llx\n", (unsigned long long)p);
            bss->sh_offset = (unsigned long long)(p - objs.back().address); 
            memset(p, 0, bss->sh_size);
            p += bss->sh_size;
        }

        p = (char*)(((unsigned long long)p + 15) & ~15);
    }

    //link_objs(objs);
    Obj obj = search_symbol(objs, funcname);
    if (obj.address == nullptr) {
        fprintf(stderr, "cannot find function %s\n", funcname);
        return -1;
    }
    printf("found function %s at address 0x%llx\n", funcname, (unsigned long long)obj.address);
    using f = int(*)();
    auto func = (f)(obj.address);
    int ret = func();
    printf("ret = %d\n", ret);

    return 0;
}