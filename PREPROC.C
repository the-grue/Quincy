/* -------- preproc.c -------- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <sys\stat.h>
#include "qnc.h"
#include "preproc.h"
#include "debugger.h"

static MACRO *FirstMacro;
int MacroCount;

/* --- #included source code files --- */
typedef struct SourceFile    {
    unsigned char *fname;
	unsigned char *IncludeIp;
    struct SourceFile *NextFile;
} SRCFILE;
static SRCFILE *FirstFile;
static SRCFILE *LastFile;
static SRCFILE *ThisFile;
static unsigned char FileCount;

static int Skipping[MAXIFLEVELS+1];
static int TrueTest[MAXIFLEVELS+1];
static int IfLevel;
static unsigned char *Line;
static unsigned char *Word;
static unsigned char *FilePath;
static unsigned char *Ip, *Op;

/* ------ local function prototypes ------- */
static void FreeBuffers(void);
static void PreProcess(void);
static void WriteEOL(void);
static void OutputLine(void);
static void DefineMacro(unsigned char*);
static void Include(unsigned char*);
static void UnDefineMacro(unsigned char*);
static void If(unsigned char *);
static void Elif(unsigned char *);
static void IfDef(unsigned char *);
static void IfnDef(unsigned char *);
static void Else(void);
static void Endif(void);
static void Error(unsigned char *);
static void DeleteFileList(SRCFILE *);

static void UnDefineAllMacros(void);
static int ReadString(void);
static void WriteChar(unsigned char);
static void WriteWord(unsigned char*);

/* --- preprocess code in SourceCode into pSrc --- */
void PreProcessor(unsigned char *pSrc,unsigned char *SourceCode)
{
    Op = pSrc;
    Ip = SourceCode;
    Ctx.CurrFileno = 0;
    Ctx.CurrLineno = 0;
    IfLevel = 0;
    Word = getmem(MAXMACROLENGTH);
    FilePath = getmem(128);
    PreProcess();
    if (IfLevel)
        error(IFSERR);
    FreeBuffers();
}
/* --- delete all preprocessor heap usage on error --- */
void CleanUpPreProcessor(void)
{
    FreeBuffers();
    DeleteFileList(FirstFile);
    FirstFile = LastFile = NULL;
    FileCount = 0;
}
/* ---- free heap buffers used by preprocessor ---- */
static void FreeBuffers(void)
{
    UnDefineAllMacros();
    free(Line);
    free(FilePath);
    free(Word);
    FilePath  = NULL;
    Word      = NULL;
    Line      = NULL;
}
/* ---- bypass source code white space ---- */
void bypassWhite(unsigned char **cp)
{
    while (isSpace(**cp))
        (*cp)++;
}
/* ---- extract a word from input --- */
void ExtractWord(unsigned char *wd,
                   unsigned char **cp, unsigned char *allowed)
{
    while (**cp)    {
        if (isalnum(**cp) || strchr(allowed, **cp))
            *wd++ = *((*cp)++);
        else
            break;
    }
    *wd = '\0';
}
/* ---- internal preprocess entry point ---- */
static void PreProcess()
{
    unsigned char *cp;
    while (ReadString() != 0)    {
		handshake();   /* to keep D-Flat clock ticking */
        if (Line[strlen(Line)-1] != '\n')
            error(LINETOOLONGERR);
        cp = Line;
        bypassWhite(&cp);
        if (*cp != '#')    {
            if (!Skipping[IfLevel])
                OutputLine();
            continue;
        }
        cp++;
        /* --- this line is a preprocessing token --- */
        bypassWhite(&cp);
        ExtractWord(Word, &cp, "");
		if (*Word ==  '\0')
			continue;
        switch (FindPreProcessor(Word))    {
            case P_DEFINE:
	            if (!Skipping[IfLevel])
                    DefineMacro(cp);
                break;
            case P_ELSE:
                Else();
                break;
            case P_ELIF:
                Elif(cp);
                break;
            case P_ENDIF:
                Endif();
                break;
			case P_ERROR:
				Error(cp);
				break;
            case P_IF:
                If(cp);
                break;
            case P_IFDEF:
                IfDef(cp);
                break;
            case P_IFNDEF:
                IfnDef(cp);
                break;
            case P_INCLUDE:
	            if (!Skipping[IfLevel])
                    Include(cp);
                break;
            case P_UNDEF:
	            if (!Skipping[IfLevel])
                    UnDefineMacro(cp);
                break;
            default:
                error(BADPREPROCERR);
                break;
        }
    }
	WriteEOL();
}
/* ----- find a macro that is already #defined ----- */
MACRO *FindMacro(unsigned char *ident)
{
    MACRO *ThisMacro = FirstMacro;
    while (ThisMacro != NULL)    {
        if (strcmp(ident, ThisMacro->id) == 0)
            return ThisMacro;
        ThisMacro = ThisMacro->NextMacro;
    }
    return NULL;
}
/* ----- compare macro parameter values ---- */
static int parmcmp(char *p, char *t)
{
    char tt[80];
    char *tp = tt;
    while (alphanum(*t))
        *tp++ = *t++;
    *tp = '\0';
    return strcmp(p, tt);
}
/* ---- add a newly #defined macro to the table ---- */
static void AddMacro(unsigned char *ident,unsigned char *plist,
                            unsigned char *value)
{
    char *prms[MAXPARMS];

    MACRO *ThisMacro = getmem(sizeof(MACRO));
    ThisMacro->id = getmem(strlen(ident)+1);
    strcpy(ThisMacro->id, ident);
    /* ---- find and count parameters ---- */
    if (plist)    {
        /* ---- there are parameters ---- */
        ThisMacro->isMacro = 1;
        plist++;
        while (*plist != ')')    {
            while (isSpace(*plist))
                plist++;
            if (alphanum(*plist))    {
                if (ThisMacro->parms == MAXPARMS)
                    error(DEFINERR);
                prms[ThisMacro->parms++] = plist;
                while (alphanum(*plist))
                    plist++;
            }
            while (isSpace(*plist))
                plist++;
            if (*plist == ',')
                plist++;
            else if (*plist != ')')
                error(DEFINERR);
        }
    }
    /* --- build value substituting parameter numbers --- */
    if (value != NULL)    {
        /* ---- there is a value ---- */
        ThisMacro->val =
            getmem(strlen(value)+1+ThisMacro->parms);
        if (ThisMacro->parms)    {
            char *pp = ThisMacro->val;
			int wasWord = 0;
            while (*value)    {
				int wasWhite = isSpace(*value);
				if (*value == '\'' || *value == '"')	{
					char term = *value;
					do	{
						if (!*value || *value == '\n')
							error(DEFINERR);
						*pp++ = *value++;
					} while (*value != term);
					*pp++ = *value++;
					continue;
				}
				bypassWhite(&value);
                if (alphanum(*value))    {
                    int p = 0;
					if (wasWhite && wasWord)
						*pp++ = ' ';
					wasWord = 1;
                    ExtractWord(Word, &value, "_");
                    while (p < ThisMacro->parms)    {
                        if (parmcmp(Word, prms[p]) == 0)  {
                            *pp++ = p | 0x80;
                            break;
                        }
                        p++;
                    }
                    if (p == ThisMacro->parms)    {
                        strcpy(pp, Word);
                        pp += strlen(Word);
                    }
					continue;
                }
				wasWord = 0;
                *pp++ = *value++;
            }
            *pp = '\0';
        }
        else
            /* --- no parameters, straight substitution --- */
            strcpy(ThisMacro->val, value);
    }
    ThisMacro->NextMacro = FirstMacro;
    FirstMacro = ThisMacro;
    MacroCount++;
}
/* ----- #define a new macro ----- */
static void DefineMacro(unsigned char *cp)
{
    unsigned char *vp = NULL, *vp1;
    unsigned char *lp = NULL;
    bypassWhite(&cp);
    ExtractWord(Word, &cp, "_");
    if (FindMacro(Word) != NULL)
        error(REDEFPPERR);    /* --- already defined --- */
    /* ---- extract parameter list ---- */
    if (*cp == '(')    {
		int len = 0;
        while (cp[len] && cp[len] != ')' && cp[len] != '\n')
            len++;
        if (cp[len] != ')')
            error(DEFINERR);
		len++;
		lp = getmem(len+1);
		strncpy(lp, cp, len);
		cp += len;
    }
    bypassWhite(&cp);
    /* ---- extract parameter definition ---- */
    if (*cp)
        vp = getmem(strlen(cp)+1);
    vp1 = vp;
    while (*cp && *cp != '\n')    {
        char *cp1 = cp;
        while (*cp && *cp != '\n')
            cp++;
        --cp;
        while (isSpace(*cp))
            --cp;
        cp++;
        strncpy(vp1, cp1, cp-cp1);
        vp1[cp-cp1] = '\0';
        vp1 = vp + strlen(vp)-1;
        if (*vp1 != '\\')
            break;
		do
			--vp1;
		while (isSpace(*vp1));
		vp1[1] = '\0';
        ReadString();
        cp = Line;
        bypassWhite(&cp);
        vp = realloc(vp, strlen(vp)+strlen(cp)+1);
        if (vp == NULL)
            error(OMERR);
        vp1 = vp + strlen(vp);
    }
    if (strcmp(Word, vp))
        AddMacro(Word, lp, vp);
    free(vp);
	free(lp);
}
/* ----- remove all macros ------ */
static void UnDefineAllMacros(void)
{
    MACRO *ThisMacro = FirstMacro;
    while (ThisMacro != NULL)    {
        MACRO *tm = ThisMacro;
        free(ThisMacro->val);
        free(ThisMacro->id);
        ThisMacro = ThisMacro->NextMacro;
        free(tm);
    }
    FirstMacro = NULL;
    MacroCount = 0;
}
/* ------ #undef a macro ------- */
static void UnDefineMacro(unsigned char *cp)
{
    MACRO *ThisMacro;
    bypassWhite(&cp);
    ExtractWord(Word, &cp, "_");
    if ((ThisMacro = FindMacro(Word)) != NULL)    {
        if (ThisMacro == FirstMacro)
            FirstMacro = ThisMacro->NextMacro;
        else     {
            MACRO *tm = FirstMacro;
            while (tm != NULL)    {
                if (ThisMacro == tm->NextMacro)    {
                    tm->NextMacro = ThisMacro->NextMacro;
                    break;
                }
                tm = tm->NextMacro;
            }
        }
        free(ThisMacro->val);
        free(ThisMacro->id);
        free(ThisMacro);
        --MacroCount;
    }
}
/* ------ #include a source code file ------ */
static void Include(unsigned char *cp)
{
    FILE *fp;
    int LocalInclude;
    int holdcount;
    unsigned char holdfileno;
    unsigned char *holdip;
    SRCFILE *holdfile;
    struct stat sb;
	static int Nesting = 0;

	if (Nesting++ == MAXINCLUDES)	{
		error(INCLUDENESTERR);
		Nesting = 0;
	}
    holdfile = ThisFile;
    *FilePath = '\0';
    bypassWhite(&cp);
    /* ---- test for #include <file> or #include "file" ---- */
    if (*cp == '"')
        LocalInclude = 1;
    else if (*cp == '<')
        LocalInclude = 0;
    else
        error(BADPREPROCERR);
    cp++;
    /* ---- extract the file name ---- */
    ExtractWord(Word, &cp, ".$_\\");
    if (*cp != (LocalInclude ? '"' : '>'))
        error(BADPREPROCERR);
    /* ---- build path to included file ---- */
    if (!LocalInclude)    {
        unsigned char *pp;
        strcpy(FilePath, _argv[0]);
        pp = strrchr(FilePath, '\\');
        if (pp != NULL)
            *(pp+1) = '\0';
    }
    strcat(FilePath, Word);
    /* ---- add to list of included files --- */
    ThisFile = getmem(sizeof(SRCFILE));
    ThisFile->fname = getmem(strlen(Word)+1);
    strcpy(ThisFile->fname, Word);
    if (LastFile != NULL)
        LastFile->NextFile = ThisFile;
    ThisFile->NextFile = NULL;
    LastFile = ThisFile;
    if (FirstFile == NULL)
        FirstFile = ThisFile;
    /* ----- get file size ----- */
    stat(FilePath, &sb);
    /* - save context of file currently being preprocessed - */ 
    holdip = Ip;
    holdcount = Ctx.CurrLineno;
    holdfileno = Ctx.CurrFileno;
    /* --- file/line numbers for #included file --- */
    Ctx.CurrFileno = ++FileCount;
    Ctx.CurrLineno = 0;
    /* -------- open the #included file ------ */
    if ((fp = fopen(FilePath, "rt")) == NULL)
        error(INCLUDEERR);
    /* ---- allocate a buffer and read it in ---- */
	Ip = ThisFile->IncludeIp = getmem(sb.st_size+1);
    fread(Ip, sb.st_size, 1, fp);
    fclose(fp);
    /* ----- preprocess the #included file ------ */
    PreProcess();
    free(Ip);
	ThisFile->IncludeIp = NULL;
    /* restore context of file previously being preprocessed */
    Ctx.CurrFileno = holdfileno;
    Ctx.CurrLineno = holdcount;
    Ip = holdip;
    ThisFile = holdfile;
	--Nesting;
}
/* ---- delete files from the file list ---- */
static void DeleteFileList(SRCFILE *thisfile)
{
    if (thisfile != NULL)    {
		DeleteFileList(thisfile->NextFile);
        free(thisfile->IncludeIp);
        free(thisfile->fname);
        free(thisfile);
    }
}
static int TestDefined(unsigned char **cpp)
{
	int not = 0;
	unsigned char *cp = *cpp;
	bypassWhite(&cp);
	if (*cp == '!')	{
		cp++;
		not = 1;
		bypassWhite(&cp);
	}
	ExtractWord(Word, &cp, "");
	if (strcmp(Word, "defined") == 0)	{
		*cpp = cp;
		return not ? -1 : 1;
	}
	return 0;
}
static int MacroDefined(unsigned char *cp)
{
    bypassWhite(&cp);
    ExtractWord(Word, &cp, "_");
    return (FindMacro(Word) != NULL);
}
static int TestIfLevel(void)
{
	int rtn;
	if (IfLevel == MAXIFLEVELS)
		error(IFNESTERR);
   	rtn = !Skipping[IfLevel++];
	Skipping[IfLevel] = Skipping[IfLevel-1];
	return rtn;
}
/* -------- #if preprocessing token -------- */
static void If(unsigned char *cp)
{
	if (TestIfLevel())	{
		int not;
		if ((not = TestDefined(&cp)) != 0)	{
			int isdef = MacroDefined(cp);
           	TrueTest[IfLevel] =
				(not == 1 && isdef) || (not == -1 && !isdef);
		}
       	else
           	TrueTest[IfLevel] = (MacroExpression(&cp) != 0);
       	Skipping[IfLevel] = !TrueTest[IfLevel];
   	}
}
/* -------- #ifdef preprocessing token -------- */
static void IfDef(unsigned char *cp)
{
	if (TestIfLevel())	{
		TrueTest[IfLevel] = MacroDefined(cp);
       	Skipping[IfLevel] = !TrueTest[IfLevel];
	}
}
/* -------- #ifndef preprocessing token -------- */
static void IfnDef(unsigned char *cp)
{
	if (TestIfLevel())	{
		TrueTest[IfLevel] = !MacroDefined(cp);
       	Skipping[IfLevel] = !TrueTest[IfLevel];
	}
}
/* -------- #else preprocessing token -------- */
static void Else()
{
    if (IfLevel == 0)
        error(ELSEERR);
    Skipping[IfLevel] = TrueTest[IfLevel];
}
/* -------- #elif preprocessing token -------- */
static void Elif(unsigned char *cp)
{
    if (IfLevel == 0)
        error(ELIFERR);
    if (!TrueTest[IfLevel])	{
		int not, eval;
		if ((not = TestDefined(&cp)) != 0)	{
			int isdef = MacroDefined(cp);
			TrueTest[IfLevel] = !((not == 1 && !isdef) || (not == -1 && isdef));
		}
		else
	        TrueTest[IfLevel] = (MacroExpression(&cp) != 0);
	   	Skipping[IfLevel] = !TrueTest[IfLevel];
	}
	else
	   	Skipping[IfLevel] = 1;
}
/* -------- #endif preprocessing token -------- */
static void Endif()
{
   	if (IfLevel == 0)
       	error(ENDIFERR);
   	Skipping[IfLevel] = 0;
   	TrueTest[IfLevel--] = 0;
}
/* -------- #error preprocessing token -------- */
static void Error(unsigned char *cp)
{
    if (IfLevel == 0 || !Skipping[IfLevel])	{
		unsigned char *ep;
    	bypassWhite(&cp);
		for (ep = cp; *ep && *ep != '\n'; ep++)
			;
		--ep;
		while (isSpace(*ep))
			--ep;
		memset(ErrorMsg, 0, sizeof ErrorMsg);
		strncpy(ErrorMsg, cp, ep-cp+1);
		strcat(ErrorMsg, " ");
		error(ERRORERR);
	}
}
static void WriteEOL()
{
    char eol[20];
    sprintf(eol, "\n/*%d:%d*/",
        Ctx.CurrFileno, Ctx.CurrLineno);
    WriteWord(eol);
}
/* ----- write a preprocessed line to output ----- */
static void OutputLine()
{
    unsigned char *cp = Line;
    unsigned char lastcp = 0;
    while (isSpace(*cp))
        cp++;
    if (*cp != '\n')
		WriteEOL();
    while (*cp && *cp != '\n')    {
        if (isSpace(*cp))    {
            while (isSpace(*cp))
                cp++;
            if (alphanum(*cp) && alphanum(lastcp))
                WriteChar(' ');
        }
        if (alphanum(*cp))    {
            ResolveMacro(Word, &cp);
            WriteWord(Word);
            lastcp = 'x';
            continue;
        }
        if (*cp == '/' && *(cp+1) == '*')    {
            int inComment = 1;
            cp += 2;
            while (inComment)    {
                while (*cp && *cp != '\n')    {
                    if (*cp == '*' && *(cp+1) == '/')    {
                        cp += 2;
                        inComment = 0;
                        break;
                    }
                    cp++;
                }
                if (inComment)    {
                    lastcp = ' ';
                    if (ReadString() == 0)
                        error(UNTERMCOMMENT);
                    cp = Line;
                }
            }
            continue;
        }
        else if (*cp == '"' || *cp == '\'')    {
			char term = *cp;
            WriteChar(*cp++);
            while (*cp != term)    {
                if (*cp == '\n' || *cp == '\0')
                    error(term == '"' ? UNTERMSTRERR: UNTERMCONSTERR);
                WriteChar(*cp++);
            }
        }
        lastcp = *cp++;
        WriteChar(lastcp);
    }
}
/* ----- write single character to output ---- */
static void WriteChar(unsigned char c)
{
    *Op++ = c;
}
/* ----- write a null-terminated word to output ----- */
static void WriteWord(unsigned char *s)
{
    int lastch = 0;
    while (*s)    {
        if (*s == '"')    {
            /* --- the word has a string literal --- */
            do
                WriteChar(*s++);
            while (*s && *s != '"');
            if (*s)
                WriteChar(*s++);
            continue;
        }
        if (isSpace(*s))    {
            /* --- white space --- */
            while (isSpace(*s))
                s++;
            /* --- insert one if char literal or id id --- */
            if (lastch == '\'' ||
                    (alphanum(lastch) && alphanum(*s)))
                WriteChar(' ');
        }
        lastch = *s;
        WriteChar(*s++);
    }
}
/* ------ read a line from input ---- */
static int ReadString()
{
    unsigned char *lp;
    Ctx.CurrLineno++;
    if (*Ip)    {
        int len;
        /* --- compute the line length --- */
        lp = strchr(Ip, '\n');
        if (lp != NULL)
            len = lp - Ip + 2;
        else
            len = strlen(Ip)+1;
        if (len)    {
			free(Line);
            Line = getmem(len);
			lp = Line;
            while ((*lp++ = *Ip++)  != '\n')
                if (*(lp-1) == '\0')
                    break;
            if (*(lp-1) == '\n')
                *lp = '\0';
            return 1;
        }
    }
    return 0;
}
/* ----- find file name from file number ---- */
char *SrcFileName(int fileno)
{
	extern WINDOW editWnd;
    ThisFile = FirstFile;
    while (ThisFile != NULL && --fileno)
        ThisFile = ThisFile->NextFile;
    return ThisFile ? ThisFile->fname : editWnd->extension;
}

