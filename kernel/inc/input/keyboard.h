#ifndef KERNEL_INPUT_KEYBOARD_H
#define KERNEL_INPUT_KEYBOARD_H

#include <stdbool.h>

#define B_KEY      0x30
#define C_KEY      0x2e
#define S_KEY      0x1f

#define CAPS_KEY   0x3a
#define LSHIFT_KEY 0x2a
#define RSHIFT_KEY 0x36
#define CTRL_KEY   0x1d
#define ALT_KEY    0x38
#define SYSRQ_KEY  0x54

void keyboard_register_key_up(void (*handler)(char));
void keyboard_register_key_down(void (*handler)(char));
bool shift_down();
bool control_down();
bool alt_down();

#endif
