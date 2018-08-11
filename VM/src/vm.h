//-------------------------------------------------------------------
//
// Fast VM (Virtual Machine):
//
// FILE:
//   vm.h
//
// COMPILE:
//   gcc -c vm.c -O2 -Wall
//
// START DATE:
//   05/08/2018 - 06:48
//
// BY: Francisco - gokernel@hotmail.com
//
//-------------------------------------------------------------------
//
#ifndef _VM_H
#define _VM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------
//---------------  DEFINE / ENUM  ---------------
//-----------------------------------------------
//
#define ASM_DEFAULT_SIZE  50000
#define UCHAR             unsigned char
#define UINT              unsigned int
#define TYPE_NO_RETURN    100
#define GVAR_SIZE         255

enum {
    OP_HALT = 0,

    // Push a int value in the stack | sp++
    //   emit: 5 bytes
    OP_PUSH_INT,

    // Push a float value in the stack | sp++
    //   emit: 5 bytes
    OP_PUSH_FLOAT,

    OP_PUSH_VAR,

    OP_POP_VAR,
    OP_POP_EAX,

    OP_MUL_INT,
    OP_DIV_INT,
    OP_ADD_INT,
    OP_SUB_INT,

    OP_MUL_FLOAT,
    OP_DIV_FLOAT,
    OP_ADD_FLOAT,
    OP_SUB_FLOAT,

    // compare and jumps:
    OP_CMP_INT,
    OP_JUMP_JMP,
    OP_JUMP_JGE,
    OP_JUMP_JLE,
    OP_JUMP_JEQ,
    OP_JUMP_JNE,
    OP_JUMP_JG,
    OP_JUMP_JL,

    OP_PRINT_EAX,

    OP_MOV_EAX_VAR,

    // Call a Native C Function ... use the stack to pass arguments,
    // the return value in ( ret.? )
    //   emit in 32 bits: 7 bytes
    //   emit in 64 bits: 11 bytes
    OP_CALL,

    OP_CALL_VM
};
enum {
    TYPE_INT = 0,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_POINTER,
    TYPE_STRUCT,
    TYPE_PSTRUCT  // struct data *p;
};

//-----------------------------------------------
//-------------------  STRUCT  ------------------
//-----------------------------------------------
//
typedef struct ASM        ASM; // opaque struct in file: 'vm.c'
typedef struct ASM_label  ASM_label;
typedef struct ASM_jump   ASM_jump;
typedef union  VALUE      VALUE;
typedef struct TVar       TVar;

struct ASM { // private struct
    UCHAR     *p;
    UCHAR     *code;
    int       ip;
    ASM_label *label;
    ASM_jump  *jump;
};
struct ASM_label { // private struct
    char      *name;
    int       pos;
    ASM_label *next;
};
struct ASM_jump { // private struct
    char      *name;
    int       pos;
    ASM_jump  *next;
};

union VALUE {
    int     i;
    float   f;
    char    *pchar;
    void    *pvoid;
    UCHAR   uchar;
};
struct TVar {
    char    *name;
    int     type;
    VALUE   value;
    void    *info;  // any information ... struct type use this
};

extern TVar   Gvar[GVAR_SIZE]; // global:

//-------------------------------------------------------------------
//-------------------------  ASM PUBLIC API  ------------------------
//-------------------------------------------------------------------
//
extern void   vm_run          (ASM *a);
//
extern ASM  * asm_new         (UINT size);
extern void   asm_reset       (ASM *a);
extern void   asm_begin       (ASM *a);
extern void   asm_end         (ASM *a);
extern UINT   asm_get_len     (ASM *a);
extern void   asm_label       (ASM *a, char *name);

//-----------------------------------------------
//-----------------  EMIT / GEN  ----------------
//-----------------------------------------------
//
extern void emit_push_int     (ASM *a, int i);
extern void emit_push_float   (ASM *a, float value);
extern void emit_push_var     (ASM *a, UCHAR index);
extern void emit_pop_var      (ASM *a, UCHAR i);
extern void emit_pop_eax      (ASM *a);
extern void emit_mul_int      (ASM *a);
extern void emit_add_int      (ASM *a);
extern void emit_print_eax    (ASM *a, UCHAR type);
extern void emit_mov_eax_var  (ASM *a, UCHAR index);
extern void emit_call         (ASM *a, void *func, UCHAR arg_count, UCHAR return_type);
extern void emit_call_vm      (ASM *a, void *func, UCHAR arg_count);
extern void emit_halt         (ASM *a);

extern void emit_cmp_int      (ASM *a);
extern void emit_jump_jge     (ASM *a, char *name);
extern void emit_jump_jle     (ASM *a, char *name);
extern void emit_jump_jeq     (ASM *a, char *name);
extern void emit_jump_jne     (ASM *a, char *name);

#ifdef __cplusplus
}
#endif

#endif // ! _VM_H
