/*
 * sys.h -- QUINCY library function codes
 */

#ifndef SYS_H
#define SYS_H

enum sysfuncs {

/*
 * System functions
 */

	SYSEXIT = 1,
	SYSSYSTEM,
	SYSSETJMP,
	SYSLONGJMP,
	SYSFINDFIRST,
	SYSFINDNEXT,
	SYSLINENO,
	SYSFILENAME,
	SYSERRNO,
/*
 * Console I/O functions
 */
	SYSGETCHAR,
	SYSPUTCHAR,
	SYSGETS,
	SYSPUTS,
	SYSSCANF,
	SYSPRINTF,
	SYSGETCH,
	SYSPUTCH,
	SYSCLRSCRN,
	SYSCURSOR,
	SYSCPRINTF,
/*
 * File I/O functions
 */
	SYSFOPEN,
	SYSFCLOSE,
	SYSFGETC,
	SYSFPUTC,
	SYSFGETS,
	SYSFPUTS,
	SYSFREAD,
	SYSFWRITE,
	SYSFTELL,
	SYSFSEEK,
	SYSUNGETC,
	SYSRENAME,
	SYSREWIND,
	SYSFSCANF,
	SYSFPRINTF,
	SYSFFLUSH,
	SYSREMOVE,
	SYSTMPFILE,
	SYSTMPNAM,
/*
 * String functions
 */
	SYSSTRCMP,
	SYSSTRCPY,
	SYSSTRNCMP,
	SYSSTRNCPY,
	SYSSTRLEN,
	SYSSTRCAT,
	SYSSTRNCAT,
/*
 * Memory allocation functions
 */
	SYSMALLOC,
	SYSFREE,
/*
 * String conversion routines
 */
	SYSATOF,
	SYSATOI,
	SYSATOL,
/*
 * Format conversion routines
 */
	SYSSSCANF,
	SYSSPRINTF,
/*
 * Time functions
 */
	SYSASCTIME,
	SYSGMTIME,
	SYSLOCALTIME,
	SYSMKTIME,
	SYSTIME,
/*
 * Math functions
 */
	SYSABS,
	SYSACOS,
	SYSASIN,
	SYSATAN,
	SYSCEIL,
	SYSCOS,
	SYSCOSH,
	SYSEXP,
	SYSFABS,
	SYSFLOOR,
	SYSSIN,
	SYSSINH,
	SYSSQRT,
	SYSTAN,
	SYSTANH,
	SYSLOG,
	SYSLOG10,
	SYSPOW,
	SYSATAN2
};

#endif

