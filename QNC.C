/* --------------- qnc.c ----------- */
#include <process.h>
#include <alloc.h>
#include <io.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include "dflat.h"
#include "qnc.h"
#include "interp.h"
#include "debugger.h"

char DFlatApplication[9] = QUINCY;
static char Untitled[] = "Untitled";
char **Argv;

#ifdef TUTORIAL
char TutorialHelpFile[] = "tutorial";
#define tuthelp TutorialHelpFile /* -- coincidence -- */
static BOOL TutLoaded;
#endif

WINDOW editWnd;
WINDOW applWnd;
WINDOW watchWnd;
BOOL Exiting;
BOOL CtrlBreaking;

struct QuincyConfig qCfg = {
	MAXPR,		  	/* user program space  */
	MAXSTACK,	   	/* stack size          */
	MAXVARIABLES,   /* number of variables */
	MAXFUNCTIONS,   /* number of functions */
	DATASPACE,      /* data bytes          */
	MAXSYMBOLTABLE, /* symbol table space  */
	MAXFUNCTIONS * AVGPROTOTYPES,
	TRUE,			/* scrollbar toggle	   */
	0				/* tutorial mode	   */
};

static char *qhdr = QUINCY " " QVERSION;
extern unsigned _stklen = 16*1024;
static void interrupt (*oldbreak)(void);
static void interrupt (*oldctrlc)(void);
static BOOL brkct;
static int QuincyProc(WINDOW, MESSAGE, PARAM, PARAM);
static void SelectFile(void);
static void OpenSourceCode(char *);
static void LoadFile(void);
static void SaveFile(int);
void ToggleBreakpoint(void);
void DeleteBreakpoints(void);
static int QncEditorProc(WINDOW, MESSAGE, PARAM, PARAM);
static char *NameComponent(char *);
static void ChangeTabs(void);
static void FixTabMenu(void);
static void CloseSourceCode(void);
static void OutputScreen(void);
static void Memory(void);
static void ChangeQncTitle(void);

void main(int argc, char *argv[])
{
    int hasscr = 0;
    if (!init_messages())
        return;
    curr_cursor(&currx, &curry);
    setcbrk(0);
    reBreak();
	_control87(MCW_EM,MCW_EM); /* mask off fp aborts */

    Argv = argv;
    if (!LoadConfig())
        cfg.ScreenLines = SCREENHEIGHT;

    applWnd = CreateWindow(APPLICATION,
                        qhdr,
                        0, 0, -1, -1,
                        &MainMenu,
                        NULL,
                        QuincyProc,
                        HASSTATUSBAR
                        );
    ClearAttribute(applWnd, CONTROLBOX);
#ifdef TUTORIAL
	if (!qCfg.inTutorial)	{
		TutLoaded = FALSE;
	    LoadHelpFile(DFlatApplication);
	}
#else
    LoadHelpFile(DFlatApplication);
#endif

    if (SendMessage(NULL, MOUSE_INSTALLED, 0, 0) && qCfg.scrollbars)
        hasscr = (VSCROLLBAR | HSCROLLBAR);

    editWnd = CreateWindow(EDITOR,
                NULL,
                GetClientLeft(applWnd), GetClientTop(applWnd),
                ClientHeight(applWnd), ClientWidth(applWnd),
                NULL,
                NULL,
                QncEditorProc,
                hasscr | HASBORDER | MULTILINE);
    ChangeQncTitle();
    SendMessage(editWnd, SETFOCUS, TRUE, 0);
    if (argc > 1)
        OpenSourceCode(argv[1]);
    while (dispatch_message())
        ;
    unBreak();
    cursor(currx, curry);
	SaveConfig();
}
/* -- interception and management of Ctrl+C and Ctrl+Break -- */
static void interrupt newbreak(void)
{
    return;
}
/* --- math exceptions: this suppresses NAN messages --- */
int cdecl matherr(struct exception *e)
{
	error(MATHERR);
	return 1;
}
int CBreak(void)
{
    if (!Stepping)    {
        CtrlBreaking = TRUE;
        if (inSystem)
            longjmp(BreakJmp, 1);
    }
    return 1;
}
void unBreak(void)
{
    if (brkct)    {
        setvect(0x1b, oldbreak);
        setvect(0x23, oldctrlc);
        ctrlbrk(CBreak);
        brkct = FALSE;
    }
	if (*DupFileOut)	{
		DupStdout = open(DupFileOut, O_CREAT | O_WRONLY | O_TEXT,
											S_IREAD | S_IWRITE);
		if (DupStdout != -1)	{
			oldstdout = dup(1);
			dup2(DupStdout, 1);
			close(DupStdout);
		}
		else
			error(STDOUTFILERR);
	}
	if (*DupFileIn)	{
		DupStdin = open(DupFileIn, O_RDONLY | O_TEXT, S_IREAD | S_IWRITE);
		if (DupStdin != -1)	{
			oldstdin = dup(0);
			dup2(DupStdin, 0);
			close(DupStdin);
		}
		else
			error(STDINFILERR);
	}
}
void reBreak(void)
{
    if (!brkct)    {
        oldctrlc = getvect(0x23);
        oldbreak = getvect(0x1b);
        setvect(0x23, newbreak);
        setvect(0x1b, newbreak);
        brkct = TRUE;
    }
	if (DupStdout != -1)	{
		dup2(oldstdout, 1);
		close(oldstdout);
	}
	if (DupStdin != -1)	{
		dup2(oldstdin, 0);
		close(oldstdin);
	}
	DupStdout = DupStdin = -1;
}
/* --- change application window title to show filename --- */
static void ChangeQncTitle(void)
{
    char *ttl;
    char *cp = Untitled;
    char *cp1 = editWnd->extension;
    int len = 13;

    if (cp1 && *cp1)    {
        cp = strrchr(cp1, '\\');
        if (cp == NULL)
            cp = strchr(cp1, ':');
        if (cp == NULL)
            cp = cp1;
        else
            cp++;
        len = strlen(cp) + 3;
    }
    ttl = DFmalloc(strlen(qhdr) + len);
    strcpy(ttl, qhdr);
    strcat(ttl, ": ");
    strcat(ttl, cp);
    AddTitle(applWnd, ttl);
    free(ttl);
}
#ifdef TUTORIAL
static void ResetHelpFile(void)
{
	if (TutLoaded)	{
		LoadHelpFile(DFlatApplication);
		TutLoaded = FALSE;
	}
}
static void DisplayThisTutorial(void)
{
	SendMessage(applWnd, DISPLAY_HELP,
		(PARAM) (*qCfg.tutorhelp ? qCfg.tutorhelp: tuthelp), 0);
}
static int isTutorial(char *fn)
{
	int i = 0;
	if (strncmp(fn, "ex", 2) == 0 && strncmp(fn+7, ".c", 2) == 0)
		for (i = 0; i < 5; i++)
			if (!isdigit(*(fn+2+i)))
				break;
	return i == 5;
}
static void DisplayNewTutorial(char *msk, int offset)
{
	if (isTutorial(qCfg.tutorhelp))	{
		char ttr[11];
		int chap, ex, chapex = atoi(qCfg.tutorhelp+2);
		if (offset == 0)
			chapex /= 1000;	/* convert chapter/exercise to chapter */
		for (;;)	{
			chapex += offset;
			sprintf(ttr, msk, chapex);
			if (SendMessage(applWnd, DISPLAY_HELP, (PARAM) ttr, 0) != 0
					|| offset == 0)
				break;
			chap = chapex / 1000;
			ex = chapex % 1000;
			if (ex == 0 && offset == -1)	{
				if (chap == 1)
					break;
				chapex = --chap * 1000 + MAXEXERCISE;
				continue;
			}
			if (ex > 1 && offset == 1)	{
				if (chap == MAXCHAP)
					break;
				chapex = ++chap * 1000;
				continue;
			}
		}
	}
}
#endif
/* --- window processing module for Quincy application --- */
static int QuincyProc(WINDOW wnd,MESSAGE msg,PARAM p1,PARAM p2)
{
    int rtn;
    static BOOL InterceptingShow = FALSE;
    static BOOL PrevSbarSetting;
    static int PrevScreenLines;
    switch (msg)    {
        case CREATE_WINDOW:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            if (SendMessage(NULL, MOUSE_INSTALLED, 0, 0) && qCfg.scrollbars)
                SetCheckBox(&Display, ID_SCROLLBARS);
            else
                ClearCheckBox(&Display, ID_SCROLLBARS);
#ifdef TUTORIAL
			if (qCfg.inTutorial && _argc < 2)	{
                SetCommandToggle(&MainMenu, ID_TUTORIAL);
				LoadHelpFile(TutorialHelpFile);
				TutLoaded = TRUE;
				PostMessage(wnd, DISPLAY_HELP, (PARAM) qCfg.tutorhelp, 0);
			}
			else 
                ClearCommandToggle(&MainMenu, ID_TUTORIAL);
#endif
            FixTabMenu();
            return rtn;
        case SIZE:
        {
            int dif = (int) p2 - GetBottom(wnd);
            int EditBottom = p2 - BottomBorderAdj(wnd);
            if (watchWnd != NULL &&
                    TestAttribute(watchWnd, VISIBLE))
                EditBottom -= WindowHeight(watchWnd);
            if (dif > 0)    {
                /* --- getting bigger--- */
                rtn = DefaultWndProc(wnd, msg, p1, p2);
                SendMessage(watchWnd, MOVE, GetLeft(watchWnd),
                                        GetTop(watchWnd)+dif);
                SendMessage(editWnd, SIZE,
                        GetClientRight(wnd), EditBottom);
            }
            else    {
                /* --- getting smaller --- */
                SendMessage(editWnd,
                    SIZE, GetClientRight(wnd), EditBottom);
                SendMessage(watchWnd, MOVE, GetLeft(watchWnd),
                                        GetTop(watchWnd)+dif);
                rtn = DefaultWndProc(wnd, msg, p1, p2);
            }
            return rtn;
        }
        case SHOW_WINDOW:
            qCfg.scrollbars =
                CheckBoxSetting(&Display, ID_SCROLLBARS);
            if (InterceptingShow)    {
                if (qCfg.scrollbars != PrevSbarSetting)    {
                    if (qCfg.scrollbars)
                        AddAttribute(editWnd,
                            VSCROLLBAR | HSCROLLBAR);
                    else     {
                        ClearAttribute(editWnd, VSCROLLBAR);
                        ClearAttribute(editWnd, HSCROLLBAR);
                    }
                }
                if (PrevScreenLines != cfg.ScreenLines)
                    SendMessage(wnd, SIZE,
                            GetRight(wnd), cfg.ScreenLines-1);
            }
            break;
        case CLOSE_WINDOW:
            SendMessage(editWnd, CLOSE_WINDOW, 0, 0);
            break;
        case SETFOCUS:
            if (p1 && editWnd)    {
                SendMessage(editWnd, msg, p1, p2);
                return TRUE;
            }
            break;
#ifdef TUTORIAL
		case DISPLAY_HELP:
			if (qCfg.inTutorial)	{
				if (strcmp(".c", ((char*)p1)+strlen((char*)p1)-2) == 0)	{
					char *cmt;
					char VariableID[30];
					char *vp = NULL;
					if (access((char *)p1, 0) != 0)
						return FALSE;
					strcpy(qCfg.tutorhelp, (char*)p1);
					if (strcmp(qCfg.tutorhelp, (char*)editWnd->extension))	{
						OpenSourceCode(qCfg.tutorhelp);
						DeleteAllWatches();
						cmt = HelpComment((char*)p1);
						while (cmt && *cmt)	{
							while (isspace(*cmt))
								cmt++;
							vp = VariableID;
							while (*cmt && *cmt != ',' && *cmt != '\n')
								*vp++ = *cmt++;
							*vp = '\0';
							AddWatch(VariableID);
							cmt++;
						}
						if (vp)
							ShowWatches();
					}
				}
			}
			break;
#endif
        case COMMAND:
            switch ((int)p1)    {
                case ID_NEW:
                    OpenSourceCode(Untitled);
                    return TRUE;
                case ID_OPEN:
                    SelectFile();
                    return TRUE;
                case ID_SAVE:
                    SaveFile(FALSE);
                    return TRUE;
                case ID_SAVEAS:
                    SaveFile(TRUE);
                    return TRUE;
                case ID_PRINTSETUP:
                    DialogBox(wnd, &PrintSetup,
                        TRUE, PrintSetupProc);
                    return TRUE;
                case ID_PRINT:
                    PrintSourceCode();
                    return TRUE;
                case ID_OUTPUT:
                    OutputScreen();
                    return TRUE;
                case ID_RUN:
                    RunProgram();
                    if (Exiting)
                        PostMessage(wnd, CLOSE_WINDOW, 0, 0);
                    return TRUE;
                case ID_STOP:
                    StopProgram();
                    return TRUE;
                case ID_EXAMINE:
                    ExamineVariable();
                    return TRUE;
                case ID_WATCH:
                    ToggleWatch();
                    return TRUE;
				case ID_FUNCTIONSTACK:
					FunctionStack();
					return TRUE;
				case ID_FUNCTIONLIST:
					FunctionList();
					return TRUE;
                case ID_ADDWATCH:
                    GetWatch();
                    return TRUE;
                case ID_SWITCHWINDOW:
                    SetNextFocus();
                    return TRUE;
                case ID_STEPOVER:
                    StepOverFunction();
                    return TRUE;
                case ID_STEP:
                    StepProgram();
                    if (Exiting)
                        PostMessage(wnd, CLOSE_WINDOW, 0, 0);
                    return TRUE;
                case ID_BREAKPOINT:
                    ToggleBreakpoint();
                    return TRUE;
                case ID_DELBREAKPOINTS:
                    DeleteBreakpoints();
                    return TRUE;
                case ID_COMMANDLINE:
                    CommandLine();
                    return TRUE;
                case ID_MEMORY:
                    Memory();
                    return TRUE;
                case ID_TAB2:
                    cfg.Tabs = 2;
                    ChangeTabs();
                    return TRUE;
                case ID_TAB4:
                    cfg.Tabs = 4;
                    ChangeTabs();
                    return TRUE;
                case ID_TAB6:
                    cfg.Tabs = 6;                    
                    ChangeTabs();
                    return TRUE;
                case ID_TAB8:
                    cfg.Tabs = 8;
                    ChangeTabs();
                    return TRUE;
                case ID_DISPLAY:
                    InterceptingShow = TRUE;
                    PrevSbarSetting =
                        CheckBoxSetting(&Display, ID_SCROLLBARS);
                    PrevScreenLines = cfg.ScreenLines;
                    rtn = DefaultWndProc(wnd, msg, p1, p2);
                    InterceptingShow = FALSE;
                    return rtn;
                case ID_EXIT:
                case ID_SYSCLOSE:
                    if (Stepping)    {
                        Exiting = TRUE;
                        StopProgram();
                        return TRUE;
                    }
                    break;
#ifdef TUTORIAL
        		case ID_HELP:
        		case ID_HELPHELP:
        		case ID_KEYSHELP:
        		case ID_HELPINDEX:
					ResetHelpFile();
					break;
#endif
                case ID_ABOUT:
                    MessageBox
                    (
                         "About Quincy",
                        "     ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n"
                        "     ³   ÜÜ  Ü   Ü  ÜÜ   ³\n"
                        "     ³  Û  Û ÛÜ  Û Û  ß  ³\n"
                        "     ³  Û  Û Û ßÜÛ Û  Ü  ³\n"
                        "     ³   ßßÛ ß   ß  ßß   ³\n"
                        "     ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n"
                        "    "      PROGTITLE         "\n"
                        "          Version " QVERSION "\n"
                        " Copyright (c) 1994 Al Stevens\n"
                        "       All Rights Reserved"
                    );
                    return TRUE;
#ifdef TUTORIAL
				case ID_TUTORCONTENTS:
					SendMessage(wnd, DISPLAY_HELP, (PARAM) tuthelp, 0);
					return TRUE;
				case ID_THISTUTOR:
					DisplayThisTutorial();
					return TRUE;
				case ID_NEXTTUTOR:
					DisplayNewTutorial("ex%05d.c", 1);
					return TRUE;
				case ID_PREVTUTOR:
					DisplayNewTutorial("ex%05d.c", -1);
					return TRUE;
				case ID_CHAPTER:
					DisplayNewTutorial("chap%d", 0);
					return TRUE;
				case ID_TUTORIAL:
					if (GetCommandToggle(&MainMenu, ID_TUTORIAL))	{
						qCfg.inTutorial = 1;
						LoadHelpFile(TutorialHelpFile);
						TutLoaded = TRUE;
						DisplayThisTutorial();
					}
					else	{
						qCfg.inTutorial = 0;
					    LoadHelpFile(DFlatApplication);
						TutLoaded = FALSE;
					}
                    return TRUE;
#endif
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}
/* --- The Open... command. Select a file  --- */
static void SelectFile(void)
{
    char FileName[64];
    if (OpenFileDialogBox("*.c", FileName))
        /* --- see if the document is already in --- */
        if (stricmp(FileName, editWnd->extension) != 0)
            OpenSourceCode(FileName);
}
/* --- open a document window and load a file --- */
static void OpenSourceCode(char *FileName)
{
    WINDOW wwnd;
    struct stat sb;
    char *ermsg;

    CloseSourceCode();
    wwnd = WatchIcon();
    if (strcmp(FileName, Untitled))    {
        editWnd->extension = DFmalloc(strlen(FileName)+1);
        strcpy(editWnd->extension, FileName);
        LoadFile();
    }
    ChangeQncTitle();
    SendMessage(wwnd, CLOSE_WINDOW, 0, 0);
    SendMessage(editWnd, PAINT, 0, 0);
}
/* --- Load source code file into editor text buffer --- */
static void LoadFile(void)
{
    FILE *fp;
    if ((fp = fopen(editWnd->extension, "rt")) != NULL)    {
        char *Buf;
        struct stat sb;

        stat(editWnd->extension, &sb);
        Buf = DFcalloc(1, sb.st_size+1);

        /* --- read the source file --- */
        fread(Buf, sb.st_size, 1, fp);
        fclose(fp);
        SendMessage(editWnd, SETTEXT, (PARAM) Buf, 0);
        free(Buf);
    }
}
/* ---------- save a file to disk ------------ */
static void SaveFile(int Saveas)
{
    FILE *fp;
    if (editWnd->extension == NULL || Saveas)    {
        char FileName[64];
        if (SaveAsDialogBox(editWnd->extension ? editWnd->extension : "*.c",
					"*.c", FileName))    {
            if (editWnd->extension == NULL || stricmp(editWnd->extension, FileName))
				if (access(FileName, 0) == 0)
			        if (!YesNoBox("Filename in use. Replace it?"))
						return;
            free(editWnd->extension);
            editWnd->extension = DFmalloc(strlen(FileName)+1);
            strcpy(editWnd->extension, FileName);
        }
        else
            return;
    }
    if (editWnd->extension != NULL)    {
        WINDOW mwnd = MomentaryMessage("Saving the file");
        if ((fp = fopen(editWnd->extension, "wt")) != NULL)    {
            CollapseTabs(editWnd);
            fwrite(GetText(editWnd),
                strlen(GetText(editWnd)), 1, fp);
            fclose(fp);
            ExpandTabs(editWnd);
        }
        SendMessage(mwnd, CLOSE_WINDOW, 0, 0);
        ChangeQncTitle();
        SendMessage(editWnd, SETFOCUS, TRUE, 0);
    }
}
/* ------ display the row and column in the statusbar ------ */
static void ShowPosition(WINDOW wnd)
{
    char status[60];
    sprintf(status, "Line:%4d  Column: %2d",
        wnd->CurrLine+1, wnd->CurrCol+1);
    SendMessage(GetParent(wnd), ADDSTATUS, (PARAM) status, 0);
}
/* ----- close and save the source code -------- */
static void CloseSourceCode(void)
{
    if (editWnd->TextChanged)
        if (YesNoBox("Text changed. Save it?"))
            SendMessage(applWnd, COMMAND, ID_SAVE, 0);
    SendMessage(editWnd, CLEARTEXT, 0, 0);
    SendMessage(editWnd, PAINT, 0, 0);
    if (editWnd->extension != NULL)    {
        free(editWnd->extension);
        editWnd->extension = NULL;
    }
    DeleteAllWatches();
    DeleteBreakpoints();
    StopProgram();
}
/* ---- count the newlines in a block of text --- */
static int CountNewLines(char *beg, char *end)
{
    int ct = 0;
    while (beg <= end)
        if (*beg++ == '\n')
            ct++;
    return ct;
}
/* ---- count the newlines in a block of editor text --- */
static int CountBlockNewLines(WINDOW wnd)
{
    return TextBlockMarked(wnd) ?
            CountNewLines(TextBlockBegin(wnd),
                TextBlockEnd(wnd)) : 0;
}
/* ---- count the newlines in clipboard text --- */
static int CountClipboardNewLines(void)
{
    return ClipboardLength ?
            CountNewLines(Clipboard,
                Clipboard+ClipboardLength-1) : 0;
}
/* ----- F1 pressed, search for help on selected word ---- */
static BOOL ContextHelp(WINDOW wnd)
{
	char fname[51];
	char *cp = CurrChar, *cp2 = fname;
	while ((isalnum(*cp) || *cp == '_' || *cp == '.') && cp > wnd->text)
		--cp;
	if (*cp != '#' && cp > wnd->text)
		cp++;
	while ((*cp == '#' || isalnum(*cp) || *cp == '_' || *cp == '.') && cp2 < fname+50)
		*cp2++ = *cp++;
	*cp2 = '\0';
#ifdef TUTORIAL
	ResetHelpFile();
#endif
	return DisplayHelp(applWnd, fname);
}
/* ----- window processing module for the editor ----- */
static int QncEditorProc(WINDOW wnd,MESSAGE msg,
                                        PARAM p1,PARAM p2)
{
    int rtn;
    switch (msg)    {
        case SETFOCUS:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            if ((int)p1 == FALSE)    {
                SendMessage(GetParent(wnd), ADDSTATUS, 0, 0);
                if (ErrorCode == 0)
                    SendMessage(NULL, HIDE_CURSOR, 0, 0);
            }
            else 
                ShowPosition(wnd);
            return rtn;
        case SCROLL:
        case PAINT:
        {
            int lno;
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            /* ---- update source line pointer and
                        breakpoint displays ------ */
            for (lno = wnd->wtop+1;
                    lno <= wnd->wtop+ClientHeight(wnd); lno++)
                if ((Stepping && lno == LastStep) ||
                        isBreakpoint(lno))
                    DisplaySourceLine(lno);
            return rtn;
        }
        case KEYBOARD:
            /* --- if user adds/deletes lines,
                    adjust breakpoint table in debugger --- */
            if ((int) p1 == '\r')
                AdjustBreakpointsInserting(wnd->CurrLine+1, 1);
            else if ((int) p1 == DEL && *CurrChar == '\n')
                AdjustBreakpointsDeleting(wnd->CurrLine+2, 1);
			else if ((int) p1 == F1)
				if (ContextHelp(wnd))
					return TRUE;
            break;
		case DOUBLE_CLICK:
			ContextHelp(wnd);
			return TRUE;
        case KEYBOARD_CURSOR:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            SendMessage(NULL, SHOW_CURSOR, 0, 0);
            ShowPosition(wnd);
            return rtn;
        case COMMAND:
            switch ((int) p1)    {
                case ID_SEARCH:
                    SearchText(wnd);
                    return TRUE;
                case ID_REPLACE:
                    ReplaceText(wnd);
                    return TRUE;
                case ID_SEARCHNEXT:
                    SearchNext(wnd);
                    return TRUE;
                case ID_CUT:
                    CopyToClipboard(wnd);
                    SendMessage(wnd, COMMAND, ID_DELETETEXT, 0);
                    SendMessage(wnd, PAINT, 0, 0);
                    return TRUE;
                case ID_COPY:
                    CopyToClipboard(wnd);
                    ClearTextBlock(wnd);
                    SendMessage(wnd, PAINT, 0, 0);
                    return TRUE;
                case ID_PASTE:
                    /* --- if user pastes lines,
                        adjust breakpoint table in debugger --- */
                    AdjustBreakpointsInserting(wnd->CurrLine+1,
                            CountClipboardNewLines());
                    PasteFromClipboard(wnd);
                    SendMessage(wnd, PAINT, 0, 0);
                    return TRUE;
                case ID_DELETETEXT:
                    /* --- if user deletes lines,
                        adjust breakpoint table in debugger --- */
                    AdjustBreakpointsDeleting(wnd->BlkBegLine+1,
                            CountBlockNewLines(wnd));
                    rtn = DefaultWndProc(wnd, msg, p1, p2);
                    SendMessage(wnd, PAINT, 0, 0);
                    return rtn;
                case ID_HELP:
                    DisplayHelp(applWnd, QUINCY);
                    return TRUE;
                default:
                    break;
            }
            break;
        case CLOSE_WINDOW:
            CloseSourceCode();
            break;
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}
/* -- point to the name component of a file specification -- */
static char *NameComponent(char *FileName)
{
    char *Fname;
    if ((Fname = strrchr(FileName, '\\')) == NULL)
        if ((Fname = strrchr(FileName, ':')) == NULL)
            Fname = FileName-1;
    return Fname + 1;
}
/* ---- rebuild display when user changes tab sizes ---- */
static void ChangeTabs(void)
{
    FixTabMenu();
    CollapseTabs(editWnd);
    ExpandTabs(editWnd);
}
/* ---- update the tab menu when user changes tabs ---- */
static void FixTabMenu(void)
{
    char *cp = GetCommandText(&MainMenu, ID_TABS);
    if (cp != NULL)    {
        cp = strchr(cp, '(');
        if (cp != NULL)    {
            *(cp+1) = cfg.Tabs + '0';
            if (GetClass(inFocus) == POPDOWNMENU)
                SendMessage(inFocus, PAINT, 0, 0);
        }
    }
}
/* ------- display the program's output screen ----- */
static void OutputScreen(void)
{
    SendMessage(NULL, HIDE_CURSOR, 0, 0);
    HideIDE();
    getkey();
    UnHideIDE();
    SendMessage(NULL, SHOW_CURSOR, 0, 0);
}
/* ---- the user may change certain
                interpreter memory parameters --- */
static void Memory(void)
{
    char text[20], *tx;
    int i;
    static struct prm {
        unsigned *max;
        unsigned id;
		unsigned lid;
    } parms[] = {
        { &qCfg.MaxProgram,     ID_PROGSIZE,    ID_PSUSED },
        { &qCfg.MaxDataSpace,   ID_DATASPACE,   ID_DSUSED },
		{ &qCfg.MaxSymbolTable, ID_SYMBOLTABLE, ID_SYUSED },
        { &qCfg.MaxVariables,   ID_VARIABLES,   ID_VAUSED },
		{ &qCfg.MaxFunctions,   ID_FUNCTIONS,   ID_FNUSED },
        { &qCfg.MaxStack,       ID_STACK,       ID_STUSED }
    };
    for (i = 0; i < (sizeof parms / sizeof(struct prm)); i++) {
        sprintf(text, "%u", *parms[i].max);
        SetEditBoxText(&MemoryDB, parms[i].id, text);
		switch (i)	{
			case 0:
		        sprintf(text, "%8u",  Progused);
				break;
			case 1:
		        sprintf(text, "%8u",  Ctx.NextData-DataSpace);
				break;
			case 2:
		        sprintf(text, "%8u",  SymbolCount);
				break;
			case 3:
		        sprintf(text, "%8u",  Ctx.NextVar-VariableMemory);
				break;
			case 4:
		        sprintf(text, "%8u",  NextFunction-FunctionMemory);
				break;
			case 5:
		        sprintf(text, "%8u",  Ctx.Stackptr-Stackbtm);
				break;
		}
		SetDlgText(&MemoryDB, parms[i].lid, text);
    }
	sprintf(text, "%lu", coreleft());
	SetDlgText(&MemoryDB, ID_HEAPLEFT, text);
    if (DialogBox(applWnd, &MemoryDB, TRUE, NULL))    {
        for (i = 0; i < (sizeof parms/sizeof(struct prm)); i++)
            *parms[i].max =
                atoi(GetEditBoxText(&MemoryDB, parms[i].id));
    }
}

