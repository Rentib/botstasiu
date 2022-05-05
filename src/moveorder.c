/* See LICENSE file for file for copyright and license details */
#include "chesslib.h"
#include "moveorder.h"
#include "position.h"

/* Scores move from 1 to 16000. */
static inline void score_move(Position *pos, Move *m);

/* Sets score of move *m to value val.
   1 <= val <= 16000 */
static inline void
set_score(Move *m, int val)
{
  *m = (Move)(*m + (val << 16));
}

/* most valuable victim */
static const int mvv[] = {
  [  PAWN] = 200,
  [KNIGHT] = 300,
  [BISHOP] = 350,
  [  ROOK] = 500,
  [ QUEEN] = 1000,
  [  KING] = -1,  /* it should not be possible to capture a king */
  [  NONE] = 100, /* move is not a capture */
};

static inline void
score_move(Position *pos, Move *m)
{
  Square from = from_sq(*m), to = to_sq(*m);
  switch (type_of(*m)) {
  case NORMAL:
    if (pos->board[to] != NONE) { /* apply mvv-lva */
      set_score(m, 9000 + mvv[pos->board[to]] - pos->board[from]);
    } else {
      if (pos->killer[0][pos->ply] == *m) {
        set_score(m, 8000 + pos->board[from]);
      } else if (pos->killer[1][pos->ply] == *m) {
        set_score(m, 7000 + pos->board[from]);
      } else {
        set_score(m, pos->history[pos->turn][pos->board[from]][to]);
      }
    }
    break;
  case EN_PASSANT:
    set_score(m, 7000);
    break;
  case PROMOTION:
    set_score(m, 10000);
    break;
  case CASTLE:
    set_score(m, 1000);
  }
}

Move *
process_moves(Position *pos, Move *move_list, Move *last, Move pvmove)
{
  Move *m;
  for (m = move_list; m != last; m++) {
    if (!is_legal(pos, *m)) continue;
    if (*m == pvmove) {
      set_score(m, 15000);
    } else {
      score_move(pos, m);
    }
    *move_list++ = *m;
  }
  return move_list;
}

void
sort_moves(Move *begin, Move *end)
{
  for (Move *m = begin, *p = begin + 1; p < end; p++) {
    Move tmp = *p, *q;
    *p = *++m;
    for (q = m; q != begin && *(q - 1) < tmp; --q)
      *q = *(q - 1);
    *q = tmp;
  }
}
