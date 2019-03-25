/* ------ print.c ----- */
#include "interp.h"

static int LineCtr;
static int CharCtr;
static char *ports[] = {
	"Lpt1",	"Lpt2",	"Lpt3",
	"Com1",	"Com2",	"Com3",	"Com4",
 	 NULL
};

/* ------- print a character -------- */
static void PrintChar(FILE *prn, int c)
{
	int i;
    if (c == '\n' || CharCtr == cfg.RightMargin)	{
		fputs("\r\n", prn);
		LineCtr++;
		if (LineCtr == cfg.BottomMargin)	{
    		fputc('\f', prn);
			for (i = 0; i < cfg.TopMargin; i++)
	    		fputc('\n', prn);
			LineCtr = cfg.TopMargin;
		}
		CharCtr = 0;
		if (c == '\n')
			return;
	}
	if (CharCtr == 0)	{
		for (i = 0; i < cfg.LeftMargin; i++)	{
			fputc(' ', prn);
			CharCtr++;
		}
	}
	CharCtr++;
    fputc(c & 0x80 ? ' ' : c, prn);
}
/* --- print the current source code file --- */
void PrintSourceCode(void)
{
	if (*cfg.PrinterPort)	{
		FILE *prn;
		if ((prn = fopen(cfg.PrinterPort, "wt")) != NULL)	{
		    unsigned char *text = GetText(editWnd);
			if (*text)	{
				long percent;
				BOOL KeepPrinting = TRUE;
				unsigned oldpct = 100;
				unsigned cct = 0;
				unsigned len = strlen(text);
				WINDOW swnd = SliderBox(20, GetTitle(editWnd),
					"Printing");
    			/* ------- print the source text --------- */
				LineCtr = CharCtr = 0;
				while (KeepPrinting && *text)	{
					PrintChar(prn, *text++);
					percent = ((long) ++cct * 100) / len;
					if ((int) percent != oldpct)	{
						oldpct = (int) percent;
						KeepPrinting =
							SendMessage(swnd, PAINT, 0, oldpct);
					}
    			}
				if (KeepPrinting)
					/* ---- user did not cancel ---- */
					if (oldpct < 100)
						SendMessage(swnd, PAINT, 0, 100);
   				/* ------- follow with a form feed? --------- */
   				if (YesNoBox("Form Feed?"))
       				fputc('\f', prn);
			}
			fclose(prn);
		}
		else
			ErrorMessage("Cannot open printer file");
	}
	else
		ErrorMessage("No printer selected");
}
int PrintSetupProc(WINDOW wnd, MESSAGE msg,PARAM p1, PARAM p2)
{
	int rtn, i = 0, mar;
	char marg[10];
	WINDOW cwnd;
    switch (msg)    {
		case CREATE_WINDOW:
		    rtn = DefaultWndProc(wnd, msg, p1, p2);
			PutItemText(wnd, ID_PRINTERPORT, cfg.PrinterPort);
			while (ports[i] != NULL)
				PutComboListText(wnd,ID_PRINTERPORT,ports[i++]);
			return rtn;
		case COMMAND:
			if ((int) p1 == ID_OK && (int) p2 == 0)
				GetItemText(wnd, ID_PRINTERPORT,
									cfg.PrinterPort, 4);
			break;
        default:
            break;
	}
    return DefaultWndProc(wnd, msg, p1, p2);
}


