"""
Convert a CSV table into a tilemap
"""

import csv
import os
import struct
import sys


def main(path, layers):
    name, suffix = os.path.splitext(path)

    if suffix != ".tsv":
        raise ValueError("Expected TSV")

    assert(layers > 0)

    with open(path, newline="", encoding="utf-8-sig") as stream:
        rows = [[int(value) for value in row] for row in csv.reader(stream, delimiter="\t")]

    if not rows:
        raise ValueError("Table invalid")

    if len(rows) % (layers + 1):
        raise ValueError("Row count invalid")

    width = len(rows[0])
    height = len(rows) // (layers + 1)

    physics = rows[:height]

    output = name + ".map"
    with open(output, "wb") as stream:
        stream.write(struct.pack("<III", width, height, layers))

        for layer in range(layers):
            for y in range(height):
                for x in range(width):
                    
                    value = rows[(layer + 1) * height + y][x] & 0x7FFFFFFF
                    if layer == 0 and physics[y][x] != 0:
                        value |= 0x80000000
                    stream.write(struct.pack("<I", value))

    print("done")

if __name__ == "__main__":
    assert(len(sys.argv) == 3, "args: <path> <num_layers>")
       
    main(sys.argv[1], int(sys.argv[2]))
