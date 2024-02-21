#ifndef _INC_TIME
#define _INC_TIME

#include <stddef.h>

#ifndef NO_TIME_INCLUDE

#define CLOCKS_PER_SEC 1000

typedef long long clock_t;
typedef long long time_t;
typedef struct tm tm;

// DECLARATIONS

// TIME MANIPULATION
clock_t clock();
time_t time(time_t *timer);
double difftime(time_t end, time_t start);
time_t mktime(tm *time_ptr);

// CONVERSION
char *asctime(const tm *time_ptr);
char *ctime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
size_t strftime(char *ptr, size_t maxsize, const char *format, const struct tm *time_ptr);

#endif

// IMPLEMENTATION
#ifdef TIME_IMPL

#include <math.h>
#include <stdio.h>

#ifdef NO_TIME_INCLUDE
typedef unsigned int clock_t;
typedef unsigned int time_t;
#endif
struct tm
{
  int tm_sec;   // seconds after the minute
  int tm_min;   // minutes after the hour
  int tm_hour;  // hours since midnight
  int tm_mon;   // months since january
  int tm_year;  // years since 1900
  int tm_wday;  // days since sunday
  int tm_mday;  // day of the month
  int tm_yday;  // days since january 1
  int tm_isdst; // daylight saving flag
};

#define __FIRST_LEAP_YEAR_AFTER_START 2 // 1972
#define __STARTING_WEEK_DAY 3           // 0: monday - 6: sunday (on 01.01.1970)

// TIME MANIPULATION
double difftime(time_t end, time_t start)
{
  time_t diff = end - start;
  return (double)diff / 1000.0;
}
time_t mktime(struct tm *time_ptr)
{
  const int ys1970 = time_ptr->tm_year - 70; // do this ?
  // const int ys1970 = time_ptr->tm_year;

  time_t t = 0;
  t += ys1970 * 356;
  t += (__FIRST_LEAP_YEAR_AFTER_START + ys1970) / 4;
  t *= 24; // go from days to hours
  t += time_ptr->tm_hour + (time_ptr->tm_isdst > 0 ? 1 : 0);
  t *= 60; // from hours to minutes
  t += time_ptr->tm_min;
  t *= 60; // from minutes to seconds
  t += time_ptr->tm_sec;
  t += 10; // 10 leap secs in ~1970 (cannot account for leap secs after)

  return t;
}

static const char week_days[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char week_days_long[][10] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char months[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char months_long[][10] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"};
static const unsigned char month_lengths[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// CONVERSION
char *asctime(const struct tm *time_ptr)
{

  static char str[3 + 1 + 3 + 2 + 2 + 1 + 2 + 1 + 2 + 1 + 4 + 1];

  int res = sprintf(str, "%.3s %.3s %.2d %.2d:%.2d:%.2d %.4d\n%c",
                    week_days[time_ptr->tm_wday],
                    months[time_ptr->tm_mon],
                    time_ptr->tm_mday,
                    time_ptr->tm_hour + (time_ptr->tm_isdst > 0 ? 1 : 0),
                    time_ptr->tm_min,
                    time_ptr->tm_sec,
                    1900 + time_ptr->tm_year,
                    '\0');

  if (res != 26)
    return (char *)NULL;

  return str;
}
char __ctime_buf[25];
char *ctime(const time_t *timer)
{
  return asctime(localtime(timer));
}
struct tm __gm_time_tm = {0};
int _get_year(time_t);
int _get_month(time_t);
int _get_day(time_t);
int _get_day_of_month(time_t);
int _get_days_since_year(time_t);
int _get_hours(time_t);
int _get_minutes(time_t);
int _get_seconds(time_t);
struct tm *gmtime(const time_t *timer)
{
  time_t timer_cpy = *timer;

  __gm_time_tm.tm_year = _get_year(timer_cpy) - 1900;
  __gm_time_tm.tm_mon = _get_month(timer_cpy);
  __gm_time_tm.tm_yday = _get_days_since_year(timer_cpy);
  __gm_time_tm.tm_mday = _get_day_of_month(timer_cpy);
  __gm_time_tm.tm_wday = _get_day(timer_cpy);
  __gm_time_tm.tm_hour = _get_hours(timer_cpy);
  __gm_time_tm.tm_min = _get_minutes(timer_cpy);
  __gm_time_tm.tm_sec = _get_seconds(timer_cpy);
  __gm_time_tm.tm_isdst = -1; // don't care about Daylight Saving Time

  return &__gm_time_tm;
}
int _get_local_year(time_t);
int _get_local_month(time_t);
int _get_local_day(time_t);
int _get_local_day_of_month(time_t);
int _get_local_days_since_year(time_t);
int _get_local_hours(time_t);
int _get_local_minutes(time_t);
int _get_local_seconds(time_t);
struct tm *localtime(const time_t *timer)
{
  time_t timer_cpy = *timer;

  __gm_time_tm.tm_year = _get_local_year(timer_cpy) - 1900;
  __gm_time_tm.tm_mon = _get_local_month(timer_cpy);
  __gm_time_tm.tm_yday = _get_local_days_since_year(timer_cpy);
  __gm_time_tm.tm_mday = _get_local_day_of_month(timer_cpy);
  __gm_time_tm.tm_wday = _get_local_day(timer_cpy);
  __gm_time_tm.tm_hour = _get_local_hours(timer_cpy);
  __gm_time_tm.tm_min = _get_local_minutes(timer_cpy);
  __gm_time_tm.tm_sec = _get_local_seconds(timer_cpy);
  __gm_time_tm.tm_isdst = -1; // don't care about Daylight Saving Time

  return &__gm_time_tm;
}
int _get_weeks_in_year(int);
int _get_timezone_offset(time_t);
// https://cplusplus.com/reference/ctime/strftime/
size_t strftime(char *ptr, size_t maxsize, const char *format, const struct tm *time_ptr)
{
  size_t fi = 0;
  size_t wi = 0;
  while (format[fi] != '\0' && wi < maxsize)
  {

    if (format[fi] == '%')
    {
      fi++;

      switch (format[fi])
      {
      case '%':
        ptr[wi++] = '%';
        break;

      case 'a':
        memcpy(ptr + wi, week_days[time_ptr->tm_wday], 3);
        wi += 3;
        break;
      case 'A':
        strcpy(ptr + wi, week_days_long[time_ptr->tm_wday]);
        wi += strlen(week_days_long[time_ptr->tm_wday]);
        break;

      case 'b':
      case 'h':
        memcpy(ptr + wi, months[time_ptr->tm_wday], 3);
        wi += 3;
        break;
      case 'B':
        strcpy(ptr + wi, months_long[time_ptr->tm_wday]);
        wi += strlen(months_long[time_ptr->tm_wday]);
        break;

      case 'c':

      case 'C':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_year / 20);
        wi += 2;
        break;

      case 'd':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_mday);
        wi += 2;
        break;

      case 'D':
        sprintf(ptr + wi, "%.2d/%.2d/%.2d", time_ptr->tm_mon + 1, time_ptr->tm_mday, time_ptr->tm_year % 100);
        wi += 2 + 1 + 2 + 1 + 2;
        break;

      case 'e':
        if (time_ptr->tm_mday < 10)
        {
          sprintf(ptr + wi, " %.1d", time_ptr->tm_mday);
        }
        else
        {
          sprintf(ptr + wi, "%.2d", time_ptr->tm_mday);
        }
        wi += 2;
        break;

      case 'F':
        sprintf(ptr + wi, "%.4d-%.2d-%.2d", time_ptr->tm_year, time_ptr->tm_mon + 1, time_ptr->tm_mday);
        wi += 4 + 1 + 2 + 1 + 2;
        break;

        // diff to y,Y ?
      case 'g':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_year % 100);
        wi += 2;
        break;
      case 'G':
        sprintf(ptr + wi, "%.4d", time_ptr->tm_year);
        wi += 4;
        break;

      case 'H':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_hour);
        wi += 2;
        break;

      case 'I':
      {
        const int mod_12 = time_ptr->tm_hour % 12;
        sprintf(ptr + wi, "%.2d", mod_12 == 0 ? 12 : mod_12);
        wi += 2;
      }
      break;

      case 'j':
        sprintf(ptr + wi, "%.3d", time_ptr->tm_yday);
        wi += 3;
        break;

      case 'm':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_mon + 1);
        wi += 2;
        break;

      case 'M':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_min);
        wi += 2;
        break;

      case 'n':
        sprintf(ptr + wi, "\n");
        wi += 1;
        break;

      case 'p':
        if (time_ptr->tm_hour < 12)
        {
          sprintf(ptr + wi, "AM");
        }
        else
        {
          sprintf(ptr + wi, "PM");
        }
        wi += 2;
        break;

      case 'r':
      {
        int mod_12 = time_ptr->tm_hour % 12;
        sprintf(ptr + wi, "%.2d:%.2d:%.2d %s", mod_12 == 0 ? 12 : mod_12, time_ptr->tm_min, time_ptr->tm_sec, time_ptr->tm_hour < 12 ? "am" : "pm");
        wi += 2 + 1 + 2 + 1 + 2 + 1 + 2;
      }
      break;
      case 'R':
        sprintf(ptr + wi, "%.2d:%.2d", time_ptr->tm_hour, time_ptr->tm_min);
        wi += 2 + 1 + 2;
        break;

      case 'S':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_sec);
        wi += 2;
        break;

      case 't':
        sprintf(ptr + wi, "\t");
        wi += 1;
        break;

      case 'T':
        sprintf(ptr + wi, "%.2d:%.2d:%.2d", time_ptr->tm_hour, time_ptr->tm_min, time_ptr->tm_sec);
        wi += 2 + 1 + 2 + 1 + 2;
        break;

      case 'u':
        sprintf(ptr + wi, "%.1d", time_ptr->tm_wday);
        wi += 1;
        break;

      case 'U':
      {
        int last_sun = time_ptr->tm_yday - time_ptr->tm_wday;
        sprintf(ptr + wi, "%.2d", last_sun / 7);
        wi += 2;
      }
      break;

      case 'V':
      {
        int last_sun = 10 + time_ptr->tm_yday - time_ptr->tm_wday;
        int w = last_sun / 7;
        int woy;
        if (w < 1)
        {
          woy = _get_weeks_in_year(time_ptr->tm_year - 1);
        }
        else if (w > _get_weeks_in_year(time_ptr->tm_year))
        {
          woy = 1;
        }
        else
        {
          woy = w;
        }
        sprintf(ptr + wi, "%.2d", woy);
        wi += 2;
      }
      break;

      case 'w':
        sprintf(ptr + wi, "%.1d", time_ptr->tm_wday);
        wi += 1;
        break;

      case 'W':
      {
        int last_mon = time_ptr->tm_yday - (time_ptr->tm_wday == 1 ? 6 : time_ptr->tm_wday - 1);
        sprintf(ptr + wi, "%.2d", last_mon / 7);
        wi += 2;
      }
      break;

      case 'x':
        sprintf(ptr + wi, "%.2d/%.2d/%.2d", time_ptr->tm_mon + 1, time_ptr->tm_mday, time_ptr->tm_year % 100);
        wi += 2 + 1 + 2 + 1 + 2;
        break;
      case 'X':
        sprintf(ptr + wi, "%.2d:%.2d:%.2d", time_ptr->tm_hour, time_ptr->tm_min, time_ptr->tm_sec);
        wi += 2 + 1 + 2 + 1 + 2;
        break;

      case 'y':
        sprintf(ptr + wi, "%.2d", time_ptr->tm_year % 100);
        wi += 2;
        break;
      case 'Y':
        sprintf(ptr + wi, "%.4d", time_ptr->tm_year);
        wi += 4;
        break;

      case 'z':
      {
        tm tmp;
        memcpy(&tmp, time_ptr, sizeof(tm));
        const int offset = _get_timezone_offset(mktime(&tmp)); // - if ahead + if behind
        const int min = (offset < 0 ? -offset : offset) % 60;
        const int hours = (offset < 0 ? -offset : offset) / 60;
        sprintf(ptr + wi, "%c%.d", (offset < 0 ? '+' : '-'), (hours * 100) + min);
        wi += 1 + (int)log10(hours) + (int)log10(min);
      }
      break;

      case 'Z':
        // No support for printing time zone abbreviation
        wi += 1;
        break;

      case 'E':
      case 'O':
        wi += 2;
        // No support for local alternative representation
        break;

      default:
        break;
      }
    }
    else
    {
      ptr[wi++] = format[fi];
    }
  }

  if (wi < maxsize)
  {
    ptr[wi] = '\0';
    return wi;
  }
  else
  {
    return 0;
  }
}

#endif

#endif