#define UNUSED(v) v __attribute__((unused))
#define NORETURN    __attribute__((noreturn))
#define PACKED      __attribute__((packed))

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)
