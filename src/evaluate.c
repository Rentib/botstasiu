/* See LICENSE file for file for copyright and license details */
#include "chesslib.h"
#include "evaluate.h"
#include "position.h"

#define FLIP(square)   ((square) ^ 56)
#define S(a, b)        ((Score){ .mg = a, .eg = b })
#define ScoreSum(a, b) ((Score){ .mg = a.mg + b.mg, .eg = a.eg + b.eg })
#define ScoreSub(a, b) ((Score){ .mg = a.mg - b.mg, .eg = a.eg - b.eg })

typedef struct {
  int mg; /* middle game score */
  int eg; /* end game score */
} Score;

static const int PawnTable[64] = {
   0,  0,  0,  0,  0,  0,  0,  0,
  20,  5, -1, -2, -2, -1,  5, 20,
   5,  0,  0,  0,  0,  0,  0,  5,
  10,  0,  0, 10,  5,  0,  0, 10,
  10,  5,  5, 20, 20,  5,  0, 10,
  10, 10, 10,  0,  0, -5,  0, 15,
  20, 20, 20, 30, 30, 30, 20, 20,
   0,  0,  0,  0,  0,  0,  0,  0,
};

static const int KnightTable[64] = {
  -5,  0,  0,  0,  0,  0,  0, -5,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0, 20, 10, 10, 20,  0,  0,
   0, 10, 20, 20, 20, 20, 10,  0,
   0,  0, 10, 20, 20, 10,  0,  0,
   5,  5, 20, 10, 10, 25,  5,  5,
   0,  0,  0,  0,  0,  0,  0,  0,
  -5,  0,  0,  0,  0,  0,  0, -5,
};

static const int BishopTable[64] = {
   0,  0, -2, -5, -5, -2,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0, 15,  2,  5,  5,  2, 15,  0,
   0,  0, 20,  5,  5, 20,  0,  0,
   0, 10,  1, 10, 10,  1, 10,  0,
   5, 11,  0,  5,  5,  0, 11,  5,
  10, 10, -1,  0,  0, -1, 10, 10,
};

static const int RookTable[64] = {
   0,  0,  0,  0,  0,  0,  0,  0,
  25, 25, 20, 20, 20, 20, 25, 25,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0, 20, 20,  2,  0,  5,
   0,  0,  0, 20, 20,  0,  0,  0,
   0,  0,  5, 20, 20,  5,  0,  0,
};

void
initialise_evaluation(void)
{
}

int
evaluate(const Position *pos)
{
  int value = pos->material[WHITE] - pos->material[BLACK];
  U64 mask;
  Square sq;

  mask = pos->piece[PAWN] & pos->color[WHITE];
  while (mask) {
    sq = pop_lsb(&mask);
    value += PawnTable[sq];
  }
  mask = pos->piece[PAWN] & pos->color[BLACK];
  while (mask) {
    sq = FLIP(pop_lsb(&mask));
    value -= PawnTable[sq];
  }

  mask = pos->piece[KNIGHT] & pos->color[WHITE];
  while (mask) {
    sq = pop_lsb(&mask);
    value += KnightTable[sq];
  }
  mask = pos->piece[KNIGHT] & pos->color[BLACK];
  while (mask) {
    sq = FLIP(pop_lsb(&mask));
    value -= KnightTable[sq];
  }

  mask = pos->piece[BISHOP] & pos->color[WHITE];
  while (mask) {
    sq = pop_lsb(&mask);
    value += BishopTable[sq];
  }
  mask = pos->piece[BISHOP] & pos->color[BLACK];
  while (mask) {
    sq = FLIP(pop_lsb(&mask));
    value -= BishopTable[sq];
  }

  mask = pos->piece[ROOK] & pos->color[WHITE];
  while (mask) {
    sq = pop_lsb(&mask);
    value += RookTable[sq];
  }
  mask = pos->piece[ROOK] & pos->color[BLACK];
  while (mask) {
    sq = FLIP(pop_lsb(&mask));
    value -= RookTable[sq];
  }

  return pos->turn == WHITE ? value : -value;
}
