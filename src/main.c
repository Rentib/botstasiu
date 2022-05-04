/* See LICENSE file for file for copyright and license details */
#include <stdio.h>

#include "bitboards.h"
#include "chesslib.h"
#include "position.h"
#include "uci.h"

/* commonly used bitboards */
const U64 FileABB = 0x0101010101010101ULL;
const U64 FileBBB = FileABB << 1;
const U64 FileCBB = FileABB << 2;
const U64 FileDBB = FileABB << 3;
const U64 FileEBB = FileABB << 4;
const U64 FileFBB = FileABB << 5;
const U64 FileGBB = FileABB << 6;
const U64 FileHBB = FileABB << 7;

const U64 Rank1BB = 0xFF00000000000000ULL;
const U64 Rank2BB = Rank1BB >> 8;
const U64 Rank3BB = Rank2BB >> 8;
const U64 Rank4BB = Rank3BB >> 8;
const U64 Rank5BB = Rank4BB >> 8;
const U64 Rank6BB = Rank5BB >> 8;
const U64 Rank7BB = Rank6BB >> 8;
const U64 Rank8BB = Rank7BB >> 8;

int
main(/* int argc, char *argv[] */)
{
  printf("Botstasiu alpha by Stanisław Bitner\n");

  initialise_bitboards();
  initialise_zobrist_keys();

  uci_loop();

  delete_bitboards();
  return 0;
}
