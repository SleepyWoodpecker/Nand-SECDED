#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "secded.h"

void test_bit_sequence_parity() {
    uint16_t test = 0b1001001;
    assert(bit_sequence_parity(test) == 0b1 && "Able to correct for odd parity");

    test = 0b1000100;
    assert(bit_sequence_parity(test) == 0b0 && "Able to correct for even parity");
}

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

void test_decode() {
    uint32_t message[] = {2909143768, 1204695435, 1134521375, 3492009847, 3660384855, 2363530907, 1281558073, 3101431083};
    uint16_t parity_bits = encode_256(message);
    uint16_t overall_parity = encode_overall_parity(message, parity_bits);

    uint16_t parity_sequence = overall_parity << NUM_PARITY_BITS | parity_bits;

    DecodeResponse_t decode_response;
    decode_256(message, parity_sequence, &decode_response);

    assert(decode_response.response_flags == 0 && decode_response.bit_position_to_correct == 0 && "No error detected when there is no bit flip");

    // test the flipping of the overall parity_bit
    parity_sequence ^= 1 << NUM_PARITY_BITS;
    decode_256(message, parity_sequence, &decode_response);
    assert(decode_response.response_flags == OVERALL_PARITY_INVALID && decode_response.bit_position_to_correct == 0 && "Detects error when overall parity bit is flipped");
    parity_sequence ^= 1 << NUM_PARITY_BITS;

    // test the flipping of parity bits 
    for (int i = 0; i < NUM_PARITY_BITS; ++i) {
        parity_sequence ^= 1 << (NUM_PARITY_BITS - 1 - i);
        decode_256(message, parity_sequence, &decode_response);
        assert(decode_response.response_flags == (OVERALL_PARITY_INVALID | BIT_CORRECTED) && decode_response.bit_position_to_correct == i + 1 && "Detects error when a parity bit that is not the overal parity bit is flipped");
        parity_sequence ^= 1 << (NUM_PARITY_BITS - 1 - i);
    }

    // test the flipping of every bit in the message
    for (int i = 0; i < NUM_32_BIT_COLS_IN_BLOCK; ++i) {
        for (int j = 0; j < 32; ++j) {
            message[i] ^= 1 << (32 - 1 - j);
            decode_256(message, parity_sequence, &decode_response);
            assert(decode_response.response_flags == (OVERALL_PARITY_INVALID | BIT_CORRECTED) && decode_response.bit_position_to_correct == i * 32 + j + 10 && "Detects error when a message bit is flipped");
            message[i] ^= 1 << (32 - 1 - j);
        }
    }
}

int main(void) { 
    test_bit_sequence_parity();
    test_encoding(); 
    test_overall_parity();
    test_decode();
}