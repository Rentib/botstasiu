/* See LICENSE file for file for copyright and license details */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  pos->st->en_passant = sq;
  pos->key ^= enpasKey[sq & 7]; /* sq & 7 == sq % 7 (file of square) */
}

/* Removes en passant. */
static inline void
rem_enpas(Position *pos)
{
  if (pos->st->en_passant != SQ_NONE) {
    pos->key ^= enpasKey[pos->st->en_passant & 7];
    pos->st->en_passant = SQ_NONE;
  }
}

static inline void
update_castle(Position *pos, Square from, Square to)
{
  pos->key ^= castleKey[pos->st->castle];
  pos->st->castle &= (update_castle_rights[from] & update_castle_rights[to]);
  pos->key ^= castleKey[pos->st->castle];
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
  Color us = pos->turn, them = !us;
  Square from = from_sq(m), to = to_sq(m);
  PieceType pt = pos->board[from], 
            captured = pos->board[to];

  /* move state to the next one */
  State *st = malloc(sizeof(State));
  *st = *(pos->st);
  st->prev = pos->st;
  pos->st = st;
  pos->st->captured = captured;
  pos->game_ply++;
  pos->ply++;

  /* move is a capture */
  if (captured != NONE)
    rem_piece(pos, captured, them, to);

  rem_piece(pos, pt, us, from);
  add_piece(pos, pt, us, to);

  rem_enpas(pos);

  if (pt == PAWN) {
    if (to - from == 16 || from - to == 16) {
      add_enpas(pos, to + (us == WHITE ? 8 : -8));
    } else if (type_of(m) == EN_PASSANT) {
      rem_piece(pos, PAWN, them, to + (us == WHITE ? 8 : -8));
    } else if (type_of(m) == PROMOTION) {
      rem_piece(pos, PAWN, us, to);
      add_piece(pos, promotion_type(m), us, to);
    }
  } else if (type_of(m) == CASTLE) { /* add rook move */
    if (from < to) { /* short */
      rem_piece(pos, ROOK, us, from + 3);
      add_piece(pos, ROOK, us, from + 1);
    } else { /* long */
      rem_piece(pos, ROOK, us, from - 4);
      add_piece(pos, ROOK, us, from - 1);
    }
  }
  switch_turn(pos);
  update_castle(pos, from, to);

  if (pt == KING)
    pos->ksq[us] = to;

  pos->empty = ~(pos->color[WHITE] | pos->color[BLACK]);

  /* TODO - 50 move rule, full move count */
}

void
undo_move(Position *pos, Move m)
{
  pos->key ^= castleKey[pos->st->castle];
  switch_turn(pos);
  Color us = pos->turn, them = !us;
  Square from = from_sq(m), to = to_sq(m);
  PieceType pt = type_of(m) == PROMOTION ? PAWN : pos->board[to],
            captured = pos->st->captured;

  if (pt == KING)
    pos->ksq[us] = from;

  if (pt == PAWN) {
    if (to - from == 16 || from - to == 16) {
      rem_enpas(pos);
    } else if (type_of(m) == EN_PASSANT) {
      add_piece(pos, PAWN, them, to + (us == WHITE ? 8 : -8));
    } else if (type_of(m) == PROMOTION) {
      rem_piece(pos, promotion_type(m), us, to);
      add_piece(pos, PAWN, us, to);
    }
  } else if (type_of(m) == CASTLE) {
    if (from < to) {
      rem_piece(pos, ROOK, us, from + 1);
      add_piece(pos, ROOK, us, from + 3);
    } else {
      rem_piece(pos, ROOK, us, from - 1);
      add_piece(pos, ROOK, us, from - 4);
    }
  }

  rem_piece(pos, pt, us, to);
  add_piece(pos, pt, us, from);

  if (captured != NONE)
    add_piece(pos, captured, them, to);

  State *st = pos->st->prev;
  free(pos->st);
  pos->st = st;
  pos->key ^= castleKey[pos->st->castle];
  pos->game_ply--;
  pos->ply--;

  pos->empty = ~(pos->color[WHITE] | pos->color[BLACK]);
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
  printf("    Enpassant: %s\n", pos->st->en_passant == SQ_NONE ? "NO" : "YES");
  printf("    Castling:      %c%c%c%c\n", 
         pos->st->castle & 4 ? 'K' : '-', pos->st->castle & 1 ? 'Q' : '-',
         pos->st->castle & 8 ? 'k' : '-', pos->st->castle & 2 ? 'q' : '-');
  printf("    Hash key:      %lx\n", pos->key);
}

void
set_position(Position *pos, const char *fen)
{
  pos->game_ply = 0;
  free(pos->st);
  pos->st = malloc(sizeof(State));
  pos->st->prev = NULL;
  pos->st->captured = NONE;
  pos->color[WHITE] = pos->color[BLACK] = 0ULL;
  for (PieceType pt = PAWN; pt <= KING; pt++)
    pos->piece[pt] = 0ULL;
  for (Square sq = SQ_A8; sq <= SQ_H1; sq++)
    pos->board[sq] = NONE;
  pos->turn = WHITE;
  pos->ksq[WHITE] = pos->ksq[BLACK] = SQ_NONE;
  pos->st->en_passant = SQ_NONE;
  pos->st->castle = 0;
  pos->key = 0ULL;

  /* board */
  for (Square sq = SQ_A8; sq <= SQ_H1; fen++) {
    if (*fen == '/') {
      continue;
    } else if ('0' <= *fen && *fen <= '9') {
      sq += *fen - '0';
    } else {
      char z = *fen | 32; /* lower case */
      Color c = (z == *fen);
      switch (z) {
      case 'p': add_piece(pos,   PAWN, c, sq); break;
      case 'n': add_piece(pos, KNIGHT, c, sq); break;
      case 'b': add_piece(pos, BISHOP, c, sq); break;
      case 'r': add_piece(pos,   ROOK, c, sq); break;
      case 'q': add_piece(pos,  QUEEN, c, sq); break;
      case 'k': add_piece(pos,   KING, c, sq); break;
      }
      sq++;
    }
  }
  pos->empty = ~(pos->color[WHITE] | pos->color[BLACK]);
  pos->ksq[WHITE] = get_square(pos->color[WHITE] & pos->piece[KING]);
  pos->ksq[BLACK] = get_square(pos->color[BLACK] & pos->piece[KING]);

  /* side */
  pos->turn = *(++fen) == 'b';
  if (pos->turn == BLACK)
    pos->key ^= turnKey;
  fen += 2;

  /* castling rights */
  while (*fen != ' ') {
    switch (*fen++) {
    case 'K': pos->st->castle |= 4; break;
    case 'Q': pos->st->castle |= 1; break;
    case 'k': pos->st->castle |= 8; break;
    case 'q': pos->st->castle |= 2; break;
    default: break;
    }
  }
  pos->key ^= castleKey[pos->st->castle];

  /* en passant */
  if (*(++fen) != '-') {
    File f = fen[0] - 'a';
    Rank r = 8 - (fen[1] - '0');
    add_enpas(pos, f + r * 8);
  }

  /* TODO */
  /* fifty move rule */

  /* full move count */
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
