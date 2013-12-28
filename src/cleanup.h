#pragma once

#define NO_FAIL_SILENT(COND)                    \
    if (COND) {                                 \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }

#define NO_FAIL(COND, ...)                      \
    if (COND) {                                 \
        fprintf(stderr, __VA_ARGS__);           \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }

#define NO_FAIL_PERROR(COND, MSG)               \
    if (COND) {                                 \
        perror(MSG);                            \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }
