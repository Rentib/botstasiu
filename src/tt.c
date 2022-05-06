#include <stdlib.h>

#include "chesslib.h"
#include "tt.h"

typedef struct {
  Key  key;
  Move best;
} Entry;

struct TT {
  Entry *entries;
  int    num; /* number of entries */
};

TT *
tt_new(int size)
{
  TT *tt;
  if (!(tt = malloc(sizeof(TT))))
    return NULL;

  size /= sizeof(TT);
  tt->num = 1;
  while (tt->num < size)
    tt->num <<= 1;

  if (!(tt->entries = malloc(sizeof(Entry) * tt->num))) {
    free(tt);
    return NULL;
  }
  
  tt_clear(tt);
  return tt;
}

void
tt_delete(TT *tt)
{
  free(tt->entries);
  free(tt);
}

void
tt_clear(TT *tt)
{
  Entry *et;
  for (et = tt->entries; et < tt->entries + tt->num; et++) {
    et->key  = 0ULL;
    et->best = MOVE_NONE;
  }
}
