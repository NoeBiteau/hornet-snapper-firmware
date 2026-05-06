#include <assert.h>
#include "../crc16.h"

int main(void) {
    /* Standard CRC-16/CCITT-FALSE check vector: "123456789" → 0x29B1 */
    const uint8_t v[] = "123456789";
    uint16_t got = hs_crc16_ccitt(v, 9);
    assert(got == 0x29B1);

    /* Empty input must return init value 0xFFFF. */
    assert(hs_crc16_ccitt((const uint8_t *)"", 0) == 0xFFFF);

    /* Single byte 0xA1: known result 0x443B (CRC-16/CCITT-FALSE, poly=0x1021, init=0xFFFF) */
    const uint8_t one = 0xA1;
    assert(hs_crc16_ccitt(&one, 1) == 0x443B);

    return 0;
}
