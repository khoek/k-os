#include <stdbool.h>
#include "lib/int.h"
#include "bug/panic.h"
#include "mm/cache.h"
#include "log/log.h"
#include "fs/type/devfs.h"
#include "driver/console/console.h"

static bool keystate[128];

static const char KEYMAP[] = {
          // 0x00
          0, 0,
          '\1', '\1',
          '1', '!',
          '2', '@',
          '3', '#',
          '4', '$',
          '5', '%',
          '6', '^',
          '7', '&',
          '8', '*',
          '9', '(',
          '0', ')',
          '-', '_',
          '=', '+',
          '\x7f', '\x7f', // backspace
          ' ', ' ',

          // 0x10
          'q', 'Q',
          'w', 'W',
          'e', 'E',
          'r', 'R',
          't', 'T',
          'y', 'Y',
          'u', 'U',
          'i', 'I',
          'o', 'O',
          'p', 'P',
          '[', '{',
          ']', '}',
          '\n', '\n',
            0, 0,
          'a', 'A',
          's', 'S',

          // 0x20
          'd', 'D',
          'f', 'F',
          'g', 'G',
          'h', 'H',
          'j', 'J',
          'k', 'K',
          'l', 'L',
          ';', ':',
          '\'', '\"',
          '`', '~',
          0, 0,
          '\\', '|',
          'z', 'Z',
          'x', 'X',
          'c', 'C',
          'v', 'V',

          // 0x30
          'b', 'B',
          'n', 'N',
          'm', 'M',
          ',', '<',
          '.', '>',
          '/', '?',
          0, 0,
          '*', '*',
          0, 0,
          ' ', ' ',
          0, 0,
          0, 0,
          0, 0,
          0, 0,
          0, 0,
          0, 0,

          // 0x40
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          0,  0,
          '7', '7',
          '\3', '\3',
          '9', '9',
          '-', '-',
          '4', '4',
          '5', '5',
          '6', '6',
          '+', '+',
          '1', '1',

          // 0x50
          '\5', '\5',
          '3', '3',
          '0', '0',
          '.', '.',
          0, 0
};

static char translate_keycode(uint8_t code) {
    bool capital = false;
    if(keystate[LSHIFT_KEY] || keystate[RSHIFT_KEY]) capital = !capital;
    if(keystate[CAPS_KEY]) capital = !capital;
    return KEYMAP[code * 2 + (capital ? 1 : 0)];
}

static char handle_keystate(uint8_t code) {
    bool press = !(code & RELEASE_BIT);
    code &= ~RELEASE_BIT;
    keystate[code] = press;
    return press ? translate_keycode(code) : 0;
}

static ssize_t tty_char_read(char_device_t UNUSED(*cdev), char *buff, size_t len) {
    size_t total = 0;
    while(total < len) {
        size_t amt = vfs_read(cdev->private, buff + total, len - total);
        for(size_t i = 0; i < amt; i++) {
            buff[total] = handle_keystate(buff[total]);
            if(buff[total]) {
                total++;
            }
        }

        //TODO possibly give up
    }
    return total;
}

static ssize_t tty_char_write(char_device_t *cdev, char *buff, size_t len) {
    //TODO move the cursor and other stuff

    vfs_write(cdev->private, buff, len);

    return len;
}

static char_device_ops_t tty_ops = {
    .read = tty_char_read,
    .write = tty_char_write,
};

void tty_create(char *name) {
    devfs_publish_pending();

    path_t out, start = ROOT_PATH(root_mount);
    char *str = devfs_get_strpath(name);
    if(!vfs_lookup(&start, str, &out)) {
        panicf("tty - lookup of console (%s) failure", str);
    }
    kfree(str, strlen(str) + 1);

    char_device_t *cdev = char_device_alloc();
    cdev->private = gfdt_get(vfs_open_file(out.dentry->inode));
    cdev->ops = &tty_ops;

    register_char_device(cdev, "tty");

    devfs_publish_pending();

    kprintf("tty - for \"%s\" created", name);
}
