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

int main(void) { 
    test_bit_sequence_parity();
    test_encoding(); 
    test_overall_parity();
}