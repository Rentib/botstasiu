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

U64
attackers_to(const Position *pos, Square sq, U64 occ)
{
  return (pawn_attacks_bb(WHITE, sq)  & pos->piece[PAWN] & pos->color[BLACK])
       | (pawn_attacks_bb(BLACK, sq)  & pos->piece[PAWN] & pos->color[WHITE])
       | (attacks_bb(KNIGHT, sq, occ) &  pos->piece[KNIGHT])
       | (attacks_bb(BISHOP, sq, occ) & (pos->piece[BISHOP] | pos->piece[QUEEN]))
       | (attacks_bb(  ROOK, sq, occ) & (pos->piece[  ROOK] | pos->piece[QUEEN]))
       | (attacks_bb(  KING, sq, occ) &  pos->piece[  KING]);
}

int
is_legal(const Position *pos, Move m)
{
  Color us = pos->turn, them = !us;
  Square ksq = pos->ksq[us];
  Square from = from_sq(m), to = to_sq(m);
  U64 from_bb = get_bitboard(from), to_bb = get_bitboard(to);
  U64 occupancy = (~pos->empty ^ from_bb) | to_bb;
  U64 enemies = pos->color[them] & ~to_bb;
  
  if (from == ksq)
    return !(attackers_to(pos, to, ~pos->empty ^ from_bb) & enemies);

  if (type_of(m) == EN_PASSANT) {
    Square capsq = to - (us == WHITE ? -8 : 8);
    occupancy ^= get_bitboard(capsq);
  }
  
  return !((attacks_bb(BISHOP, ksq, occupancy) & 
            enemies & (pos->piece[QUEEN] | pos->piece[BISHOP])))
      && !((attacks_bb(  ROOK, ksq, occupancy) &
            enemies & (pos->piece[QUEEN] | pos->piece[  ROOK])));
}
