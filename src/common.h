#include <stdint.h>
#include <stdbool.h>

int open_and_read(FILE**, int8_t**, uint32_t *, const char*, const char*);
bool file_exists(const char *path);
const char *file_basename(const char *path);
int file_copy(const char* from, const char *to);
