#include "secded.h"

#include <stdint.h>
#include <stddef.h>

// NOTE: Smallest return type is uint16_t, mostly for the sake of standardization

static uint16_t bit_sequence_parity(uint32_t input);
static uint16_t get_message_parity(const uint32_t raw_data[]);
static ParityBlock_t split_parity_bits(const uint16_t parity_bits, const int set_number);
static uint16_t extract_block_parity_sequence(const uint8_t parity_sequence[], const int parity_sequence_idx, const int set_number);

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

uint16_t encode_overall_parity(const uint32_t raw_data[], const uint16_t parity_bits) {
  uint16_t current_parity_sum = get_message_parity(raw_data);
  uint16_t sequence_parity_bits = parity_bits & 0b111111111;
  current_parity_sum ^= bit_sequence_parity(sequence_parity_bits);

  return current_parity_sum;
}

void decode_256(const uint32_t raw_data[], const uint16_t parity_bits, DecodeResponse_t *decode_response) {
  // zero out the decode response first
  decode_response->bit_position_to_correct = 0;
  decode_response->response_flags = 0;

  uint16_t overall_bit_sum = 0;
  for (int i = 0; i < TOTAL_NUM_PARITY_BITS ; ++i) {
    uint16_t local_bit_sum = bit_sequence_parity(parity_bits & decode_matrix[i][0]);
    for (int j = 1; j < NUM_COLS_IN_DECODE_MATRIX; ++j) {
      local_bit_sum ^= bit_sequence_parity(raw_data[j - 1] & decode_matrix[i][j]);
    }

    overall_bit_sum <<= 1;
    overall_bit_sum |= local_bit_sum;
  }

  // the error syndromes start from 512. Subtract this offset from the syndromes so that they can be sequentially accessed
  // to also ensure that a result of 0 corresponds to no error, subtract 511
  // if the overall sum is < 511, it must mean that there is a double bit error
  // except for the 0 case where there are no bits that are wrong
  // an overall bit sum of 0 means that there is no error

  if (overall_bit_sum == 0) {
    return;
  } else if (overall_bit_sum < SYNDROME_OFFSET + 1) {
    decode_response->response_flags = DOUBLE_BIT_ERROR;
    return;
  } 

  uint16_t overall_parity_bit = parity_bits >> INDIVIDUAL_PARITY_BITS;
  decode_response->response_flags = (
    BIT_CORRECTED | 
    ((overall_parity_bit != encode_overall_parity(raw_data, parity_bits)) ? OVERALL_PARITY_INVALID : 0)
  );

  decode_response->bit_position_to_correct = error_location[overall_bit_sum - SYNDROME_OFFSET];

  return;
}

bool resolve_decode(uint32_t raw_data[], DecodeResponse_t *decode_response) {
  // the only way there would be no errors in the message is if:
  // 1. overall parity has error and 1 bit was flipped
  // 2. overall parity has no error and no bit was flipped
  if (decode_response->response_flags == 0) {
    return true;
  }
  else if (decode_response->response_flags == (BIT_CORRECTED | OVERALL_PARITY_INVALID)) {
    uint16_t data_bit_to_correct = decode_response->bit_position_to_correct;

    // if it is a parity bit, just return normally
    if (data_bit_to_correct < TOTAL_NUM_PARITY_BITS) {
      return true;
    }

    // offset from total number of parity bits. index 11 would refer to first message bit
    data_bit_to_correct -= (TOTAL_NUM_PARITY_BITS + 1);
    // divide by 32
    uint16_t column_to_flip = data_bit_to_correct >> 5;
    // get data_bit_to_correct % 32
    uint16_t position_to_flip = data_bit_to_correct & 0b11111;

    raw_data[column_to_flip] ^= (1 << (31 - position_to_flip));
    return true;
  }

  return false;
}

void encode_page(const uint8_t raw_data[restrict], uint8_t parity_bit_sequences[restrict]) {
  // first, recast the pointers to the appropriate types
  uint32_t *r_raw_data = (uint32_t *)raw_data;
  size_t parity_bit_sequences_idx = 0;

  for (int i = 0; i < NUM_BLOCKS_IN_PAGE; ++i) {
    uint16_t parity_seq = encode_256(r_raw_data);
    uint16_t overall_parity_seq = encode_overall_parity(r_raw_data, parity_seq);

    uint16_t overall_parity = (overall_parity_seq << INDIVIDUAL_PARITY_BITS) | parity_seq; 
    
    ParityBlock_t parity_blocks = split_parity_bits(overall_parity, i);
    // write the parity bits to the block
    parity_bit_sequences[parity_bit_sequences_idx] |= parity_blocks.first_section;
    parity_bit_sequences[parity_bit_sequences_idx + 1] |= parity_blocks.second_section;

    parity_bit_sequences_idx++;
    // the sequence repeats itself every 4 blocks -> perform a full wraparound
    // n & 0b11 takes mod 4
    if (i != 0 && ((i + 1) & 0b11) == 0) {
      parity_bit_sequences_idx++;
    }

    r_raw_data += NUM_32_BIT_COLS_IN_BLOCK;
  }
}

void encode_page_without_restrict(const uint8_t raw_data[], uint8_t parity_bit_sequences[]) {
  // first, recast the pointers to the appropriate types
  uint32_t *r_raw_data = (uint32_t *)raw_data;
  size_t parity_bit_sequences_idx = 0;

  for (int i = 0; i < NUM_BLOCKS_IN_PAGE; ++i) {
    uint16_t parity_seq = encode_256(r_raw_data);
    uint16_t overall_parity_seq = encode_overall_parity(r_raw_data, parity_seq);

    uint16_t overall_parity = (overall_parity_seq << INDIVIDUAL_PARITY_BITS) | parity_seq; 
    
    ParityBlock_t parity_blocks = split_parity_bits(overall_parity, i);
    // write the parity bits to the block
    parity_bit_sequences[parity_bit_sequences_idx] |= parity_blocks.first_section;
    parity_bit_sequences[parity_bit_sequences_idx + 1] |= parity_blocks.second_section;

    parity_bit_sequences_idx++;
    // the sequence repeats itself every 4 blocks -> perform a full wraparound
    // n & 0b11 takes mod 4
    if (i != 0 && ((i + 1) & 0b11) == 0) {
      parity_bit_sequences_idx++;
    }

    r_raw_data += NUM_32_BIT_COLS_IN_BLOCK;
  }
}

bool decode_page(uint8_t raw_data[restrict], uint8_t parity_bit_sequences[restrict]) {
  uint32_t *r_raw_data = (uint32_t *)raw_data;
  int parity_bit_sequence_idx = 0;
  for (int i = 0; i < NUM_BLOCKS_IN_PAGE; ++i) {
    DecodeResponse_t decode_response;
    uint16_t parity_bits = extract_block_parity_sequence(parity_bit_sequences, parity_bit_sequence_idx, i);
    decode_256(r_raw_data, parity_bits, &decode_response);

    if (!resolve_decode(r_raw_data, &decode_response)) {
      return false;
    }

    parity_bit_sequence_idx++;
    if (i != 0 && ((i + 1) & 0b11) == 0) {
      parity_bit_sequence_idx++;
    }

    r_raw_data += NUM_32_BIT_COLS_IN_BLOCK;
  }

  return true;
}

bool decode_page_without_restrict(uint8_t raw_data[], uint8_t parity_bit_sequences[]) {
  uint32_t *r_raw_data = (uint32_t *)raw_data;
  int parity_bit_sequence_idx = 0;
  for (int i = 0; i < NUM_BLOCKS_IN_PAGE; ++i) {
    DecodeResponse_t decode_response;
    uint16_t parity_bits = extract_block_parity_sequence(parity_bit_sequences, parity_bit_sequence_idx, i);
    decode_256(r_raw_data, parity_bits, &decode_response);

    if (!resolve_decode(r_raw_data, &decode_response)) {
      return false;
    }

    parity_bit_sequence_idx++;
    if (i != 0 && ((i + 1) & 0b11) == 0) {
      parity_bit_sequence_idx++;
    }

    r_raw_data += NUM_32_BIT_COLS_IN_BLOCK;
  }

  return true;
}

/**
 * @brief: Calculate the parity for the provided bit sequence
 * @param input: the 32 bit sequence for which the parity will be determined
 * @return: The parity for the provided bit sequence. 1 if overall sequence is odd, 0 if the oevrall sequence is even
 *
 * NOTE: this approach makes use of "bit folding", where the upper half is folded into the lower half. This approach avoids the use of a loop.
 */
static uint16_t bit_sequence_parity(uint32_t input) {
  input ^= input >> 16;
  input ^= input >> 8;
  input ^= input >> 4;
  input ^= input >> 2;
  input ^= input >> 1;

  return input & 1;
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

/**
 * @brief: Split the parity block that it can be put properly into a uint8_t array
 * @param parity_bits: the encoded parity bits
 * @param set_number: the block number for which the parity bits were generated for
 * @return: the split up bit sequence to fit into the spare region
 *
 * Splits look like:
 * 8, 2
 * 6, 4
 * 4, 6
 * 2, 8
 */
static ParityBlock_t split_parity_bits(const uint16_t parity_bits, const int set_number) {
  ParityBlock_t block;
  // n & 0b11 takes mod 4
  int num_bits_in_second_block = (set_number & 0b11) * 2 + 2;

  block.first_section = parity_bits >> num_bits_in_second_block;
  block.second_section = (parity_bits ^ (block.first_section << num_bits_in_second_block)) << (8 - num_bits_in_second_block);

  return block;
}

/**
 * @brief: Return the 10 bit parity sequence for a block
 * @param parity_sequence: pointer to the entire spare region
 * @param parity_sequence_idx: index corresponding to the block being accessed within the parity region
 * @param set_number: block number of spare block that is being decoded
 * @return: the 10 bit parity sequence that encodes that block
 * 
 * Splits look like:
 * 8, 2
 * 6, 4
 * 4, 6
 * 2, 8
 */
static uint16_t extract_block_parity_sequence(const uint8_t parity_sequence[], const int parity_sequence_idx, const int set_number) {
  // exatract the first section
  int num_bits_in_second_block = (set_number & 0b11) * 2 + 2;

  uint16_t top_half = ((uint16_t)parity_sequence[parity_sequence_idx] << num_bits_in_second_block) >> num_bits_in_second_block;
  uint16_t bottom_half = (parity_sequence[parity_sequence_idx + 1] >> (8 - num_bits_in_second_block));

  // return the bottom 10 bits
  return (top_half << num_bits_in_second_block | bottom_half) & 0x3FF;
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
  {1023, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295},
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
    0,    1,   10,    9,   11,    8,   12,   13,
   14,    7,   15,   16,   17,   18,   19,   20,
   21,    6,   22,   23,   24,   25,   26,   27,
   28,   29,   30,   31,   32,   33,   34,   35,
   36,    5,   37,   38,   39,   40,   41,   42,
   43,   44,   45,   46,   47,   48,   49,   50,
   51,   52,   53,   54,   55,   56,   57,   58,
   59,   60,   61,   62,   63,   64,   65,   66,
   67,    4,   68,   69,   70,   71,   72,   73,
   74,   75,   76,   77,   78,   79,   80,   81,
   82,   83,   84,   85,   86,   87,   88,   89,
   90,   91,   92,   93,   94,   95,   96,   97,
   98,   99,  100,  101,  102,  103,  104,  105,
  106,  107,  108,  109,  110,  111,  112,  113,
  114,  115,  116,  117,  118,  119,  120,  121,
  122,  123,  124,  125,  126,  127,  128,  129,
  130,    3,  131,  132,  133,  134,  135,  136,
  137,  138,  139,  140,  141,  142,  143,  144,
  145,  146,  147,  148,  149,  150,  151,  152,
  153,  154,  155,  156,  157,  158,  159,  160,
  161,  162,  163,  164,  165,  166,  167,  168,
  169,  170,  171,  172,  173,  174,  175,  176,
  177,  178,  179,  180,  181,  182,  183,  184,
  185,  186,  187,  188,  189,  190,  191,  192,
  193,  194,  195,  196,  197,  198,  199,  200,
  201,  202,  203,  204,  205,  206,  207,  208,
  209,  210,  211,  212,  213,  214,  215,  216,
  217,  218,  219,  220,  221,  222,  223,  224,
  225,  226,  227,  228,  229,  230,  231,  232,
  233,  234,  235,  236,  237,  238,  239,  240,
  241,  242,  243,  244,  245,  246,  247,  248,
  249,  250,  251,  252,  253,  254,  255,  256,
  257,    2,  258,  259,  260,  261,  262,  263,
  264,  265,  266
};