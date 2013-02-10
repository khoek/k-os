#include <stdbool.h>
#include <string.h>
#include "erasure_tool.h"
#include "common.h"
#include "idt.h"
#include "pit.h"
#include "mm.h"
#include "ide.h"
#include "console.h"
#include "keyboard.h"
#include "io.h"

static char key;

static void handle_down(char code) {
   key = code;
}

static char get_key() {
   while(!key) hlt();

   char temp = key;
   key = 0;
   return temp;
}

static void wait_for_enter() {
   while(get_key() != '\n');
}

static void print_header() {
   console_clear();

   kprintf("Welcome to the K-OS Secure Data Erasure Tool!\n\nCopyright (c) 2013 Keeley Hoek\nAll rights reserved.\n\n");

   console_color(0x0C);
   kprintf("WARNING: ");
   console_color(0x07);
   kprintf("This software irreversably and indescriminatley destroys data.\n\n");
}

static void print_tech_info() {
   console_color(0x0E);
   kprintf("Technical Information\n");
   console_color(0x07);

   kprintf("The K-OS Secure Data Erasure Tool works by repeatadly writing over the contents of attached ATA drives. ");
   kprintf("Normal HDD erasure involves (a maximum of) a single pass over the drive where all bits are set to zero.");

   kprintf("\n\nIt has been postulated by some that the \"deleted\" bits of the HDD could potentially be recovered, and so this method of deletion is not very secure. ");
   kprintf("This tool attempts to remedy the problem by providing a simple means of overwriting HDDs using multiple passes. ");
   kprintf("Simply enter the number of passes that you would like the tool to write, and it will do the rest for you.");

   kprintf("\n\nFor every odd pass, the tool will write zeroes to the disk, and for every even pass, the tool will write ones to the disk.");

   kprintf("\n\n\n\n\nPress \"ENTER\" go back.");

   wait_for_enter();
}

static void run_tool() {
   kprintf("Please select the drives which you would like to erase.\n");

   uint8_t highlighted = 1;
   bool selected[4] = {0,0,1,0};

   bool selecting = true;
   while (selecting) {
      for (uint8_t i = 0; i < 4; i++) {
         if (ide_device_is_present(i) && ide_device_get_type(i) == 0) {
            uint8_t foreground = 0x7, background = 0x0;
            if (selected[i]) foreground = 0xC;
            if (highlighted == i) {
               background = 0xF;
               if (foreground == 0x7) foreground = 0x0;
            }

            console_color((background << 4) + foreground);
            kprintf("   - ATA Device %u %7uMB - %s\n", i, ide_device_get_size(i) / 1024 / 2, ide_device_get_string(i, IDE_STRING_MODEL));
         }
      }

      kprintf("\n\nUse the \"ARROW KEYS\" to highlight a drive.\n");
      kprintf("Press \"SPACE\"  to select/deselect the currently highlighted drive.\n\n");
      kprintf("Press \"ENTER\"  to continue.\n");
      kprintf("Press \"ESCAPE\" to go back.");

      switch (get_key()) {
         case '\1':
            return;
         case '\n':
            selecting = false;
            break;
      }
   }

   kprintf("Please enter the number of passes to be performed (max 99): ");

   uint8_t idx = 0;
   char raw_passes[3];

   char c;
   do {
      c = get_key();

      if (c == '\x7f' && idx > 0) {
         idx--;
         console_cursor(console_row(), console_col() - 1);
         kprintf(" ");
         console_cursor(console_row(), console_col() - 1);
      } else if (c >= '0' && c <= '9' && idx < 2) {
         kprintf("%c", c);
         raw_passes[idx] = c;
         idx++;
      }
   } while (!(c == '\n' && idx != 0));

   raw_passes[idx] = '\0';
   uint32_t passes = atoi(raw_passes);

   kprintf("\n\nPress enter to ");
   console_color(0x0C);
   kprintf("WIPE ALL ATTACHED ATA Disks");
   console_color(0x07);
   kprintf(" with %u passes.\nPress ESCAPE to cancel and go back.\n\n", passes);

   bool wait = true;
   while (wait) {
      switch (get_key()) {
         case '\1':
            return;
            break;
         case '\n':
            wait = false;
      }
   }

   console_clear();

   kprintf("Starting Data Erasure Tool...\n\n");

   uint8_t *space = (uint8_t *) page_to_address(alloc_page());
   for (uint32_t i = 1; i < 64; i++) {
      alloc_page();
      kprintf("Initializing... %3u%%\r", (i * 100) / 64);
   }
   kprintf("Initializing... 100%%\n");

   uint8_t bit = 0x00;
   for (uint32_t pass = 0; pass < passes; pass++) {
      console_clear();

      kprintf("Starting Data Erasure Tool...\n\n");

      kprintf("Initializing... 100%%\n");

      kprintf("\nWriting Pass %02u of %02u...\n", pass + 1, passes);
      for (uint32_t i = 0; i < (64 * 4 * 1024); i++) {
         space[i] = bit;
      }

      for (uint32_t i = 0; i < 4; i++) {
         if (selected[i]) {
            uint32_t written = 0;
            while(written < ide_device_get_size(i)) {
               written += ide_write_sectors_same(i, ide_device_get_size(i) - written, written, space) / 512;
               kprintf("   - Writing to device %u... %3u%%\r", i, (written * 100) / ide_device_get_size(i));
            }
            kprintf("   - Writing to device %u... 100%\n", i);
         }
      }

      bit = ~bit;
   }

   console_color(0x0A);
   kprintf("\nOperation completed successfully! ");
   console_color(0x07);
   kprintf("You may now turn the computer off.");

   beep();
}

static void main_menu() {
   while (true) {
      print_header();

      kprintf("It will attempt to completley destory ALL DATA on ALL ATTACHED ATA Disks.\n\n");

      kprintf("The following devices have been detected:\n");

      uint8_t devices = 0;
      for (uint8_t i = 0; i < 4; i++) {
         if (ide_device_is_present(i) && ide_device_get_type(i) == 0) {
            kprintf("   - ATA Device %u %7uMB - %s\n", i, ide_device_get_size(i) / 1024 / 2, ide_device_get_string(i, IDE_STRING_MODEL));
            devices++;
         }
      }

      if (devices == 0) {
         console_color(0x0C);
         kprintf("No devices have been detected. Running this tool will not do anything.\n");
         console_color(0x07);
      }

      for (uint8_t i = 0; i < 12 - devices + (devices == 0 ? 0 : 1); i++) {
         kprintf("\n");
      }

      kprintf("Press \"i\"     to view technical information.\n");
      kprintf("Press \"ENTER\" to continue.");

      char c = get_key();
      print_header();
      switch (c) {
         case 'i':
            print_tech_info();
            break;
         case '\n':
            run_tool();
            break;
      }
   }
}

void erasure_tool_run() {
   keyboard_register_key_down(handle_down);

   console_clear();
   kprintf("Welcome to K-OS!\n\nCopyright (c) 2013 Keeley Hoek\nAll rights reserved.\n\n");

   kprintf("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n");
   kprintf("ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n");
   kprintf("WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
   kprintf("DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR\n");
   kprintf("ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n");
   kprintf("(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n");
   kprintf("LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n");
   kprintf("ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
   kprintf("(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n");
   kprintf("SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n");

   kprintf("\n\n\n\n\n\n\n");
   kprintf("If you do not agree to the above terms TURN THE COMPUTER OFF NOW.\n");
   kprintf("If you do agree to the above terms press enter to continue.");
   
   beep();

   wait_for_enter();

   main_menu();
}
