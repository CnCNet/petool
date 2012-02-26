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
#include "pe.h"

int main(int argc, char **argv)
{
    FILE *fh;
    unsigned char *data;
    unsigned int bytes = 0;
    long length;
    int i;
    unsigned int address = 0;

    PIMAGE_DOS_HEADER dos_hdr;
    PIMAGE_NT_HEADERS nt_hdr;
    PIMAGE_SECTION_HEADER sct_hdr;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <executable> [bytes]\n", argv[0]);
        return 1;
    }

    fh = fopen(argv[1], "rb+");
    if (!fh)
    {
        perror("Error opening executable");
        return 1;
    }

    fseek(fh, 0L, SEEK_END);
    length = ftell(fh);
    rewind(fh);

    data = malloc(length);

    if (fread(data, length, 1, fh) != 1)
    {
        fclose(fh);
        perror("Error reading executable");
        return 1;
    }

    rewind(fh);

    dos_hdr = (void *)data;
    nt_hdr = (void *)(data + dos_hdr->e_lfanew);
    sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    if (argc > 2)
    {
        bytes = abs(atoi(argv[2]));
    }
    if (bytes < 1) bytes = 1;
    bytes = (bytes / nt_hdr->OptionalHeader.SectionAlignment + (bytes % nt_hdr->OptionalHeader.SectionAlignment ? 1 : 0)) * nt_hdr->OptionalHeader.SectionAlignment;

    printf("   section    start      end   length    vaddr flg\n");
    printf("--------------------------------------------------\n");

    nt_hdr->FileHeader.NumberOfSections++;

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (i == nt_hdr->FileHeader.NumberOfSections - 1)
        {
            strcpy((char *)sct_hdr->Name, ".patch");
            sct_hdr->Misc.VirtualSize = bytes;
            sct_hdr->VirtualAddress = address;
            sct_hdr->SizeOfRawData = bytes;
            sct_hdr->PointerToRawData = length;
            sct_hdr->Characteristics = IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_CNT_CODE;

            nt_hdr->OptionalHeader.SizeOfCode               += bytes;
            nt_hdr->OptionalHeader.SizeOfImage              += bytes;
            nt_hdr->OptionalHeader.SizeOfInitializedData    += bytes;
            nt_hdr->OptionalHeader.CheckSum                 = 0x00000000; /* do I really need to recalc this? */
        }
        else
        {
            if (sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE)
            {
                /* give write access to original code when loaded */
                sct_hdr->Characteristics |= IMAGE_SCN_MEM_WRITE;
            }

            if (sct_hdr->VirtualAddress >= address)
            {
                address = ((sct_hdr->VirtualAddress + sct_hdr->SizeOfRawData) / nt_hdr->OptionalHeader.SectionAlignment + ((sct_hdr->VirtualAddress + sct_hdr->SizeOfRawData) % nt_hdr->OptionalHeader.SectionAlignment ? 1 : 0)) * nt_hdr->OptionalHeader.SectionAlignment;
            }
        }

        printf("%10s %8d %8d %8d %8X %s%s%s %s\n",
                sct_hdr->Name,
                sct_hdr->PointerToRawData,
                sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData,
                sct_hdr->SizeOfRawData,
                sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase,
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ       ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE      ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE    ? "x" : "-",
                i == nt_hdr->FileHeader.NumberOfSections - 1        ? "<- you are here" : ""
        );

        sct_hdr++;
    }

    fwrite(data, length, 1, fh);

    memset(data, 0x00, bytes);
    fwrite(data, bytes, 1, fh);

    free(data);
    fclose(fh);

    return 0;
}
