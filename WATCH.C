/* ---------- watch.c --------- */
#include "qnc.h"
#include "interp.h"
#include "debugger.h"

static BOOL Assignable;
static int wHeight = WATCHHT;
static int WatchCount;
char *Watches[MAXWATCHES];
int (*changeproc)(WINDOW, MESSAGE, PARAM, PARAM);

jmp_buf Displayjmp;
int dispsize;
jmp_buf Watchjmp;
int Watching;

static DATUM Svvalue;
static unsigned char *Svprogptr;
static char Svtoken;
static ITEM *Svstackptr;
static VARIABLE *Svcurvar;
static int SvBpCtr;
static BOOL SvStepping;

static void GetVariableValue(char *, char *);
static void GetWatchValue(char *, char *);
static void BuildExamination(WINDOW);
static int ChangeProc(WINDOW, MESSAGE msg, PARAM, PARAM);
static void ChangeVariable(char *, char *);
static int ExamineProc(WINDOW, MESSAGE, PARAM, PARAM);
static void DeleteWatch(void);
static int WatchProc(WINDOW, MESSAGE, PARAM, PARAM);
static void OpenWatch(void);
static void SetEditorSize(void);
static void savegl(void);
static void restgl(void);
static void ccat(char *, int);
static char *tocesc(char, char *);

/* ---------- Watch functions ----------------- */
static void DeleteWatch(void)
{
	if (WatchCount)	{
		int sel = SendMessage(watchWnd, LB_CURRENTSELECTION, 0, 0);
		if (sel > -1)	{
			int i;
			--WatchCount;
			free(Watches[sel]);
			for (i = sel; i < WatchCount; i++)
				Watches[i] = Watches[i+1];

			if (wHeight > WATCHHT)	{
				--wHeight;
				SendMessage(watchWnd, SIZE, GetRight(watchWnd),
											GetBottom(watchWnd)-1);
				SendMessage(watchWnd, MOVE, GetLeft(watchWnd),
											GetTop(watchWnd)+1);
				SetEditorSize();
			}

			UpdateWatches();
			if (sel && sel == WatchCount)
				SendMessage(watchWnd, LB_SETSELECTION, sel-1, 0);
		}
	}
}

static int WatchProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
	switch (msg)	{
		case KEYBOARD:
			switch ((int)p1)	{
				case INS:
					GetWatch();
    	    	    return TRUE;
				case DEL:
					DeleteWatch();
					return TRUE;
				default:
					break;
			}
			break;
		default:
			break;
	}
    return DefaultWndProc(wnd, msg, p1, p2);
}

static void SetEditorSize(void)
{
	SendMessage(editWnd, SIZE, GetRight(editWnd),
							GetBottom(applWnd)-wHeight-3);
}

static void OpenWatch(void)
{
	if (watchWnd == NULL || !isVisible(watchWnd))	{
		SetCommandToggle(&MainMenu, ID_WATCH);
		ToggleWatch();
	}
	SendMessage(watchWnd, SETFOCUS, TRUE, 0);
}

void ToggleWatch(void)
{
	if (GetCommandToggle(&MainMenu, ID_WATCH))	{
		SetEditorSize();
		if (watchWnd != NULL)
			SendMessage(watchWnd, SHOW_WINDOW, 0, 0);
		else
			watchWnd = CreateWindow(LISTBOX, "Watch Variables",
						GetClientLeft(applWnd),
						GetClientBottom(applWnd)-(wHeight+1),
						wHeight+2, ClientWidth(applWnd),
						NULL, applWnd,
						WatchProc,	/* wndproc */
						HASBORDER | VSCROLLBAR | VISIBLE);
		ReFocus(editWnd);
	}
	else	{
		SendMessage(watchWnd, HIDE_WINDOW, 0, 0);
		SendMessage(editWnd, SIZE, GetRight(editWnd),
			GetClientBottom(applWnd));
		SendMessage(editWnd, SETFOCUS, TRUE, 0);
	}
}

void GetWatch(void)
{
	static char VariableID[31] = "";
	if (WatchCount >= MAXWATCHES)
		ErrorMessage("Too many watch variables");
	else if (InputBox(applWnd, DFlatApplication, "Watch variable:",
											VariableID, 30, 0))	{
		AddWatch(VariableID);
		ShowWatches();
	}
}

void ShowWatches(void)
{
	OpenWatch();
	UpdateWatches();
	SendMessage(editWnd, SETFOCUS, TRUE, 0);
}

void AddWatch(char *VariableID)
{
	char *cp;
	Watches[WatchCount] = DFmalloc(strlen(VariableID)+1);
	strcpy(Watches[WatchCount++], VariableID);
	if (WatchCount > wHeight && wHeight < MAXWATCHHT)	{
		wHeight++;
		SetEditorSize();
		SendMessage(watchWnd, MOVE, GetLeft(watchWnd),
									GetTop(watchWnd)-1);
		SendMessage(watchWnd, SIZE, GetRight(watchWnd),
									GetBottom(watchWnd)+1);
	}
}

void DeleteAllWatches(void)
{
	if (WatchCount)	{
		Assert(watchWnd != NULL);
		SendMessage(watchWnd, CLEARTEXT, 0, 0);
		if (wHeight > WATCHHT)	{
			wHeight = WATCHHT;
			SendMessage(watchWnd, SIZE, GetRight(watchWnd),
										GetTop(watchWnd)+wHeight+1);
			SendMessage(watchWnd, MOVE, GetLeft(watchWnd),
								GetClientBottom(applWnd)-(wHeight+1));
			SetEditorSize();
		}
		if (isVisible(watchWnd))
			SendMessage(watchWnd, PAINT, 0, 0);
		while (WatchCount)
			free(Watches[--WatchCount]);
	}
	SendMessage(editWnd, SETFOCUS, TRUE, 0);
}

static void GetWatchValue(char *value, char *VariableID)
{
	savegl();
	Watching = TRUE;
	if (setjmp(Watchjmp) == 0)
		GetVariableValue(value, VariableID);
	else	{
		ErrorCode = 0;
		sprintf(value, "%s ??", VariableID);
	}
	Watching = FALSE;
	restgl();
}

void UpdateWatches(void)
{
	if (isVisible(watchWnd))	{
		char value[SIZEWATCH+20];
		char VariableID[32];
		int i;

		ClearVisible(watchWnd);
		SendMessage(watchWnd, CLEARTEXT, 0, 0);
		for (i = 0; i < WatchCount; i++)	{
			strcpy(VariableID, Watches[i]);
			GetWatchValue(value, VariableID);
			SendMessage(watchWnd, ADDTEXT, (PARAM) value, 0);
		}
		SetVisible(watchWnd);
		SendMessage(watchWnd, PAINT, 0, 0);
	}
}

/* ----------------- Examine functions ---------------------- */
void ExamineVariable(void)
{
	DialogBox(applWnd, &ExamineDB, TRUE, ExamineProc);
}

static void BuildExamination(WINDOW wnd)
{
	char value[SIZEWATCH+20];
	char VariableID[31];
	GetItemText(wnd, ID_EXAMINE, VariableID, 30);
	GetVariableValue(value, VariableID);
	PutItemText(wnd, ID_VALUE, value);
}

/* --- if the variable being examined is not an lvalue, don't let
  the change editbox get the focus --- */
static int ChangeProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
	if (msg == SETFOCUS && p1 && !Assignable)
		return FALSE;
	return (*changeproc)(wnd, msg, p1, p2);
}

static void ChangeVariable(char *vr, char *val)
{
	char Srcbuf[130];
	char Tknbuf[130];
	char *cp;

	if ((cp = strchr(vr, '\n')) != NULL)
		*cp = '\0';
	if ((cp = strchr(val, '\n')) != NULL)
		*cp = '\0';

	sprintf(Srcbuf, "%s=(%s);", vr, val);
	tokenize(Tknbuf, Srcbuf);
	Ctx.Progptr = Tknbuf;
	getoken();
	expression();
	pop();
}

static int ExamineProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
	static char previd[31];
	CTLWINDOW *ct;
	WINDOW cwnd;
	switch (msg)	{
		case CREATE_WINDOW:
			savegl();
			DefaultWndProc(wnd, msg, p1, p2);

			ct = FindCommand(&ExamineDB, ID_EXAMINE, EDITBOX);
			Assert(ct != NULL);
			cwnd = ct->wnd;
			SendMessage(cwnd, SETTEXTLENGTH, 30, 0);
			SendMessage(cwnd, CLEARTEXT, 0, 0);

			ct = FindCommand(&ExamineDB, ID_CHANGE, EDITBOX);
			Assert(ct != NULL);
			cwnd = ct->wnd;
			SendMessage(cwnd, SETTEXTLENGTH, 30, 0);
			SendMessage(cwnd, CLEARTEXT, 0, 0);
			changeproc = cwnd->wndproc;
			cwnd->wndproc = &ChangeProc;

			Watching = TRUE;
			*previd = '\0';
			return TRUE;
		case COMMAND:
			if ((int) p1 == ID_OK && (int) p2 == 0)	{
				char *ex, *ch;

				ct = FindCommand(&ExamineDB, ID_EXAMINE, EDITBOX);
				Assert(ct != NULL);
				ex = GetText((WINDOW)(ct->wnd));

				ct = FindCommand(&ExamineDB, ID_CHANGE, EDITBOX);
				Assert(ct != NULL);
				ch = GetText((WINDOW)(ct->wnd));

				if (ex == NULL || *ex == '\0')
					/* --- nothing to examine --- */
					break;
				if (strcmp(ex, previd) == 0)	{
					/* --- no change to the identifier --- */
					if (ch && *ch)	{
						/* --- user entered change value --- */
						if (setjmp(Watchjmp) == 0)
							ChangeVariable(ex, ch);
						else	{
							DisplayError();
							return TRUE;
						}
					}
				}
				else	{
					/* --- new identifier, get it's value --- */
					strcpy(previd, ex);
					if (setjmp(Watchjmp) == 0)	{
						BuildExamination(wnd);
						if (Assignable)
							SendMessage(ct->wnd, SETFOCUS, TRUE, 0);
					}
					else 
						DisplayError();
					return TRUE;
				}
			}
			break;
		case CLOSE_WINDOW:
			Watching = FALSE;
			restgl();
			UpdateWatches();
			break;
		default:
			break;
	}
    return DefaultWndProc(wnd, msg, p1, p2);
}

static void AppendDisplayString(char *val, char *s)
{
	strcat(val, s);
	dispsize += strlen(s);
	if (dispsize >= SIZEWATCH)
		longjmp(Displayjmp, 1);
}

static void DisplayItemValue(char *value, ITEM item)
{
	char val[20];
	if (ItemisAddress(item))
		sprintf(val, "%p", item.value.cptr);
	else	{
		torvalue(&item);
		switch (item.type)	{
			case CHAR:
				if (ItemisAddressOrPointer(item))
					item.value.cval = *item.value.cptr;
				if (' ' <= item.value.cval && item.value.cval <= '~')
					sprintf(val, "'%c'", item.value.cval);
				else
					sprintf(val, item.isunsigned ?
										"%u (0x%02x)" :
										"%d (0x%02x)",
									item.value.cval, item.value.cval);
				break;
			case INT:
				if (ItemisAddressOrPointer(item))
					item.value.ival = *item.value.iptr;
				sprintf(val, item.isunsigned ? "%u" : "%d", item.value.ival);
				break;
			case LONG:
				if (ItemisAddressOrPointer(item))
					item.value.lval = *item.value.lptr;
				sprintf(val, item.isunsigned ? "%lu" : "%ld", item.value.lval);
				break;
			case FLOAT:
				if (ItemisAddressOrPointer(item))
					item.value.fval = *item.value.fptr;
				sprintf(val, "%g", item.value.fval);
				break;
			default:
				return;
		}
	}
	AppendDisplayString(value, val);
}

static char *tocesc(char c, char *buf)
{
	/*
	 * Convert the character "c" to its appropriate character
	 * escape sequence.
	 */
	if (isprint(c & 255) || c == ' ' && c != '"' && c != '\\')
		*buf++ = c;
	else	{
		*buf++ ='\\';
		switch ( c )	{
			case '\b':
				*buf++ ='b';
				break;
			case '\n':
				*buf++ ='n';
				break;
			case '\t':
				*buf++ ='t';
				break;
			case '\f':
				*buf++ ='f';
				break;
			case '\r':
				*buf++ ='r';
				break;
			case '\'':
				*buf++ = '\'';
				break;
			case '"':
				*buf++ = '"';
				break;
			default:
				sprintf( buf, "x%02x", c );
				while ( *buf )
					++buf;
		}
	}
	return buf;
}

static ITEM ParseVariable(char *VariableID)
{
	ITEM item;
	char Tknbuf[30];

	ccat(VariableID, ';');
	tokenize(Tknbuf, VariableID);
	VariableID[strlen(VariableID)-1] = '\0';
	Ctx.Progptr = Tknbuf;
	getoken();
	primary();
	item = *Ctx.Stackptr;
	pop();
	return item;
}

static void DisplayArrayElements(char *value, char *sname, VARIABLE *var)
{
	ITEM item;
	char arr[100];
	int elems = ArrayElements(var);
	int sub, dim;
	int dims = ArrayDimensions(var);
	char *cp;

	strcpy(arr, sname);
	for (dim = 0; dim < dims-1; dim++)
		strcat(arr, "[0]");
	cp = arr+strlen(arr);
	for (sub = 0; sub < elems; sub++)	{
		sprintf(cp, "[%d]", sub);
		item = ParseVariable(arr);
		DisplayItemValue(value+strlen(value), item);
		if (sub < elems-1)
			AppendDisplayString(value, ",");
	}
}

static void GetVariableValue(char *value, char *VariableID)
{
	static char *types[] = {
		"void","char","int","long","float","struct","union"
	};
	int reference = strpbrk(VariableID, "*[+-") != NULL;

	dispsize = 0;
	if (setjmp(Displayjmp) == 0)	{
		ITEM item;
		VARIABLE *var;
		char *sname;

		*value = '\0';
		item = ParseVariable(VariableID);
		var = Ctx.Curvar;
		sname = FindSymbolName(var->vsymbolid);
		Assignable = item.lvalue;
		if (item.type < ENUM+1)	{
			if (item.isunsigned)
				AppendDisplayString(value, "unsigned ");
			AppendDisplayString(value, types[item.type]);
		}
		AppendDisplayString(value, " ");
		if (var->vtype == UNION || var->vtype == STRUCT)	{
			char *sn = FindSymbolName(var->vstruct->vsymbolid);
			if (sn != NULL)	{
				AppendDisplayString(value, sn);
				AppendDisplayString(value, " ");
			}
		}
		if (isPointer(var))	{
			int cl = var->vclass;
			if (var->vkind & FUNCT)
				AppendDisplayString(value, "(");
			if (!reference)
				while (cl--)
					AppendDisplayString(value, "*");
		}
		AppendDisplayString(value, VariableID);

		if (isArray(var) && !item.lvalue && !reference)	{
			char arr[20];
			int i;
			for (i = 0; var->vdims[i] && i < MAXDIM; i++)	{
				sprintf(arr, "[%d]", var->vdims[i]);
				AppendDisplayString(value, arr);
			}
		}

		if (var->vkind & FUNCT)	{
			if (isPointer(var))
				AppendDisplayString(value, ")");
			AppendDisplayString(value, "()");
			return;
		}
		/* --- display nothing more about structs/unions --- */
		if (var->velem.vfirst != NULL)
			return;

		if (ItemisPointer(item))	{
			if (*item.value.pptr == NULL)	{
				AppendDisplayString(value, "=NULL");
				return;
			}
			AppendDisplayString(value, "->");
		}
		else
			AppendDisplayString(value, "=");

		if (item.type == CHAR && item.class == 1)	{
			/* ----- char string ----- */
			int slen, dim;
			char *cp1, *cp2;
			torvalue(&item);
			slen = SIZEWATCH-strlen(value)-2;
			dim = item.dims[0] ? item.dims[0] : slen;
			cp1 = value+strlen(value);
			cp2 = item.value.cptr;
			*cp1++ = '"';
			while (*cp2 && cp1 < value+slen && dim--)
				cp1 = tocesc(*cp2++, cp1);
			*cp1++ = '"';
			*cp1 = '\0';
			return;
		}
		if (!reference && isArray(var) && ItemisAddressOrPointer(item))
			DisplayArrayElements(value, sname, var);
		else
			DisplayItemValue(value+strlen(value), item);
	}
}

static void savegl(void)
{
	/*
	 * Save globals
	 */
	Svcurvar = Ctx.Curvar;
	Svprogptr =  Ctx.Progptr;
	Svtoken = Ctx.Token;
	Svvalue = Ctx.Value;
	Svstackptr= Ctx.Stackptr;
	SvBpCtr = BreakpointCount;
	BreakpointCount = 0;
	SvStepping = Stepping;
	Stepping = FALSE;
}

static void restgl(void)
{
	/*
	 * Restore globals
	 */
	Ctx.Curvar = Svcurvar;
	Ctx.Progptr = Svprogptr;
	Ctx.Stackptr = Svstackptr;
	Ctx.Token = Svtoken;
	Ctx.Value = Svvalue;
	BreakpointCount = SvBpCtr;
	Stepping = SvStepping;
}

static void ccat(char *s, int c)
{
	s += strlen(s);
	*s++ = c;
	*s = '\0';
}


