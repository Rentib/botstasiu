/* See LICENSE file for file for copyright and license details */
#ifndef __MOVEGEN_H__
#define __MOVEGEN_H__

#include "bitboards.h"
#include "chesslib.h"
#include "position.h"

typedef enum {
  ALL, 
  QUIET,
  CAPTURES,
} GenType;

/* Generates moves and returns pointer to last elemnt of move_list array. */
Move *generate_moves(GenType gt, Move *move_list, Position *pos);

#endif /* __MOVEGEN_H__ */
