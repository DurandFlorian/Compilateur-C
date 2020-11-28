#include "symbol-table.h"

char *StringFromType[] = { 
	"Var",
	"Fun",
	"Arg" };

char *StringFromReturnType[] = { "Int",
	"Char",
	"VoidR",
	"CharPointer",
	"IntPointer" };

int hash(char *name)
{
	int value = 0;
	int i;
	for (i = 0; i < ID_SIZE && name[i] != '\0'; ++i)
	{
		value += name[i];
	}
	return value % ST_SIZE;
}

int insert(Symbol **symbole_table, Symbol *s)
{
	int h = hash(s->name);
	if (symbole_table[h] == NULL)
	{
		symbole_table[h] = s;

		return 1;
	}
	Symbol *last = symbole_table[h];
	while (last->next != NULL)
	{
		if (strcmp(last->name, s->name) == 0 && s->scope == last->scope)
		{
			return 0;
		}
		last = last->next;
	}

	if (strcmp(last->name, s->name) == 0 && s->scope == last->scope)
	{
		return 0;
	}

	last->next = s;

	return 1;
}

Symbol* lookout(Symbol **symbole_table, char *name)
{
	int h = hash(name);
	if (symbole_table[h] == NULL)
	{
		return NULL;
	}
	Symbol *out = symbole_table[h];
	Symbol *last = symbole_table[h];
	while (last->next != NULL)
	{
		if (strcmp(last->name, name) == 0 && last->scope >= out->scope)
		{
			out = last;
		}
		last = last->next;
	}
	if (strcmp(last->name, name) == 0 && last->scope >= out->scope)
	{
		out = last;
	}
	return out;
}

Symbol* removeSymbol(Symbol *s, int scope)
{
	if (s == NULL)
		return NULL;
	if (s->scope >= scope)
	{
		Symbol * tempS;
		tempS = s->next;
		free(s);
		s = NULL;
		return removeSymbol(tempS, scope);
	}
	s->next = removeSymbol(s->next, scope);
	return s;
}

void removeScope(Symbol **symbole_table, int scope)
{
	int i;
	for (i = 0; i < ST_SIZE; ++i)
	{
		symbole_table[i] = removeSymbol(symbole_table[i], scope);
	}
}

void printSymbol(Symbol *s)
{
	if (s == NULL)
	{
		printf("NULL\n");
		return;
	}

	printf("%s | %s | %s | %d |[", s->name, StringFromType[s->type], StringFromReturnType[s->returnType], s->nbArguments);
	int i;
	for (i = 0; i < s->nbArguments; ++i)
	{
		if (i > 0)
		{
			printf(",");
		}
		printf("%s", StringFromReturnType[s->arguments[i]]);
	}
	printf("] ");
	printf("| %d | %d \n", s->scope, s->address);
}

void printTable(Symbol **symbole_table)
{
	int i;
	for (i = 0; i < ST_SIZE; ++i)
	{
		Symbol *s = symbole_table[i];
		if (s != NULL)
		{
			printf("%d : ", i);
			printSymbol(s);
		}
		else
		{
			continue;
		}
		while (s->next != NULL)
		{
			s = s->next;
			printf("%d : ", i);
			printSymbol(s);
		}
	}
}

Symbol **initTable()
{
	Symbol **symbole_table = malloc(ST_SIZE* sizeof(Symbol));
	if (!symbole_table)
	{
		printf("Run out of memory\n");
		exit(1);
	}
	return symbole_table;
}

Symbol* makeVar(char *name, ReturnType returnType, int scope, int address)
{
	Symbol *s = malloc(sizeof(Symbol));
	if (!s)
	{
		printf("Run out of memory\n");
		exit(1);
	}
	strcpy(s->name, name);
	s->type = Var;
	s->returnType = returnType;
	s->nbArguments = 0;
	s->arguments = NULL;
	s->scope = scope;
	s->address = address;
	s->next = NULL;
	return s;
}

Symbol* makeArg(char *name, ReturnType returnType, int scope, int address)
{
	Symbol *s = malloc(sizeof(Symbol));
	if (!s)
	{
		printf("Run out of memory\n");
		exit(1);
	}
	strcpy(s->name, name);
	s->type = Arg;
	s->returnType = returnType;
	s->nbArguments = 0;
	s->arguments = NULL;
	s->scope = scope;
	s->address = address;
	s->next = NULL;
	return s;
}

Symbol* makeFun(char *name, ReturnType returnType, int nbArguments, ReturnType *arguments, int scope, int address)
{
	Symbol *s = malloc(sizeof(Symbol));
	if (!s)
	{
		printf("Run out of memory\n");
		exit(1);
	}
	strcpy(s->name, name);
	s->type = Fun;
	s->returnType = returnType;
	s->nbArguments = nbArguments;
	s->arguments = arguments;
	s->scope = scope;
	s->address = address;
	s->next = NULL;
	return s;
}

void deleteTable(Symbol **symbole_table)
{
	int i;
	for (i = 0; i < ST_SIZE; ++i)
	{
		Symbol *s = symbole_table[i];
		while (s != NULL)
		{
			Symbol *sfree = s;
			s = s->next;
			free(sfree);
			sfree = NULL;
		}
	}
	free(symbole_table);
	symbole_table = NULL;
}

int addressSizeByType(ReturnType type){
	switch(type){
		case Int:
			return 4;
		case Char:
			return 1;
		case CharPointer:
			return 8;
		case IntPointer:
			return 8;
		default:
			return 0;
	}

}