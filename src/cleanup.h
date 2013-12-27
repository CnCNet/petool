#pragma once

#define ENSURE(COND, ...)                       \
    if (COND) {                                 \
        fprintf(stderr, __VA_ARGS__);           \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }

#define ENSURE_PERROR(COND, MSG)                \
    if (COND) {                                 \
        perror(MSG);                            \
        ret = EXIT_FAILURE;                     \
        goto cleanup;                           \
    }
