/* ------- stdarg.h ------- */
#ifndef _STDARG_H
#define _STDARG_H

typedef char *va_list;
#define va_start(ap,pn) (void)((ap)=(va_list)&(pn)+sizeof(pn))
#define va_arg(ap,ty) (*(ty*)(((ap)+=sizeof(ty))-sizeof(ty)))
#define va_end(ap) (void)0

#endif
