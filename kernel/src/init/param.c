#include <stdbool.h>
#include "init/multiboot.h"
#include "init/param.h"
#include "init/initcall.h"
#include "lib/string.h"
#include "mm/cache.h"
#include "video/log.h"

#define MAX_CMDLINE_LEN 512

extern cmdline_param_t param_start[], param_end[];

static void handle_param(char *key, char *value, uint32_t value_len) {
    for(uint32_t i = 0; i < (((uint32_t) param_end) - ((uint32_t) param_start)) / sizeof(cmdline_param_t); i++) {
        if(!strcmp(key, param_start[i].name)) {
            if(!param_start[i].handle) {
                logf("param \"%s\": duplicate ignored", key);
            } else if(!param_start[i].handle(value)) {
                logf("param \"%s\": invalid value \"%s\"", key, value);
                kfree(value, value_len);
            }

            param_start[i].handle = NULL;

            return;
        }
    }

    logf("param \"%s\": not recognized", key);
}

static char *empty_string = "";
static char __initdata buff[MAX_CMDLINE_LEN + 1];

void parse_cmdline() {
    uint32_t cmdline_len = strlen((void *) multiboot_info->cmdline);
    if(cmdline_len > MAX_CMDLINE_LEN) {
        cmdline_len = MAX_CMDLINE_LEN;

        logf("truncated cmdline to %u characters (too long)", MAX_CMDLINE_LEN);
    }
    memcpy(buff, (void *) multiboot_info->cmdline, cmdline_len);
    buff[cmdline_len + 1] = '\0';

    logf("parsing cmdline: \"%s\"", buff);

    char *cmdline = buff;
    char *key, *value;
    uint32_t value_len;

    while(*cmdline) {
        key = cmdline;
        value = empty_string;
        value_len = 0;

        while(true) {
            if(*cmdline == ' ' || *cmdline == '=' || !*cmdline) {
                if(*cmdline == '=' && *(cmdline + 1)) {
                    value = cmdline + 1;
                }

                *(cmdline++) = '\0';

                break;
            }

            cmdline++;
        }

        if(value != empty_string) while(true) {
            if(*cmdline == ' ' || !*cmdline) {
                *(cmdline++) = '\0';

                break;
            }

            cmdline++;
            value_len++;
        }

        handle_param(key, value, value_len);
    }
}
