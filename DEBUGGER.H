/* -------- debugger.h ------- */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#define MAXBREAKS 25
#define SIZEWATCH 110
#define MAXARGS 20
#define WATCHHT 2
#define MAXWATCHHT 5
#define MAXWATCHES 25
#define MAXEXERCISE 50
#define MAXCHAP 9

extern BOOL Running;
extern BOOL Stepping;
extern BOOL CtrlBreaking;
extern BOOL Exiting;
extern WINDOW watchWnd;
extern DBOX ExamineDB;
extern DBOX FunctionStackDB;
extern char *errs[];
extern int LastStep;
extern int BreakpointCount;
extern int DupStdin;
extern int DupStdout;
extern int oldstdin;
extern int oldstdout;
extern char DupFileIn[65];
extern char DupFileOut[65];
extern char ErrorMsg[160];

int isBreakpoint(int);
void AdjustBreakpointsInserting(int, int);
void AdjustBreakpointsDeleting(int, int);
void SetBreakpointColor(WINDOW);
void SetBreakpointCursorColor(WINDOW);
void DisplaySourceLine(int);
void DisplayError(void);
void RunProgram(void);
void StepProgram(void);
void StepOverFunction(void);
void StopProgram(void);
void CommandLine(void);
void ExamineVariable(void);
void ToggleWatch(void);
void AddWatch(char *);
void FunctionStack(void);
void FunctionList(void);
void GetWatch(void);
void ShowWatches(void);
void DelWatch(void);
void DeleteAllWatches(void);
void UpdateWatches(void);
void unBreak(void);
void reBreak(void);

#endif

