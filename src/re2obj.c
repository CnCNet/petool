/*
 * Copyright (c) 2013 Toni Spets <toni.spets@iki.fi>
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
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>

#include "pe.h"
#include "cleanup.h"
#include "common.h"

#pragma pack(push,2)
struct reloc_entry {
    uint32_t offset;
    uint32_t unk;
    uint16_t type;
};

struct sym_entry {
    char e_name[8];
    uint32_t e_value;
    int16_t e_scnum;
    uint16_t e_type;
    uint8_t e_sclass;
    uint8_t e_numaux;
};
#pragma pack(pop)

struct reloc_entry relocs[1024];
uint32_t nrelocs = 0;

void traverse_directory(uint32_t base, void *root, PIMAGE_RESOURCE_DIRECTORY rdir, int level)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY rdir_ent = (void *)((uint8_t *)rdir + sizeof(IMAGE_RESOURCE_DIRECTORY));

    for (int i = 0; i < (rdir->NumberOfNamedEntries + rdir->NumberOfIdEntries); i++)
    {
        if (level < 2)
        {
            traverse_directory(base, root, (void *)((uint8_t *)root + (rdir_ent->OffsetToData &~ (1 << 31))), level + 1);
        }
        else
        {
            PIMAGE_RESOURCE_DATA_ENTRY leaf = (void *)((uint8_t *)root + (rdir_ent->OffsetToData &~ (1 << 31)));

            leaf->OffsetToData = leaf->OffsetToData - base;

            relocs[nrelocs].offset = (uint8_t *)&leaf->OffsetToData - (uint8_t *)root;
            relocs[nrelocs].type = 7;
            nrelocs++;
        }

        rdir_ent++;
    }
}

int re2obj(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh    = NULL;
    int8_t *image = NULL;
    FILE   *ofh   = stdout;

    FAIL_IF(argc < 2, "usage: petool re2obj <image> [ofile]\n");

    uint32_t length;
    FAIL_IF_SILENT(open_and_read(&fh, &image, &length, argv[1], "r+b"));

    if (argc > 2)
    {
        FAIL_IF(file_exists(argv[2]), "%s: output file already exists.\n", argv[2]);
        ofh = fopen(argv[2], "w");
        FAIL_IF_PERROR(ofh == NULL, "%s");
    }

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    char *section = ".rsrc";
    int8_t *data = NULL;
    uint32_t data_len = 0;

    for (int32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr) + i;

        if (strcmp(section, (char *)sct_hdr->Name) == 0)
        {
            data = image + sct_hdr->PointerToRawData;
            data_len = sct_hdr->SizeOfRawData;
            if (sct_hdr ->Misc.VirtualSize > 0 && sct_hdr->Misc.VirtualSize < data_len)
                data_len = sct_hdr->Misc.VirtualSize;

            traverse_directory(sct_hdr->VirtualAddress, data, (PIMAGE_RESOURCE_DIRECTORY)data, 0);
            break;
        }

        sct_hdr++;
    }

    FAIL_IF(data == NULL, "No '%s' section in given PE image.\n", section);

    IMAGE_FILE_HEADER FileHeader;
    memset(&FileHeader, 0, sizeof FileHeader);
    FileHeader.Machine = 0x014C;
    FileHeader.NumberOfSections = 1;
    FileHeader.Characteristics = 0x0104;

    IMAGE_SECTION_HEADER SectionHeader;
    memset(&SectionHeader, 0, sizeof SectionHeader);
    strcpy((char *)SectionHeader.Name, ".rsrc");
    SectionHeader.SizeOfRawData = data_len;
    SectionHeader.PointerToRawData = sizeof(FileHeader) + sizeof(SectionHeader);
    SectionHeader.PointerToRelocations = data_len + SectionHeader.PointerToRawData;
    SectionHeader.NumberOfRelocations = nrelocs;
    SectionHeader.Characteristics = 0xC0300040;

    FileHeader.PointerToSymbolTable = SectionHeader.PointerToRelocations + (sizeof(struct reloc_entry) * nrelocs);
    FileHeader.NumberOfSymbols = 1;

    fwrite(&FileHeader, sizeof FileHeader, 1, ofh);
    fwrite(&SectionHeader, sizeof SectionHeader, 1, ofh);
    fwrite(data, data_len, 1, ofh);

    for (uint32_t i = 0; i < nrelocs; i++)
        fwrite(&relocs[i], sizeof (struct reloc_entry), 1, ofh);

    struct sym_entry ent;
    strcpy(ent.e_name, ".rsrc");
    ent.e_value = 0;
    ent.e_scnum = 1;
    ent.e_type = 0;
    ent.e_sclass = 3;
    ent.e_numaux = 0;

    fwrite(&ent, sizeof ent, 1, ofh);

cleanup:
    if (image) free(image);
    if (fh)    fclose(fh);
    if (argc > 2)
    {
        if (ofh) fclose(ofh);
    }
    return ret;
}
