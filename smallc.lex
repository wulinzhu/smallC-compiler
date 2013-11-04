/*regular expression*/
%{
#include <string.h>
#define DEFAULT_POS_PROC(x) \
	yylloc->first_column = yylloc->last_column;\
	yylloc->last_column += x;
%}
%option bison-bridge bison-locations yylineno
delim		[" "]
ws			{delim}+
integer		(0x|0X)[0-9a-fA-F]+|[0-9]+
id			[a-zA-Z_][0-9a-zA-Z_]*
%%

ws		{ DEFAULT_POS_PROC(yyleng);}
"\t"	{ DEFAULT_POS_PROC(4); }
"\n"	{ yylloc->first_column = yylloc->last_column; yylloc->first_line = yylloc->last_line++; yylloc->last_column=1; }
;		{ DEFAULT_POS_PROC(1); return (SEMI);}
,		{ DEFAULT_POS_PROC(1); yylval->type=COMMA; yylval->id=NULL; yylval->value=0; return (COMMA);}
int		{ DEFAULT_POS_PROC(3); return (TYPE);}
"("		{ DEFAULT_POS_PROC(1); return (LP);}
")"		{ DEFAULT_POS_PROC(1); return (RP);}
"["		{ DEFAULT_POS_PROC(1); return (LB);}
"]"		{ DEFAULT_POS_PROC(1); return (RB);}
"{"		{ DEFAULT_POS_PROC(1); return (LC);}
"}"		{ DEFAULT_POS_PROC(1); return (RC);}
struct		{ DEFAULT_POS_PROC(6); return (STRUCT);}
if		{ DEFAULT_POS_PROC(2); return (IF);}
else		{ DEFAULT_POS_PROC(4); return (ELSE);}
return		{ DEFAULT_POS_PROC(6); return (RETURN);}
break		{ DEFAULT_POS_PROC(5); return (BREAK);}
continue	{ DEFAULT_POS_PROC(8); return (CONT);}
for		{ DEFAULT_POS_PROC(3); return (FOR);}
write	{ DEFAULT_POS_PROC(5); return (MWRITE);}
read	{ DEFAULT_POS_PROC(4); return (MREAD);}
{integer}	{ DEFAULT_POS_PROC(yyleng); yylval->id=strdup(yytext); return (INT);}
{id}		{ DEFAULT_POS_PROC(yyleng); yylval->id=strdup(yytext); return (ID);}
"."		{ DEFAULT_POS_PROC(1); return (DOT);}
"-"		{ DEFAULT_POS_PROC(1); return (MINUS);}
"!"		{ DEFAULT_POS_PROC(1); return (LOGNOT);}
"++"		{ DEFAULT_POS_PROC(2); return (INC);}
"--"		{ DEFAULT_POS_PROC(2); return (DEC);}
"~"		{ DEFAULT_POS_PROC(1); return (BITNOT);}
"*"		{ DEFAULT_POS_PROC(1); return (KMUL);}
"/"		{ DEFAULT_POS_PROC(1); return (KDIV);}
"%"		{ DEFAULT_POS_PROC(1); return (MOD);}
"+"		{ DEFAULT_POS_PROC(1); return (PLUS);}
"<<"		{ DEFAULT_POS_PROC(2); return (SHL);}
">>"		{ DEFAULT_POS_PROC(2); return (SHR);}
">"		{ DEFAULT_POS_PROC(1); return (GREAT);}
">="		{ DEFAULT_POS_PROC(2); return (GREATEQ);}
"<"		{ DEFAULT_POS_PROC(1); return (LESS);}
"<="		{ DEFAULT_POS_PROC(2); return (LESSEQ);}
"=="		{ DEFAULT_POS_PROC(2); return (EQUAL);}
"!="		{ DEFAULT_POS_PROC(2); return (NOEQUAL);}
"&"		{ DEFAULT_POS_PROC(1); return (BITAND);}
"^"		{ DEFAULT_POS_PROC(1); return (BITXOR);}
"|"		{ DEFAULT_POS_PROC(1); return (BITOR);}
"&&"		{ DEFAULT_POS_PROC(2); return (LOGAND);}
"||"		{ DEFAULT_POS_PROC(2); return (LOGOR);}
"="		{ DEFAULT_POS_PROC(1); return (ASSIGN);}
"+="		{ DEFAULT_POS_PROC(2); return (PLUSASS);}
"-="		{ DEFAULT_POS_PROC(2); return (MINUSASS);}
"*="		{ DEFAULT_POS_PROC(2); return (MULASS);}
"/="		{ DEFAULT_POS_PROC(2); return (DIVASS);}
"&="		{ DEFAULT_POS_PROC(2); return (ANDASS);}
"^="		{ DEFAULT_POS_PROC(2); return (XORASS);}
"|="		{ DEFAULT_POS_PROC(2); return (ORASS);}
"<<="		{ DEFAULT_POS_PROC(3); return (SHLASS);}
">>="		{ DEFAULT_POS_PROC(3); return (SHRASS);}	
%%
