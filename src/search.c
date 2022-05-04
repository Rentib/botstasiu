#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "movegen.h"
#include "position.h"

static uint64_t perft_help(Position *pos, int depth);
static inline void print_move(Move m);

void
search(Position *pos, int depth)
{
  printf("bestmove ");
  print_move(make_move(SQ_E2, SQ_E4));
  printf("\n");
}

void
perft(Position *pos, int depth)
{
}

static const char *sq_to_str[] = {
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
  "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
  "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
  "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
  "-",
};

static const char *pc_to_str[] = { "p", "n", "b", "r", "q", "k" };

static inline void print_move(Move m)
{
  printf("%s%s%s", sq_to_str[from_sq(m)], sq_to_str[to_sq(m)],
                   type_of(m) == PROMOTION ?
                   pc_to_str[KNIGHT + promotion_type(m)] : "");
}

