//symbol table definition of parser
// symbol.h
//
//

#include "string.h"
#include "stdlib.h"
#include "assert.h"
//basic data type of small C
#define VARIABLE_INT		0
#define VARIABLE_STRUCT		1
#define CONSTANT			2
#define FUNCTION			3
#define ARRAY_INT			4
#define ARRAY_STRUCT		5
#define NOTCLEAR			6

#define KEYWORD_SIZE	9	
#define INIT_STRUCT_TYPE_SIZE	32
#define INIT_STRUCT_FIELD_SIZE	32

#define IS_KEYWORD	0
#define ID_EXIST	1
#define SUCCESS		2

//symbol table type
#define NORMAL			1
#define APARA			2

//error index
#define SYM_UNDEF	0
#define VAR_RED		1
#define OPE_INV		2
#define SYM_INV		3
#define TYP_UNDEF	4
#define SRT_NEST	5
#define FUN_RED		6
#define FUN_UNDEF	7
#define ARG_TYP		8
#define ARG_NUM		9
#define FUN_MAIN	10
#define FUN_RET		11
#define STR_TYP		12
#define ARY_DEC		13
#define INIT		14
#define ASS_ERR		15
#define EXP_IF_FOR	16
#define BRK_CON		17
#define ARI_INV		18
#define OTHER		19

extern int yyerror(char*);

//all keyword in small C
char* keyword[KEYWORD_SIZE]={"main","for",
"if","else","return","struct","int","read","write"};

//error message
char* errorMsg[]={
	"undefined symbol",												//SYM_UNDEF
	"variable redeclare",											//VAR_RED
	"invalid operation: Is variable a valid left value?Is expression an array with decomposed operation",
	"use invalid symbol: is that keyword, redundant comma?",		//SYM_INV
	"type undefined",												//TYP_UNDEF
	"nested struct declare",										//STR_NEST
	"function redecalre",											//FUN_RED
	"function undefined",											//FUN_UNDEF
	"type of arguments mismatched, or type is invalid",				//ARG_TYP
	"number of argument mismatched",									//ARG_NUM
	"lack of main function",										//FUN_MAIN
	"invalid return type: only int could be used",					//FUN_RET
	"invalid type in struct: only int could be used",				//STR_TYP
	"invalid redecalration of array",								//ARY_DEC
	"invalid to initialize variable whose type is neither int nor array of int",
	"assign to right value",
	"type of expression or condiction is not int",
	"invalid usage of break or continue",
	"invalid arithmetic operation Is expression type of int?",

	"unresolved error"
};

struct symbol
{
	//identifier of variable, for constant, this field should NULL
	char* id;

	//for int variable or constant, it indicate the value of them;
	//for function, it's the index of argument of function in array functionList
	int value;			
	int type;

	int arraySize;

	struct structField* structDefLoc;		//indicate location of definition of struct
	struct functionDef* funLoc;		//function definition location in function list

	struct symbol* left;
	struct symbol* right;

	int* arrayInitValue;

	int dim[10];
	//
//	struct variableDescriptor* varDes;
	int isPara;
};

//defintion of struct type
struct structField
{
	char* typeName;
	char** field;

	struct structField* next;
	int varNum;
};

//argument apir of function
struct arg
{
	char* name;		//arguament name;
	char type;		//argument type;

	int value;

	struct arg* next;
};

//function list is constructed as a linked list,
//and it's a global variable since it's not allowed to declare a function within another function
struct functionDef
{
	char* name;
	struct arg* para;

	struct functionDef* next;

	int offset;
	int start;
	int numOfPara;
};

//symbol table is constructed as a binary search tree
struct env
{
	struct symbol* root;
	int num;						// sum of variable in env
	struct env* next;
	struct env* prev;
	int envDepth;

	int offset;						// space of parameter and local variable
	int inStruct;
};

struct funRecord
{
	struct functionDef* record;
	struct arg* exeLoc;				// indicate return postion of second f is 1 f(f(a),b)

	int isFunCall;

	struct funRecord* prev;
	struct funRecord* next;
};

// check if it's a keyword 
int isKeyWord(char* word)
{
	int i=0;
	for(;i<KEYWORD_SIZE;++i)
		if(strcmp(word,keyword[i])==0)
			return 1;
	return 0;
}

//find symbol in current symbol table with fixed key
struct symbol* findSymbol(struct env* e, char* key)
{
	if(e->root==NULL)
		return NULL;

	int flag;

	struct symbol* tmp=e->root;
	while(tmp!=NULL)
	{
		flag=strcmp(key,tmp->id);
		if(flag==0)
			return tmp;
		else if(flag>0)
			tmp=tmp->right;
		else
			tmp=tmp->left;
	}
	return NULL;
}

//initialize symbol table
void initSymbolTable(struct env* e,int dep)
{
	e->num=0;
	e->root=NULL;
	e->prev=e->next=NULL;
	e->envDepth = dep;
}

//insert a symbol into symbol table
int insertSymbol(struct env* e, char* word, int value, char type,int isPara,struct structField* loc)
{
	struct symbol* sym=malloc(sizeof(struct symbol));
	sym->id=strdup(word);
	sym->value=value;
	sym->type=type;
	sym->left=sym->right=NULL;
	sym->structDefLoc=loc;
	sym->isPara=isPara;

	if(findSymbol(e,word)!=NULL)
		return ID_EXIST;
	if(e->root==NULL)
		e->root=sym;
	else
	{
		struct symbol* tmp=e->root;
		int flag;
		while(1)
		{
			flag=strcmp(word,tmp->id);
			assert(flag!=0);
			if(flag>0)
			{
				if(tmp->right==NULL)
				{
					tmp->right=sym;
					break;
				}
				else
					tmp=tmp->right;
			}
			else
			{
				if(tmp->left==NULL)
				{
					tmp->left=sym;
					break;
				}
				else
					tmp=tmp->left;
			}
		}
	}
	e->num++;
	return SUCCESS;
}

void fillStructField(char** field,struct symbol* s,int* j)
{
	if(s==NULL)
		return;
	field[*j]=strdup(s->id);
	(*j)++;
	fillStructField(field,s->left,j);
	fillStructField(field,s->right,j);
}

//find function in function list: we can only define function global domain
struct functionDef* findFunction(struct functionDef* fl, char* t)
{
	if(fl->next==NULL)
		return NULL;
	
	struct functionDef* tmp=fl;
	//in here, take recently consulted element in the head
	while(tmp->next != NULL)
	{
		if(strcmp(tmp->next->name,t)==0)
		{
			struct functionDef* tmp1=tmp->next;
			tmp->next=tmp1->next;
			tmp1->next=fl->next;
			fl->next=tmp1;
			return fl->next;
		}
		tmp=tmp->next;
	}
	return NULL;
}

//irtFun must be initialized and be allocated space
void insertFunction(struct functionDef* fl,struct functionDef* irtFun)
{
	assert(irtFun->name!=NULL);
	irtFun->next=fl->next;
	fl->next=irtFun;
}

//find a struct type
struct structField* findStructType(struct structField* rl, char* t)
{
	if(rl->next == NULL)
		return NULL;

	struct structField* tmp = rl;
	while(tmp->next != NULL)
	{
		if(strcmp(tmp->next->typeName,t) == 0)
		{
			struct structField* tmp1 = tmp->next;
			tmp->next = tmp1->next;
			tmp1->next = rl->next;
			rl->next = tmp1;
			return rl->next;
		}
		tmp = tmp->next;
	}
	return NULL;
}

void insertStructType(struct structField* rl, struct structField* irtSt)
{
	irtSt->next = rl->next;
	rl->next = irtSt;
}

static void freeSymbol(struct symbol* sym)
{
}

void freeEnv(struct env* e)
{}

//check whether two type can be applied arithmetic operation
int typeMismatch(int t1,int t2)
{
	if(t1 == t2)
		return 0;
	if(t1 == VARIABLE_INT && t2 == CONSTANT || t1 == CONSTANT && t2 == VARIABLE_INT)
		return 0;
	return 1;
}

//type convertion
int typeInfer(int t1,int t2)
{
	if(t1 == CONSTANT && t2 == CONSTANT)
		return CONSTANT;
	else
		return VARIABLE_INT;
}

//declaration of variable
struct symbol* decVar(char* id,int type1,int value1,int prevType0,int prevTypem1,
		int value0,int valuem1,struct structField* f0,struct structField* f1,int cm,struct env* top)
{
	//insert into symbol table
	int type,value;
	struct structField* f;
	if(prevType0 == cm)
	{
		type = prevTypem1;
		value = valuem1;
		f = f1;
	}
	else
	{
		type = prevType0;
		value = value0;
		f = f0;
	}
	if(findSymbol(top,id) != NULL)
		yyerror(errorMsg[VAR_RED]);
	else if(isKeyWord(id))
		yyerror(errorMsg[SYM_INV]);
	else
	{
		if(type == VARIABLE_STRUCT && type1 == VARIABLE_INT)
			insertSymbol(top,id,value,VARIABLE_STRUCT,0,f);
		else if(type == VARIABLE_STRUCT && type1 == ARRAY_INT)
			insertSymbol(top,id,value,ARRAY_STRUCT,0,f);
		else if(type == VARIABLE_INT && type1 == ARRAY_INT)
			insertSymbol(top,id,value1,ARRAY_INT,0,NULL);
		else if(type == VARIABLE_INT && type1 == VARIABLE_INT)
			insertSymbol(top,id,value1,VARIABLE_INT,0,NULL);
		else
			yyerror(errorMsg[OTHER]);
	}
	return findSymbol(top,id);
}

int checkFor(int i)
{
	return i>0;
}

int check_left_value(int t,int l,int c)
{
	if(t!=VARIABLE_INT)
		yyerror(errorMsg[ASS_ERR]);
	return t==VARIABLE_INT;
}
//convert digit stream to decimal number
int toInt(char* t,int l,int c)
{
	assert(t!=NULL);
	int len=strlen(t);
	int num=0,i=-1;
	if(t[0]!='0')
		//decimal
		while(++i<len)
			num=num*10+(int)(t[i]-'0');
	else if(t[1]=='x'||t[1]=='X')
	{	//hex
		i+=2;
		while(++i<len)
			if(t[i]<='9'&&t[i]>='0')num=num*16+(t[i]-'0');
			else if(t[i]<='X'&&t[i]>='A')num=num*16+(t[i]-'A'+10);
			else num=num*16+(t[i]-'a'+10);
	}
	else if(t[1]!='\0')
	{
		//oct
		i++;
		while(++i<len)
			if(t[i]<='7'&&t[i]>='0')num=num*8+(t[i]-'0');
			else fprintf(stderr,"invalid oct number at line %i, column %i\n",l,c);
	}
	return num;
}
