#include <string.h>
#include <k/sys.h>
#include <k/log.h>

void log(const char *str) {
    _log(str, strlen(str));
}
