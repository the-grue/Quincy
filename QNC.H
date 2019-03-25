/*
 * qnc.h QUINCY Interpreter - header file
 */

#ifndef QNC_H
#define QNC_H

#include <setjmp.h>
#include <ctype.h>

#undef isxdigit
#undef isalnum
#undef isdigit
#undef isalpha

#include "dflat.h"
#include "errs.h"
#include "tokens.h"

#define PROGTITLE "The Quincy C Interpreter"
#define QVERSION "4.1"

#ifdef NDEBUG
#define Assert(p)   ((void)0)
#else
void AssertFail(char *, char *, int);
#define Assert(p)   ((p) ? (void)0 : AssertFail(#p, __FILE__, __LINE__))
#endif

#define STDLIB   "stdlib.qnc"

#define alphanum(c)	(isalpha(c)||isdigit(c)||c=='_')
#define isSymbol() ((Ctx.Token)==T_SYMBOL||(Ctx.Token)==T_IDENTIFIER)

#define TABS	4	/* tabstops are set at every TABS columns */

/*
 * Table size constants
 */
#define MAXSTACK	   256		 /* default program stack size 		 */
#define MAXPR		   (16*1024) /* default user program space 		 */
#define MAXVARIABLES   1024		 /* maximum variables 				 */
#define MAXFUNCTIONS   200		 /* maximum functions in program 	 */
#define DATASPACE	   (16*1024) /* data space for program 			 */
#define MAXPARMS       10        /* maximum macro parameters 		 */
#define MAXSYMBOLTABLE 1024		 /* symbol table space 				 */
#define AVGPROTOTYPES  10		 /* average prototype bytes/function */
#define MAXDIM         4         /* maximum dimensions for arrays    */
#define MAXOPENFILES   15        /* maximum open FILEs               */
#define MAXINCLUDES    10		 /* maximum nested #include files    */
#define MAXIFLEVELS    25		 /* maximum nested #if...s			 */
/*
 * Constants
 */
#define RVALUE	0				/* a constant */
#define LVALUE	1				/* a variable */

enum Type { VOID, CHAR, INT, LONG, FLOAT, STRUCT, UNION, ENUM };

#define FUNCT		1	/* a function */
#define STRUCTELEM	2	/* structure element */
#define LABEL       4   /* goto label */
#define TYPEDEF     8   /* typedef */

/* ---- storage classes ----- */
#define AUTO        1
#define REGISTER    2
#define VOLATILE    4
#define EXTERN      8

/*
 * Variable table entry
 * (one for each declared variable)
 */
typedef struct variable {
	int		vsymbolid;		/* variable identifier (assoc w/name)	*/
	char	vclass;			/* its indirection level 				*/
	char    vkind;          /* kind of variable (func, struct elem, etc. */
	int		vtype;			/* type, INT, CHAR, etc.				*/
	int		vsize;			/* size of variable 					*/
	int		vdims[MAXDIM];	/* lengths (if an array) 				*/
	char	vconst;			/*    0 = read/write					*/
							/*    1 = variable is const				*/
							/*	  2 = pointer -> const				*/
							/*    3 = both							*/
	char   vstatic;         /* 1 = static                           */
	char   vqualifier;      /* 1 = auto
							   2 = register
							   4 = volatile
							   8 = extern */
	char   islocal;         /* 1 = local variable, 2 = argument     */
	char   isunsigned;      /* 1 = unsigned, 0 = signed             */
	char   isinitialized;	/* 1 = variable is initialized			*/
	int    voffset;			/* offset of data from start of buffer	*/
	int    vwidth;			/* width of data space					*/
	int	   vBlkNesting;		/* block nesting level 					*/
	struct variable *vstruct;	/* for a struct var, -> definition	*/
	int	   fileno;			/* file number where declared           */
	int	   lineno;			/* line number where declared           */
	int	   enumval;			/* integer value for an enum constant	*/
	/* ----- must be same structure as VARIABLELIST below ---- */
	struct	{
		struct variable *vfirst;
		struct variable *vlast;
	} velem;				/* VARIABLELIST of elements if a struct */
	struct variable *vprev;	/* backward link (1st item ->last)		*/
	struct variable *vnext;	/* forward link 						*/
} VARIABLE;

/*
 * Variable list
 */
typedef struct {
	VARIABLE *vfirst;	
	VARIABLE *vlast;
} VARIABLELIST;

/*
 * Function definition
 * (one for each declared function)
 */
typedef struct {
	int symbol;				/* function symbol id					*/
	char ismain;			/* 1 = main()							*/
	int libcode;			/* > 0 = standard library function      */
	char *proto;			/* function prototype 					*/
	void *code;				/* function code						*/
	int	type;				/* return type, INT, CHAR, etc.			*/
	char class;				/* indirection level of function return */
	unsigned char fileno;	/* where the function is                */
	int lineno;             /* line no of function header           */
	char fconst;			/*    0 = read/write					*/
							/*    1 = function is const				*/
							/*	  2 = pointer -> const				*/
							/*    3 = both							*/
	int width;				/* width of auto variables				*/
	VARIABLELIST locals;	/* list of local variables				*/
	int BlkNesting;		    /* block nesting level 					*/
} FUNCTION;

/*
 * Running function table entry
 * (one instance for each iteration of recursive function)
 */
typedef struct funcrunning {
	FUNCTION *fvar;		    	/* function variable 	*/
	char *ldata;				/* local data 			*/
	int arglength;				/* length of arguments  */
	struct funcrunning *fprev;	/* calling function 	*/
	/* need this so debugger can find correct variables */
	int BlkNesting;		    	/* block nesting level 	*/
} FUNCRUNNING;

/*
 * Stack data item's value
 */
typedef union datum {
	char	 		cval;		/* character values 		  */
	int		 		ival;		/* integer values 			  */
	long	 		lval;		/* long values 				  */
	double	 		fval;		/* floating point values 	  */
	char	 		*cptr;		/* pointers to chars		  */
	unsigned char	*ucptr;		/* pointers to unsigned chars */
	int		 		*iptr;		/* pointers to ints		 	  */
	unsigned int	*uiptr;		/* pointers to unsigned ints  */
	long	 		*lptr;		/* pointers to longs 		  */
	unsigned long 	*ulptr;		/* pointers to unsigned longs */
	double	 		*fptr;		/* pointers to floats 		  */
	FUNCTION 		*funcptr;  	/* pointers to functions      */
	char	 		**pptr;		/* pointers to pointers 	  */
} DATUM;

/*
 * Stack item with attributes
 */
typedef struct item {
	char kind;          /* STRUCTELEM, FUNCT, LABEL, TYPEDEF  */
	char isunsigned;    /* 1 = unsigned, 0 = signed           */
	char class;	 		/* pointer or array indirection level */
	char lvalue; 		/* 1 == LVALUE, 0 == RVALUE 		  */
	char vconst; 		/* 0 = read/write, 1,2,3 = const 	  */
	char vqualifier;	/* storage class, etc.                */
	int	size;	 		/* size of the thing on the stack 	  */
	char type;	 		/* type of the thing on the stack 	  */
	int	dims[MAXDIM];	/* array dimensions					  */
	VARIABLE *vstruct;	/* for a struct var, -> definition	  */
	VARIABLELIST *elem;	/* structure's element variable list  */
	DATUM value;	 	/* the value of the thing 			  */
} ITEM;

#define rslva(a,l) ((l)?(a):(char*)(&a))
#define rslvs(s,c) ((c)?sizeof(void *):s)

#define NullVariable(var) memset(var, 0, sizeof(VARIABLE))
#define NullFunction(fnc) memset(fnc, 0, sizeof(FUNCTION))

enum PreProcTokens {
	P_DEFINE = 1,
	P_ELSE,
	P_ELIF,
	P_ENDIF,
	P_ERROR,
	P_IF,
	P_IFDEF,
	P_IFNDEF,
	P_INCLUDE,
	P_UNDEF
};

/* ----- program running context ----- */
typedef struct context {
	unsigned char *Progptr;	/* statement pointer */
	unsigned char *svpptr;	/* saved statement pointer */
	char svToken;			/* saved token value */
	VARIABLE *svCurvar;		/* saved variable */
	int CurrFileno;			/* current source file */
	int CurrLineno;			/* current source file line number */
	VARIABLE *Curvar;		/* points to current variable declaration */
	FUNCTION *Curfunction;	/* points to current function declaration */
	FUNCTION *Linkfunction;	/* points to function being linked */
	ITEM *Stackptr;			/* stack pointer */
	DATUM Value;			/* value on stack */
	char Token;				/* current token value */
	FUNCRUNNING *Curfunc;	/* current running function */
	VARIABLE *NextVar;		/* next available stack frame variable */
	VARIABLELIST Curstruct; /* list of current structure members */
	char *NextData;			/* next available data space */
	int Looping;			/* set when inside a "while" or "for" loop */
	int Switching;			/* set when inside a "switch" */
} CONTEXT;

/* -------- setjmp buffer ----------- */
typedef struct jmpbuf {
	int jmp_id;
	jmp_buf jb;
	CONTEXT jmp_ctx;
} JMPBUF;

typedef struct symbol {
	char *symbol;
	int ident;
} SYMBOLTABLE;

/* -------- shell prototypes --------- */
void shell(char *);
int goline(int);
void predit(int, unsigned char *, int, int);
void qcexit(int);
void *getmem(unsigned);
void error(int);
void qcom(void);
int getkeys(char *, int);
void qcparam_in(char *, char *, int);
void putlno(int);
void putline(int, char *);
void prtline(int, char *);
void putstr(char *);
int prload(char *,char *);

/* ------- preprocessor/linker/compiler prototypes ------ */
void PreProcessor(unsigned char*,unsigned char*);
void CleanUpPreProcessor(void);
int FindPreProcessor(char *);

VARIABLE *SearchVariable(int, int);
VARIABLE *InstallVariable(VARIABLE*,VARIABLELIST*,int,int,int,int);
FUNCTION *FindFunction(int);
void InstallFunction(FUNCTION *);
VARIABLE *DeclareVariable(VARIABLELIST *, int, int, int, int);
void Initializer(VARIABLE *, char *, int);
int VariableWidth(VARIABLE *);
void *AllocVariable(void);
void *GetDataSpace(int,int);
int isTypeDeclaration(void);
void link(VARIABLELIST *);
int tokenize(char *, char *);
int istypespec(void);
int SearchLibrary(char *);
int FindKeyword(char *);
int FindOperator(char *);
int SearchSymbols(char *, struct symbol *, int, int);
int FindSymbol(char *);
char *FindSymbolName(int);
int AddSymbol(char *);
void DeleteSymbols(void);
char *SrcFileName(int);
void *DataAddress(VARIABLE *pvar);
void ClearHeap(void);

void Compile(void);
void OutputToken(void);
void OutputExpression(void);
void PromptIDE(void);
int CBreak(void);

/* -------- interpreter prototypes----------- */
void stmtend(void);
void stmtbegin(void);
int ExpressionOne(void);
int expression(void);
void cond(void);
void assignment(void);
void callfunc(void);
void DeleteJmpbufs(void);
void torvalue(ITEM *);
int getoken(void);
void skip(char, char);
void statement(void);
VARIABLE *primary(void);
void sys(void);
int readonly(ITEM *sp);
char MakeType(char tok);
int TypeSize(char type);
void TestZeroReturn(void);
void OpenStdout(void);
int ArrayElements(VARIABLE *);
int ArrayDimensions(VARIABLE *);
int ItemArrayDimensions(ITEM *);
int ItemArrayElements(ITEM *);
int ElementWidth(ITEM *);
void TypeQualifier(VARIABLE *);
char MakeTypeToken(char, int *);
void TerminateProgram(void);

/* ------- stack prototypes ------- */
int	popint(void);
long poplng(void);
double popflt(void);
void store(void *, int, void *, int, char);
void psh(void);
void pop(void);
void popn(int);
void push(char, char, char, char, unsigned, char, VARIABLELIST *, DATUM *, char);
void pushint(int);
void pushlng(long);
void pushptr(void *, char);
void pushflt(double);
int popnint(int);
int popint(void);
void *popptr(void);
long poplng(void);
double popflt(void);
int StackItemisNumericType(void);
void topget(ITEM *);
void topset(ITEM *);
void topdup(void);
void FixStackType(char);

/* ------- global data definitions ------- */
extern unsigned char *Progstart; /* start of user program */
extern unsigned char *NextProto; /* addr of next prototype */
extern unsigned Progused;		 /* bytes of program space used */
extern int Saw_return;		/* set when "return" encountered in user program */
extern int Saw_break;		/* set when "break" encountered in user program */
extern int Saw_continue;	/* set when "continue" encountered in user program */
extern int Looping;			/* set when inside a "while" or "for" loop */
extern int Switching;		/* set when inside a "switch" */
extern int Linking;			/* set when in linker */
extern int Linklib;			/* set when linking stdlib */
extern int ConstExpression;	/* set when initializing globals */
extern int SkipExpression;	/* set when skipping the effect of expression */

extern SYMBOLTABLE *SymbolTable; /* symbol table */
extern int SymbolCount;
extern ITEM *Stackbtm;			 /* start of program stack */
extern ITEM *Stacktop;			 /* end of program stack */
extern FUNCTION *Functions; 	 /* list of functions */
extern FUNCTION *NextFunction;	 /* next available function in table */
extern VARIABLE *Blkvar;		 /* beginning of local block auto variables */
extern VARIABLELIST Globals;	 /* linked list of global variables */
extern VARIABLELIST Curstruct;	 /* linked list of current structure members */

extern CONTEXT Ctx;

extern VARIABLE *VariableMemory;
extern FUNCTION *FunctionMemory;
extern char *DataSpace;
extern char *PrototypeMemory;

extern struct QuincyConfig	{
	unsigned int MaxProgram;	 /* user program space 	  */
	unsigned int MaxStack;		 /* stack size 			  */
	unsigned int MaxVariables;	 /* number of variables   */
	unsigned int MaxFunctions;	 /* number of functions   */
	unsigned int MaxDataSpace;	 /* data bytes 			  */
	unsigned int MaxSymbolTable; /* symbol table space 	  */
	unsigned int MaxPrototype;	 /* prototype table space */
	char scrollbars;
	char inTutorial;
	char tutorhelp[11];
} qCfg;

extern jmp_buf Shelljmp;
extern jmp_buf Watchjmp;
extern jmp_buf PreProcessjmp;
extern jmp_buf Includejmp;
extern int Including;
extern int PreProcessing;
extern int Watching;
extern int ErrorCode;
extern int inSystem;
extern jmp_buf BreakJmp;

#define ItemisAddressOrPointer(i) ((i).class)
#define ItemisPointer(i) (ItemisAddressOrPointer(i) && (i).lvalue)
#define ItemisAddress(i) (ItemisAddressOrPointer(i) && !(i).lvalue)
#define ItemisArray(i) ((i).dims[0] != 0)
#define ItemisInteger(i) \
	((i)->type == INT || (i)->type == CHAR || (i)->type == LONG)

#define StackItemisAddressOrPointer() (Ctx.Stackptr->class)
#define StackItemisPointer() \
	(StackItemisAddressOrPointer() && Ctx.Stackptr->lvalue)
#define StackItemisAddress() \
	(StackItemisAddressOrPointer() && !Ctx.Stackptr->lvalue)
// ==== this is a bad test. It returns true for char address, too
#define StackItemisString() (StackItemisAddress() && Ctx.Stackptr->type == CHAR)

#define isTypedef(var) (((var)->vkind&TYPEDEF) != 0)
#define isArray(var) ((var)->vdims[0])
#define isPointerArray(var) (((var)->vclass) > ArrayDimensions(var) && isArray(var))
#define isPointer(var) ((((var)->vclass) && !isArray(var))||isPointerArray(var))
#define isAddressOrPointer(var) ((var)->vclass)

#define rslvaddr(addr, lval) (lval ? *addr : (char *)addr)
#define rslvsize(size, class) (class ? sizeof(void *) : size)

#endif
