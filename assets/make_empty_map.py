import argparse, os, struct

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=str)
    parser.add_argument("w", type=int)
    parser.add_argument("h", type=int)
    parser.add_argument("l", type=int)
    args = parser.parse_args()

    assert not os.path.exists(args.path), f"File {args.path} already exists!"
    assert args.w >= 50, "small, might be mistake?"
    assert args.h >= 50, "small"
    assert args.l == 8, "use 8 layers please!"

    width, height, layers = args.w, args.h, args.l

    with open(args.path, "wb") as stream:
        stream.write(struct.pack("<III", width, height, layers))
        for layer in range(layers):
            for y in range(height):
                for x in range(width):
                    
                    value = 0
                    stream.write(struct.pack("<I", value))
