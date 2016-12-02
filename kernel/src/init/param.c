#include <stdbool.h>
#include "init/multiboot.h"
#include "init/param.h"
#include "init/initcall.h"
#include "lib/string.h"
#include "mm/mm.h"
#include "log/log.h"

#define MAX_CMDLINE_LEN 512

extern cmdline_param_t param_start[], param_end[];

static void handle_param(char *key, char *value, uint32_t value_len) {
    for(uint32_t i = 0; i < (((uint32_t) param_end) - ((uint32_t) param_start)) / sizeof(cmdline_param_t); i++) {
        if(!strcmp(key, param_start[i].name)) {
            if(!param_start[i].handle) {
                kprintf("param - param \"%s\": duplicate ignored", key);
            } else if(!param_start[i].handle(value)) {
                kprintf("param - param \"%s\": invalid value \"%s\"", key, value);
                kfree(value);
            }

            param_start[i].handle = NULL;

            return;
        }
    }

    kprintf("param - param \"%s\": not recognized", key);
}

static char *empty_string = "";
static char __initdata buff[MAX_CMDLINE_LEN + 1];

void load_cmdline() {
    uint32_t cmdline_len = strlen((void *) mbi->cmdline);
    if(cmdline_len > MAX_CMDLINE_LEN) {
        cmdline_len = MAX_CMDLINE_LEN;

        kprintf("param - truncated cmdline to %u characters (too long)", MAX_CMDLINE_LEN);
    }
    memcpy(buff, (void *) mbi->cmdline, cmdline_len);
    buff[cmdline_len + 1] = '\0';
}

void parse_cmdline() {
    kprintf("param - parsing cmdline: \"%s\"", buff);

    char *cmdline = buff;
    while(*cmdline) {
        char *key = cmdline;
        char *value = empty_string;
        uint32_t value_len = 0;

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

        char *value_copy = kmalloc(value_len + 1);
        memcpy(value_copy, value, value_len);
        value_copy[value_len] = '\0';
        handle_param(key, value_copy, value_len);
    }
}
