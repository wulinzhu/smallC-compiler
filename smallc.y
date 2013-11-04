%code top{
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "def.h"
#define YYLTYPE YYLTYPE
typedef struct YYLTYPE
{
	int first_line;
	int first_column;
	int last_line;
	int last_column;
}YYLTYPE;

//current symbol table, the initial one is the global symbol table
struct env* top;	

//temp variable ; comman use
struct env* tmpEnv;
struct functionDef* tmpFun;

//function definition list
struct functionDef* functionList;

//function activation list
struct funRecord* callList;

//struct definition list
struct structField* structList;

//error number
int errorNum;

//nested depth of for
int numFor;

YYLTYPE yylloc;
int db[10];

#define YYSTYPE YYSTYPE
typedef struct YYSTYPE
{
	char* id;
	int value;
	int type;
	struct structField* defLoc;

	int code;
	struct relList* trueList;
	struct relList* falseList;
}YYSTYPE;

int default_exp_check(int t1,int l,int c);

}

%code {
#define YYLEX_PARAM &yylval,&yylloc
}
%token	SEMI
%token	COMMA
%token	MREAD MWRITE
%token	TYPE STRUCT
%token	IF ELSE
%token	RETURN
%token	BREAK CONT
%token	FOR
%token	INT
%token	ID
%token	LP RP LB RB LC RC
%right	ASSIGN PLUSASS MINUSASS MULASS DIVASS ANDASS XORASS ORASS SHLASS SHRASS
%left	LOGOR
%left	LOGAND
%left	BITOR
%left	BITXOR
%left	BITAND
%left	EQUAL NOEQUAL
%left	GREAT GREATEQ LESS LESSEQ
%left	SHL SHR
%left	PLUS MINUS
%left	KMUL KDIV MOD
%right	UMINUS LOGNOT INC DEC BITNOT
%left	DOT

%%


program	: extdefs {
						//check if there is a main function
						if(findFunction(functionList,"main") == NULL)
							yyerror(errorMsg[FUN_MAIN]);
					}
	;

extdefs	: extdef extdefs	{ } 
	| /* empty */	{}
	;

extdef	: spec extvars SEMI{/*global definition: done in dec*/}
	| spec func {
					if($<type>1 != VARIABLE_INT)			//declare function
						yyerror(errorMsg[FUN_RET]);
					tmpFun->start=nextInstruction();		//add definition to function list
					tmpFun->numOfPara=$<value>2;
					insertFunction(functionList,tmpFun);
					$<id>1=strdup(tmpFun->name);
	} stmtblock	{
					tmpEnv = top;						// setup previous environment
					top = top->prev;
					top->next = NULL;
					tmpFun=findFunction(functionList,$<id>1);
					newCode(MEND,tmpFun->start,0,tmpFun);	// add end symbol of function 
	}
	;

extvars	: dec					{}
	| dec COMMA extvars			{}
	| /*empty */				{}
	;

spec	: TYPE {
				$<id>$ = NULL; 
				$<type>$ = VARIABLE_INT; //set type to variable
				$<value>$ = 0;
				$<defLoc>$ = NULL;
		  }	
	| stspec {
				$<id>$ = NULL; 
				$<type>$ = VARIABLE_STRUCT; //set type to variable
				$<value>$ = $<value>1;
				$<defLoc>$ = $<defLoc>1;
			}
	;

stspec	: STRUCT opttag LC {
				top->next = malloc(sizeof(struct env));				// create new environment of struct
				initSymbolTable(top->next,top->envDepth+1);		
				top->next->prev = top;
				top = top->next;
				top->inStruct=1;
		  } defs RC			{
							top = top->prev;				//					first check element of this
							tmpEnv = top->next;				//					struct type, then add the defintion
							$<id>$ = NULL;						//				to the struct definition list,
							$<type>$ = VARIABLE_STRUCT;		//					then return to the previous 
							$<value>$ = -1;							//			environment
							assert(structList != NULL);				//
							if(top->envDepth != 0)				//
								yyerror(errorMsg[STR_TYP]);				//
							if(findStructType(structList,$<id>2) != NULL)		//
								yyerror(errorMsg[VAR_RED]);				//
							else											//
							{
								struct structField* tmpStr = malloc(sizeof(struct structField));
								tmpStr->typeName = $<id>2;
								tmpStr->field = malloc(sizeof(char*)*tmpEnv->num);
								tmpStr->varNum = tmpEnv->num;
								int j=0;
								fillStructField(tmpStr->field,tmpEnv->root,&j);
								insertStructType(structList,tmpStr);
								$<defLoc>$ = tmpStr;
								free(tmpEnv);
								top->next=NULL;
							}
		  }
	| STRUCT ID	{
					$<id>$ = NULL;
					$<value>$ = -1;
					tmpEnv=top;
					$<defLoc>$ = findStructType(structList,$<id>2);		//check whether specific type have been defined
					$<type>$ = VARIABLE_STRUCT;
					if($<defLoc>$ == NULL)
						yyerror(errorMsg[TYP_UNDEF]);
				}
	;

opttag	: ID {
				$<id>$=strdup($<id>1);
				$<type>$=0;
				$<value>$=0;
			}
	| /* empty */	{
						$<id>$=NULL;
						$<type>$=1;
						$<value>$=0;
					}
	;

var	: ID vartail		{	
							$<id>$ = $<id>1; 
							$<type>$ = $<type>2;
							$<value>$ = $<value>2;
						}
	;

vartail:vartail LB INT RB 		{	
							$<id>$ = NULL; 
							$<type>$ = ARRAY_INT;
							$<value>$ = $<value>1+1; 
							db[$<value>1] = toInt($<id>3,@3.first_line,@3.first_column); 
							if(db[$<value>1]==0)
								yyerror(errorMsg[TYP_UNDEF]);
						}
	|					{
							$<id>$ = NULL; 
							$<type>$ = VARIABLE_INT; 
							$<value>$ = 0; 
						}
	;

func	: ID LP{
					tmpFun = malloc(sizeof(struct functionDef));
					tmpFun->name = strdup($<id>1);
					tmpFun->para = NULL;
					tmpFun->next = NULL;							//generate IR code
					tmpFun->numOfPara=0;							//create new symbol table for function
					top->next = malloc(sizeof(struct env));			//add definition to function list
					initSymbolTable(top->next,top->envDepth);		//
					top->next->prev = top;
					top = top->next;
					top->offset = 0;
					top->inStruct=0;
					newCode(MFUNCTION,0,0,tmpFun);		//**********************
		  } paras RP{
			  $<value>$ = $<value>4;
		  }
	;

paras	: para COMMA paras	{
			  struct arg* hd = malloc(sizeof(struct arg));
			  hd->name = strdup($<id>1);						//store parameter definition in temporary
			  hd->type = $<type>1;								//link list. The link list is used for 
			  $<value>$ = $<value>3 + 1;						//parameter def in function list
			  hd->next = tmpFun->para;
			  tmpFun->para = hd;
				if($<id>3 == NULL)
					yyerror(errorMsg[SYM_INV]);
			  /*done*/}
	| para			{
							$<id>$ = $<id>1;
							$<defLoc>$ = NULL;
							$<value>$ = 1;
							tmpFun->para = malloc(sizeof(struct arg));
							tmpFun->para->name = strdup($<id>1);
							tmpFun->para->type = $<type>1;
							tmpFun->para->next=NULL;
					}
	| /* empty */	{
						$<id>$ = NULL;
						$<value>$ = 0;
						$<defLoc>$ = NULL;
					}
|error COMMA paras
	;

para	: spec var		{
							$<id>$ = $<id>2;
							$<type>$ = VARIABLE_INT;
							$<defLoc>$ = NULL;
							if(findSymbol(top,$<id>2) != NULL)
							{
								yyerror(errorMsg[VAR_RED]);
							}
							else
							{
								if($<type>1 == VARIABLE_STRUCT)
									yyerror(errorMsg[ARG_TYP]);
								else
								{
									if($<type>1 == VARIABLE_INT && $<type>2 == ARRAY_INT)
									{
										$<type>$ = ARRAY_INT;
										$<value>$ = $<value>2;
										if(insertSymbol(top,$<id>2,$<value>$,ARRAY_INT,++tmpFun->numOfPara,NULL) != SUCCESS)
											yyerror(errorMsg[SYM_INV]);
									}
									else if($<type>1 == VARIABLE_INT && $<type>2 == VARIABLE_INT)
									{
										if(insertSymbol(top,$<id>2,$<value>$,VARIABLE_INT,++tmpFun->numOfPara,NULL) != SUCCESS)
											yyerror(errorMsg[SYM_INV]);
								fprintf(stderr,"%s at line %i column %i\n",errorMsg[ARG_NUM],@1.first_line,@1.first_column);
										newCode(MID,0,-2,findSymbol(top,$<id>2));
										//-2 means para
									}
									else
										yyerror(errorMsg[OTHER]);
								}
							}
		  }
	;

stmtblock :	LC{	/* envDepth == number of LC */
				if(top->envDepth > 0)					//	here topDepth is used to indicate the top 
				{										//	symbol table position, in global, it's 0,
					top->next = malloc(sizeof(struct env));		//for struct can be defined only in global
					initSymbolTable(top->next,top->envDepth+1);	//it can be tell us whether it's in a struct
					top->next->prev=top;						//or not
					top = top->next;
					top->inStruct=0;
					top->offset=0;
				}
				else
					top->envDepth++;
			}defs  stmts RC {
								if(top->envDepth > 1)
								{
									top = top->prev;
									freeEnv(top->next);
									top->next = NULL;
								}
								$<trueList>$=$<trueList>4;$<falseList>$=$<falseList>4;
			}
	;
stmts	: stmt stmts {		//trueList and falseList in stmt is the list of break and continue
			  $<trueList>$=merge($<trueList>1,$<trueList>2);$<falseList>$=merge($<falseList>1,$<falseList>2);
		  }
|error stmts
	| /* empty */ {$<trueList>$=$<falseList>$=NULL;}
	;
stmt	: exporemp SEMI			{$<trueList>$=$<falseList>$=NULL;updatePPList();}//pplist : late add of i++
	| stmtblock					{$<trueList>$=$<trueList>1;$<falseList>$=$<falseList>1;}
	| MREAD LP exp RP SEMI		{$<trueList>$=$<falseList>$=NULL;
								if($<type>3!=VARIABLE_INT)			//check type of function read
									fprintf(stderr,"invalid type of argument at %i %i\n",@3.first_line,@3.first_column);
								newCode(READ,$<code>3,0,NULL);
								}
	| MWRITE LP exp RP SEMI		{$<trueList>$=$<falseList>$=NULL;
								if($<type>3!=VARIABLE_INT&&$<type>3!=CONSTANT)
									fprintf(stderr,"invalid type of argument at %i %i\n",@3.first_line,@3.first_column);
								newCode(WRITE,$<code>3,0,NULL);
								}
	| RETURN exp SEMI			{	//check type of exp
		default_exp_check($<type>2,@2.first_line,@2.first_column);
		int tp=defaultBP($<trueList>2,$<falseList>2,$<code>2);
		updatePPList();
		tp==-1?newCode(MRETURN,$<code>2,-1,NULL):newCode(MRETURN,tp,-1,NULL);		
								}
	| IF LP exp{
		if($<type>3 != VARIABLE_INT&&$<type>3!=CONSTANT)
			yyerror(errorMsg[EXP_IF_FOR]);
		backPatch($<trueList>3,nextInstruction());		//backpatch
		updatePPList();							//update i++ in exp
		if($<trueList>3 == NULL && $<falseList>3 == NULL)
		{
			newCode(MIF,$<code>3,nextInstruction()+2,NULL);
			$<falseList>3=makeList(newCode(MGOTO,0,0,NULL));
		}
	} RP stmt {backPatch($<falseList>3,nextInstruction()+1);
			$<falseList>3=makeList(newCode(MGOTO,0,0,NULL));}
			estmt	{$<trueList>$=merge($<trueList>6,$<trueList>8);			/*for break and*/
					$<falseList>$=merge($<falseList>6,$<falseList>8);
					if($<value>8)newCode(MGOTO,nextInstruction()+1,0,NULL);
			backPatch($<falseList>3,nextInstruction());}/*continue statement*/
	| FOR{numFor++;} LP exporemp {
		if($<type>4 != VARIABLE_INT&&$<type>4!=CONSTANT && $<value>4 !=-1)
			yyerror(errorMsg[EXP_IF_FOR]);
		if($<value>4!=-1)newCode(MGOTO,nextInstruction()+1,0,NULL);
		updatePPList();
	}SEMI{$<value>6=nextInstruction();} exporemp {
		if($<type>8 != VARIABLE_INT&&$<type>8!=CONSTANT && $<value>8 != -1)
			yyerror(errorMsg[EXP_IF_FOR]);
		if($<value>8!=-1)
		{
			updatePPList();
			if($<trueList>8==NULL&&$<falseList>8==NULL)
			{
				$<trueList>8=makeList(newCode(MIF,$<code>8,-1,NULL));
				$<falseList>8=makeList(newCode(MGOTO,0,-1,NULL));
			}
		}
		else $<trueList>8=$<falseList>8=NULL;
	}SEMI{$<value>10=nextInstruction();} exporemp {
		if($<type>12 != VARIABLE_INT&&$<type>12!=CONSTANT && $<value>12 != -1)
			yyerror(errorMsg[EXP_IF_FOR]);
		backPatch($<trueList>12,$<value>6);backPatch($<falseList>12,$<value>6);
		updatePPList();
	}RP{$<value>14=nextInstruction();} stmt{
		numFor--;			//exit for loop
		newCode(MGOTO,$<value>6,0,NULL);																			//-----------
	//																															|
		// condition jump backpatch																								|
		backPatch($<trueList>8,$<value>10);
		backPatch($<falseList>8,nextInstruction());
		freeList($<trueList>8,$<falseList>8);//	|
//																																|
		//break and continue handle																								|
		backPatch($<trueList>16,$<value>10+nextInstruction()+1-$<value>14);backPatch($<falseList>16,nextInstruction());//			|
		exchange($<value>10,$<value>14,nextInstruction()-1);			//											<------------
		$<falseList>$=$<trueList>$=NULL;freeList($<trueList>16,$<falseList>16);
	}	
	| CONT SEMI	{
		if(checkFor(numFor))
		{
			$<trueList>$=makeList(nextInstruction());$<falseList>$=NULL;
			newCode(MGOTO,0,-1,NULL);
		}
		else
			fprintf(stderr,"continue at line %i column %i\n",@1.first_line,@1.first_column);
	}
	| BREAK SEMI {
		if(checkFor(numFor))
		{
			$<falseList>$=makeList(nextInstruction());$<trueList>$=NULL;
			newCode(MGOTO,0,-1,NULL);
		}
		else
			fprintf(stderr,"break at line %i column %i\n",@1.first_line,@1.first_column);
			;}
	| error SEMI
	;
exporemp:exp{
			 $<code>$=$<code>1;$<trueList>$=$<trueList>1;$<falseList>$=$<falseList>1;
			 $<value>$=0;
		 }
		|/*empty*/{$<code>$=-1;$<trueList>$=$<falseList>$=NULL;$<value>$=-1;}
		;
estmt	: ELSE stmt	{$<value>$=1;$<falseList>$=$<falseList>2;$<trueList>$=$<trueList>2;}
	| /* empty */{$<value>$=0;$<falseList>$=$<trueList>$=NULL;}
	;
defs	: def defs	{
		  }
	| /* empty */
	;
def	: spec decs SEMI	{
	  } 
|spec error SEMI
	;
decs	: dec COMMA decs {}
	| dec			{}
	;

dec	: var			{
		struct symbol* rs = decVar($<id>$,$<type>1,$<value>1,$<type>0,$<type>-1,$<value>0,$<value>-1,$<defLoc>0,$<defLoc>-1,COMMA,top);
		rs->arraySize=$<value>1;
		memcpy(rs->dim,db,$<value>1*4);
		int x=0,y=1;
		for(x=0;x<$<value>1;++x)
			y*=db[x];
		bzero(db,40);
		switch(rs->type)
		{		//add symbol definition to symbol table
			case VARIABLE_INT: newCode(MDEC_INT,-1,1-top->inStruct,rs);top->offset+=4;break;
			case VARIABLE_STRUCT: newCode(MDEC_AREA,rs->structDefLoc->varNum * 4,1-top->inStruct,rs);
								  top->offset+=rs->structDefLoc->varNum*4;break;
			case ARRAY_INT: newCode(MDEC_AREA,y * 4,1-top->inStruct,rs);top->offset+=y * 4;break;
			case ARRAY_STRUCT: newCode(MDEC_AREA,y * rs->structDefLoc->varNum * 4,1-top->inStruct,rs);
							   top->offset+=y*rs->structDefLoc->varNum*4; break;
		}
		$<defLoc>$=rs->structDefLoc;
		$<value>$=rs->value;
		$<type>$=rs->type;
					}
	| var ASSIGN init {
			struct symbol* rs = decVar($<id>$,$<type>1,$<value>1,$<type>0,$<type>-1,$<value>0,$<value>-1,$<defLoc>0,$<defLoc>-1,COMMA,top);
			int i=0;
		$<defLoc>$=rs->structDefLoc;
		$<value>$=rs->value;
		$<type>$=rs->type;
		rs->arraySize=$<value>1;
		memcpy(rs->dim,db,$<value>1*4);
		int x=0,y=1;
		for(x=0;x<$<value>1;++x)
			y*=db[x];
		bzero(db,40);
		switch(rs->type)
		{		// add symbol definition and initializztion value to symbol table
			case VARIABLE_INT: newCode(MINIT_INT,$<code>3,1-top->inStruct,rs);top->offset+=4;break;
			case ARRAY_INT: newCode(MINIT_AREAH,y * 4,1-top->inStruct,rs);top->offset+=y * 4;
							rs->arrayInitValue=malloc(sizeof(int)*rs->dim[0]);
							for(;i<rs->dim[0];++i)
							{	//add value of symbol
								rs->arrayInitValue[i]=callList->exeLoc->value;
								newCode(MINIT_AREA,rs->arrayInitValue[i],-1,NULL);
								struct arg* targ=callList->exeLoc;callList->exeLoc=targ->next;
								free(targ);
							}
							callList=callList->prev;free(callList->next);
							callList->next=NULL;
							if(rs->arraySize!=1)
							{
								fprintf(stderr,"only one dimensional array could be initilize at line %i column %i\n",
										@1.first_line,@1.first_column);
								errorNum++;
							}
							if($<value>3!=rs->dim[0])
							{
								fprintf(stderr,"%s at line %i column %i\n",errorMsg[ARG_NUM],@1.first_line,@1.first_column);
								errorNum++;
							}
							break;
		}
	}
	;
init	: exp {
				int t = $<type>-1;
				if(t != VARIABLE_INT)
					yyerror(errorMsg[INIT]);
				else if($<type>1 != VARIABLE_INT && $<type>1 != CONSTANT)
					yyerror(errorMsg[ARG_TYP]);
				$<code>$ = $<code>1; $<id>$ = NULL;
				$<type>$ = $<type>1; $<value>$ = $<value>1;
		  } 
	| LC {
			callList->next = malloc(sizeof(struct funRecord));
			callList->next->prev = callList;
			callList = callList->next;
			callList->isFunCall = 0;
			callList->record = NULL;
			callList->exeLoc = NULL;
			callList->next = NULL;
		}args RC {
			int t = $<type>-1;
			if(t != ARRAY_INT)
				yyerror(errorMsg[INIT]);
			else
			{
				int atcSize = 0;
				struct arg* t = callList->exeLoc;
				while(t != NULL)
				{
					if(t->type != CONSTANT)
						yyerror(errorMsg[ARG_TYP]);
					t = t->next; atcSize++;
				}
				$<id>$ = NULL; $<value>$ = atcSize;
				$<type>$ = ARRAY_INT;
			}
		}
	;



exp	: exp KMUL{

/**********************the following is the arithmentic operation***************************/
/*first check type of exp: variable_int or int, then update the special exp such as logic and*/
/*expression by using backpatch, after that, update the IR code*/

		  $<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);}  exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MMUL,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	  }
	| exp KDIV{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MDIV,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp MOD {	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);}exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MMOD,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp PLUS{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MPLUS,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp MINUS {	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp	{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MMINUS,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp SHL{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MSHL,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp SHR {	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);}exp{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MSHR,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp GREAT{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MGREAT,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp GREATEQ{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MGREATEQ,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp LESS{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MLESS,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp LESSEQ{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MLESSEQ,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	}
	| exp EQUAL{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp	{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MEQUAL,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	  }
	| exp NOEQUAL{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MNOEQUAL,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	  }
	| exp BITAND {	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);}exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);			
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MBITAND,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	  }
	| exp BITXOR{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);			
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MBITXOR,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	  }
	| exp BITOR{	$<code>2=defaultBP($<trueList>1,$<falseList>1,$<code>1);} exp	{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);			
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
			$<code>$ = defaultBP2(MBITOR,$<code>1,$<code>4,$<code>2,tp);
		}
		else $<code>$ = -1;
	  }
	| exp LOGAND{
			if($<trueList>1 == NULL && $<falseList>1 == NULL)
			{
				$<trueList>1 = NULL;
				//$<falseList>1 = makeList(newCode(MIF,$<code>1,-1,NULL));
				newCode(MIF,$<code>1,nextInstruction()+2,NULL);
				$<falseList>1=makeList(newCode(MGOTO,0,-1,NULL));
			}
			$<value>2 = nextInstruction();
		} exp {
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL;
			$<defLoc>$ = NULL;
			$<value>$ = 0;
			if($<trueList>4 == NULL && $<falseList>4 == NULL)
			{
				$<trueList>4 = makeList(newCode(MIF,$<code>4,-1,NULL));
				$<falseList>4 = makeList(newCode(MGOTO,0,-1,NULL));
			}

			backPatch($<trueList>1,$<value>2);
			$<trueList>$ = $<trueList>4;
			$<falseList>$ = merge($<falseList>1,$<falseList>4);
			$<code>$ = newCode(MLOGAND,$<code>4,$<code>1,NULL);
		}
		else $<code>$ = -1;
	  }
	| exp LOGOR{
			if($<trueList>1 == NULL && $<falseList>1 == NULL)
			{
				$<falseList>1 = NULL;
				$<trueList>1 = makeList(newCode(MIF,$<code>1,-1,NULL));
				newCode(MGOTO,nextInstruction()+1,-1,NULL);
			}
			$<value>2 = nextInstruction();
	} exp	{
		if(default_exp_check($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>4,@4.first_line,@4.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>4);
			$<id>$ = NULL;
			$<defLoc>$ = NULL;
			$<value>$ = 0;

			if($<trueList>4 == NULL && $<falseList>4 == NULL)
			{
				$<trueList>4 = makeList(newCode(MIF,$<code>4,-1,NULL));
				$<falseList>4 = makeList(newCode(MGOTO,0,-1,NULL));
			}

			backPatch($<falseList>1,$<value>2);
			$<falseList>$ = $<falseList>4;
			$<trueList>$ = merge($<trueList>1,$<trueList>4);
			$<code>$ = newCode(MLOGOR,$<code>1,$<code>4,NULL);
		}
		else $<code>$ = -1;
	  }
	| exp ASSIGN exp{
		if($<type>1 == CONSTANT)
			yyerror(errorMsg[ASS_ERR]);
		else if($<type>1 != VARIABLE_INT)
			yyerror(errorMsg[OPE_INV]);
		else if(default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = VARIABLE_INT;
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MASSIGN,$<code>1,$<code>3,-1,tp);
		}
		else $<code>$ = -1;
	  }
	| exp PLUSASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MPLUSASS,$<code>1,$<code>3,-1,tp);
		}else $<code>$ = -1;
	  }
	| exp MINUSASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MMINUSASS,$<code>1,$<code>3,-1,tp);
		}else $<code>$ = -1;
	  }
	| exp MULASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MMULASS,$<code>1,$<code>3,-1,tp);
		}
		else $<code>$ = -1;
	  }
	| exp DIVASS exp{
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MDIVASS,$<code>1,$<code>3,-1,tp);
		}else $<code>$ = -1;
	  }
	| exp ANDASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MANDASS,$<code>1,$<code>3,-1,tp);
		}else $<code>$ = -1;
	  }
	| exp XORASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MXORASS,$<code>1,$<code>3,-1,tp);
		}else $<code>$ = -1;
	  }
	| exp ORASS exp	{
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MORASS,$<code>1,$<code>3,-1,tp);
		}
		else $<code>$ = -1;
	  }
	| exp SHLASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			backPatch($<falseList>3,nextInstruction()); backPatch($<trueList>3,nextInstruction());
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MSHLASS,$<code>1,$<code>3,-1,tp);
		}
		else $<code>$ = -1;
	  }
	| exp SHRASS exp {
		if(check_left_value($<type>1,@1.first_line,@1.first_column) && default_exp_check($<type>3,@3.first_line,@3.first_column))
		{
			$<type>$ = typeInfer($<type>1,$<type>3);
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0; $<trueList>$ = $<falseList>$ = NULL;
			int tp=defaultBP($<trueList>3,$<falseList>3,$<code>3);
			$<code>$ = defaultBP2(MSHRASS,$<code>1,$<code>3,-1,tp);
		}
		else $<code>$ = -1;
	  }
	| INC exp{
		if(check_left_value($<type>2,@2.first_line,@2.first_column))
		{
			$<type>$ = VARIABLE_INT;
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0;
			$<trueList>$ = $<falseList>$ = NULL;
			$<code>$ = newCode(MINC_PRE,$<code>2,-1,NULL);
		}
		else $<code>$ = -1;
	} 
	| exp INC {
		if(check_left_value($<type>1,@1.first_line,@1.first_column))
		{
			$<type>$ = VARIABLE_INT;
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0;
			$<trueList>$ = $<falseList>$ = NULL;
			$<code>$ = newCode(MINC,$<code>1,-1,NULL);
		}
		else $<code>$ = -1;
	}
	| DEC exp {
		if(check_left_value($<type>2,@2.first_line,@2.first_column))
		{
			$<type>$ = VARIABLE_INT;
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0;
			$<trueList>$ = $<falseList>$ = NULL;
			$<code>$ = newCode(MDEC_PRE,$<code>2,-1,NULL);
		}
		else $<code>$ = -1;
	}
	| exp DEC {
		if(check_left_value($<type>1,@1.first_line,@1.first_column))
		{
			$<type>$ = VARIABLE_INT;
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0;
			$<trueList>$ = $<falseList>$ = NULL;
			$<code>$ = newCode(MDEC,$<code>1,-1,NULL);
		}
		else $<code>$ = -1;
	}
	| LOGNOT exp {
		if(default_exp_check($<type>2,@2.first_line,@2.first_column))
		{
			$<type>$ = $<type>2;
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0;
			$<code>$ = newCode(MLOGNOT,$<code>2,-1,NULL);
			$<trueList>$ = $<falseList>2;
			$<falseList>$ = $<trueList>2;
		}
		else $<code>$ = -1;
	}
	| BITNOT exp {
		if(default_exp_check($<type>2,@2.first_line,@2.first_column))
		{
			$<type>$ = $<type>2;
			$<id>$ = NULL; $<defLoc>$ = NULL; $<value>$ = 0;
			int tp=defaultBP($<trueList>2,$<falseList>2,$<code>2);
			$<code>$ = defaultBP2(MBITNOT,$<code>2,-1,tp,-1);
		}
		else $<code>$ = -1;
	}
	| MINUS exp %prec UMINUS{
		if(default_exp_check($<type>2,@2.first_line,@2.first_column))
		{
			$<type>$ = $<type>2;
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0;
			int tp=defaultBP($<trueList>2,$<falseList>2,$<code>2);
			$<code>$ = defaultBP2(MMINUS_PRE,$<code>2,-1,tp,-1);
		}
		else $<code>$ = -1;
	}		
	| LP exp RP	{
		if(default_exp_check($<type>2,@2.first_line,@2.first_column))
		{
			$<type>$ = $<type>2;
			$<id>$ = NULL;$<defLoc>$ = NULL;$<value>$ = 0;
			$<falseList>$ = $<falseList>2; $<trueList>$ = $<trueList>2;
			$<code>$ = $<code>2;
		}
		else $<code>$ = -1;
	}
	| ID LP	{
				/* construct a activation record stack */
				tmpFun = findFunction(functionList,$<id>1);
				if(tmpFun == NULL)
					yyerror(errorMsg[FUN_UNDEF]);
				else
				{
					callList->next = malloc(sizeof(struct funRecord));
					callList->next->prev = callList;
					callList = callList->next;
					callList->next = NULL;
					callList->record = tmpFun;
					callList->exeLoc = tmpFun->para;
					callList->isFunCall = 1;
				}
		} args RP	{
						if(callList->exeLoc != NULL)
							yyerror(errorMsg[ARG_NUM]);
						$<code>$=newCode(MCALL,$<value>4,-1,callList->record);
						callList = callList->prev;
						free(callList->next);
						$<id>$ = NULL; $<type>$ = VARIABLE_INT; $<value>$ = 0;
						$<defLoc>$ = NULL; $<trueList>$ = $<falseList>$ = NULL;
			}
	| ID arrs	{
					tmpEnv = top;
					struct symbol* tmpSym;
					while(tmpEnv != NULL)
					{
						tmpSym = findSymbol(tmpEnv,$<id>1);				//error check:
						if( tmpSym != NULL)
							break;
						tmpEnv = tmpEnv->prev;
					}
					if(tmpEnv == NULL)
					{
						yyerror(errorMsg[SYM_UNDEF]);
						$<code>$ = -1;
					}
					else												//*******************
					{
						$<id>$ = $<id>1;
						$<value>$ = tmpSym->value;
						$<defLoc>$ = tmpSym->structDefLoc;			//definiton position
						$<trueList>$ = $<falseList>$ = NULL;
						if($<value>2 == 0)
						{
							$<type>$ = tmpSym->type;
							$<code>$ = newCode(MID,0,-1,tmpSym);
							if(tmpSym->type != VARIABLE_INT && tmpSym->type != VARIABLE_STRUCT)
								yyerror("warning:");
						}
						else if($<value>2 == tmpSym->arraySize && tmpSym->type == ARRAY_INT)
						{
							$<type>$ = VARIABLE_INT;
							int x=1,c=db[0],b;
							for(;x<$<value>2;++x)
							{
								b=newCode(MINT,tmpSym->dim[x],0,NULL);
								b=newCode(MMUL,c,b,NULL);
								c=newCode(MPLUS,b,db[x],NULL);
							}
							bzero(db,40);
							$<code>$ = newCode(MID,c,0,tmpSym);	
						}
						else if($<value>2 == tmpSym->arraySize && tmpSym->type == ARRAY_STRUCT)
						{
							$<type>$ = VARIABLE_STRUCT;
							int x=1,c=db[0],b;
							for(;x<$<value>2;++x)
							{
								b=newCode(MINT,tmpSym->dim[x],0,NULL);
								b=newCode(MMUL,c,b,NULL);
								c=newCode(MPLUS,b,db[x],NULL);
							}
							bzero(db,40);
							$<code>$ = newCode(MID,c,0,tmpSym);
						}
						else
						{
							$<code>$ = -1;
							yyerror(errorMsg[OPE_INV]);
						}
					}
				}
	| exp DOT ID	{
						if($<type>1 == VARIABLE_STRUCT)
						{
							tmpEnv = top;
							freeList($<falseList>1,$<trueList>1);
							$<trueList>$ = $<falseList>$ = NULL;
							struct symbol* tmpSym;
							while(tmpEnv != NULL)
							{
								tmpSym = findSymbol(tmpEnv,$<id>1);
								tmpEnv=tmpEnv->prev;
								if(tmpSym != NULL) break;
							}
							struct structField* tmpFld = tmpSym->structDefLoc;
							// check the type of corresponding symbol in the symbol table
							if(tmpSym->type != VARIABLE_STRUCT&&tmpSym->type!=ARRAY_STRUCT)
								yyerror(errorMsg[OPE_INV]);
							else
							{
								int i = 0;
								for(;i < tmpFld->varNum;++i)
									if(strcmp($<id>3,tmpFld->field[i]) == 0)
										break;
								if(i == tmpFld->varNum)
									yyerror(errorMsg[SYM_UNDEF]);
								else
								{
									if($<code>1 == -1) 
										$<code>$ = -1;
									else
										$<code>$ = newCode(MDOT,$<code>1,i,tmpSym);
									$<type>$ = VARIABLE_INT;
									$<id>$ = NULL;
									$<defLoc>$ = NULL;
								}
							}
						}
						else
							yyerror(errorMsg[OPE_INV]);
					}
	| INT			{
						$<type>$ = CONSTANT;
						$<id>$ = NULL; $<value>$ = toInt($<id>1,@1.first_line,@1.first_column); $<defLoc>$ = NULL;
						$<trueList>$ = $<falseList>$ = NULL;
						$<code>$ = newCode(MINT,$<value>$,0,NULL);
					}
	;
arrs	: LB exp RB arrs	{
								$<id>$ = NULL;
								$<value>$ = $<value>4 + 1;
								$<defLoc>$ = NULL;
								$<type>$ = ARRAY_INT;
								db[$<value>4]=$<code>2;
							}
		|	/*empty*/		{
								$<id>$ = NULL;
								$<value>$ = 0;
								$<defLoc>$ = NULL;
								$<type>$ = NOTCLEAR;
							}
		;
args	: args COMMA{
						if($<id>1 == NULL && $<value>1 == -1)
							yyerror(errorMsg[SYM_INV]);
		  } exp	{
					$<id>$ = NULL;
					if(callList->isFunCall == 1)			//argument for function
					{
						if(callList->exeLoc == NULL || typeMismatch($<type>4, callList->exeLoc->type))
							yyerror(errorMsg[ARG_TYP]);
						else
						{
							callList->exeLoc = callList->exeLoc->next;
							$<type>$ = $<type>1;$<value>$ = $<value>1+1;$<defLoc>$ = $<defLoc>1;
							int tp=defaultBP($<trueList>4,$<falseList>4,$<code>4);
							updatePPList();
							tp==-1?newCode(MPARAM,$<code>4,-1,NULL):newCode(MPARAM,tp,-1,NULL);
						}
					}
					else					//argument for initial
					{
						struct arg* tmpArg = callList->exeLoc;
						while(tmpArg->next != NULL)
							tmpArg = tmpArg->next;
						tmpArg->next = malloc(sizeof(struct arg));
						tmpArg->next->next = NULL;
						tmpArg->next->type = $<type>4;
						tmpArg->next->name = NULL;
						tmpArg->next->value = $<code>4;
					}
		  }
	| exp	{	
				$<id>$ = NULL;
				if(callList->isFunCall == 1)
				{
					if(callList->exeLoc == NULL || typeMismatch($<type>1, callList->exeLoc->type) ) 
						yyerror(errorMsg[ARG_TYP]);
					else
					{
						callList->exeLoc = callList->exeLoc->next;
						$<type>$ = $<type>1;$<value>$ = 1;$<defLoc>$ = $<defLoc>1;
						int tp=defaultBP($<trueList>1,$<falseList>1,$<code>1);
						updatePPList();
						tp==-1?newCode(MPARAM,$<code>1,-1,NULL):newCode(MPARAM,tp,-1,NULL);
					}
				}
				else
				{
					assert(callList->exeLoc == NULL);
					callList->exeLoc = malloc(sizeof(struct arg));
					callList->exeLoc->name = $<id>1 == NULL?NULL:strdup($<id>1);
					callList->exeLoc->type = $<type>1;
					callList->exeLoc->next = NULL;
					callList->exeLoc->value = $<code>1;
				}
			}
	|				{
						$<id>$ = NULL;
						$<value>$ = 0;
						$<type>$ = NOTCLEAR;
						$<defLoc>$ = NULL;
					}
|error COMMA exp
	;
%%
#include "lex.yy.c"
void main(int argc,char* argv[])
{
	
	if(argc!=3)
	{
		fprintf(stderr,"you should provide name of both input file and output file");
		exit(0);
	}
	//token location
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	
	//symbol table
	top = malloc(sizeof(struct env));
	initSymbolTable(top,0);
	top->inStruct=0;

	//function definition list
	functionList = malloc(sizeof(struct functionDef));
	functionList->next = NULL;
	functionList->name = NULL;
	functionList->para = NULL;

	//function called list
	callList = malloc(sizeof(struct funRecord));
	callList->prev = callList->next = NULL;
	callList->record = NULL;
	callList->isFunCall = 0;
	//struct definition list
	structList = malloc(sizeof(struct structField));
	structList->next = NULL;
	structList->typeName = NULL;
	structList->field = NULL;
	structList->varNum = 0;

	errorNum = 0;
	numFor = 0;
	initTree();
	if((yyin=fopen(argv[1],"r"))==NULL)
	{
		fprintf(stderr,"can't find file %s\n",argv[1]);
		exit(0);
	}
	yyparse();

	if(errorNum > 0)
	{
		FILE* f=fopen(argv[2],"w");
		fprintf(f,"error\n");
		fprintf(stderr,"error: %i\n", errorNum);
		return;
	}
	output("1.out");
	reduceGoto();
	markBlock();
	repConstant();
	initReg();
	updateUseInfo();
	output("2.out");
	genCode();
	opt();
	printCode(argv[2]);
}

int default_exp_check(int t,int line,int col)
{
	if(t != VARIABLE_INT && t != CONSTANT)
	{
		errorNum++;
		fprintf(stderr,"%s at line %i, column %i \n", errorMsg[ARI_INV], line, col);
		return 0;
	}
	return 1;
}

int yyerror(char* msg)
{
	fprintf(stderr,"%s at line %i, column %i \n",msg,yylineno,yylloc.first_column);
	errorNum++;
}
