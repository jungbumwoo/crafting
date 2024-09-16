#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

// take a Value that is expected to contain a pointer to a valid ObjString on the heap.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value)) // returns the ObjString *pointer.
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value)) -> chars) // return character array itself.

typedef enum {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;  // Each Obj gets a pointer to the next Obj in the chain.
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
