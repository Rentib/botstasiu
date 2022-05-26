/* See LICENSE file for file for copyright and license details */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "bitboards.h"
#include "chesslib.h"
#include "misc.h"

/* Fancy magics */
typedef struct {
  U64       mask;    /* mask of relevant bits */
  U64       magic;   /* magic number */
  U64      *attacks; /* magic attacks */
  unsigned  shift;   /* number of relevant bits */
} Magic;

/* attack lookup tables */
static U64 pawn_attacks[2][64]; /* [color][square] */
static U64 knight_attacks[64];  /* [square] */
static U64 king_attacks[64];    /* [square] */
static U64 between[64][64];

static Magic RookMagics[64];
static Magic BishopMagics[64];

static void find_magic(PieceType pt, Square sq);
static void mask_pawn_attacks(Square sq);
static void mask_knight_attacks(Square sq);
static void mask_king_attacks(Square sq);
static void mask_rook_relevant_squares(Square sq);
static void mask_bishop_relevant_squares(Square sq);
static U64 rook_attacks_bb(Square sq, U64 occ);
static U64 bishop_attacks_bb(Square sq, U64 occ);
static U64 generate_rook_attacks_slow(Square sq, U64 occ);
static U64 generate_bishop_attacks_slow(Square sq, U64 occ);
static U64 get_mask_state(U64 mask, int idx);
static U64 slide(Direction dir, Square sq, U64 occ);

void
initialise_bitboards(void)
{
  for (Square sq = SQ_A8; sq <= SQ_H1; sq++) {
    mask_pawn_attacks(sq);
    mask_knight_attacks(sq);
    mask_king_attacks(sq);
    mask_bishop_relevant_squares(sq);
    mask_rook_relevant_squares(sq);
    find_magic(  ROOK, sq);
    find_magic(BISHOP, sq);
  }

  for (Square sq1 = SQ_A8; sq1 <= SQ_H1; sq1++) {
    /* calculate between lookup table */
    for (Square sq2 = SQ_A8; sq2 <= SQ_H1; sq2++) {
      if (get_bit(attacks_bb(BISHOP, sq1, 0ULL), sq2)) {
	  			between[sq1][sq2] = attacks_bb(BISHOP, sq1, get_bitboard(sq2)) 
                            & attacks_bb(BISHOP, sq2, get_bitboard(sq1));
      }
      if (get_bit(attacks_bb(ROOK, sq1, 0ULL), sq2)) {
	  			between[sq1][sq2] = attacks_bb(ROOK, sq1, get_bitboard(sq2)) 
                            & attacks_bb(ROOK, sq2, get_bitboard(sq1));
      }
      between[sq1][sq2] |= get_bitboard(sq1) | get_bitboard(sq2);
    }
  }
}

void
delete_bitboards(void)
{
  for (Square sq = 0; sq < 64; sq++) {
    free(  RookMagics[sq].attacks);
    free(BishopMagics[sq].attacks);
  }
}

U64
attacks_bb(PieceType pt, Square sq, U64 occ)
{
  switch (pt) {
  case ROOK  : return rook_attacks_bb(sq, occ);
  case KNIGHT: return knight_attacks[sq];
  case BISHOP: return bishop_attacks_bb(sq, occ);
  case QUEEN : return rook_attacks_bb(sq, occ) | bishop_attacks_bb(sq, occ);
  case KING  : return king_attacks[sq];
  default    : return 0ULL;
  }
}

U64
pawn_attacks_bb(Color c, Square sq)
{
  return pawn_attacks[c][sq];
}

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

static void
find_magic(PieceType pt, Square sq)
{
  Magic *m = pt == ROOK ? &RookMagics[sq] : &BishopMagics[sq];
  unsigned size = 1 << m->shift, idx, i, cnt, checked[size];
  U64 occupancy[size]; /* relevant occupancy bitboards */
  U64 attacks[size];   /* brutally generated attack bitboards for occupancies */
  m->attacks = calloc(size, sizeof(U64));
  for (i = 0; i < size; i++) {
    checked[i] = 0;
    occupancy[i] = get_mask_state(m->mask, i);
    attacks[i] = pt == ROOK ?   generate_rook_attacks_slow(sq, occupancy[i])
                            : generate_bishop_attacks_slow(sq, occupancy[i]);
  }

  for (i = 0, cnt = 1; i < size; cnt++) {
    for (m->magic = 0ULL; popcount((m->magic * m->mask) >> 56) < 6; )
      m->magic = magic_number_candidate();
    for (i = 0; i < size; i++) {
      idx = (occupancy[i] * m->magic) >> (64 - m->shift);
      if (checked[idx] < cnt) {
        checked[idx] = cnt;
        m->attacks[idx] = attacks[i];
      }
      else if (m->attacks[idx] != attacks[i])
        break;
    }
  }
}

static void 
mask_pawn_attacks(Square sq) 
{
  /* pawns dont exist on 1st and 8th ranks */
  U64 mask = get_bitboard(sq);
  U64 attacksBB = shift(WEST, mask) | shift(EAST, mask);
  pawn_attacks[WHITE][sq] = shift(NORTH, attacksBB);
  pawn_attacks[BLACK][sq] = shift(SOUTH, attacksBB);
}

static void 
mask_knight_attacks(Square sq) 
{
  U64 mask = get_bitboard(sq);
  U64 b1; // mask storing single west and east shifts
  U64 b2; // mask storing double west and east shifts
  b1 = shift(WEST, mask) | shift(EAST, mask);
  b2 = shift(WEST, shift(WEST, mask)) | shift(EAST, shift(EAST, mask));
  knight_attacks[sq] = b1 << 16 | b1 >> 16 | b2 << 8 | b2 >> 8;
}

static void 
mask_king_attacks(Square sq) 
{
  U64 mask = get_bitboard(sq);
  U64 b; // mask storing west and east shifts
  b = shift(WEST, mask) | shift(EAST, mask);
  king_attacks[sq] = b | shift(NORTH, mask | b) | shift(SOUTH, mask | b);
}

static void
mask_bishop_relevant_squares(Square sq)
{
  U64 res = 0ULL;
  res |= slide(NORTH_EAST, sq, 0ULL) & ~(Rank8BB | FileHBB);
  res |= slide(SOUTH_EAST, sq, 0ULL) & ~(Rank1BB | FileHBB);
  res |= slide(SOUTH_WEST, sq, 0ULL) & ~(Rank1BB | FileABB);
  res |= slide(NORTH_WEST, sq, 0ULL) & ~(Rank8BB | FileABB);
  BishopMagics[sq].mask = res;
  BishopMagics[sq].shift = popcount(res);
}

static void
mask_rook_relevant_squares(Square sq)
{
  U64 res = 0ULL;
  res |= slide(NORTH, sq, 0ULL) & ~Rank8BB;
  res |= slide( EAST, sq, 0ULL) & ~FileHBB;
  res |= slide(SOUTH, sq, 0ULL) & ~Rank1BB;
  res |= slide( WEST, sq, 0ULL) & ~FileABB;
  RookMagics[sq].mask = res;
  RookMagics[sq].shift = popcount(res);
}

static U64
rook_attacks_bb(Square sq, U64 occ)
{
  occ &= RookMagics[sq].mask;
  occ *= RookMagics[sq].magic;
  occ >>= (64 - RookMagics[sq].shift);
  return RookMagics[sq].attacks[occ];
}

static U64
bishop_attacks_bb(Square sq, U64 occ)
{
  occ &= BishopMagics[sq].mask;
  occ *= BishopMagics[sq].magic;
  occ >>= (64 - BishopMagics[sq].shift);
  return BishopMagics[sq].attacks[occ];
}

static U64
generate_rook_attacks_slow(Square sq, U64 occ)
{
  return slide(NORTH, sq, occ)
       | slide( EAST, sq, occ)
       | slide(SOUTH, sq, occ)
       | slide( WEST, sq, occ);
}

static U64 
generate_bishop_attacks_slow(Square sq, U64 occ)
{
  return slide(NORTH_EAST, sq, occ)
       | slide(SOUTH_EAST, sq, occ)
       | slide(SOUTH_WEST, sq, occ)
       | slide(NORTH_WEST, sq, occ);
}

static U64
get_mask_state(U64 mask, int idx)
{
  U64 res = 0ULL;
  Square sq;
  for (int bit = 0; mask; bit++) {
    sq = pop_lsb(&mask);
    if ((1 << bit) & idx)
      set_bit(res, sq);
  }
  return res;
}

static U64
slide(Direction dir, Square sq, U64 occupancy)
{
  U64 res = 0ULL, mask = get_bitboard(sq);
  while ((mask = shift(dir, mask)) && !(mask & occupancy))
    res |= mask;
  return res | mask;
}

U64
between_bb(Square sq1, Square sq2)
{
  return between[sq1][sq2];
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
