/* ----------- qdialogs.c --------------- */

#include "dflat.h"
#include "interp.h"

/* -------------- the File Open dialog box --------------- */
DIALOGBOX( FileOpen )
    DB_TITLE(        "Open File",    -1,-1,19,57)
    CONTROL(TEXT,    "~Filename:",    3, 1, 1, 9, ID_FILENAME)
    CONTROL(EDITBOX, NULL,           13, 1, 1,40, ID_FILENAME)
    CONTROL(TEXT,    NULL,            3, 3, 1,50, ID_PATH ) 
    CONTROL(TEXT,    "F~iles:",       3, 5, 1, 6, ID_FILES )
    CONTROL(LISTBOX, NULL,            3, 6,10,14, ID_FILES )
    CONTROL(TEXT,    "~Directories:",19, 5, 1,12, ID_DIRECTORY )
    CONTROL(LISTBOX, NULL,           19, 6,10,13, ID_DIRECTORY )
    CONTROL(TEXT,    "Dri~ves:",     34, 5, 1, 7, ID_DRIVE )
    CONTROL(LISTBOX, NULL,           34, 6,10,10, ID_DRIVE )
    CONTROL(BUTTON,  "   ~OK   ",    46, 7, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ",    46,10, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",    46,13, 1, 8, ID_HELP)
ENDDB

/* -------------- the Save As dialog box --------------- */
DIALOGBOX( SaveAs )
    DB_TITLE(        "Save As",    -1,-1,19,57)
    CONTROL(TEXT,    "~Filename:",    3, 1, 1, 9, ID_FILENAME)
    CONTROL(EDITBOX, NULL,           13, 1, 1,40, ID_FILENAME)
    CONTROL(TEXT,    NULL,            3, 3, 1,50, ID_PATH ) 
    CONTROL(TEXT,    "F~iles:",       3, 5, 1, 6, ID_FILES )
    CONTROL(LISTBOX, NULL,            3, 6,10,14, ID_FILES )
    CONTROL(TEXT,    "~Directories:",19, 5, 1,12, ID_DIRECTORY )
    CONTROL(LISTBOX, NULL,           19, 6,10,13, ID_DIRECTORY )
    CONTROL(TEXT,    "Dri~ves:",     34, 5, 1, 7, ID_DRIVE )
    CONTROL(LISTBOX, NULL,           34, 6,10,10, ID_DRIVE )
    CONTROL(BUTTON,  "   ~OK   ",    46, 7, 1, 8, ID_OK)
    CONTROL(BUTTON,  " ~Cancel ",    46,10, 1, 8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Help  ",    46,13, 1, 8, ID_HELP)
ENDDB

/* -------------- The Printer Setup dialog box ------------------ */
DIALOGBOX( PrintSetup )
	DB_TITLE( "Printer Setup",   -1, -1, 8, 32)
	CONTROL(TEXT,     "~Port:",   4,  1,  1,  5, ID_PRINTERPORT)
	CONTROL(COMBOBOX, NULL,      12,  1,  8,  9, ID_PRINTERPORT)
    CONTROL(BUTTON, "   ~OK   ",  1,  4,  1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ", 11,  4,  1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ", 21,  4,  1,  8, ID_HELP)
ENDDB

/* -------------- the Search Text dialog box --------------- */
DIALOGBOX( SearchTextDB )
    DB_TITLE(        "Search Text",    -1,-1,9,48)
    CONTROL(TEXT,    "~Search for:",          2, 1, 1, 11, ID_SEARCHFOR)
    CONTROL(EDITBOX, NULL,                   14, 1, 1, 29, ID_SEARCHFOR)
    CONTROL(TEXT, "~Match upper/lower case:", 2, 3, 1, 23, ID_MATCHCASE)
	CONTROL(CHECKBOX,  NULL,                 26, 3, 1,  3, ID_MATCHCASE)
    CONTROL(BUTTON, "   ~OK   ",              7, 5, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",             19, 5, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",             31, 5, 1,  8, ID_HELP)
ENDDB

/* -------------- the Replace Text dialog box --------------- */
DIALOGBOX( ReplaceTextDB )
    DB_TITLE(        "Replace Text",    -1,-1,12,50)
    CONTROL(TEXT,    "~Search for:",          2, 1, 1, 11, ID_SEARCHFOR)
    CONTROL(EDITBOX, NULL,                   16, 1, 1, 29, ID_SEARCHFOR)
    CONTROL(TEXT,    "~Replace with:",        2, 3, 1, 13, ID_REPLACEWITH)
    CONTROL(EDITBOX, NULL,                   16, 3, 1, 29, ID_REPLACEWITH)
    CONTROL(TEXT, "~Match upper/lower case:", 2, 5, 1, 23, ID_MATCHCASE)
	CONTROL(CHECKBOX,  NULL,                 26, 5, 1,  3, ID_MATCHCASE)
    CONTROL(TEXT, "Replace ~Every Match:",    2, 6, 1, 23, ID_REPLACEALL)
	CONTROL(CHECKBOX,  NULL,                 26, 6, 1,  3, ID_REPLACEALL)
    CONTROL(BUTTON, "   ~OK   ",              7, 8, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",             20, 8, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",             33, 8, 1,  8, ID_HELP)
ENDDB

/* -------------- generic message dialog box --------------- */
DIALOGBOX( MsgBox )
    DB_TITLE(       NULL,  -1,-1, 0, 0)
    CONTROL(TEXT,   NULL,   1, 1, 0, 0, 0)
    CONTROL(BUTTON, NULL,   0, 0, 1, 8, ID_OK)
    CONTROL(0,      NULL,   0, 0, 1, 8, ID_CANCEL)
ENDDB

/* ----------- InputBox Dialog Box ------------ */
DIALOGBOX( InputBoxDB )
    DB_TITLE(        NULL,      -1,-1, 9, 0)
    CONTROL(TEXT,    NULL,       1, 1, 1, 0, 0)
	CONTROL(EDITBOX, NULL,       1, 3, 1, 0, ID_INPUTTEXT)
    CONTROL(BUTTON, "   ~OK   ", 0, 5, 1, 8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ", 0, 5, 1, 8, ID_CANCEL)
ENDDB

/* ----------- SliderBox Dialog Box ------------- */
DIALOGBOX( SliderBoxDB )
    DB_TITLE(       NULL,      -1,-1, 9, 0)
    CONTROL(TEXT,   NULL,       0, 1, 1, 0, 0)
    CONTROL(TEXT,   NULL,       0, 3, 1, 0, 0)
    CONTROL(BUTTON, " Cancel ", 0, 5, 1, 8, ID_CANCEL)
ENDDB

#define offset 0

/* ------------ Display dialog box -------------- */
DIALOGBOX( Display )
    DB_TITLE(     "Display", -1, -1, 12+offset, 35)
	CONTROL(BOX,      "Colors",    1, 1+offset,5,15, 0)
    CONTROL(RADIOBUTTON, NULL,     3, 2+offset,1,3,ID_COLOR)
    CONTROL(TEXT,     "Co~lor",    7, 2+offset,1,5,ID_COLOR)
    CONTROL(RADIOBUTTON, NULL,     3, 3+offset,1,3,ID_MONO)
    CONTROL(TEXT,     "~Mono",     7, 3+offset,1,4,ID_MONO)
    CONTROL(RADIOBUTTON, NULL,     3, 4+offset,1,3,ID_REVERSE)
    CONTROL(TEXT,     "~Reverse",  7, 4+offset,1,7,ID_REVERSE)

	CONTROL(BOX,      "Lines",    17, 1+offset,5,15, 0)
    CONTROL(RADIOBUTTON, NULL,    19, 2+offset,1,3,ID_25LINES)
    CONTROL(TEXT,     "~25",      23, 2+offset,1,2,ID_25LINES)
    CONTROL(RADIOBUTTON, NULL,    19, 3+offset,1,3,ID_43LINES)
    CONTROL(TEXT,     "~43",      23, 3+offset,1,2,ID_43LINES)
    CONTROL(RADIOBUTTON, NULL,    19, 4+offset,1,3,ID_50LINES)
    CONTROL(TEXT,     "~50",      23, 4+offset,1,2,ID_50LINES)

    CONTROL(CHECKBOX,    NULL,     1, 6+offset, 1, 3, ID_SCROLLBARS)
    CONTROL(TEXT,  "Scroll ~bars", 5, 6+offset, 1,11, ID_SCROLLBARS)

    CONTROL(CHECKBOX,    NULL,    17, 6+offset,1,3,ID_SNOWY)
    CONTROL(TEXT,     "S~nowy",   20, 6+offset,1,7,ID_SNOWY)

    CONTROL(BUTTON, "   ~OK   ",   2, 8+offset,1,8,ID_OK)
    CONTROL(BUTTON, " ~Cancel ",  12, 8+offset,1,8,ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",  22, 8+offset,1,8,ID_HELP)
ENDDB

/* ------------ the Help window dialog box -------------- */
DIALOGBOX( HelpBox )
    DB_TITLE(         NULL,       -1, -1, 0, 45)
    CONTROL(TEXTBOX, NULL,         1,  1, 0, 40, ID_HELPTEXT)
    CONTROL(BUTTON,  "  ~Close ",  0,  0, 1,  8, ID_CANCEL)
    CONTROL(BUTTON,  "  ~Back  ", 10,  0, 1,  8, ID_BACK)
    CONTROL(BUTTON,  "<< ~Prev ", 20,  0, 1,  8, ID_PREV)
    CONTROL(BUTTON,  " ~Next >>", 30,  0, 1,  8, ID_NEXT)
ENDDB


/* ----------- Examine Dialog Box ------------ */
DIALOGBOX( ExamineDB )
    DB_TITLE(    "Examine",      -1,-1,11, 50)
    CONTROL(TEXT, "Data ~item:",  1, 1, 1, 10, ID_EXAMINE)
	CONTROL(EDITBOX, NULL,       12, 1, 1, 30, ID_EXAMINE)
	CONTROL(TEXT, "Value:",       1, 3, 1,  6, 0 )
	CONTROL(TEXT, NULL,          12, 3, 1, 30, ID_VALUE)
    CONTROL(TEXT, "~Modify:",  	  1, 5, 1,  7, ID_CHANGE)
	CONTROL(EDITBOX, NULL,       12, 5, 1, 30, ID_CHANGE)
    CONTROL(BUTTON, "   ~OK   ", 15, 7, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ", 25, 7, 1,  8, ID_CANCEL)
ENDDB

/* ----------- Memory Dialog Box ------------ */
DIALOGBOX( MemoryDB )
    DB_TITLE("Memory Options",      -1,-1,19, 41)
    CONTROL(TEXT, "~Program size:", 3, 1, 1, 13, ID_PROGSIZE)
	CONTROL(EDITBOX, NULL,         18, 1, 1, 10, ID_PROGSIZE)
	CONTROL(TEXT, NULL,            29, 1, 1,  8, ID_PSUSED)
    CONTROL(TEXT, "~Data space:",   3, 3, 1, 11, ID_DATASPACE)
	CONTROL(EDITBOX, NULL,         18, 3, 1, 10, ID_DATASPACE)
	CONTROL(TEXT, NULL,            29, 3, 1,  8, ID_DSUSED)
    CONTROL(TEXT, "S~ymbol table:", 3, 5, 1, 13, ID_SYMBOLTABLE)
	CONTROL(EDITBOX, NULL,         18, 5, 1, 10, ID_SYMBOLTABLE)
	CONTROL(TEXT, NULL,            29, 5, 1,  8, ID_SYUSED)
    CONTROL(TEXT, "~Variables:",    3, 7, 1, 10, ID_VARIABLES)
	CONTROL(EDITBOX, NULL,         18, 7, 1, 10, ID_VARIABLES)
	CONTROL(TEXT, NULL,            29, 7, 1,  8, ID_VAUSED)
    CONTROL(TEXT, "~Functions:",    3, 9, 1, 10, ID_FUNCTIONS)
	CONTROL(EDITBOX, NULL,         18, 9, 1, 10, ID_FUNCTIONS)
	CONTROL(TEXT, NULL,            29, 9, 1,  8, ID_FNUSED)
    CONTROL(TEXT, "~Stack items:",  3,11, 1, 13, ID_STACK)
	CONTROL(EDITBOX, NULL,         18,11, 1, 10, ID_STACK)
	CONTROL(TEXT, NULL,            29,11, 1,  8, ID_STUSED)
	CONTROL(TEXT, "Unused heap:",   3,13, 1, 12, 0)
	CONTROL(TEXT, NULL,            18,13, 1, 10, ID_HEAPLEFT)
    CONTROL(BUTTON, "   ~OK   ",    3,15, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",   15,15, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",   27,15, 1,  8, ID_HELP)
ENDDB

/* ------------ Function Stack Dialog Box -------- */
DIALOGBOX( FunctionStackDB )
	DB_TITLE(NULL, -1, -1, 15, 32)
    CONTROL(LISTBOX, NULL,          1, 1, 9, 28, ID_FUNCSTACK )
    CONTROL(BUTTON, "   ~OK   ",    1,11, 1,  8, ID_OK)
    CONTROL(BUTTON, " ~Cancel ",   11,11, 1,  8, ID_CANCEL)
    CONTROL(BUTTON, "  ~Help  ",   21,11, 1,  8, ID_HELP)
ENDDB
