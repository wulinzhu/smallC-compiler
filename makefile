all:lex.yy.c smallc.tab.c scc 
lex.yy.c:
	flex smallc.lex
smallc.tab.c:
	bison smallc.y
scc:
	gcc -ggdb3 smallc.tab.c -ly -ll -o scc

