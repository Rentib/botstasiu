/* See LICENSE file for file for copyright and license details */
#ifndef __POSITION_H__
#define __POSITION_H__

#include "bitboards.h"
#include "chesslib.h"

typedef uint64_t Key;

typedef struct State State;
struct State {
  Square    en_passant;
  int       castle; /* QqKk (bitfield) */
  int       fifty_move_rule;
  PieceType captured;
  State    *prev;
};

typedef struct {
  Color     turn;
  U64       color[2];
  U64       empty;
  U64       piece[6];
  PieceType board[64];
  Square    ksq[2];
  Key       key; /* zobrist hash of a position */
  int       game_ply; /* ply of game */
  int       ply;      /* ply of search */
  State    *st;
} Position;

void do_move(Position *pos, Move m);
void undo_move(Position *pos, Move m);
void initialise_zobrist_keys(void);
void print_position(const Position *pos);
void set_position(Position *pos, const char *fen);
U64 attackers_to(const Position *pos, Square sq, U64 occ);
int is_legal(const Position *pos, Move m);

#endif /* __POSITION_H__ */
