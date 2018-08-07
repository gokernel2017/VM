//-------------------------------------------------------------------
//
// A Mini Language with less 1500 Lines ... using VM
//
// TANKS TO:
// ----------------------------------------------
//
//   01: God the creator of the heavens and the earth in the name of Jesus Christ.
//
// ----------------------------------------------
//
// COMPILE:
//   make clean
//   make
//
// FILE:
//   mini.c
//
//-------------------------------------------------------------------
//
#include "src/vm.h"

#define STR_ERRO_SIZE   1024

enum {
    TOK_INT = 255,
    TOK_ID,
    TOK_NUMBER,
    TOK_STRING
};

typedef struct TFunc TFunc;
struct TFunc {
    char    *name;
    char    *proto; // prototype
    UCHAR   *code;  // the function on JIT MODE | or VM in VM MODE
    int     type;   // FUNC_TYPE_NATIVE_C = 0, FUNC_TYPE_COMPILED, FUNC_TYPE_VM
    int     len;
    TFunc   *next;
};
void lib_info (int arg);
static TFunc stdlib[]={
  //-----------------------------------------------------------------
  // char*        char*   UCHAR*/ASM*             int   int   FUNC*
  // name         proto   code                    type  len   next
  //-----------------------------------------------------------------
  { "info",       "0i",   (UCHAR*)lib_info,       0,    0,    NULL },
  { NULL,         NULL,   NULL,                   0,    0,    NULL }
};


char *str, token[1025];
int erro, tok, line, main_variable_type, var_type, is_function;

static void word_int  (ASM *a);
static int  expr0     (ASM *a);
static void expr1     (ASM *a);
static void expr2     (ASM *a);
static void expr3     (ASM *a);
static void atom      (ASM *a);
//
void Erro (char *s);
int VarFind (char *name);
TFunc *FuncFind (char *name);
void execute_call (ASM *a, TFunc *func);

int lex (void) {
    register char *p = token;
    *p = 0;

top:
    while (*str && *str <= 32) {
        if (*str == '\n') line++;
        str++; // remove space
    }

    if (*str == 0) return tok = 0;

    //##########  WORD, IDENTIFIER ...  #########
    if ((*str >= 'a' && *str <= 'z') || (*str >= 'A' && *str <= 'Z') || *str == '_') {
        while (
        (*str >='a' && *str <= 'z')  || (*str >= 'A' && *str <= 'Z') ||
        (*str >= '0' && *str <= '9') || *str == '_')
        {
            *p++ = *str++;
        }
        *p = 0;

        if (!strcmp(token, "int")) { return tok = TOK_INT; }

        return tok = TOK_ID;
    }

    //#################  NUMBER  ################
    if (*str >= '0' && *str <= '9') {
        while ((*str >= '0' && *str <= '9') || *str == '.')
            *p++ = *str++;
        *p = 0;

        return tok = TOK_NUMBER;
    }

    //##########  REMOVE COMMENTS  ##########
    if (*str == '/'){
        if (str[1] == '*') { // comment block
            str += 2;
            do {
                while (*str && *str != '*') {
                    if (*str == '\n') line++; //<<<<<<<<<<  line++  >>>>>>>>>>
                    str++;
                }
                str++;
            } while (*str && *str != '/');
            if (*str=='/') str++;
            else Erro ("BLOCK COMMENT ERRO: '/'");
            goto top;
        } else if (str[1] == '/') { // comment line
            str += 2;
            while ((*str) && (*str != '\n') && (*str != '\r'))
                str++;
            goto top;
        }
    }

    *p++ = *str;
    *p = 0;
    return tok = *str++;
}
static int expr0 (ASM *a) {
    if (tok == TOK_ID) {
        int i;
        if ((i=VarFind(token)) != -1) {
            char _token[255];
            sprintf (_token, "%s", token);
            char *_str = str; // save
            int _tok = tok;
            lex();
            if (tok == '=') {
                lex();
                expr0(a);
                // Copia o TOPO DA PILHA ( sp ) para a variavel ... e decrementa sp++.
                emit_pop_var (a,i);
                return i;
            } else {
                sprintf (token, "%s", _token);
                str = _str; // restore
                tok = _tok;
            }
        }
    }
    expr1(a);
    return -1;
}
static void expr1 (ASM *a) { // '+' '-' : ADDITION | SUBTRACTION
    int op;
    expr2(a);
    while ((op=tok) == '+' || op == '-') {
        lex();
        expr2(a);
        if (main_variable_type==TYPE_FLOAT) {
//            if (op=='+') emit_add_float (a);
        } else { // INT
            if (op=='+') emit_add_int (a);
        }

    }
}
static void expr2 (ASM *a) { // '*' '/' : MULTIPLICATION | DIVISION
    int op;
    expr3(a);
    while ((op=tok) == '*' || op == '/') {
        lex();
        expr3(a);
        if (main_variable_type==TYPE_FLOAT) {
//            if (op=='*') emit_mul_float (a);
        } else { // LONG
            if (op=='*') emit_mul_int (a);
        }
    }
}
static void expr3 (ASM *a) { // '('
    if (tok=='(') {
printf ("SUB EXPRESSION (\n");
        lex(); expr0(a);
        if (tok != ')') {
            Erro ("ERRO )\n");
        }
        lex();
    }
    else atom(a); // atom:
}
// atom:
static void atom (ASM *a) {
    if (tok==TOK_ID) {
        int i;

        if ((i=VarFind(token))!=-1) {
            var_type = Gvar[i].type;

            emit_push_var (a, i);

            lex();

        } else {
            char buf[255];
            sprintf(buf, "ERRO LINE(%d expr0()): Var Not Found '%s'", line, token);
            Erro (buf);
        }
    }
    else if (tok==TOK_NUMBER) {
        if (strchr(token, '.'))
            var_type = TYPE_FLOAT;

        if (var_type==TYPE_FLOAT)
            emit_push_float (a, atof(token));
        else
            emit_push_int (a, atoi(token));

        lex();
    }
    else { Erro("Expression"); printf ("LINE: %d token(%s)\n", line, token); }
}
void CreateVarInt (char *name, int value) {
    TVar *v = Gvar;
    int i = 0;
    while (v->name) {
        if (!strcmp(v->name, name))
      return;
        v++;
        i++;
    }
    if (i < GVAR_SIZE) {
        v->name = strdup(name);
        v->type = TYPE_INT;
        v->value.i = value;
        v->info = NULL;
    }
}
void CreateVarFloat (char *name, float value) {
    TVar *v = Gvar;
    int i = 0;
    while (v->name) {
        if (!strcmp(v->name, name))
      return;
        v++;
        i++;
    }
    if (i < GVAR_SIZE) {
        v->name = strdup(name);
        v->type = TYPE_FLOAT;
        v->value.f = value;
        v->info = NULL;
    }
}
int VarFind (char *name) {
    TVar *v = Gvar;
    int i = 0;
    while(v->name) {
        if (!strcmp(v->name, name))
      return i;
        v++;
        i++;
    }
    return -1;
}


static char strErro[STR_ERRO_SIZE];
void Erro (char *s) {
    erro++;
    if ((strlen(strErro) + strlen(s)) < STR_ERRO_SIZE)
        strcat (strErro, s);
}
char *ErroGet (void) {
    if (strErro[0])
        return strErro;
    else
        return NULL;
}
void ErroReset (void) {
    erro = 0;
    strErro[0] = 0;
}

void expression (ASM *a) {
    char buf[255];

    if (tok == TOK_ID || tok == TOK_NUMBER) {
        TFunc *fi;

        main_variable_type = var_type = TYPE_INT; // 0

        // call a function without return:
        // function_name (...);
        if ((fi = FuncFind(token)) != NULL) {
            execute_call (a, fi);
      return;
        }

        // expression type:
        // 10 * 20 + 3 * 5;
        if (expr0(a) == -1) {
            emit_pop_eax (a);
            emit_print_eax (a,TYPE_INT);
        }

    } else {
        sprintf (buf, "EXPRESSION ERRO LINE(%d) - Ilegar Word (%s)\n", line, token);
        Erro (buf);
    }
}

int stmt (ASM *a) {
    lex();
    switch (tok) {
    case TOK_INT: word_int (a); return 1;
    case ';':
        return 1;
    default: expression (a); return 1;
    case 0: return 0;
    }
    return 1;
}

int Parse (ASM *a, char *text) {
    str = text;
    erro = 0;
    line = 1;
    asm_reset (a);
    asm_begin (a);
    while (!erro && stmt(a)) {
        // ... compiling ...
    }
    asm_end (a);
    return erro;
}

static void word_int (ASM *a) {
    while (lex()) {
        if (tok == TOK_ID) {
            char name[255];
            int value = 0;

            strcpy (name, token); // save

            lex ();
            if (tok == '=') {
                lex ();
                if (tok == TOK_NUMBER)
                    value = atoi (token);
            }
            if (is_function) {
/*
                strcpy (local.name[local.count], name);
                local.value[local.count] = value;
//  c7 45   fc    39 30 00 00 	movl   $0x3039,0xfffffffc(%ebp)
//  c7 45   f8    dc 05 00 00 	movl   $0x5dc,0xfffffff8(%ebp)
                g3(a,0xc7,0x45,(char)pos);
                *(long*)(a->code+a->len) = value;
                a->len += sizeof(long);

                pos -= 4;
                local.count++;
*/
            }
            else CreateVarInt (name, value);

        }
        if (tok == ';') break;
    }
}

//
// function_name (a, b, c + d);
//
void execute_call (ASM *a, TFunc *func) {
    int count = 0, pos = 0, size = 4;
    int return_type = TYPE_INT;

    // get next: '('
    if (lex() != '(') {
        char buf[100];
        sprintf (buf, "ERRO LINE(%d) | Function(%s) need char: '('\n", line, func->name);
        Erro (buf);
        return;
    }

    while (lex()) {
        expr0 (a);
        pos += size;
        if (count++ > 15) break;
        if (tok == ')' || tok == ';') break;
    }
    if (func->proto) {
        if (func->proto[0] == '0') return_type = TYPE_NO_RETURN;
        if (func->proto[0] == 'f') return_type = TYPE_FLOAT;
    }
    emit_call (a, (void*)func->code, (UCHAR)count, return_type);
}

void lib_info (int arg) {
    switch (arg) {
    case 1: {
        TVar *v = Gvar;
        int i = 0;
        printf ("VARIABLES:\n---------------\n");
        while (v->name) {
            if (v->type==TYPE_INT)   printf ("Gvar[%d](%s) = %d\n", i, v->name, v->value.i);
            if (v->type==TYPE_FLOAT) printf ("Gvar[%d](%s) = %f\n", i, v->name, v->value.f);
            v++; i++;
        }
        } break;

    case 2:
        break;
    case 3:
        break;

    default:
        printf ("USAGE(%d): info(1);\n\nInfo Options:\n 1: Variables\n 2: Functions\n 3: Defines\n 4: Words\n",arg);
    }
}

TFunc *FuncFind (char *name) {
    TFunc *lib;

    // array:
    lib = stdlib;
    while (lib->name) {
        if ((lib->name[0]==name[0]) && !strcmp(lib->name, name))
      return lib;
        lib++;
    }

    return NULL;
}



int main (int argc, char *argv[]) {
    FILE *fp;
    ASM *a;

    if ((a = asm_new (ASM_DEFAULT_SIZE)) == NULL)
  return -1;

    if (argc >= 2 && (fp=fopen(argv[1], "rb"))!=NULL) {
        int i = 0, c;
#define PROG_SIZE  50000
        char prog [PROG_SIZE];
        while ((c=getc(fp))!=EOF) prog[i++] = c; // store prog[];
        fclose (fp);

        if (Parse(a,prog)==0)
        {
            vm_run (a);
        }
        else printf ("ERRO:\n%s\n", ErroGet());
    }
    else printf ("\nUSAGE:\n  %s <file.mini>\n\n", argv[0]);

    printf ("Exiting With Sucess: \n");

    return 0;
}
