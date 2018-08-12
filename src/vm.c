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
static int callvm_stage2_position = 0;
static int flag;

void callvm (ASM *a) {
    vm_run(a);
}
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

case OP_PUSH_ARG: {
    UCHAR i = (UCHAR)a->code[a->ip++];
    sp++;
    sp[0] = a->arg[i];
//printf ("PUSH ARGUMENT(%d) = %ld\n", i, sp->l);
    } continue;


case OP_POP_VAR: {
    UCHAR i = (UCHAR)a->code[a->ip++];
    Gvar[i].value = sp[0]; break;
/*
    switch (Gvar[i].type) {
    case TYPE_INT:  Gvar[i].value.i = sp->i; break;
    case TYPE_FLOAT: Gvar[i].value.f = sp->f; break;
    }
*/
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

case OP_PRINT_STRING: {
    UCHAR i = (UCHAR)(a->code[a->ip++]);
    while (i--){
        if (a->code[a->ip]=='\\' && a->code[a->ip+1]=='n') {
            printf("%c", 10); // new line
            i--;
            a->ip++;
        } else printf ("%c", a->code[a->ip]);

        a->ip++;
    }
    } continue;

case OP_PRINT_ARG0:
    printf ("VM Function arg[0]: %d\n", a->arg[0].i);
    break;

case OP_MOV_EAX_VAR: {
    UCHAR i = (UCHAR)a->code[a->ip++];
    Gvar[i].value = eax;
    } continue;

case OP_MUL_INT: sp[-1].i *= sp[0].i; sp--; continue;
case OP_DIV_INT: sp[-1].i /= sp[0].i; sp--; continue;
case OP_ADD_INT: sp[-1].i += sp[0].i; sp--; continue;
case OP_SUB_INT: sp[-1].i -= sp[0].i; sp--; continue;

case OP_MUL_FLOAT: sp[-1].f *= sp[0].f; sp--; continue;
case OP_DIV_FLOAT: sp[-1].f /= sp[0].f; sp--; continue;
case OP_ADD_FLOAT: sp[-1].f += sp[0].f; sp--; continue;
case OP_SUB_FLOAT: sp[-1].f -= sp[0].f; sp--; continue;

case OP_CMP_INT:
    sp--;
    flag = (int)(sp[0].i - sp[1].i);
    sp--;
    continue;
//
// simple_language_0.9.0
//
case OP_JUMP_JMP:
    a->ip = *(unsigned short*)(a->code+a->ip);
    continue;

case OP_JUMP_JEQ: // !=
    if (!flag)
        a->ip = *(unsigned short*)(a->code+a->ip);
    else
        a->ip += sizeof(unsigned short);
    continue;

case OP_JUMP_JNE: // ==
    if (flag)
        a->ip = *(unsigned short*)(a->code+a->ip);
    else
        a->ip += sizeof(unsigned short);
    continue;

case OP_JUMP_JGE: // =<
    if (flag >= 0)
        a->ip = *(unsigned short*)(a->code+a->ip);
    else
        a->ip += sizeof(unsigned short);
    continue;

case OP_JUMP_JLE: // >=
    if (flag <= 0)
        a->ip = *(unsigned short*)(a->code+a->ip);
    else
        a->ip += sizeof(unsigned short);
    continue;

case OP_JUMP_JG:
    if (flag > 0)
        a->ip = *(unsigned short*)(a->code+a->ip);
    else
        a->ip += sizeof(unsigned short);
    continue;

case OP_JUMP_JL:
    if (flag < 0)
        a->ip = *(unsigned short*)(a->code+a->ip);
    else
        a->ip += sizeof(unsigned short);
    continue;

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



// call a VM Function
//
case OP_CALL_VM: {
    ASM *local = *(void**)(a->code+a->ip);
    a->ip += sizeof(void*);
    UCHAR arg_count = (UCHAR)(a->code[a->ip++]); //printf ("CALL ARG_COUNT = %d\n", arg_count);

    switch(arg_count){
    case 1: local->arg[0] = sp[0]; sp--; break;
    case 2:
        local->arg[1] = sp[0]; sp--;
        local->arg[0] = sp[0]; sp--;
        break;
    case 3:
        local->arg[2] = sp[0]; sp--;
        local->arg[1] = sp[0]; sp--;
        local->arg[0] = sp[0]; sp--;
        break;
    case 4:
        local->arg[3] = sp[0]; sp--;
        local->arg[2] = sp[0]; sp--;
        local->arg[1] = sp[0]; sp--;
        local->arg[0] = sp[0]; sp--;
        break;
    }//: switch(arg_count)

//printf ("call_vm arg_count: %d\n", arg_count);

    if (local == a) {

        //-----------------------------------------------------------
        //
        // here is position the next opcode BEFORE of recursive function.
        //
        //-----------------------------------------------------------
        //
        callvm_stage2_position = local->ip;
        a->ip = 0;
        //printf ("Todo antes da FUNCAO pos(%d) RECURSIVA CODE: %d\n", callvm_stage2_position, local->code[callvm_stage2_position]);
    } else {
        //printf ("PRIMEIRA VEZ EXECUTANDO\n");
        callvm_stage2_position = 0;
        local->ip = 0;
    }

    callvm (local);

    local->ip = callvm_stage2_position;
    //local->ip = vm->ip - 5;

    } continue;



case OP_HALT:
    //printf ("\nFunction vm_run()\nOP_HALT: (sp - stack): %d  sp[0].i = %d\n\n", (sp-stack), sp[0].i);
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

void asm_label (ASM *a, char *name) {
    if (name) {
        ASM_label *lab;
        ASM_label *l = a->label;

        // find if exist:
        while (l) {
            if (!strcmp(l->name, name)) {
                printf ("Label Exist: '%s'\n", l->name);
                return;
            }
            l = l->next;
        }

        if ((lab = (ASM_label*)malloc(sizeof(ASM_label))) != NULL) {
            lab->name = strdup (name);
            lab->pos  = (a->p - a->code); // the index

            // add on top:
            lab->next = a->label;
            a->label = lab;
        }
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

void emit_push_arg (ASM *a, UCHAR i) {
    *a->p++ = OP_PUSH_ARG;
    *a->p++ = i;
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
void emit_print_string (ASM *a, UCHAR size, const char *str) {
//printf("PRINTS POS: %d\n", (vm->p-vm->code));
    *a->p++ = OP_PRINT_STRING;
    *a->p++ = size;
    while (*str) {
        *a->p++ = *str++;
    }
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

void emit_call_vm (ASM *a, void *func, UCHAR arg_count) {
    *a->p++ = OP_CALL_VM;
    *(void**)a->p = func;
    a->p += sizeof(void*);
    *a->p++ = arg_count;
}


void emit_mov_eax_var (ASM *a, UCHAR index) {
    *a->p++ = OP_MOV_EAX_VAR;
    *a->p++ = index;
}

UINT asm_get_len (ASM *a) {
    return (a->p - a->code);
}

void emit_cmp_int (ASM *a) {
    *a->p++ = OP_CMP_INT;
}

static void conditional_jump (ASM *vm, char *name, UCHAR type) {
    ASM_jump *jump;

    if (name && (jump = (ASM_jump*)malloc (sizeof(ASM_jump))) != NULL) {

        *vm->p++ = type;

        jump->name = strdup (name);
        jump->pos  = (vm->p - vm->code); // the index

        // add on top:
        jump->next = vm->jump;
        vm->jump = jump;

        // to change ...
        *(unsigned short*)vm->p = (jump->pos+2); // the index
        vm->p += sizeof(unsigned short);
    }
}

void emit_jump_jge (ASM *a, char *name) {
    conditional_jump (a, name, OP_JUMP_JGE);
}
void emit_jump_jle (ASM *a, char *name) {
    conditional_jump (a, name, OP_JUMP_JLE);
}
void emit_jump_jeq (ASM *a, char *name) {
    conditional_jump (a, name, OP_JUMP_JEQ);
}
void emit_jump_jne (ASM *a, char *name) {
    conditional_jump (a, name, OP_JUMP_JNE);
}

