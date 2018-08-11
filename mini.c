//-------------------------------------------------------------------
//
// Mini Language with LEXER:
//
// A Mini Language with less than 1000 Lines ... using VM
//
// TANKS TO:
// ----------------------------------------------
//
//   01: God the creator of the heavens and the earth in the name of Jesus Christ.
//
// ----------------------------------------------
//
// IMPLEMENTATION ( lex(): using LEXER ):
//   01: word int
//   02: word if
//   03: word function
//   -----------------
//   04: expression
//   05: call C Native Functions
//
// COMPILE:
//   make clean
//   make
//
// PROJECT PAGE:
//   https://github.com/gokernel2017/VM
//
// FILE:
//   mini.c
//
// BY: Francisco - gokernel@hotmail.com
//
//-------------------------------------------------------------------
//

#include "src/vm.h"

#define LEXER_NAME_SIZE   255
#define LEXER_TOKEN_SIZE  1024 * 4
#define STR_ERRO_SIZE     1024

typedef struct {
    char  *text;
    char  name  [LEXER_NAME_SIZE];
    char  token [LEXER_TOKEN_SIZE];
    //
    int   pos; // text [ pos ]
    int   line;
    int   tok;
}LEXER;

enum { FUNC_TYPE_NATIVE_C = 0, FUNC_TYPE_COMPILED, FUNC_TYPE_VM };

typedef struct TFunc TFunc;
struct TFunc {
    char    *name;
    char    *proto; // prototype
    UCHAR   *code;  // the function on JIT MODE | or VM in VM MODE
    int     type;   // FUNC_TYPE_NATIVE_C = 0, FUNC_TYPE_COMPILED, FUNC_TYPE_VM
    int     len;
    TFunc   *next;
};
void  lib_info  (int arg);
int   lib_add   (int a, int b);
void  lib_help  (void);
static TFunc stdlib[]={
  //-----------------------------------------------------------------
  // char*        char*   UCHAR*/ASM*             int   int   FUNC*
  // name         proto   code                    type  len   next
  //-----------------------------------------------------------------
  { "info",       "0i",   (UCHAR*)lib_info,       0,    0,    NULL },
  { "add",        "iii",  (UCHAR*)lib_add,        0,    0,    NULL },
  { "help",       "00",   (UCHAR*)lib_help,       0,    0,    NULL },
  { NULL,         NULL,   NULL,                   0,    0,    NULL }
};

enum {
    // types:
    TOK_INT = 255,
    TOK_FLOAT,
    TOK_IF,
    TOK_FUNCTION,
    //-----------------------
    TOK_ID,
    TOK_STRING,
    TOK_NUMBER,
    //-------------
    TOK_PLUS_PLUS,    // ++
    TOK_MINUS_MINUS,  // --
    TOK_EQUAL_EQUAL,  // ==
    TOK_NOT_EQUAL,    // !=
    TOK_AND_AND,      // &&
    TOK_PTR           // ->
};

static TFunc *Gfunc = NULL;  // store the user functions
ASM *asm_function;
int erro, is_function, is_recursive, main_variable_type, var_type, argument_count;
static char
    func_name [100]
    ;

// prototypes:
static void word_int      (LEXER *l, ASM *a);
static void word_if       (LEXER *l, ASM *a);
static void word_function (LEXER *l, ASM *a);
static int  expr0         (LEXER *l, ASM *a);
static int  see           (LEXER *l);

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

//
// return tok number
//
int lex (LEXER *l) {
    register char *p;
    register int c;
    int next; // next char

    p = l->token;
    *p = 0;

top:
    c = l->text[ l->pos ];

    //##############  REMOVE SPACE  #############
    if (c <= 32) {
        if (c == 0) {
            l->tok = 0;
            return 0;
        }
        if (c == '\n') l->line++;
        l->pos++; //<<<<<<<<<<  increment position  >>>>>>>>>>
        goto top;
    }

    //################  STRING  #################
    if (c == '"') {
        l->pos++; // '"'
        while ((c=l->text[l->pos]) && c != '"' && c != '\r' && c != '\n') {
            l->pos++;
            *p++ = c;
        }
        *p = 0;

        if (c=='"') l->pos++; else Erro("String erro");

        l->tok = TOK_STRING;
        return TOK_STRING;
    }

    //##########  WORD, IDENTIFIER ...  #########
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        for (;;) {
            c = l->text[l->pos];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                l->pos++;
                *p++ = c;
            } else break;
        }
        *p = 0;

        if (!strcmp(l->token, "int"))       { l->tok = TOK_INT;       return TOK_INT; }
        if (!strcmp(l->token, "float"))     { l->tok = TOK_FLOAT;     return TOK_FLOAT; }
        if (!strcmp(l->token, "if"))        { l->tok = TOK_IF;        return TOK_IF; }
        if (!strcmp(l->token, "function"))  { l->tok = TOK_FUNCTION;  return TOK_FUNCTION; }

        l->tok = TOK_ID;
        return TOK_ID;
    }

    //#################  NUMBER  ################
    if (c >= '0' && c <= '9') {
        for (;;) {
            c = l->text[l->pos];
            if ((c >= '0' && c <= '9') || c == '.') {
                l->pos++;
                *p++ = c;
            } else break;
        }
        *p = 0;
        l->tok = TOK_NUMBER;
        return TOK_NUMBER;
    }

    //##########  REMOVE COMMENTS  ##########
    if (c == '/') {
        if (l->text[l->pos+1] == '*') { // comment block
            l->pos += 2;
            do {
                while ((c=l->text[l->pos]) && c != '*') {
                    if (c == '\n') l->line++; //<<<<<<<<<<  line++  >>>>>>>>>>
                    l->pos++;
                }
                if (c) {
                    l->pos++;
                    c = l->text[l->pos];
                }
            } while (c && c != '/');
            if (c == '/') l->pos++;
            else          Erro ("BLOCK COMMENT ERRO: '/'");
            goto top;

        } else if (l->text[l->pos+1] == '/') { // comment line
            l->pos += 2;
            while ((c=l->text[l->pos]) && (c != '\n') && (c != '\r'))
                l->pos++;
            goto top;
        }
    }//: if (c == '/')


    //---------------------------------------------------------------
    //################# ! C suported character ...  #################
    //---------------------------------------------------------------
    //
    next = l->text[ l->pos+1 ];

    if (c=='=' && next == '=') { // ==
        *p++ = '='; *p++ = '='; *p = 0;
        l->pos += 2;
        l->tok = TOK_EQUAL_EQUAL;
        return TOK_EQUAL_EQUAL;
    }

    *p++ = c;
    *p = 0;
    l->pos++;
    l->tok = c;
    return c;

}//: lex()

void lex_set (LEXER *l, char *text, char *name) {
    if (l && text) {
        l->pos = 0;
        l->line = 1;
        l->text = text;
        if (name)
            strcpy (l->name, name);
    }
}//: lex_set()

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
TFunc *FuncFind (char *name) {
    TFunc *lib;
    TFunc *func;

    // array:
    lib = stdlib;
    while (lib->name) {
        if ((lib->name[0]==name[0]) && !strcmp(lib->name, name))
      return lib;
        lib++;
    }

    // linked list:
    func = Gfunc;
    while(func){
        if ((func->name[0]==name[0]) && !strcmp(func->name, name))
      return func;
        func = func->next;
    }

    return NULL;
}

static int see (LEXER *l) {
    char *s = l->text+l->pos;
    while (*s) {
        if (*s=='\n' || *s==' ' || *s==9 || *s==13) {
            s++;
        } else return *s;
    }
    return 0;
}
char * FileOpen (const char *FileName) {
    FILE *fp;

    if ((fp = fopen (FileName, "rb")) != NULL) {
        char *str;
        int size, i;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        str = (char *)malloc (size + 5);
        if(!str){
            fclose (fp);
            return NULL;
        }
        i = fread(str, 1, size, fp);
        fclose(fp);
        str[i] = 0;
        str[i+1] = 0;

        return str;
    }
    else printf ("File Not Found: '%s'\n", FileName);

    return NULL;
}

static void atom (LEXER *l, ASM *a) {
    if (l->tok==TOK_ID) {
        int i;

        if ((i=VarFind(l->token))!=-1) {
            var_type = Gvar[i].type;

            emit_push_var (a,i);

            lex(l);

        } else {
            char buf[255];
            sprintf(buf, "ERRO LINE(%d expr0()): Var Not Found '%s'", l->line, l->token);
            Erro (buf);
        }
    }
    else if (l->tok==TOK_NUMBER) {
        if (strchr(l->token, '.'))
            var_type = TYPE_FLOAT;

        if (var_type==TYPE_FLOAT)
            emit_push_float (a, atof(l->token));
        else
            emit_push_int (a, atoi(l->token));

        lex(l);
    }
    else { Erro("Expression"); printf ("LINE: %d token(%s)\n", l->line, l->token); }
}
static void expr3 (LEXER *l, ASM *a) { // '('
    if (l->tok=='(') {
        lex(l); expr0(l,a);
        if (l->tok != ')') {
            Erro ("ERRO )\n");
        }
        lex(l);
    }
    else atom(l,a); // atom:
}
static void expr2 (LEXER *l, ASM *a) { // '*' '/' : MULTIPLICATION | DIVISION
    int op;
    expr3(l,a);
    while ((op=l->tok) == '*' || op == '/') {
        lex(l);
        expr3(l,a);
        if (main_variable_type==TYPE_FLOAT) {
//            if (op=='*') emit_mul_float (a);
        } else { // LONG
            if (op=='*') emit_mul_int (a);
        }
    }
}
static void expr1 (LEXER *l, ASM *a) { // '+' '-' : ADDITION | SUBTRACTION
    int op;
    expr2(l,a);
    while ((op=l->tok) == '+' || op == '-') {
        lex(l);
        expr2(l,a);
        if (main_variable_type==TYPE_FLOAT) {
//            if (op=='+') emit_add_float (a);
        } else { // INT
            if (op=='+') emit_add_int (a);
        }

    }
}
static int expr0 (LEXER *l, ASM *a) {
    if (l->tok == TOK_ID) {
        int i;
        if ((i=VarFind(l->token)) != -1) {
            char token[255];
            sprintf (token, "%s", l->token);
            int pos = l->pos; // save
            int tok = l->tok;
            lex(l);
            if (l->tok == '=') {
                lex(l);
                expr0(l,a);
                // Copia o TOPO DA PILHA ( sp ) para a variavel ... e decrementa sp++.
                emit_pop_var (a,i);
                return i;
            } else {
                sprintf (l->token, "%s", token);
                l->pos = pos; // restore
                l->tok = tok;
            }
        }
    }
    expr1(l,a);
    return -1;
}

//
// function_name (a, b, c + d);
//
void execute_call (LEXER *l, ASM *a, TFunc *func) {
    int count = 0;
    int return_type = TYPE_INT;

    // no argument
    if (func->proto && func->proto[1] == '0') {
        while (lex(l))
            if (l->tok == ')' || l->tok == ';') break;
    } else {
        // get next: '('
        if (lex(l)!='(') { Erro ("Function need char: '('\n"); return; }

        while (lex(l)) {
            if (l->tok==TOK_ID || l->tok==TOK_NUMBER || l->tok=='(') {
                expr0 (l,a);
                if (count++ > 15) break;
            }
            if (l->tok == ')' || l->tok == ';') break;
        }
    }

    if (func->proto) {
        if (func->proto[0] == '0') return_type = TYPE_NO_RETURN;
        if (func->proto[0] == 'f') return_type = TYPE_FLOAT;
    }
    if (func->type == FUNC_TYPE_VM) {
//printf ("VM funcao(%s)\n", func->name);
//        ASM *local = fi->code;
//        local->ip = 0;
        //
        // here: fi->code ==  ASM*
        //
        emit_call_vm (a, (ASM*)(func->code), (UCHAR)count);
    } else {
        //asm_call (a, fi->code, (UCHAR)count);
        emit_call (a, (void*)func->code, (UCHAR)count, return_type);
    }
}

void expression (LEXER *l, ASM *a) {
    char buf[255];

    if (l->tok == TOK_ID || l->tok == TOK_NUMBER) {
        int i;
        TFunc *fi;

        main_variable_type = var_type = TYPE_INT; // 0

        //
        // call a function without return:
        //   function_name (...);
        //
        if ((fi = FuncFind(l->token)) != NULL) {
            execute_call (l, a, fi);
      return;
        }

        if ((i = VarFind(l->token)) != -1) {

            main_variable_type = var_type = Gvar[i].type;

            if (see(l) == '=') {
                char token[255];
                sprintf (token, "%s", l->token);
                int pos = l->pos; // save
                int tok = l->tok;
                lex(l); // "="
                lex(l); // "function_name"
                //
                // call a function with return:
                //   var = function_name (...);
                //
                if ((fi = FuncFind(l->token)) != NULL) {

                    execute_call (l, a, fi);

                    emit_mov_eax_var(a,i); // copy return function to var:

                    return;
                } else {
                    // .. !restore token ...
                    sprintf (l->token, "%s", token);
                    l->pos = pos; // restore
                    l->tok = tok;
                }
            }//: if (see(l) == '=')

        }//: if ((i = VarFind(l->token)) != -1)

        // expression type:
        // 10 * 20 + 3 * 5;
        if (expr0(l,a) == -1) {
            emit_pop_eax (a);
            emit_print_eax (a,main_variable_type);
        }

    } else {
        sprintf (buf, "EXPRESSION ERRO LINE(%d) - Ilegar Word (%s)\n", l->line, l->token);
        Erro (buf);
    }
}


static int stmt (LEXER *l, ASM *a) {
    switch (lex(l)) {
    case '{':
        while (!erro && l->tok && l->tok != '}') stmt(l,a);
        //----------------------------------------------------
        //do_block (l,a); //<<<<<<<<<<  no recursive  >>>>>>>>>>
        //----------------------------------------------------
        return 1;
    case TOK_INT:       word_int      (l,a); return 1;
    case TOK_IF:        word_if       (l,a); return 1;
    case TOK_FUNCTION:  word_function (l,a); return 1;
    default:            expression    (l,a); return 1;
    case ';':
    case '}':
        return 1;
    case 0: return 0;
    }
    return 1;
}
static void word_int (LEXER *l, ASM *a) {
    while (lex(l)) {
        if (l->tok == TOK_ID) {
            char name[255];
            int value = 0;

            strcpy (name, l->token); // save

            lex(l);
            if (l->tok == '=') {
                lex (l);
                if (l->tok == TOK_NUMBER)
                    value = atoi (l->token);
            }
            if (is_function) {
                // ... need implementation ...
            }
            else CreateVarInt (name, value);

        }
        if (l->tok == ';') break;
    }
    if (l->tok != ';') Erro ("ERRO: The word(float) need the char(;) on the end\n");
}
static void word_if (LEXER *l, ASM *a) { // if (a > b && c < d && i == 10 && k) { ... }
    //**** to "push/pop"
    static char array[20][20];
    static int if_count_total = 0;
    static int if_count = 0;
    int is_negative;
    int tok=0;

    if (lex(l)!='(') { Erro ("ERRO SINTAX (if) need char: '('\n"); return; }

    if_count++;
    sprintf (array[if_count], "_IF%d", if_count_total++);

    while (!erro && lex(l)) { // pass arguments: if (a > b) { ... }
        is_negative = 0;

        if (l->tok == '!') { is_negative = 1; lex(l); }

        expr0 (l,a);

        if (erro) return;

/*
        if (tok == ')' || tok == TOK_AND_AND) asm_pop_eax(a); // 58     pop   %eax
        else                                  g (a,0x5a); // 5a     pop   %edx
*/
        tok = l->tok;
        switch (tok) {
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
        case TOK_NOT_EQUAL:   // if (a != b) { jeq }
            lex(l);
            expr0(l,a);
            emit_cmp_int (a);
            if (tok=='>') emit_jump_jle (a,array[if_count]);
            if (tok=='<') emit_jump_jge (a,array[if_count]);
            if (tok==TOK_EQUAL_EQUAL) emit_jump_jne (a,array[if_count]); // ==
            if (tok==TOK_NOT_EQUAL) emit_jump_jeq (a,array[if_count]); // !=
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
        if (l->tok==')') break;
    }

    if (see(l)=='{') stmt (l, a); else Erro ("word(if) need start block: '{'\n");

    asm_label (a, array[if_count]);
    if_count--;
}
static void word_function (LEXER *l, ASM *a) {
    struct TFunc *func;
    char name[255], proto[255] = { '0', 0 };
    int i;

    lex(l);

    strcpy (name, l->token);

    // if exist ... return
    //
    if (FuncFind(name)!=NULL) {
        int brace = 0;

        printf ("Function exist: ... REBOBINANDO '%s'\n", name);

        while(lex(l) && l->tok !=')');

        if (see(l)=='{') { } else Erro ("word(if) need start block: '{'\n");

        while (lex(l)){
            if (l->tok == '{') brace++;
            if (l->tok == '}') brace--;
            if (brace <= 0) break;
        }

  return;
    }

    // PASSA PARAMETROS ... IMPLEMENTADA APENAS ( int ) ... AGUARDE
    //
    // O analizador de expressao vai usar esses depois...
    //
    // VEJA EM ( expr3() ):
    // ---------------------
    // Funcoes usadas:
    //     ArgumentFind();
    //     asm_push_argument();
    // ---------------------
    //
    argument_count = 0;
    while (lex(l)) {

        if (l->tok==TOK_INT) {
//            argument[argument_count].type[0] = TYPE_INT; // 0
            //strcpy (argument[argument_count].type, "int");
            lex(l);
            if (l->tok==TOK_ID) {
//                strcpy (argument[argument_count].name, l->token);
                strcat (proto, "i");
                argument_count++;
            }
        }
/*
        else if (tok==TOK_FLOAT) {
            argument[argument_count].type[0] = TYPE_FLOAT; // 1
            //strcpy (argument[argument_count].type, "int");
            tok=lex();
            if (tok==TOK_ID) {
                strcpy (argument[argument_count].name, token);
                strcat (proto, "f");
                argument_count++;
            }
        }
        else if (tok==TOK_ID) {
            argument[argument_count].type[0] = 0;
            strcpy (argument[argument_count].name, token);
            strcat (proto, "i");
            argument_count++;
        }
*/
        if (l->tok=='{') break;
    }
    if (argument_count==0) {
        proto[1] = '0';
        proto[2] = 0;
    }
    if (l->tok=='{') l->pos--; else { Erro("Word Function need char: '{'"); return; }

    is_function = 1;
//    local.count = 0;
    strcpy (func_name, name);

    // compiling to buffer ( f ):
    //
    asm_reset (asm_function);
    asm_begin (asm_function);
    //stmt (l,a); // here start from char: '{'
    stmt (l,asm_function); // here start from char: '{'
    asm_end (asm_function);

    if (erro) return;

//#ifdef USE_VM
    ASM *vm;
    int len = asm_get_len (asm_function);

    if ((vm = asm_new (len+5)) != NULL) {
        // new function:
        //
        func = (struct TFunc*) malloc (sizeof(struct TFunc));
        func->name = strdup (func_name);
        func->proto = strdup (proto);
        func->type = FUNC_TYPE_VM;
        func->len = len;// ASM_LEN (asm_function);//asm_function->len;
        // NOW: copy the buffer ( f ):
        for (i=0;i<func->len;i++) {
            vm->code[i] = asm_function->code[i];
        }
        vm->code[func->len  ] = 0;
        vm->code[func->len+1] = 0;
        vm->code[func->len+2] = 0;

//printf ("FUNCAO SIZE: %d\n", func->len);

        //-------------------------------------------
        // HACKING ... ;)
        // Resolve Recursive:
        // change 4 bytes ( func_null ) to this
        //-------------------------------------------
/*
        if (is_recursive)
        for (i=0;i<func->len;i++) {
            if (vm->code[i]==OP_CALL && *(void**)(vm->code+i+1) == func_null) {
//printf ("FUNCAO POSITION: %d\n", i);
                vm->code[i] = OP_CALLVM;      //<<<<<<<  change here  >>>>>>>
                *(void**)(vm->code+i+1) = vm; //<<<<<<<  change here  >>>>>>>
                i += 5;
            }
        }
*/
        func->code = (UCHAR*)vm;

    } else {
        is_function = is_recursive = argument_count = *func_name = 0;
        return;
    }
//#endif

    // add on top:
    func->next = Gfunc;
    Gfunc = func;

    is_function = is_recursive = argument_count = *func_name = 0;

}//: word_function ()

void lib_info (int arg) {
    switch (arg) {
    case 1: {
        TVar *v = Gvar;
        int i = 0;
        printf ("VARIABLES:\n---------------\n");
        while (v->name) {
            if (v->type==TYPE_INT)   printf ("Gvar[%d](%s) = %d\n", i, v->name, v->value.i);
            else
            if (v->type==TYPE_FLOAT) printf ("Gvar[%d](%s) = %f\n", i, v->name, v->value.f);
            v++; i++;
        }
        } break;

    case 2: {
        TFunc *fi = stdlib;
        printf ("FUNCTIONS:\n---------------\n");
        while (fi->name) {
            if(fi->proto){
                char *s=fi->proto;
                if (*s=='0') printf ("void  ");
                else
                if (*s=='i') printf ("int   ");
                else
                if (*s=='f') printf ("float ");
                else
                if (*s=='s') printf ("char  *");
                else
                if (*s=='p') printf ("void * ");
                printf ("%s (", fi->name);
                s++;
                while(*s){
                    if (*s=='i') printf ("int");
                    else
                    if (*s=='f') printf ("float");
                    s++;
                    if(*s) printf (", ");
                }
                printf (");\n");
            }
            fi++;
        }
        fi = Gfunc;
        while(fi){
            if(fi->proto) printf ("%s ", fi->proto);
            printf ("%s\n", fi->name);
            fi = fi->next;
        }

        }
        break;

    case 3:
        break;

    default:
        printf ("USAGE(%d): info(1);\n\nInfo Options:\n 1: Variables\n 2: Functions\n 3: Defines\n 4: Words\n",arg);
    }
}
int lib_add (int a, int b) {
    return a + b;
}
void lib_help (void) {
    printf ("Function: HELP ... testing ...\n");
}


int Parse (LEXER *l, ASM *a, char *text, char *name) {
    lex_set (l, text, name);
    ErroReset ();
    asm_reset (a);
    asm_begin (a);
    while (!erro && stmt(l,a)) {
        // ... compiling ...
    }
    asm_end (a);
    return erro;
}
ASM *mini_Init (UINT size) {
    static int init = 0;
    ASM *a;
    if (!init) {
        init = 1;
        if ((a            = asm_new (size))==NULL) return NULL;
        if ((asm_function = asm_new (size))==NULL) return NULL;
        return a;
    }
    return NULL;
}
int main (int argc, char **argv) {
    LEXER l;
    ASM *a;
    char *text;

    if ((a = mini_Init (ASM_DEFAULT_SIZE)) == NULL)
  return -1;

    if (argc >= 2 && (text = FileOpen (argv[1])) != NULL) {
        if (Parse(&l, a, text, argv[1])==0) {
            vm_run (a);
        }
        else printf ("ERRO:\n%s\n", ErroGet());
        free (text);

    } else {

//-------------------------------------------
// INTERACTIVE MODE:
//-------------------------------------------

        char string [1024];

        printf ("__________________________________________________________________\n\n");
        printf (" MINI Language Version: %d.%d.%d\n\n", 0, 0, 1);
        printf (" To exit type: 'quit' or 'q'\n");
        printf ("__________________________________________________________________\n\n");

        for (;;) {
            printf ("MINI > ");
            gets (string);
            if (!strcmp(string, "quit") || !strcmp(string, "q")) break;

            if (!strcmp(string, "clear") || !strcmp(string, "cls")) {
                #ifdef WIN32
                system("cls");
                #endif
                #ifdef __linux__
                system("clear");
                #endif
                continue;
            }

            if (*string==0) strcpy(string, "info(0);");

            if (Parse(&l, a, string, "stdin")==0) {
                vm_run (a);
            }
            else printf ("\n%s\n", ErroGet());
        }
    }


    printf ("Exiting With Sucess: \n");

    return 0;
}
