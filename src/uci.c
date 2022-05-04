/* See LICENSE file for file for copyright and license details */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chesslib.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "uci.h"

#define startpos "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static Move parse_move(Position *pos, char *move_string);
static inline void uci_isready(void);
static inline void uci_ucinewgame(Position *pos);
static inline void uci_uci(void);
static void uci_go(Position *pos, char *input);
static void uci_position(Position *pos, char *input);

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
    if (from != from_sq(*m) || to != to_sq(*m)
    || !is_legal(pos, *m))
      continue;
    if (type_of(*m) == PROMOTION) {
      if (promotion_type(*m) == KNIGHT && move_string[4] == 'n')
        return *m;
      if (promotion_type(*m) == BISHOP && move_string[4] == 'b')
        return *m;
      if (promotion_type(*m) ==   ROOK && move_string[4] == 'r')
        return *m;
      if (promotion_type(*m) ==  QUEEN && move_string[4] == 'q')
        return *m;
    } else {
      return *m;
    }
  }

  return MOVE_NONE;
}

static inline void
uci_isready(void)
{
  printf("readyok\n");
}

static inline void
uci_ucinewgame(Position *pos)
{
  set_position(pos, startpos);
}

static inline void
uci_uci(void)
{
  printf("Botstasiu alpha by Stanisław Bitner\n");
}

static void
uci_go(Position *pos, char *input)
{
  int depth = -1;
  char *token = NULL;
  int type = 0;

  /* input += 3; /1* skip "go " *1/ */
  /* fixed depth search */
  if ((token = strstr(input, "depth")))
    type = 1;
  else if ((token = strstr(input, "perft")))
    type = 2;
  if (type)
    depth = atoi(token + 6); /* skip "depth " */
     
  if (type == 1) {
    search(pos, depth);
  } else if (type == 2) {
    perft(pos, depth);
  }
}

static void
uci_position(Position *pos, char *input)
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
  Position pos;
  set_position(&pos, startpos);
  setbuf(stdin,  NULL);
  setbuf(stdout, NULL);
  char input[6969];

  while (1) {
    memset(input, 0, sizeof(input));
    fflush(stdout);

    if (!fgets(input, 6969, stdin)
    ||  input[0] == '\n')
      continue;

    if (!strncmp(input, "isready", 7))
      uci_isready();
    else if (!strncmp(input, "ucinewgame", 10))
      uci_ucinewgame(&pos);
    else if (!strncmp(input, "uci", 3))
      uci_uci();
    else if (!strncmp(input, "position", 8))
      uci_position(&pos, input);
    else if (!strncmp(input, "go", 2))
      uci_go(&pos, input);
    else if (!strncmp(input, "d", 1))
      print_position(&pos);
    else if (!strncmp(input, "quit", 4)
         ||  !strncmp(input, "q", 1))
      break;
    else
      printf("Unknown command: %s", input);
  }
}
