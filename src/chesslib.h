/* See LICENSE file for file for copyright and license details */
#ifndef __CHESSLIB_H__
#define __CHESSLIB_H__

#include <inttypes.h>

#define MAX_GAME_PLY 1024
#define MAX_PLY      64
#define INFINITY     50000
#define MATE_VALUE   49000

typedef uint64_t U64;

typedef int File;
typedef int Rank;

typedef enum {
  SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
  SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
  SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
  SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
  SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
  SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
  SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
  SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
  SQ_NONE,
} Square;

typedef enum {
  WHITE, 
  BLACK,
} Color;

typedef enum {
  PAWN, 
  KNIGHT, 
  BISHOP, 
  ROOK, 
  QUEEN, 
  KING, 
  NONE,
} PieceType;

inline char
piece_to_char(PieceType pt) {
  switch (pt) {
  case PAWN  : return 'P';
  case KNIGHT: return 'N';
  case BISHOP: return 'B';
  case ROOK  : return 'R';
  case QUEEN : return 'Q';
  case KING  : return 'K';
  default    : return ' ';
  }
}

typedef enum {
  NORTH       = -8,
  NORTH_NORTH = -16,
  EAST        = +1,
  SOUTH       = +8,
  SOUTH_SOUTH = +16,
  WEST        = -1,
  NORTH_EAST  = -7,
  SOUTH_EAST  = +9,
  SOUTH_WEST  = +7,
  NORTH_WEST  = -9,
} Direction;

typedef enum {
  MOVE_NONE,
  MOVE_NULL = 65,
} Move;

/*
 * 00000000000000|00|00|000000|000000
 *   move score  |pt|mt| from |  to  
 * from - from square
 * to   -   to square
 * mt   - move type
 * pt   - promotion piece type
 */
typedef enum {
  NORMAL     = 0 << 12,
  PROMOTION  = 1 << 12,
  CASTLE     = 2 << 12,
  EN_PASSANT = 3 << 12,
} MoveType;

inline Square
to_sq(Move m)
{
  return (Square)(m & 0x3F);
}

inline Square
from_sq(Move m)
{
  return (Square)((m >> 6) & 0x3F);
}

inline MoveType
type_of(Move m)
{
  return (MoveType)(m & (3 << 12));
}

inline PieceType
promotion_type(Move m)
{
  return (PieceType)(((m >> 14) & 3) + KNIGHT);
}

inline Move
make_move(Square from, Square to)
{
  return (Move)((from << 6) + to);
}

inline Move
make_promotion(Square from, Square to, PieceType pt)
{
  return (Move)(((pt - KNIGHT) << 14) + PROMOTION + (from << 6) + to);
}

inline Move
make_en_passant(Square from, Square to)
{
  return (Move)(EN_PASSANT + (from << 6) + to);
}

/* king movement is enough to make castle move */
inline Move
make_castle(Square from, Square to)
{
  return (Move)(CASTLE + (from << 6) + to);
}

#endif /* __CHESSLIB_H__ */
