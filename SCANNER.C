/*
 * QUINCY Interpreter - scanner.c - Lexical analyzer routines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <dos.h>
#include "qnc.h"
#include "sys.h"
#undef isxdigit

#define MAXDEFLVL	10
static unsigned char *Defstack[MAXDEFLVL];
static int Deflevel;

static void sappend(char **, char *);
static int uncesc(char **);
static void fltnum(char **, char **);
static void intnum(char **, char **);

int getoken()
{
	static int isStruct;
	for (;;)	{
		switch (Ctx.Token = *Ctx.Progptr++)	{
			case T_LINENO:
				Ctx.CurrFileno = *Ctx.Progptr++;
				Ctx.CurrLineno = *(int*)Ctx.Progptr;
				Ctx.Progptr += sizeof(int);
				break;
			case ' ':
				break;
			case T_EOF:
				Ctx.Value.ival = *Ctx.Progptr--;
				isStruct = 0;
				return Ctx.Token;

			case T_SYMBOL:
				Ctx.Value.ival = *(int*)Ctx.Progptr;
				Ctx.Curvar = SearchVariable(Ctx.Value.ival, isStruct);
				if (!isStruct && Ctx.Curvar == NULL)
					Ctx.Curvar = SearchVariable(Ctx.Value.ival, 1);
				Ctx.Progptr += sizeof(int);
				isStruct = 0;
				return Ctx.Token;
			case T_IDENTIFIER:
				isStruct = 0;
				Ctx.Curvar = MK_FP(FP_SEG(VariableMemory),
										*(unsigned*)Ctx.Progptr);
				Ctx.Progptr += sizeof(int);
				return Ctx.Token;
			case T_FUNCTION:
				Ctx.Curfunction = FindFunction(*(int*)Ctx.Progptr);
				Ctx.Progptr += sizeof(int);
				return Ctx.Token;

			case T_FUNCTREF:
				Ctx.Curfunction = FunctionMemory + *(int*)Ctx.Progptr;
				Ctx.Progptr += sizeof(int);
				return Ctx.Token;
			case T_CHRCONST:
				Ctx.Value.ival = *Ctx.Progptr++;
				return Ctx.Token;
			case T_STRCONST:
				Ctx.Value.cptr = Ctx.Progptr + 1;
				Ctx.Progptr += *Ctx.Progptr;
				return Ctx.Token;
			case T_INTCONST:
				Ctx.Value.ival = *((int *)Ctx.Progptr);
				Ctx.Progptr += sizeof(int);
				return Ctx.Token;
			case T_LNGCONST:
				Ctx.Value.lval = *((long *)Ctx.Progptr);
				Ctx.Progptr += sizeof(long);
				return Ctx.Token;
			case T_FLTCONST:
				Ctx.Value.fval = *((double *)Ctx.Progptr);
				Ctx.Progptr += sizeof(double);
				return Ctx.Token;
			case T_STRUCT:
			case T_UNION:
				isStruct = 1;
				return Ctx.Token;
			default:
				isStruct = 0;
				return Ctx.Token;
		}
	}
}

int tokenize(char *tknbuf, char *srcbuf)
{
	/*
	 * Lexical analyzer converts C code in srcbuf to tokens in tknbuf.
	 */
	char *start, *laststring = NULL, *cp, c, c2, c3, op;
	char buf[8];
	int i;
	int BraceCount = 0;
	char *tknptr = tknbuf;
	int sawCond = 0;
	int sawCase = 0;

	while (*srcbuf)	{
		if ((i = FindOperator(srcbuf)) != 0)	{
			srcbuf+=2;
			if ((i == T_SHL || i == T_SHR) && *srcbuf == '=')	{
				srcbuf++;
				i |= 0x80;
			}
			*tknptr++ = i;
			continue;
		}
		c = *srcbuf++;
		c &= 0x7f;
		op = 0;
		c2 = *srcbuf;
		c3 = *(srcbuf+1);
		if (c != '"' && c != '\n')
			laststring = NULL;
		switch (c)	{
			case '\n':
			{
				/* --- File/Line --- 
			 	 *  _____________
			 	 * | T_LINENO    |
			 	 * |_____________|
			 	 * |fileno (byte)|
			 	 * |_____________|
			 	 * |             |
			 	 * |lineno (word)| 
			 	 * |             |
			 	 * |_____________|
				 */
				handshake();   /* to keep D-Flat clock ticking */
				*tknptr++ = T_LINENO;
				Ctx.CurrFileno = atoi(srcbuf+2);
				*tknptr++ = (unsigned char) Ctx.CurrFileno;
				srcbuf = strchr(srcbuf, ':');
				Assert(srcbuf != NULL);
				srcbuf++;
				Ctx.CurrLineno = atoi(srcbuf);
				*(int*)tknptr = Ctx.CurrLineno;
				tknptr += sizeof(int);
				srcbuf = strchr(srcbuf, '/');
				Assert(srcbuf != NULL);
				srcbuf++;
				break;
			}
			case '"':
				/*
			 	* It's a string constant:
			 	*  ___________
			 	* | T_STRCONST|
			 	* |___________|
			 	* |   length  |
			 	* |___________|
			 	* |   char    |
			 	* |___________|
			 	* |     .     |
			 	* |___________|
			 	* |     0     |
			 	* |___________|
			 	*/
				if (laststring != NULL)
					/* ---- concatenated string ---- */
					tknptr = laststring+strlen(laststring);
				else	{
					*tknptr++ = T_STRCONST;
					laststring = tknptr++;
				}
				while ((c = *srcbuf) != '"' && c)
					*tknptr++ = uncesc(&srcbuf);
				*tknptr++ = '\0';
				*laststring = tknptr - laststring;

				if (c)
					++srcbuf;
				break;
			case '\'':
				/*
			 	* It's a character constant
			 	*  ___________
			 	* | T_CHRCONST|
			 	* |___________|
			 	* |   value   |
			 	* |___________|
			 	*/
				*tknptr++ = T_CHRCONST;
				*tknptr++ = uncesc(&srcbuf);

				/*
			 	* Skip to delimiting apostrophe or end of line.
			 	*/
				while ((c = *srcbuf++) != '\'' && c)
					;
				if (!c)
					--srcbuf;
				break;
			/*
		 	* All kinds of operators:
		 	*  ___________
		 	* |  op token |
		 	* |___________|
		 	*/
			case '=':
				if (op)	{
					tknptr[-1] |= 128;
					++srcbuf;
				}
				else if (c2 == '=')	{
					*tknptr++ = T_EQ;
					++srcbuf;
				}
				else
					*tknptr++ = T_ASSIGN;
				break;
			case '*':
			case '^':
			case '%':
			case '&':
			case '|':
			case '+':
			case '-':
			case '/':
				op = c;
			case '!':
			case '<':
			case '>':
			case '[':
			case ']':
			case '(':
			case ')':
			case ',':
			case '~':
			case ' ':
			case ';':
				*tknptr++ = c;
				break;
			case '?':
				sawCond++;
				*tknptr++ = c;
				break;
			case ':':
				if (sawCond)
					--sawCond;
				sawCase = 0;
				*tknptr++ = c;
				break;
			case '{':
				BraceCount++;
				*tknptr++ = c;
				break;
			case '}':
				--BraceCount;
				*tknptr++ = c;
				break;
			case '.':
				if (c2 == '.' && c3 == '.')	{
					*tknptr++ = T_ELLIPSE;
					srcbuf += 2;
				}
				else if (isdigit(c2)) {
					/*
				 	* floating pointer number.
				 	*/
					--srcbuf;
					fltnum(&srcbuf, &tknptr);
				}
				else 
					*tknptr++ = c;
				break;
			default:
				if (isdigit(c))	{
					/*
				 	 * It's some kind of number:
				 	 *  ___________
				 	 * | T_INTCONST| (or T_LNGCONST, T_FLTCONST, etc.)
				 	 * |___________|
				 	 * |   value   | <- binary value of the
				 	 * |___________|    number. Number of
				 	 * |     .     |    bytes depends on type
				 	 * |___________|
				 	 * |     .     |
				 	 * |___________|
				 	 * |     .     |
				 	 * |___________|
				 	 */
					--srcbuf;
					intnum(&srcbuf, &tknptr);
				}
				else if (alphanum(c))	{
					/*
				 	 * identifier
				 	 */
					start = cp = tknptr+2;
					--srcbuf;
					while (alphanum(*srcbuf))
						*cp++ = *srcbuf++;
					*cp++ = 0;
					if ((i = FindKeyword(start)) != 0)	{
						/*
					 	 * keyword
					 	 *  ___________
					 	 * | key token |
					 	 * |___________|
					 	 */
						*tknptr++ = i;
						if (i == T_CASE)
							sawCase = 1;
					}
					else if (!sawCond && !sawCase && *srcbuf == ':')	{
						/*
						 * label for gotos
						 */
						VARIABLE var, *lvar;
						NullVariable(&var);
						var.vkind = LABEL;
						var.vsymbolid = AddSymbol(start);
						var.vclass = BraceCount;
						lvar = InstallVariable(&var, &Ctx.Curfunction->locals, 0, 0, 1, 0);
						lvar->voffset = tknptr - tknbuf;
						srcbuf++;
					}
					else	{
						/*
						 * symbol, function declaration,
						 * prototype, or call?
						 */
						FUNCTION *funcp;
						int fsymbol = AddSymbol(start);
						
						if ((funcp = FindFunction(fsymbol)) != NULL)	{
							/* ----- declaration, function call, or address
					 	 	 *  _____________
						 	 * |  T_FUNCTREF |
						 	 * |_____________|
						 	 * | Function No |
						 	 * |             |
						 	 * |_____________|
							 */
							*tknptr++ = T_FUNCTREF;
							*(unsigned *)tknptr = (funcp - FunctionMemory);
							tknptr += sizeof(unsigned);
						}
						else if (*srcbuf == '(' && BraceCount == 0)	{
							FUNCTION func;
							NullFunction(&func);
							/* --- declaration or prototype --- */
							/*
					 	 	 *  _____________
					 	 	 * | T_FUNCTION  |
					 	 	 * |_____________|
					 	 	 * |symbol offset|
						 	 * |             |
					 	 	 * |_____________|
					 	 	 */
							/* ----- install the function ------- */
							func.symbol = fsymbol;
							func.libcode = SearchLibrary(start);
							func.ismain = (strcmp(start, "main") == 0);
							func.fileno = Ctx.CurrFileno;
							func.lineno = Ctx.CurrLineno;
							Ctx.Curfunction = NextFunction;
							InstallFunction(&func);
							*tknptr++ = T_FUNCTION;
							*(int *)tknptr = func.symbol;
							tknptr += sizeof(int);
						}
						else	{
							/*
					 	 	 * variable reference:
					 	 	 *  _____________
					 	 	 * |  T_SYMBOL   |
					 	 	 * |_____________|
				 	 	 	 * |symbol offset|
					 	 	 * |             |
					 	 	 * |_____________|
					 	 	 */
							*tknptr++ = T_SYMBOL;
							*(int *)tknptr = fsymbol;
							tknptr += sizeof(int);
						}
					}
				}
				else
					/*
				 	 * Bad character in input line
				 	 */
					error(LEXERR);
		}
		if (*srcbuf == '=' && op)	{
			tknptr[-1] |= 128;
			++srcbuf;
		}
	}
	*tknptr++ = T_EOF;
	*tknptr = '\0';
	return tknptr - tknbuf;
}

static int uncesc(char **bufp)
{
	/*
	 * Unescape character escapes
	 */
	char *buf, c;

	buf = *bufp;
	if ((c = *buf++)  == '\\')	{
		int i;
		char n[4];

		switch (c = *buf++)	{
			case 'a':  c = '\a'; break;
			case 'b':  c = '\b'; break;
			case 'f':  c = '\f'; break;
			case 'n':  c = '\n'; break;
			case 'r':  c = '\r'; break;
			case 't':  c = '\t'; break;
			case 'v':  c = '\v'; break;
			case '\\': c = '\\'; break;
			case '\'': c = '\''; break;
			case '"':  c = '"';  break;
			case 'x':
				sscanf(buf, "%x", &i);
				c = i;
				while (isxdigit(*buf))
					buf++;
				break;
			default:
				if (isdigit(c))	{
					--buf;
					for (i=0; i<3 && isdigit(*buf); ++i)
						n[i] = *buf++;
					n[i] = 0;
					sscanf(n, "%o", &i);
					c = i;
				}
				break;
		}
	}
	*bufp = buf;

	return c;
}

void skip(char left, char right)
{
	/*
	 * Skip balanced delimiters and everything in between
	 */
	int parity;
	unsigned char *svprogptr;

	parity = 1;
	svprogptr = Ctx.Progptr;
	while (getoken() != T_EOF)	{
		if (Ctx.Token == left)
			++parity;
		else if (Ctx.Token == right)
			--parity;
		if (!parity)	{
			Ctx.svpptr = Ctx.Progptr;
			getoken();
			return;
		}
	}
	Ctx.Progptr = svprogptr;
	error(RBRACERR);
}

static void fltnum(char **srcstr, char **tknstr)
{
	/*
	 * Parse a floating point number
	 */
	char *srcp, *cp;
	char numbuf[64];
	char c, n, dot, e, sign;
	double f, atof();

	n = dot = e = sign = 0;
	srcp = *srcstr;
	**tknstr = T_FLTCONST;
	++(*tknstr);

	while (*srcp)	{
		if ((c = *srcp++) == '.')	{
			if (dot)	{
				/*
				 * Already saw a dot.
				 */
				--srcp;
				break;
			}
			++dot;
		}
		else if (c=='e' || c=='E')	{
			if (!(dot || n) || e)	{
				/*
				 * 'E' does not immediately follow a dot
				 * or number.
				 */
				--srcp;
				break;
			}
			++e;
		}
		else if (c=='+' || c=='-')	{
			if (e!=1 || sign)	{
				/*
				 * Sign does not immediately follow an 'E'
				 */
				--srcp;
				break;
			}
			++sign;
		}
		else if (isdigit(c))	{
			++n;
			if (e)	{
				/*
				 * number follows an 'E' - don't allow
				 * the sign anymore.
				 */
				++e;
			}
		}
		else	{
			--srcp;
			break;
		}
	}
	/*
	 * copy the number into a local buffer and NULL terminate it.
	 */
	n = 0;
	cp = *srcstr;
	while (cp < srcp)
		numbuf[n++] = *cp++;
	numbuf[n] = 0;

	f = atof(numbuf);
	movmem(&f, *tknstr, sizeof(double));
	*srcstr = srcp;
	*tknstr += sizeof(double);
}

static void intnum(char **srcstr, char **tknstr)
{
	/*
	 * Parse a decimal, octal or hexadecimal number
	 */
	char *srcp, *cp, c;
	int i;
	long j, atol();
	int isDecimal = 1;

	/*
	 * test for float number
	 */
	srcp = *srcstr;
	while (isdigit(*srcp))
		++srcp;
	if (*srcp == '.' || *srcp == 'e' || *srcp == 'E')	{
		fltnum(srcstr, tknstr);
		return;
	}
	/* ----- not a float ----- */
	c = T_INTCONST;
	srcp = *srcstr;
	if (*srcp++ == '0')	{
		if (isdigit(*srcp))	{
			/*
			 * octal constant
			 */
			sscanf(srcp, "%o", &i);
			while (isdigit(*srcp))
				++srcp;
			isDecimal = 0;
		}
		else if (tolower(*srcp) == 'x')	{
			/*
			 * It's a hexadecimal constant
			 */
			sscanf(++srcp, "%x", &i);
			while (isxdigit(*srcp))
				++srcp;
			isDecimal = 0;
		}
	}
	if (isDecimal)	{
		cp = --srcp;
		while (isdigit(*cp))
			++cp;
		/*
		 * decimal integer number
		 */
		i = atoi(srcp);
		j = atol(srcp);
		if (*cp == 'U')
			cp++;
		if (*cp == 'l' || *cp == 'L')	{
			c = T_LNGCONST;
			++cp;
		}
		else if (j != (long)i)
			c = T_LNGCONST;
		srcp = cp;
	}
	*srcstr = srcp;

	**tknstr = c;
	++(*tknstr);
	if (c == T_LNGCONST)	{
		*((long *)*tknstr) = j;
		*tknstr += sizeof(long);
	}
	else	{
		*((int *)*tknstr) = i;
		*tknstr += sizeof(int);
	}
}

