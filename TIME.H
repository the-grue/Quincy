/* ------ time.h ------- */

#ifndef _TIME_H
#define _TIME_H

typedef long time_t;

/* --------- broken-down time structure ---------- */
struct tm
{
  int   tm_sec;    /* seconds (0-61)               */
  int   tm_min;    /* minutes (0-59)               */
  int   tm_hour;   /* hours   (0-23                */
  int   tm_mday;   /* day of the month (1-31)      */
  int   tm_mon;    /* months since January (0-11)  */
  int   tm_year;   /* years since 1900             */
  int   tm_wday;   /* days since Sunday (0-6)      */
  int   tm_yday;   /* days since January 1 (0-365) */
  int   tm_isdst;  /* Daylight Savings Time flag   */
};

char *asctime(const struct tm*);
char *ctime(const time_t*);
double difftime(time_t, time_t);
struct tm *gmtime(const time_t*);
struct tm *localtime(const time_t*);
time_t mktime(struct tm*);
time_t time(time_t *);

double difftime(time_t t1, time_t t0)
{
	return t1 - t0;
}

char *ctime(const time_t *timer)
{
	return asctime(localtime(timer));
}

#endif
