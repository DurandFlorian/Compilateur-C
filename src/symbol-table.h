#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ST_SIZE 1021
#define ID_SIZE 64

typedef enum
{
	Var,
	Fun,
	Arg
}
Type;

typedef enum
{
	Int,
	Char,
	VoidR,
	CharPointer,
	IntPointer,
}
ReturnType;

typedef struct Symbol
{
	char name[64];
	Type type;
	ReturnType returnType;
	int nbArguments;
	ReturnType * arguments;
	int scope;
	int address;
	struct Symbol * next;
}
Symbol;

int hash(char *name);

int insert(Symbol **symbole_table, Symbol *s);

Symbol* lookout(Symbol **symbole_table, char *name);

void removeScope(Symbol **symbole_table, int scope);

void printSymbol(Symbol *s);

void printTable(Symbol **symbole_table);

Symbol **initTable();

Symbol* makeVar(char *name, ReturnType returnType, int scope, int address);

Symbol* makeFun(char *name, ReturnType returnType, int nbArguments, ReturnType *arguments, int scope, int address);

Symbol* makeArg(char *name, ReturnType returnType, int scope, int address);

void deleteTable(Symbol **symbole_table);

int addressSizeByType(ReturnType type);