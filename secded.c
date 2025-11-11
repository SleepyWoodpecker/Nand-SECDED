#include "secded.h"

#include <stdint.h>

// NOTE: Smallest return type is uint16_t, mostly for the sake of standardization

static uint16_t bit_sequence_parity(uint32_t input);
static uint16_t get_message_parity(const uint32_t raw_data[]);

/**
 * @brief: Given a 256 bit block of data, generate the parity bits
 * @param raw_data: pointer to the 256 bit block of data, split into 32 bit chunks and represented as an array
 * @return: the 9 parity bits for the message sequence
 */
uint16_t encode_256(const uint32_t raw_data[]) {
  // calculate parity bits for data
  uint16_t parity_bits = 0;
  for (int i = 0; i < INDIVIDUAL_PARITY_BITS; ++i) {
    int sequence_parity = 0;
    for (int j = 0; j < NUM_32_BIT_COLS_IN_BLOCK; ++j) {
      sequence_parity ^= bit_sequence_parity(raw_data[j] & parity_generator_idxs[i][j]);
    }
    parity_bits <<= 1;
    parity_bits |= sequence_parity;
  }

  // expect 9 bits to come back from parity encoding
  return parity_bits & 0x1FF;
}

/**
 * @brief: Calculate the overall parity bit
 * @param raw_data: pointer to the 256 bit block of data, split into 32 bit chunks and represented as an array
 * @param parity_bits: the 9 parity bits generated for this 256 bits of data
 * @return: the overall parity bit
*/
uint16_t encode_overall_parity(const uint32_t raw_data[], const uint16_t parity_bits) {
  uint16_t current_parity_sum = get_message_parity(raw_data);
  uint16_t sequence_parity_bits = parity_bits & 0b111111111;
  current_parity_sum ^= bit_sequence_parity(sequence_parity_bits);

  return current_parity_sum;
}

/**
 * @brief: Decode the raw data, and update decode_response with the status of the data
 * @param raw_data: pointer to the 256 bit block of data, split into 32 bit chunks and represented as an array
 * @param parity_bits: the 9 parity bits generated from the 256 bits of data + the overall parity bit for the block of data
 * @param decode_response: pointer to a struct that would inform the user of the decode status of the data
 */
void decode_256(const uint32_t raw_data[], const uint16_t parity_bits, DecodeResponse_t *decode_response) {
  // remove the parity bit, since it is not used to determine the position of the flipped bit
  uint16_t overall_parity_bit = parity_bits >> INDIVIDUAL_PARITY_BITS;
  uint16_t decode_parity_bits = parity_bits & 0b111111111;

  uint16_t overall_bit_sum = 0;
  for (int i = 0; i < INDIVIDUAL_PARITY_BITS; ++i) {
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

/**
 * @brief: resolve the data decoding, based on the decode_response
 * @param raw_data: pointer to the 256 bit block of data, split into 32 bit chunks and represented as an array
 * @param decode_response: the decode status for this 256 bit block of data
 * @return: whether the returned data is valid
 */
bool resolve_decode(uint32_t raw_data[], DecodeResponse_t *decode_response) {
  // the only way there would be no errors in the message is if:
  // overall parity has error and 1 bit was flipped
  // overall parity no error and no bit was flipped
  if (decode_response->response_flags == 0 && decode_response->bit_position_to_correct == 0) {
    return true;
  }
  else if (decode_response->response_flags == (BIT_CORRECTED | OVERALL_PARITY_INVALID)) {
    uint16_t data_bit_to_correct = decode_response->bit_position_to_correct;

    // if it is a parity bit, just return normally
    if (data_bit_to_correct < TOTAL_NUM_PARITY_BITS) {
      return true;
    }

    // offset from parity bits
    data_bit_to_correct -= TOTAL_NUM_PARITY_BITS;
    // divide by 32
    uint16_t column_to_flip = data_bit_to_correct >> 5;
    // get data_bit_to_correct % 32
    uint16_t position_to_flip = data_bit_to_correct & 0b11111;

    raw_data[column_to_flip] ^= (1 << (31 - position_to_flip));
    return true;
  }

  return false;
}

/**
 * @brief: Calculate the parity for the provided bit sequence
 * @param input: the 32 bit sequence for which the parity will be determined
 * @return: The parity for the provided bit sequence. 1 if overall sequence is odd, 0 if the oevrall sequence is even
 */
static uint16_t bit_sequence_parity(uint32_t input) {
  uint16_t sequence_parity = 0;
  for (int i = 0; i < 32; ++i) {
    sequence_parity ^=  input & 0b1;
    input >>= 1;
  }

  return sequence_parity;
}

/**
 * @brief: Calculate the parity for the message
 * @param raw_data: pointer to the 256 bit block of data, split into 32 bit chunks and represented as an array
 * @return: the overall parity of the 256 bit block message
 */
static uint16_t get_message_parity(const uint32_t raw_data[]) {
  uint16_t current_parity_sum = 0;
  for (int i = 0; i < NUM_32_BIT_COLS_IN_BLOCK; ++i) {
    current_parity_sum ^= bit_sequence_parity(raw_data[i]);
  }
  return current_parity_sum;
}

const uint32_t parity_generator_idxs[INDIVIDUAL_PARITY_BITS][NUM_32_BIT_COLS_IN_BLOCK] = {
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