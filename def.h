
/*
   there are two part in this file:
   intermediate code generator ( in the front) and 
   machine code generate (behind the ir code)
 */


//three address code node
#include <string.h>
#include <stdio.h>
#include "symbol.h"
#define INIT_TREE_SIZE 1024

//IR Command


#define MMUL		8				//****************************************
#define MDIV		9
#define MMOD		10
#define MPLUS		11
#define MMINUS		12
#define MSHL		13
#define MSHR		14
#define MGREAT		15
#define MGREATEQ	16
#define MLESS		17
#define MLESSEQ		18
#define MEQUAL		19
#define MNOEQUAL	20
#define MBITAND		21				//**********	arithmetic	****************
#define MBITXOR		22
#define MBITOR		23
#define MBITNOT		24
#define MMINUS_PRE	25

#define MLOGAND		26
#define MLOGOR		27

#define MPLUSASS	28
#define MMINUSASS	29
#define MMULASS		30
#define MDIVASS		31
#define MANDASS		32
#define MXORASS		33
#define MORASS		34
#define MSHLASS		35
#define MSHRASS		36				//********************************************
#define MASSIGN		37

#define MINC		38
#define MDEC_PRE	39
#define MDEC		40
#define MINC_PRE	41

#define MLOGNOT		42

#define MCALL		43
#define MID			44
#define MDOT		45
#define MINT		46
#define MPARAM		47
#define MNOP		48
#define MTMP		49

#define MFUNCTION	50
#define MRETURN		51
#define MIF			52
#define MGOTO		53

#define	MDEC_INT	54
#define MDEC_AREA	55
#define MINIT_INT	56
#define MINIT_AREAH	57
#define MINIT_AREA	58


#define MIFFALSE	59
#define	MEND		60
#define	MERROR		61

#define SYSCALL		62
#define READ		63
#define	WRITE		64

#define REF			1
#define VAL			2
#define GLB			3
#define STK			4

#define GP			0x10008000

#define REG_A0	64
#define	REG_A1	65
#define	REG_A2	66
#define	REG_A3	67

#define	REG_T0	68
#define	REG_T1	69
#define	REG_T2	70
#define	REG_T3	71
#define	REG_T4	72
#define	REG_T5	73
#define	REG_T6	74
#define	REG_T7	75
#define	REG_T8	76
#define	REG_T9	77
#define	REG_S0	78
#define	REG_S1	79
#define	REG_S2	80
#define	REG_S3	81
#define	REG_S4	82
#define	REG_S5	83
#define	REG_S6	84
#define	REG_S7	85

#define	REG_V0	86
#define REG_V1	87
#define	REG_0	88
#define	REG_SP	89
#define	REG_FP	90
#define REG_GP	91
#define	REG_RA	92

#define LW		96
#define	SW		97
#define	ADD		98
#define	SUB		99
#define	JR		100
#define	JAL		101
#define	LI		102
#define	BEQ		103
#define	BNE		104
#define	ADDI	105
#define	AND		106
#define	ANDI	107
#define	OR		108
#define	ORI		109
#define	XOR		110
#define	MUL		111
#define	DIV		112
#define	REM		113
#define	NEG		114
#define MULI	115
#define SLL		116
#define	SLLV	117
#define	MOVE	118
#define SRAV	119
#define L		120
#define	DATA	121
#define	VALUE	122
#define	TEXT	123
#define	WORD	124
#define	SRA		125
#define	SLT		126
#define	SLTI	127
#define	SEQ		128
#define	SNE		129
#define	NOT		130
#define	XORI	131
#define	SLE		132
int sizeOfTrees;			// current size of expression tree

int capacityOfTrees;		// capacity of tree

struct node* tree;				// tree array

struct useInfo
{
	int inst;
	struct useInfo* next;
};

struct node
{
	int op;						// operation node
	int left;					// left operand
	int right;					//right operand

	void* sym;

	int label;

	int leftType;
	int rightType;
	int baseAddr;

	int blockIndex;
	
	struct useInfo live;		// use information
	struct useInfo unlive;
	int dt[REG_V0-REG_A0+1];	// is there value in reg
	int isNew;				// is there new value in the offset
	int stableSize;			// size of local variable
	int offset;				// offset to $fp
	int storePos;
};

//manage postfix add/plus i++,i--

struct node ppList[256];
int ppSize=0;

char rn[REG_RA-REG_A0+1][4]={"$a0","$a1","$a2","$a3","$t0","$t1","$t2","$t3","$t4","$t5","$t6",
"$t7","$t8","$t9","$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7","$v1","$v0","$0","$sp","$fp","$gp","$ra"};

// struct to store machine code 
struct mCode
{
	//char* dec;			// for global declaration 
	int instr;			// instruction
	int rd;				//register number of rd
	int rs;				//register number of rs
	int rt;				//register number of rt
};

struct xOp
{
	int inst;
	struct xOp* next;
};

struct regDescriptor
{
	struct xOp* operand;
};

struct mCode* codeBuffer;

int codeBufferSize=0;

int codeBufferCapacity=2048;

struct regDescriptor regFile[REG_V0-REG_A0+1];
int regV1Des;
int addr[2];


void addToPPList(int op,int left,int right,struct symbol* sym);

void addVarToReg(int i,int j);

int newCode(int op,int left,int right,void* sym);

int nextInstruction();

void initTree();


struct relList
{
	int index;
	struct relList* next;
};

struct relList* merge(struct relList* l0,struct relList* l1);

void backPatch(struct relList* l,int i);

void freeList(struct relList* l0,struct relList* l1);

int defaultBP(struct relList* t1,struct relList* f1,int c1);

int defaultBP2(int op,int c1,int c3,int g1,int g3);

void addToPPList(int op,int left,int right,struct symbol* sym);

void updatePPList();

void exchange(int s1,int m,int e2);

void reduceGoto();

void markBlock();

//calculate consrant result in compile time
//
void repConstant();

//********************************************************************************************************
//								the following is the machine code generate
//
//
//



void initReg();

void updateUseInfo();

void addUseInfo(struct useInfo* nd,int k);

int needToBeSave(int i,int j);

void updateTmpReg(int pos);

int readRegByIndex(int i,int origin,int excep);

//clear the corresponding variable descriptor
void unset(struct xOp* dt,int r,int excep);

int readRegByValue(int v);

void getAddr(int i,int excep);

int sizeOfTmp(int i);

int writeRegByIndex(int i,int ori);

void addCode(int inst,int rd,int rs,int rt);

void genCode();

void printCode();

void output();

void dispReg();

char*	labelStr[2048];
int labelSize=0;

FILE* cFile; 
extern struct functionDef* functionList;

//clear the register content when leave the function block
void flushTmp(int i)
{
	int j=i;
	while(tree[--j].op!=MFUNCTION);			//find end and start of function
	int r=REG_A0;
	for(;r<=REG_V0;++r)
	{
		struct xOp* tp=regFile[r-REG_A0].operand;	//save the global variable that are dirty
		while(tp->next!=NULL)
		{
			if(tree[tp->next->inst].storePos==GLB&&!tree[tp->next->inst].isNew)
			{
				getAddr(tp->next->inst,-1);
				addCode(SW,readRegByIndex(tp->next->inst,i,-1),addr[0],addr[1]);
			}
			tp=tp->next;
		}
		regFile[r-REG_A0].operand->next=NULL;
	}
	regV1Des=0;
}

int getSizeOfPara(int i)
{
	return ((struct functionDef*)tree[i].sym)->numOfPara;
}

//get space need for local variable
int getSizeOfLocal(int i)
{
	int sum=0;
	while(tree[i++].op!=MEND)
	{
		switch(tree[i].op)
		{
			case MDEC_INT:
			case MINIT_INT:sum++;break;
			case MDEC_AREA:
			case MINIT_AREAH:sum+=tree[i].left/4;break;
		}
	}
	return sum;
}

void freeReg(struct xOp* p)
{}

//add jump label of branch statement
int addLabel(char* str,int i)
{
	int j=0;
	if(str!=NULL)					// first check the label has been used or not
	{
		for(;j<labelSize;++j)
			if(strcmp(labelStr[j],str)==0)
				return j;
		labelStr[labelSize++]=str;
		return labelSize-1;
	}
	char buf[10];
	sprintf(buf,"L%d",i);		// add new label start with 'L' and a number
	for(j=0;j<labelSize;++j)
		if(strcmp(labelStr[j],buf)==0)
			return j;
	labelStr[labelSize++]=strdup(buf);
	return labelSize-1;
}

//add use infomation of register for a 3 address code
void addVarToReg(int i,int j)
{
	struct xOp* tmp=regFile[i].operand;
	while((tmp=tmp->next)!=NULL)
		if(tmp->inst==j)
			return;
	tmp=malloc(sizeof(struct xOp));
	tmp->next=regFile[i].operand->next;
	regFile[i].operand->next=tmp;
	tmp->inst=j;
}

int skipBlock(int i)
{
	return tree[i].op==MFUNCTION||tree[i].op==MEND||tree[i].op==MIF||tree[i].op==MGOTO||tree[i].op==MRETURN;
}

int newCode(int op,int left,int right,void* sym)
{
	if(left==-1&&right==-1)
		return -1;

	if(sizeOfTrees==capacityOfTrees)
	{
		capacityOfTrees*=2;
		struct node* t=malloc(sizeof(struct node)*capacityOfTrees);
		int i=0;
		memcpy(t,tree,sizeof(struct node)*sizeOfTrees);
		free(tree);
		tree=t;
	}

	tree[sizeOfTrees].op=op;
	tree[sizeOfTrees].left=left;
	tree[sizeOfTrees].right=op>=MDEC_INT&&op<=MINIT_AREAH?0:right;
	tree[sizeOfTrees].sym=sym;
	tree[sizeOfTrees].isNew=1;
	tree[sizeOfTrees].live.next=tree[sizeOfTrees].unlive.next=NULL;

	if(op>=8&&op<=25)				// is arithmetic operation
	{								// find common sub exp
		int i=sizeOfTrees-1;
		while(i>=0)
		{
			if(tree[i].op==op&&tree[i].left==left&&tree[i].right==right)break;
			if(skipBlock(i))break;
			i--;
		}
		if(i<0||skipBlock(i))
		{
			sizeOfTrees++;
			return sizeOfTrees-1;
		}
		else
			return i;
	}
	
	switch(op)
	{
		case MINIT_AREAH:
		case MINIT_INT:tree[sizeOfTrees].isNew=1;
		case MDEC_AREA:
		case MDEC_INT:sizeOfTrees=right==0?sizeOfTrees-1:sizeOfTrees;break;
		case MDEC:
		case MINC:addToPPList(op,left,right,NULL);return left;
		case MTMP:
		case MINT:tree[sizeOfTrees].left=left;break;
	}
	
	if(op==MID&&right>=-1)
	{
		int i=sizeOfTrees-1,count=0;
		struct symbol* ts=(struct symbol*)sym;
		if(ts->type==ARRAY_STRUCT||ts->type==ARRAY_INT)
		{
			if(right==0)//indicate it's a valid decomposed of pointer,-1 otherwise
			{
				while(i>=0)
				{
					if(sym==tree[i].sym&&(left==tree[i].right&&count==0||tree[i].op==MDEC_AREA||tree[i].op==MINIT_AREAH))
						break;
					if(skipBlock(i))count++;
					--i;
				}
				if(left==tree[i].right&&count==0)
					return i;
				tree[sizeOfTrees].left=i;
				tree[sizeOfTrees].right=left;			//depend on tree[left]
			}
			sizeOfTrees++;
			return sizeOfTrees-1;
		}
		if(ts->type==VARIABLE_STRUCT)
			return 0;
		while(i>=0)
		{
			if((ts->type==VARIABLE_INT&&count==0||tree[i].op==MDEC_INT||tree[i].op==MINIT_INT||ts->isPara)&&sym==tree[i].sym)
				break;
			if(skipBlock(i))count++;
			--i;
		}
		if(count==0&&tree[i].op==MID||ts->isPara)
			return i;
		tree[sizeOfTrees].left=i;
		tree[sizeOfTrees].right=0;
	}
	if(op==MDOT)
	{
		if(left<0)
			return -1;
		int i=sizeOfTrees,count=0;
		if(((struct symbol*)sym)->type==VARIABLE_STRUCT)
		{
			while(--i>0)
			{
				if(tree[i].sym==sym&&(tree[i].op==MDOT&&tree[i].right==right&&count==0||tree[i].op==MDEC_AREA))
					break;
				if(skipBlock(i))count++;
			}
			if(tree[i].op==MDOT&&count==0)
				return i;
			tree[sizeOfTrees].left=i;
			tree[sizeOfTrees].right=right;
		}
		else
		{
			while(--i>0)
			{
				if(tree[i].sym==sym&&tree[i].op==MDOT&&tree[i].left==left&&tree[i].right==right)
					break;
				if(skipBlock(i))count++;
			}
			if(count==0&&tree[i].op==MDOT)
				return i;
			tree[sizeOfTrees].left=left;
			tree[sizeOfTrees].right=right;
		}
	}
	sizeOfTrees++;
	return sizeOfTrees-1;
}

int nextInstruction()
{
	return sizeOfTrees;
}

void initTree()
{
	sizeOfTrees = 1;
	capacityOfTrees = INIT_TREE_SIZE;
	tree = malloc(sizeof(struct node) * capacityOfTrees);
	ppSize=0;
	tree[0].op=MNOP;
}

struct relList* makeList(int x)
{
	struct relList* tmp=malloc(sizeof(struct relList));
	tmp->index=x;
	tmp->next=NULL;
	return tmp;
}

//merge false list and true list of jump
struct relList* merge(struct relList* l0,struct relList* l1)
{
	if(l0==NULL)return l1;
	if(l1==NULL)return l0;
	struct relList* tmp=l0;
	while(tmp->next!=NULL)
		tmp=tmp->next;
	tmp->next=l1;
	return l0;
}

void backPatch(struct relList* l,int i)
{
	while(l!=NULL)
	{
		switch(tree[l->index].op)
		{
			case MGOTO:	tree[l->index].left=i;break;
			case MIF:tree[l->index].right=i;break;
		}
		l=l->next;
	}
}

void freeList(struct relList* l0,struct relList* l1)
{
	struct relList* tmp;
	if(l0!=NULL)
	{
		tmp=l0->next;
		while(l0!=NULL)
		{
			tmp=l0->next;
			free(l0);
			l0=tmp;
		}
	}
	if(l1!=NULL)
	{
		tmp=l1->next;
		while(l1!=NULL)
		{
			tmp=l1->next;
			free(l1);
			l1=tmp;
		}
	}
}

//default backpatch for arithmetic exp
int defaultBP(struct relList* t1,struct relList* f1,int c1)
{
	if(c1==-1)return -1;
	int rv,tp;
	if(tree[c1].op==MLOGOR||tree[c1].op==MLOGAND||tree[c1].op==MLOGNOT)
	{
		backPatch(f1,nextInstruction());
		tp=newCode(MINT,1,0,NULL);
		newCode(MINT,0,0,NULL);
		rv=newCode(MTMP,0,-1,NULL);
		newCode(MASSIGN,rv,tp+1,NULL);
		newCode(MGOTO,nextInstruction()+2,-1,NULL);
		backPatch(t1,nextInstruction());
		newCode(MASSIGN,rv,tp,NULL);
		freeList(f1,t1);
		return rv;
	}
	return -1;
}

int defaultBP2(int op,int c1,int c3,int g1,int g3)
{
	if(g1==-1&&g3==-1)
		return newCode(op,c1,c3,NULL);
	else if(g1!=-1&&g3==-1)
		return newCode(op,g1,c3,NULL);
	else if(g1==-1&&g3!=-1)
		return newCode(op,c1,g3,NULL);
	return newCode(op,g1,g3,NULL);
}

void addToPPList(int op,int left,int right,struct symbol* sym)
{
	ppList[ppSize].op=op;
	ppList[ppSize].left=left;
	ppList[ppSize].right=right;
	ppList[ppSize].sym=sym;
	ppSize++;
}

void updatePPList()
{
	int i=0;
	for(;i<ppSize;++i)
	{
		if(ppList[i].op==MDEC_PRE)
			newCode(MDEC_PRE,ppList[i].left,ppList[i].right,ppList[i].sym);
		else
			newCode(MINC_PRE,ppList[i].left,ppList[i].right,ppList[i].sym);
	}
	ppSize=0;
}

//change the position of statement of for loop
void exchange(int s1,int m,int e2)
{
	if(m==s1||m==e2)
		return;
	if(s1>e2)
		exchange(e2,m,s1);
	struct node* buffer=malloc(sizeof(struct node)*(e2-s1));
	int i,j;
	for(i=s1;tree[i].op==MID||tree[i].op==MDOT;++i);
	for(j=i;i<m;++i)
	{
		if(tree[i].op==MID||tree[i].op==MDOT)
		{
			int tmp=tree[j].left;tree[j].left=tree[i].left;tree[i].left=tmp;
			tmp=tree[j].right;tree[j].right=tree[i].right;tree[i].right=tmp;
			tmp=tree[j].op;tree[j].op=tree[i].op;tree[i].op=tmp;
			struct symbol* ts=tree[j].sym;tree[i].sym=tree[j].sym;tree[j].sym=ts;
			for(tmp=i+1;tmp<e2;++tmp)
			{
				if(tree[tmp].op==MINT||tree[tmp].op==MTMP||tree[i].op==MID||
					tree[tmp].op>=MDEC_INT&&tree[tmp].op<=MINIT_AREAH)continue;
				if(tree[tmp].left==i)tree[tmp].left=j;
				if(tree[tmp].right==i&&tree[tmp].op!=MDOT&&tree[tmp].op!=MIF)tree[tmp].right=j;
			}
			j++;
		}
	}
	int tem=j;
	for(i=tem,j=0;i<e2;i++,j++)
	{
		if(tree[i].op==MINT||tree[i].op==MTMP||tree[i].op==MDEC_AREA||
				tree[i].op==MINIT_AREAH||tree[i].op==MCALL)
		{
			memcpy(&buffer[j],&tree[i],sizeof(struct node));
			continue;
		}
		buffer[j].op=tree[i].op;
		if(tree[i].left<=e2&&tree[i].left>=m)
			buffer[j].left=tree[i].left-(m-tem);
		else if(tree[i].left<=m&&tree[i].left>=tem)
			buffer[j].left=tree[i].left+e2-m;
		else
			buffer[j].left=tree[i].left;
			
		if(tree[i].right<=e2&&tree[i].right>=m)
			buffer[j].right=tree[i].right-(m-tem);
		else if(tree[i].right<=m&&tree[i].right>=tem)
			buffer[j].right=tree[i].right+e2-m;
		else
			buffer[j].right=tree[i].right;
		buffer[j].sym=tree[i].sym;
	}	
	for(i=tem,j=m-tem;i<e2-m+tem;++i,++j)
		memcpy(&tree[i],&buffer[j],sizeof(struct node));
	for(j=0;i<e2;++i,++j)
		memcpy(&tree[i],&buffer[j],sizeof(struct node));
}

//reduce if and the following goto
void reduceGoto()
{
	int i=0;
	for(;i<sizeOfTrees;++i)
	{
		if(tree[i].op==MIF&&tree[i+1].op==MGOTO)
		{
			if(tree[i].right==i+2)
			{
				tree[i].right=tree[i+1].left;			//i:	IF	a	i+2		IFFALSE	a	m
				tree[i].op=MIFFALSE;					//i+1:	GOTO	m		NOP
				tree[i+1].op=MNOP;						//i+2:
			}
		}
		tree[i].label=0;
		if(tree[i].op!=MINT&&tree[i].op!=MEND)
			tree[i].leftType=tree[i].rightType=REF;
		else tree[i].leftType=VAL;
	}
}

//mark basic block
void markBlock()
{
	int i,flag=0;
	for(i=0;i<sizeOfTrees;++i)
	{
		tree[i].isNew=1;
		if((tree[i].op==MIF||tree[i].op==MIFFALSE||tree[i].op==MGOTO)&&flag)
		{
			int dest=tree[i].op==MIF||tree[i].op==MIFFALSE?tree[i].right:tree[i].left;
			tree[dest].label=1;
			tree[i+1].label=1;
		}
		if(tree[i].op==MCALL)
			tree[i+1].label=1;
		if(tree[i].op==MFUNCTION)
			tree[i].label=1;
		if(tree[i].op==MFUNCTION)flag=1;
		if(tree[i].op==MEND)flag=0;
	}
	int j=0;
	for(i=1;i<sizeOfTrees;++i)
	{
		if(tree[i].label==1)j++;
		tree[i].blockIndex=j;
	}
}

//calculate consrant result in compile time
//
void repConstant()
{
	int i=0,ofs=0,flag=1;
	ppSize=0;
	ppList[ppSize].op=0;
	for(ofs=0;i<sizeOfTrees;++i)
	{
		if(flag&&(tree[i].op==MIF||tree[i].op==MIFFALSE||tree[i].op==MGOTO))		// flush the jump statement in global domain
			tree[i].op=MNOP;
		if((tree[i].op==MIF||tree[i].op==MIFFALSE)&&tree[i].leftType==REF&&tree[tree[i].left].op==MLOGNOT)
		{															//check for logic jump
			tree[i].op=tree[i].op==MIF?MIFFALSE:MIF;
			tree[i].leftType=tree[tree[i].left].leftType;
			tree[i].left=tree[tree[i].left].left;
		}
		if(tree[i].op>=MMUL&&tree[i].op<=MMINUS_PRE||tree[i].op==MCALL||tree[i].op==MTMP)
		{															//calculate the offset of temporary 
			tree[i].offset=ofs;
			ofs+=4;
		}
		if(tree[i].op==MGREATEQ||tree[i].op==MGREAT)
		{												//change great relation to less
			tree[i].op=tree[i].op==MGREAT?MLESS:MLESSEQ;
			int t=tree[i].left;tree[i].left=tree[i].right;tree[i].right=t;
			t=tree[i].leftType;tree[i].leftType=tree[i].rightType;tree[i].rightType=i;
		}
		if(tree[i].op==MINC_PRE||tree[i].op==MDEC_PRE)
		{												//change inc to plus
			tree[i].right=tree[i].op==MINC_PRE?1:-1;
			tree[i].op=MPLUSASS;
			tree[i].rightType=VAL;
		}

		if(tree[i].op==MFUNCTION)
		{
			ppList[++ppSize].op=0;
			ofs=0;
			flag=0;
		}
		if(tree[i].op==MEND)
			flag=1;
		if(tree[i].op==MID||tree[i].op==MDOT)			//mark variable store position
			tree[i].storePos=tree[tree[i].left].storePos;
		if(tree[i].op==MDOT&&((struct symbol*)tree[i].sym)->type==VARIABLE_STRUCT)
			tree[i].left=tree[tree[i].left].baseAddr;

		//calculate base address for variable base on global or stack address 
		if(tree[i].op>=MDEC_INT&&tree[i].op<=MINIT_AREAH)
		{
			tree[i].baseAddr=ppList[ppSize].op;
			ppList[ppSize].op+=tree[i].op==MDEC_INT||tree[i].op==MINIT_INT?4:tree[i].left;
			tree[i].storePos=flag==1?GLB:STK;
			if(tree[i].leftType==REF&&tree[i].op==MINIT_INT&&tree[tree[i].left].leftType==VAL&&tree[tree[i].left].op==MINT)
			{
				tree[i].left=tree[tree[i].left].left;
				tree[i].leftType=VAL;
			}
		}
		else if(tree[i].op==MID&&((struct symbol*)tree[i].sym)->isPara)
			ppList[ppSize].op+=4;
		else
		{
			if(tree[i].left>0&&tree[i].leftType==REF&&tree[tree[i].left].op==MERROR)
			{
EX:
				fprintf(stderr,"warning:divide by zero\n");exit(0);
			}
			else if(tree[i].right>0&&tree[i].rightType==REF&&tree[tree[i].right].op==MERROR)
			{
				if(tree[i].op==MLOGAND&&tree[i].leftType==VAL)
				{
					if(tree[i].left==0)
					{
						tree[i].op=MINT;tree[i].left=0;tree[i].right=0;
					}
					else goto EX;
				}
				else if(tree[i].op==MLOGOR&&tree[i].leftType==VAL)
				{
					if(tree[i].left!=0)
					{
						tree[i].op=MINT;tree[i].left=1;tree[i].right=0;
					}
					else goto EX;
				}
				else if(tree[i].op!=MLOGOR&&tree[i].op!=MLOGAND)
					goto EX;
			}
			//calculate result of constant arithmetic operation
			if(tree[i].left>=0&&tree[i].leftType==REF&&tree[i].op!=MGOTO)
			{
				if(tree[tree[i].left].op==MINT)
				{
					tree[i].left=tree[tree[i].left].left;
					tree[i].leftType=VAL;
				}
				else if(tree[i].op==MID&&tree[tree[i].left].op>=MDEC_INT&&tree[tree[i].left].op<=MINIT_AREAH)
				{
					tree[i].left=tree[tree[i].left].baseAddr;
					tree[i].leftType=VAL;
				}
			}

			if(tree[i].right>=0&&tree[i].rightType==REF&&tree[tree[i].right].op==MINT&&tree[i].op!=MIF&&tree[i].op!=MIFFALSE)
			{
				tree[i].right=tree[tree[i].right].left;
				tree[i].rightType=VAL;
			}

			if(tree[i].op==MID&&((struct symbol*)tree[i].sym)->type==ARRAY_STRUCT&&tree[tree[i].left].op==MDEC_AREA)
			{
				if(tree[tree[i].right].op==MINT)
				{
					tree[i].rightType=VAL;tree[i].right=tree[tree[i].right].left;
				}
			}
			

			if(tree[i].leftType==VAL&&tree[i].rightType==VAL)
			{
				int err=0;
				switch(tree[i].op)
				{
					case MMUL:tree[i].left*=tree[i].right;break;
					case MDIV:tree[i].right!=0?tree[i].left/=tree[i].right:(err=1);break;
					case MMOD:tree[i].right!=0?tree[i].left%=tree[i].right:(err=1);break;
					case MPLUS:tree[i].left+=tree[i].right;break;
					case MMINUS:tree[i].left-=tree[i].right;break;
					case MSHL:tree[i].left<<=tree[i].right;break;
					case MSHR:tree[i].left>>=tree[i].right;break;
					case MGREAT:tree[i].left=(tree[i].left>tree[i].right);break;
					case MGREATEQ:tree[i].left=(tree[i].left>=tree[i].right);break;
					case MLESS:tree[i].left=(tree[i].left<tree[i].right);break;
					case MLESSEQ:tree[i].left=(tree[i].left<=tree[i].right);break;
					case MEQUAL:tree[i].left=(tree[i].left==tree[i].right);break;
					case MNOEQUAL:tree[i].left=(tree[i].left!=tree[i].right);break;
					case MBITAND:tree[i].left&=tree[i].right;break;		//**********	arithmetic	****************
					case MBITXOR:tree[i].left^=tree[i].right;break;
					case MBITOR:tree[i].left|=tree[i].right;break;
					case MLOGAND:tree[i].left=tree[i].left*tree[i].right?1:0;break;
					case MLOGOR:tree[i].left=tree[i].left+tree[i].left?1:0;break;
				}
				if(tree[i].op>=MMUL&&tree[i].op<=MBITOR||tree[i].op==MLOGAND||tree[i].op==MLOGOR)
				{
					tree[i].op=MINT;tree[i].right=0;
				}
				if(err)
					tree[i].op=MERROR;
			}
			if(tree[i].leftType==VAL)
			{
				if(tree[i].op==MBITNOT)
				{
					tree[i].left=!tree[i].left;
					tree[i].op=MINT;
				}
				else if(tree[i].op==MMINUS_PRE)
				{
					tree[i].left=-tree[i].left;
					tree[i].op=MINT;
				}
			}
		}
	}

	for(i=0;i<sizeOfTrees;++i)
	{
		if(tree[i].op!=MID&&tree[i].op!=MDOT)continue;
		struct symbol* ts=(struct symbol*)tree[i].sym;
		if(ts->type!=ARRAY_INT&&ts->type!=ARRAY_STRUCT)continue;
		int j=i;
		for(;j<sizeOfTrees&&tree[j].label!=1;++j)
		{
			if(tree[j].sym==ts&&tree[j].left==tree[i].left&&tree[i].rightType==tree[j].rightType&&tree[i].right==tree[j].right)
			{
				for(flag=j;flag<sizeOfTrees&&tree[flag].label!=1;++flag)
					if(tree[flag].leftType==REF&&tree[flag].left==j)tree[flag].left=i;
					else if(tree[flag].rightType==REF&&tree[flag].right==j)tree[flag].right=i;
			}
		}
	}
}



//********************************************************************************************************
//								the following is the machine code generate
//
//
//


//initial register
void initReg()
{
	int i=REG_A0;
	regV1Des=0;
	for(;i<=REG_V0;++i)
	{
		regFile[i-REG_A0].operand=malloc(sizeof(struct xOp));
		regFile[i-REG_A0].operand->next=NULL;
	}
	codeBuffer=malloc(sizeof(struct mCode)*codeBufferCapacity);
	codeBufferSize=0;
}

void updateUseInfo()
{
	int i,j=1,k;
	for(i=sizeOfTrees-1;i>=0;--i)
	{
		for(k=0;k<=REG_V0-REG_A0;++k)
			tree[i].dt[k]=0;
		j=tree[i].op;
		if(j==MID&&(((struct symbol*)tree[i].sym)->type==ARRAY_INT||((struct symbol*)tree[i].sym)->type==ARRAY_STRUCT))
			if(tree[i].rightType==REF)
			{
				addUseInfo(&tree[tree[i].right].live,i);
				struct useInfo* tu=&tree[i].live;
				while((tu=tu->next)!=NULL)
					addUseInfo(&tree[tree[i].right].live,tu->inst);
				tu=&tree[i].unlive;
				while((tu=tu->next)!=NULL)
					addUseInfo(&tree[tree[i].right].live,tu->inst);
			}

		if(j==MDOT&&((struct symbol*)tree[i].sym)->type==ARRAY_STRUCT)
			if(tree[i].leftType==REF)addUseInfo(&tree[tree[i].left].live,i);
		if(j>=MMUL&&j<=MMINUS_PRE||j>=MPLUSASS&&j<=MINC_PRE||j==MPARAM||
				j>=MRETURN&&j<=MGOTO||j==MIFFALSE)
		{
			if(j>=MPLUSASS&&j<=MINC_PRE)
				addUseInfo(&tree[tree[i].left].unlive,i);
			else if(tree[i].leftType==REF&&tree[i].left>0)
				addUseInfo(&tree[tree[i].left].live,i);
			if(tree[i].rightType==REF&&tree[i].right>0&&j!=MIF&&j!=MIFFALSE)
				addUseInfo(&tree[tree[i].right].live,i);
		}
		if(j==MCALL)
		{
			int pram=getSizeOfPara(i);
			for(k=i-1,j=0;pram>0;--k)
			{
				if(tree[k].op==MPARAM)
				{
					if(j==0){addUseInfo(&tree[k].live,i);pram--;}
					else --j;
				}
				else if(tree[k].op==MCALL)
					j=getSizeOfPara(k);
			}
		}
	}
}

void addUseInfo(struct useInfo* nd,int k)
{
	struct useInfo* t=malloc(sizeof(struct useInfo));
	t->next=nd->next;
	t->inst=k;
	nd->next=t;
}

//check for live register
int isLive(int i,int origin)
{
	struct useInfo* tmp=&tree[i].live;
	int nextUse=sizeOfTrees,unUse=sizeOfTrees;
	while(tmp!=NULL)
	{
		if(tmp->inst<nextUse&&tmp->inst>=origin)
			nextUse=tmp->inst;
		tmp=tmp->next;
	}
	tmp=&tree[i].unlive;
	while(tmp!=NULL)
	{
		if(tmp->inst<unUse&&tmp->inst>origin)
			unUse=tmp->inst;
		tmp=tmp->next;
	}
	if(unUse<nextUse||nextUse==sizeOfTrees)
		return 0;
	return 3;
}

int needToBeSave(int i,int j)
{
	if(tree[i].sym!=NULL)
		return tree[i].isNew==0;
	return isLive(i,j);
}

void updateTmpReg(int pos)
{
	int j;
	struct xOp* tmp;
	for(j=REG_A0;j<=REG_V0;++j)
	{
		tmp=regFile[j-REG_A0].operand;
		while(tmp->next!=NULL)
		{
			if(needToBeSave(tmp->next->inst,pos+1))
			{
				getAddr(tmp->next->inst,j);
				addCode(SW,j,addr[0],addr[1]);
				tree[tmp->next->inst].isNew=1;
			}
			tree[tmp->next->inst].dt[j-REG_A0]=0;
			tmp=tmp->next;
		}
		regFile[j-REG_A0].operand->next=NULL;
	}
}

int existValue(int i,int j)
{
	int x=0;
	for(;x<=REG_S7-REG_A0;++x)
		if(tree[i].dt[x]==1&&x!=j)
			return 1;
	return 0;
}

//read an value of node, if not in register
//load it from memory
int readRegByIndex(int i,int origin,int excep)
{
	int j;

	//select a reg contains i
	for(j=0;j<=REG_V0-REG_A0;++j)
		if(tree[i].dt[j]!=0)
			return j+REG_A0;

	struct xOp* tmp;
	struct symbol* ts=(struct symbol*)tree[i].sym;
	if(tree[i].op==MID&&ts->isPara>0&&ts->isPara<=4)
	{
		tmp=regFile[ts->isPara-1].operand;
		while(tmp->next!=NULL)
		{
			if(needToBeSave(tmp->next->inst,origin))
			{
				getAddr(tmp->next->inst,excep);
				addCode(SW,ts->isPara-1+REG_A0,addr[0],addr[1]);
				tree[tmp->next->inst].isNew=1;
			}
			tmp=tmp->next;
		}
		unset(regFile[ts->isPara-1].operand,ts->isPara-1,i);
		getAddr(i,excep);
		addCode(LW,ts->isPara-1+REG_A0,addr[0],addr[1]);
		tree[i].dt[ts->isPara-1]=1;
		addVarToReg(ts->isPara-1,i);
		return ts->isPara-1+REG_A0;
	}


	//select a empty reg
	for(j=REG_S7;j>=REG_T0;--j)
		if(regFile[j-REG_A0].operand->next==NULL)
		{
			regFile[j-REG_A0].operand->next=malloc(sizeof(struct xOp));
			goto select;
		}

//the worst case, need to replace the exist register
	int minr,minv=100000;
	for(j=REG_S7;j>=REG_T0;--j)
	{
		if(j==excep)continue;
		// find an reg whose variables have been saved in other places
		tmp=regFile[j-REG_A0].operand;
		int countForNeedToSave=0;
		while(tmp->next!=NULL)
		{
			if(!existValue(tmp->next->inst,j-REG_A0)&&tmp->next->inst!=i)
				countForNeedToSave++;
			tmp=tmp->next;
		}
		if(countForNeedToSave==0)
		{
			unset(regFile[j-REG_A0].operand,j-REG_A0,i);
			freeReg(regFile[j-REG_A0].operand->next);
			goto select;
		}
		if(countForNeedToSave<minv)
		{
			minv=countForNeedToSave;
			minr=j;
		}

		//find an reg which is exactly the destination of instruction
		if(tree[origin].op>=MPLUSASS&&tree[origin].op<=MINC_PRE&&tree[tree[origin].left].dt[j-REG_A0])
		{
			tmp=regFile[j-REG_A0].operand;
			//update value in saved in this register and save it in the stack
			while(tmp->next!=NULL)
			{
				if(needToBeSave(tmp->next->inst,origin)&&tmp->next->inst!=i)
				{
					tree[tmp->next->inst].isNew=1;
					getAddr(tmp->next->inst,excep);
					addCode(SW,j,addr[0],addr[1]);
				}
				tmp=tmp->next;
			}
			unset(regFile[j-REG_A0].operand,j-REG_A0,i);
			goto select;
		}

		//find an unlived register
		tmp=regFile[j-REG_A0].operand;
		countForNeedToSave=0;
		while(tmp->next!=NULL)
		{
			if(isLive(tmp->next->inst,origin)||needToBeSave(tmp->next->inst,origin))	/*use occur behind write back*/
				countForNeedToSave++;
			tmp=tmp->next;
		}
		if(countForNeedToSave==0)

			goto select;
		if(countForNeedToSave<minv)
		{
			minv=countForNeedToSave;
			minr=j;
		}
	}
	//select a least save register
	j=minr;
	tmp=regFile[j-REG_A0].operand;
	while(tmp->next!=NULL)
	{
		if(needToBeSave(tmp->next->inst,origin))
		{
			getAddr(tmp->next->inst,excep);
			addCode(SW,j,addr[0],addr[1]);
			tree[tmp->next->inst].isNew=1;
		}
		tmp=tmp->next;
	}
	unset(regFile[j-REG_A0].operand,j-REG_A0,i);
	freeReg(regFile[j-REG_A0].operand->next);
select:
	getAddr(i,excep);
	addCode(LW,j,addr[0],addr[1]);
	tree[i].dt[j-REG_A0]=1;
	regFile[j-REG_A0].operand->next->next=NULL;
	regFile[j-REG_A0].operand->next->inst=i;
	return j;
}

//clear the corresponding variable descriptor
void unset(struct xOp* dt,int r,int excep)
{
	while(dt->next!=NULL)
	{
		if(dt->next->inst!=excep)
			tree[dt->next->inst].dt[r]=0;
		dt=dt->next;
	}
}

int readRegByValue(int v)
{
	if(v==0) return REG_0;
	if(v==regV1Des) return REG_V1;
	addCode(LI,REG_V1,v,0);
	regV1Des=v;
	return REG_V1;
}


void getAddr(int i,int excep)
{
	if(tree[i].op==MDEC_INT||tree[i].op==MINIT_INT)
	{
		addr[0]=tree[i].storePos==GLB?REG_GP:REG_FP;
		addr[1]=tree[i].storePos==GLB?tree[i].baseAddr:0-tree[i].baseAddr-4;
	}
	if(tree[i].op==MID)
	{
		struct symbol* ts=tree[i].sym;
		switch(ts->type)
		{
			case VARIABLE_INT:
				if(!ts->isPara)
				{
					if(tree[i].storePos==GLB){ addr[0]=REG_GP; addr[1]=tree[i].left;}
					else if(tree[i].sym!=NULL){ addr[0]=REG_FP;addr[1]=0-tree[i].left-4;}
					else {addr[0]=REG_SP;addr[1]=0-tree[i].offset;}
				}
				else if(ts->isPara<=4)
					{addr[0]=REG_FP; addr[1]=(ts->isPara-ppList[ppSize-1].right-1)*4;}
				else
					{addr[0]=REG_FP; addr[1]=(ts->isPara-4+1)*4;}
				break;
			case ARRAY_INT:
					if(tree[i].rightType==VAL)
					{
						addr[0]=tree[i].storePos==GLB?REG_GP:REG_FP;
						addr[1]=tree[i].storePos==GLB?tree[i].left+tree[i].right*4:0-tree[i].left-tree[i].right*4-4;
					}
					else
					{
						addCode(SLL,REG_V1,readRegByIndex(tree[i].right,i,excep),2);
						tree[i].storePos==GLB?addCode(ADD,REG_V1,REG_V1,REG_GP):addCode(SUB,REG_V1,REG_FP,REG_V1);
						addr[0]=REG_V1;addr[1]=tree[i].storePos==GLB?tree[i].left:0-tree[i].left-4;
						regV1Des=0;
					}
					break;
		}
	}
	
	if(tree[i].op==MDOT)
	{
		switch(((struct symbol*)tree[i].sym)->type)
		{
			case VARIABLE_STRUCT:
				addr[0]=tree[i].storePos==GLB?REG_GP:REG_FP;
				addr[1]=tree[i].storePos==GLB?tree[i].left+4*tree[i].right:0-tree[i].left-4*tree[i].right;
				break;
			case ARRAY_STRUCT:
				if(tree[tree[i].left].rightType==VAL)
				{
					addr[0]=tree[i].storePos==GLB?REG_GP:REG_FP;
					int ofs=tree[tree[i].left].left+4*(tree[tree[i].left].right*((struct symbol*)tree[i].sym)->structDefLoc->varNum+tree[i].right);
					addr[1]=tree[i].storePos==GLB?ofs:0-ofs-4;
				}
				else
				{
					addCode(MULI,REG_V1,readRegByIndex(tree[tree[i].left].right,i,excep),((struct symbol*)tree[i].sym)->structDefLoc->varNum*4);
					tree[i].storePos==GLB?addCode(ADD,REG_V1,REG_V1,REG_GP):addCode(SUB,REG_V1,REG_FP,REG_V1);
					addr[0]=REG_V1;
					addr[1]=tree[i].storePos==GLB?tree[tree[i].left].left+tree[i].right*4:0-tree[tree[i].left].left-tree[i].right*4-4;
					regV1Des=0;
				}
				break;
		}
	}
	if(tree[i].op>=MMUL&&tree[i].op<=MMINUS_PRE||tree[i].op==MCALL||tree[i].op==MTMP)
	{
		addr[0]=REG_SP;addr[1]=0-tree[i].offset-4;
	}
	if(tree[i].op==MPARAM)
	{
		addr[0]=REG_SP;addr[1]=0-tree[tree[i].left].offset-4;
	}
}

int sizeOfTmp(int i)
{
	int rt=0;
	for(;i>1;--i)
		if(tree[i].op>=MMUL&&tree[i].op<=MMINUS_PRE||tree[i].op==MCALL||tree[i].op==MTMP)
			++rt;
		else if(tree[i].op==MFUNCTION) break;

	return rt;
}

int writeRegByIndex(int i,int origin)
{
	int j;
	struct xOp* tmp;
	//select a reg contains i
	for(j=0;j<=REG_S7-REG_A0;++j)
		if(tree[i].dt[j]!=0)
		{
			tmp=regFile[j].operand;
			while(tmp->next!=NULL)
			{
				//other value may save in this register,update these value
				if(needToBeSave(tmp->next->inst,origin)&&tmp->next->inst!=i)
				{
					getAddr(tmp->next->inst,j);
					addCode(SW,j+REG_A0,addr[0],addr[1]);
					tree[tmp->next->inst].isNew=1;
					tree[tmp->next->inst].dt[j]=0;
				}
				tmp=tmp->next;
			}
			return j+REG_A0;
		}
	
	//select a empty reg
	for(j=REG_S7;j>=REG_T0;--j)
		if(regFile[j-REG_A0].operand->next==NULL)
		{
			regFile[j-REG_A0].operand->next=malloc(sizeof(struct xOp));
			goto select;
		}
	int minr,minv=100000;
	for(j=REG_S7;j>=REG_T0;--j)
	{
		tmp=regFile[j-REG_A0].operand;
		// find an reg whose variables have been saved in other places
		int countForNeedToSave=0;
		while(tmp->next!=NULL)
		{
			if(!existValue(tmp->next->inst,j-REG_A0))
				countForNeedToSave++;
			tmp=tmp->next;
		}
		if(countForNeedToSave==0)
		{
			unset(regFile[j-REG_A0].operand,j-REG_A0,i);
			freeReg(regFile[j-REG_A0].operand->next);
			goto select;
		}
		if(countForNeedToSave<minv)
		{
			minv=countForNeedToSave;
			minr=j;
		}

		//find an unlived register
		tmp=regFile[j-REG_A0].operand;
		countForNeedToSave=0;
		while(tmp->next!=NULL)
		{
			if(isLive(tmp->next->inst,origin)||needToBeSave(tmp->next->inst,origin))
				countForNeedToSave++;
			tmp=tmp->next;
		}
		if(countForNeedToSave==0)
		{
			unset(regFile[j-REG_A0].operand,j-REG_A0,i);
			goto select;
		}
		if(countForNeedToSave<minv)
		{
			minv=countForNeedToSave;
			minr=j;
		}
	}
	//select a least save register
	j=minr;
	tmp=regFile[j-REG_A0].operand;
	while(tmp->next!=NULL)
	{
		if(needToBeSave(tmp->next->inst,origin+1))
		{
			getAddr(tmp->next->inst,j);
			addCode(SW,j,addr[0],addr[1]);
			tree[tmp->next->inst].isNew=1;
		}
		tmp=tmp->next;
	}
	unset(regFile[j-REG_A0].operand,j-REG_A0,i);
	freeReg(regFile[j-REG_A0].operand->next);
select:
	tree[i].dt[j-REG_A0]=1;
	tree[i].isNew=0;
	regFile[j-REG_A0].operand->next->next=NULL;
	regFile[j-REG_A0].operand->next->inst=i;
	return j;
}


void addCode(int inst,int rd,int rs,int rt)
{
	codeBuffer[codeBufferSize].instr=inst;
	codeBuffer[codeBufferSize].rd=rd;
	codeBuffer[codeBufferSize].rs=rs;
	codeBuffer[codeBufferSize].rt=rt;
	if(++codeBufferSize==codeBufferCapacity)
	{
		codeBufferCapacity*=2;
		struct mCode* tpb=malloc(sizeof(struct mCode)*codeBufferCapacity);
		memcpy(tpb,codeBuffer,sizeof(struct mCode)*codeBufferSize);
		codeBuffer=tpb;
		free(codeBuffer);
	}
}

void defaultExpGen(int instv,int insti,int index,int ex)
{
	int rd,rs,rt,flag=0;

	// instruction can't be applied commutative law
	if(ex==0)
	{
		// get reg for rs
		if(tree[index].leftType==VAL)
			rs=readRegByValue(tree[index].left);
		else
			rs=readRegByIndex(tree[index].left,index,-1);
		if(instv==NOT||instv==NEG)goto WR;
		//get reg for rt
		if(tree[index].rightType==VAL)
		{
			// instruction have none intermediate section
			//use addi to replace sub
			if(instv==SUB)
			{
				flag=1;
				goto WR;
			}
			if(insti==-1)
				rt=readRegByValue(tree[index].right);
			flag=1;
		}
		else
			rt=readRegByIndex(tree[index].right,index,rs);
	}
	else			// apply commutative law
	{
		if(tree[index].leftType==VAL)
		{
			if(insti==-1)rt=readRegByValue(tree[index].left);
			rs=readRegByIndex(tree[index].right,index,-1);
			flag=1;
		}
		else if(tree[index].rightType==VAL)
		{
			rs=readRegByIndex(tree[index].left,index,-1);
			if(insti==-1)rt=readRegByValue(tree[index].right);
			flag=1;
		}
		else
		{
			rs=readRegByIndex(tree[index].left,index,-1);
			rt=readRegByIndex(tree[index].right,index,rs);
		}
	}
WR:	rd=writeRegByIndex(index,index);
	addVarToReg(rd-REG_A0,index);
	tree[index].isNew=0;
	if(instv==NOT||instv==NEG)addCode(instv,rd,rs,0);
	else if(instv==SUB&&flag)addCode(ADDI,rd,rs,-tree[index].right);
	else if(insti!=-1&&flag)tree[index].rightType==VAL?addCode(insti,rd,rs,tree[index].right):addCode(insti,rd,rs,tree[index].left);
	else addCode(instv,rd,rs,rt);
	tree[index].dt[rd-REG_A0]=1;
}

void assExpGen(int instv,int insti,int index)
{
	int rt,rs,rd;
	//operation wouldn't change the initial value
	if(tree[index].rightType==VAL&&tree[index].right==0&&(
				instv==ADD||instv==SUB||instv==OR||instv==SLLV||instv==SRAV))
		return;
	if(tree[index].rightType==VAL&&tree[index].right==1&&(
				instv==MUL&&instv==DIV&&instv==AND))
		return;

	if(instv==MOVE)
	{
		if(tree[index].rightType!=VAL)
		{
			int i=0,j=0;
			for(;i<=REG_V0-REG_A0;++i)
				if(tree[tree[index].right].dt[i]!=0)
					break;
			if(i<=REG_V0-REG_A0)
			{
				for(j=0;j<REG_V0-REG_A0;++j)
					if(tree[tree[index].left].dt[j])
					{
						struct xOp* tp=regFile[j].operand;
						while(tp->next!=NULL)
							if(tp->next->inst==tree[index].left)
								tp->next=tp->next->next;
							else tp=tp->next;
					}
				addVarToReg(i,tree[index].left);
				addVarToReg(i,index);
				bzero(tree[index].dt,sizeof(int)*(REG_V0-REG_A0+1));
				bzero(tree[tree[index].left].dt,sizeof(int)*(REG_V0-REG_A0+1));
				tree[index].dt[i]=1;tree[tree[index].left].dt[i]=1;
				tree[tree[index].left].isNew=tree[index].isNew=0;
			}
			else
			{
				int rg;
				getAddr(tree[index].right,-1);
				rg=writeRegByIndex(tree[index].left,index);
				addCode(LW,rg,addr[0],addr[1]);
				tree[tree[index].right].dt[rg-REG_A0]=1;
				tree[index].dt[rg-REG_A0]=1;
				tree[tree[index].left].dt[rg-REG_A0]=1;
			
				addVarToReg(rg-REG_A0,index);
				addVarToReg(rg-REG_A0,tree[index].right);
				
				getAddr(tree[index].left,rg);
				addCode(SW,rg,addr[0],addr[1]);
				tree[tree[index].left].isNew=1;
			}
			return;
		}
		else
		{
				int rd=writeRegByIndex(tree[index].left,index);
				addVarToReg(rd-REG_A0,tree[index].left);
				addCode(LI,rd,tree[index].right,0);
				bzero(tree[index].dt,sizeof(int)*(REG_V0-REG_A0+1));

				tree[tree[index].left].dt[rd-REG_A0]=1;
				tree[index].dt[rd-REG_A0]=1;
			getAddr(tree[index].left,rd);
			addCode(SW,rd,addr[0],addr[1]);
			tree[tree[index].left].isNew=tree[index].isNew=1;
			return;
		}
	}
	if(tree[index].rightType==VAL&&insti==-1)
		rt=readRegByValue(tree[index].right);
	else if(tree[index].rightType==REF)
		rt=readRegByIndex(tree[index].right,index,-1);
	rs=readRegByIndex(tree[index].left,index,rt);
	rd=writeRegByIndex(tree[index].left,index);

	if(tree[index].rightType==VAL&&insti!=-1)
		addCode(insti,rd,rs,tree[index].right);
	else addCode(instv,rd,rs,rt);
	bzero(&tree[tree[index].left].dt,sizeof(int)*(REG_V0-REG_A0+1));
	tree[tree[index].left].dt[rd-REG_A0]=tree[index].dt[rd-REG_A0]=1;
	tree[tree[index].left].isNew=tree[index].isNew=0;
	addVarToReg(rd-REG_A0,tree[index].left);addVarToReg(rd-REG_A0,index);
}


//generate mips code from IR code
void genCode()
{
	int i=1,isGlobal=1,tmp,j,x,y,z;		//temporary use variable
	ppSize=0;
	int endLabel=-1;
	for(;i<=sizeOfTrees;++i)
	{
		if(tree[i].label)
			addCode(L,addLabel(NULL,i),0,0);
		switch(tree[i].op)
		{
			case MERROR:addCode(BEQ,REG_0,REG_0,addLabel(NULL,sizeOfTrees));break;
			case MDEC_AREA:
			case MDEC_INT:if(isGlobal==1)
						  {
							  addCode(DATA,GP+tree[i].baseAddr,0,0);
						  }
						  break;
			case MINIT_INT:if(isGlobal==1) 
						   {
							   addCode(DATA,GP+tree[i].baseAddr,0,0);
							   addCode(WORD,tree[i].left,0,0);
							   addCode(VAL,0,0,0);
						   }
						   else if(tree[i].left!=0)
						   {
							   //initial the data in the memory
							   if(tree[i].leftType==VAL)
							   {
								   addCode(ADDI,REG_V1,REG_0,tree[i].left);
								   addCode(SW,REG_V1,REG_FP,0-tree[i].baseAddr-4);
							   }
							   else
							   {
								   int r=readRegByIndex(tree[i].left,i,-1);
								   addCode(SW,r,REG_FP,0-tree[i].baseAddr-4);
							   }
							   regV1Des=0;
						   }
						   break;
			case MINIT_AREAH:j=-1;
							 if(isGlobal==1)
							 {
								 addCode(DATA,GP+tree[i].baseAddr,0,0);

								 // generate static data with initiate value
								 while(++j<tree[i].left/4)
									 j==0?addCode(WORD,tree[i+j+1].left,0,0):addCode(VALUE,tree[i+j+1].left,0,0);
								 addCode(VAL,0,0,0);
							 }
							 else
							 {
								 int* fg=malloc(sizeof(int)*tree[i].left/4);
								 bzero(fg,sizeof(int)*tree[i].left/4);
								 for(j=0;j<tree[i].left/4;++j)
									 if(fg[j]==0)
									 {
										 addCode(LI,REG_V1,tree[i+j+1].left,0);
										 for(x=j;x<tree[i].left/4;++x)
											 if(tree[i+x+1].left==tree[i+j+1].left)
											 {
												 addCode(SW,REG_V1,REG_FP,0-tree[i].baseAddr-4-x*4);
												 fg[x]=1;
											 }
									 }
							 }
							 i+=tree[i].left/4;
							 break;
			case MFUNCTION:isGlobal--;
						   addCode(TEXT,addLabel(((struct functionDef*)tree[i].sym)->name,0),0,0);

						   //setup the stack
						   //first calculate the size of local variable
						   j=getSizeOfPara(i);
						   j=j>=4?4:j;
						   tmp=getSizeOfLocal(i)+j+2;		// 4=sizeof($ra) 
						   
						   addCode(SW,REG_RA,REG_SP,-4);
						   addCode(SW,REG_FP,REG_SP,-8);
						   addCode(ADDI,REG_FP,REG_SP,-8);
						   addCode(ADDI,REG_SP,REG_SP,0-tmp*4);
						   
						   ppList[ppSize].left=tmp-j-2;		//local number
						   ppList[ppSize].right=j;			//para number(<=4),alse use in getAddr()
						   ppList[ppSize].label=0;		// remain 0 if this function doesn`t call other function
						 
						   ppList[ppSize].stableSize=0;	//named variable
						   ppList[ppSize].offset=0;		//it's not fixed in the begining for some IR instruction,
														//when some temporary(used by compiler) need to be saved,
														//this field should be used
						   x=i+1;
						   while(tree[x].op!=MEND)
							   if(tree[x++].op==MCALL) ppList[ppSize].label++;
						   ppSize++;


						   int* pram=NULL;
						   if(j>0)
						   {
							   pram=malloc(sizeof(int)*j);
							   for(y=0,z=i+1;y<j&&y<4;++z)
								   if(tree[z].op==MID&&((struct symbol*)tree[z].sym)->isPara)
								   {
									   addVarToReg(y,z);
									   tree[z].dt[y]=1;
									   tree[z].isNew=0;
									   pram[y++]=z;
								   }
						   }
						   if(ppList[ppSize-1].label>0)
						   {
							   //number of parameter
							   switch(j)
							   {
									//store first 4 parameter in the stack, the remain have been in the stack
								   // this process can be eliminated if there is no call in the function
								   case 4:addCode(SW,REG_A3,REG_FP,-j*4+12);tree[pram[3]].isNew=1;
								   case 3:addCode(SW,REG_A2,REG_FP,-j*4+8);tree[pram[2]].isNew=1;
								   case 2:addCode(SW,REG_A1,REG_FP,-j*4+4);tree[pram[1]].isNew=1;
								   case 1:addCode(SW,REG_A0,REG_FP,-j*4);tree[pram[0]].isNew=1;
								   default:;
							   }
						   }
						   break;
			case MRETURN:endLabel=endLabel==-1?addLabel(NULL,i):endLabel;
						 tree[i].leftType==REF?addCode(MOVE,REG_V0,readRegByIndex(tree[i].left,i,-1),0):
							 addCode(MOVE,REG_V0,readRegByValue(tree[i].left),0);
						 updateTmpReg(i);
						 addCode(BEQ,REG_0,REG_0,endLabel);
						 break;
			case MEND:   if(endLabel!=-1)
							 addCode(L,endLabel,0,0);
						 else
						 {
							 fprintf(stderr,"in function %s: no return statement, return 0 by default\n",
									 ((struct functionDef*)tree[i].sym)->name);
							 addCode(MOVE,REG_V0,REG_0,0);
						 }

						 endLabel=-1;
						 flushTmp(i);
						 addCode(LW,REG_RA,REG_FP,4);


						 //restore stack
						 addCode(ADDI,REG_SP,REG_SP,(ppList[ppSize-1].left+ppList[ppSize-1].right+2)*4);
						 addCode(LW,REG_FP,REG_SP,-8);
						 //jump
						 if(strcmp(((struct functionDef*)tree[i].sym)->name,"main")==0)
						 {
							 addCode(LI,REG_V1,10,0);
							 addCode(SYSCALL,0,0,0);
						 }
						 addCode(JR,REG_RA,0,0);
				
						 isGlobal++;ppSize--;break;
			case MCALL://resize the stack to store some temporary
					   tmp=sizeOfTmp(i);	//including the temporary register and stack location in the current
				//	   updateTmpReg(i);	
					   j=getSizeOfPara(i);								//**************stack model****************//
				//	   if(j>4)											//	-----------------------
				//		   addCode(ADDI,REG_SP,REG_SP,0-(tmp+j-4)*4);	//	|		$ra				|
				//	   else												//	-------------------------
				//		   addCode(ADDI,REG_SP,REG_SP,0-tmp*4);			//	|		$fp(old)		|
/*																			-------------------------
					   for(x=0;x<10;++x)							$fp->	|	$a0	$a1 $a2 $a3		|
						   if(needStore(REG_T0+x))							-------------------------
						   {												|		para			|
							   if(j>4) addCode(SW,REG_T0+x,REG_SP,)			-------------------------
						   }*/									//			|		loacl			|
					   											//			-------------------------
					   //find above j para of function in IR	//			|			tmp			|<-	$sp
					  // for(x=i,y=0;y<j;--x)						//			-------------------------
						//   if(tree[x].op==MPARAM)				// actually, program need to maintain the offset to $fp, and the corresponding
						//	   ++y;								//		$fp

					   //parameter index for function call
					   int* param=malloc(sizeof(int)*j);
					   for(x=i-1,y=j-1,z=0;y>=0;--x)
					   {
						   if(tree[x].op==MPARAM)
							   z==0?(param[y--]=x):z--;
						   else if(tree[x].op==MCALL)
							   z+=getSizeOfPara(x);
					   }
					   for(y=0;y<j;++y)
					   {
						   if(y<4)													//generate code for parameter
							   if(tree[param[y]].leftType==REF)addCode(MOVE,REG_A0+y,readRegByIndex(tree[param[y]].left,i,-1),0);
							   else addCode(LI,REG_A0+y,tree[param[y]].left,0);
						   else
						   {		// latter save in stack
							   if(tree[param[y]].leftType==REF)
								   z=readRegByIndex(tree[param[y]].left,i,-1);
							   else
								   z=readRegByValue(tree[param[y]].left);
							   addCode(SW,z,REG_SP,(-tmp-j+y)*4);
						   }
					   }
					   free(param);
					   updateTmpReg(i);
					   if(j>4)											//	-----------------------
						   addCode(ADDI,REG_SP,REG_SP,0-(tmp+j-4)*4);	//	|		$ra				|
					   else												//	-------------------------
						   addCode(ADDI,REG_SP,REG_SP,0-tmp*4);			//	|		$fp(old)		|
					   addCode(JAL,addLabel(((struct functionDef*)tree[i].sym)->name,0),0,0);
					   if(j>4)
						   addCode(ADDI,REG_SP,REG_SP,(tmp+j-4)*4);
					   else
						   addCode(ADDI,REG_SP,REG_SP,tmp*4);
					   addVarToReg(REG_V0-REG_A0,i);				//store the result in the stack
					   tree[i].isNew=0;
					   tree[i].dt[REG_V0-REG_A0]=1;
					   break;
			case MMUL:defaultExpGen(MUL,-1,i,0);break;					// some simple optimize:
			case MDIV:defaultExpGen(DIV,-1,i,0);break;					//	li	$s0,10
			case MMOD:defaultExpGen(REM,-1,i,0);break;					//	sub	$s1,$s1,$s0
			case MPLUS:defaultExpGen(ADD,ADDI,i,1);break;				// to:
			case MMINUS:defaultExpGen(SUB,-1,i,0);break;				//	addi $s1,$s1,-10
			case MSHL:defaultExpGen(SLLV,SLL,i,0);break;
			case MSHR:defaultExpGen(SRAV,SRA,i,0);break;
			case MLESS:defaultExpGen(SLT,SLTI,i,0);break;
			case MLESSEQ:defaultExpGen(SLE,-1,i,0);break;
			case MEQUAL:defaultExpGen(SEQ,-1,i,0);break;		//		opt here**********************************************(((				
			case MNOEQUAL:defaultExpGen(SNE,-1,i,0);break;
			case MBITAND:defaultExpGen(AND,ANDI,i,1);break;				//**********	arithmetic	****************
			case MBITXOR:defaultExpGen(XOR,XORI,i,1);break;
			case MBITOR:defaultExpGen(OR,ORI,i,1);break;		
			case MBITNOT:defaultExpGen(NOT,-1,i,0);break;
			case MMINUS_PRE:defaultExpGen(NEG,-1,i,0);break;

			case MPLUSASS:assExpGen(ADD,ADDI,i);break;
			case MMINUSASS:assExpGen(SUB,-1,i);break;
			case MMULASS:assExpGen(MUL,-1,i);break;
			case MDIVASS:assExpGen(DIV,-1,i);break;
			case MANDASS:assExpGen(AND,ANDI,i);break;
			case MXORASS:assExpGen(XOR,XORI,i);break;
			case MORASS:assExpGen(OR,ORI,i);break;
			case MSHLASS:assExpGen(SLLV,SLL,i);break;
			case MSHRASS:assExpGen(SRAV,SRA,i);break;
			case MASSIGN:assExpGen(MOVE,LI,i);break;
			case MIF:
						 if(tree[i].leftType==REF)
						 {
							 tmp=readRegByIndex(tree[i].left,i,-1);
							 updateTmpReg(i);
							 addCode(BNE,tmp,REG_0,addLabel(NULL,tree[i].right));
						 }
						 else if(tree[i].left)addCode(BEQ,REG_0,REG_0,addLabel(NULL,tree[i].right));
					 break;
			case MGOTO:updateTmpReg(i);
					 addCode(BEQ,REG_0,REG_0,addLabel(NULL,tree[i].left));
					   break;
			case MIFFALSE:if(tree[i].leftType==REF)
						  {
							  tmp=readRegByIndex(tree[i].left,i,-1);
							  updateTmpReg(i);
							  addCode(BEQ,tmp,REG_0,addLabel(NULL,tree[i].right));
						  }
						 else if(!tree[i].left)
							 addCode(BEQ,REG_0,REG_0,addLabel(NULL,tree[i].right));
						  break;
			case READ:addCode(LI,REG_V1,5,0);
					  regV1Des=0;
					  addCode(SYSCALL,0,0,0);
					  tmp=writeRegByIndex(tree[i].left,i);
					  addCode(MOVE,tmp,REG_V1,0);
					  break;
			case WRITE:
					  if(tree[i].leftType==VAL||tree[tree[i].left].dt[0]!=1)
					  {
						  struct xOp* tp=regFile[REG_A0-REG_A0].operand;
						  //save the varaible in the register $a0
						  while(tp->next!=NULL)
						  {
							  if(needToBeSave(tp->next->inst,i))
							  {
								  getAddr(tp->next->inst,-1);
								  addCode(SW,addr[0],addr[1],0);
								  tree[tp->next->inst].isNew=0;
							  }
							  tree[tp->next->inst].dt[0]=0;
							  tp=tp->next;
					      }
						  regFile[REG_A0-REG_A0].operand->next=NULL;
						  if(tree[i].leftType==VAL)
							  addCode(LI,REG_A0,tree[i].left,0);
						  else
						  {
							  tmp=readRegByIndex(tree[i].left,i,-1);
							  addCode(MOVE,REG_A0,tmp,0);
						  }
					  }
					  addCode(LI,REG_V1,1,0);
					  addCode(SYSCALL,0,0,0);
					  addCode(LI,REG_A0,10,0);
					  addCode(LI,REG_V1,11,0);
					  addCode(SYSCALL,0,0,0);
					  break;
					   
		}
	}
	addCode(L,addLabel(NULL,sizeOfTrees),0,0);
}

//optimize for mips code
void opt()
{
	int i=0,start,end,j,k,m;
	for(;i<codeBufferSize;++i)
	{
		if(codeBuffer[i].instr!=L)
			continue;
		else codeBuffer[i].rs=0;
		if(i>=codeBufferSize-1)break;
		end=start=i;
		while(codeBuffer[++end].instr!=L);
		//reduce lw and sw
		for(j=start;j<end;++j)
		{
			//reduce the usage of redundant seq sne
			if(codeBuffer[j].instr==SEQ||codeBuffer[j].instr==SNE)
			{
				for(m=0,k=j+1;k<end;++k)
					if((codeBuffer[k].instr==BEQ||codeBuffer[k].instr==BNE)&&codeBuffer[k].rd==codeBuffer[j].rd)
					{
						if(codeBuffer[j].instr==SEQ&&codeBuffer[k].instr==BEQ)
							codeBuffer[k].instr=BNE;
						else if(codeBuffer[j].instr==SEQ&&codeBuffer[k].instr==BNE)
							codeBuffer[k].instr==BEQ;
						codeBuffer[k].rd=codeBuffer[j].rs;
						codeBuffer[k].rs=codeBuffer[j].rt;
					}
					else if(codeBuffer[j].rd==codeBuffer[k].rs||codeBuffer[j].rd==codeBuffer[k].rt)
						++m;
				if(!m)codeBuffer[j].instr=MNOP;
			}
			if(codeBuffer[j].instr==LI&&codeBuffer[j].rs==0&&codeBuffer[j].rs>=REG_T0&&codeBuffer[j].rs<=REG_S7)
			{
				for(k=j+1;k<end;++k)
					if(codeBuffer[k].rd==codeBuffer[j].rd)
					{
						if(codeBuffer[k].instr==SW)codeBuffer[k].rd=REG_0;
						else if(codeBuffer[k].instr!=BEQ&&codeBuffer[k].instr!=BNE)break;
					}
					else if(codeBuffer[k].rs==codeBuffer[j].rd)
						codeBuffer[k].rs=REG_0;
					else if(codeBuffer[k].rt==codeBuffer[j].rd)
						codeBuffer[k].rt==REG_0;
				codeBuffer[j].instr=MNOP;
			}
			if(codeBuffer[j].instr==JAL)
			{
				for(m=0,k=j-1;k>=0&&codeBuffer[k].instr!=TEXT;--k)
					if(codeBuffer[k].instr==SW&&codeBuffer[k].rs==REG_SP&&codeBuffer[k].rd!=REG_FP)m++;
				for(k=j+1;k<codeBufferSize&&codeBuffer[k].instr!=TEXT;++k)
					if(codeBuffer[k].instr==SW&&codeBuffer[k].rs==REG_SP&&codeBuffer[k].rd!=REG_FP)m++;

				if(!m)
				{
					if(codeBuffer[j+1].instr==ADDI&&codeBuffer[j+1].rd==REG_SP&&
							codeBuffer[j-1].instr==ADDI&&codeBuffer[j-1].rd==REG_SP)
						codeBuffer[j+1].instr=codeBuffer[j-1].instr=MNOP;
				}
			}
			if(codeBuffer[j].instr==MOVE&&codeBuffer[j].rd==codeBuffer[j].rs)
				codeBuffer[j].instr=MNOP;
		}
	}
	//reduce beq
	int* ln=malloc(sizeof(int)*labelSize);
	bzero(ln,sizeof(int)*labelSize);
	for(i=0;i<codeBufferSize;++i)
		if(codeBuffer[i].instr==BEQ||codeBuffer[i].instr==BNE)
			ln[codeBuffer[i].rt]++;
	for(i=0;i<labelSize;++i)
		if(!ln[i]&&findFunction(functionList,labelStr[i])==NULL)labelStr[i]=NULL;

	for(i=0;i<codeBufferSize;++i)
		if(codeBuffer[i].instr==BEQ||codeBuffer[i].instr==BNE)
		{
			if(codeBuffer[i+2].instr==BEQ&&codeBuffer[i+2].rd==REG_0&&codeBuffer[i+2].rs==REG_0
					&&codeBuffer[i+1].instr==L&&ln[codeBuffer[i+1].rd]==0)
			{
				if(codeBuffer[i+3].instr==L&&(codeBuffer[i].rt==codeBuffer[i+3].rd||codeBuffer[i+4].instr==L&&
							codeBuffer[i+4].rd==codeBuffer[i].rt))
				{
					codeBuffer[i].instr=codeBuffer[i].instr==BEQ?BNE:BEQ;
					codeBuffer[i].rt=codeBuffer[i+2].rt;
					codeBuffer[i+2].instr=MNOP;
				}
			}
			else if(codeBuffer[i].rd==REG_0&&codeBuffer[i].rs==REG_0&&codeBuffer[i+1].instr==L
					&&(codeBuffer[i].rt==codeBuffer[i+1].rd||codeBuffer[i+2].instr==L&&codeBuffer[i].rt==codeBuffer[i+2].rd))
				codeBuffer[i].instr=MNOP;
		}
	bzero(ln,sizeof(int)*labelSize);
	for(i=0;i<codeBufferSize;++i)
		if(codeBuffer[i].instr==BEQ||codeBuffer[i].instr==BNE)
			ln[codeBuffer[i].rt]++;
	for(i=0;i<labelSize;++i)
		if(!ln[i]&&labelStr[i]!=NULL&&findFunction(functionList,labelStr[i])==NULL)labelStr[i]=NULL;
	free(ln);
}

//print mips code
void printCode(char* file)
{
	cFile=fopen(file,"w");
	int i=0;
	for(;i<codeBufferSize;++i)
	{
		switch(codeBuffer[i].instr)
		{
			case L:		if(labelStr[codeBuffer[i].rd]!=NULL)
							fprintf(cFile,"%s:\n",labelStr[codeBuffer[i].rd]);
						break;
			case DATA:	fprintf(cFile,".data\t0x%08x\n",codeBuffer[i].rd);break;
			case WORD:	fprintf(cFile,".word\t%i ",codeBuffer[i].rd);break;
			case VALUE:	fprintf(cFile,"%i ",codeBuffer[i].rd);break;
			case VAL:	fprintf(cFile,"\n");break;
			case TEXT:	fprintf(cFile,".text\n%s:\n",labelStr[codeBuffer[i].rd]);break;
			case SW:	fprintf(cFile,"\tsw %s,%i(%s)\n",rn[codeBuffer[i].rd-REG_A0],codeBuffer[i].rt,rn[codeBuffer[i].rs-REG_A0]);
						break;
			case LW:	fprintf(cFile,"\tlw %s,%i(%s)\n",rn[codeBuffer[i].rd-REG_A0],codeBuffer[i].rt,rn[codeBuffer[i].rs-REG_A0]);
						break;
			case ADDI:	fprintf(cFile,"\taddi ");goto L1;
			case ADD:	fprintf(cFile,"\tadd ");goto L2;
			case SUB:	fprintf(cFile,"\tsub ");goto L2;
			case JR:	fprintf(cFile,"\tjr %s\n",rn[codeBuffer[i].rd-REG_A0]);break;
			case JAL:	fprintf(cFile,"\tjal %s\n",labelStr[codeBuffer[i].rd]);break;
			case MUL:	fprintf(cFile,"\tmul ");goto L2;
			case MULI:	fprintf(cFile,"\tmul ");goto L1;
			case DIV:	fprintf(cFile,"\tdiv ");goto L2;
			case REM:	fprintf(cFile,"\trem ");goto L2;
			case SLLV:	fprintf(cFile,"\tsllv ");goto L2;
			case SRAV:	fprintf(cFile,"\tsrav ");goto L2;
			case SLT:	fprintf(cFile,"\tslt ");goto L2;
			case SLE:	fprintf(cFile,"\tsle ");goto L2;
			case SEQ:	fprintf(cFile,"\tseq ");goto L2;
			case SNE:	fprintf(cFile,"\tsne ");goto L2;
			case AND:	fprintf(cFile,"\tand ");goto L2;
			case OR:	fprintf(cFile,"\tor");goto L2;
			case XOR:	fprintf(cFile,"\txor ");goto L2;
			
			case NOT:	fprintf(cFile,"\tnot %s,%s\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0]);break;
			case NEG:	fprintf(cFile,"\tneg %s,%s\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0]);break;
			case MOVE:	fprintf(cFile,"\tmove %s,%s\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0]);break;
			
			case SLL:	fprintf(cFile,"\tsll ");goto L1;
			case SRA:	fprintf(cFile,"\tsra ");goto L1;
			case SLTI:	fprintf(cFile,"\tslti ");goto L1;
			case ANDI:	fprintf(cFile,"\tandi ");goto L1;
			case ORI:	fprintf(cFile,"\tori ");goto L1;
			case XORI:	fprintf(cFile,"\txori ");goto L1;
			case LI:	fprintf(cFile,"\tli %s,%i\n",rn[codeBuffer[i].rd-REG_A0],codeBuffer[i].rs);break;
			case BNE:	fprintf(cFile,"\tbne %s,%s,%s\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0],labelStr[codeBuffer[i].rt]);
						break;
			case BEQ:	fprintf(cFile,"\tbeq %s,%s,%s\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0],labelStr[codeBuffer[i].rt]);
						break;
			case SYSCALL:fprintf(cFile,"\tsyscall\n");break;
		}
		continue;
L1:		fprintf(cFile,"%s,%s,%i\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0],codeBuffer[i].rt);
		continue;
L2:		fprintf(cFile,"%s,%s,%s\n",rn[codeBuffer[i].rd-REG_A0],rn[codeBuffer[i].rs-REG_A0],rn[codeBuffer[i].rt-REG_A0]);
		continue;
	}
}
void output(char* name)
{
	FILE* fout=fopen(name,"w");
	int i=0;
	for(;i<sizeOfTrees;++i)
	{
		if(tree[i].label==1)
			fprintf(fout,"%i ",i);
		switch(tree[i].op)
		{
			case MMUL:fprintf(fout,"%i:\tMUL\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MDIV:fprintf(fout,"%i:\tDIV\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MMOD:fprintf(fout,"%i:\tMOD\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MPLUS:fprintf(fout,"%i:\tPLUS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MMINUS:fprintf(fout,"%i:\tMINUS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MSHL	:fprintf(fout,"%i:\tSHL\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MSHR:fprintf(fout,"%i:\tSHR\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MGREAT:fprintf(fout,"%i:\tGREAT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MGREATEQ:fprintf(fout,"%i:\tGREATEQ\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MLESS:fprintf(fout,"%i:\tLESS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MLESSEQ:fprintf(fout,"%i:\tLESSEQ\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MEQUAL	:fprintf(fout,"%i:\tEQUAL\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MNOEQUAL:fprintf(fout,"%i:\tNOEQUAL\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MBITAND	:fprintf(fout,"%i:\tBITAND\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MBITXOR:fprintf(fout,"%i:\tBITXOR\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MBITOR	:fprintf(fout,"%i:\tBITOR\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MLOGAND:fprintf(fout,"%i:\tLOGAND\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MLOGOR	:fprintf(fout,"%i:\tLOGOR\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MBITNOT:fprintf(fout,"%i:\tBITNOT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MMINUS_PRE:fprintf(fout,"%i:\tMINUS_PRE\t%i\t%i\n",i,tree[i].left,tree[i].right);break;

			case MPLUSASS:fprintf(fout,"%i:\tPLUSASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MMINUSASS:fprintf(fout,"%i:\tMINUSASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MMULASS:fprintf(fout,"%i:\tMULASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MDIVASS:fprintf(fout,"%i:\tDIVASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MANDASS:fprintf(fout,"%i:\tANDASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MXORASS:fprintf(fout,"%i:\tXORASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MORASS	:fprintf(fout,"%i:\tORASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MSHLASS:fprintf(fout,"%i:\tSHLASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MSHRASS:fprintf(fout,"%i:\tSHRASS\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MASSIGN	:fprintf(fout,"%i:\tASSIGN\t%i\t%i\n",i,tree[i].left,tree[i].right);break;

			case MINC	:fprintf(fout,"%i:\tINC\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MDEC_PRE:fprintf(fout,"%i:\tDEC_PRE\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MDEC	:fprintf(fout,"%i:\tDEC\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MLOGNOT:fprintf(fout,"%i:\tLOGNOT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MINC_PRE:fprintf(fout,"%i:\tINC_PRE\t%i\t%i\n",i,tree[i].left,tree[i].right);break;

			case MCALL:fprintf(fout,"%i:\tCALL\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MID:fprintf(fout,"%i:\tID\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MDOT:fprintf(fout,"%i:\tDOT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MINT:fprintf(fout,"%i:\tINT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MPARAM	:fprintf(fout,"%i:\tPARAM\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MNOP:fprintf(fout,"%i:\tNOP\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MTMP:fprintf(fout,"%i:\tTMP\t%i\t%i\n",i,tree[i].left,tree[i].right);break;

			case MFUNCTION:fprintf(fout,"%i:\tFUNCTION\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MRETURN:fprintf(fout,"%i:\tRETURN\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MIF:fprintf(fout,"%i:\tIF\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MIFFALSE:fprintf(fout,"%i:\tIFFALSE\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MGOTO:fprintf(fout,"%i:\tGOTO\t%i\t%i\n",i,tree[i].left,tree[i].right);break;

			case MDEC_INT:fprintf(fout,"%i:\tDEC_INT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MDEC_AREA:fprintf(fout,"%i:\tDEC_AREA\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MINIT_INT:fprintf(fout,"%i:\tINIT_INT\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MINIT_AREAH:fprintf(fout,"%i:\tINIT_AREAH\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case MINIT_AREA:fprintf(fout,"%i:\tINIT_AREA\t%i\t%i\n",i,tree[i].left,tree[i].right);break;
			case WRITE:fprintf(fout,"%i:\twrite\t%i\n",i,tree[i].left);break;
			case READ:fprintf(fout,"%i:\tread\t%i\n",i,tree[i].left);break;
		}
	}
	fclose(fout);
}
