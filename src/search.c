#include <stdio.h>
#include <string.h>

#include "chesslib.h"
#include "evaluate.h"
#include "misc.h"
#include "movegen.h"
#include "moveorder.h"
#include "position.h"
#include "search.h"

static int negamax(Position *pos, int alpha, int beta, int depth);
static uint64_t perft_help(Position *pos, int depth);
static inline void print_move(Move m);

SearchData data;

static int
negamax(Position *pos, int alpha, int beta, int depth)
{
  int value = -INFINITY;
  int ksq = pos->ksq[pos->turn];
  U64 checkers = attackers_to(pos, ksq, ~pos->empty) & pos->color[!pos->turn];
  Move *m, *last, move_list[256];
  Position tmp; /* used instead of undo move */
  if (data.ply >= MAX_PLY) return evaluate(pos);

  /* dont end search on check */
  if (depth <= 0) {
    if (!checkers)
      return evaluate(pos);
    depth = 1;
  }

  data.nodes++;

  last = generate_moves(ALL, move_list, pos);
  last = process_moves(pos, move_list, last);

  /* checkmate or stalemate */
  if (move_list == last)
    return checkers ? data.ply - MATE_VALUE : 0;

  for (m = move_list; m != last; m++) {
    tmp = *pos;
    do_move(&tmp, *m);
    data.ply++;

    value = -negamax(&tmp, -beta, -alpha, depth - 1);

    data.ply--;
    if (value >= beta)
      return beta;
    if (value > alpha) {
      if (data.ply == 0) data.best_move = *m;
      alpha = value;
    }
  }

  return alpha;
}

void
search(Position *pos, int depth)
{
  int alpha = -INFINITY, beta = INFINITY;

  data.nodes = 0ULL;
  data.ply = 0;
  data.best_move = MOVE_NONE;
  data.beg_time = get_time();
  data.score = -negamax(pos, alpha, beta, depth);
  data.end_time = get_time();

  printf("info depth %d score %d nodes %lu time %d pv ...",
         depth, data.score, data.nodes, data.end_time - data.beg_time);
  printf("\n");

  printf("bestmove ");
  print_move(data.best_move);
  printf("\n");
}

U64
perft_help(Position *pos, int depth)
{
  if (depth == 0) return 1ULL;
  Move *m, *last, move_list[256];
  uint64_t nodes_searched = 0ULL;
  last = generate_moves(ALL, move_list, pos);
  if (depth == 1) {
    for (m = move_list; m != last; m++)
      nodes_searched += is_legal(pos, *m);
  } else {
    for (m = move_list; m != last; m++) {
      if (!is_legal(pos, *m)) continue;
      Position tmp = *pos;
      do_move(&tmp, *m);
      nodes_searched += perft_help(&tmp, depth - 1);
    }
  }
  return nodes_searched;
}

void
perft(Position *pos, int depth)
{
  uint64_t nodes_searched = 0ULL, cnt;
  Move *m, *last, move_list[256];
  int t = get_time(); /* start time */
  last = generate_moves(ALL, move_list, pos);
  for (m = move_list; m != last; m++) {
    if (!is_legal(pos, *m)) continue;
    Position tmp = *pos;
    do_move(&tmp, *m);
    cnt = perft_help(&tmp, depth - 1);
    nodes_searched += cnt;
    print_move(*m);
    printf(": %lu\n", cnt);
  }
  printf("\nNodes searched: %lu (%dms)\n\n", nodes_searched, get_time() - t);
}

static inline void
print_move(Move m)
{
  static const char *pc_to_str[] = { "p", "n", "b", "r", "q", "k" };

  const char *t[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "--",
  };

  printf("%s%s%s", t[from_sq(m)],t[to_sq(m)],
                   type_of(m) == PROMOTION ?
                   pc_to_str[KNIGHT + promotion_type(m)] : "");
}

