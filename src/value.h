#ifndef SKOOMA_VALUE_H
#define SKOOMA_VALUE_H

#include "array.h"
#include "common.h"
#include "hash.h"

typedef struct O            O;
typedef struct OString      OString;
typedef struct OFunction    OFunction;
typedef struct OClosure     OClosure;
typedef struct OUpvalue     OUpvalue;
typedef struct OClass       OClass;
typedef struct OInstance    OInstance;
typedef struct OBoundMethod OBoundMethod;

#ifdef S_NAN_BOX

// Skooma 'Value' is NAN boxed except 'double'.
//
// Here is what each bit represents [0..63].
// bits 0..3              -> object type (or value if literal)
// bits 4..48             -> object pointer,
// bits 49..50            -> unused,
// bit  51                -> QNaN Floating-Point Indefinite bit
//                           (Intel Manual Volume 1: Chapter 4, 4-3 Table),
// bit  52                -> Quiet NaN bit,
// bits 53..62            -> NaN bits (exponent).
typedef uint64_t Value;

    // NAN 'box' mask
    #define QNAN 0x7ffc000000000000

    // Value type tags
    #define NIL_TAG    0x01
    #define FALSE_TAG  0x02
    #define TRUE_TAG   0x03
    #define EMPTY_TAG  0x04
    #define OBJECT_TAG 0x05 // 'O*' is on 8 byte alignment (first 3 bits are 0)

    #define AS_OBJ(val)        ((O*)((uintptr_t)((val) & 0x0000fffffffffff8)))
    #define AS_BOOL(val)       ((bool)((val) == TRUE_VAL))
    #define AS_NUMBER(val)     (vton(val))
    #define AS_NUMBER_REF(val) *(val)

    #define NUMBER_VAL(num) (ntov(num))
    #define OBJ_VAL(ptr)                                                        \
        ((Value)((((uint64_t)(ptr)) & 0x0000fffffffffff8) | (OBJECT_TAG | QNAN)))
    #define BOOL_VAL(boolean) ((Value)((FALSE_TAG | ((boolean) & 0x01)) | QNAN))
    #define TRUE_VAL          ((Value)(TRUE_TAG | QNAN))
    #define FALSE_VAL         ((Value)(FALSE_TAG | QNAN))
    #define NIL_VAL           ((Value)(QNAN | NIL_TAG))
    #define EMPTY_VAL         ((Value)(QNAN | EMPTY_TAG))
    #define UNDEFINED_VAL     EMPTY_VAL

    #define IS_NUMBER(val)    (((val) & QNAN) != QNAN)
    #define IS_NIL(val)       ((val) == NIL_VAL)
    #define IS_OBJ(val)       (((val) & (OBJECT_TAG | QNAN)) == (OBJECT_TAG | QNAN))
    #define IS_BOOL(val)      (((val) | 0x01) == TRUE_VAL)
    #define IS_EMPTY(val)     ((val) == EMPTY_VAL)
    #define IS_UNDEFINED(val) IS_EMPTY(val)

    #define OBJ_TYPE(val) (otype(AS_OBJ(val)))

// https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html (-fstrict-aliasing)
static inline Value ntov(double n)
{
    union {
        double n;
        Value  val;
    } bitcast;
    bitcast.n = n;
    return bitcast.val;
}

static inline double vton(Value val)
{
    union {
        double n;
        Value  val;
    } bitcast;
    bitcast.val = val;
    return bitcast.n;
}

static force_inline bool veq(Value a, Value b)
{
    if(IS_NUMBER(a) && IS_NUMBER(b)) return AS_NUMBER(a) == AS_NUMBER(b);
    return a == b;
}

#else

typedef enum {
    VAL_BOOL = 0,
    VAL_NUMBER,
    VAL_NIL,
    VAL_OBJ,
    VAL_EMPTY,
} ValueType;


    #define AS_OBJ(value)        ((value).as.object)
    #define AS_BOOL(value)       ((value).as.boolean)
    #define AS_NUMBER(value)     ((value).as.number)
    #define AS_NUMBER_REF(value) ((value)->as.number)

    #define IS_OBJ(value)     ((value).type == VAL_OBJ)
    #define IS_BOOL(value)    ((value).type == VAL_BOOL)
    #define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
    #define IS_NIL(value)     ((value).type == VAL_NIL)
    #define IS_EMPTY(value)   ((value).type == VAL_EMPTY)
    #define IS_UNDEFINED(val) IS_EMPTY(val)

    #define OBJ_VAL(value)    ((Value){.type = VAL_OBJ, {.object = (O*)value}})
    #define BOOL_VAL(value)   ((Value){.type = VAL_BOOL, {.boolean = value}})
    #define NUMBER_VAL(value) ((Value){.type = VAL_NUMBER, {.number = value}})
    #define EMPTY_VAL         ((Value){.type = VAL_EMPTY, {0}})
    #define NIL_VAL           ((Value){.type = VAL_NIL, {0}})
    #define UNDEFINED_VAL     EMPTY_VAL

    #define OBJ_TYPE(value)   (Obj_type(AS_OBJ(value)))
    #define VALUE_TYPE(value) ((value).type)

typedef struct {
    ValueType type;
    union {
        bool   boolean;
        double number;
        O*     object;
    } as;
} Value;

bool Value_eq(Value a, Value b);

#endif

ARRAY_NEW(Array_Value, Value);

#ifndef SKOOMA_VMACHINE_H
typedef struct VM VM;
#endif

OString* vtostr(VM* vm, Value value);
void     vprint(Value value);
Hash     vhash(Value value);
Byte     dtos_generic(double dbl, char* dest, UInt len);
Byte     booltos_generic(bool boolean, char* dest, UInt len);
Byte     niltos_generic(char* dest, UInt len);

#endif
