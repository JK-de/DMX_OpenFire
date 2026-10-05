"""Write src/perm_table.inc: a Fisher-Yates shuffle of every uint16.

Seed and xorshift32 are fixed so every regeneration matches.
"""

import os

SEED = 0xC0FFEE01
COUNT = 65536


def xorshift32(state):
    state &= 0xFFFFFFFF
    state ^= (state << 13) & 0xFFFFFFFF
    state ^= state >> 17
    state ^= (state << 5) & 0xFFFFFFFF
    return state & 0xFFFFFFFF


def permutation():
    values = list(range(COUNT))
    state = SEED
    for i in range(COUNT - 1, 0, -1):
        state = xorshift32(state)
        j = state % (i + 1)
        values[i], values[j] = values[j], values[i]
    return values


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    path = os.path.join(root, "src", "perm_table.inc")
    values = permutation()
    with open(path, "w", encoding="ascii", newline="\n") as out:
        out.write(f"/* Fisher-Yates of 0..65535, xorshift32 seed 0x{SEED:08X}. */\n")
        for i in range(0, COUNT, 8):
            chunk = ", ".join(f"0x{values[i + k]:04X}" for k in range(8))
            comma = "," if i + 8 < COUNT else ""
            out.write(f"{chunk}{comma}\n")
    print(path)


if __name__ == "__main__":
    main()
