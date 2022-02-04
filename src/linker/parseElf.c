#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "headers/linker.h"
#include "headers/common.h"

// 解析各种表
// str: address pointing to the char string
// ent: address in stack, pointing to address in heap, remember to free after use
static int parse_table_entry(char *str, char ***ent)
{
    int count_col = 1;
    int len = strlen(str);
    
    // count columns
    for (int i = 0; i < len; i ++)
    {
        if (str[i] == ',')
        {
            count_col ++;
        }
    }

    // malloc and create list
    // arr pointing to each column, whose type is (char *)
    char **arr = malloc(count_col * sizeof(char *)); 
    *ent = arr; // ent pointing to each arr in heap, whose type is (char **)

    int col_index = 0;
    int col_width = 0;
    char col_buf[32]; // 暂存每个column的内容

    for (int i = 0; i < len + 1; i ++)
    {
        if (str[i] == ',' || str[i] == '\0')
        {
            // malloc and copy
            char *col = malloc((col_width + 1) * sizeof(char)); // col: in heap
            for (int j = 0; j < col_width; j ++)
            {
                col[j] = col_buf[j]; // copy from stack to heap
            }
            col[col_width] = '\0';

            // update
            arr[col_index] = col;
            col_index ++;
            col_width = 0;
        }
        else   // 一个column未结束
        {
            col_buf[col_width] = str[i];
            col_width ++;
        }
    }
    return count_col;
}

// 释放内存
// ** ent 为堆上的地址
static void free_table_entry(char **ent, int n)
{
    for (int i = 0; i < n; i ++)
    {
        free(ent[i]); // 先free char *
    }
    free(ent); // 再free heap上的 char **
}

// 解析节头表的每行
// str：each line of section header table
// sh: each item resolved from str, saving in sh
static void parse_sh(char *str, sh_entry_t *sh)
{
    // sh_name, sh_addr, sh_offset, sh_size
    // .text,0x0,4,22
    char **cols; // address in stack
    int num_cols = parse_table_entry(str, &cols);
    assert(num_cols == 4);

    assert(sh != NULL);
    strcpy(sh->sh_name, cols[0]);
    sh->sh_addr = string2uint(cols[1]);
    sh->sh_offset = string2uint(cols[2]); // 每个section具体的起始地址
    sh->sh_size = string2uint(cols[3]);   // 每个section具体多少行

    free_table_entry(cols, num_cols);
}

static void print_sh_entry(sh_entry_t *sh)
{
    debug_printf(DEBUG_LINKER, "%s\t%x\t%d\t%d\n",
        sh->sh_name,
        sh->sh_addr,
        sh->sh_offset,
        sh->sh_size);
}
// 解析符号表
static void parse_symtab(char *str, st_entry_t *ste)
{   
    // st_name,bind,type,st_shndx,st_value,st_size
    // sum,STB_GLOBAL,STT_FUNC,.text,0,22
    char **cols; // address in stack
    int num_cols = parse_table_entry(str, &cols);
    assert(num_cols == 6);

    assert(ste != NULL);
    strcpy(ste->st_name, cols[0]);

    // select symbol bind
    if (strcmp(cols[1], "STB_LOCAL") == 0)
    {
        ste->bind = STB_LOCAL;
    }
    else if (strcmp(cols[1], "STB_GLOBAL") == 0)
    {
        ste->bind = STB_GLOBAL;
    }
    else if (strcmp(cols[1], "STB_WEAK") == 0)
    {
        ste->bind = STB_WEAK;
    }
    else
    {
        printf("symbol bind is nor LOCAL, GLOBAL nor WEAK\n");
        exit(0);
    }

    // select symbol type
    if (strcmp(cols[2], "STT_NOTYPE") == 0)
    {
        ste->type = STT_NOTYPE;
    }
    else if (strcmp(cols[2], "STT_OBJECT") == 0)
    {
        ste->type = STT_OBJECT;
    }
    else if (strcmp(cols[2], "STT_FUNC") == 0)
    {
        ste->type = STT_FUNC;
    }
    else
    {
        printf("symbol type is nor NOTYPE, OBJECT nor FUNC\n");
        exit(0);
    }

    strcpy(ste->st_shndx, cols[3]);
    ste->st_value = string2uint(cols[4]);
    ste->st_size = string2uint(cols[5]);
    
    free_table_entry(cols, num_cols);
}

static void print_symtab_entry(st_entry_t *ste)
{
    debug_printf(DEBUG_LINKER, "%s\t%d\t%d\t%s\t%d\t%d\n",
        ste->st_name,
        ste->bind,
        ste->type,
        ste->st_shndx,
        ste->st_value,
        ste->st_size);
}

// read from filename, then store to where bufaddr pointing to
static int read_elf(const char *filename, uint64_t bufaddr)
{
    // open file and read
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        debug_printf(DEBUG_LINKER, "unable to open file %s\n", filename);
        exit(1);
    }

    // read text file line by line
    char line[MAX_ELF_FILE_WIDTH];
    int line_counter = 0;

    while (fgets(line, MAX_ELF_FILE_WIDTH, fp) != NULL)
    {
        int len = strlen(line);
        if ((len == 0) || 
            ((len >= 1) && (line[0] == '\n' || line[0] == '\r' || line[0] == '\t')) ||
            (len >= 2 && (line[0] == '/') && (line[1] == '/')))
        {
            continue;
        }

        // check if is empty or while line
        int iswhite = 1;
        for (int i = 0; i < len; i ++)
        {
            iswhite = iswhite && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r');
        }
        if (iswhite == 1)
        {
            continue;
        }

        // to this line, this line is not white and contains information
        if (line_counter < MAX_ELF_FILE_LENGTH) // 若bufaddr所指向的buffer不会溢出
        {
            // store this line to buffer[line_counter]
            uint64_t addr = bufaddr + line_counter * MAX_ELF_FILE_WIDTH * sizeof(char);
            char *linebuf = (char *)addr; // 字符数组

            int i = 0;
            while (i < len && i < MAX_ELF_FILE_WIDTH)
            {
                // omit last line-changed symbol:\n,\r
                // omit in-line comment //
                if ( (line[i] == '\r' || line[i] == '\n') ||
                    ((i + 1) < len && (i + 1) < MAX_ELF_FILE_WIDTH && 
                        line[i] == '/' && line[i + 1] == '/'))
                {
                    break;
                } 
                linebuf[i] = line[i]; 
                i ++;      
            }
            linebuf[i] = '\0';

            line_counter ++;
        }
        else
        {
            debug_printf(DEBUG_LINKER, "elf file %s is too long (>%d)\n", 
                            filename, MAX_ELF_FILE_LENGTH);
            fclose(fp);
            exit(1);
        }
    }
    fclose(fp);
    // 断言：有效行数和buffer中存的是一致的
    assert(string2uint((char *)bufaddr) == line_counter);
    return line_counter;
}


void parse_elf(char *filename, elf_t *elf)
{
    assert(elf != NULL);
    int line_count = read_elf(filename, (uint64_t)&(elf->buffer)); // filename文件读到elf->buffer中
    
    for (int i = 0; i < line_count; i ++)
    {
        printf("[%d]\t%s\n", i, elf->buffer[i]); // 有效行
    }

    // parse section header
    elf->sht_count = string2uint(elf->buffer[1]);  // index = 1的行就是section header table的行数
    elf->sht = malloc(elf->sht_count * sizeof(sh_entry_t)); // 为每个SHT行（表项）分配内存

    sh_entry_t *symt_sh = NULL;  // .symtab在section header table中的表项
    for (int i = 0; i < elf->sht_count; i ++)
    {
        parse_sh(elf->buffer[2 + i], &(elf->sht[i])); 
        print_sh_entry(&(elf->sht[i]));

        if (strcmp(elf->sht[i].sh_name, ".symtab") == 0)
        {
            // this is section header for symbol table
            symt_sh = &(elf->sht[i]);
        }
    }

    assert(symt_sh != NULL);

    // parse symbol table 
    elf->symt_count = symt_sh->sh_size;  // count of symbol table lines
    elf->symt = malloc(elf->symt_count * sizeof(st_entry_t));
    for (int i = 0; i < symt_sh->sh_size; i ++) // 遍历.symtab每一行
    {
        // 记得相对于.symtab表的首地址，所以加sh_offset
        parse_symtab(
            elf->buffer[i + symt_sh->sh_offset], 
            &(elf->symt[i])); // 解析字符串(实际地址)到elf->symt[i]
        print_symtab_entry(&(elf->symt[i]));
    }

}

void free_elf(elf_t *elf)
{
    assert(elf != NULL);
    free(elf->sht);
}