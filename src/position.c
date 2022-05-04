/* See LICENSE file for file for copyright and license details */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bitboards.h"
#include "chesslib.h"
#include "misc.h"
#include "position.h"

static inline void add_piece(Position *pos, PieceType pt, Color c, Square sq);
static inline void rem_piece(Position *pos, PieceType pt, Color c, Square sq);
static inline void add_enpas(Position *pos, Square sq);
static inline void rem_enpas(Position *pos);
static inline void update_castle(Position *pos, Square from, Square to);
static inline void switch_turn(Position *pos);

/* Numbers for & operation that update castle rights. */
static const int update_castle_rights[64] = {
  13, 15, 15, 15,  5, 15, 15,  7,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  14, 15, 15, 15, 10, 15, 15, 11,
};

/* Zobrist keys. */
static Key turnKey;
static Key pieceKey[2][6][64]; /* [Color][PieceType][Square] */
static Key enpasKey[8];        /* [File] */
static Key castleKey[16];      /* [CastleMask] */

/* Adds piece to the given square, assumes that nothing is there. */
static inline void
add_piece(Position *pos, PieceType pt, Color c, Square sq)
{
  U64 bb = get_bitboard(sq);
  pos->board[sq]  = pt;
  pos->color[c]  |= bb;
  pos->piece[pt] |= bb;
  pos->key       ^= pieceKey[c][pt][sq];
}

/* Removes piece from the given square, assumes that something is there. */
static inline void
rem_piece(Position *pos, PieceType pt, Color c, Square sq)
{
  U64 bb = get_bitboard(sq);
  pos->board[sq]  = NONE;
  pos->color[c]  ^= bb;
  pos->piece[pt] ^= bb;
  pos->key       ^= pieceKey[c][pt][sq];
}

/* Adds en passant. */
static inline void
add_enpas(Position *pos, Square sq)
{
  pos->en_passant = sq;
  pos->key ^= enpasKey[sq & 7]; /* sq & 7 == sq % 7 (file of square) */
}

/* Removes en passant. */
static inline void
rem_enpas(Position *pos)
{
  if (pos->en_passant != SQ_NONE) {
    pos->key ^= enpasKey[pos->en_passant & 7];
    pos->en_passant = SQ_NONE;
  }
}

static inline void
update_castle(Position *pos, Square from, Square to)
{
  pos->key ^= castleKey[pos->castle];
  pos->castle &= (update_castle_rights[from] & update_castle_rights[to]);
  pos->key ^= castleKey[pos->castle];
}

static inline void
switch_turn(Position *pos)
{
  pos->turn ^= 1;
  pos->key  ^= turnKey;
}

void
do_move(Position *pos, Move m)
{

}

void
initialise_zobrist_keys(void)
{
  turnKey = rand_uint64();
  for (Color c = WHITE; c <= BLACK; c++)
    for (PieceType pt = PAWN; pt <= KING; pt++)
      for (Square sq = SQ_A8; sq <= SQ_H1; sq++)
        pieceKey[c][pt][sq] = rand_uint64();
  for (File f = 0; f < 8; f++)
    enpasKey[f] = rand_uint64();
  for (int castle_mask = 0; castle_mask < 16; castle_mask++)
    castleKey[castle_mask] = rand_uint64();
}

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
