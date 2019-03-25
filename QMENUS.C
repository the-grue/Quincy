/* -------------- qmenus.c ------------- */

#include "dflat.h"
#include "interp.h"
#include "debugger.h"

void PrepViewMenu(void *, struct Menu *);
void PrepRunMenu(void *, struct Menu *);
void PrepDebugMenu(void *, struct Menu *);
#ifdef TUTORIAL
void PrepTutorMenu(void *, struct Menu *);
void PrepHelpMenu(void *, struct Menu *);
extern char TutorialHelpFile[];
#else
#define PrepHelpMenu NULL
#endif

/* --------------------- the main menu --------------------- */
DEFMENU(MainMenu)
    /* --------------- the File popdown menu ----------------*/
    POPDOWN( "~File",  PrepFileMenu, "Read/write/print files. Go to DOS" )
        SELECTION( "~New",        ID_NEW,          0, 0 )
        SELECTION( "~Open...",    ID_OPEN,         0, 0 )
        SEPARATOR
        SELECTION( "~Save",       ID_SAVE,         0, 0 )
        SELECTION( "Save ~as...", ID_SAVEAS,       0, 0 )
        SEPARATOR
        SELECTION( "~Print",      ID_PRINT,        0, INACTIVE)
        SELECTION( "P~rinter setup...", ID_PRINTSETUP, 0, 0   )
        SEPARATOR
        SELECTION( "~DOS",        ID_DOS,          0, 0 )
        SELECTION( "E~xit",       ID_EXIT,     ALT_X, 0 )
    ENDPOPDOWN

    /* --------------- the Edit popdown menu ----------------*/
    POPDOWN( "~Edit", PrepEditMenu, "Clipboard, delete text, paragraph" )
        SELECTION( "Cu~t",       ID_CUT,   SHIFT_DEL, INACTIVE)
        SELECTION( "~Copy",      ID_COPY,  CTRL_INS,  INACTIVE)
        SELECTION( "~Paste",     ID_PASTE, SHIFT_INS, INACTIVE)
        SEPARATOR
        SELECTION( "~Delete",    ID_DELETETEXT, DEL,  INACTIVE)
    ENDPOPDOWN

	/* --------------- the View popdown menu ----------------*/
	POPDOWN( "~View", PrepViewMenu, "View program components, output" )
		SELECTION( "~Examine...",       ID_EXAMINE, 0, INACTIVE)
		SELECTION( "~Output screen",    ID_OUTPUT,  0, 0)
		SELECTION( "~Functions",        ID_FUNCTIONLIST,  0, 0)
		SELECTION( "F~unction history", ID_FUNCTIONSTACK, 0, 0)
		SELECTION( "~Watch window",     ID_WATCH,   F6, TOGGLE)
		SELECTION( "~Switch windows",   ID_SWITCHWINDOW,  ALT_F6, 0)
	ENDPOPDOWN

	/* --------------- the Run popdown menu ----------------*/
	POPDOWN( "~Run", PrepRunMenu, "Run the program, step, trace" )
		SELECTION( "~Run",					ID_RUN,   F9, INACTIVE)
		SELECTION( "~Step",					ID_STEP,  F7, INACTIVE)
		SELECTION( "Step ~over",		ID_STEPOVER,  F8, INACTIVE)
		SELECTION( "Sto~p",					ID_STOP,   0, INACTIVE)
	ENDPOPDOWN

	/* --------------- the Debug popdown menu ----------------*/
	POPDOWN( "~Debug", PrepDebugMenu, "Debug procedures" )
		SELECTION( "Add a ~watch variable...",ID_ADDWATCH, ALT_W, 0)
		SELECTION( "~Breakpoint on/off",  	  ID_BREAKPOINT, F2, INACTIVE)
		SELECTION( "~Delete all breakpoints", ID_DELBREAKPOINTS, 0, INACTIVE)
	ENDPOPDOWN

	/* --------------- the Search popdown menu ----------------*/
    POPDOWN( "~Search", PrepSearchMenu, "Search and replace" )
        SELECTION( "~Search...", ID_SEARCH,      0,    0)
        SELECTION( "~Replace...",ID_REPLACE,     0,    0)
        SELECTION( "~Next",      ID_SEARCHNEXT,  F3,   0)
    ENDPOPDOWN

    /* ------------- the Options popdown menu ---------------*/
    POPDOWN( "~Options", NULL, "Editor and display options" )
		SELECTION( "~Command line...", ID_COMMANDLINE, 0,    0 )
        SELECTION( "~Display...",   ID_DISPLAY,     0,      0 )
        SELECTION( "~Tabs ( )",     ID_TABS,        0,  CASCADED)
        SEPARATOR
		SELECTION( "~Memory",       ID_MEMORY,	0, 0 )
    ENDPOPDOWN

    /* --------------- the Help popdown menu ----------------*/
    POPDOWN( "~Help", PrepHelpMenu, "Get help" )
        SELECTION(  "~Help for help...",  ID_HELPHELP,  0, 0 )
        SELECTION(  "~Keys help...",      ID_KEYSHELP,  0, 0 )
        SELECTION(  "Help ~index...",     ID_HELPINDEX, 0, 0 )
        SEPARATOR
        SELECTION(  "~About...",          ID_ABOUT,     0, 0 )
#ifdef TUTORIAL
		SEPARATOR
		SELECTION(  "~Tutorial",          ID_TUTOR,     0, CASCADED )
#endif
    ENDPOPDOWN

#ifdef TUTORIAL
	CASCADED_POPDOWN( ID_TUTOR, PrepTutorMenu )
		SELECTION(  "T~utorial Mode",  ID_TUTORIAL,      0, TOGGLE )
		SELECTION(  "~Contents",       ID_TUTORCONTENTS, ALT_C, 0 )
		SELECTION(  "Ch~apter",        ID_CHAPTER,       ALT_A, 0 )
		SEPARATOR
		SELECTION(  "~This exercise",  ID_THISTUTOR,     ALT_T, 0 )
		SELECTION(  "~Next exercise",  ID_NEXTTUTOR,     ALT_N, 0 )
		SELECTION(  "~Prior exercise", ID_PREVTUTOR,     ALT_P, 0 )
    ENDPOPDOWN
#endif

	/* ----- cascaded pulldown from Tabs... above ----- */
	CASCADED_POPDOWN( ID_TABS, NULL )
		SELECTION( "~2 tab stops", ID_TAB2, 0, 0)
		SELECTION( "~4 tab stops", ID_TAB4, 0, 0)
		SELECTION( "~6 tab stops", ID_TAB6, 0, 0)
		SELECTION( "~8 tab stops", ID_TAB8, 0, 0)
    ENDPOPDOWN

ENDMENU

/* ------------- the System Menu --------------------- */
DEFMENU(SystemMenu)
    POPDOWN("System Menu", NULL, NULL)
#ifdef INCLUDE_RESTORE
        SELECTION("~Restore",  ID_SYSRESTORE,  0,         0 )
#endif
        SELECTION("~Move",     ID_SYSMOVE,     0,         0 )
        SELECTION("~Size",     ID_SYSSIZE,     0,         0 )
#ifdef INCLUDE_MINIMIZE
        SELECTION("Mi~nimize", ID_SYSMINIMIZE, 0,         0 )
#endif
#ifdef INCLUDE_MAXIMIZE
        SELECTION("Ma~ximize", ID_SYSMAXIMIZE, 0,         0 )
#endif
        SEPARATOR
        SELECTION("~Close",    ID_SYSCLOSE,    CTRL_F4,   0 )
    ENDPOPDOWN
ENDMENU

#ifdef TUTORIAL
void PrepHelpMenu(void *w, struct Menu *mnu)
{
	static BOOL prepped = FALSE;
	if (!prepped)	{
		char path[128];
		prepped = TRUE;
		BuildFileName(path, TutorialHelpFile, ".hlp");
		if (access(path, 0) != 0)
			MainMenu.PullDown[7].Selections[6].SelectionTitle = NULL;
	}
}

void PrepTutorMenu(void *w, struct Menu *mnu)
{
	if (GetCommandToggle(&MainMenu, ID_TUTORIAL))	{
		ActivateCommand(&MainMenu, ID_TUTORCONTENTS);
		ActivateCommand(&MainMenu, ID_CHAPTER);
		ActivateCommand(&MainMenu, ID_THISTUTOR);
		ActivateCommand(&MainMenu, ID_NEXTTUTOR);
		ActivateCommand(&MainMenu, ID_PREVTUTOR);
	}
	else	{
		DeactivateCommand(&MainMenu, ID_TUTORCONTENTS);
		DeactivateCommand(&MainMenu, ID_CHAPTER);
		DeactivateCommand(&MainMenu, ID_THISTUTOR);
		DeactivateCommand(&MainMenu, ID_NEXTTUTOR);
		DeactivateCommand(&MainMenu, ID_PREVTUTOR);
	}
}
#endif

void PrepRunMenu(void *w, struct Menu *mnu)
{
	DeactivateCommand(&MainMenu, ID_RUN);
	DeactivateCommand(&MainMenu, ID_STEP);
	DeactivateCommand(&MainMenu, ID_STOP);
	DeactivateCommand(&MainMenu, ID_STEPOVER);
	if (*editWnd->text)	{
		ActivateCommand(&MainMenu, ID_RUN);
		ActivateCommand(&MainMenu, ID_STEP);
	}
	if (Stepping)	{
		ActivateCommand(&MainMenu, ID_STOP);
		ActivateCommand(&MainMenu, ID_STEPOVER);
	}
}

void PrepViewMenu(void *w, struct Menu *mnu)
{
	DeactivateCommand(&MainMenu, ID_EXAMINE);
	DeactivateCommand(&MainMenu, ID_SWITCHWINDOW);
	DeactivateCommand(&MainMenu, ID_FUNCTIONSTACK);
	DeactivateCommand(&MainMenu, ID_FUNCTIONLIST);
	if (Stepping)	{
		ActivateCommand(&MainMenu, ID_EXAMINE);
		ActivateCommand(&MainMenu, ID_FUNCTIONSTACK);
		ActivateCommand(&MainMenu, ID_FUNCTIONLIST);
	}
	if (watchWnd)
		ActivateCommand(&MainMenu, ID_SWITCHWINDOW);
}

void PrepDebugMenu(void *w, struct Menu *mnu)
{
	DeactivateCommand(&MainMenu, ID_BREAKPOINT);
	DeactivateCommand(&MainMenu, ID_DELBREAKPOINTS);
	if (w == editWnd && *editWnd->text)
		ActivateCommand(&MainMenu, ID_BREAKPOINT);
	if (BreakpointCount)
		ActivateCommand(&MainMenu, ID_DELBREAKPOINTS);
}

void PrepFileMenu(void *w, struct Menu *mnu)
{
	DeactivateCommand(&MainMenu, ID_PRINT);
	if (*editWnd->text)
		ActivateCommand(&MainMenu, ID_PRINT);
}

void PrepSearchMenu(void *w, struct Menu *mnu)
{
	DeactivateCommand(&MainMenu, ID_SEARCH);
	DeactivateCommand(&MainMenu, ID_REPLACE);
	DeactivateCommand(&MainMenu, ID_SEARCHNEXT);
	if (w == editWnd && *editWnd->text)	{
		ActivateCommand(&MainMenu, ID_SEARCH);
		ActivateCommand(&MainMenu, ID_REPLACE);
		ActivateCommand(&MainMenu, ID_SEARCHNEXT);
	}
}

void PrepEditMenu(void *w, struct Menu *mnu)
{
	DeactivateCommand(&MainMenu, ID_CUT);
	DeactivateCommand(&MainMenu, ID_COPY);
	DeactivateCommand(&MainMenu, ID_CLEAR);
	DeactivateCommand(&MainMenu, ID_DELETETEXT);
	DeactivateCommand(&MainMenu, ID_PASTE);
	if (w == editWnd && *editWnd->text)	{
		if (TextBlockMarked(editWnd))	{
			ActivateCommand(&MainMenu, ID_CUT);
			ActivateCommand(&MainMenu, ID_COPY);
			ActivateCommand(&MainMenu, ID_CLEAR);
			ActivateCommand(&MainMenu, ID_DELETETEXT);
		}
	}
	if (w == editWnd && Clipboard != NULL)
		ActivateCommand(&MainMenu, ID_PASTE);
}


