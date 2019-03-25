/* ---------- preexpr.c ---------- */

#include <string.h>
#include <stdlib.h>
#include "qnc.h"
#include "preproc.h"

void bypassWhite(unsigned char **cp);

/* ------- compile a #define macro ------- */
static void CompileMacro(unsigned char *wd, MACRO *mp,
                                        unsigned char **cp)
{
    char *args[MAXPARMS];
    int i, argno = 0;
    char *val = mp->val;
	int inString = 0;

    if (**cp != '(')
        error(SYNTAXERR);
    /* ---- pull the arguments out of the macro call ---- */
    (*cp)++;
    while (**cp && **cp != ')')    {
        char *ca = getmem(80);
		int parens = 0, cs = 0;
        args[argno] = ca;
	    bypassWhite(cp);
        while (**cp)	{
			if (**cp == ',' && parens == 0)
				break;
			if (**cp == '(')
				parens++;
			if (**cp == ')')	{
				if (parens == 0)
					break;
				--parens;
			}
			if (cs++ == 80)
		        error(SYNTAXERR);
            *ca++ = *((*cp)++);
		}
        *ca = '\0';
        argno++;
        if (**cp == ',')
            (*cp)++;
    }
    /* ---- build the statement substituting the
                    arguments for the parameters ---- */
    while (*val)    {
        if (*val & 0x80 || (*val == '#' && !inString))    {
            char *arg;
			int stringizing = 0;
			if (*val == '#')	{
				val++;
				if (*val == '#')	{
					val++;
					continue;
				}
				else 	{
					*wd++ = '"';
					stringizing = 1;
				}
			}
            arg = args[*val & 0x3f];
			while (isspace(*arg))
				arg++;
            while (*arg != '\0')
                *wd++ = *arg++;
			if (stringizing)	{
				while (isspace(*(wd-1)))
					--wd;
				*wd++ = '"';
			}
            val++;
        }
        else if ((*wd++ = *val++) == '"')
			inString ^= 1;
    }
    *wd = '\0';
    for (i = 0; i < argno; i++)
        free(args[i]);
    if (argno != mp->parms)
        error(ARGERR);
    if (**cp != ')')
        error(SYNTAXERR);
    (*cp)++;
}
/* ---- resolve a macro to its #defined value ----- */
int ResolveMacro(unsigned char *wd, unsigned char **cp)
{
    unsigned char *mywd = getmem(MAXMACROLENGTH);
    MACRO *mp;
    int sct = 0;
    ExtractWord(wd, cp, "_");
    while (alphanum(*wd) && (mp = FindMacro(wd)) != NULL &&
                        sct != MacroCount)    {
        if (mp->val == NULL)
            break;
        if (mp->isMacro)    {
            unsigned char *mw = mywd;
			int inString = 0;
            CompileMacro(mywd, mp, cp);
            while (*mw)    {
				if (*mw == '"' && (mw == mywd || *(mw-1) != '\\'))
					inString ^= 1;
                if (!inString && alphanum(*mw))    {
                    ResolveMacro(wd, &mw);
                    wd += strlen(wd);
                }
                else
                    *wd++ = *mw++;
            }
            *wd = '\0';
        }
        else
            strcpy(wd, mp->val);
        sct++;
    }
    free(mywd);
    return sct;
}
/* --- recursive descent expression evaluation for #if --- */
static int MacroPrimary(unsigned char **cp)
{
    /* ---- primary:
            highest precedence;
            bottom of the descent ---- */
    int result = 0, tok;
    if (**cp == '(')    {
        /* ---- parenthetical expression ---- */
        (*cp)++;
        result = MacroExpression(cp);
        if (**cp != ')')
            error(IFERR);
        (*cp)++;
    }
    else if (isdigit(**cp) || **cp == '\'')    {
        /* --- numerical constant expression ---- */
        char num[80];
        char con[80];
        char *ch = *cp, *cc = con;
        while (isdigit(*ch) || strchr(".Ee'xX", *ch))
            *cc++ = *ch++;
        *cc = '\0';
        tokenize(num, con);
        *cp += cc - con;
        switch (*num)    {
            case T_CHRCONST:
                result = *(unsigned char*) (num+1);
                break;
            case T_INTCONST:
                result = *(int*) (num+1);
                break;
            case T_LNGCONST:
                result = (int) *(long*) (num+1);
                break;
            default:
                error(IFERR);
        }
    }
    else if (alphanum(**cp))    {
        /* ----- macro identifer expression ----- */
        unsigned char *np = getmem(MAXMACROLENGTH);
        unsigned char *npp = np;
        result = (ResolveMacro(np, cp) == 0) ? 0 :
                                        MacroPrimary(&npp);
        free(np);
    }
    else    {
        /* ----- unary operators ----- */
        tok = **cp;
        (*cp)++;
        result = MacroPrimary(cp);
        switch (tok)    {
            case '+':
                break;
            case '-':
                result = -result;
                break;
            case '!':
                result = !result;
                break;
            case '~':
                result = ~result;
                break;
            default:
                error(IFERR);
        }
    }
    bypassWhite(cp);
    return result;
}
/* ----- * and / operators ----- */
static int MacroMultiplyDivide(unsigned char **cp)
{
    int result = MacroPrimary(cp);
    int iresult, op;
    for (;;)    {
        if (**cp == '*')
            op = 0;
        else if (**cp == '/')
            op = 1;
        else if (**cp == '%')
            op = 2;
        else
            break;
        (*cp)++;
        iresult = MacroPrimary(cp);
        result = op == 0 ? (result * iresult) :
                 op == 1 ? (result / iresult) :
                           (result % iresult);
    }
    return result;
}
/* ------ + and - binary operators ------- */
static int MacroAddSubtract(unsigned char **cp)
{
    int result = MacroMultiplyDivide(cp);
    int iresult, ad;
    while (**cp == '+' || **cp == '-')    {
        ad = **cp == '+';
        (*cp)++;
        iresult = MacroMultiplyDivide(cp);
        result = ad ? (result+iresult) : (result-iresult);
    }
    return result;
}
/* -------- <, >, <=, and >= operators ------- */
static int MacroRelational(unsigned char **cp)
{
    int result = MacroAddSubtract(cp);
	 int iresult;
    while (**cp == '<' || **cp == '>')    {
        int lt = **cp == '<';
        (*cp)++;
        if (**cp ==  '=')    {
            (*cp)++;
            iresult = MacroAddSubtract(cp);
            result = lt ? (result <= iresult) :
                                (result >= iresult);
        }
        else    {
            iresult = MacroAddSubtract(cp);
            result = lt ? (result < iresult) :
                                (result > iresult);
        }
    }
    return result;
}
/* --------  == and != operators  -------- */
static int MacroEquality(unsigned char **cp)
{
    int result = MacroRelational(cp);
    int iresult, eq;
    while ((**cp == '=' || **cp == '!') && *(*cp+1) == '=')  {
        eq = **cp == '=';
        (*cp) += 2;
        iresult = MacroRelational(cp);
        result = eq ? (result==iresult) : (result!=iresult);
    }
    return result;
}
/* ---------- & binary operator ---------- */
static int MacroBoolAND(unsigned char **cp)
{
    int result = MacroEquality(cp);
    while (**cp == '&' && *(*cp+1) != '&')    {
        (*cp) += 2;
        result &= MacroEquality(cp);
    }
    return result;
}
/* ----------- ^ operator ------------- */
static int MacroBoolXOR(unsigned char **cp)
{
    int result = MacroBoolAND(cp);
    while (**cp == '^')    {
        (*cp)++;
        result ^= MacroBoolAND(cp);
    }
    return result;
}
/* ---------- | operator -------- */
static int MacroBoolOR(unsigned char **cp)
{
    int result = MacroBoolXOR(cp);
    while (**cp == '|' && *(*cp+1) != '|')    {
        (*cp) += 2;
        result |= MacroBoolXOR(cp);
    }
    return result;
}
/* ---------- && operator ---------- */
static int MacroLogicalAND(unsigned char **cp)
{
    int result = MacroBoolOR(cp);
    while (**cp == '&' && *(*cp+1) == '&')    {
        (*cp) += 2;
        result = MacroBoolOR(cp) && result;
    }
    return result;
}
/* ---------- || operator -------- */
static int MacroLogicalOR(unsigned char **cp)
{
    int result = MacroLogicalAND(cp);
    while (**cp == '|' && *(*cp+1) == '|')    {
        (*cp) += 2;
        result = MacroLogicalAND(cp) || result;
    }
    return result;
}
/* -------- top of the descent ----------- */
int MacroExpression(unsigned char **cp)
{
    bypassWhite(cp);
    return MacroLogicalOR(cp);
}

