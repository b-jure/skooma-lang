#include "debug.h"
#include "hashtable.h"
#include "mem.h"
#include "object.h"
#include "parser.h"
#include "skconf.h"
#include "value.h"
#include "vmachine.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>




void omark(VM* vm, O* obj)
{
    if(obj == NULL || oismarked(obj)) return;
    osetmark(obj, true);
    if(otype(obj) == OBJ_STRING) {
#ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*)obj);
        vprint(OBJ_VAL(obj));
        printf("\n");
#endif
        return;
    }
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)obj);
    vprint(OBJ_VAL(obj));
    printf("\n");
#endif
    GSARRAY_PUSH(vm, obj);
}


sstatic force_inline void marktable(VM* vm, HashTable* table)
{
    for(UInt i = 0; i < table->cap; i++) {
        Entry* entry = &table->entries[i];
        if(!IS_EMPTY(entry->key)) {
            vmark(vm, entry->key);
            vmark(vm, entry->value);
        }
    }
}





// Generic function signature for Mark and Sweep functions
#define MS_FN(name) sstatic force_inline void name(VM* vm)

MS_FN(markglobals)
{
    for(UInt i = 0; i < vm->globids.cap; i++) {
        Entry* entry = &vm->globids.entries[i];
        if(!IS_EMPTY(entry->key)) {
            // Mark identifier (ObjString)
            omark(vm, AS_OBJ(entry->key));
            // Mark value
            UInt idx = (UInt)AS_NUMBER(entry->value);
            vmark(vm, vm->globvals[idx].value);
        }
    }
}

MS_FN(markstack)
{
    for(Value* local = vm->stack; local < vm->sp; local++)
        vmark(vm, *local);
}

MS_FN(markframes)
{
    for(Int i = 0; i < vm->fc; i++)
        omark(vm, (O*)vm->frames[i].closure);
}

MS_FN(markupvalues)
{
    for(OUpvalue* upval = vm->open_upvals; upval != NULL; upval = upval->next)
        omark(vm, (O*)upval);
}

MS_FN(markstatics)
{
    for(UInt i = 0; i < SS_SIZE; i++)
        omark(vm, (O*)vm->statics[i]);
}

MS_FN(markloaded)
{
    marktable(vm, &vm->loaded);
}

MS_FN(marktemp)
{
    for(UInt i = 0; i < vm->temp.len; i++)
        vmark(vm, vm->temp.data[i]);
}

MS_FN(markroots)
{
    markstack(vm);
    markframes(vm);
    markupvalues(vm);
    markglobals(vm);
    markstatics(vm);
    markloaded(vm);
    vmark(vm, vm->script);
}

MS_FN(rmweakrefs)
{
    for(UInt i = 0; i < vm->strings.cap; i++) {
        Entry* entry = &vm->strings.entries[i];
        if(IS_OBJ(entry->key) && !oismarked(AS_OBJ(entry->key)))
            HashTable_remove(&vm->strings, entry->key);
    }
}

MS_FN(sweep)
{
    O* previous = NULL;
    O* current  = vm->objects;
    while(current != NULL) {
        if(oismarked(current)) {
            osetmark(current, false);
            previous = current;
            current  = onext(current);
        } else {
            O* unreached = current;
            current      = onext(current);
            if(previous != NULL) osetnext(previous, current);
            else vm->objects = current;
            ofree(vm, unreached);
        }
    }
}






void mark_black(VM* vm, O* obj)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)obj);
    vprint(OBJ_VAL(obj));
    printf("\n");
#endif
#ifdef S_PRECOMPUTED_GOTO
    #define OBJ_TABLE
    #include "jmptable.h"
    #undef OBJ_TABLE
#else
    #define DISPATCH(x) switch(x)
    #define CASE(label) case label:
    #define BREAK       break
#endif
    ASSERT(oismarked(obj), "Object is not marked.");
    DISPATCH(otype(obj))
    {
        CASE(OBJ_UPVAL)
        {
            vmark(vm, ((OUpvalue*)obj)->closed.value);
            BREAK;
        }
        CASE(OBJ_FUNCTION)
        {
            OFunction* fn = (OFunction*)obj;
            omark(vm, (O*)fn->name);
            for(UInt i = 0; i < fn->chunk.constants.len; i++)
                vmark(vm, fn->chunk.constants.data[i]);
            BREAK;
        }
        CASE(OBJ_CLOSURE)
        {
            OClosure* closure = (OClosure*)obj;
            omark(vm, (O*)closure->fn);
            for(UInt i = 0; i < closure->upvalc; i++)
                omark(vm, (O*)closure->upvals[i]);
            BREAK;
        }
        CASE(OBJ_CLASS)
        {
            OClass* oclass = (OClass*)obj;
            omark(vm, (O*)oclass->name);
            marktable(vm, &oclass->methods);
            omark(vm, (O*)oclass->overloaded);
            BREAK;
        }
        CASE(OBJ_INSTANCE)
        {
            OInstance* instance = (OInstance*)obj;
            omark(vm, (O*)instance->oclass);
            marktable(vm, &instance->fields);
            BREAK;
        }
        CASE(OBJ_BOUND_METHOD)
        {
            OBoundMethod* bound_method = (OBoundMethod*)obj;
            omark(vm, (O*)bound_method);
            vmark(vm, bound_method->receiver);
            omark(vm, (O*)bound_method->method);
            BREAK;
        }
        CASE(OBJ_NATIVE)
        {
            ONative* native = (ONative*)obj;
            omark(vm, (O*)native->name);
            BREAK;
        }
        CASE(OBJ_STRING)
        unreachable;
    }
}

size_t gc(VM* vm)
{
#ifdef DEBUG_LOG_GC
    printf("--> GC start\n");
#endif
    size_t old_allocation = vm->gc_allocated;
    markroots(vm);
#ifdef S_PRECOMPUTED_GOTO
    static const void* jmptable[] = {&&mark, &&skip};
    goto*              jmptable[runtime];
mark:
    mark_function_roots(vm);
skip:
#else
    mark_function_roots(vm);
#endif
    while(vm->gslen > 0)
        mark_black(vm, GSARRAY_POP(vm));
    rmweakrefs(vm);
    sweep(vm);
    vm->gc_next =
        MAX((double)vm->gc_allocated * vm->config.gc_grow_factor,
            vm->config.gc_min_heap_size);
#ifdef DEBUG_LOG_GC
    printf("--> GC end\n");
    printf(
        "    collected %lu bytes (from %lu to %lu) next collection at %lu\n",
        old_allocation - vm->gc_allocated,
        old_allocation,
        vm->gc_allocated,
        (size_t)vm->gc_next);
#endif
    return old_allocation - vm->gc_allocated;
}

/* Allocator that can trigger gc. */
void* gcrealloc(VM* vm, void* ptr, ssize_t oldc, ssize_t newc)
{
    vm->gc_allocated += newc - oldc;
    ASSERT(
        newc >= oldc,
        "Tried freeing memory with gcrealloc() [newsize:%ld | oldsize:%ld].",
        newc,
        oldc);
#ifdef DEBUG_STRESS_GC
    if(newc > oldc) gc(vm);
#else
    if(!GC_CHECK(vm, GC_MANUAL_BIT) && vm->gc_next <= vm->gc_allocated) gc(vm);
#endif
    return REALLOC(vm, ptr, newc);
}

/* Freeing memory never triggers GC */
void* gcfree(VM* vm, void* ptr, ssize_t oldc, ssize_t newc)
{
    vm->gc_allocated += newc - oldc;
    ASSERT(
        newc <= oldc,
        "Tried allocating memory with gcfree() [newsize:%ld | oldsize:%ld]",
        newc,
        oldc);
    return REALLOC(vm, ptr, newc);
}

/* Allocator that never triggers gc. */
void* reallocate(void* ptr, size_t newc, void* _)
{
    UNUSED(_);
    if(newc == 0) {
        free(ptr);
        return NULL;
    }
    void* alloc = realloc(ptr, newc);
    if(alloc == NULL) {
        fprintf(stderr, "Internal error, allocation failure!\n");
        exit(errno);
    }
    return alloc;
}
