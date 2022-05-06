/* See LICENSE file for file for copyright and license details */
#ifndef __BITBOARD_H__
#define __BITBOARD_H__

#include "chesslib.h"

#define get_bit(bitboard, square) (bitboard & (1ULL << (square)))
#define set_bit(bitboard, square) (bitboard |= (1ULL << (square)))
#define pop_bit(bitboard, square) (bitboard &= ~(1ULL << (square)))
#define get_bitboard(square)      (1ULL << (square))
#define get_square(bitboard)      (Square)(__builtin_ctzll(bitboard))
#define popcount(x)               (__builtin_popcountll(x))

/* commonly used bitboards */
extern const U64 FileABB;
extern const U64 FileBBB;
extern const U64 FileCBB;
extern const U64 FileDBB;
extern const U64 FileEBB;
extern const U64 FileFBB;
extern const U64 FileGBB;
extern const U64 FileHBB;

extern const U64 Rank1BB;
extern const U64 Rank2BB;
extern const U64 Rank3BB;
extern const U64 Rank4BB;
extern const U64 Rank5BB;
extern const U64 Rank6BB;
extern const U64 Rank7BB;
extern const U64 Rank8BB;

/* Creates lookup tables for pawns and leaping pieces.
   Creates magic bitboards for sliding pieces. */
void initialise_bitboards(void);

/* Frees memory allocated for magic bitboards. */
void delete_bitboards(void);

/* Returns bitboard of possible moves for a piece.
   pt  - type of checked piece
   sq  - square on which the piece stands
   occ - occupancy bitboard */
U64 attacks_bb(PieceType pt, Square sq, U64 occ);

/* Returns bitboard of captures for a pawn.
  c  - color of checked pawn
  sq - square on which the pawn stands */
U64 pawn_attacks_bb(Color c, Square sq);

/* Returns a bitboard shifted in the given direction. */
U64 shift(Direction dir, U64 mask);

/* Prints pretty image of a bitboard. */
void pretty(U64 mask);

/* Returns a bitmask with least significant 1bit. */
inline U64 
lsb(const U64 mask)
{
  return mask & -mask;
}

/* Pops least significant 1bit from a bitboard and returns its bitboard. */
inline Square
pop_lsb(U64 *mask)
{
  U64 x = lsb(*mask);
  *mask ^= x;
  return get_square(x);
}

/* Returns mask of squares in between sq1 and sq2. */
U64 between_bb(Square sq1, Square sq2);

#endif /* __BITBOARD_H__ */
