/* See LICENSE file for file for copyright and license details */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chesslib.h"
#include "misc.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "uci.h"

#define startpos "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static Move parse_move(Position *pos, char *move_string);

static inline void isready(void);
static inline void uci(void);
static void go(Position *pos, char *input);
static void position(Position *pos, char *input);

static Move
parse_move(Position *pos, char *move_string)
{
  Move move_list[256];
  Move *m, *last;
  Square from, to;
  from = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
  to   = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;

  last = generate_moves(ALL, move_list, pos);
  for (m = move_list; m != last; m++) {
    if (from != from_sq(*m) || to != to_sq(*m) || !is_legal(pos, *m)) continue;
    if (type_of(*m) == PROMOTION) {
      if ((promotion_type(*m) == KNIGHT && move_string[4] == 'n')
      ||  (promotion_type(*m) == BISHOP && move_string[4] == 'b')
      ||  (promotion_type(*m) ==   ROOK && move_string[4] == 'r')
      ||  (promotion_type(*m) ==  QUEEN && move_string[4] == 'q')) 
        return *m;
    } else {
      return *m;
    }
  }

  return MOVE_NONE;
}

static inline void
isready(void)
{
  printf("readyok\n");
}

static inline void
uci(void)
{
  printf("id name Botstasiu alpha\n");
  printf("id author Stanisław Bitner\n");
  printf("uciok\n");
}

static void
go(Position *pos, char *input)
{
  int depth = -1, movestogo = 30, movetime = -1;
  int time = -1, inc = 0;
  char *token = NULL;

  info.timeset = 0;

  if ((token = strstr(input, "infinite")))
    depth = MAX_PLY;
  if ((token = strstr(input, "winc")) && pos->turn == WHITE)
    inc = atoi(token + 5);
  if ((token = strstr(input, "binc")) && pos->turn == BLACK)
    inc = atoi(token + 5);
  if ((token = strstr(input, "wtime")) && pos->turn == WHITE)
    time = atoi(token + 6);
  if ((token = strstr(input, "btime")) && pos->turn == BLACK)
    time = atoi(token + 6);
  if ((token = strstr(input, "movestogo")))
    movestogo = atoi(token + 10);
  if ((token = strstr(input, "movetime")))
    movetime = atoi(token + 9);
  if ((token = strstr(input, "depth")))
    depth = atoi(token + 6);

  info.starttime = get_time();

  info.depth = depth == -1 ? MAX_PLY : depth;

  if (movetime != -1) {
    time = movetime;
    movestogo = 1;
  }

  if (time != -1) {
    info.timeset = 1;
    time /= movestogo;
    if (time > 1500) time -= 50;
    info.stoptime = info.starttime + time + inc;
  }

  search(pos);
}

static void
position(Position *pos, char *input)
{
  char *token;
  input += 9; /* skip "position " */

  if (!strncmp(input, "startpos", 8)) {
    set_position(pos, startpos);
    input += 8;
  } else {
    token = strstr(input, "fen");
    if (token == NULL)
      set_position(pos, startpos);
    else {
      token += 4; /* skip "fen " */
      set_position(pos, token);
    }
  }
  token = strstr(input, "moves");
  if (token != NULL) {
    token += 6; /* skip "moves " */
    Move m;
    while (*token) {
      if ((m = parse_move(pos, token)) == MOVE_NONE)
        break;
      do_move(pos, m);
      while (*token && *token++ != ' ');
    }
  }
}

void
uci_loop(void)
{
  Position pos = (Position){ .tt = NULL, .st = NULL };
  set_position(&pos, startpos);

  setbuf(stdin,  NULL);
  setbuf(stdout, NULL);
  char input[6969];

  info.quit = 0;

  while (!info.quit) {
    memset(input, 0, sizeof(input));
    fflush(stdout);

    if (!fgets(input, 6969, stdin) ||  input[0] == '\n')
      continue;

    if (!strncmp(input, "isready", 7))
      isready();
    else if (!strncmp(input, "ucinewgame", 10))
      position(&pos, "position startpos");
    else if (!strncmp(input, "uci", 3))
      uci();
    else if (!strncmp(input, "position", 8))
      position(&pos, input);
    else if (!strncmp(input, "go", 2))
      go(&pos, input);
    else if (!strncmp(input, "d", 1))
      print_position(&pos);
    else if (!strncmp(input, "stop", 4))
      info.stopped = 1;
    else if (!strncmp(input, "quit", 4)
         ||  !strncmp(input, "q", 1))
      info.quit = 1;
    else
      printf("Unknown command: %s", input);
  }

  while (pos.st) {
    State *xd = pos.st->prev;
    free(pos.st);
    pos.st = xd;
  }
  tt_delete(pos.tt);
}
