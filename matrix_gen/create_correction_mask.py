from pprint import pprint

correction_pairs = []
with open("decode_transposed.txt", "r") as file:
    for bit_number, code_number in enumerate(file):
        correction_pairs.append((bit_number, code_number.strip()))

correction_pairs.sort(key=lambda entry: entry[1])
correction_pairs = [pair[0] for pair in correction_pairs]

print(correction_pairs)
