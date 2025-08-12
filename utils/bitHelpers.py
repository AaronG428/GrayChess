import numpy as np

def display_bb(bb: int) -> str:
    board_string = ""
    for i in range(8):
        for j in range(8):
            board_string += "|"
            if ((1 << (63 - (8 * i + j))) & bb) != 0:
                board_string += "X"
            else:
                board_string += " "
        board_string += "|\n"
    print(hex(bb))
    print(board_string)
    return board_string

display_bb(0xEFEFEFEFEFEFEFEF);
display_bb(0x7F7F7F7F7F7F7F7F);
display_bb(0x3B3B3B3B3B3B3B3B);
display_bb(0x00FF00FF00FF00FF);
display_bb(0x00FF00FF00FF00FF);
display_bb(0x00FF00FF00FF00FF);

display_bb(0xff00);

display_bb((0xff00 << 7) ^ 0xFEFEFEFEFEFEFEF)

display_bb(0x007e7e7e7e7e7e00)