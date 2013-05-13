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
#include <ctype.h>
#include <unistd.h>
#include "pe.h"
#include "list.h"

struct annotation {
    char *type;
    int argc;
    char **argv; 
    struct annotation *next;
};

static struct annotation *annotations = NULL;

char *read_stream(FILE *stream)
{
    char *buf;
    int i, len = 1024, pos = 0;

    if ((buf = malloc(len + 1)) == NULL)
    {
        fprintf(stderr, "malloc: out of memory\r\n");
        return NULL;
    }

    buf[pos] = '\0';

    while ((i = fgetc(stream)) != EOF)
    {
        if (pos == len)
        {
            len += 1024;
            if ((buf = realloc(buf, len + 1)) == NULL)
            {
                fprintf(stderr, "realloc: out of memory\r\n");
                return NULL;
            }
        }

        buf[pos++] = (char)i;
    }

    buf[pos] = 0;

    return buf;
}

int parse_annotation(const char *str, struct annotation *ann)
{
    int i, tok = -1, len;

    if (*str != '@')
    {
        return 0;
    }

    len = strlen(++str);

    for (i = 0; i <= len; i++)
    {
        if (!isspace(str[i]) && tok == -1)
        {
            tok = i;
        }
        else if (tok > -1 && !isspace(str[i-1]) && (i == len || isspace(str[i])))
        {
            if (ann->type == NULL)
            {
                int j;
                ann->type = calloc(1, i - tok + 1);
                for (j = 0; j < i - tok; j++)
                    ann->type[j] = tolower(*(str + tok + j));
            }
            else
            {
                ann->argc++;
                ann->argv = realloc(ann->argv, ann->argc * sizeof(char *));

                if (ann->argv)
                {
                    ann->argv[ann->argc-1] = calloc(1, i - tok + 1);
                    memcpy(ann->argv[ann->argc-1], str + tok, i - tok);
                }
                else
                {
                    /* out of memory */
                    ann->argc = 0;
                }
            }

            tok = -1;
        }
    }

    if (ann->type)
    {
        return 1;
    }

    return 0;
}

void get_annotations(const char *src)
{
    int pos = 0, len = strlen(src), tok = -1;

    for (; pos < len; pos++)
    {
        if (tok == -1 && src[pos] == '@' && (pos == 0 || src[pos-1] == '\n' || (pos > 1 && src[pos-2] == '\n' && isspace(src[pos-1]))))
        {
            tok = pos;
        }

        if (tok > -1 && (src[pos] == '\r' || src[pos] == '\n'))
        {
            struct annotation *ann = LIST_ALLOC(struct annotation);
            char *line = malloc((pos - tok) + 1);
            memcpy(line, src + tok, pos - tok);
            memset((void *)(src + tok), ' ', pos - tok);
            line[pos - tok] = '\0';

            if (parse_annotation(line, ann))
            {
                LIST_INSERT(annotations, ann);
            }
            else
            {
                free(ann);
            }

            free(line);
            tok = -1;
        }
    }
}

void update_annotations(const char *k, unsigned int v)
{
    LIST_FOREACH(annotations, struct annotation, ann)
    {
        if (ann->argc > 0)
        {
            for (int i = 0; i < ann->argc; i++)
            {
                if (strcmp(ann->argv[i], k) == 0)
                {
                    free(ann->argv[i]);
                    ann->argv[i] = calloc(1, 11);
                    sprintf(ann->argv[i], "0x%08X", v);
                }
            }
        }
    }
}

int patch_image(void *image, unsigned int address, void *patch, int length)
{
    int i;

    PIMAGE_DOS_HEADER dos_hdr       = image;
    PIMAGE_NT_HEADERS nt_hdr        = (PIMAGE_NT_HEADERS)((char *)image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr   = IMAGE_FIRST_SECTION(nt_hdr);

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
        {
            unsigned int offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

            if (sct_hdr->SizeOfRawData < length)
            {
                fprintf(stderr, "Error: section length (%d) is less than patch length (%d), maybe expand the image a bit more?\r\n", sct_hdr->SizeOfRawData, length);
                return 0;
            }

            memcpy((char *)image + offset, patch, length);

            return 1;
        }

        sct_hdr++;
    }

    fprintf(stderr, "Error: memory address %08X not found in image\r\n", address);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *exe;
    FILE *patch;
    unsigned int exe_length;
    unsigned int patch_length;
    char *exe_data;
    char *patch_data;

    char label[256];
    unsigned int address = 0;
    unsigned int a;
    char *l;
    int i;
    char *map_buf;
    FILE *fh;
    char *source;
    char *nasm = "nasm";
    char nasm_flags[1024] = { '\0' };
    char *tmp_name;
    char *out_name;
    char *inc_name;
    char *exe_name;
    char buf[1024];

    if (argc < 3)
    {
        fprintf(stderr, "nasm-patcher linker git~%s (c) 2012-2013 Toni Spets\r\n\r\n", REV);
        fprintf(stderr, "usage: %s <source file> <out include file> <target executable> [nasm [nasm flags]]\r\n", argv[0]);
        return 1;
    }

    inc_name = argv[2];
    exe_name = argv[3];
    tmp_name = tempnam(NULL, "lnkr-");
    out_name = tempnam(NULL, "lnkr-");

    if (argc > 3)
    {
        nasm = argv[4];
    }

    if (argc > 4)
    {
        int i = 5;
        for (; i < argc; i++)
        {
            strncat(nasm_flags, " ", sizeof nasm_flags);
            strncat(nasm_flags, argv[i], sizeof nasm_flags);
        }
    }

    snprintf(buf, 1024, "%s%s -e %s", nasm, nasm_flags, argv[1]);

    if ((fh = popen(buf, "r")) == NULL)
    {
        fprintf(stderr, "%s\r\n", buf);
        perror(nasm);
        return 1;
    }

    source = read_stream(fh);

    if (pclose(fh) != 0)
    {
        return 1;
    }

    get_annotations(source);

    if ((fh = fopen(tmp_name, "wb")) == NULL)
    {
        perror(tmp_name);
        return 1;
    }

    /* always working with 32 bit images and need to map all for linking */
    fprintf(fh, "[bits 32]\r\n[map all]\r\n");
    fwrite(source, strlen(source), 1, fh);
    fclose(fh);

    free(source);

    snprintf(buf, 1024, "%s%s -f bin %s -o %s", nasm, nasm_flags, tmp_name, out_name);

    if ((fh = popen(buf, "r")) == NULL)
    {
        fprintf(stderr, "%s\r\n", buf);
        perror(nasm);
        unlink(tmp_name);
        return 1;
    }

    map_buf = read_stream(fh);

    if (pclose(fh) != 0)
    {
        unlink(tmp_name);
        return 1;
    }

    unlink(tmp_name);
    free(tmp_name);

    fh = fopen(inc_name, "wb");
    fprintf(fh, "; generated by linker\r\n");

    l = strtok(map_buf, "\n");
    i = 0;
    do {
        if (l == NULL)
            break;

        if (address == 0)
            sscanf(l, "%X", &address);

        if (strstr(l, "Section .text"))
            i = 1;

        if (i && sscanf(l, "%*X %X  %s", &a, label) == 2)
        {
            if (strstr(l, ".") == NULL)
            {
                fprintf(fh, "%%define %-32s 0x%08X\r\n", label, a);
                update_annotations(label, a);
            }
        }
    } while((l = strtok(NULL, "\n")));

    fclose(fh);

    free(map_buf);

    if (!address)
    {
        fprintf(stderr, "linker: ORG not found in map\r\n");
        return 1;
    }

    exe = fopen(exe_name, "rb+");
    if (!exe)
    {
        perror(exe_name);
        return 1;
    }

    fseek(exe, 0L, SEEK_END);
    exe_length = ftell(exe);
    rewind(exe);

    exe_data = malloc(exe_length);

    if (fread(exe_data, exe_length, 1, exe) != 1)
    {
        fclose(exe);
        perror("linker: error reading executable");
        return 1;
    }

    rewind(exe);

    patch = fopen(out_name, "rb");
    if (!patch)
    {
        fclose(exe);
        perror(out_name);
        unlink(out_name);
        return 1;
    }

    fseek(patch, 0L, SEEK_END);
    patch_length = ftell(patch);
    rewind(patch);

    /* ignore empty patches */
    if (patch_length == 0)
    {
        free(exe_data);
        fclose(exe);
        return 0;
    }

    patch_data = malloc(patch_length);

    if (fread(patch_data, patch_length, 1, patch) != 1)
    {
        fclose(exe);
        fclose(patch);
        unlink(out_name);
        perror("linker: error reading patch");
        return 1;
    }

    fclose(patch);
    unlink(out_name);
    free(out_name);

    if (!patch_image(exe_data, (unsigned int)address, patch_data, patch_length))
        return 1;

    printf("PATCH  %8d bytes -> %8X\r\n", patch_length, address);

    /* create annotation patches */
    LIST_FOREACH(annotations, struct annotation, ann)
    {
        if ((strcmp(ann->type, "hook") == 0 || strcmp(ann->type, "jmp") == 0))
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: JMP requires two arguments\r\n");
                return 1;
            }
            else
            {
                unsigned int from = strtol(ann->argv[0], NULL, 0);
                unsigned int to = strtol(ann->argv[1], NULL, 0);

                if (abs(to - from) < 128)
                {
                    unsigned char buf[] = { 0xEB, 0x00 };
                    *(signed char *)(buf + 1) = to - from - 2;
                    if (!patch_image(exe_data, from, buf, sizeof buf))
                        return 1;

                    printf("%-12s %8X -> %8X\r\n", "JMP SHORT", from, to);
                }
                else
                {
                    unsigned char buf[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
                    *(signed int *)(buf + 1) = to - from - 5;
                    if (!patch_image(exe_data, from, buf, sizeof buf))
                        return 1;

                    printf("%-12s %8X -> %8X\r\n", "JMP", from, to);
                }
            }
        }
        else if (strcmp(ann->type, "call") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: CALL requires two arguments\r\n");
                return 1;
            }
            else
            {
                unsigned int from = strtol(ann->argv[0], NULL, 0);
                unsigned int to = strtol(ann->argv[1], NULL, 0);
                unsigned char buf[] = { 0xE8, 0x00, 0x00, 0x00, 0x00 };
                *(signed int *)(buf + 1) = to - from - 5;
                if (!patch_image(exe_data, from, buf, sizeof buf))
                    return 1;

                printf("%-12s %8X -> %8X\r\n", "CALL", from, to);
            }
        }
        else if (strcmp(ann->type, "clear") == 0)
        {
            if (ann->argc < 3)
            {
                fprintf(stderr, "linker: CLEAR requires three arguments\r\n");
                return 1;
            }
            else
            {
                unsigned int from = strtol(ann->argv[0], NULL, 0);
                unsigned int character = strtol(ann->argv[1], NULL, 0);
                unsigned int to = strtol(ann->argv[2], NULL, 0);
                unsigned int length = to - from;
                char *zbuf = malloc(length);

                if (!zbuf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for zbuf\r\n", length);
                    return 1;
                }

                memset(zbuf, (char)character, length);

                if (!patch_image(exe_data, from, zbuf, length))
                    return 1;

                free(zbuf);

                printf("CLEAR  %8d bytes -> %8X\r\n", length, from);
            }
        }
        else if (strcmp(ann->type, "setb") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: SETB requires at least two arguments\r\n");
                return 1;
            }
            else
            {
                int i;
                unsigned int address = strtol(ann->argv[0], NULL, 0);
                unsigned int length = ann->argc - sizeof(char);
                char *buf = malloc(length);

                if (!buf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for SETB buf\r\n", length);
                    return 1;
                }

                for (i = 1; i < ann->argc; i++) {
                    if (strlen(ann->argv[i]) == 3 && ann->argv[i][0] == '\'' && ann->argv[i][2] == '\'')
                        buf[i-1] = ann->argv[i][1];
                    else
                        buf[i-1] = (char)strtol(ann->argv[i], NULL, 0);
                }

                if (!patch_image(exe_data, address, buf, length))
                    return 1;

                free(buf);

                printf("SETB   %8d bytes -> %8X\r\n", length, address);
            }
        }
        else if (strcmp(ann->type, "setw") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: SETW requires at least two arguments\r\n");
                return 1;
            }
            else
            {
                int i;
                unsigned int address = strtol(ann->argv[0], NULL, 0);
                unsigned int length = (ann->argc - 1) * sizeof(short);
                short *buf = malloc(length);

                if (!buf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for SETW buf\r\n", length);
                    return 1;
                }

                for (i = 1; i < ann->argc; i++) {
                    buf[i-1] = (short)strtol(ann->argv[i], NULL, 0);
                }

                if (!patch_image(exe_data, address, buf, length))
                    return 1;

                free(buf);

                printf("SETW   %8d bytes -> %8X\r\n", length, address);
            }
        }
        else if (strcmp(ann->type, "setd") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: SETD requires at least two arguments\r\n");
                return 1;
            }
            else
            {
                int i;
                unsigned int address = strtol(ann->argv[0], NULL, 0);
                unsigned int length = (ann->argc - 1) * sizeof(int);
                int *buf = malloc(length);

                if (!buf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for SETD buf\r\n", length);
                    return 1;
                }

                for (i = 1; i < ann->argc; i++) {
                    buf[i-1] = (int)strtol(ann->argv[i], NULL, 0);
                }

                if (!patch_image(exe_data, address, buf, length))
                    return 1;

                free(buf);

                printf("SETD   %8d bytes -> %8X\r\n", length, address);
            }
        }
        else if (strcmp(ann->type, "setq") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: SETQ requires at least two arguments\r\n");
                return 1;
            }
            else
            {
                int i;
                unsigned int address = strtol(ann->argv[0], NULL, 0);
                unsigned int length = (ann->argc - 1) * sizeof(long long int);
                long long int *buf = malloc(length);

                if (!buf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for SETQ buf\r\n", length);
                    return 1;
                }

                for (i = 1; i < ann->argc; i++) {
                    buf[i-1] = strtoll(ann->argv[i], NULL, 0);
                }

                if (!patch_image(exe_data, address, buf, length))
                    return 1;

                free(buf);

                printf("SETQ   %8d bytes -> %8X\r\n", length, address);
            }
        }
        else if (strcmp(ann->type, "setf") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: SETF requires at least two arguments\r\n");
                return 1;
            }
            else
            {
                int i;
                unsigned int address = strtol(ann->argv[0], NULL, 0);
                unsigned int length = (ann->argc - 1) * sizeof(float);
                float *buf = malloc(length);

                if (!buf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for SETF buf\r\n", length);
                    return 1;
                }

                for (i = 1; i < ann->argc; i++) {
                    buf[i-1] = strtof(ann->argv[i], NULL);
                }

                if (!patch_image(exe_data, address, buf, length))
                    return 1;

                free(buf);

                printf("SETF   %8d bytes -> %8X\r\n", length, address);
            }
        }
        else if (strcmp(ann->type, "setdf") == 0)
        {
            if (ann->argc < 2)
            {
                fprintf(stderr, "linker: SETDF requires at least two arguments\r\n");
                return 1;
            }
            else
            {
                int i;
                unsigned int address = strtol(ann->argv[0], NULL, 0);
                unsigned int length = (ann->argc - 1) * sizeof(double);
                double *buf = malloc(length);

                if (!buf)
                {
                    fprintf(stderr, "linker: out of memory when allocating %d for SETDF buf\r\n", length);
                    return 1;
                }

                for (i = 1; i < ann->argc; i++) {
                    buf[i-1] = strtod(ann->argv[i], NULL);
                }

                if (!patch_image(exe_data, address, buf, length))
                    return 1;

                free(buf);

                printf("SETDF  %8d bytes -> %8X\r\n", length, address);
            }
        }
        else
        {
            fprintf(stderr, "linker: warning: unknown annotation \"%s\"\r\n", ann->type);
        }
    }

    if (fwrite(exe_data, exe_length, 1, exe) != 1)
    {
        perror("linker: error writing to executable file");
        return 1;
    }

    free(exe_data);
    free(patch_data);

    fclose(exe);

    LIST_FOREACH(annotations, struct annotation, ann)
    {
        free(ann->type);

        for (int i = 0; i < ann->argc; i++)
        {
            free(ann->argv[i]);
        }

        free(ann->argv);
    }

    LIST_FREE(annotations);

    return 0;
}
