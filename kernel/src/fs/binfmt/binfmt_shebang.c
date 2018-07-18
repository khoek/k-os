#include <stdbool.h>

#include "common/types.h"
#include "init/initcall.h"
#include "fs/binfmt.h"
#include "sched/task.h"
#include "log/log.h"

#define MAX_LINE_LEN 256

#define SHEBANG_MAGIC "#!"
#define SHEBANG_MAGIC_LEN 2

static bool is_whitespace(char c) {
    return (c == ' ') || (c == '\t');
}

//must (obviously) be null terminated
static char * advance_past_whitespace(char *str) {
    while(*str && is_whitespace(*str)) {
        str++;
    }
    return str;
}

static char * advance_to_whitespace(char *str) {
    while(*str && !is_whitespace(*str)) {
        str++;
    }
    return str;
}

static int32_t load_shebang(binary_t *binary) {
    char buff[MAX_LINE_LEN + 1];

    int32_t ret = vfs_seek(binary->file, 0, SEEK_SET);
    if(ret < 0) {
        return ret;
    }
    if(ret > 0) {
        return -EIO;
    }

    ret = vfs_read(binary->file, buff, MAX_LINE_LEN);
    //the '<=' ensures there it is at least one more char after the magic
    if(ret < 0) {
        return ret;
    }
    if(ret <= SHEBANG_MAGIC_LEN
      || memcmp(buff, SHEBANG_MAGIC, SHEBANG_MAGIC_LEN)) {
        return -ENOEXEC;
    }

    buff[ret] = '\0';

    char *interp = advance_past_whitespace(buff + SHEBANG_MAGIC_LEN);

    //this implicitly handles the case where "*interp == '\0'"
    char *line_end = strchr(interp, '\n');
    if(!line_end) {
        return -ENOEXEC;
    }
    *line_end = '\0';

    char *interp_end = advance_to_whitespace(interp);
    char *optional_arg = interp_end;
    if(*optional_arg) {
        optional_arg = advance_past_whitespace(optional_arg);
    }
    *interp_end = '\0';

    path_t path;
    ret = vfs_lookup(&obtain_fs_context(current)->pwd, interp, &path);
    if(ret) {
        return ret;
    }

    file_t *f = vfs_open_file(&path);
    if(!f) {
        return -EIO;
    }

    binary->file = f;

    char **old_argv = binary->argv;
    uint32_t old_argc = strtab_len(binary->argv);
    binary->argv = alloc_strtab(old_argc + 1 + (*optional_arg ? 1 : 0));
    binary->argv[0] = strdup(interp);
    uint32_t argv_off = 1;
    if(*optional_arg) {
        binary->argv[argv_off] = strdup(optional_arg);
        argv_off++;
    }
    memcpy(binary->argv + argv_off, old_argv, sizeof(char *) * old_argc);

    return binfmt_load(binary);
}

static binfmt_t shebang = {
    .load = load_shebang,
};

static INITCALL shebang_register_binfmt() {
    binfmt_register(&shebang);

    return 0;
}

core_initcall(shebang_register_binfmt);
