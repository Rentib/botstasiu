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
  /* MVV-LVA */
  set_score(m, 10000 + mvv[pos->board[to_sq(*m)]] - pos->board[from_sq(*m)]);
}

Move *
process_moves(Position *pos, Move *move_list, Move *last)
{
  Move *m;
  for (m = move_list; m != last; m++) {
    if (!is_legal(pos, *m)) continue;
    score_move(pos, m);
    *move_list++ = *m;
  }
  return move_list;
}
