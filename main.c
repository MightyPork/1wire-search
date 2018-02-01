#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "ow_search.h"

// ----------- DEMO -----------------

void main(void)
{
    struct ow_search_state search_state;
    ow_search_init(&search_state, 0xFF); // the real onewire search command will go here when used with real hw

    ow_romcode_t addresses[4];

    int x=0;
    while (search_state.status == OW_SEARCH_MORE) {
        uint16_t count = ow_search_run(&search_state, addresses, 4);
        printf("Found %d addresses, status %d\n", count, search_state.status);
        for(int i=0; i<count; i++) {
            for(int j=0; j<8; j++) {
                printf("%02x ", addresses[i][j]);
            }
            printf("\n");
            uint64_t numeric = *((uint64_t*)(void*)&addresses[i][0]);
            printf("n = 0x%016"PRIx64"\n", numeric);
        }
        printf("\n");
        if(++x > 10) break;
    }
}

// ------------ SIMULATOR ---------------

struct owunit {
    ow_romcode_t romcode;
    bool selected;
    uint8_t rompos;
    int state;
};

#define UNITS_COUNT 12
struct owunit units[UNITS_COUNT] = {
    {.romcode = {0b00000000}},//00
    {.romcode = {0b00000001}},//01
    {.romcode = {0b00010001}},//11
    {.romcode = {0b00110001}},//31
    {.romcode = {0b00110101}},//35
    {.romcode = {0b01010001}},//51
    {.romcode = {0b10000000}},//80
    {.romcode = {0b10101010}},//aa
    {.romcode = {0b11110101}},//f5
    {.romcode = {0b11110111}},//f7
    {.romcode = {0xFF,0x00,0xFF,0x00,0x55,0x00,0xAA,0x00}},
    {.romcode = {0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0}},
};

bool rom_device_reset(struct owunit *device)
{
    device->rompos = 0;
    device->state = 0;
    device->selected = true;
}

bool rom_device_read(struct owunit *device)
{
    if (!device->selected) return true;
    if (device->state == 0) {
        device->state++;
        bool res = ow_code_getbit(device->romcode, device->rompos);
        return res;
    }
    if (device->state == 1) {
        device->state++;
        bool res = !ow_code_getbit(device->romcode, device->rompos);
        return res;
    }
}

void rom_device_write(struct owunit *device, bool selectedbit)
{
    if (!device->selected) return;

    if (ow_code_getbit(device->romcode, device->rompos) != selectedbit) {
        device->selected = false;
    }
    else {
        device->rompos++;
        device->state = 0;
    }
}

bool ow_reset(void)
{
    for (int i = 0; i < UNITS_COUNT; ++i) {
        rom_device_reset(&units[i]);
    }
    return (UNITS_COUNT > 0);
}

bool ow_read_bit(void)
{
    bool bus = 1;
    for (int i = 0; i < UNITS_COUNT; ++i) {
        bus &= rom_device_read(&units[i]);
    }
    return bus;
}

void ow_write_bit(bool value)
{
    for (int i = 0; i < UNITS_COUNT; ++i) {
        rom_device_write(&units[i], value);
    }
}

void ow_write_u8(uint8_t value)
{
    // Dummy, do nothing
}
