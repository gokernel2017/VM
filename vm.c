//-------------------------------------------------------------------
//
// VM (Virtual Machine):
//
// COMPILE:
//   gcc vm.c -o vm -O2 -Wall
//
// START DATE:
//   05/08/2018 - 06:48
//
// BY: Francisco - gokernel@hotmail.com
//
//-------------------------------------------------------------------
//
#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------
//---------------  DEFINE / ENUM  ---------------
//-----------------------------------------------
//
#define ASM_DEFAULT_SIZE  50000
#define UCHAR             unsigned char
#define UINT              unsigned int
#define STACK_SIZE        1024

enum {
    OP_HALT = 0,

    // Push a int value in the stack | sp++
    //   emit: 4 bytes
    OP_PUSH_INT,

    // Push a float value in the stack | sp++
    //   emit: 4 bytes
    OP_PUSH_FLOAT,

    OP_MUL_INT,
    OP_DIV_INT,
    OP_ADD_INT,
    OP_SUB_INT,

    OP_MUL_FLOAT,
    OP_DIV_FLOAT,
    OP_ADD_FLOAT,
    OP_SUB_FLOAT,

    // Call a Native C Function ... use the stack to pass arguments,
    // the return value in ( ret.? )
    //   emit in 32 bits: 6 bytes
    //   emit in 64 bits: 10 bytes
    OP_CALL
};
enum {
    TYPE_NONE = 0,
    TYPE_INT,
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
typedef struct ASM        ASM;
typedef struct ASM_label  ASM_label;
typedef struct ASM_jump   ASM_jump;
typedef union  VALUE      VALUE;

struct ASM {
    UCHAR     *p;
    UCHAR     *code;
    int       ip;
    ASM_label *label;
    ASM_jump  *jump;
};
struct ASM_label {
    char      *name;
    int       pos;
    ASM_label *next;
};
struct ASM_jump {
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

//-----------------------------------------------
//-----------------  VARIABLES  -----------------
//-----------------------------------------------
//
VALUE stack [STACK_SIZE];
VALUE *sp = stack;
VALUE ret;

//-------------------------------------------------------------------
//--------------------------  START VM RUN  -------------------------
//-------------------------------------------------------------------
//
void vm_run (ASM *a) {

    for (;;) {
    switch (a->code[a->ip++]) {

case OP_PUSH_INT:
    sp++;
    sp->i = *(int*)(a->code+a->ip);
    a->ip += 4;
    continue;

case OP_PUSH_FLOAT:
    sp++;
    sp->f = *(float*)(a->code+a->ip);
    a->ip += 4;
    continue;

case OP_MUL_INT: sp[-1].i *= sp[0].i; sp--; continue;
case OP_DIV_INT: sp[-1].i /= sp[0].i; sp--; continue;
case OP_ADD_INT: sp[-1].i += sp[0].i; sp--; continue;
case OP_SUB_INT: sp[-1].i -= sp[0].i; sp--; continue;

case OP_MUL_FLOAT: sp[-1].f *= sp[0].f; sp--; continue;
case OP_DIV_FLOAT: sp[-1].f /= sp[0].f; sp--; continue;
case OP_ADD_FLOAT: sp[-1].f += sp[0].f; sp--; continue;
case OP_SUB_FLOAT: sp[-1].f -= sp[0].f; sp--; continue;

//
// call a C Function
//
case OP_CALL:
    {
    int (*func)() = *(void**)(a->code+a->ip);
    float (*func_float)() = *(void**)(a->code+a->ip);
    a->ip += sizeof(void*);
    UCHAR arg_count = (UCHAR)(a->code[a->ip++]);
    UCHAR return_type = (UCHAR)(a->code[a->ip++]);

    switch (arg_count) {
    case 0: // no argument
        if (return_type == TYPE_NONE) // 0 | no return
            func ();
        else if (return_type == TYPE_FLOAT)
            ret.f = func_float ();
        else
            ret.i = func ();
        break; // no argument
    case 1: // 1 argument
        if (return_type == TYPE_NONE) // 0 | no return
            func (sp[0]);
        else if (return_type == TYPE_FLOAT)
            ret.f = func_float (sp[0]);
        else
            ret.i = func (sp[0]);
        sp--;
        break; // 1 argument
    case 2: // 2 arguents
        if (return_type == TYPE_NONE) // 0 | no return
            func (sp[-1], sp[0]);
        else if (return_type == TYPE_FLOAT)
            ret.f = func_float (sp[-1], sp[0]);
        else
            ret.i = func (sp[-1], sp[0]);
        sp -= 2;
        break; // 2 arguments

    }//: switch (arg_count)

    } continue; //: case OP_CALL:

case OP_HALT:
    printf ("\nFunction vm_run()\nOP_HALT: (sp - stack): %d  sp[0].i = %d\n\n", (sp-stack), sp[0].i);
    return;

    }//: switch (a->code[a->ip++])
    }//: for (;;)

}//: vm_run ()
//
//-------------------------------------------------------------------
//---------------------------  END VM RUN  --------------------------
//-------------------------------------------------------------------

ASM *asm_new (UINT size) {
    ASM *a = (ASM*)malloc (sizeof(ASM));
    if (a && (a->code = (UCHAR*)malloc(size)) != NULL) {
        a->p = a->code;
        a->label = NULL;
        a->jump = NULL;
        a->ip = 0;
        return a;
    }
    return NULL;
}

void emit_push_int (ASM *a, int i) {
    *a->p++ = OP_PUSH_INT;
    *(int*)a->p = i;
    a->p += 4;
}
void emit_add_int (ASM *a) {
    *a->p++ = OP_ADD_INT;
}
void emit_halt (ASM *a) {
    *a->p++ = OP_HALT;
}
void emit_call (ASM *a, void *func, UCHAR arg_count, UCHAR return_type) {
    *a->p++ = OP_CALL;
    *(void**)a->p = func;
    a->p += sizeof(void*);
    *a->p++ = arg_count;
    *a->p++ = return_type;
}

int function_add (int a, int b) {
    printf ("Function_add: a(%d), b(%d)\n", a, b);
    return a + b;
}

int main (int argc, char **argv) {
    ASM *a;

    if ((a = asm_new (ASM_DEFAULT_SIZE)) == NULL)
  return -1;


    emit_push_int (a,1500); // pass argument 1
    emit_push_int (a,2000); // pass argument 2

    //------------------------------------------
    // call a C Function with:
    //   a: 2 arguments
    //   b: return type int
    //
    // RESULT IN:
    //   ret.i
    //
    //------------------------------------------
    //
    emit_call (a, (void*)function_add, 2, TYPE_INT);
    emit_halt (a);

    vm_run (a);

    printf ("Exiting With Sucess: ret.i = %d\n", ret.i);

    return 0;
}
