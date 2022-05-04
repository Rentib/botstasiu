/* See LICENSE file for file for copyright and license details */
#ifndef __MISC_H__
#define __MISC_H__

#include <inttypes.h>

/* Returns time in milliseconds  */
int get_time(void);

uint64_t rand_uint64(void);

/* Returns a bitboard with low amount of 1 bits. */
uint64_t magic_number_candidate(void);

#endif /* __MISC_H__ */
