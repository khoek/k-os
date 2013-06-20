#include "ethernet.h"
#include "log.h"

void ethernet_handle(uint8_t *packet, uint16_t length) {
    logf("ethernet - packet (0x%p) of length %u bytes recieved", packet, length);
}
