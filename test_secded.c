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

int main(void) { 
    test_bit_sequence_parity();
    test_encoding(); 
}