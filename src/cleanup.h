#pragma once

#define FAIL_IF_SILENT(COND)                    \
    if (COND) {                                 \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }

#define FAIL_IF(COND, ...)                      \
    if (COND) {                                 \
        fprintf(stderr, __VA_ARGS__);           \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }

#define FAIL_IF_PERROR(COND, MSG)               \
    if (COND) {                                 \
        perror(MSG);                            \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }
