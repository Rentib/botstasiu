/* See LICENSE file for file for copyright and license details */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "chesslib.h"
#include "evaluate.h"
#include "misc.h"
#include "movegen.h"
#include "moveorder.h"
#include "position.h"
#include "search.h"

#define ASPIRATION 30

static inline void listen(void);
static int quiescence(Position *pos, int alpha, int beta);
static int negamax(Position *pos, PV *pv, int alpha, int beta, int depth, int cutnode);
static uint64_t perft_help(Position *pos, int depth);
static inline void print_move(Move m);

SearchInfo info;

// http://home.arcor.de/dreamlike/chess/
int input_waiting()
{
	struct timeval tv;
	fd_set readfds;

	FD_ZERO (&readfds);
	FD_SET (fileno(stdin), &readfds);
	tv.tv_sec=0; tv.tv_usec=0;
	select(16, &readfds, 0, 0, &tv);

	return (FD_ISSET(fileno(stdin), &readfds));
}

void read_input() {
	int bytes;
	char input[256] = "", *endc;

	if(input_waiting()) {
		info.stopped = 1;
		do {
			bytes=read(fileno(stdin), input, 256);
		} while (bytes<0);
		endc = strchr(input, '\n');
		if (endc)
			*endc = 0;

		if (strlen(input) > 0) {
			if (!strncmp(input, "quit", 4))	{
				info.quit = 1;
			} else if (!strncmp(input, "stop", 4))
        info.stopped = 1;
		}
		return;
	}
}

static inline void
listen(void)
{
  if (info.timeset && get_time() > info.stoptime)
    info.stopped = 1;
  read_input();
}

static inline int
is_rep(Position *pos)
{
  int i = pos->game_ply - 1;
  int n = 1;
  while (i >= pos->game_ply - pos->st->fifty_move_rule && n < 3)
    n += (pos->reps[i--] == pos->key);
  return n >= 3;
}

static int
quiescence(Position *pos, int alpha, int beta)
{

  int value = evaluate(pos);
  if (value >= beta)
    return beta;
  if (value > alpha)
    alpha = value;

  Move *m, *last, move_list[256];

  if (!(info.nodes++ & 4095)) listen();

  last = generate_moves(CAPTURES, move_list, pos);
  last = process_moves(pos, move_list, last, MOVE_NONE);

  sort_moves(move_list, last);
  for (m = move_list; m != last; m++) {
    do_move(pos, *m);
    value = -quiescence(pos, -beta, -alpha);
    undo_move(pos, *m);

    if (info.stopped)
      return 0;

    if (value >= beta)
      return beta;
    if (value > alpha)
      alpha = value;
  }

  return alpha;
}

static int
negamax(Position *pos, PV *pv, int alpha, int beta, int depth, int cutnode)
{
  PV new_pv;
  pv->cnt = 0;

  int value      = -INFINITY;
  int best_value = -INFINITY;
  int old_alpha  = alpha;
  int is_root    = pos->ply == 0;

  Move *m, *last, move_list[256];
  Move best_move = MOVE_NONE;
  Move hash_move = MOVE_NONE;

  Square ksq = pos->ksq[pos->turn];
  U64 checkers = attackers_to(pos, ksq, ~pos->empty) & pos->color[!pos->turn];


  if (!is_root) {
    if (pos->ply >= MAX_PLY)
      return checkers ? 0 : evaluate(pos);

    /* 50 moves with no pawn move / capture or 3fold repetition */
    if (pos->st->fifty_move_rule >= 100 || is_rep(pos))
      return 0;

    /* dont end search if in check */
    if (depth <= 0) {
      if (!checkers)
        return quiescence(pos, alpha, beta);
      depth = 1;
    }
  }

  if (!(info.nodes++ & 4095)) listen();

  /* null move prunning */
  if (cutnode && !is_root && !checkers && depth >= 4 
  && ((pos->piece[QUEEN] | pos->piece[ROOK]) & pos->color[pos->turn])) {
    do_null_move(pos);
    value = -negamax(pos, &new_pv, -beta, -beta + 1, depth - 4, 0);
    undo_null_move(pos);
    if (value >= beta)
      return beta;
  }

  hash_move = tt_probe(pos->tt, pos->key);
  last = generate_moves(ALL, move_list, pos);
  last = process_moves(pos, move_list, last, hash_move);

  /* checkmate or stalemate */
  if (move_list == last)
    return checkers ? pos->ply - MATE_VALUE : 0;

  sort_moves(move_list, last);
  for (m = move_list; m != last; m++) {
    do_move(pos, *m);

    value = -negamax(pos, &new_pv, -beta, -alpha, depth - 1, 1);

    undo_move(pos, *m);
  
    if (info.stopped)
      return 0;

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
      best_value = value;
      best_move = *m;

      if (pos->board[to_sq(*m)] == NONE) {
        pos->history[pos->turn][pos->board[from_sq(*m)]][to_sq(*m)] += depth;
      }
    }
  }

  if (alpha != old_alpha)
    tt_store(pos->tt, pos->key, best_move);

  return alpha;
}

void
search(Position *pos)
{
  int value;
  int alpha = -INFINITY, beta = INFINITY;
  PV pv;
  Move bestmove = MOVE_NONE;

  pos->ply = 0;
  tt_clear(pos->tt);

  info.stopped = 0;
  info.nodes = 0;

  memset(pos->killer, MOVE_NONE, sizeof(pos->killer));
  memset(pos->history, 0, sizeof(pos->history));

  for (int depth = 1; depth <= info.depth; depth++, info.nodes = 0) {
    value = negamax(pos, &pv, alpha, beta, depth, 0);
    if (value <= alpha || value >= beta) {
      alpha = -INFINITY;
      beta  =  INFINITY;
      depth--;
      continue;
    }

    if (depth > 4) {
      alpha = value - ASPIRATION;
      beta  = value + ASPIRATION;
    }

    if (info.stopped)
      break;

    printf("info depth %d score cp %d nodes %lu pv",
           depth, value, info.nodes);

    bestmove = pv.m[0];
    for (int i = 0; i < pv.cnt; i++) {
      printf(" ");
      print_move(pv.m[i]);
    }
    printf("\n");
  }

  printf("bestmove ");
  print_move(bestmove);
  printf("\n");
}

static U64
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

