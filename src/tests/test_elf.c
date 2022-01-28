#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "headers/linker.h"
#include "headers/common.h"

void parse_elf(char *filename, elf_t *elf);
void free_elf(elf_t *elf);

int main()
{   
    elf_t elf;
    parse_elf("./files/exe/sum.elf.txt", &elf);
    free_elf(&elf);

    return 0;
}