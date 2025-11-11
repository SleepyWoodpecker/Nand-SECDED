#ifndef SECDED_H_
#define SECDED_H_

#include <stdint.h>
#include <stdbool.h>

#define INDIVIDUAL_PARITY_BITS      9 // these are the parity bits that encode a subset of the sequence
#define TOTAL_NUM_PARITY_BITS       (INDIVIDUAL_PARITY_BITS + 1)
#define NUM_32_BIT_COLS_IN_BLOCK    8
#define NUM_COLS_IN_DECODE_MATRIX   (NUM_32_BIT_COLS_IN_BLOCK + 1)
#define NUM_BLOCKS_IN_PAGE          (4096 / 32)

// matrix used to encode the generated message
extern const uint32_t parity_generator_idxs[INDIVIDUAL_PARITY_BITS][NUM_32_BIT_COLS_IN_BLOCK];
// matrix used to decode the received message
extern const uint32_t decode_matrix[][NUM_COLS_IN_DECODE_MATRIX];
// lookup table to determine, based on the result of decoding the matrix, which bit in the sequence was flipped
extern const uint16_t error_location[];

// set flags to define the overall result from decoding the message together with parity bits
#define BIT_CORRECTED               (1 << 0)
#define OVERALL_PARITY_INVALID      (1 << 1)
typedef struct {
  uint8_t response_flags;
  uint16_t bit_position_to_correct;
} DecodeResponse_t;

uint16_t encode_256(const uint32_t raw_data[]);
uint16_t encode_overall_parity(const uint32_t raw_data[], const uint16_t parity_bits);
void decode_256(const uint32_t raw_data[], const uint16_t parity_bits, DecodeResponse_t *decode_response);
bool resolve_decode(uint32_t raw_data[], DecodeResponse_t *decode_response);

typedef struct {
  uint8_t first_section;
  uint8_t second_section; 
} ParityBlock_t;

void encode_page(const uint8_t raw_data[restrict], uint8_t parity_bit_sequences[restrict]);
#endif