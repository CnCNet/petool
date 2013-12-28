#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdint.h>
#include <inttypes.h>

#include "cleanup.h"
#include "common.h"


// Caller cleans up fh and image, length is optional
int open_and_read(FILE **fh, int8_t **image, uint32_t *length,
                  const char* executable, const char *fopen_attr)
{
    int ret = EXIT_SUCCESS;

    *fh = fopen(executable, fopen_attr);
    NO_FAIL(!*fh, "Could not open executable\n");

    NO_FAIL_PERROR(fseek(*fh, 0L, SEEK_END),
		   "Need seekable file for executable, not stream");

    uint32_t len = ftell(*fh);
    rewind(*fh);

    *image = malloc(len);
    NO_FAIL(!*image, "Failed to allocate memory to read executable with\n");

    NO_FAIL_PERROR(fread(*image, len, 1, *fh) != 1, "Error reading executable");

    if (length) *length = len;

cleanup:
    return ret;
}
