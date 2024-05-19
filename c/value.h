#ifndef clox_value_h
#define clox_value_h

#include "common.h"

/*
p.324
The name “Obj” itself refers to a struct that contains the state shared across
all object types. It’s sort of like the “base class” for objects. 
Because of some cyclic dependencies between values and objects, 
we forward-declare it in the “value” module.

And the actual definition is in a new module. -> object.h
*/
typedef struct Obj Obj; // forward declaration.
typedef struct ObjString ObjString; // forward declaration.

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

// union: 순서가 규칙적이지 않고, 미리 알 수 없는 다양한 타입의 데이터를 저장할 수 있도록 설계된 타입
typedef struct {
    ValueType type; // 타입
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as; // 실제 값 저장. 다양한 타입을 Value 구조체 하나 통합하여 관리
} Value;


// type checking macros 
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

// value extract macro. 값 추출 매크로
#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

// Value creation macros. Value 생성 매크로
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean=value}})
#define NIL_VAL ((Value){VAL_NIL, {.number=0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number=value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)object}})

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
