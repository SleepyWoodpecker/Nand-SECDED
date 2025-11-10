#ifndef SECDED_H_
#define SECDED_H_

#include <stdint.h>

#define NUM_PARITY_BITS 9
#define NUM_32_BIT_COLS_IN_BLOCK 8

extern const uint32_t parity_generator_idxs[NUM_PARITY_BITS][NUM_32_BIT_COLS_IN_BLOCK];
// TODO: last number should not be 32 bits :/
extern const uint32_t decode_matrix[][9];
extern const uint16_t error_location[];

// encode a 256 byte block and return the parity bits
uint16_t encode_256(const uint32_t raw_data[]);
uint16_t encode_overall_parity(const uint32_t raw_data[], const uint16_t parity_bits);
void decode_256(const uint8_t *raw_data, uint8_t *decoded_data);

uint8_t bit_sequence_parity(uint32_t input);

#endif