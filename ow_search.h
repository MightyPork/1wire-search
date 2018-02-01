//
// Created by MightyPork on 2018/02/01
// MIT license
//

#ifndef OW_SEARCH_H
#define OW_SEARCH_H

#include <stdint.h>
#include <stdbool.h>

// --------------------------------------------------------------------------------------

// External API functions for interfacing the bus
// Customize as needed (and also update calls in the search function)

/**
 * Reset the 1-wire bus
 *
 * @return presence pulse received
 */
extern bool ow_reset(void);

/**
 * Write a byte to the bus
 *
 * @param[in] value byte value
 */
extern void ow_write_u8(uint8_t value);
/**
 * Write a bit to the 1-wire bus
 *
 * @param[in] value bit value
 */
extern void ow_write_bit(bool value);

/**
 * Read a bit from the 1-wire bus
 *
 * @return bit value
 */
extern bool ow_read_bit(void);

// --------------------------------------------------------------------------------------

/**
 * Data type holding a romcode
 */
typedef uint8_t ow_romcode_t[8];

/**
 * Get a single bit from a romcode
 */
#define ow_code_getbit(code, index) (bool)((code)[(index) >> 3] & (1 << ((index) & 7)))

/**
 * Convert to unsigned 64-bit integer
 * (works only on little-endian systems - eg. OK on x86/x86_64, not on PowerPC)
 */
#define ow_romcode_to_u64(code) (*((uint64_t *) (void *)(code)))

/**
 * States of the search algorithm
 */
enum ow_search_result {
    OW_SEARCH_DONE = 0,
    OW_SEARCH_MORE = 1,
    OW_SEARCH_FAILED = 2,
};

/**
 * Internal state of the search algorithm.
 * Check status to see if more remain to be read or an error occurred.
 */
struct ow_search_state {
    int8_t prev_last_fork;
    ow_romcode_t prev_code;
    uint8_t command;
    enum ow_search_result status;
    bool first;
    bool test_checksums;
};

/**
 * Init the search algorithm state structure
 *
 * @param[out] state - inited struct
 * @param[in] command - command to send for requesting the search (e.g. SEARCH_ROM)
 * @param[in] test_checksums - verify checksums of all read romcodes
 */
void ow_search_init(struct ow_search_state *state, uint8_t command, bool test_checksums);

/**
 * Perform a search of the 1-wire bus, with a state struct pre-inited
 * using ow_search_init().
 *
 * Romcodes are stored in the provided array in a numerically ascending order.
 *
 * This function may be called repeatedly to retrieve more addresses than could fit
 * in the address buffer.
 *
 * @param[in,out] state - search state, used for multiple calls with limited buffer size
 * @param[out] codes - buffer for found romcodes
 * @param[in] capacity - buffer capacity
 * @return number of romcodes found. Search status is stored in state->status
 */
uint16_t ow_search_run(struct ow_search_state *state, ow_romcode_t *codes, uint16_t capacity);

/**
 * Compute a 1-wire type checksum.
 * If the buffer includes the checksum, the result should be 0.
 *
 * (this function may be used externally, or you can delete the implementation
 * from the c file if another implementation is already available)
 *
 * @param[in] buf - buffer of bytes to verify
 * @param[in] len - buffer length
 * @return checksum
 */
uint8_t ow_checksum(const uint8_t *buf, uint16_t len);

#endif //OW_SEARCH_H
