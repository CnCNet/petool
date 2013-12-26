/*
 * Copyright (c) 2012, 2013 Toni Spets <toni.spets@iki.fi>
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
#include <unistd.h>
#include <ctype.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include "pe.h"
#endif

int strflags(char *flag_list);
unsigned long bytealign(unsigned long raw, unsigned long align);

int petool_section_add(void **image, long *length, int argc, char **argv);
int petool_section_remove(void **image, long *length, int argc, char **argv);
int petool_section_edit(void **image, long *length, int argc, char **argv);
int petool_dump(void **image, long *length, int argc, char **argv);
int petool_section_export(void **image, long *length, int argc, char **argv);
int petool_section_import(void **image, long *length, int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "nasm-patcher petool git~%s (c) 2012-2013 Toni Spets\n\n", REV);
        fprintf(stderr, "usage: %s <executable> <command> [args ...]\n", argv[0]);
        return 1;
    }

    FILE *fh = fopen(argv[1], "rb+");
    if (!fh)
    {
        perror("Error opening executable");
        return 1;
    }

    fseek(fh, 0L, SEEK_END);
    long length = ftell(fh);
    rewind(fh);

    void *image = malloc(length);

    if (fread(image, length, 1, fh) != 1)
    {
        fclose(fh);
        perror("Error reading executable");
        return 1;
    }

    rewind(fh);

    long ret = 0;

    if (strcmp(argv[2], "dump") == 0)
    {
        ret = petool_dump(&image, &length, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[2], "remove") == 0)
    {
        ret = petool_section_remove(&image, &length, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[2], "edit") == 0)
    {
        ret = petool_section_edit(&image, &length, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[2], "add") == 0)
    {
        ret = petool_section_add(&image, &length, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[2], "export") == 0)
    {
        ret = petool_section_export(&image, &length, argc - 2, &argv[2]);
    }
    else if (strcmp(argv[2], "import") == 0)
    {
        ret = petool_section_import(&image, &length, argc - 2, &argv[2]);
    }

    if (ret)
    {
        fwrite(image, length, 1, fh);
        ftruncate(fileno(fh), length);
    }

    free(image);
    fclose(fh);

    return 0;
}

int strflags(char *flag_list)
{
    int flags = 0;

    for (int i = 0; i < strlen(flag_list); i++)
    {
        switch (tolower(flag_list[i]))
        {
            case 'r': flags |= IMAGE_SCN_MEM_READ;                  break;
            case 'w': flags |= IMAGE_SCN_MEM_WRITE;                 break;
            case 'x': flags |= IMAGE_SCN_MEM_EXECUTE;               break;
            case 'c': flags |= IMAGE_SCN_CNT_CODE;                  break;
            case 'i': flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;      break;
            case 'u': flags |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;    break;
        }
    }

    return flags;
}

unsigned long bytealign(unsigned long raw, unsigned long align)
{
    return ((raw / align) + (raw % align > 0)) * align;
}

int petool_section_add(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    if (argc < 4)
    {
        printf("Error: Section name, flags and size required.\n");
        return 0;
    }

    char *name = argv[1];
    int flags = strflags(argv[2]);
    int bytes = abs(atoi(argv[3]));
    unsigned int address = 0, start = 0, first_ptr = -1;
    long olength = *length;

    nt_hdr->FileHeader.NumberOfSections++;

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (i == nt_hdr->FileHeader.NumberOfSections - 1)
        {
            if (((char *)sct_hdr + sizeof(*sct_hdr) - (char *)*image) >= first_ptr)
            {
                fprintf(stderr, "Error: new section would overflow image data!\n");
                return 1;
            }

            /* for once, strncpy is useful */
            strncpy((char *)sct_hdr->Name, name, 8);
            sct_hdr->Misc.VirtualSize = bytes;
            sct_hdr->VirtualAddress = address;
            sct_hdr->Characteristics = flags;
            sct_hdr->PointerToRawData = start;

            if (flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
            {
                nt_hdr->OptionalHeader.SizeOfUninitializedData  += sct_hdr->SizeOfRawData;
            }
            else
            {
                sct_hdr->SizeOfRawData = bytealign(bytes, nt_hdr->OptionalHeader.FileAlignment);
                *length += sct_hdr->SizeOfRawData;
            }

            if (flags & IMAGE_SCN_CNT_CODE)
            {
                nt_hdr->OptionalHeader.SizeOfCode               += sct_hdr->SizeOfRawData;
            }

            if (flags & IMAGE_SCN_CNT_INITIALIZED_DATA)
            {
                nt_hdr->OptionalHeader.SizeOfInitializedData    += sct_hdr->SizeOfRawData;
            }

            nt_hdr->OptionalHeader.SizeOfImage  = bytealign(sct_hdr->VirtualAddress +  sct_hdr->Misc.VirtualSize, nt_hdr->OptionalHeader.SectionAlignment);
            nt_hdr->OptionalHeader.CheckSum     = 0x00000000;

            // set DataDirectory entry if adding resources
            if (strcmp(name, ".rsrc") == 0)
            {
                nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = sct_hdr->VirtualAddress;
                nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = sct_hdr->Misc.VirtualSize;
            }
        }
        else
        {
            if (sct_hdr->VirtualAddress >= address)
            {
                address = sct_hdr->VirtualAddress + bytealign(sct_hdr->Misc.VirtualSize ? sct_hdr->Misc.VirtualSize : sct_hdr->SizeOfRawData, nt_hdr->OptionalHeader.SectionAlignment);

                if (sct_hdr->Characteristics & ~IMAGE_SCN_CNT_INITIALIZED_DATA)
                {
                    start = sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData;
                }
            }

            if (sct_hdr->PointerToRawData > 0 && sct_hdr->PointerToRawData < first_ptr)
            {
                first_ptr = sct_hdr->PointerToRawData;
            }
        }

        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s %s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-",
                (i == nt_hdr->FileHeader.NumberOfSections - 1               ? "<- this was created" : "")
        );

        sct_hdr++;
    }

    // move symbol table if present
    if (nt_hdr->FileHeader.PointerToSymbolTable >= start)
    {
        nt_hdr->FileHeader.PointerToSymbolTable += (*length - olength);
    }

    *image = realloc(*image, *length);

    // move and zero the new section
    memmove((char *)*image + start + (*length - olength), (char *)*image + start, olength - start);
    memset((char *)*image + start, 0, *length - olength);

    return 1;
}

int petool_section_remove(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    if (nt_hdr->FileHeader.NumberOfSections == 0)
    {
        printf("Error: No sections remaining.\n");
        return 0;
    }

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s %s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-",
                (i == nt_hdr->FileHeader.NumberOfSections - 1               ? "<- this was removed" : "")
        );

        if (i == nt_hdr->FileHeader.NumberOfSections - 1)
        {
            if (sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE)
                nt_hdr->OptionalHeader.SizeOfCode               -= sct_hdr->SizeOfRawData;

            if (sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
                nt_hdr->OptionalHeader.SizeOfInitializedData    -= sct_hdr->SizeOfRawData;

            if (sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
                nt_hdr->OptionalHeader.SizeOfUninitializedData  -= sct_hdr->SizeOfRawData;

            nt_hdr->OptionalHeader.SizeOfImage  = bytealign(sct_hdr->VirtualAddress, nt_hdr->OptionalHeader.SectionAlignment);
            nt_hdr->OptionalHeader.CheckSum     = 0x00000000;

            // move symbol table if present
            if (nt_hdr->FileHeader.PointerToSymbolTable >= sct_hdr->PointerToRawData)
            {
                nt_hdr->FileHeader.PointerToSymbolTable -= sct_hdr->SizeOfRawData;
            }

            // remove pointer from DataDirectory if present
            for (int j = 0; j < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; j++)
            {
                PIMAGE_DATA_DIRECTORY idd = &nt_hdr->OptionalHeader.DataDirectory[j];

                if (idd->VirtualAddress >= sct_hdr->VirtualAddress && idd->VirtualAddress <= sct_hdr->VirtualAddress + sct_hdr->Misc.VirtualSize)
                {
                    memset(idd, 0, sizeof *idd);
                }
            }

            // move the rest of the image
            memmove((char *)*image + sct_hdr->PointerToRawData, (char *)*image + sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData, *length - (sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData));

            *length -= sct_hdr->SizeOfRawData;

            // zero the removed section header
            memset(sct_hdr, 0, sizeof *sct_hdr);
        }

        sct_hdr++;
    }

    nt_hdr->FileHeader.NumberOfSections--;

    return 1;
}

int petool_section_edit(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    if (argc < 3)
    {
        printf("Error: Section name and flags required.\n");
        return 0;
    }

    char *name = argv[1];
    int flags = strflags(argv[2]);

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    if (nt_hdr->FileHeader.NumberOfSections == 0)
    {
        printf("Error: No sections remaining.\n");
        return 0;
    }

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        char sct_name[9];
        memset(sct_name, 0, sizeof name);
        memcpy(sct_name, sct_hdr->Name, 8);

        if (strcmp(sct_name, name) == 0)
        {
            sct_hdr->Characteristics = flags;
            nt_hdr->OptionalHeader.CheckSum     = 0x00000000;
        }

        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s %s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-",
                strcmp(sct_name, name) == 0                                 ? "<- this was updated" : ""
        );

        sct_hdr++;
    }

    return 1;
}

int petool_dump(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-"
        );

        sct_hdr++;
    }

    return 0;
}

int petool_section_export(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    if (argc < 3)
    {
        printf("Error: Section name and output file required.\n");
        return 0;
    }

    char *name = argv[1];
    char *output = argv[2];

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        char sct_name[9];
        memset(sct_name, 0, sizeof name);
        memcpy(sct_name, sct_hdr->Name, 8);

        if (strcmp(sct_name, name) == 0)
        {
            FILE *fh = fopen(output, "wb");
            if (!fh)
            {
                perror(output);
                return 0;
            }

            fwrite((char *)*image + sct_hdr->PointerToRawData, sct_hdr->SizeOfRawData, 1, fh);
            fclose(fh);
        }

        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s %s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-",
                strcmp(sct_name, name) == 0                                 ? "<- this was exported" : ""
        );

        sct_hdr++;
    }

    return 0;
}

int petool_section_import(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    if (argc < 3)
    {
        printf("Error: Section name and output file required.\n");
        return 0;
    }

    char *name = argv[1];
    char *input = argv[2];

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        char sct_name[9];
        memset(sct_name, 0, sizeof name);
        memcpy(sct_name, sct_hdr->Name, 8);

        if (strcmp(sct_name, name) == 0)
        {
            FILE *fh = fopen(input, "r");
            if (!fh)
            {
                perror(input);
                return 0;
            }

            fseek(fh, 0, SEEK_END);
            long length = ftell(fh);
            fseek(fh, 0, SEEK_SET);

            if (length > sct_hdr->SizeOfRawData)
            {
                fprintf(stderr, "Error: Section is too small for this file.\n");
                fclose(fh);
                return 0;
            }

            fread((char *)*image + sct_hdr->PointerToRawData, length, 1, fh);
        }

        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s %s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-",
                strcmp(sct_name, name) == 0                                 ? "<- this was exported" : ""
        );

        sct_hdr++;
    }

    return 1;
}
