#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "secded.h"

#define SPARE_MAGIC 0x33

bool check_array(uint32_t decoded_message[]);
bool check_spare(uint8_t spare_region[162]);

void test_encoding() {
    uint32_t message[] = {2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083};

    uint16_t parity_bits = encode_256(message);

    assert(parity_bits == 0b100111011 && "Parity bits do not match");
}

void test_overall_parity() {
    uint32_t odd_message[] = {2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083};
    uint16_t parity_bits = encode_256(odd_message);
    uint16_t overall_parity = encode_overall_parity(odd_message, parity_bits);
    assert(overall_parity == 0b1 && "Overall parity bit does not match");

    // created by flipping one bit 
    uint32_t even_message[] = {2909143769, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083};
    parity_bits = encode_256(odd_message);
    overall_parity = encode_overall_parity(even_message, parity_bits);
    assert(overall_parity == 0b0 && "Overall parity bit does not match");
}

/*
Test all possible combinations of single bit flips
*/
void test_single_bit_flip(uint32_t message[], uint16_t parity_sequence) {
    DecodeResponse_t decode_response;
    // test the flipping of the overall parity_bit
    parity_sequence ^= 1 << INDIVIDUAL_PARITY_BITS;
    decode_256(message, parity_sequence, &decode_response);
    assert(decode_response.response_flags == OVERALL_PARITY_INVALID && decode_response.bit_position_to_correct == 0 && "Detects error when overall parity bit is flipped");
    parity_sequence ^= 1 << INDIVIDUAL_PARITY_BITS;

    // test the flipping of parity bits 
    for (int i = 0; i < INDIVIDUAL_PARITY_BITS; ++i) {
        parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - i);
        decode_256(message, parity_sequence, &decode_response);
        assert(decode_response.response_flags == (OVERALL_PARITY_INVALID | BIT_CORRECTED) && decode_response.bit_position_to_correct == i + 1 && "Detects error when a parity bit that is not the overal parity bit is flipped");
        parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - i);
    }

    // test the flipping of every bit in the message
    for (int i = 0; i < NUM_32_BIT_COLS_IN_BLOCK; ++i) {
        for (int j = 0; j < 32; ++j) {
            message[i] ^= 1 << (32 - 1 - j);
            decode_256(message, parity_sequence, &decode_response);
            assert(decode_response.response_flags == (OVERALL_PARITY_INVALID | BIT_CORRECTED) && decode_response.bit_position_to_correct == i * 32 + j + 10 && "Detects error when a message bit is flipped");
            assert(resolve_decode(message, &decode_response) && "Message is still valid after a bit has been flipped");
            assert(check_array(message) && "Original message can still be recovered");
        }
    }
}

/*
Test all possible combination of double bit flips
*/
void test_double_bit_flip(uint32_t message[], uint16_t parity_sequence) {
    // test flipping 2 bits
    // start with the parity bit first
    DecodeResponse_t decode_response;

    parity_sequence ^= 1 << INDIVIDUAL_PARITY_BITS;
    for (int i = 0; i < INDIVIDUAL_PARITY_BITS; ++i) {
        parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - i);
        decode_256(message, parity_sequence, &decode_response);
        assert(!resolve_decode(message, &decode_response) && "Message should not be valid when overall parity bit and parity bit have been flipped");
        parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - i);
    }

    // test the flipping of every bit in the message
    for (int i = 0; i < NUM_32_BIT_COLS_IN_BLOCK; ++i) {
        for (int j = 0; j < 32; ++j) {
            message[i] ^= 1 << (32 - 1 - j);
            decode_256(message, parity_sequence, &decode_response);
            assert(!resolve_decode(message, &decode_response) && "Message should not be valid after parity bit and data bit have been flipped");
            assert(!check_array(message) && "Array should no longer be valid");
            message[i] ^= 1 << (32 - 1 - j);
        }
    }
    parity_sequence ^= 1 << INDIVIDUAL_PARITY_BITS;

    // test flipping one of the other parity bits
    for (int i = 0; i < INDIVIDUAL_PARITY_BITS; ++i) {
        parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - i);

        for (int j = i + 1; j < INDIVIDUAL_PARITY_BITS; ++j) {
            parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - j);
            decode_256(message, parity_sequence, &decode_response);
            assert(!resolve_decode(message, &decode_response) && "Message should no longer be valid when two parity bits have been flipped");
            parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - j);
        }

        for (int k = 0; k < NUM_32_BIT_COLS_IN_BLOCK; ++k) {
            for (int j = 0; j < 32; ++j) {
                message[k] ^= 1 << (32 - 1 - j);
                decode_256(message, parity_sequence, &decode_response);
                assert(!resolve_decode(message, &decode_response) && "Message should no longer be valid when a parity bit and a data bit have been flipped");
                assert(!check_array(message) && "Array should no longer be valid");
                message[k] ^= 1 << (32 - 1 - j);
            }
        }

        parity_sequence ^= 1 << (INDIVIDUAL_PARITY_BITS - 1 - i);
    }

    assert(check_array(message));

    // test flipping of 2 data bits
    for (int i = 0; i < NUM_32_BIT_COLS_IN_BLOCK; ++i) {
        for (int ii = 0; ii < 32; ++ii) {
            message[i] ^= 1 << (32 - 1 - ii);

            for (int k = 0; k < NUM_32_BIT_COLS_IN_BLOCK; ++k) {
                for (int j = 0; j < 32; ++j) {
                    // avoid flipping the same bit twice
                    if (i == k && ii == j) {
                        continue;
                    }

                    message[k] ^= 1 << (32 - 1 - j);
                    decode_256(message, parity_sequence, &decode_response);
                    assert(!resolve_decode(message, &decode_response) && "Message should no longer be valid when a parity bit and a data bit have been flipped");
                    assert(!check_array(message) && "Array should no longer be valid");
                    message[k] ^= 1 << (32 - 1 - j);
                }
            }

            message[i] ^= 1 << (32 - 1 - ii);
        }
    }
}

/*
Test the decode function by checking that:
1. No error is raised when there are no bit flips
2. Single bit flip can be corrected and no error is raised
3. Double bit flips cannot be corrected and error is raised
*/
void test_decode() {
    uint32_t message[] = {2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083};
    uint16_t parity_bits = encode_256(message);
    uint16_t overall_parity = encode_overall_parity(message, parity_bits);

    uint16_t parity_sequence = overall_parity << INDIVIDUAL_PARITY_BITS | parity_bits;

    DecodeResponse_t decode_response;
    decode_256(message, parity_sequence, &decode_response);

    assert(decode_response.response_flags == 0 && decode_response.bit_position_to_correct == 0 && "No error detected when there is no bit flip");

    test_single_bit_flip(message, parity_sequence);
    test_double_bit_flip(message, parity_sequence);
}

void test_page_encode() {
    // simulate 4096 byte block
    uint32_t message[] = {
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
        2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083,
    };

    uint8_t spare_block[162];
    memset(spare_block, 0, 162);
    // mark the first and last byte to make sure there is no out of bounds access
    spare_block[0] = SPARE_MAGIC;
    spare_block[161] = SPARE_MAGIC;

    encode_page((uint8_t *)message, spare_block + 1);
    assert(check_spare(spare_block) && "Spare region was incorrectly created");
}

int main(void) { 
    test_encoding(); 
    test_overall_parity();
    test_decode();
    test_page_encode();
}

bool check_array(uint32_t decoded_message[]) {
    uint32_t message[] = {2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083};
    for (int i = 0; i < 8; ++i) {
        if (message[i] != decoded_message[i]) {
            return false;
        }
    }

    return true;
}

bool check_spare(uint8_t spare_region[162]) {
    assert(spare_region[0] == SPARE_MAGIC && "Spare region at start was overridden");
    assert(spare_region[161] == SPARE_MAGIC && "Spare region at end was overridden");
    spare_region += 1;

    uint8_t expected_spare[160] = {
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
        0b11001110, 0b11110011, 0b10111100, 0b11101111, 0b00111011,
    };
    
    for (int i = 0; i < 160; ++i) {
        if (spare_region[i] != expected_spare[i]) {
            printf("position: %d was incorrect | received: %d, expected: %d\n", i, spare_region[i], expected_spare[i]);
            return false;
        };
    }

    return true;
}