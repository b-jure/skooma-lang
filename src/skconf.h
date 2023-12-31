#ifndef SKOOMA_CONFIG_H
#define SKOOMA_CONFIG_H

#define __STDC_LIMIT_MACROS

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __STDC_VERSION__
    #if __STDC_VERSION__ < 201112L
        #error "Minimum required C standard must be C11 (201112L)."
    #endif
#elif defined __cplusplus
    #if __cplusplus < 201103L
        #error "Minimum required C++ standard must be C++11 (201103L)."
    #endif
#else
    #error "Compiler not supported (no __STDC_VERSION__ or __cplusplus defined)."
#endif

#if !defined(__STDC_IEC_559__) || __DBL_DIG__ != 15 ||                          \
    __DBL_MANT_DIG__ != 53 || __DBL_MAX_10_EXP__ != 308 ||                      \
    __DBL_MAX_EXP__ != 1024 || __DBL_MIN_10_EXP__ != -307 ||                    \
    __DBL_MIN_EXP__ != -1021

    #error "Compiler missing IEEE-754 double precision floating point!"
#endif

static_assert(sizeof(void*) == 8, "Size of 'void*' is not 8.");
static_assert(
    sizeof(double) == sizeof(long),
    "Size of 'long' and 'double' not equal.");
static_assert(
    sizeof(int64_t) == sizeof(double),
    "Size of 'int64_t' and 'double' not equal.");

/**
 *
 * Note: Assuming GCC, MSVC or clang are newer versions
 * because they are supporting C11 or C++11!
 *
 * GCC      >= 4.6
 * Clang    >= 3.1
 * MSVC     >= 16.8
 *
 **/


#if defined(__GNUC__)

    #define S_PRECOMPUTED_GOTO

    #define force_inline   __always_inline
    #define likely(cond)   __glibc_likely(cond)
    #define unlikely(cond) __glibc_unlikely(cond)
    #define unused         __attribute__((unused))
    #define unreachable    __builtin_unreachable()

#elif defined(_MSC_VER) && !defined(__clang__)

    #define force_inline __force_inline
    #define unreachable  __assume(0)
    #define unused

    #ifndef __cplusplus
        #define inline         _inline
        #define likely(cond)   cond
        #define unlikely(cond) cond
    #elif _MSC_VER >= 1926
        #define likely(cond)   [[likely]] cond
        #define unlikely(cond) [[unlikely]] cond
    #else
        #define likely(cond)   cond
        #define unlikely(cond) cond
    #endif

    #define unused

#elif defined(__clang__)

    #define S_PRECOMPUTED_GOTO

    #if __has_attribute(always_inline)
        #define force_inline __attribute__((always_inline))
    #else
        #define force_inline inline
    #endif

    #if __has_attribute(unused)
        #define unused __attribute__((unused))
    #else
        #define unused
    #endif

    #if __has_builtin(__builtin_expect)
        #define likely(cond)   __builtin_expect(cond, 1)
        #define unlikely(cond) __builtin_expect(cond, 0)
    #else
        #define likely(cond)   cond
        #define unlikely(cond) cond
    #endif

    #if __has_builtin(__builtin_unreachable)
        #define unreachable __builtin_unreachable()
    #else
        #define unreachable                                                     \
            #include<stdio.h> #include<stdlib.h> printf(                        \
                "Unreachable code: %s:%d\n",                                    \
                __FILE__,                                                       \
                __LINE__);                                                      \
            abort();
    #endif

#else

    #define force_inline
    #define likely(cond)   cond
    #define unlikely(cond) cond
    #define unused
    #define unreachable                                                         \
        #include<stdio.h> #include<stdlib.h> printf(                            \
            "Unreachable code: %s:%d\n",                                        \
            __FILE__,                                                           \
            __LINE__);                                                          \
        abort();

#endif




#define sstatic static

#ifdef DEBUG
    #define sdebug
#else
    #define sdebug unused
#endif

/**
 * Max stack size in bytes, default set to 524 KiB.
 **/
#define S_STACK_MAX (1 << 19)

/**
 * Max temporary values VM can hold when
 * returning from a function.
 **/
#define S_TEMP_MAX 0xffffff

/**
 * Max function call frames.
 * This grows each time user calls a
 * callable value.
 **/
#define S_CALLFRAMES_MAX 256

/**
 * Allow NaN boxing of values.
 **/
#define S_NAN_BOX

/**
 * Default heap grow factor
 **/
#define GC_HEAP_GROW_FACTOR 2



/* For debug builds comment out 'defines' you dont want. */
#ifdef DEBUG

    /* Enable asserts */
    #define DEBUG_ASSERTIONS

    /* Print and disassemble bytecode for each chunk/function */
    #define DEBUG_PRINT_CODE

    /* Trace VM stack while executing */
    #define DEBUG_TRACE_EXECUTION

    /* Run garbage collection on each allocation */
    #define DEBUG_STRESS_GC

    /* Log garbage collection */
    // #define DEBUG_LOG_GC

    /* Dump stack of local variables on compile error */
    #define DEBUG_LOCAL_STACK

#endif


/* Virtual Machine */
typedef struct VM VM;


/**
 * Generic allocator function, used for every allocation, free and reallocation.
 * */
typedef void* (*AllocatorFn)(void* ptr, size_t newsize, void* userdata);


/**
 * Canonicalize the name of the script, the returned name will
 * be used: when reporting error inside that script, in comparisons
 * when resolving duplicate loads and finally will be passed
 * to 'ScriptLoadFinFn' as argument.
 **/
typedef const char* (
    *ScriptRenameFn)(VM* vm, const char* importer_script, const char* name);


/* Forward declare */
typedef struct ScriptLoadResult ScriptLoadResult;


/**
 * Function called after 'ScriptLoadFn' finishes, to perform
 * cleanup (if any).
 **/
typedef void (
    *ScriptLoadFinFn)(VM* vm, const char* name, ScriptLoadResult result);


/* Return result of 'ScriptLoadFn'. */
struct ScriptLoadResult {
    const char*     source; // Source file
    ScriptLoadFinFn finfn; // Cleanup/post-script-load function
    void*           userdata; // Custom user data (if any)
};


/**
 * Finds and loads the script returning 'ScriptLoadResult'.
 * Any memory allocated inside this function is user managed.
 * In order to cleanup that memory (if any) the 'ScriptLoadFinFn'
 * function should be provided, that function will run right after
 * this one finishes.
 **/
typedef ScriptLoadResult (*ScriptLoadFn)(VM* vm, const char* name);

/**
 * User modifiable configuration.
 * Create and initialize this 'Config' struct and pass it to 'VM_new'.
 **/
typedef struct {
    AllocatorFn    reallocate; // Generic allocator function
    ScriptRenameFn rename_script; // Script rename fn
    ScriptLoadFn   load_script; // Script loader fn
    void*          userdata; // User data (for 'AllocatorFn')
    size_t         gc_init_heap_size; // Initial heap allocation
    size_t         gc_min_heap_size; // Minimum size of heap after recalculation
    double         gc_grow_factor; // Heap grow factor
} Config;

void Config_init(Config* config);

#endif
