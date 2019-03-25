/* ---------- debugger.c ---------- */

#include "dflat.h"
#include "qnc.h"
#include "debugger.h"
#include "interp.h"

static BOOL StdoutOpened;
static BOOL inProgram;
static BOOL inIDE;
static BOOL Changed;
static BOOL Stopping;
static BOOL FuncStack;
static char CmdLine[61];
static char Commands[61];
static int argc;
static char *args[MAXARGS+1];
static int StuffKey;

BOOL Running;
BOOL Stepping;
int LastStep;
int SteppingOver;
int ErrorCode;
int currx, curry;
char ErrorMsg[160];

static char line[135];

static void CopyRedirectedFilename(char *, char **);
static int StartProgram(void);
static BOOL LineNoOnCurrentPage(int);
static void TurnPageToLineNo(int);
static void TestFocus(void);
static void BuildDisplayLine(int);
static BOOL SourceChanged(int);
static BOOL TestRestart(void);
static void TerminateMessage(int);
static void RunIDE(void);

int DupStdin = -1;
int DupStdout = -1;
int oldstdin;
int oldstdout;
char DupFileIn[65];
char DupFileOut[65];

/* --- run the program from Run command --- */
void RunProgram(void)
{
    int rtn = -1;
    TestFocus();
    if (SourceChanged(F9))
        return;
    Stopping = FALSE;
    if (Stepping)    {
        /* --- executed Run after Step or Breakpoint --- */
        OpenStdout();
        inIDE = FALSE;
        Stepping = FALSE;
        return;
    }
    /* --- Running program from start --- */
    StdoutOpened = FALSE;
//    HideIDE();
    Changed = editWnd->TextChanged;
    editWnd->TextChanged = FALSE;
    inProgram = FALSE;

    rtn = StartProgram();    /* run the program */

    /* --- returned from running the program --- */
    editWnd->TextChanged |= Changed;

    if (Stepping)    {
        /* --- Stepping would be Run/Bkpt/Step, returning here
            when program exits --- */
        Stepping = FALSE;
        SteppingOver = FALSE;
        Stopping = FALSE;
        LastStep = 0;
        SendMessage(editWnd, PAINT, 0, 0); /* clear IP cursor */
    }
    else    {
        if (ErrorCode==0 && !Exiting && !Stopping && !StuffKey)
            PromptIDE();
        UnHideIDE();
    }
    if (ErrorCode)
        DisplayError();
    else if (!TestRestart())
        TerminateMessage(rtn);
}
/* --- step over the current statement --- */
void StepOverFunction(void)
{
    SteppingOver = 1;
    StepProgram();
}
/* --- execute one statement (Step) --- */
void StepProgram(void)
{
    TestFocus();
    if (SourceChanged(F7))
        return;
    if (!Stepping)    {
        /* ---- the first step ---- */
        int rtn = -1;
        Stepping = TRUE;
        Stopping = FALSE;
        StdoutOpened = FALSE;
        LastStep = 0;
        inProgram = FALSE;
        Changed = editWnd->TextChanged;
        editWnd->TextChanged = FALSE;
        rtn = StartProgram();     /* take the step */
        editWnd->TextChanged |= Changed;
        Stepping = FALSE;
        LastStep = 0;
        if (!isVisible(applWnd))    {
            UnHideIDE();        /* display the IDE */
            SteppingOver = FALSE;
        }
        if (ErrorCode)
            DisplayError();
        SendMessage(editWnd, PAINT, 0, 0);
        Stopping = FALSE;
        if (!TestRestart())
            TerminateMessage(rtn);
    }
    else
        inIDE = FALSE;
}
/* --- process Stop command --- */
void StopProgram(void)
{
    Stopping = TRUE;
    Stepping = FALSE;
    inProgram = FALSE;
    inIDE = FALSE;
    if (LastStep && LineNoOnCurrentPage(LastStep))    {
        WriteTextLine(editWnd, NULL, LastStep-1, FALSE);
        LastStep = 0;
    }
    TestFocus();
}
/* -- start running the program from Run or Step command -- */
static int StartProgram(void)
{
    int rtn = -1;
    char *cp = CmdLine;
    char *cm = Commands;
    argc = 0;
    args[argc++] = editWnd->extension;
    while (*cp && argc < MAXARGS)    {
        while (isspace(*cp))
            cp++;
        if (*cp == '>')    {
            CopyRedirectedFilename(DupFileOut, &cp);
            continue;
        }
        else if (*cp == '<')    {
            CopyRedirectedFilename(DupFileIn, &cp);
            continue;
        }
        args[argc++] = cm;
        while (*cp && !isspace(*cp))
            *cm++ = *cp++;
        *cm++ = '\0';
    }
    CtrlBreaking = FALSE;
    unBreak();
    if (ErrorCode == 0)    {
        Running = TRUE;
        rtn = qinterpmain(editWnd->text, argc, args);
        Running = FALSE;
    }
    reBreak();
    *DupFileIn = *DupFileOut = '\0';
    return rtn;
}
/* --- entry to debugger from interpreter --- */
BOOL debugger(void)
{
    if (SteppingOver)    {
        SteppingOver = 0;
        Stepping = TRUE;
        curr_cursor(&currx, &curry);
        UnHideIDE();
    }
    if (inProgram == FALSE)    {
        /* -- don't debug the first statement (main call) -- */
        inProgram = TRUE;
        return FALSE;
    }
    if ((CtrlBreaking || Stepping || BreakpointCount) &&
                                    Ctx.CurrFileno == 0)    {
        int lno = Ctx.CurrLineno;
        if (Stepping)    {
            if (lno != LastStep)    {
                if (LastStep)    {
                    int ln = LastStep;
                    LastStep = 0;
                    DisplaySourceLine(ln);
                }
                LastStep = lno;
                DisplaySourceLine(lno);
                SendMessage(editWnd, KEYBOARD_CURSOR, 0,
                                    lno-editWnd->wtop-1);
                /* --- let D-Flat run the IDE --- */
                reBreak();
                RunIDE();
                unBreak();
            }
        }
        else if (CtrlBreaking || isBreakpoint(lno))    {
            LastStep = lno;
            Stepping = TRUE;
            UnHideIDE();
            TurnPageToLineNo(lno);
            SendMessage(editWnd, KEYBOARD_CURSOR, 0,
                                    lno-1-editWnd->wtop);
            MessageBox(DFlatApplication,
              CtrlBreaking ? errs[CTRLBREAK-1] : "Breakpoint");
            CtrlBreaking = FALSE;
            /* --- let D-Flat run the IDE --- */
            reBreak();
            RunIDE();
            unBreak();
        }
    }
    return Stopping;
}
/* --- copy a redirected filename from command line --- */
static void CopyRedirectedFilename(char *fname, char **cp)
{
    (*cp)++;
    while (isspace(**cp))
        (*cp)++;
    while (**cp && !isspace(**cp))
        *fname++ = *((*cp)++);
    *fname = '\0';
}
/* -- display source code line with bkpt/cursor highlights - */
void DisplaySourceLine(int lno)
{
    TurnPageToLineNo(lno);
    BuildDisplayLine(lno);
    if (isBreakpoint(lno))    {
        if (Stepping && lno == LastStep)
            SetBreakpointCursorColor(editWnd);
        else
            SetBreakpointColor(editWnd);
    }
    else if (Stepping && lno == LastStep)
        SetReverseColor(editWnd);
    else
        SetStandardColor(editWnd);
    PutWindowLine(editWnd, line, 0, lno-1-editWnd->wtop);
}
/* --- build a display line of code ---- */
static void BuildDisplayLine(int lno)
{
    char *tx = TextLine(editWnd, lno-1);
    int wd = min(134, ClientWidth(editWnd));
    char *ln = line;
    char *cp1 = ln;
    char *cp2 = tx+editWnd->wleft;
    while (*cp2 != '\n' && cp1 < ln+wd)
        *cp1++ = *cp2++;
    while (cp1 < ln+wd)
        *cp1++ = ' ';
    *cp1 = '\0';
}
/* ---- prompt user on output screen ---- */
void PromptIDE(void)
{
    printf("\nPress any key to return to Quincy...");
    getkey();
    printf("\r                                    ");
}
/* --- test Run/Step command for source changed --- */
static BOOL SourceChanged(int key)
{
    if (Stepping && editWnd->TextChanged)    {
        MessageBox(DFlatApplication,
            "Source changed, must restart");
        StopProgram();
        StuffKey = key;
        return TRUE;
    }
    return FALSE;
}
/* --- test for restarting with Run or Step command --- */
static BOOL TestRestart(void)
{
    if (StuffKey)    {
        SendMessage(editWnd, PAINT, 0, 0);
        PostMessage(editWnd, KEYBOARD, StuffKey, 0);
        StuffKey = 0;
        return TRUE;
    }
    return FALSE;
}
/* --- run the IDE --- */
static void RunIDE(void)
{
    inIDE = TRUE;
    UpdateWatches();
    while (inIDE && dispatch_message())
        ;
}
/* ------------- set command line parameters ------------- */
void CommandLine(void)
{
    InputBox(applWnd, DFlatApplication, "Command line:",
            CmdLine, 128, 25);
}
/* ---- display program terminated message ---- */
static void TerminateMessage(int rtn)
{
    char msg[50];
    sprintf(msg, "Program terminated: code %d", rtn);
    MessageBox(DFlatApplication, msg);
}
/* --- test if source line is in view ---- */
static BOOL LineNoOnCurrentPage(int lno)
{
    return (lno-1 >= editWnd->wtop &&
            lno-1 < editWnd->wtop+ClientHeight(editWnd));
}
/* ---- display page with specified source code line --- */
static void TurnPageToLineNo(int lno)
{
    if (!LineNoOnCurrentPage(lno))    {
        if ((editWnd->wtop = lno-ClientHeight(editWnd)/2) < 0)
            editWnd->wtop = 0;
        SendMessage(editWnd, PAINT, 0, 0);
    }
}
/* --- make sure that edit window has the focus --- */
static void TestFocus(void)
{
    if (editWnd != inFocus)
        SendMessage(editWnd, SETFOCUS, TRUE, 0);
}
/* --- display an error message when program terminates --- */
void DisplayError(void)
{
    if (!Watching)    {
        if (Ctx.CurrFileno == 0)    {
            TestFocus();
            TurnPageToLineNo(Ctx.CurrLineno);
            SendMessage(editWnd, KEYBOARD_CURSOR,
                    0, Ctx.CurrLineno-1-editWnd->wtop);
        }
    }
    strcat(ErrorMsg, errs[ErrorCode-1]);
    beep();
    ErrorMessage(ErrorMsg);
    ErrorCode = 0;
}
/* --- hide IDE so program can use output screen --- */
void HideIDE(void)
{
    if (isVisible(applWnd))    {
        SendMessage(applWnd, HIDE_WINDOW, 0, 0);
        SendMessage(NULL, HIDE_MOUSE, 0, 0);
        cursor(currx, curry);
    }
}
/* -- display IDE when program is done with output screen -- */
void UnHideIDE(void)
{
    if (!isVisible(applWnd))    {
        curr_cursor(&currx, &curry);
        SendMessage(applWnd, SHOW_WINDOW, 0, 0);
        SendMessage(NULL, SHOW_MOUSE, 0, 0);
    }
}
/* --- interpreter needs output screen for stdout --- */
void OpenStdout(void)
{
    if (Stepping)
        HideIDE();
    if (!StdoutOpened)    {
        StdoutOpened = TRUE;
        putchar('\n');
    }
}
/* --- interpreter is done with output screen for stdout --- */
void CloseStdout(void)
{
    if (Stepping)
        if (!SteppingOver)
            UnHideIDE();
}
/* ---- window proc module for Function Stack DB ------- */
static int FunctionStackProc(WINDOW wnd, MESSAGE msg,
                                        PARAM p1, PARAM p2)
{
    FUNCRUNNING *fr = Ctx.Curfunc;
    FUNCTION *fun = FunctionMemory;
    int sel;
    CTLWINDOW *ct =
        FindCommand(wnd->extension,ID_FUNCSTACK,LISTBOX);
    WINDOW lwnd = ct ? ct->wnd : NULL;

    switch (msg)    {
        case INITIATE_DIALOG:
            Assert(lwnd != NULL);
            SendMessage(lwnd, CLEARTEXT, 0, 0);
            if (FuncStack)    {
                while (fr != NULL)    {
                    char *fn=FindSymbolName(fr->fvar->symbol);
                    if (fn)
                        SendMessage(lwnd,ADDTEXT,(PARAM)fn,0);
                    fr = fr->fprev;
                }
            }
            else    {
                while (fun < NextFunction)    {
                    if (fun->libcode == 0)    {
                        char *fn = FindSymbolName(fun->symbol);
                        if (fn)
                          SendMessage(lwnd,ADDTEXT,(PARAM)fn,0);
                    }
                    fun++;
                }
            }
            SendMessage(lwnd, PAINT, 0, 0);
            break;
        case COMMAND:
            Assert(lwnd != NULL);
            switch ((int) p1)    {
                case ID_FUNCSTACK:
                    if ((int) p2 == LB_CHOOSE)
                        SendMessage(wnd, COMMAND, ID_OK, 0);
                    break;
                case ID_OK:
                    if (p2)
                        break;
                    sel = SendMessage(lwnd,
                            LB_CURRENTSELECTION, 0, 0);
                    if (FuncStack)    {
                        while (fr != NULL && sel--)
                            fr = fr->fprev;
                        fun = fr->fvar;
                    }
                    else
                        while (fun < NextFunction)    {
                            if (fun->libcode == 0)
                                if (sel-- == 0)
                                    break;
                            fun++;
                        }
                    if (fun->fileno == 0)    {
                        TurnPageToLineNo(fun->lineno);
                        SendMessage(editWnd, KEYBOARD_CURSOR,0,
                            fun->lineno-1-editWnd->wtop);
                    }
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}
/* ---- display the function stack ------- */
void FunctionStack(void)
{
    FuncStack = TRUE;
    FunctionStackDB.dwnd.title = "Function History";
    DialogBox(applWnd, &FunctionStackDB,
        TRUE, FunctionStackProc);
}
/* ---- display list of functions ------- */
void FunctionList(void)
{
    FuncStack = FALSE;
    FunctionStackDB.dwnd.title = "Function List";
    DialogBox(applWnd, &FunctionStackDB,
        TRUE, FunctionStackProc);
}

