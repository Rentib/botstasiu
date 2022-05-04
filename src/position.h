/* See LICENSE file for file for copyright and license details */
#ifndef __POSITION_H__
#define __POSITION_H__

#include "bitboards.h"
#include "chesslib.h"

typedef uint64_t Key;

typedef struct {
  Color     turn;
  U64       color[2];
  U64       empty;
  U64       piece[6];
  PieceType board[64];
  Square    ksq[2];
  Square    en_passant;
  int       castle; /* QqKk (bitfield) */
  int       fifty_move_rule;
  int       full_move_count;
  Key       key; /* zobrist hash of a position */
} Position;

void initialise_zobrist_keys(void);
void print_position(const Position *pos);
U64 attackers_to(const Position *pos, Square sq, U64 occ);
int is_legal(const Position *pos, Move m);

#endif /* __POSITION_H__ */
