#include <stdio.h>
#include <string.h>

#include "chesslib.h"
#include "evaluate.h"
#include "misc.h"
#include "movegen.h"
#include "moveorder.h"
#include "position.h"
#include "search.h"

static int quiescence(Position *pos, int alpha, int beta);
static int negamax(Position *pos, PV *pv, int alpha, int beta, int depth);
static uint64_t perft_help(Position *pos, int depth);
static inline void print_move(Move m);

SearchInfo info;

static int
quiescence(Position *pos, int alpha, int beta)
{
  int value = evaluate(pos);
  if (value >= beta)
    return beta;
  if (value > alpha)
    alpha = value;

  Move *m, *last, move_list[256];

  info.nodes++;

  last = generate_moves(CAPTURES, move_list, pos);
  last = process_moves(pos, move_list, last, MOVE_NONE);

  sort_moves(move_list, last);
  for (m = move_list; m != last; m++) {
    do_move(pos, *m);
    value = -quiescence(pos, -beta, -alpha);
    undo_move(pos, *m);
    if (value >= beta)
      return beta;
    if (value > alpha)
      alpha = value;
  }

  return alpha;
}

static int
negamax(Position *pos, PV *pv, int alpha, int beta, int depth)
{
  int value = -INFINITY;
  int ksq = pos->ksq[pos->turn];
  U64 checkers = attackers_to(pos, ksq, ~pos->empty) & pos->color[!pos->turn];
  Move *m, *last, move_list[256];
  Move hash_move = MOVE_NONE;

  PV new_pv;
  pv->cnt = 0;

  if (pos->ply >= MAX_PLY) return evaluate(pos);

  /* dont end search on check */
  if (depth <= 0) {
    if (!checkers)
      return quiescence(pos, alpha, beta);
    depth = 1;
  }

  info.nodes++;

  last = generate_moves(ALL, move_list, pos);
  last = process_moves(pos, move_list, last, hash_move);

  /* checkmate or stalemate */
  if (move_list == last)
    return checkers ? pos->ply - MATE_VALUE : 0;

  sort_moves(move_list, last);
  for (m = move_list; m != last; m++) {
    do_move(pos, *m);

    value = -negamax(pos, &new_pv, -beta, -alpha, depth - 1);

    undo_move(pos, *m);
    if (value >= beta) {
      if (pos->board[to_sq(*m)] == NONE) { /* found killer */
        pos->killer[1][pos->ply] = pos->killer[0][pos->ply];
        pos->killer[0][pos->ply] = *m;
      }
      return beta;
    }
    if (value > alpha) {
      pv->m[0] = *m;
      pv->cnt = new_pv.cnt + 1;
      memcpy(pv->m + 1, new_pv.m, new_pv.cnt * sizeof(Move));
      alpha = value;

      if (pos->board[to_sq(*m)] == NONE) {
        pos->history[pos->turn][pos->board[from_sq(*m)]][to_sq(*m)] += depth;
      }
    }
  }

  return alpha;
}

void
search(Position *pos)
{
  int value;
  int alpha = -INFINITY, beta = INFINITY;
  PV pv;

  pos->ply = 0;
  info.nodes = 0;
  memset(pos->killer, MOVE_NONE, sizeof(pos->killer));
  memset(pos->history, 0, sizeof(pos->history));

  for (int depth = 1; depth <= info.depth; depth++, info.nodes = 0) {
    info.beg_time = get_time();
    value = -negamax(pos, &pv, alpha, beta, depth);
    info.end_time = get_time();

    printf("info depth %d score %d nodes %lu time %d pv",
           depth, value, info.nodes, info.end_time - info.beg_time);

    for (int i = 0; i < pv.cnt; i++) {
      printf(" ");
      print_move(pv.m[i]);
    }
    printf("\n");
  }

  printf("bestmove ");
  print_move(pv.m[0]);
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
      do_move(pos, *m);
      nodes_searched += perft_help(pos, depth - 1);
      undo_move(pos, *m);
    }
  }
  return nodes_searched;
}

void
perft(Position *pos)
{
  uint64_t nodes_searched = 0ULL, cnt;
  Move *m, *last, move_list[256];
  int t = get_time(); /* start time */
  last = generate_moves(ALL, move_list, pos);
  for (m = move_list; m != last; m++) {
    if (!is_legal(pos, *m)) continue;
    do_move(pos, *m);
    cnt = perft_help(pos, info.depth - 1);
    undo_move(pos, *m);
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

