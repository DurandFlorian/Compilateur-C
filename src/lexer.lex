%{
/* 	Projet de compilation :
		- CAHAY Florian
		- DURAND Florian
*/

	#include "../src/abstract-tree.h"
	#include "../bin/parser.tab.h"
	#include <string.h>
	
	int lineno = 1;
	int charno = 0;
	char line[300];
	void read_char();
	void new_line();
	void set_line();
	void updateLexem();
	void updateVoidLexem();
%}

letter [a-zA-Z]
digit [0-9]

%option nounput
%option noinput

%x COMMENT PROG

%%
(.*\n|.*) 									{ set_line(); BEGIN PROG; yyless(0); }
<PROG>\/\* 									{ read_char(); BEGIN COMMENT; }
<COMMENT>. 									{ read_char(); set_line(); }
<COMMENT>\n 								{ charno = 0; new_line(); } 
<COMMENT>\*\/ 								{ read_char(); set_line(); BEGIN INITIAL; }
<PROG>if 									{ updateVoidLexem(); read_char(); return IF; }
<PROG>else 									{ updateVoidLexem(); read_char(); return ELSE; }
<PROG>reade 								{ updateVoidLexem(); read_char(); return READE; }
<PROG>readc 								{ updateVoidLexem(); read_char(); return READC; }
<PROG>return 								{ updateVoidLexem(); read_char(); return RETURN; }
<PROG>void 									{ updateVoidLexem(); read_char(); return VOID; }
<PROG>print 								{ updateVoidLexem(); read_char(); return PRINT; }
<PROG>\|\| 									{ updateVoidLexem(); read_char(); return OR; }
<PROG>&& 									{ updateVoidLexem(); read_char(); return AND; }
<PROG>(\+|-) 								{ updateLexem(); read_char(); yylval.lexem.u.addsub=yytext[0]; return ADDSUB; }
<PROG>(<|<=|>|>=) 							{ updateLexem(); read_char(); strcpy(yylval.lexem.u.comp,yytext); return ORDER; }
<PROG>(==|!=) 								{ updateLexem(); read_char(); strcpy(yylval.lexem.u.comp,yytext); return EQ; }
<PROG>(int|char) 							{ updateLexem(); read_char(); strcpy(yylval.lexem.u.type,yytext); return TYPE; }
<PROG>while 								{ updateVoidLexem(); read_char(); return WHILE; }
<PROG>\'\\n\'								{ updateLexem(); read_char(); yylval.lexem.u.character='\n'; return CHARACTER; }
<PROG>\'\\t\'								{ updateLexem(); read_char(); yylval.lexem.u.character='\t'; return CHARACTER; }
<PROG>\'\\?.\'									{ updateLexem(); read_char(); yylval.lexem.u.character=yytext[1]; return CHARACTER; }
<PROG>{letter}({letter}|{digit}|_){0,63} 	{ updateLexem(); read_char(); strcpy(yylval.lexem.u.ident,yytext); return IDENT; }
<PROG>{digit}+ 								{ updateLexem(); read_char(); sscanf(yytext, "%d", &(yylval.lexem.u.num)); return NUM; }
<PROG>[ ] 									{ read_char(); }
<PROG>\t 									{ charno += 8; }
<PROG>\n 									{ charno = 0; new_line(); BEGIN INITIAL; }
<PROG>\*									{ updateVoidLexem(); read_char(); return STAR; } 
<PROG>\/									{ updateVoidLexem(); read_char(); return SLASH; } 
<PROG>%									    { updateVoidLexem(); read_char(); return MOD; } 
<PROG>. 									{ read_char(); return yytext[0]; }
<PROG><<EOF>> 								{ return 0; }
%%

/* Add lexem read size to charno */
void read_char(){
	charno += strlen(yytext);
}

/* Update lineno value and reset line */
void new_line(){
	++lineno;
	strcpy(line,"");
}

/* Concat yytext with line if line doesn't exceed 300 char */
void set_line(){
	if(strlen(line)>300){
		strcat(line,"Line exceeds 300 characters");
	}else{
		strcat(line,yytext);
	}
}

/* update charno and lineno for lexem struct*/
void updateLexem(){
	yylval.lexem.charno=charno+1;
	yylval.lexem.lineno=lineno;
}

/* update charno and lineno for voidLexem struct */
void updateVoidLexem(){
	yylval.voidLexem.charno=charno+1;
	yylval.voidLexem.lineno=lineno;
}