#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "headers/linker.h"
#include "headers/common.h"


int read_elf(const char *filename, uint64_t bufaddr)
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
        if (line_counter < MAX_ELF_FILE_LENGTH)
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
    // 断言：有效行数是否正确
    assert(string2uint((char *)bufaddr) == line_counter);
    return line_counter;
}