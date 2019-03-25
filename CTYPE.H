/* ------- ctype.h ------- */

#ifndef _CTYPE_H
#define _CTYPE_H

#define isdigit(c) ((c)>='0'&&(c)<='9')
#define isupper(c) ((c)>='A'&&(c)<='Z')
#define islower(c) ((c)>='a'&&(c)<='z')
#define isalpha(c) (isupper(c)||islower(c))
#define isalnum(c) (isalpha(c)||isdigit(c))
#define isprint(c) ((c)>31&&(c)<128)
#define isspace(c) ((c)==' '  || (c)=='\t' ||  \
                    (c)=='\n' || (c)=='\v' ||  \
                    (c)=='\f' || (c)=='\r')
#define toupper(c) (islower(c)?((c)-32):(c))
#define tolower(c) (isupper(c)?((c)+32):(c))

#endif

