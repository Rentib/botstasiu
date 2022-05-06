/* See LICENSE file for file for copyright and license details */
#include <stdlib.h>
#include <sys/time.h>

#include "misc.h"

static uint64_t state = 0xfb195a46da7c765aULL; /* seed for getting pseudo random numbers */
static uint32_t rand_uint32(void);

int 
get_time(void)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000 + t.tv_usec / 1000;
}

uint64_t
magic_number_candidate(void)
{
  return rand_uint64() & rand_uint64() & rand_uint64();
}

static uint32_t
rand_uint32(void)
{
  uint32_t x = state;
  x ^= x >> 13;
  x ^= x << 17;
  x ^= x >> 5;
  return state = x;
}

uint64_t
rand_uint64(void)
{
  return (uint64_t)(rand_uint32() & 0xFFFF) << 0
       | (uint64_t)(rand_uint32() & 0xFFFF) << 16
       | (uint64_t)(rand_uint32() & 0xFFFF) << 32
       | (uint64_t)(rand_uint32() & 0xFFFF) << 48;
}
