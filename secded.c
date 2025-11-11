#include "secded.h"

#include <stdint.h>

#include <stdio.h>

uint16_t get_message_parity(const uint32_t raw_data[]) {
  uint16_t current_parity_sum = 0;
  for (int i = 0; i < NUM_32_BIT_COLS_IN_BLOCK; ++i) {
    current_parity_sum ^= bit_sequence_parity(raw_data[i]);
  }
  return current_parity_sum;
}

uint16_t encode_256(const uint32_t raw_data[]) {
  // calculate parity bits for data
  uint16_t parity_bits = 0;
  for (int i = 0; i < NUM_PARITY_BITS; ++i) {
    int sequence_parity = 0;
    for (int j = 0; j < NUM_32_BIT_COLS_IN_BLOCK; ++j) {
      sequence_parity ^= bit_sequence_parity(raw_data[j] & parity_generator_idxs[i][j]);
    }
    parity_bits <<= 1;
    parity_bits |= sequence_parity;
  }

  // expect 9 bits to come back from parity encoding
  return parity_bits & 0x1ff;
}

uint16_t encode_overall_parity(const uint32_t raw_data[], const uint16_t parity_bits) {
  uint16_t current_parity_sum = get_message_parity(raw_data);
  uint16_t sequence_parity_bits = parity_bits & 0b111111111;
  current_parity_sum ^= bit_sequence_parity(sequence_parity_bits);

  return current_parity_sum;
}

// TODO: there should be some way to test this later on
uint8_t bit_sequence_parity(uint32_t input) {
  uint8_t sequence_parity = 0;
  for (int i = 0; i < 32; ++i) {
    sequence_parity ^=  input & 0b1;
    input >>= 1;
  }

  return sequence_parity;
}

void decode_256(const uint32_t raw_data[], const uint16_t parity_bits, DecodeResponse_t *decode_response) {
  // remove the parity bit, since it is not used to determine the position of the flipped bit
  uint16_t overall_parity_bit = parity_bits >> NUM_PARITY_BITS;
  uint16_t decode_parity_bits = parity_bits & 0b111111111;

  uint16_t overall_bit_sum = 0;
  for (int i = 0; i < NUM_PARITY_BITS; ++i) {
    uint16_t local_bit_sum = bit_sequence_parity(decode_parity_bits & decode_matrix[i][0]);
    for (int j = 1; j < NUM_COLS_IN_DECODE_MATRIX; ++j) {
      local_bit_sum ^= bit_sequence_parity(raw_data[j - 1] & decode_matrix[i][j]);
    }

    overall_bit_sum <<= 1;
    overall_bit_sum |= local_bit_sum;
  }
 
  decode_response->response_flags = (
    ((error_location[overall_bit_sum] != 0) ? BIT_CORRECTED : 0) | 
    ((overall_parity_bit != encode_overall_parity(raw_data, parity_bits)) ? OVERALL_PARITY_INVALID : 0)
  );

  decode_response->bit_position_to_correct = error_location[overall_bit_sum];

  return;
}

const uint32_t parity_generator_idxs[NUM_PARITY_BITS][NUM_32_BIT_COLS_IN_BLOCK] = {
  {0, 0, 0, 0, 0, 0, 0, 511},
  {0, 0, 0, 255, 4294967295, 4294967295, 4294967295, 4294966784},
  {0, 127, 4294967295, 4294967040, 0, 511, 4294967295, 4294966784},
  {63, 4294967168, 255, 4294967040, 511, 4294966784, 511, 4294966784},
  {2097088, 8388480, 16776960, 16776960, 33553920, 33553920, 33553920, 33553920},
  {266354624, 2139127680, 4278255360, 4278255361, 4261543425, 4261543425, 4261543425, 4261543427},
  {1910752199, 2273806223, 252645135, 252645150, 505290270, 505290270, 505290270, 505290300},
  {3060583641, 2576980403, 858993459, 858993510, 1717986918, 1717986918, 1717986918, 1717987020},
  {3669316970, 2863311573, 1431655765, 1431655850, 2863311530, 2863311530, 2863311530, 2863311701},
};

// last number should not be 32 bits :/
const uint32_t decode_matrix[][NUM_COLS_IN_DECODE_MATRIX] = {
  {256, 0, 0, 0, 0, 0, 0, 0, 511}, 
  {128, 0, 0, 0, 255, 4294967295, 4294967295, 4294967295, 4294966784}, 
  {64, 0, 127, 4294967295, 4294967040, 0, 511, 4294967295, 4294966784}, 
  {32, 63, 4294967168, 255, 4294967040, 511, 4294966784, 511, 4294966784}, 
  {16, 2097088, 8388480, 16776960, 16776960, 33553920, 33553920, 33553920, 33553920}, 
  {8, 266354624, 2139127680, 4278255360, 4278255361, 4261543425, 4261543425, 4261543425, 4261543427}, 
  {4, 1910752199, 2273806223, 252645135, 252645150, 505290270, 505290270, 505290270, 505290300}, 
  {2, 3060583641, 2576980403, 858993459, 858993510, 1717986918, 1717986918, 1717986918, 1717987020}, 
  {1, 3669316970, 2863311573, 1431655765, 1431655850, 2863311530, 2863311530, 2863311530, 2863311701}
};

const uint16_t error_location[] = {
  0, 9, 8, 10, 7, 11, 12, 13,
  6, 14, 15, 16, 17, 18, 19, 20,
  5, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 34, 35,
  4, 36, 37, 38, 39, 40, 41, 42,
  43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 65, 66,
  3, 67, 68, 69, 70, 71, 72, 73,
  74, 75, 76, 77, 78, 79, 80, 81,
  82, 83, 84, 85, 86, 87, 88, 89,
  90, 91, 92, 93, 94, 95, 96, 97,
  98, 99, 100, 101, 102, 103, 104, 105,
  106, 107, 108, 109, 110, 111, 112, 113,
  114, 115, 116, 117, 118, 119, 120, 121,
  122, 123, 124, 125, 126, 127, 128, 129,
  2, 130, 131, 132, 133, 134, 135, 136,
  137, 138, 139, 140, 141, 142, 143, 144,
  145, 146, 147, 148, 149, 150, 151, 152,
  153, 154, 155, 156, 157, 158, 159, 160,
  161, 162, 163, 164, 165, 166, 167, 168,
  169, 170, 171, 172, 173, 174, 175, 176,
  177, 178, 179, 180, 181, 182, 183, 184,
  185, 186, 187, 188, 189, 190, 191, 192,
  193, 194, 195, 196, 197, 198, 199, 200,
  201, 202, 203, 204, 205, 206, 207, 208,
  209, 210, 211, 212, 213, 214, 215, 216,
  217, 218, 219, 220, 221, 222, 223, 224,
  225, 226, 227, 228, 229, 230, 231, 232,
  233, 234, 235, 236, 237, 238, 239, 240,
  241, 242, 243, 244, 245, 246, 247, 248,
  249, 250, 251, 252, 253, 254, 255, 256,
  1, 257, 258, 259, 260, 261, 262, 263,
  264, 265
};