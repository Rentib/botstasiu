#ifndef __TT_H__
#define __TT_H__

#include <inttypes.h>

typedef uint64_t Key;

typedef struct TT TT;

TT *tt_new(int size);
void tt_delete(TT *tt);
void tt_clear(TT *tt);

#endif /* __TT_H__ */
