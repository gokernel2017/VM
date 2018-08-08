//-------------------------------------------------------------------
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
// COMPILE:
//   make clean
//   make
//
// FILE:
//   mini.c | mini001.c
//
//-------------------------------------------------------------------
//
#include "src/vm.h"

#define MINI_VERSION        001
//
#define LEXER_NAME_SIZE     255
#define LEXER_TOKEN_SIZE    1024
#define STR_ERRO_SIZE       1024

enum {
    TOK_INT = 255,
    TOK_FLOAT,
    //-------------
    TOK_ID,
    TOK_NUMBER,
    TOK_STRING
};

typedef struct LEXER  LEXER;
typedef struct TFunc  TFunc;

struct LEXER {
    char  *text;
    char  name  [LEXER_NAME_SIZE];
    char  token [LEXER_TOKEN_SIZE];
    //
    int   pos; // text [ pos ]
    int   line;
    int   tok;
};
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

int erro, is_function, main_variable_type, var_type;

//-----------------------------------------------
//-----------------  PROTOTYPES  ----------------
//-----------------------------------------------
//
static void word_int (LEXER *l, ASM *a);
//
void Erro (char *s);
static int  expr0 (LEXER *l, ASM *a);
static void expr1 (LEXER *l, ASM *a);
static void expr2 (LEXER *l, ASM *a);
static void expr3 (LEXER *l, ASM *a);
static void atom  (LEXER *l, ASM *a);


//
// New Lexer Implementation:
//
//   return 1 or 0
//
int lex (LEXER *l) {
    register int c;
    register char *temp; // pointer buffer of: lexer->token[]

    temp = l->token;
    *temp = 0;

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
            *temp++ = c;
        }
        *temp = 0;

        if (c=='"') l->pos++; else Erro("String erro");

        l->tok = TOK_STRING;

        return 1;
    }

    //##########  WORD, IDENTIFIER ...  #########
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        for (;;) {
            c = l->text[l->pos];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                l->pos++;
                *temp++ = c;
            } else break;
        }
        *temp = 0;

        if (!strcmp(l->token, "int"))    { l->tok = TOK_INT;    return 1; }
        if (!strcmp(l->token, "float"))  { l->tok = TOK_FLOAT;  return 1; }

        l->tok = TOK_ID;

        return 1;
    }

    //#################  NUMBER  ################
    if (c >= '0' && c <= '9') {
        for (;;) {
            c = l->text[l->pos];
            if ((c >= '0' && c <= '9') || c == '.') {
                l->pos++;
                *temp++ = c;
            } else break;
        }
        *temp = 0;

        l->tok = TOK_NUMBER;

        return 1;
    }

    //##########  REMOVE COMMENTS  ##########
    if (c == '/'){
        if (l->text[l->pos+1] == '*') { // comment block
/*
            str += 2;
            do {
                while (*str && *str != '*') {
                    if (*str == '\n') line++; //<<<<<<<<<<  line++  >>>>>>>>>>
                    str++;
                }
                str++;
            } while (*str && *str != '/');
            if (*str=='/') str++;
            else    asm_Erro ("BLOCK COMMENT ERRO: '/'");
            goto top;
*/
        } else if (l->text[l->pos+1] == '/') { // comment line
            l->pos += 2;
            while ((c=l->text[l->pos]) && (c != '\n') && (c != '\r'))
                l->pos++;
            goto top;
        }
    }

    *temp++ = c;
    *temp++ = 0;
    l->tok = c;
    l->pos++;
    return 1;
}

void lex_set (LEXER *l, char *text, char *name) {
    if (l && text) {
        l->pos = 0;
        l->line = 1;
        l->text = text;
        if (name)
            strcpy (l->name, name);
    }
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
// atom:
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

//
// function_name (a, b, c + d);
//
void execute_call (LEXER *l, ASM *a, TFunc *func) {
    int count = 0, pos = 0, size = 4;
    int return_type = TYPE_INT;

    // get next: '('
    lex(l);
    if (l->tok != '(') {
        char buf[100];
        sprintf (buf, "ERRO LINE(%d) | Function(%s) need char: '('\n", l->line, func->name);
        Erro (buf);
        return;
    }

    while (lex(l)) {
        expr0 (l,a);
        pos += size;
        if (count++ > 15) break;
        if (l->tok == ')' || l->tok == ';') break;
    }
    if (func->proto) {
        if (func->proto[0] == '0') return_type = TYPE_NO_RETURN;
        if (func->proto[0] == 'f') return_type = TYPE_FLOAT;
    }
    emit_call (a, (void*)func->code, (UCHAR)count, return_type);
}

void expression (LEXER *l, ASM *a) {
    char buf[255];

    if (l->tok == TOK_ID || l->tok == TOK_NUMBER) {
        TFunc *fi;

        main_variable_type = var_type = TYPE_INT; // 0

        // call a function without return:
        // function_name (...);
        if ((fi = FuncFind(l->token)) != NULL) {
            execute_call (l, a, fi);
      return;
        }

        // expression type:
        // 10 * 20 + 3 * 5;
        if (expr0(l,a) == -1) {
            emit_pop_eax (a);
            emit_print_eax (a,TYPE_INT);
        }

    } else {
        sprintf (buf, "EXPRESSION ERRO LINE(%d) - Ilegar Word (%s)\n", l->line, l->token);
        Erro (buf);
    }
}

int stmt (LEXER *l, ASM *a) {
    lex(l);
    switch (l->tok) {
    case TOK_INT: word_int    (l,a); return 1;
    default:      expression  (l,a); return 1;
    case ';':
        return 1;
    case 0: return 0;
    }
    return 1;
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

int main (int argc, char **argv) {
    FILE *fp;
    ASM *a;
    LEXER lexer;

    if ((a = asm_new (ASM_DEFAULT_SIZE)) == NULL)
  return -1;

    if (argc >= 2 && (fp=fopen(argv[1], "rb"))!=NULL) {
        int i = 0, c;
#define PROG_SIZE  50000
        char prog [PROG_SIZE];
        while ((c=getc(fp))!=EOF) prog[i++] = c; // store prog[];
        prog[i] = 0;
        fclose (fp);
        if (Parse(&lexer, a, prog, argv[1])==0) {
            vm_run (a);
        }
        else printf ("ERRO:\n%s\n", ErroGet());

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

            if (Parse(&lexer, a, string, "stdin")==0)
            {
                vm_run (a);
            }
            else printf ("\n%s\n", ErroGet());
        }
    }


    printf ("Exiting With Sucess: \n");

    return 0;
}
