//
// Created by MightyPork on 2018/02/01.
// MIT license
//

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ow_search.h"

// -------- CHECKSUM --------

static inline uint8_t crc8_bits(uint8_t data)
{
    uint8_t crc = 0;
    if(data & 1)     crc ^= 0x5e;
    if(data & 2)     crc ^= 0xbc;
    if(data & 4)     crc ^= 0x61;
    if(data & 8)     crc ^= 0xc2;
    if(data & 0x10)  crc ^= 0x9d;
    if(data & 0x20)  crc ^= 0x23;
    if(data & 0x40)  crc ^= 0x46;
    if(data & 0x80)  crc ^= 0x8c;
    return crc;
}

static uint8_t crc8_add(uint8_t cksum, uint8_t byte)
{
    return crc8_bits(byte ^ cksum);
}

uint8_t ow_checksum(const uint8_t *buff, uint16_t len)
{
    uint8_t cksum = 0;
    for(uint16_t i = 0; i < len; i++) {
        cksum = crc8_add(cksum, buff[i]);
    }
    return cksum;
}

// -----------------------------


void ow_search_init(struct ow_search_state *state, uint8_t command, bool test_checksums)
{
    state->prev_last_fork = 64;
    memset(state->prev_code, 0, 8);
    state->status = OW_SEARCH_MORE;
    state->command = command;
    state->first = true;
    state->test_checksums = test_checksums;
}

uint16_t ow_search_run(struct ow_search_state *state, ow_romcode_t *codes, uint16_t capacity)
{
    if (state->status != OW_SEARCH_MORE) return 0;

    uint16_t found_devices = 0;

    while (found_devices < capacity) {
        uint8_t index = 0;
        ow_romcode_t code = {};
        int8_t last_fork = -1;

        // Start a new transaction. Devices respond to reset
        if (!ow_reset()) {
            state->status = OW_SEARCH_FAILED;
            goto done;
        }
        // Send the search command (SEARCH_ROM, SEARCH_ALARM)
        ow_write_u8(state->command);

        uint8_t *code_byte = &code[0];

        bool p, n;
        while (index != 64) {
            // Read a bit and its complement
            p = ow_read_bit();
            n = ow_read_bit();

            if (!p && !n) {
                // A fork: there are devices on the bus with different bit value
                // (the bus is open-drain, in both cases one device pulls it low)
                if ((found_devices > 0 || !state->first) && index < state->prev_last_fork) {
                    // earlier than the last fork, take the same turn as before
                    p = ow_code_getbit(state->prev_code, index);
                    if (!p) last_fork = index; // remember for future runs, 1 not explored yet
                }
                else if (index == state->prev_last_fork) {
                    p = 1; // both forks are now exhausted
                }
                else { // a new fork
                    last_fork = index;
                }
            }
            else if (p && n) {
                // No devices left connected - this doesn't normally happen
                state->status = OW_SEARCH_FAILED;
                goto done;
            }

            // All devices have a matching bit here, or it was resolved in a fork
            if (p) *code_byte |= (1 << (index & 7));
            ow_write_bit(p);

            index++;
            if((index & 7) == 0) {
                code_byte++;
            }
        }

        memcpy(state->prev_code, code, 8);

        if (!state->test_checksums || 0 == ow_checksum(code, 8)) {
            // Record a found address
            memcpy(codes[found_devices], code, 8);
            found_devices++;
        }

        // Stop condition
        if (last_fork == -1) {
            state->status = OW_SEARCH_DONE;
            goto done;
        }

        state->prev_last_fork = last_fork;
    }

done:
    state->first = false;
    return found_devices;
}
