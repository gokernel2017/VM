//-------------------------------------------------------------------
//
// A Mini Language with less than 1500 Lines ... using VM
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
//   mini.c | mini000.c
//
//-------------------------------------------------------------------
//
#include "src/vm.h"

#define MINI_VERSION        000 // base
//
#define STR_ERRO_SIZE       1024

enum {
    TOK_INT = 255,
    TOK_IF,
    TOK_ID,
    TOK_NUMBER,
    TOK_STRING,
    TOK_EQUAL_EQUAL
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
static void word_if   (ASM *a); // if (a > b && c < d && i == 10 && k) { ... }
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

static int see (void);

int lex (void) {
    int c;
    register char *p = token;
    *p = 0;

top:
    //##############  REMOVE SPACE  #############
    while (*str && *str <= 32) {
        if (*str == '\n') line++;
        str++;
    }

    if (*str == 0) return tok = 0;

    //################  STRING  #################
    if (*str == '"') {
        str++; // '"'
        while (*str && *str != '"' && *str != '\r' && *str != '\n')
            *p++ = *str++;
        *p = 0;
        if (*str=='"') str++; else Erro("String erro");

        //return tok = TOK_STRING;
        return TOK_STRING;
    }

    //##########  WORD, IDENTIFIER ...  #########
    if ((*str >= 'a' && *str <= 'z') || (*str >= 'A' && *str <= 'Z') || *str == '_') {
        while (
        (*str >='a' && *str <= 'z')  || (*str >= 'A' && *str <= 'Z') ||
        (*str >= '0' && *str <= '9') || *str == '_')
        {
            *p++ = *str++;
        }
        *p = 0;

        if (!strcmp(token, "int")) { return TOK_INT; }
        if (!strcmp(token, "if")) { return TOK_IF; }

        return TOK_ID;
    }

    //#################  NUMBER  ################
    if (*str >= '0' && *str <= '9') {
        while ((*str >= '0' && *str <= '9') || *str == '.')
            *p++ = *str++;
        *p = 0;

        return TOK_NUMBER;
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
            if (*str=='/')  str++;
            else            Erro ("BLOCK COMMENT ERRO: '/'");
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

        if (str[0]=='=' && str[1]=='=') { *p++ = str[1]; *p=0; str += 2; printf ("token(%s) == \n", token); return TOK_EQUAL_EQUAL; }
/*
    if (str[0] == '=' && str[1] == '=') {
        str += 2;
        token[0] = '='; token[1] = '='; token[2] = 0;
        printf ("lex token(%s)\n", token);
        tok = TOK_EQUAL_EQUAL;
        return TOK_EQUAL_EQUAL;
    }
*/
    *p++ = *str;
    *p = 0;
    return *str++;
}
static int expr0 (ASM *a) {
    if (tok == TOK_ID) {
        int i;
        if ((i=VarFind(token)) != -1) {
            char _token[255];
            sprintf (_token, "%s", token);
            char *_str = str; // save
            int _tok = tok;
            tok=lex();
            if (tok == '=') {
                tok=lex();
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
        tok=lex();
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
        tok=lex();
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
        tok=lex(); expr0(a);
        if (tok != ')') {
            Erro ("ERRO )\n");
        }
        tok=lex();
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

            tok=lex();

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

        tok=lex();
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
    tok=lex();
    switch (tok) {
    case '{':
        while (!erro && tok && tok != '}') stmt(a);
        //----------------------------------------------------
        //do_block (l,a); //<<<<<<<<<<  no recursive  >>>>>>>>>>
        //----------------------------------------------------
        return 1;
    case TOK_INT: word_int (a); return 1;
    case TOK_IF:  word_if (a);  return 1;
    case ';':
        return 1;
    default: expression (a); return 1;
    case 0: return 0;
    }
    return 1;
}

int Parse (ASM *a, char *text) {
    str = text;
    line = 1;
    ErroReset ();
    asm_reset (a);
    asm_begin (a);
    while (!erro && stmt(a)) {
        // ... compiling ...
    }
    asm_end (a);
    return erro;
}

static int see (void) {
    char *s = str;
    while (*s) {
        if (*s=='\n' || *s==' ' || *s==9 || *s==13) {
            s++;
        } else return *s;
    }
    return 0;
}

static void word_int (ASM *a) {
    while ((tok=lex())) {
        if (tok == TOK_ID) {
            char name[255];
            int value = 0;

            strcpy (name, token); // save

            tok=lex ();
            if (tok == '=') {
                tok=lex ();
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
static void word_if (ASM *a) { // if (a > b && c < d && i == 10 && k) { ... }
    //**** to "push/pop"
    static char array[20][20];
    static int if_count_total = 0;
    static int if_count = 0;
    int is_negative;
    int _tok=0;

    if (lex()!='(') { Erro ("ERRO SINTAX (if) need char: '('\n"); return; }

    if_count++;
    sprintf (array[if_count], "_IF%d", if_count_total++);

    while (!erro && lex()) { // pass arguments: if (a > b) { ... }
        is_negative = 0;

        if (tok == '!') { is_negative = 1; lex(); }

        expr0 (a);

        if (erro) return;

/*
        if (tok == ')' || tok == TOK_AND_AND) asm_pop_eax(a); // 58     pop   %eax
        else                                  g (a,0x5a); // 5a     pop   %edx
*/
        _tok = tok;
        switch (_tok) {
/*
        case ')': // if (a) { ... }
        case TOK_AND_AND:
            g2(a,0x85,0xc0); // 85 c0    test   %eax,%eax
            if (is_negative == 0) asm_je  (a, array[if_count]);
            else                  asm_jne (a, array[if_count]);
            break;
*/

        case '>':             // if (a > b)  { jle }
        case '<':             // if (a < b)  { jge }
        case TOK_EQUAL_EQUAL: // if (a == b) { jne }
//        case TOK_NOT_EQUAL:   // if (a != b) { jeq }
            tok=lex();
            expr0(a);
            emit_cmp_int (a);
            if (tok=='>') emit_jump_jle (a,array[if_count]);
            if (tok=='<') emit_jump_jge (a,array[if_count]);
            if (tok==TOK_EQUAL_EQUAL) emit_jump_jne (a,array[if_count]); // ==
//            if (tok==TOK_NOT_EQUAL) emit_jump_jeq (a,array[if_count]); // !=
            break;
/*
        // if (a != b) { je }
        case TOK_NOT_EQUAL: // !=
            tok=lex(); expr0(a); asm_pop_eax(a); // 58     : pop   %eax
            asm_cmp_eax_edx(a);                 // 39 c2  : cmp   %eax,%edx
            asm_je (a,array[if_count]);
            break;
*/
        }//:switch(t->tok)
        if (tok==')') break;
    }

    if (see()=='{') stmt (a); else Erro ("word(if) need start block: '{'\n");

    asm_label (a, array[if_count]);
    if_count--;
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

        if (Parse(a,prog)==0) {
            vm_run (a);
        }
        else printf ("ERRO:\n%s\n", ErroGet());
    }
    else printf ("USAGE: %s <file.min>\n", argv[0]);

    printf ("Exiting With Sucess: \n");

    return 0;
}
