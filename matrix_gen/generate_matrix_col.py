def generate_for_parity_num(parity_num, encoded_word_num_bits):
    arr = []
    for i in range(encoded_word_num_bits):
        if i & (i - 1) == 0:
            continue

        if i & parity_num:
            arr.append(1)
        else:
            arr.append(0)

    return arr


def generate_encoding_matrix(parity_matrices, num_data_bits):
    """
    Generate the transpose of the encoding matrix, with format:
    p3,
    p2,
    p1
    ...,
    d1,
    d2,
    ...
    """
    generator_matrix = []

    for parity_select in parity_matrices:
        generator_matrix.append("".join([str(c) for c in parity_select]))

    for n in range(num_data_bits - 1, -1, -1):
        select_string = bin(1 << n)[2:].zfill(num_data_bits)
        generator_matrix.append(select_string)

    generator_matrix_as_num = []
    # split the encode matrix into 32 bit strings
    for el in generator_matrix:
        split_list = []
        for bit_string_idx in range(0, len(el), 32):
            split_list.append(int(el[bit_string_idx : bit_string_idx + 32], 2))

        generator_matrix_as_num.append(split_list)

    return generator_matrix_as_num


def generate_decode_matrix(parity_matrices, num_parity_bits):
    """
    Generate the decoding matrix, with format:
    p3 + data bits used to generate p3
    p2 + data bits used to generate p2
    .
    .
    .
    """
    decode_matrix = []

    for n in range(num_parity_bits - 1, -1, -1):
        i = num_parity_bits - 1 - n
        decode_matrix.append(bin(1 << n)[2:].zfill(num_parity_bits))

    chunked_decode_matrix = [[0b1111111111]]
    for i in range(8):
        chunked_decode_matrix[0].append(0b11111111111111111111111111111111)

    for i, parity_selection in enumerate(parity_matrices):
        decode_matrix_chunks = []
        decode_matrix_chunks.append(int(decode_matrix[i], 2))
        decode_matrix_string = "".join([str(select) for select in parity_selection])
        # split into 32 bit chunks
        for idx in range(0, len(decode_matrix_string), 32):
            decode_matrix_chunks.append(int(decode_matrix_string[idx : idx + 32], 2))
        chunked_decode_matrix.append(decode_matrix_chunks)

    return chunked_decode_matrix


def transpose_decode_matrix(decode_matrix, codeword_bits, parity_bits):
    # number of bits for row of the transposed matrix would be equal to the number of parity bits
    number_strings = []
    for val_list in decode_matrix:
        number_strings.append(
            "".join([bin(val)[2:].zfill(min(32, codeword_bits)) for val in val_list])
        )

    transposed = [0]
    for i in range(codeword_bits):
        num_string = ""
        for j in range(parity_bits):
            num_string += number_strings[j][i]

        transposed.append(int(num_string, 2))

    return transposed


def bit_sum(num):
    initial_sum = 0
    for bit in bin(num)[2:]:
        initial_sum ^= int(bit)
    return initial_sum


def encode_message(message, encoding_matrix):
    encoded_message = ""
    for val_list in encoding_matrix:
        current_num = 0
        for i, val in enumerate(val_list):
            current_num ^= bit_sum(int(message[i * 32 : (i + 1) * 32], 2) & val)
        encoded_message += str(current_num)

    # add the overall parity
    encoded_message = str(bit_sum(int(message, 2))) + encoded_message

    return encoded_message


def get_decode_position(error_correction_idx):
    arr = [
        0,
        1,
        10,
        9,
        11,
        8,
        12,
        13,
        14,
        7,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        6,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
        32,
        33,
        34,
        35,
        36,
        5,
        37,
        38,
        39,
        40,
        41,
        42,
        43,
        44,
        45,
        46,
        47,
        48,
        49,
        50,
        51,
        52,
        53,
        54,
        55,
        56,
        57,
        58,
        59,
        60,
        61,
        62,
        63,
        64,
        65,
        66,
        67,
        4,
        68,
        69,
        70,
        71,
        72,
        73,
        74,
        75,
        76,
        77,
        78,
        79,
        80,
        81,
        82,
        83,
        84,
        85,
        86,
        87,
        88,
        89,
        90,
        91,
        92,
        93,
        94,
        95,
        96,
        97,
        98,
        99,
        100,
        101,
        102,
        103,
        104,
        105,
        106,
        107,
        108,
        109,
        110,
        111,
        112,
        113,
        114,
        115,
        116,
        117,
        118,
        119,
        120,
        121,
        122,
        123,
        124,
        125,
        126,
        127,
        128,
        129,
        130,
        3,
        131,
        132,
        133,
        134,
        135,
        136,
        137,
        138,
        139,
        140,
        141,
        142,
        143,
        144,
        145,
        146,
        147,
        148,
        149,
        150,
        151,
        152,
        153,
        154,
        155,
        156,
        157,
        158,
        159,
        160,
        161,
        162,
        163,
        164,
        165,
        166,
        167,
        168,
        169,
        170,
        171,
        172,
        173,
        174,
        175,
        176,
        177,
        178,
        179,
        180,
        181,
        182,
        183,
        184,
        185,
        186,
        187,
        188,
        189,
        190,
        191,
        192,
        193,
        194,
        195,
        196,
        197,
        198,
        199,
        200,
        201,
        202,
        203,
        204,
        205,
        206,
        207,
        208,
        209,
        210,
        211,
        212,
        213,
        214,
        215,
        216,
        217,
        218,
        219,
        220,
        221,
        222,
        223,
        224,
        225,
        226,
        227,
        228,
        229,
        230,
        231,
        232,
        233,
        234,
        235,
        236,
        237,
        238,
        239,
        240,
        241,
        242,
        243,
        244,
        245,
        246,
        247,
        248,
        249,
        250,
        251,
        252,
        253,
        254,
        255,
        256,
        257,
        2,
        258,
        259,
        260,
        261,
        262,
        263,
        264,
        265,
        266,
    ]
    return arr[error_correction_idx]


def decode_message(encoded_message, decoding_matrix, num_parity_bits):
    error_val = ""

    raw_message = encoded_message[10:]
    for val_list in decoding_matrix:
        local_sum = bit_sum(int(encoded_message[:10], 2) & val_list[0])
        for i, val in enumerate(val_list[1:]):
            local_sum ^= bit_sum(val & int(raw_message[i * 32 : (i + 1) * 32], 2))

        error_val += str(local_sum)

    assert num_parity_bits + 1 == len(error_val)
    error_location_idx = get_decode_position(int(error_val, 2) - 511)
    corrected_message = False
    if error_location_idx >= 0:
        # print("Error at bit", error_location_idx - 1)
        encoded_message = flip_bit(encoded_message, error_location_idx - 1)
        corrected_message = True

    message = encoded_message[num_parity_bits + 1 :]

    return message, corrected_message


def flip_bit(message, bit_to_flip):
    message_copy = message
    if message_copy[bit_to_flip] == "0":
        message_copy = (
            message_copy[:bit_to_flip] + "1" + message_copy[bit_to_flip + 1 :]
        )

    elif message_copy[bit_to_flip] == "1":
        message_copy = (
            message_copy[:bit_to_flip] + "0" + message_copy[bit_to_flip + 1 :]
        )

    return message_copy


def overall_parity_check(message):
    overall_parity = int(message[0])
    for bit in message[1:]:
        overall_parity ^= int(bit)

    return overall_parity == 0


def test_hamming(
    encoding_matrix,
    decoding_matrix,
    num_parity_bits,
    message,
    bit_to_flip=None,
    second_bit_to_flip=None,
):

    encoded_message = encode_message(message, encoding_matrix=encoding_matrix)

    if bit_to_flip != None:
        encoded_message = flip_bit(encoded_message, bit_to_flip)

        if second_bit_to_flip != None:
            encoded_message = flip_bit(encoded_message, second_bit_to_flip)

    # first check parity of the entire message
    overall_parity_valid = overall_parity_check(encoded_message)

    original_message, corrected = None, None
    try:
        original_message, corrected = decode_message(
            encoded_message,
            decoding_matrix=decoding_matrix,
            num_parity_bits=num_parity_bits,
        )
    except IndexError:
        if second_bit_to_flip != None:
            return False, "", False

    return overall_parity_valid, original_message, corrected


def main():
    encoded_word_bits = 256 + 9 + 1
    num_parity_bits = 9
    num_data_bits = 256
    message = "1010110101100110000000101101100001000111110011100011000110001011010000111001111101101100000111111101000000100011110101110111011111011010001011010000101001010111100011001110000010011110100110110100110001100011000001100011100110111000110111000001010100101011"

    parity_matrices = []
    for i in range(num_parity_bits):
        parity_matrices.append(
            generate_for_parity_num(
                parity_num=(1 << i),
                encoded_word_num_bits=encoded_word_bits,
            )
        )

    parity_matrices.reverse()

    encoding_matrix = generate_encoding_matrix(
        parity_matrices=parity_matrices,
        num_data_bits=num_data_bits,
    )

    decoding_matrix = generate_decode_matrix(
        parity_matrices=parity_matrices, num_parity_bits=num_parity_bits
    )

    # test single bit flips
    for i in range(266):
        overall_parity_valid, original_message, corrected = test_hamming(
            encoding_matrix=encoding_matrix,
            decoding_matrix=decoding_matrix,
            num_parity_bits=num_parity_bits,
            message=message,
            bit_to_flip=i,
        )

        assert original_message == message, f"Bit {i}"
        no_error = (corrected and not overall_parity_valid) or (
            not corrected and overall_parity_valid
        )
        assert no_error, f"Bit {i}"

    # test all possible double bit flips
    for i in range(266):
        for j in range(i + 1, 266):
            overall_parity_valid, original_message, corrected = test_hamming(
                encoding_matrix=encoding_matrix,
                decoding_matrix=decoding_matrix,
                num_parity_bits=num_parity_bits,
                message=message,
                bit_to_flip=i,
                second_bit_to_flip=j,
            )

            # the only way to have no error is:
            # you have no errors, and the overall message is valid
            # you have an error, and you corrected one bit in the message
            no_error = (corrected and not overall_parity_valid) or (
                not corrected and overall_parity_valid
            )
            assert (
                not no_error
            ), f"Bit 1: {i}, Bit 2: {j}, Corrected: {corrected}, Overall valid: {overall_parity_valid} "


if __name__ == "__main__":
    main()
