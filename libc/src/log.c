#include <string.h>
#include <k/sys.h>
#include <k/log.h>

void kprint(const char *str) {
    _log(str, strlen(str));
}
