#ifndef SECDED_H_
#define SECDED_H_

#include <stdint.h>
#include <stdbool.h>

#define NUM_PARITY_BITS 9
#define NUM_32_BIT_COLS_IN_BLOCK 8
#define NUM_COLS_IN_DECODE_MATRIX (NUM_32_BIT_COLS_IN_BLOCK + 1)

// set flags to define the overall result from decoding the message together with parity bits
#define BIT_CORRECTED (1 << 0)
#define OVERALL_PARITY_INVALID (1 << 1)

extern const uint32_t parity_generator_idxs[NUM_PARITY_BITS][NUM_32_BIT_COLS_IN_BLOCK];
// TODO: last number should not be 32 bits :/
extern const uint32_t decode_matrix[][NUM_COLS_IN_DECODE_MATRIX];
extern const uint16_t error_location[];

typedef struct {
  uint8_t response_flags;
  uint16_t bit_position_to_correct;
} DecodeResponse_t;

// encode a 256 byte block and return the parity bits
uint16_t encode_256(const uint32_t raw_data[]);
uint16_t encode_overall_parity(const uint32_t raw_data[], const uint16_t parity_bits);
void decode_256(const uint32_t raw_data[], const uint16_t parity_bits, DecodeResponse_t *decode_response);
bool resolve_decode(uint32_t raw_data[], DecodeResponse_t *decode_response);

uint8_t bit_sequence_parity(uint32_t input);

#endif