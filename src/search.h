/* See LICENSE file for file for copyright and license details */
#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "chesslib.h"
#include "position.h"

typedef struct {
  int  cnt;
  Move m[MAX_PLY];
} PV;

typedef struct {
  int beg_time;
  int end_time;

  int depth;
  int depthset;
  int timeset;

  int movestogo;
  int infinite;

  int quit;    /* flag for quitting program */
  int stopped; /* flag for stopping search */

  uint64_t nodes; /* nodes visited during search */
} SearchInfo;

extern SearchInfo info;

void perft(Position *pos);
void search(Position *pos);

#endif /* __SEARCH_H__ */
