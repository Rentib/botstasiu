/* See LICENSE file for file for copyright and license details */
#ifndef __MOVEORDER_H__
#define __MOVEORDER_H__

#include "chesslib.h"
#include "position.h"

/* Excludes nonlegal moves,
   gives moves their values,
   returns pointer to last element of move_list. */
Move *process_moves(Position *pos, Move *move_list, Move *last);

/* Sorts moves by their values. */
void sort_moves(Move *begin, Move *end);

#endif /* __MOVEORDER_H__ */
