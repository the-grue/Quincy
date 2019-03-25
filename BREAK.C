/* ----------- break.c ---------- */
#include "qnc.h"
#include "interp.h"
#include "debugger.h"

static int Breakpoints[MAXBREAKS];
int BreakpointCount;

static void ClearBreakpoint(int);
static void AddBreakpoint(int);

/* ------------------ breakpoint functions -------------- */
void DeleteBreakpoints(void)
{
	memset(Breakpoints, 0, sizeof Breakpoints);
	SendMessage(editWnd, PAINT, 0, 0);
	BreakpointCount = 0;
}

void ToggleBreakpoint(void)
{
	int lno = editWnd->CurrLine + 1;
	if (isBreakpoint(lno))
		/* --- breakpoint is at this line number --- */
		ClearBreakpoint(lno);
	else
		/* --- set a breakpoint at the line --- */
		AddBreakpoint(lno);
}

int isBreakpoint(int lno)
{
	int i;
	for (i = 0; i < MAXBREAKS; i++)
		if (Breakpoints[i] == lno)
			break;
	return i < MAXBREAKS;
}

static void AddBreakpoint(int lno)
{
	int i;
	for (i = 0; i < MAXBREAKS; i++)	{
		if (Breakpoints[i] == 0)	{
			Breakpoints[i] = lno;
			BreakpointCount++;
			DisplaySourceLine(lno);
			break;
		}
	}
	if (i == MAXBREAKS)
		ErrorMessage("Too many breakpoints");
}

static void ClearBreakpoint(int lno)
{
	int i;
	for (i = 0; i < MAXBREAKS; i++)	{
		if (Breakpoints[i] == lno)	{
			Breakpoints[i] = 0;
			DisplaySourceLine(lno);
			--BreakpointCount;
			break;
		}
	}
}

void AdjustBreakpointsInserting(int lno, int linecount)
{
	int last = GetTextLines(editWnd);
	while (last >= lno)	{
		if (isBreakpoint(last))	{
			ClearBreakpoint(last);
			AddBreakpoint(last+linecount);
		}
		--last;
	}
}

void AdjustBreakpointsDeleting(int lno, int linecount)
{
	int last = GetTextLines(editWnd);
	int olno;
	olno = lno+linecount;
	while (lno <= last)	{
		if (isBreakpoint(lno))	{
			ClearBreakpoint(lno);
			if (lno >= olno)
				AddBreakpoint(lno-linecount);
		}
		lno++;
	}
}
