#include "../src/abstract-tree.h"
#include "../src/symbol-table.h"
#include "../bin/parser.tab.h"
#include <stdarg.h>

extern FILE * yyin;
extern int charno;
extern int lineno;
extern void yyerror (char *s);
extern Node * abstract_Tree;
extern char line[300];


Symbol * currentFunction; /* function we are actually looking at during tree browsing */
char * returnTypeToString[] = {
    "int",
    "char",
    "void",
    "char *",
    "int *"
};
char * returnTypeToRegister[] = { 
    "eax",
    "al",
    "",
    "rax",
    "rax" };

char * returnTypeToDataSize[] = {
    "dword",
    "byte",
    "",
    "qword",
    "qword" };

char * returnTypeToRegisterRDI[] = { 
    "edi",
    "dil",
    "",
    "rdi",
    "rdi" };

char * returnTypeToRegisterRSI[] = { 
    "esi",
    "sil",
    "",
    "rsi",
    "rsi" };

char * returnTypeToRegisterRCX[] = { 
    "ecx",
    "cl",
    "",
    "rcx",
    "rcx" };

char * returnTypeToRegisterRDX[] = { 
    "edx",
    "dl",
    "",
    "rdx",
    "rdx" };

char * returnTypeToRegisterR8[] = { 
    "r8d",
    "r8b",
    "",
    "r8",
    "r8" };

char * returnTypeToRegisterR9[] = { 
    "r9d",
    "r9b",
    "",
    "r9",
    "r9" };

char * parameterRegister[]={
    "rsi",
    "rdi",
    "rcx",
    "rdx",
    "r8",
    "r9"
};

FILE * asm_output=NULL;
int address_sum=0;
int label_Number=0;

void exitTree(Symbol ** symbol_table);
void semanticError(int c,int l,char * s,...);
void browseTree(Node * node,Symbol ** symbol_table,int scope);
void browseVarDeclList(Node * node,Symbol ** symbol_table,int scope);
int browseFonct(Node * node, Symbol ** symbol_table,int scope);
void browseInstruction(Node * node, Symbol ** symbol_table,int scope);
ReturnType browseExpression(Node * node, Symbol ** symbol_table,int scope,int isVoidAllowed);
int browseParameters(Node * node, Symbol ** symbol_table, int scope, ReturnType * arguments);
char * getDataLocation(int scope,Type type);
char * getParameterRegister(int i,ReturnType type);

int main(int argc, char **argv)
{
    if(yyparse()){
        return 1;
    }else{
        browseTree(abstract_Tree, initTable(), 0);
        deleteTree(abstract_Tree);
    }
    return 0;
}

/* Exit tree browsing */
void exitTree(Symbol ** symbol_table){
    deleteTree(abstract_Tree);
    deleteTable(symbol_table);
    exit(1);
}

/* Print semantic error with character position, line position and error to print */ 
void semanticError(int c, int l, char *s, ...)
{
    rewind(yyin);
    char error[400];
    va_list args;
    va_start(args, s);
    vsprintf(error, s, args);
    va_end(args);
    charno = c;
    lineno = l;
    int i = 1;
    while (fgets(line, sizeof line, yyin) != NULL)
    {
        if (i == l)
        {
            yyerror(error);
            break;
        }
        i++;
    }
}

/* Look at main nodes tree */
void browseTree(Node *node, Symbol **symbol_table, int scope)
{
    switch (node->kind)
    {
        case Program:
            asm_output = fopen("bin/prog.asm","wb");
            if(asm_output==NULL){
                perror("Error fopen ");
                exitTree(symbol_table);
            }
            if(FIRSTCHILD(node)->kind!=VarDeclList){
                fprintf(asm_output, "section .text\nglobal _start\nextern print_integer\nextern read_integer\n");
            }
            for (Node *child = node->firstChild; child != NULL; child = child->nextSibling)
            {
                browseTree(child, symbol_table, 0);
            }
            Symbol *m = NULL;
            if ((m = lookout(symbol_table, "main")) != NULL && m->type == Fun && (m->returnType == Int || m->returnType == VoidR) ) {
                if(m->nbArguments>0){
                    fprintf(stderr, "semantic error main function requires no arguments (%d given)\n",m->nbArguments);
                    exitTree(symbol_table);
                }
                if(m->returnType==Int){
                    fprintf(asm_output, "\n_start :\ncall _main\nmov rdi, rax\nmov rax, 60\nsyscall\n");
                }else{
                    fprintf(asm_output, "\n_start :\ncall _main\nmov rax, 60\nmov rdi, 0\nsyscall\n");
                }
            }
            else
            {
                fprintf(stderr, "semantic error undefined reference to main function\n");
                exitTree(symbol_table);
            }
            deleteTable(symbol_table);
            break;
        case VarDeclList:
            for (Node *child = node->firstChild; child != NULL; child = child->nextSibling)
            {
                browseVarDeclList(child, symbol_table, scope);
            };
            if(scope==0){
                fprintf(asm_output, "section .bss\nglobal_vars:resb %d\nsection .text\nglobal _start\nextern print_integer\nextern read_integer\n",address_sum);
            }else{
                fprintf(asm_output, "sub rsp,%d\n",address_sum );
            }
            break;
        case Function:
            address_sum=0;
            browseFonct(node->firstChild, symbol_table, scope); /* function header */
            if(SECONDCHILD(node)->firstChild!=NULL){
                if(SECONDCHILD(node)->firstChild->kind!=VarDeclList){
                    fprintf(asm_output, "sub rsp,%d\n",address_sum);
                }
            }
            for (Node *child = SECONDCHILD(node); child != NULL; child = child->nextSibling)
            {
                browseTree(child, symbol_table, scope + 1); /* function content */
            }
            fprintf(asm_output, "xor rax,rax\nmov rsp,rbp\npop rbp\nret\n");
            removeScope(symbol_table, scope + 1);
            break;
        case Instruction:
            browseInstruction(node->firstChild, symbol_table, scope);
            break;
        default:
            for (Node *child = node->firstChild; child != NULL; child = child->nextSibling)
            {
                browseTree(child, symbol_table, scope);
            }
            break;
    }
}

/* Add variables in symbol table*/
void browseVarDeclList(Node *node, Symbol **symbol_table, int scope)
{
    ReturnType firstType, type;
    if (node->kind == IntLiteral)
    {
        firstType = Int;
    }
    else if (node->kind == CharLiteral)
    {
        firstType = Char;
    }
    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling)
    {
        type = firstType;
        Node * id;
        if (child->kind == Pointer)
        {
            if (type == Int)
            {
                type = IntPointer;
            }
            else if (type == Char)
            {
                type = CharPointer;
            }
            id = FIRSTCHILD(child);
        }
        else
        {
            id = child;
        }
        if(scope!=0){
            address_sum+=addressSizeByType(type);
        }
        if (!insert(symbol_table, makeVar(id->u.identifier, type, scope, address_sum)))
        {
            semanticError(id->charno, id->lineno, "semantic error conflicting types : variable '%s'", id->u.identifier);
            exitTree(symbol_table);
        }
        if(scope==0){
            address_sum+=addressSizeByType(type);
        }
    }
}

/* Look at fonction header with return type, name and arguments  */
int browseFonct(Node *node, Symbol **symbol_table, int scope)
{
    Node *child = node->firstChild;
    ReturnType t;
    if (child->kind == IntLiteral)
    {
        t = Int;
    }
    else if (child->kind == CharLiteral)
    {
        t = Char;
    }
    else if (child->kind == Void)
    {
        t = VoidR;
    }
    Node * id;
    if (FIRSTCHILD(child)->kind == Pointer)
    {
        if (t == Int)
        {
            t = IntPointer;
        }
        else if (t == Char)
        {
            t = CharPointer;
        }
        id = FIRSTCHILD(FIRSTCHILD(child));
    }
    else
    {
        id = FIRSTCHILD(child);
    }
    fprintf(asm_output, "\n_%s :\npush rbp\nmov rbp,rsp\n",id->u.identifier);
    ReturnType *arguments = malloc(sizeof(ReturnType)); /* arguments types array */
    int nbArguments = browseParameters(SECONDCHILD(node), symbol_table, scope + 1, arguments);
    currentFunction = makeFun(id->u.identifier, t, nbArguments, arguments, scope, 0);
    if (!insert(symbol_table, currentFunction))
    {
        semanticError(id->charno, id->lineno, "semantic error conflicting types : function '%s'", id->u.identifier);
        exitTree(symbol_table);
    }
    return nbArguments;
}

/* Look at function parameters */
int browseParameters(Node *node, Symbol **symbol_table, int scope, ReturnType *arguments)
{
    Node * id;
    ReturnType type;
    int i,j,adr = 0;
    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling){
        if(child->kind==Void){
            return i;
        }
        i += 1;
    }

    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling)
    {

        if (child->kind == IntLiteral)
        {
            type = Int;
        }
        else if (child->kind == CharLiteral)
        {
            type = Char;
        }
        if (j % 10 == 0)
        {
            arguments = realloc(arguments, (j + 10) *sizeof(ReturnType));
            if (arguments == NULL)
            {
                fprintf(stderr, "Run out of memory\n");
                exitTree(symbol_table);
            }
        }
        if (FIRSTCHILD(child)->kind == Pointer)
        {
            if (type == Int)
            {
                type = IntPointer;
            }
            else if (type == Char)
            {
                type = CharPointer;
            }
            id = FIRSTCHILD(FIRSTCHILD(child));
        }
        else
        {
            id = FIRSTCHILD(child);
        }
        if(j>5){
            adr=8+8*(i);
        }else{
            address_sum+=addressSizeByType(type);
            adr=-address_sum;
            fprintf(asm_output, "mov [rbp-%d],%s\n",address_sum,getParameterRegister(j,type));
        }
        if (!insert(symbol_table, makeArg(id->u.identifier, type, scope,adr)))
        {
            semanticError(id->charno, id->lineno, "semantic error conflicting types : argument '%s'", id->u.identifier);
            exitTree(symbol_table);
        }
        arguments[j] = type;
        i -= 1;
        j += 1;
        
    }

    return j;
}

/* Look at all type of instruction */
void browseInstruction(Node *node, Symbol **symbol_table, int scope)
{
    Symbol *c = NULL;
    ReturnType t;
    int label;
    switch (node->kind)
    {
        case Assign:
            if ((c = lookout(symbol_table, FIRSTCHILD(node)->u.identifier)) != NULL)
            {
                t=browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
                if(c->returnType==Int){
                    if(t==CharPointer|| t==IntPointer){
                        semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error variable '%s' is not a pointer", FIRSTCHILD(node)->u.identifier);
                        exitTree(symbol_table);
                    }
                }
                else if(c->returnType==Char){
                    if(t==Int){
                        semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic warning assigning a int value to a char : identifier '%s'", FIRSTCHILD(node)->u.identifier);
                    }
                    if(t==CharPointer|| t==IntPointer){
                        semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error variable '%s' is not a pointer", FIRSTCHILD(node)->u.identifier);
                        exitTree(symbol_table);
                    }
                }
                fprintf(asm_output, "pop rax\nmov [%s%d],%s\n",getDataLocation(c->scope,c->type),c->address,returnTypeToRegister[c->returnType]);   
            }
            else
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error variable '%s' undeclared", FIRSTCHILD(node)->u.identifier);
                exitTree(symbol_table);
            }
            break;
        case Evaluation:
            label=label_Number+2;
            label_Number+=2;
            browseExpression(FIRSTCHILD(FIRSTCHILD(node)), symbol_table, scope, 0);
            fprintf(asm_output, "pop rax\ncmp rax, 0\nje L%d\n",label-1);
            for (Node *child = SECONDCHILD(FIRSTCHILD(node)); child != NULL; child = child->nextSibling)
            {
                browseInstruction(child, symbol_table, scope + 1);   
            }
            fprintf(asm_output, "jmp L%d\nL%d :\n",label,label-1);
            removeScope(symbol_table, scope + 1);
            if(SECONDCHILD(node)!=NULL){
                browseInstruction(SECONDCHILD(node), symbol_table, scope + 1);
            }
            fprintf(asm_output, "L%d :\n",label);
            break;
        case While:
            label=label_Number+2;
            label_Number+=2;
            fprintf(asm_output, "L%d : \n",label );
            browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output,"pop rax\ncmp rax, 0\nje L%d\n",label-1);
            for (Node *child = SECONDCHILD(node); child != NULL; child = child->nextSibling)
            {
                browseInstruction(child, symbol_table, scope + 1);
                
            }
            fprintf(asm_output, "jmp L%d\nL%d :\n",label,label-1);
            removeScope(symbol_table, scope + 1);
            break;
        case ReadC:
            if ((c = lookout(symbol_table, FIRSTCHILD(node)->u.identifier)) != NULL)
            {
                if (c->returnType != Char)
                {
                    semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error impossible read char on identifier '%s' (%s)", FIRSTCHILD(node)->u.identifier, returnTypeToString[c->returnType]);
                    exitTree(symbol_table);
                }
                if(c->scope==0 || c->type==Arg){
                    fprintf(asm_output, "mov rax,%s0\nadd rax,%d\nmov rsi,rax\nmov rax, 0\nmov rdi, 0\nmov rdx, 1\nsyscall\n",getDataLocation(scope,c->type),c->address);
                }else{
                    fprintf(asm_output, "mov rax,%s0\nsub rax,%d\nmov rsi,rax\nmov rax, 0\nmov rdi, 0\nmov rdx, 1\nsyscall\n",getDataLocation(scope,c->type),c->address);
                }
            }
            else
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error '%s' undeclared", FIRSTCHILD(node)->u.identifier);
                exitTree(symbol_table);
            }
            break;
        case ReadE:
            if ((c = lookout(symbol_table, FIRSTCHILD(node)->u.identifier)) != NULL)
            {
                if (c->returnType != Int)
                {
                    semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error impossible read int on identifier '%s' (%s)", FIRSTCHILD(node)->u.identifier, returnTypeToString[c->returnType]);
                    exitTree(symbol_table);
                }
                fprintf(asm_output, "call read_integer\nmov [%s%d],eax\n",getDataLocation(scope,c->type),c->address);
            }
            else
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error '%s' undeclared", FIRSTCHILD(node)->u.identifier);
                exitTree(symbol_table);
            }
            break;
        case Print:
            t = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            if (t == IntPointer || t == CharPointer)
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error trying to print pointer (%s)", returnTypeToString[t]);
                exitTree(symbol_table);
            }
            if(t==Char){
                fprintf(asm_output, "mov rsi,rsp\nmov rax,1\nmov rdi,0\nmov rdx,1\nsyscall\n");
            }else{
                fprintf(asm_output, "pop rax\ncall print_integer\n");
            }
            
            break;
        case Return:
            if (FIRSTCHILD(node) == NULL)
            {
                t = VoidR;
            }
            else
            {
                t = browseExpression(FIRSTCHILD(node), symbol_table, scope, 1);
                fprintf(asm_output, "pop rax\n");
            }
            if (currentFunction->returnType != t)
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic function '%s' must return (%s) but returning (%s)", currentFunction->name, returnTypeToString[currentFunction->returnType], returnTypeToString[t]);
                exitTree(symbol_table);
            }
            fprintf(asm_output, "mov rsp,rbp\npop rbp\nret\n");
            break;
        case Call:
            browseExpression(node, symbol_table, scope, 1);
            break;
        default:
            for (Node *child = node->firstChild; child != NULL; child = child->nextSibling)
            {
                browseInstruction(child, symbol_table, scope);
            }
            break;
    }
}

/* look at expressions */
ReturnType browseExpression(Node *node, Symbol **symbol_table, int scope, int isVoidAllowed)
{
    Symbol *p = NULL;
    ReturnType t1, t2;
    int label;
    switch (node->kind)
    {
        case Add:
            if (SECONDCHILD(node) != NULL)
            {
                t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
                t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
                fprintf(asm_output, "pop rcx\npop rax\nadd rax,rcx\npush rax\n");
                if (t1 == t2)
                {
                    return t1;
                }
                else if (t1 == IntPointer && t2 != CharPointer)
                {
                    return t1;
                }
                else if (t1 == CharPointer && t2 != IntPointer)
                {
                    return t1;
                }
                else if (t2 == CharPointer && t1 != IntPointer)
                {
                    return t2;
                }
                else if (t2 == IntPointer && t1 != CharPointer)
                {
                    return t2;
                }
                else if ((t1 != IntPointer && t1 != CharPointer) && (t2 != IntPointer && t2 != CharPointer))
                {
                    return Int;
                }
                else
                {
                    semanticError(node->charno, node->lineno, "semantic error addition on (%s) and (%s)", returnTypeToString[t1], returnTypeToString[t2]);
                    exitTree(symbol_table);
                }
            }
            else
            {
                t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
                return t1;
            }
        case Sub:
            if (SECONDCHILD(node) != NULL)
            {
                t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
                t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
                fprintf(asm_output, "pop rcx\npop rax\nsub rax,rcx\npush rax\n");
                if (t1 == t2)
                {
                    return t1;
                }
                else if (t1 == IntPointer && t2 != CharPointer)
                {
                    return t1;
                }
                else if (t1 == CharPointer && t2 != IntPointer)
                {
                    return t1;
                }
                else if (t2 == CharPointer && t1 != IntPointer)
                {
                    return t2;
                }
                else if (t2 == IntPointer && t1 != CharPointer)
                {
                    return t2;
                }
                else if ((t1 != IntPointer && t1 != CharPointer) && (t2 != IntPointer && t2 != CharPointer))
                {
                    return Int;
                }
                else
                {
                    semanticError(node->charno, node->lineno, "semantic error subtraction on (%s) and (%s)", returnTypeToString[t1], returnTypeToString[t2]);
                    exitTree(symbol_table);
                }
            }
            else
            {
                t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
                fprintf(asm_output, "pop rax\nmov rcx, -1\nimul rcx\npush rax\n");
                return t1;
            }
        case Mul:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\nxor rdx, rdx\nimul rcx\npush rax\n");
            if ((t1 == Int || t1 == Char) && (t2 == Int || t2 == Char))
            {
                return Int;
            }
            else
            {
                semanticError(node->charno, node->lineno, "semantic error multiplication on (%s) and (%s)", returnTypeToString[t1], returnTypeToString[t2]);
                exitTree(symbol_table);
            }
        case Div:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,0\nje L%d\njng L%d\nxor rdx,rdx\nL%d :\nidiv rcx\njmp L%d\nL%d :\nxor rdx,rdx\nL%d :\npush rax\n",label_Number+2,label_Number+1,label_Number+1,label_Number+3,label_Number+2,label_Number+3 );
            label_Number+=3;
            if ((t1 == Int || t1 == Char) && (t2 == Int || t2 == Char))
            {
                return Int;
            }
            else
            {
                semanticError(node->charno, node->lineno, "semantic error division on (%s) and (%s)", returnTypeToString[t1], returnTypeToString[t2]);
                exitTree(symbol_table);
            }
        case Mod:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,0\nje L%d\njng L%d\nxor rdx,rdx\nL%d :\nidiv rcx\njmp L%d\nL%d :\nxor rdx,rdx\nL%d :\npush rdx\n",label_Number+2,label_Number+1,label_Number+1,label_Number+3,label_Number+2,label_Number+3 );
            label_Number+=3;
            if ((t1 == Int || t1 == Char) && (t2 == Int || t2 == Char))
            {
                return Int;
            }
            else
            {
                semanticError(node->charno, node->lineno, "semantic error modulo on (%s) and (%s)", returnTypeToString[t1], returnTypeToString[t2]);
                exitTree(symbol_table);
            }
        case Call:
            if(FIRSTCHILD(node)->kind==Pointer){
                p = lookout(symbol_table, FIRSTCHILD(FIRSTCHILD(node))->u.identifier);
                if(p->returnType!=Int || p->returnType!=Char){
                    if(p->returnType==IntPointer){
                        p->returnType=Int;
                    }else{
                        p->returnType=Char;
                    }
                }else{
                    semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno,"semantic error trying to get value of function '%s' which doesn't return a pointer",FIRSTCHILD(FIRSTCHILD(node))->u.identifier);
                    exitTree(symbol_table);
                }
            }else{
                p = lookout(symbol_table, FIRSTCHILD(node)->u.identifier);
            }
            if ( p!= NULL && p->type == Fun)
            {
                int i = 0;
                for (Node *child = SECONDCHILD(node); child != NULL; child = child->nextSibling)
                {
                    if (i >= p->nbArguments)
                    {
                        semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error calling function '%s' with too many arguments (requires %d)", p->name, p->nbArguments);
                        exitTree(symbol_table);
                    }
                    t1 = browseExpression(child, symbol_table, scope, 0);
                    if(i<6){
                        fprintf(asm_output, "pop rax\nmov %s, rax\n",getParameterRegister(i,IntPointer));
                    }
                    if (t1 != p->arguments[i])
                    {
                        semanticError(child->charno, child->lineno, "semantic error calling function '%s', argument number %d must have (%s) but (%s) given", p->name, i + 1, returnTypeToString[p->arguments[i]], returnTypeToString[t1]);
                        exitTree(symbol_table);
                    }
                    ++i;
                }
                if (i < p->nbArguments)
                {
                    semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error calling function '%s' requires more arguments (requires %d)", p->name, p->nbArguments);
                    exitTree(symbol_table);
                }
                i=-1;
            }
            else
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error function '%s' undeclared", FIRSTCHILD(node)->u.identifier);
                exitTree(symbol_table);
            }
            if(FIRSTCHILD(node)->kind==Pointer){
                fprintf(asm_output, "call _%s\nmov rax,[rax]\npush rax\n",p->name);
            }else{
                fprintf(asm_output, "call _%s\npush rax\n",p->name);
            }
            if (p->returnType == VoidR && !isVoidAllowed)
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error can't operate on void function '%s'", p->name);
                exitTree(symbol_table);
            }
            return p->returnType;
        case Not:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rax\ncmp rax,0\nje L%d\nmov rax, 0\njmp L%d\nL%d:\nmov rax, 1\nL%d:\npush rax\n",label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case Equal:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,rcx\nje L%d\nmov rax, 0\njmp L%d\nL%d:\nmov rax, 1\nL%d:\npush rax\n",label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case NotEqual:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,rcx\nje L%d\nmov rax, 1\njmp L%d\nL%d:\nmov rax, 0\nL%d:\npush rax\n",label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case Greater:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,rcx\njg L%d\nmov rax, 0\njmp L%d\nL%d:\nmov rax, 1\nL%d:\npush rax\n",label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case GreaterEqual:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,rcx\njg L%d\nje L%d\nmov rax, 0\njmp L%d\nL%d:\nmov rax, 1\nL%d:\npush rax\n",label_Number+1,label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case Less:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,rcx\nje L%d\njng L%d\nmov rax, 0\njmp L%d\nL%d:\nmov rax, 1\nL%d:\npush rax\n",label_Number+2,label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case LessEqual:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rcx\npop rax\ncmp rax,rcx\njng L%d\nje L%d\nmov rax, 0\njmp L%d\nL%d:\nmov rax, 1\nL%d:\npush rax\n",label_Number+1,label_Number+1,label_Number+2,label_Number+1,label_Number+2);
            label_Number+=2;
            return Int;
        case Or:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rax\ncmp rax,0\njne L%d\n",label_Number+1);
            label_Number+=1;
            label=label_Number;
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rax\ncmp rax,0\njne L%d\nje L%d\n",label, label_Number+1);
            fprintf(asm_output, "L%d :\nmov rax,1\njmp L%d\nL%d :\nmov rax, 0\nL%d :\npush rax\n", label, label_Number+2, label_Number+1, label_Number+2);
            label_Number+=2;
            return Int;
        case And:
            t1 = browseExpression(FIRSTCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rax\ncmp rax,0\nje L%d\n",label_Number+1);
            label_Number+=1;
            label=label_Number;
            t2 = browseExpression(SECONDCHILD(node), symbol_table, scope, 0);
            fprintf(asm_output, "pop rax\ncmp rax,0\nje L%d\njne L%d\n",label, label_Number+1);
            fprintf(asm_output, "L%d :\nmov rax,0\njmp L%d\nL%d :\nmov rax, 1\nL%d :\npush rax\n", label, label_Number+2, label_Number+1, label_Number+2);
            label_Number+=2;
            return Int;
        case IntLiteral:
            fprintf(asm_output, "push %d\n",node->u.integer);
            return Int;
        case CharLiteral:
            fprintf(asm_output, "push %d\n",node->u.character);
            return Char;
        case Deref:
            if ((p = lookout(symbol_table, FIRSTCHILD(node)->u.identifier)) != NULL && (p->returnType == Int || p->returnType == Char)) {
                if(p->scope==0 || p->type==Arg){
                    fprintf(asm_output, "mov rax,%s0\nadd rax,%d\npush rax\n",getDataLocation(p->scope,p->type),p->address);
                }else{
                    fprintf(asm_output, "mov rax,%s0\nsub rax,%d\npush rax\n",getDataLocation(p->scope,p->type),p->address);
                }
            }
            else
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error can't get variable '%s' address because it is already pointing on a value", FIRSTCHILD(node)->u.identifier);
                exitTree(symbol_table);
            }
            if(p->returnType==Char){
                return CharPointer;
            }
            return IntPointer;
        case Pointer:

            if ((p = lookout(symbol_table, FIRSTCHILD(node)->u.identifier)) != NULL && (p->returnType == IntPointer || p->returnType == CharPointer)) {
                if(p->returnType==CharPointer){
                    t1=Char;
                }else{
                    t1=Int;
                }
                if(t1!=CharPointer && t1!=IntPointer){
                    fprintf(asm_output, "mov rax,[%s%d]\nmovsx rax,%s[rax]\npush rax\n",getDataLocation(p->scope,p->type),p->address,returnTypeToDataSize[t1]);
                }else{
                    fprintf(asm_output, "mov rax,[%s%d]\nmov rax,[rax]\npush rax\n",getDataLocation(p->scope,p->type),p->address);
                }
            }
            else
            {
                semanticError(FIRSTCHILD(node)->charno, FIRSTCHILD(node)->lineno, "semantic error can't get variable '%s' value because it is not pointing on any value", FIRSTCHILD(node)->u.identifier);
                exitTree(symbol_table);
            }
            return t1;
        case Identifier:
            if ((p = lookout(symbol_table, node->u.identifier)) != NULL && (p->type == Var || p->type == Arg) ) {
                if(p->returnType!=CharPointer && p->returnType!=IntPointer){
                    fprintf(asm_output, "movsx rax, %s [%s%d]\npush rax\n",returnTypeToDataSize[p->returnType],getDataLocation(p->scope,p->type),p->address);
                }else{
                    fprintf(asm_output, "mov rax, [%s%d]\npush rax\n",getDataLocation(p->scope,p->type),p->address);
                }
            }
            else
            {
                semanticError(node->charno, node->lineno, "semantic error identifier '%s' undeclared \n", node->u.identifier);
                exitTree(symbol_table);
            }
            return p->returnType;
        default:
            return VoidR;
    }
}

char * getDataLocation(int scope,Type type){
    if(scope==0){
        return "global_vars+";
    }
    if(type==Var){
        return "rbp-";
    }
    return "rbp+";
}

char * getParameterRegister(int i,ReturnType type){
    switch(i){
        case 0 :
            return returnTypeToRegisterRSI[type];
        case 1 :
            return returnTypeToRegisterRDI[type];
        case 2 :
            return returnTypeToRegisterRCX[type];
        case 3 :
            return returnTypeToRegisterRDX[type];
        case 4 :
            return returnTypeToRegisterR8[type];
        case 5 :
            return returnTypeToRegisterR9[type];
        default:
            return "";
    }
}