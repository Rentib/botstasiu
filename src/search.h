/* See LICENSE file for file for copyright and license details */
#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "chesslib.h"
#include "position.h"

#define MAX_PLY    64
#define INFINITY   50000
#define MATE_VALUE 49000

typedef struct {
  uint64_t nodes;
  int      ply;
  int      beg_time;
  int      end_time;
  int      score;
  Move     best_move;
} SearchData;

extern SearchData data;

void perft(Position *pos, int depth);
void search(Position *pos, int depth);

#endif /* __SEARCH_H__ */
