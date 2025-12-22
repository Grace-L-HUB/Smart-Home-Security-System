#include <stdio.h>
#include "Hardware/W25Q64.h"

int main() {
    printf("Size of DataRecord_t: %d bytes\n", sizeof(DataRecord_t));
    printf("Size of uint32_t: %d bytes\n", sizeof(uint32_t));
    printf("Size of uint8_t: %d bytes\n", sizeof(uint8_t));
    printf("Size of uint16_t: %d bytes\n", sizeof(uint16_t));
    return 0;
}