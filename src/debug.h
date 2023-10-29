#ifndef __SKOOMA_DEBUG_H__
#define __SKOOMA_DEBUG_H__

#include "chunk.h"
#include "vmachine.h"

void Chunk_debug(Chunk* chunk, const char* name);
UInt Instruction_debug(Chunk* chunk, UInt offset);

#ifdef DEBUG_ASSERTIONS
    #include <stdio.h>
    #include <stdlib.h>
    #define ASSERT(expr, message)                                                        \
        do {                                                                             \
            if(!(expr)) {                                                                \
                fprintf(                                                                 \
                    stderr,                                                              \
                    "Assertion failed at %d:%s\n\t'#expr'\n\t%s",                        \
                    __LINE__,                                                            \
                    __FILE__,                                                            \
                    message);                                                            \
                abort();                                                                 \
            }                                                                            \
        } while(false)
#else
    #define ASSERT(expr, message)
#endif

#endif
