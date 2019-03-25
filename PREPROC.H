/* ------- preproc.h -------- */

#ifndef PREPROC_H
#define PREPROC_H

/* ---- #define macro table ---- */
typedef struct MacroTbl    {
    unsigned char *id;      /* macro identification */
    unsigned char *val;     /* macro value          */
    int isMacro;            /* true if () macro     */
    unsigned char parms;    /* number of parameters */
    struct MacroTbl *NextMacro;
} MACRO;

extern int MacroCount;

#define isSpace(c) \
    ( c == ' ' || c == '\t' || c == '\t'+128 || c == '\f'+128)
#define MAXMACROLENGTH 2048

int MacroExpression(unsigned char **);
int ResolveMacro(unsigned char *, unsigned char **);
MACRO *FindMacro(unsigned char*);
void ExtractWord(unsigned char *,
        unsigned char **, unsigned char *);
#endif

