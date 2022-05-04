/* See LICENSE file for file for copyright and license details */
#include <inttypes.h>
#include <stdio.h>

#include "bitboards.h"
#include "chesslib.h"

U64
shift(Direction D, U64 b)
{
  switch (D) {
  case NORTH       : return b >> 8;
  case NORTH_NORTH : return b >> 16;
  case SOUTH       : return b << 8;
  case SOUTH_SOUTH : return b << 16;
  case WEST        : return (b & ~FileABB) >> 1;
  case EAST        : return (b & ~FileHBB) << 1;
  case NORTH_WEST  : return (b & ~FileABB) >> 9;
  case NORTH_EAST  : return (b & ~FileHBB) >> 7;
  case SOUTH_WEST  : return (b & ~FileABB) << 7;
  case SOUTH_EAST  : return (b & ~FileHBB) << 9;
  default: return 0;
  }
}

void
pretty(U64 bitboard)
{
  const char *sep = "  +---+---+---+---+---+---+---+---+";
  printf("%s\n", sep);
  for (Rank r = 8; r >= 1; r--) {
    printf("%d ", r);
    for (File f = 1; f <= 8; f++) {
      char c = get_bit(bitboard, (8 - r) * 8 + f - 1) ? 'X' : ' ';
      printf("|" "\x1B[91m" " %c " "\x1B[0m", c);
    }
    printf("|\n%s\n", sep);
  }
	printf("    a   b   c   d   e   f   g   h\n\n");
  printf("    Bitboard: %lu\n", bitboard);
}
