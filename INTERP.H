/* ------- interp.h -------- */

#ifndef INTERP_H
#define INTERP_H

#include "dflat.h"

#define QUINCY "Quincy"

/* ----- Menu and dialog box command codes ------ */
enum    {
    ID_OUTPUT = 200,
    ID_SCROLLBARS,
    ID_RUN,
    ID_STEP,
    ID_STOP,
    ID_BREAKPOINT,
    ID_WATCH,
    ID_DELBREAKPOINTS,
    ID_COMMANDLINE,
    ID_EXAMINE,
    ID_CHANGE,
    ID_VALUE,
    ID_STEPOVER,
	ID_FUNCTIONSTACK,
	ID_FUNCTIONLIST,
	ID_FUNCSTACK,
    ID_ADDWATCH,
    ID_SWITCHWINDOW,
    ID_MEMORY,
    ID_PROGSIZE,
    ID_DATASPACE,
    ID_VARIABLES,
	ID_FUNCTIONS,
    ID_STACK,
	ID_HEAPLEFT,
	ID_PSUSED,
	ID_DSUSED,
	ID_SYUSED,
	ID_VAUSED,
	ID_FNUSED,
	ID_STUSED,
	ID_SYMBOLTABLE
#ifdef TUTORIAL
	,
	ID_TUTORIAL,
	ID_TUTOR,
	ID_THISTUTOR,
	ID_NEXTTUTOR,
	ID_PREVTUTOR,
	ID_TUTORCONTENTS,
	ID_CHAPTER
#endif
};

int qinterpmain(unsigned char *source, int argc, char *argv[]);
void HideIDE(void);
void UnHideIDE(void);
void PrintSourceCode(void);
void unBreak(void);
void reBreak(void);
int PrintSetupProc(WINDOW, MESSAGE, PARAM, PARAM);

extern WINDOW editWnd;
extern WINDOW applWnd;
extern int currx, curry;
extern DBOX Display;
extern DBOX PrintSetup;
extern DBOX MemoryDB;

#endif
