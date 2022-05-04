/* See LICENSE file for file for copyright and license details */
#include "bitboards.h"
#include "chesslib.h"
#include "movegen.h"
#include "position.h"

static inline Move *
make_promotions(Direction dir,
                Move *move_list, Square to) {
  *move_list++ = make_promotion(to - dir, to,  QUEEN);
  *move_list++ = make_promotion(to - dir, to, KNIGHT);
  *move_list++ = make_promotion(to - dir, to, BISHOP);
  *move_list++ = make_promotion(to - dir, to,   ROOK);
  return move_list;
}

static Move *
generate_pawn_moves(GenType gt, 
                    Move *move_list, Position *pos, U64 target)
{
  const Color us = pos->turn, them = !us;
  const Direction up  = us == WHITE ? NORTH : SOUTH;
  const Direction upl = us == WHITE ? NORTH_WEST : SOUTH_WEST;
  const Direction upr = us == WHITE ? NORTH_EAST : SOUTH_EAST;
  const U64 rank4     = us == WHITE ? Rank4BB : Rank5BB;
  const U64 rank7     = us == WHITE ? Rank7BB : Rank2BB;
  const U64 empty   = target & pos->empty;       /* empty squares that are targets */
  const U64 enemies = target & pos->color[them]; /* enemies that are targets */
  U64 b1, b2; /* helper variables for pawn masks */
  Square to; /* square where pawn moves to */

  U64 pawns = pos->piece[PAWN] & pos->color[us] & ~rank7; /* mask of all pawns */
  U64 promo = pos->piece[PAWN] & pos->color[us] &  rank7; /* promotion pawns */

  /* quiet moves - single and double pawn pushes */
  if (gt != CAPTURES) {
    b1 = shift(up, pawns) & pos->empty;    /* single push */
    b2 = shift(up,    b1) & empty & rank4; /* double push */
    b1 &= empty; /* necessary for blocking checks with double push */
    while (b1) {
      to = pop_lsb(&b1);
      *move_list++ = make_move(to - up, to);
    }
    while (b2) {
      to = pop_lsb(&b2);
      *move_list++ = make_move(to - up - up, to);
    }
  }

  /* captures - normal and en_passant */
  if (gt != QUIET) {
    b1 = shift(upl, pawns) & enemies; /* attacks to west */
    b2 = shift(upr, pawns) & enemies; /* attacks to east */
    while (b1) {
      to = pop_lsb(&b1);
      *move_list++ = make_move(to - upl, to);
    }
    while (b2) {
      to = pop_lsb(&b2);
      *move_list++ = make_move(to - upr, to);
    }

    if (pos->en_passant != SQ_NONE)
      for (b1 = pawns & pawn_attacks_bb(them, pos->en_passant); b1; )
        *move_list++ = make_en_passant(pop_lsb(&b1), pos->en_passant);
  }

  /* promotion moves - quiet and captures */
  if (promo) {
    if (gt != QUIET) {
      b1 = shift(upl, promo) & enemies;
      b2 = shift(upr, promo) & enemies;
      while (b1) move_list = make_promotions(upl, move_list, pop_lsb(&b1));
      while (b2) move_list = make_promotions(upr, move_list, pop_lsb(&b2));
    }
    if (gt != CAPTURES) {
      b1 = shift(up, promo) & empty;
      while (b1) move_list = make_promotions(up, move_list, pop_lsb(&b1));
    }
  }

  return move_list;
}

static Move *
generate_piece_moves(PieceType pt,
                      Move *move_list, Position *pos, U64 target)
{
  U64 pieces = pos->piece[pt] & pos->color[pos->turn];
  U64 attacks;
  Square from;
  while (pieces) {
    from = pop_lsb(&pieces);
    attacks = attacks_bb(pt, from, ~pos->empty) & target;
    while (attacks)
      *move_list++ = make_move(from, pop_lsb(&attacks));
  }
  return move_list;
}

Move *
generate_moves(GenType gt,
               Move *move_list, Position *pos)
{
  return move_list;
}
