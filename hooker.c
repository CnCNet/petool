/*
 * Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "pe.h"

int main(int argc, char **argv)
{
    FILE *exe;
    FILE *hooks;
    unsigned int exe_length;
    unsigned int hooks_length;
    unsigned char *exe_data;
    unsigned char *hooks_data;
    int p,i;

    PIMAGE_DOS_HEADER dos_hdr;
    PIMAGE_NT_HEADERS nt_hdr;
    PIMAGE_SECTION_HEADER sct_hdr;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <hooks> <exe>\n", argv[0]);
        return 1;
    }

    hooks = fopen(argv[1], "rb");
    if (!hooks)
    {
        fclose(hooks);
        perror("hooker: failed to open hooks file");
        return 1;
    }

    exe = fopen(argv[2], "rb+");
    if (!exe)
    {
        perror("hooker: failed to open executable file");
        return 1;
    }

    fseek(exe, 0L, SEEK_END);
    exe_length = ftell(exe);
    rewind(exe);

    exe_data = malloc(exe_length);

    if (fread(exe_data, exe_length, 1, exe) != 1)
    {
        free(exe_data);
        fclose(exe);
        perror("hooker: error reading executable");
        return 1;
    }

    rewind(exe);

    fseek(hooks, 0L, SEEK_END);
    hooks_length = ftell(hooks);
    rewind(hooks);

    if (hooks_length == 0)
    {
        free(exe_data);
        fclose(exe);
        printf("hooker: empty hooks file\n");
        return 0;
    }

    hooks_data = malloc(hooks_length);

    if (fread(hooks_data, hooks_length, 1, hooks) != 1)
    {
        free(hooks_data);
        free(exe_data);
        fclose(exe);
        perror("hooker: error reading hooks");
        return 1;
    }

    fclose(hooks);

    dos_hdr = (void *)exe_data;
    nt_hdr = (void *)(exe_data + dos_hdr->e_lfanew);

    p = 0;
    while (p < hooks_length)
    {
        if (*(hooks_data + p) == 0)
        {
            unsigned int address = *(unsigned int *)(hooks_data + p + 1);
            unsigned int destination = *(unsigned int *)(hooks_data + p + 5);

            sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);
            for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
            {
                if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
                {
                    unsigned int offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

                    if (abs(destination - address) < 128)
                    {
                        printf("hooker: JMP SHORT Ox%08X -> 0x%08X\n", address, destination);
                        exe_data[offset] = 0xEB;
                        *(signed char *)(exe_data + offset + 1) = destination - address - 2;
                    }
                    else
                    {
                        printf("hooker: JMP Ox%08X -> 0x%08X\n", address, destination);
                        exe_data[offset] = 0xE9;
                        *(signed int *)(exe_data + offset + 1) = destination - address - 5;
                    }

                    break;
                }

                sct_hdr++;
            }

            p += 9;
        }
        else if (*(hooks_data + p) == 1)
        {
            unsigned int address = *(unsigned int *)(hooks_data + p + 1);
            unsigned char data = *(hooks_data + p + 5);
            unsigned int length = *(unsigned int *)(hooks_data + p + 6);

            sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);
            for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
            {
                if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
                {
                    unsigned int offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

                    printf("hooker: MEMSET Ox%08X -> 0x%08X = 0x%02X\n", address, address + length, data);
                    memset(exe_data + offset, data, length);

                    break;
                }

                sct_hdr++;
            }

            p += 10;
        }
        else
        {
            fprintf(stderr, "hooker: unknown command %d\n", *(hooks_data + p));
            return 1;
        }
    }

    if (fwrite(exe_data, exe_length, 1, exe) != 1)
    {
        perror("hooker: error writing to executable file");
        return 1;
    }

    free(exe_data);
    free(hooks_data);

    fclose(exe);

    return 0;
}
