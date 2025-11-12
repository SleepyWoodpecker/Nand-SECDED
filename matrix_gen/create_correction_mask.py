from pprint import pprint

# transpose the decode.txt
# with open("transposed2.txt", "w") as output:
#     with open("matrix_gen/decode.txt", "r") as file:
#         data = file.readlines()
#         for i in range(len(data[0])):
#             for j in range(len(data)):
#                 output.write(data[j][i])

#             output.write("\n")
#     output.flush()

# correction_pairs = []
# with open("transposed2.txt", "r") as file:
#     for bit_number, code_number in enumerate(file):
#         correction_pairs.append((bit_number, code_number.strip()))

# correction_pairs.sort(key=lambda entry: int(entry[1], 2) - 511)
# correction_pairs = [pair[0] for pair in correction_pairs]

# print(correction_pairs)
# print(len(correction_pairs))

# there is an offset of 511
# pairs = []
# with open("transposed2.txt", "r") as file:
#     for line in file:
#         pairs.append(int(line.strip(), 2))

# pairs.sort(reverse=True)
# print(pairs)

out = []
with open("matrix_gen/decode.txt", "r") as file:
    for line in file:
        out_small = []
        line = line.rstrip()
        out_small.append(int(line[:10], 2))

        for i in range(10, len(line), 32):
            out_small.append(int(line[i : i + 32], 2))

        out.append(out_small)

print(out)
