//-------------------------------------------------------------------
//
// VM (Virtual Machine):
//
// TANKS TO:
// ----------------------------------------------
//
//   01: God the creator of the heavens and the earth in the name of Jesus Christ.
//
// ----------------------------------------------
//
// FILE:
//   vm.c
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
// LINES: 280
#include "vm.h"

#define STACK_SIZE    1024

typedef struct ASM_label  ASM_label;
typedef struct ASM_jump   ASM_jump;

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
struct data {
    int a;
    int b;
    int i;
};

//-----------------------------------------------
//-----------------  VARIABLES  -----------------
//-----------------------------------------------
//
TVar    Gvar [GVAR_SIZE]; // global:
//
static VALUE  stack [STACK_SIZE];
static VALUE *sp = stack;
static VALUE eax;

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

case OP_PUSH_VAR: {
    UCHAR i = (UCHAR)a->code[a->ip++];
    sp++;
    switch (Gvar[i].type) {
    case TYPE_INT:   sp->i = Gvar[i].value.i; break;
    case TYPE_FLOAT: sp->f = Gvar[i].value.f; break;
    }
    } continue;


case OP_POP_VAR: {
    UCHAR i = (UCHAR)a->code[a->ip++];
    switch (Gvar[i].type) {
    case TYPE_INT:  Gvar[i].value.i = sp->i; break;
    case TYPE_FLOAT: Gvar[i].value.f = sp->f; break;
    }
    sp--;
    } continue;

case OP_POP_EAX:
    eax = sp[0];
    sp--;
    continue;

case OP_PRINT_EAX: {
    UCHAR i = (UCHAR)a->code[a->ip++];
    switch (i) {
    case TYPE_INT:   printf ("%d\n", eax.i); break;
    case TYPE_FLOAT: printf ("%f\n", eax.f); break;
    }
    } continue;

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
        if (return_type == TYPE_NO_RETURN) // 0 | no return
            func ();
        else if (return_type == TYPE_FLOAT)
            eax.f = func_float ();
        else
            eax.i = func ();
        break; // no argument
    case 1: // 1 argument
        if (return_type == TYPE_NO_RETURN) // 0 | no return
            func (sp[0]);
        else if (return_type == TYPE_FLOAT)
            eax.f = func_float (sp[0]);
        else
            eax.i = func (sp[0]);
        sp--;
        break; // 1 argument
    case 2: // 2 arguents
        if (return_type == TYPE_NO_RETURN) // 0 | no return
            func (sp[-1], sp[0]);
        else if (return_type == TYPE_FLOAT)
            eax.f = func_float (sp[-1], sp[0]);
        else
            eax.i = func (sp[-1], sp[0]);
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
void asm_begin (ASM *a) {
}
void asm_reset (ASM *a) {

    a->p = a->code;

    // reset ASM_label:
    while (a->label != NULL) {
        ASM_label *temp = a->label->next;
        if (a->label->name)
            free (a->label->name);
        free (a->label);
        a->label = temp;
    }
    // reset ASM_jump:
    while (a->jump != NULL) {
        ASM_jump *temp = a->jump->next;
        if (a->jump->name)
            free(a->jump->name);
        free (a->jump);
        a->jump = temp;
    }

    a->label = NULL;
    a->jump  = NULL;
    a->ip = 0;
}

void asm_end (ASM *a) {
    ASM_label *label = a->label;

    *a->p++ = OP_HALT;

    //-----------------------
    // change jump:
    //-----------------------
    //
    while (label) {

        ASM_jump *jump = a->jump;

        while (jump) {
            if (!strcmp(label->name, jump->name)) {

                *(unsigned short*)(a->code+jump->pos) = label->pos;

            }
            jump = jump->next;
        }
        label = label->next;
    }
}

void emit_push_int (ASM *a, int i) {
    *a->p++ = OP_PUSH_INT;
    *(int*)a->p = i;
    a->p += 4;
}
void emit_push_float (ASM *a, float value) {
    *a->p++ = OP_PUSH_FLOAT;
    *(float*)a->p = value;
    a->p += 4;
}
void emit_push_var (ASM *a, UCHAR index) {
    *a->p++ = OP_PUSH_VAR;
    *a->p++ = index;
}

void emit_pop_var (ASM *a, UCHAR i) {
    *a->p++ = OP_POP_VAR;
    *a->p++ = i;
}
void emit_pop_eax (ASM *a) {
    *a->p++ = OP_POP_EAX;
}

void emit_mul_int (ASM *a) {
    *a->p++ = OP_MUL_INT;
}
void emit_add_int (ASM *a) {
    *a->p++ = OP_ADD_INT;
}
void emit_add_float (ASM *a) {
    *a->p++ = OP_ADD_FLOAT;
}

void emit_print_eax (ASM *a, UCHAR type) {
    *a->p++ = OP_PRINT_EAX;
    *a->p++ = type;
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
