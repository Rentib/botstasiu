/* See LICENSE file for file for copyright and license details */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bitboards.h"
#include "chesslib.h"
#include "misc.h"
#include "position.h"

void
print_position(const Position *pos)
{
  const char *sep = "  +---+---+---+---+---+---+---+---+";
  printf("%s\n", sep);
  for (Rank r = 8; r >= 1; r--) {
    printf("%d ", r);
    for (File f = 1; f <= 8; f++) {
      Square sq = (8 - r) * 8 + f - 1;
      char c = piece_to_char(pos->board[sq]);
      if (get_bit(pos->color[BLACK], sq))
        c |= 32;
      printf("| %c ", c);
    }
    printf("|\n%s\n", sep);
  }
	printf("    a   b   c   d   e   f   g   h\n\n");
  printf("    Turn: %s", pos->turn ? "BLACK" : "WHITE");
  printf("    Enpassant: %s\n", pos->en_passant == SQ_NONE ? "NO" : "YES");
  printf("    Castling:      %c%c%c%c\n", 
         pos->castle & 4 ? 'K' : '-', pos->castle & 1 ? 'Q' : '-',
         pos->castle & 8 ? 'k' : '-', pos->castle & 2 ? 'q' : '-');
}