/* See LICENSE file for file for copyright and license details */
#include "chesslib.h"
#include "bitboards.h"
#include "evaluate.h"
#include "position.h"

#define FLIP(square) ((square) ^ 56)

static U64 rank_mask[64];
static U64 file_mask[64];
static U64 adj_file_mask[64]; /* mask of adjaccent files */
static U64 passed_mask[2][64];

void
initialise_evaluation(void)
{
  Square sq; File f; Rank r;
  U64 mask;

  /* preprocess file and rank masks for each square */
  for (sq = SQ_A8; sq <= SQ_H1; sq++) {
    file_mask[sq] = 0ULL;
    rank_mask[sq] = 0ULL;
    for (r = sq >> 3, f = 0; f < 8; f++)
      set_bit(rank_mask[sq], 8 * r + f);
    for (f = sq & 7, r = 0; r < 8; r++)
      set_bit(file_mask[sq], 8 * r + f);
  }

  /* preprocess passed pawn masks */
  for (sq = SQ_A8; sq <= SQ_H1; sq++) {
    mask = 0ULL;
    /* 3 adjaccent files */
    mask |= shift(WEST, file_mask[sq]);
    mask |= file_mask[sq];
    mask |= shift(EAST, file_mask[sq]);

    /* for white pawns remove all bits above sq */
    passed_mask[WHITE][sq] = mask &  (get_bitboard(sq) - 1);
    /* we also need to remove bits on the same rank */
    passed_mask[WHITE][sq] &= ~rank_mask[sq];

    /* for black pawns remove all bits below sq */
    passed_mask[BLACK][sq] = mask & ~(get_bitboard(sq) - 1);
    /* we also need to remove bits on the same rank */
    passed_mask[BLACK][sq] &= ~rank_mask[sq];

    /* isolated pawns dont have any ally pawns on adjaccent files */
    adj_file_mask[sq] = shift(WEST, file_mask[sq]) | shift(EAST, file_mask[sq]);
  }
}

static const int pawn_pcsq[64][2] = {
  { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0},
  { 3,7}, { 0,5}, { 0,5}, { 0,5}, { 0,5}, { 0,5}, { 0,5}, { 3,7},
  { 2,6}, { 0,4}, { 0,4}, { 0,4}, { 0,4}, { 0,4}, { 0,4}, { 2,6},
  { 1,5}, { 0,3}, { 0,3}, { 0,3}, { 0,3}, { 0,3}, { 0,3}, { 1,5},
  { 1,4}, { 0,2}, { 5,2}, {20,2}, {20,2}, { 5,2}, { 0,2}, { 1,4},
  { 5,3}, {10,1}, { 0,1}, {10,1}, {10,1}, {-5,1}, {10,1}, { 5,3},
  {10,2}, {10,0}, { 9,0}, { 5,0}, { 5,0}, {10,0}, {10,0}, {10,2},
  { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0},
};

static const int passed[8][2] = {{0,0},{53,100},{31,45},{15,22},{8,14},{5,9},{2,5},{0,0}};
static const int isolated[2]  = { -10, -20 };
static const int doubled[2]   = { -10, -25 };

static const int knight_pcsq[64][2] = {
  {-15,-25}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, {-15,-25},
  {-10,-20}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, {-10,-20},
  {-10,-20}, { 0,0}, {10,0}, {10,0}, {10,0}, {10,0}, { 0,0}, {-10,-20},
  {-10,-20}, { 5,0}, {10,0}, {20,0}, {20,0}, {10,0}, { 5,0}, {-10,-20},
  {-10,-20}, { 5,0}, {10,0}, {20,0}, {20,0}, {10,0}, { 5,0}, {-10,-20},
  {-10,-20}, { 0,0}, {10,0}, { 5,0}, { 5,0}, {10,0}, { 0,0}, {-10,-20},
  {-10,-20}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, {-10,-20},
  {-15,-25}, {-6,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, {-8,0}, {-15,-25},
};

static const int bishop_pcsq[64][2] = {
  { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0}, { 0,0},
  { 0,0}, { 0,5}, { 0,5}, { 0,5}, { 0,5}, { 0,5}, { 0,5}, { 0,0},
  { 0,0}, { 0,5}, { 0,9}, { 0,9}, { 0,9}, { 0,9}, { 0,5}, { 0,0},
  { 0,0}, {18,5}, { 0,9}, { 0,9}, { 0,9}, { 0,9}, {18,5}, { 0,0},
  { 0,0}, { 0,5}, {20,9}, {20,9}, {20,9}, {20,9}, { 0,5}, { 0,0},
  { 5,0}, { 0,5}, { 7,9}, {10,9}, {10,9}, { 7,9}, { 0,5}, { 5,0},
  { 0,0}, {10,5}, { 0,5}, { 7,5}, { 7,5}, { 0,5}, {10,5}, { 0,0},
  { 0,0}, { 0,0}, {-6,0}, { 0,0}, { 0,0}, {-8,0}, { 0,0}, { 0,0},
};

static const int outpost[2] = { 15, 10 };
static const int bishop_pair[2] = { 20, 40 };

static const int rook_pcsq[64] = {
   5,  5,  7, 10, 10,  7,  5,  5,
  20, 20, 20, 20, 20, 20, 20, 20,
   0,  0,  5, 10, 10,  5,  0,  0,
   0,  0,  5, 10, 10,  5,  0,  0,
   0,  0,  5, 10, 10,  5,  0,  0,
   0,  0,  5, 10, 10,  5,  0,  0,
   0,  0,  5, 10, 10,  5,  0,  0,
};

static const int open_file[2] = { 10, 30 }; /* semiopen, open */
static const int king_file    = 10;

static const int king_pcsq[64][2] = {
  {-10,-50}, {-10,-20}, {-10,-20}, {-10,-20}, {-10,-20}, {-10,-20}, {-10,-20}, {-10,-50},
  {-10,-10}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,-10},
  {-10,-10}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,-10},
  {-10,-10}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,-10},
  {-10,-10}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,-10},
  {-10,-10}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,-10},
  {-10,-10}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,  0}, {-10,-10},
  { 10,-50}, { 20,-20}, { 20,-20}, {-10,-20}, {  0,-20}, {-10,-20}, { 20,-20}, { 20,-50},
};

static const int king_shield[2] = { 2, 5 };

#include <stdio.h>
int
evaluate(const Position *pos)
{
  int value       = pos->material[WHITE] - pos->material[BLACK];
  int endgame     = pos->material[WHITE] + pos->material[BLACK] <= 3000;
  U64 white_pawns = pos->color[WHITE] & pos->piece[PAWN];
  U64 black_pawns = pos->color[BLACK] & pos->piece[PAWN];
  U64 occupancy   = pos->color[WHITE] | pos->color[BLACK];
  U64 mask;
  Square sq, fsq;

  /* PAWNS */
  mask = white_pawns;
  while (mask) {
    sq = pop_lsb(&mask);
    value += pawn_pcsq[sq][endgame];

    if (!(passed_mask[WHITE][sq] & black_pawns))
      value += passed[sq >> 3][endgame];

    if (!(adj_file_mask[sq] & white_pawns))
      value += isolated[endgame];

    if ((get_bitboard(sq) - 1) & white_pawns)
      value += doubled[endgame];
  }

  mask = black_pawns;
  while (mask) {
    sq = pop_lsb(&mask);
    fsq = FLIP(sq);
    value -= pawn_pcsq[fsq][endgame];

    if (!(passed_mask[BLACK][sq] & white_pawns))
      value -= passed[fsq >> 3][endgame];

    if (!(adj_file_mask[sq] & black_pawns))
      value -= isolated[endgame];

    if ((get_bitboard(sq) - 1) & black_pawns)
      value -= doubled[endgame];
  }

  /* KNGHTS */
  mask = pos->piece[KNIGHT] & pos->color[WHITE];
  while (mask) {
    sq = pop_lsb(&mask);
    value += knight_pcsq[sq][endgame];

    if (!((get_bitboard(sq) - 1) & adj_file_mask[sq] & black_pawns))
      value += outpost[endgame];

    /* mobility */
    value += popcount(attacks_bb(KNIGHT, sq, 0) & ~pos->color[WHITE]);
  }

  mask = pos->piece[KNIGHT] & pos->color[BLACK];
  while (mask) {
    sq = pop_lsb(&mask);
    fsq = FLIP(sq);
    value -= knight_pcsq[fsq][endgame];

    if (!(~(get_bitboard(sq) - 1) & adj_file_mask[sq] & white_pawns))
      value -= outpost[endgame];

    /* mobility */
    value -= popcount(attacks_bb(KNIGHT, sq, 0) & ~pos->color[BLACK]);
  }

  /* BISHOPS */
  mask = pos->piece[BISHOP] & pos->color[WHITE];

  if (mask & (mask - 1))
    value += bishop_pair[endgame];

  while (mask) {
    sq = pop_lsb(&mask);
    value += bishop_pcsq[sq][endgame];

    if (!((get_bitboard(sq) - 1) & adj_file_mask[sq] & black_pawns))
      value += outpost[endgame];

    value += popcount(attacks_bb(BISHOP, sq, occupancy) & ~pos->color[WHITE]);
  }

  mask = pos->piece[BISHOP] & pos->color[BLACK];

  if (mask & (mask - 1))
    value -= bishop_pair[endgame];

  while (mask) {
    sq = pop_lsb(&mask);
    fsq = FLIP(sq);
    value -= bishop_pcsq[fsq][endgame];

    if (!(~(get_bitboard(sq) - 1) & adj_file_mask[sq] & white_pawns))
      value -= outpost[endgame];

    value -= popcount(attacks_bb(BISHOP, sq, occupancy) & ~pos->color[BLACK]);
  }

  /* ROOKS */
  mask = pos->piece[ROOK] & pos->color[WHITE];
  while (mask) {
    sq = pop_lsb(&mask);
    value += rook_pcsq[sq];

    if (!(file_mask[sq] & pos->piece[PAWN]))
      value += open_file[1];
    else if (!(file_mask[sq] & white_pawns))
      value += open_file[0];
  
    if ((sq & 7) == (pos->ksq[BLACK] & 7) || (sq >> 3) == (pos->ksq[BLACK] >> 3))
      value += king_file;
  }

  mask = pos->piece[ROOK] & pos->color[BLACK];
  while (mask) {
    sq = pop_lsb(&mask);
    fsq = FLIP(sq);
    value -= rook_pcsq[fsq];

    if (!(file_mask[sq] & pos->piece[PAWN]))
      value -= open_file[1];
    else if (!(file_mask[sq] & black_pawns))
      value -= open_file[0];
  
    if ((sq & 7) == (pos->ksq[WHITE] & 7) || (sq >> 3) == (pos->ksq[WHITE] >> 3))
      value -= king_file;
  }

  /* KINGS */
  value += king_pcsq[pos->ksq[WHITE]][endgame] - king_pcsq[FLIP(pos->ksq[BLACK])][endgame];
  value += king_shield[endgame] * 
           (popcount(attacks_bb(KING, pos->ksq[WHITE], 0) & white_pawns) - 
            popcount(attacks_bb(KING, pos->ksq[BLACK], 0) & black_pawns));

  return pos->turn == WHITE ? value : -value;
}
