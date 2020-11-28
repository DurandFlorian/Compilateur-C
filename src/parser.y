%{
    /*  Projet de compilation :
        - CAHAY Florian
        - DURAND Florian
    */
    #include <stdio.h>
    #include <string.h>
    #include "../src/abstract-tree.h"

    extern int yylex();
    extern int lineno;
    extern int charno;
    extern char line[300];

    void yyerror (char *s);
    Node * abstract_Tree;
%}

%union {
    /* lexem which can contains varying elements */
    struct{
        union{
            int num;
            char comp[2];
            char character;
            char addsub;
            char type[5];
            char ident[64]; 
        }u;
        int charno;
        int lineno;
    }lexem;
    /* lexem which don't contains varying elements */
    struct {
        int charno;
        int lineno;
    }voidLexem;
    /* non-terminal */
    Node * node;
}

%token <lexem> ADDSUB CHARACTER ORDER EQ IDENT NUM TYPE
%token <voidLexem> OR AND IF WHILE RETURN VOID PRINT READC READE ELSE STAR SLASH MOD

%type <node> Prog DeclVars Declarateurs DeclFoncts DeclFonct EnTeteFonct Parametres ListTypVar Corps SuiteInstr Instr Exp TB FB M E T F LValue Arguments ListExp

%precedence ')'
%precedence ELSE

%%
Prog: DeclVars DeclFoncts   {   
                                $$ = makeNode(Program, charno, lineno);
                                addChild($$, $1);
                                addChild($$, $2);
                                abstract_Tree=$$;
                            }
    ;
DeclVars:
       DeclVars TYPE Declarateurs ';'   {
                                            if($1==NULL){
                                                $$=makeNode(VarDeclList,charno,lineno);
                                            }else{
                                                $$=$1;
                                            }
                                            Node * i = NULL;
                                            if(strcmp($2.u.type,"int")==0){
                                                i=makeNode(IntLiteral,$2.charno,$2.lineno);
                                            }else if (strcmp($2.u.type,"char")==0){
                                                i=makeNode(CharLiteral,$2.charno,$2.lineno);                                               
                                            }
                                            addChild(i,$3);
                                            addChild($$,i);
                                        }
    |                                   {
                                            $$=NULL;
                                        }                                   
    ;
Declarateurs:
       Declarateurs ',' IDENT           {
                                            $$=$1;
                                            addSibling($$,makeNodeIdentifier($3.u.ident,$3.charno,$3.lineno));
                                        }
    |  Declarateurs ',' STAR IDENT       {
                                            $$=$1;
                                            Node * p = makeNode(Pointer,$3.charno,$3.lineno);
                                            addChild(p,makeNodeIdentifier($4.u.ident,$4.charno,$4.lineno));
                                            addSibling($$,p);
                                        }
    |  IDENT                            {
                                            $$=makeNodeIdentifier($1.u.ident,$1.charno,$1.lineno);
                                        }
    |  STAR IDENT                        {
                                            $$=makeNode(Pointer,$1.charno,$1.lineno);
                                            addChild($$,makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                        }
    ;
DeclFoncts:
       DeclFoncts DeclFonct             {
                                            $$=$1;
                                            Node * f = makeNode(Function,charno,lineno);
                                            addChild(f,$2);
                                            addSibling($$,f);
                                        }
    |  DeclFonct                        {
                                            $$=makeNode(Function,charno,lineno);
                                            addChild($$,$1);
                                        }
    ;
DeclFonct:
       EnTeteFonct Corps                {
                                            $$=$1;
                                            addSibling($$,$2);
                                        }
    ;
EnTeteFonct:
       TYPE IDENT '(' Parametres ')'    {
                                            $$=makeNode(EnTeteFonct,charno,lineno);
                                            if(strcmp($1.u.type,"int")==0){
                                                addChild($$,makeNode(IntLiteral,$1.charno,$1.lineno));
                                            }else if (strcmp($1.u.type,"char")==0){
                                                addChild($$,makeNode(CharLiteral,$1.charno,$1.lineno));                                               
                                            }
                                            addChild(FIRSTCHILD($$),makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                            Node * para=makeNode(Parameters,charno,lineno);
                                            addChild(para,$4);
                                            addChild($$,para);
                                        }
    |  TYPE STAR IDENT '(' Parametres ')'{
                                            $$=makeNode(EnTeteFonct,charno,lineno);
                                            if(strcmp($1.u.type,"int")==0){
                                                addChild($$,makeNode(IntLiteral,$1.charno,$1.lineno));
                                            }else if (strcmp($1.u.type,"char")==0){
                                                addChild($$,makeNode(CharLiteral,$1.charno,$1.lineno));                                               
                                            }
                                            Node * p=makeNode(Pointer,$2.charno,$2.lineno);
                                            addChild(p,makeNodeIdentifier($3.u.ident,$3.charno,$3.lineno));
                                            addChild(FIRSTCHILD($$),p);
                                            Node * para=makeNode(Parameters,charno,lineno);
                                            addChild(para,$5);
                                            addChild($$,para);
                                        }
    |  VOID IDENT '(' Parametres ')'    {
                                            $$=makeNode(EnTeteFonct,charno,lineno);
                                            addChild($$,makeNode(Void,$1.charno,$1.lineno));
                                            addChild(FIRSTCHILD($$),makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                            Node * para=makeNode(Parameters,charno,lineno);
                                            addChild(para,$4);
                                            addChild($$,para);

                                        }
    ;
Parametres:
       VOID                             {
                                            $$=makeNode(Void,$1.charno,$1.lineno);
                                        }
    |  ListTypVar                       {
                                            $$=$1;
                                        }
    ;
ListTypVar:
       ListTypVar ',' TYPE IDENT        {
                                            $$=$1;
                                            Node * n;
                                            if(strcmp($3.u.type,"int")==0){
                                                n=makeNode(IntLiteral,$3.charno,$3.lineno);
                                            }else if (strcmp($3.u.type,"char")==0){
                                                n=makeNode(CharLiteral,$3.charno,$3.lineno);                                             
                                            }
                                            addChild(n,makeNodeIdentifier($4.u.ident,$4.charno,$4.lineno));
                                            addSibling($$,n);
                                        }
    |  ListTypVar ',' TYPE STAR IDENT    {
                                            $$=$1;
                                            Node * n;
                                            if(strcmp($3.u.type,"int")==0){
                                                n=makeNode(IntLiteral,$3.charno,$3.lineno);
                                            }else if (strcmp($3.u.type,"char")==0){
                                                n=makeNode(CharLiteral,$3.charno,$3.lineno);                                               
                                            }
                                            Node * p = makeNode(Pointer,$4.charno,$4.lineno);
                                            addChild(p,makeNodeIdentifier($5.u.ident,$5.charno,$5.lineno));
                                            addChild(n,p);
                                            addSibling($$,n);
                                        }
    |  TYPE IDENT                       {
                                            if(strcmp($1.u.type,"int")==0){
                                                $$=makeNode(IntLiteral,$1.charno,$1.lineno);
                                            }else if (strcmp($1.u.type,"char")==0){
                                                $$=makeNode(CharLiteral,$1.charno,$1.lineno);                                               
                                            }
                                            addChild($$,makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                        }
    |  TYPE STAR IDENT                   {
                                            if(strcmp($1.u.type,"int")==0){
                                                $$=makeNode(IntLiteral,$1.charno,$1.lineno);
                                            }else if (strcmp($1.u.type,"char")==0){
                                                $$=makeNode(CharLiteral,$1.charno,$1.lineno);                                               
                                            }
                                            Node * p = makeNode(Pointer,$2.charno,$2.lineno);
                                            addChild(p,makeNodeIdentifier($3.u.ident,$3.charno,$3.lineno));
                                            addChild($$,p);
                                        }
    ;
Corps: '{' DeclVars SuiteInstr '}'      {
                                            $$=makeNode(Corps,charno,lineno);
                                            if($2!=NULL){
                                                addChild($$,$2);
                                            }
                                            if($3!=NULL){
                                                addChild($$,$3);
                                            }
                                        }
    ;
SuiteInstr:
       SuiteInstr Instr                 {
                                            if($1!=NULL){
                                                $$=$1;
                                            }
                                            Node * i = makeNode(Instruction,charno,lineno);
                                            addChild(i,$2);
                                            if($1==NULL){
                                                $$=i;
                                            }else{
                                                addSibling($$,i);
                                            }
                                            
                                        }
    |                                   {
                                            $$=NULL;
                                        }                                 
     ;
Instr:
       LValue '=' Exp ';'               {
                                            
                                            $$=makeNode(Assign,charno,lineno);
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  READE '(' IDENT ')' ';'          {
                                            $$=makeNode(ReadE,$1.charno,$1.lineno);
                                            addChild($$,makeNodeIdentifier($3.u.ident,$3.charno,$3.lineno));
                                        }
    |  READC '(' IDENT ')' ';'          {
                                            $$=makeNode(ReadC,$1.charno,$1.lineno);
                                            addChild($$,makeNodeIdentifier($3.u.ident,$3.charno,$3.lineno));
                                        }
    |  PRINT '(' Exp ')' ';'            {
                                            $$=makeNode(Print,$1.charno,$1.lineno);
                                            addChild($$,$3);
                                        }
    |  IF '(' Exp ')' Instr             {
                                            $$=makeNode(Evaluation,$1.charno,$1.lineno);
                                            addChild($$,makeNode(If,$1.charno,$1.lineno)); 
                                            addChild(FIRSTCHILD($$),$3);
                                            addChild(FIRSTCHILD($$),$5);
                                        }
    |  IF '(' Exp ')' Instr ELSE Instr  {
                                            $$=makeNode(Evaluation,$1.charno,$1.lineno);
                                            addChild($$,makeNode(If,$1.charno,$1.lineno)); 
                                            addChild(FIRSTCHILD($$),$3);
                                            addChild(FIRSTCHILD($$),$5);
                                            Node * els = makeNode(Else,$6.charno,$6.lineno);
                                            addChild(els,$7);
                                            addChild($$,els);
                                        }
    |  WHILE '(' Exp ')' Instr          {
                                            $$=makeNode(While,$1.charno,$1.lineno);
                                            addChild($$,$3);
                                            addChild($$,$5);
                                        }
    |  IDENT '(' Arguments  ')' ';'     {
                                            $$=makeNode(Call,charno,lineno);
                                            addChild($$,makeNodeIdentifier($1.u.ident,$1.charno,$1.lineno));
                                            addChild($$,$3);
                                        }
    |  RETURN Exp ';'                   {
                                            $$=makeNode(Return,$1.charno,$1.lineno);
                                            addChild($$,$2);
                                        }
    |  RETURN ';'                       {
                                            $$=makeNode(Return,$1.charno,$1.lineno);
                                        }
    |  '{' SuiteInstr '}'               {
                                            $$=$2;
                                        }
    |  ';'                              {
                                            $$=NULL;
                                        }
    ;
Exp :  Exp OR TB                        {
                                            $$=makeNode(Or,$2.charno,$2.lineno);
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  TB                               {
                                            $$=$1;
                                        }
    ;
TB  :  TB AND FB                        {
                                            $$=makeNode(And,$2.charno,$2.lineno);
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  FB                               {
                                            $$=$1;
                                        }
    ;
FB  :  FB EQ M                          {
                                            if(strcmp($2.u.comp,"==")==0){
                                                $$=makeNode(Equal,$2.charno,$2.lineno);
                                            }else if(strcmp($2.u.comp,"!=")==0){
                                                $$=makeNode(NotEqual,$2.charno,$2.lineno);
                                            }
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  M                                {
                                            $$=$1;
                                        }
    ;
M   :  M ORDER E                        {
                                            if(strcmp($2.u.comp,">")==0){
                                                $$=makeNode(Greater,$2.charno,$2.lineno);
                                            }else if(strcmp($2.u.comp,"<")==0){
                                                $$=makeNode(Less,$2.charno,$2.lineno);
                                            }else if(strcmp($2.u.comp,">=")==0){
                                                $$=makeNode(GreaterEqual,$2.charno,$2.lineno);
                                            }else if(strcmp($2.u.comp,"<=")==0){
                                                $$=makeNode(LessEqual,$2.charno,$2.lineno);
                                            }
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }   
    |  E                                {
                                            $$=$1;
                                        }
    ;
E   :  E ADDSUB T                       {
                                            if($2.u.addsub=='+'){
                                                $$=makeNode(Add,$2.charno,$2.lineno);
                                            }else if($2.u.addsub=='-'){
                                                $$=makeNode(Sub,$2.charno,$2.lineno);
                                            }
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  T                                {
                                            $$=$1;
                                        }
    ;    
T   :  T STAR F                          {
                                            $$=makeNode(Mul,$2.charno,$2.lineno);
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  T SLASH F                          {
                                            $$=makeNode(Div,$2.charno,$2.lineno);
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  T MOD F                          {
                                            $$=makeNode(Mod,$2.charno,$2.lineno);
                                            addChild($$,$1);
                                            addChild($$,$3);
                                        }
    |  F                                {
                                            $$=$1;
                                        }
    ;
F   :  ADDSUB F                         {
                                            if($1.u.addsub=='+'){
                                                $$=makeNode(Add,$1.charno,$1.lineno);
                                            }else if($1.u.addsub=='-'){
                                                $$=makeNode(Sub,$1.charno,$1.lineno);
                                            }
                                            addChild($$,$2);
                                        }
    |  '!' F                            {
                                            $$=makeNode(Not,charno,lineno);
                                            addChild($$,$2);
                                        }
    |  '&' IDENT                        {
                                            $$=makeNode(Deref,charno,lineno);
                                            addChild($$,makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                        }
    |  '(' Exp ')'                      {
                                            $$=$2;
                                        }
    |  NUM                              {
                                            $$=makeNode(IntLiteral,$1.charno,$1.lineno);
                                            $$->u.integer=$1.u.num;
                                        }
    |  CHARACTER                        {
                                            $$=makeNode(CharLiteral,$1.charno,$1.lineno);
                                            $$->u.character=$1.u.character;
                                        }
    |  LValue                           {
                                            $$=$1;
                                        }
    |  IDENT '(' Arguments  ')'         {
                                            $$=makeNode(Call,charno,lineno);
                                            addChild($$,makeNodeIdentifier($1.u.ident,$1.charno,$1.lineno));
                                            addChild($$,$3);
                                        }
    |  STAR IDENT '(' Arguments  ')'     {
                                            $$=makeNode(Call,charno,lineno);
                                            addChild($$,makeNode(Pointer,charno,lineno));
                                            addChild(FIRSTCHILD($$),makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                            addChild($$,$4);
                                        }
    ;
LValue:
       IDENT                            {
                                            $$=makeNodeIdentifier($1.u.ident,$1.charno,$1.lineno);
                                        }
    |  STAR IDENT                        {
                                            $$ = makeNode(Pointer,charno,lineno);
                                            addChild($$,makeNodeIdentifier($2.u.ident,$2.charno,$2.lineno));
                                        }
    ;
Arguments:
       ListExp                          {
                                            $$=$1;
                                        }
    |                                   {
                                            $$=NULL;
                                        } 
    ;
ListExp:
       ListExp ',' Exp                  {
                                            $$=$1;
                                            addSibling($$,$3);
                                        }
    |  Exp                              {
                                            $$=$1;
                                        }
    ;
%%

/* print error specifying line and first character positions */
void yyerror(char *s)
{
    fprintf(stderr, "%s near line %d at char %d :\n%s\033[31;1m%*c\033[0m\n", s, lineno, charno, line, charno, '^');
}
