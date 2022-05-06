#ifndef __TT_H__
#define __TT_H__

#include <inttypes.h>

#include "chesslib.h"

typedef uint64_t Key;

typedef struct TT TT;

TT *tt_new(int size);
void tt_delete(TT *tt);
void tt_clear(TT *tt);

void tt_store(TT *tt, const Key key, const Move m);
Move tt_probe(TT *tt, const Key key);

#endif /* __TT_H__ */
