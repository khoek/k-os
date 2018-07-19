#include <stdbool.h>
#include "common/types.h"
#include "common/ringbuff.h"
#include "bug/panic.h"
#include "mm/mm.h"
#include "sched/sched.h"
#include "log/log.h"
#include "fs/type/devfs.h"
#include "driver/console/console.h"

#define BUFFLEN 2048

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
          '\x9', '\x9',   // tab

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

typedef struct tty {
    bool keystate[128];

    file_t *console;
    ringbuff_head_t rb;
    spinlock_t lock;

    volatile uint32_t read_waiting;
    semaphore_t wait_semaphore;

    //FIXME I'm meant to be attached to a session!
    pgroup_t *pgroup;
} tty_t;

pgroup_t * tty_get_pgroup(file_t *file) {
    //FIXME Check file really is a tty, and at the same time implemenet the
    //      sycall sys_isatty().
    tty_t *tty = devfs_get_chardev(file)->private;
    return tty->pgroup;
}

void tty_set_pgroup(file_t *file, pgroup_t *pg) {
    //FIXME Check file really is a tty, and at the same time implemenet the
    //      sycall sys_isatty().
    tty_t *tty = devfs_get_chardev(file)->private;
    tty->pgroup = pg;
}

static char translate_keycode(tty_t *tty, uint8_t code) {
    bool capital = false;
    if(tty->keystate[LSHIFT_KEY] || tty->keystate[RSHIFT_KEY]) capital = !capital;
    if(tty->keystate[CAPS_KEY]) capital = !capital;
    return KEYMAP[code * 2 + (capital ? 1 : 0)];
}

static char handle_keystate(tty_t *tty, uint8_t code) {
    bool press = !(code & RELEASE_BIT);
    code &= ~RELEASE_BIT;
    tty->keystate[code] = press;
    return press ? translate_keycode(tty, code) : 0;
}

//always called with tty lock held!
static ssize_t read_one_from_console(tty_t *tty, uint8_t *c) {
    //FIXME this is a terrible hack. We should completely delete the current
    //"/dev/console" implementation and instead
    return vfs_read(tty->console, c, 1);
}

static bool is_ctrl_down(tty_t *tty) {
    return tty->keystate[CTRL_KEY];
}

static bool is_shift_down(tty_t *tty) {
    return tty->keystate[LSHIFT_KEY] || tty->keystate[LSHIFT_KEY];
}

static bool is_alt_down(tty_t *tty) {
    return tty->keystate[ALT_KEY];
}

#define KBD_SPECIAL_CHAR 0xE0

static ssize_t populate_rb(tty_t *tty) {
    ssize_t ret;

    uint8_t c;
    ret = read_one_from_console(tty, &c);
    if(ret < 0) {
        return ret;
    }
    if(ret == 0) {
        return -EIO;
    }

    if(c == KBD_SPECIAL_CHAR) {
        char *s;

        ret = read_one_from_console(tty, &c);
        if(ret < 0) {
            return ret;
        }
        if(ret == 0) {
            return -EIO;
        }

        switch(c) {
            //Arrowkey up
            case 0x48: { s = CSI_DOUBLE "A"; break; }
            //Arrowkey left
            case 0x4B: { s = CSI_DOUBLE "D"; break; }
            //Arrowkey right
            case 0x4D: { s = CSI_DOUBLE "C"; break; }
            //Arrowkey down
            case 0x50: { s = CSI_DOUBLE "B"; break; }
            //Home
            case 0x47: { s = CSI_DOUBLE "H"; break; }
            //End
            case 0x4F: { s = CSI_DOUBLE "F"; break; }
            //Delete key (not DEL!)
            case 0x53: { s = CSI_DOUBLE "3~"; break; }
            //Drop the code
            default: { return 0; }
        }

        ret = ringbuff_write(&tty->rb, s, strlen(s), char);
        if(!ret) {
            panic("tty buff full");
        }
    } else {
        c = (char) handle_keystate(tty, c);

        switch(c) {
            case 'c': {
                if(is_ctrl_down(tty)
                    && !is_shift_down(tty)
                    && !is_alt_down(tty)) {
                      pgroup_send_signal(tty->pgroup, SIGINT);
                      c = 0;
                }
            }
        }

        if(!c) {
            return 0;
        }


        ret = ringbuff_write(&tty->rb, &c, 1, char);
        if(!ret) {
            panic("tty buff full");
        }
    }

    return ret;
}

//read at most len available bytes, or block if none are avaiable
static ssize_t tty_char_read(char_device_t *cdev, char *buff, size_t len) {
    tty_t *tty = cdev->private;

    uint32_t flags;
    spin_lock_irqsave(&tty->lock, &flags);

    // There might not be any real characters to give back (i.e. handle_keystate
    // might return 0 for every char in the buffer---normally there are just a
    // few chars waiting to be processed). In this case, we just retry, with
    // vfs_read handling the blocking if neccessary.
    ssize_t ret;

    tty->read_waiting++;
    //FIXME this is a terrible hack. We should completely delete the current
    //"/dev/console" implementation and instead
    while(!(ret = ringbuff_read(&tty->rb, buff, len, char))) {
        if(should_abort_slow_io()) {
            ret = -EINTR;
            break;
        }

        spin_unlock_irqstore(&tty->lock, flags);
        semaphore_down(&tty->wait_semaphore);
        spin_lock_irqsave(&tty->lock, &flags);
    }
    tty->read_waiting--;

    spin_unlock_irqstore(&tty->lock, flags);

    //we promise to always return something
    if(ret == 0) {
        panicf("tty returned zero");
    }

    return ret;
}

static ssize_t tty_char_write(char_device_t *cdev, const char *buff, size_t len) {
    tty_t *tty = cdev->private;

    //TODO move the cursor and other stuff

    return vfs_write(tty->console, buff, len);
}

static ssize_t tty_char_poll(char_device_t *cdev, fpoll_data_t *fp) {
    tty_t *tty = cdev->private;

    uint32_t flags;
    spin_lock_irqsave(&tty->lock, &flags);
    fp->readable = ringbuff_is_empty(&tty->rb, char);
    spin_unlock_irqstore(&tty->lock, flags);

    fp->writable = true;
    fp->errored = false;
    return 0;
}

static char_device_ops_t tty_ops = {
    .read = tty_char_read,
    .write = tty_char_write,
    .poll = tty_char_poll,
};

//FIXME essentially put this file in console.c, so that this hack isn't needed
//(and so we can have virtual ttys and stuff like that).

static tty_t *master;

void tty_notify() {
    if(!master) {
        return;
    }

    uint32_t flags;
    spin_lock_irqsave(&master->lock, &flags);

    populate_rb(master);

    if(master->read_waiting) {
        semaphore_up(&master->wait_semaphore);
    }

    spin_unlock_irqstore(&master->lock, flags);
}

void tty_create(char *name) {
    devfs_publish_pending();

    path_t out;
    int32_t ret = devfs_lookup(name, &out);
    if(ret) {
        panicf("tty - lookup of console (%s) failure: %d", name, ret);
    }

    tty_t *tty = kmalloc(sizeof(tty_t));
    tty->console = vfs_open_file(&out);
    memset(tty->keystate, 0, sizeof(tty->keystate));
    ringbuff_init(&tty->rb, BUFFLEN, char);
    spinlock_init(&tty->lock);

    tty->read_waiting = 0;
    semaphore_init(&tty->wait_semaphore, 0);

    master = tty;

    char_device_t *cdev = char_device_alloc();
    cdev->private = tty;
    cdev->ops = &tty_ops;

    register_char_device(cdev, "tty");

    devfs_publish_pending();

    kprintf("tty - for \"%s\" created", name);
}
