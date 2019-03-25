/* --- errno.h --- */

#ifndef _ERRNO_H
#define _ERRNO_H

#define EDOM   33
#define ERANGE 34

int *_Errno(void);

#define errno (*_Errno())

#endif
