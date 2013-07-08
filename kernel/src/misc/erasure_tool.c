#include <stdbool.h>

#include "lib/string.h"
#include "common/asm.h"
#include "bug/panic.h"
#include "arch/idt.h"
#include "mm/mm.h"
#include "driver/disk/ide.h"
#include "sound/pit.h"
#include "video/console.h"
#include "input/keyboard.h"
#include "misc/erasure_tool.h"

static char twirls[] = {
    '|', '/', '-', '\\'
};

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

    console_puts("Welcome to the K-OS Secure Data Erasure Tool!\n\nCopyright (c) 2013 Keeley Hoek\nAll rights reserved.\n\n");

    console_color(0x0C);
    console_puts("WARNING: ");
    console_color(0x07);
    console_puts("This software irreversably and indescriminatley destroys data.\n\n");
}

static void print_tech_info() {
    console_clear();
    print_header();

    console_color(0x0E);
    console_puts("Technical Information\n");
    console_color(0x07);

    console_puts("The K-OS Secure Data Erasure Tool works by repeatadly writing over the contents of attached ATA drives. ");
    console_puts("Normal HDD erasure involves (a maximum of) a single pass over the drive where all bits are set to zero.");

    console_puts("\n\nIt has been postulated by some that the \"deleted\" bits of the HDD could potentially be recovered, and so this method of deletion is not very secure. ");
    console_puts("This tool attempts to remedy the problem by providing a simple means of overwriting HDDs using multiple passes. ");
    console_puts("Simply enter the number of passes that you would like the tool to write, and it will do the rest for you.");

    console_puts("\n\nFor every odd pass, the tool will write zeroes to the disk, and for every even pass, the tool will write ones to the disk.");

    console_puts("\n\n\n\n\nPress \"ENTER\" go back.");

    wait_for_enter();
}

static void run_tool(uint8_t devices) {
    uint8_t highlighted;

    for (uint8_t i = 0; i < 4; i++) {
        if (ide_device_is_present(i) && ide_device_get_type(i) == 0) {
            highlighted = i;
            break;
        }
    }

    bool selected[4] = {0,0,0,0};
    bool selecting = true;
    uint8_t selectedNum = 0;

    while (selecting) {
        console_clear();
        print_header();

        console_puts("Please select the drives which you would like to erase.\n");

        for (uint8_t i = 0; i < 4; i++) {
            if (ide_device_is_present(i) && ide_device_get_type(i) == 0) {
                uint8_t foreground = 0x7, background = 0x0;
                if (selected[i]) foreground = 0xC;
                if (highlighted == i) {
                    background = 0xF;
                    if (foreground == 0x7) foreground = 0x0;
                }

                console_color((background << 4) + foreground);
                console_putsf("    - ATA Device %u (%s) %7uMB - %s\n", i, ide_device_is_dma_capable(i) ? "DMA" : "PIO", ide_device_get_size(i) / 1024 / 2, ide_device_get_string(i, IDE_STRING_MODEL));
                console_color(0x07);
            }
        }

        for (int i = 0; i < 10 - devices; i++) console_puts("\n");

        console_puts("Use the \"ARROW KEYS\" to highlight a drive.\n");
        console_puts("Press \"SPACE\"  to select/deselect the currently highlighted drive.\n\n");

        console_puts("NOTE: You must select at least one drive to continue.\n\n");

        console_puts("Press \"ENTER\"  to continue.\n");
        console_puts("Press \"ESCAPE\" to go back.");

        switch (get_key()) {
            case '\n':
                if (selectedNum > 0) {
                    selecting = false;
                }
                break;
            case '\1':
                return;
            case '\3':
                do {
                    highlighted--;
                    if (highlighted > 3) highlighted = 3;
                } while (!(ide_device_is_present(highlighted) && ide_device_get_type(highlighted) == 0));
                break;
            case '\5':
                do {
                    highlighted++;
                    if (highlighted == 4) highlighted = 0;
                } while (!(ide_device_is_present(highlighted) && ide_device_get_type(highlighted) == 0));
                break;
            case ' ':
                selected[highlighted] = !selected[highlighted];
                selectedNum += selected[highlighted] ? 1 : -1;
                break;
        }
    }

    console_clear();
    print_header();

    console_puts("Please select the drives which you would like to erase.\n");

    for (uint8_t i = 0; i < 4; i++) {
        if (ide_device_is_present(i) && ide_device_get_type(i) == 0) {
            uint8_t foreground = 0x7, background = 0x0;
            if (selected[i]) foreground = 0xC;

            console_color((background << 4) + foreground);
            console_putsf("    - ATA Device %u (%s) %7uMB - %s\n", i, ide_device_is_dma_capable(i) ? "DMA" : "PIO", ide_device_get_size(i) / 1024 / 2, ide_device_get_string(i, IDE_STRING_MODEL));
            console_color(0x07);
        }
    }

    console_puts("\n\nPlease enter the number of passes to be performed (max 99, 0 to cancel): ");

    uint8_t idx = 0;
    char raw_passes[3];

    char c;
    do {
        c = get_key();

        if (c == '\x7f' && idx > 0) {
            idx--;
            console_cursor(console_row(), console_col() - 1);
            console_puts(" ");
            console_cursor(console_row(), console_col() - 1);
        } else if (c >= '0' && c <= '9' && idx < 2) {
            console_putsf("%c", c);
            raw_passes[idx] = c;
            idx++;
        }
    } while (!(c == '\n' && idx != 0));

    raw_passes[idx] = '\0';
    uint32_t passes = atoi(raw_passes);

    if (passes == 0) return;

    console_puts("\n\nPress \"ENTER\"  to ");
    console_color(0x0C);
    console_puts("WIPE THE ATA DISKS SELECTED ABOVE");
    console_color(0x07);
    console_putsf(" in %u pass", passes);

    if (passes != 1) console_puts("es");

    console_puts(".\nPress \"ESCAPE\" to cancel and go back.\n\n");

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

    console_puts("Starting Data Erasure Tool...\n\n");

    uint8_t *space = (uint8_t *) page_to_virt(alloc_page(0));
    for (uint32_t i = 1; i < 63; i++) {
        //alloc_page();
        console_putsf("Initializing... %3u%%\r", ((i + 1) * 100) / 64);
    }
    console_puts("Initializing... 100%%\n");

    uint8_t bit = 0x00;
    for (uint32_t pass = 0; pass < passes; pass++) {
        console_clear();

        console_puts("Starting Data Erasure Tool...\n\n");

        console_putsf("Initializing... 100%%\n", space);

        console_putsf("\nWriting Pass %02u of %02u...\n", pass + 1, passes);
        for (uint32_t i = 0; i < (64 * 4 * 1024); i++) {
            space[i] = bit;
        }

        int device = 0;
        for (uint32_t i = 0; i < 4; i++) {
            if (selected[i]) {
                device++;
                uint32_t count = 0;
                uint32_t written = 0;
                while(written < ide_device_get_size(i)) {
                    count++;
                    console_putsf("    - Writing to device %u... %3u%%                                             %c\r", device, (written * 100) / ide_device_get_size(i), twirls[(count / 10) % 4]);
                    written += ide_write_sectors_same(i, ide_device_get_size(i) - written, written, space) / 512;
                }
                console_putsf("    - Writing to device %u... 100%%                                              \n", device);
            }
        }

        bit = ~bit;
    }

    console_color(0x0A);
    console_puts("\nOperation completed successfully! ");
    console_color(0x07);
    console_puts("You may now turn the computer off.");

    beep();
    die();
}

static void main_menu() {
    while (true) {
        print_header();

        console_puts("It will attempt to completley destory ALL DATA on SELECTED ATA DISKS.\n\n");

        console_puts("The following devices have been detected:\n");

        uint8_t devices = 0;
        for (uint8_t i = 0; i < 4; i++) {
            if (ide_device_is_present(i) && ide_device_get_type(i) == 0) {
                console_putsf("    - ATA Device %u (%s) %7uMB - %s\n", i, ide_device_is_dma_capable(i) ? "DMA" : "PIO", ide_device_get_size(i) / 1024 / 2, ide_device_get_string(i, IDE_STRING_MODEL));
                devices++;
            }
        }

        if (devices == 0) {
            console_color(0x0C);
            console_puts("\nNo devices have been detected. You cannot run this tool.\n");
            console_color(0x07);
        }

        for (uint8_t i = 0; i < 12 - devices + 1 - (devices ? 0 : 2); i++) console_puts("\n");

        console_puts("Press \"i\"      to view technical information.\n");

        if (devices == 0) {
            console_puts("You may turn the computer off.");
        } else {
            console_puts("Press \"ENTER\" to continue.");
        }

        char c = get_key();
        switch (c) {
            case 'i':
                print_tech_info();
                break;
            case '\n':
                if(devices) run_tool(devices);
                break;
        }
    }
}

void erasure_tool_run() {
    keyboard_register_key_down(handle_down);

    console_clear();
    console_puts("Welcome to K-OS!\n\nCopyright (c) 2013 Keeley Hoek\nAll rights reserved.\n\n");

    console_puts("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND\n");
    console_puts("ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n");
    console_puts("WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
    console_puts("DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR\n");
    console_puts("ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n");
    console_puts("(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n");
    console_puts("LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n");
    console_puts("ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
    console_puts("(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n");
    console_puts("SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n");

    console_puts("\n\n\n\n\n\n\n");
    console_puts("If you do not agree to the above terms TURN THE COMPUTER OFF NOW.\n");
    console_puts("If you do agree to the above terms press enter to continue.");

    beep();

    wait_for_enter();

    main_menu();
}
