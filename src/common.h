#include <stdbool.h>

int open_and_read(FILE**, int8_t**, uint32_t *, const char*, const char*);
bool file_exists(const char *path);
const char *file_basename(const char *path);
