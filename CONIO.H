/* ---- conio.h ---- */

#ifndef _CONIO_H
#define _CONIO_H

void clrscr(void);
int getch(void);
void putch(int);
void cursor(int x, int y);
int cprintf(const char *, ...);

#define ESC    27
#define RUBOUT 8
#define HOME   199
#define UP     200
#define PGUP   201
#define BS     203
#define FWD    205
#define END    207
#define DN     208
#define PGDN   209
#define INS    210
#define DEL    211
enum Fkeys { F1=187,F2,F3,F4,F5,F6,F7,F8,F9 };

#ifndef NULL
#define NULL (void*)0
#endif

#endif
