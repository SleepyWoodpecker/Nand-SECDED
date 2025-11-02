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

    chunked_decode_matrix = []
    for i, parity_selection in enumerate(parity_matrices):
        decode_matrix_string = decode_matrix[i] + "".join(
            [str(select) for select in parity_selection]
        )
        decode_matrix_chunks = []
        # split into 32 bit chunks
        for idx in range(0, len(decode_matrix_string), 32):
            decode_matrix_chunks.append(int(decode_matrix_string[idx : idx + 32], 2))
        chunked_decode_matrix.append(decode_matrix_chunks)

    print(chunked_decode_matrix)
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
    # TODO: add overall parity
    encoded_message = ""
    for val_list in encoding_matrix:
        for i, val in enumerate(val_list):
            encoded_message += str(
                bit_sum(int(message[i * 32 : (i + 1) * 32], 2) & val)
            )

    # add the overall parity
    # encoded_message = str(bit_sum(int(message, 2))) + encoded_message

    return encoded_message


def decode_message(
    encoded_message, decoding_matrix, num_parity_bits, transposed_decode_matrix
):
    # encoded_message = encoded_message[1:]
    error_val = ""
    for val_list in decoding_matrix:
        local_sum = 0
        for i, val in enumerate(val_list):
            local_sum ^= bit_sum(val & int(encoded_message[i * 32 : (i + 1) * 32], 2))

        error_val += str(local_sum)
    print(error_val)

    assert num_parity_bits == len(error_val)

    error_location_idx = transposed_decode_matrix.index(int(error_val, 2))
    if error_location_idx:
        print("Error at bit", error_location_idx - 1)
        encoded_message = flip_bit(encoded_message, error_location_idx - 1)
    else:
        print("No error detected!")

    message = encoded_message[num_parity_bits:]

    return message


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


def main():
    encoded_word_bits = 2**9 - (2**9 - 266)
    num_parity_bits = 9
    num_data_bits = 256

    num_parity_bits = 3
    num_data_bits = 4
    # make it +1 because you always skip 0
    encoded_word_bits = num_parity_bits + num_data_bits + 1

    parity_matrices = []
    for i in range(num_parity_bits):
        parity_matrices.append(
            generate_for_parity_num(
                parity_num=(1 << i),
                encoded_word_num_bits=encoded_word_bits,
            )
        )

    # to allow final result to give the position of the
    # flipped bit, the final parity bit must come first
    parity_matrices.reverse()

    encoding_matrix = generate_encoding_matrix(
        parity_matrices=parity_matrices,
        num_data_bits=num_data_bits,
    )

    decoding_matrix = generate_decode_matrix(
        parity_matrices=parity_matrices, num_parity_bits=num_parity_bits
    )

    transposed_decode_matrix = transpose_decode_matrix(
        decode_matrix=decoding_matrix,
        codeword_bits=num_parity_bits + num_data_bits,
        parity_bits=num_parity_bits,
    )

    message = "1010"
    encoded_message = encode_message(message, encoding_matrix=encoding_matrix)

    bit_to_flip = 6
    encoded_message = flip_bit(encoded_message, bit_to_flip)
    print(encoded_message)

    original_message = decode_message(
        encoded_message,
        decoding_matrix=decoding_matrix,
        num_parity_bits=num_parity_bits,
        transposed_decode_matrix=transposed_decode_matrix,
    )

    print(f"The original message is: {original_message}")


if __name__ == "__main__":
    main()
