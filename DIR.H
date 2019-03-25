/* -------- dir.h -------- */

#ifndef _DIR_H
#define _DIR_H

struct ffblk {
    char ff_[30];
    char ff_name[13];
};

int findfirst(const char*,struct ffblk*,int);
int findnext(struct ffblk*);

#endif

