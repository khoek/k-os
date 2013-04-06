#include "console.h"
#include "keyboard.h"
#include "idt.h"

#define SHELLCOLUMN 2

static uint8_t prompt_row;

static void key_pressed(char input) {
   switch (input) {
      case '\n':
         console_puts("\n$ ");
         prompt_row = console_row();
         break;
      case '\x7f':
         if (!(console_row() == prompt_row && console_col() <= SHELLCOLUMN)) {
            if (console_col() == 0) {
                console_cursor(console_row() - 1, CONSOLE_WIDTH - 1);
                console_puts(" ");
                console_cursor(console_row() - 1, CONSOLE_WIDTH - 1);
            } else {
                console_cursor(console_row(), console_col() - 1);
                console_puts(" ");
                console_cursor(console_row(), console_col() - 1);
            }
         }
         break;
      case '\0':
      case '\t':
         break;
      default:
         if (!(prompt_row == 0 && console_row() == CONSOLE_HEIGHT - 1 && console_col() == CONSOLE_WIDTH - 1)) {
             if (prompt_row != 0 && console_col() == 0) {
                 prompt_row--;
             }

             console_write(&input, 1);
         }

         break;
   }
}

void run_shell() {
   sti();

   keyboard_register_key_down(&key_pressed);

   console_clear();
   kprintf("Welcome to K-OS! Written By Keeley Hoek (escortkeel)\n\n$ "); 
   prompt_row = console_row();

   while(1) hlt();
}
