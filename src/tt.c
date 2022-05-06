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
  if (tt) {
    free(tt->entries);
    free(tt);
  }
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

void
tt_store(TT *tt, const Key key, const Move m)
{
  int index = key & (tt->num - 1);
  tt->entries[index] = (Entry){ .key = key, .best = m };
}

Move
tt_probe(TT *tt, const Key key)
{
  int index = key & (tt->num - 1);
  if (key == tt->entries[index].key)
    return tt->entries[index].best;
  return MOVE_NONE;
}
