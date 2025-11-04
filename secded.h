#ifndef SECDED_H_
#define SECDED_H_

#include <stdint.h>

#define NUM_PARITY_BITS 9
#define PARITY_GENERATOR_NUM_32_BIT_COLS 8

extern const uint32_t parity_generator_idxs[NUM_PARITY_BITS][PARITY_GENERATOR_NUM_32_BIT_COLS];
// TODO: last number should not be 32 bits :/
extern const uint32_t decode_matrix[][9];
extern const uint16_t error_location[];

// encode a 256 byte block and return the parity bits
uint16_t encode_256(const uint32_t *raw_data);
void decode_256(const uint8_t *raw_data, uint8_t *decoded_data);

#endif