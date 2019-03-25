/* --------- symbols.c --------- */

#include <string.h>
#include <stdlib.h>
#include "qnc.h"
#include "sys.h"

SYMBOLTABLE LibraryFunctions[] = {
	/* ---- NOTE: These have to be maintained in alphabetic order ---- */
	{ "_Errno",     SYSERRNO     },
	{ "_filename",  SYSFILENAME  },
	{ "_lineno",    SYSLINENO    },
	{ "abs",		SYSABS	 	 },	 
	{ "acos",		SYSACOS 	 },	 
	{ "asctime",	SYSASCTIME	 },
	{ "asin",		SYSASIN 	 },
	{ "atan",		SYSATAN 	 },
	{ "atan2",		SYSATAN 	 },
	{ "atof",		SYSATOF		 },
	{ "atoi",		SYSATOI		 },
	{ "atol",		SYSATOL		 },
	{ "ceil",		SYSCEIL 	 },
	{ "clrscr",		SYSCLRSCRN   },
	{ "cos",		SYSCOS 		 },
	{ "cosh",		SYSCOSH 	 },
	{ "cprintf", 	SYSCPRINTF	 },
	{ "cursor",		SYSCURSOR 	 },
	{ "exit",		SYSEXIT 	 },	 
	{ "exp",		SYSEXP 		 },
	{ "fabs",		SYSFABS 	 },
	{ "fclose",		SYSFCLOSE 	 },
	{ "fflush",     SYSFFLUSH    },
	{ "fgetc",		SYSFGETC 	 },
	{ "fgets",		SYSFGETS 	 },
	{ "findfirst",	SYSFINDFIRST },
	{ "findnext", 	SYSFINDNEXT  },
	{ "floor",		SYSFLOOR 	 },
	{ "fopen",		SYSFOPEN 	 },	 
	{ "fprintf",	SYSFPRINTF	 },
	{ "fputc",		SYSFPUTC 	 },
	{ "fputs",		SYSFPUTS 	 },
	{ "fread",		SYSFREAD 	 },
	{ "free",		SYSFREE 	 },
	{ "fscanf",		SYSFSCANF 	 },	 
	{ "fseek",		SYSFSEEK 	 },
	{ "ftell",		SYSFTELL 	 },
	{ "fwrite",		SYSFWRITE 	 },
	{ "getch",		SYSGETCH     },
	{ "getchar",	SYSGETCHAR   },
	{ "gets",		SYSGETS  	 },
	{ "gmtime",		SYSGMTIME	 },
	{ "localtime",	SYSLOCALTIME },
	{ "log",		SYSLOG 		 },
	{ "log10",		SYSLOG10 	 },
	{ "longjmp",  	SYSLONGJMP   },
	{ "malloc",		SYSMALLOC 	 },
	{ "mktime",		SYSMKTIME	 },
	{ "pow",		SYSPOW 		 },
	{ "printf", 	SYSPRINTF	 },
	{ "putch",		SYSPUTCH     },
	{ "putchar",	SYSPUTCHAR   },
	{ "puts",		SYSPUTS  	 },
	{ "remove", 	SYSREMOVE 	 },
	{ "rename", 	SYSRENAME 	 },
	{ "rewind", 	SYSREWIND 	 },
	{ "scanf",		SYSSCANF 	 },	 
	{ "setjmp",	  	SYSSETJMP    },
	{ "sin",		SYSSIN 		 },
	{ "sinh",		SYSSINH 	 },
	{ "sprintf",	SYSSPRINTF 	 },
	{ "sqrt",		SYSSQRT 	 },
	{ "sscanf",		SYSSSCANF 	 },	 
	{ "strcat",		SYSSTRCAT 	 },	 
	{ "strcmp",		SYSSTRCMP 	 },	 
	{ "strcpy",		SYSSTRCPY 	 },
	{ "strlen",		SYSSTRLEN 	 },
	{ "strncat",	SYSSTRNCAT 	 },	 
	{ "strncmp",	SYSSTRNCMP 	 },
	{ "strncpy",	SYSSTRNCPY 	 },
	{ "system",	  	SYSSYSTEM    },
	{ "tan",		SYSTAN 		 },
	{ "tanh",		SYSTANH 	 },
	{ "time",		SYSTIME		 },
	{ "tmpfile", 	SYSTMPFILE 	 },
	{ "tmpnam", 	SYSTMPNAM 	 },
	{ "ungetc",		SYSUNGETC 	 }
};
#define MAXLIBFUNCTIONS (sizeof(LibraryFunctions)/sizeof(SYMBOLTABLE))

/* --------- keyword lookup table ------------ */
static SYMBOLTABLE Keywords[] = {
	/* ---- NOTE: These have to be maintained in alphabetic order ---- */
	{ "auto",		T_AUTO 		},
	{ "break",		T_BREAK 	},
	{ "case",		T_CASE 		},
	{ "char",		T_CHAR 		},
	{ "const",		T_CONST 	},
	{ "continue",	T_CONTINUE 	},
	{ "default",	T_DEFAULT 	},
	{ "do",			T_DO 		},
	{ "double",		T_DOUBLE 	},
	{ "else",		T_ELSE 		},
	{ "enum",		T_ENUM 		},
	{ "extern",		T_EXTERN 	},
	{ "float",		T_FLOAT 	},
	{ "for",		T_FOR 		},
	{ "goto",		T_GOTO 		},
	{ "if",			T_IF 		},
	{ "int",		T_INT 		},
	{ "long",		T_LONG 		},
	{ "register",	T_REGISTER 	},
	{ "return",		T_RETURN 	},
	{ "short",      T_SHORT     },
	{ "sizeof",		T_SIZEOF 	},
	{ "static",		T_STATIC 	},
	{ "struct",		T_STRUCT 	},
	{ "switch",		T_SWITCH 	},
	{ "typedef",    T_TYPEDEF   },
	{ "union",		T_UNION 	},
	{ "unsigned",   T_UNSIGNED  },
	{ "void",		T_VOID 		},
	{ "volatile",	T_VOLATILE 	},
	{ "while",		T_WHILE		}
};
#define MAXKEYWORDS (sizeof(Keywords)/sizeof(SYMBOLTABLE))

/* -------- multi-character operator lookup tbl ------------ */
static SYMBOLTABLE Operators[] = {
	/* ---- NOTE: These have to be maintained in collating order ---- */
	{ "!=",		T_NE },
	{ "&&",		T_LAND },
	{ "++",		T_INCR },
	{ "--",		T_DECR },
	{ "->",		T_ARROW },
	{ "<<",		T_SHL },
	{ "<=",		T_LE },
	{ "==",		T_EQ },
	{ ">=",		T_GE },
	{ ">>",		T_SHR },
	{ "||",		T_LIOR }
};
#define MAXOPERATORS (sizeof(Operators)/sizeof(SYMBOLTABLE))

static SYMBOLTABLE PreProcessors[] = {
	/* ---- NOTE: These have to be maintained in collating order ---- */
	{ "define",  P_DEFINE  },
	{ "elif",    P_ELIF    },
	{ "else",    P_ELSE    },
	{ "endif",   P_ENDIF   },
	{ "error",   P_ERROR   },
	{ "if",      P_IF      },
	{ "ifdef",   P_IFDEF   },
	{ "ifndef",  P_IFNDEF  },
	{ "include", P_INCLUDE },
	{ "undef",   P_UNDEF   }
};
#define MAXPREPROCESSORS (sizeof(PreProcessors)/sizeof(SYMBOLTABLE))

int SearchSymbols(char *arg, SYMBOLTABLE *tbl, int siz, int wd)
{
	int i, mid, lo, hi;

	lo = 0;
	hi = siz-1;

	while (lo <= hi)	{
		mid = (lo + hi) / 2;
		i = wd ? strncmp(arg, tbl[mid].symbol, wd) :
				 strcmp(arg, tbl[mid].symbol);
		if (i < 0)
			hi = mid-1;
		else if (i)
			lo = mid + 1;
		else
			return tbl[mid].ident;
	}
	return 0;
}

int SearchLibrary(char *fname)
{
	return SearchSymbols(fname, LibraryFunctions, MAXLIBFUNCTIONS, 0);
}

int FindKeyword(char *keyword)
{
	return SearchSymbols(keyword, Keywords, MAXKEYWORDS, 0);
}

int FindOperator(char *oper)
{
	return SearchSymbols(oper, Operators, MAXOPERATORS, 2);
}

int FindPreProcessor(char *preproc)
{
	return SearchSymbols(preproc, PreProcessors, MAXPREPROCESSORS, 0);
}

int FindSymbol(char *sym)
{
	if (SymbolTable != NULL)
		return SearchSymbols(sym, SymbolTable, SymbolCount, 0);
	return 0;
}

char *FindSymbolName(int id)
{
	int i;
	for (i = 0; i < SymbolCount; i++)
		if (SymbolTable[i].ident == id)
			return SymbolTable[i].symbol;
	return NULL;
}

int AddSymbol(char *sym)
{
	int symbolid = 0;
	if (SymbolTable != NULL)	{
		symbolid = FindSymbol(sym);
		if (symbolid == 0)	{
			if (SymbolCount < qCfg.MaxSymbolTable)	{
				int i, j;
				int len = strlen(sym)+1;
				char *s = getmem(len);
				strcpy(s, sym);
				for (i = 0; i < SymbolCount; i++)
					if (strcmp(sym, SymbolTable[i].symbol) < 0)
						break;
				for (j = SymbolCount; j > i; --j)
					SymbolTable[j] = SymbolTable[j-1];
				SymbolTable[i].symbol = s;
				SymbolTable[i].ident = ++SymbolCount;
				symbolid = SymbolCount;
			}
			else
				error(SYMBOLTABLERR);
		}
	}
	return symbolid;
}

void DeleteSymbols(void)
{
	int i;
	for (i = 0; i < SymbolCount; i++)
		free(SymbolTable[i].symbol);
}



