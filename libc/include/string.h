#ifndef _INC_STRING
#define _INC_STRING

#include <errno.h>
#include <stdlib.h>

#ifndef NO_STRING_INCLUDE

// DEFINES
#include <stddef.h>

#ifdef SIMPLE_PRINT
void _print_string(const char *);
#endif

// INCLUDES

// MEMORY
void *memchr(const void *ptr, int value, size_t num);
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void *memcpy(void *dest, const void *src, size_t size);
void *memmove(void *dest, const void *src, size_t num);
void *memset(void *ptr, int value, size_t num);

// STRING
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *strchr(const char *str, int character);
int strcmp(const char *str1, const char *str2);
int strcoll(const char *str1, const char *str2);
size_t strcspn(const char *str1, const char *str2);
char *strerror(int errnum);
char *strncat(char *dest, const char *src, size_t num);
int strncmp(const char *str1, const char *str2, size_t num);
char *strncpy(char *dest, const char *src, size_t num);
char *strpbrk(const char *str1, const char *str2);
char *strrchr(const char *str, int character);
size_t strspn(const char *str1, const char *str2);
char *strstr(const char *str1, const char *str2);
char *strtok(char *str, const char *delimiters);
size_t strxfrm(char *dest, const char *src, size_t num);

#endif

// IMPLEMENTATIONS
#ifdef STRING_IMPL

// MEMORY
void *memchr(const void *ptr, int value, size_t num)
{
  unsigned char *dest_cast = (unsigned char *)ptr;
  for (size_t i = 0; i < num; i++)
  {
    if (dest_cast[i] == (unsigned char)value)
    {
      return (unsigned char *)ptr + i;
    }
  }

  return (void *)0;
}
int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
  unsigned char *ptr1_cast = (unsigned char *)ptr1;
  unsigned char *ptr2_cast = (unsigned char *)ptr2;
  for (size_t i = 0; i < num; i++)
  {
    if (ptr1_cast[i] < ptr2_cast[i])
    {
      return -1;
    }
    else if (ptr1_cast[i] > ptr2_cast[i])
    {
      return 1;
    }
  }

  return 0;
}
void *memcpy(void *dest, const void *src, size_t size)
{
  unsigned char div8_leftover = size % sizeof(unsigned long long);
  unsigned long size64 = (size - div8_leftover) / sizeof(unsigned long long);

  unsigned long long *dest_cast_8 = (unsigned long long *)dest;
  unsigned long long *src_cast_8 = (unsigned long long *)src;
  for (size_t i = 0; i < size64; i++)
  {
    dest_cast_8[i] = src_cast_8[i];
  }

  unsigned char *dest_cast = (unsigned char *)dest;
  unsigned char *src_cast = (unsigned char *)src;
  for (unsigned char i = 0; i < div8_leftover; i++)
  {
    dest_cast[i] = src_cast[i];
  }

  return dest;
}
void *memmove(void *dest, const void *src, size_t num)
{
  unsigned char *src_buff = (unsigned char *)malloc(num);
  memcpy(src_buff, (void *)src, num);

  unsigned char *dest_cast = (unsigned char *)dest;
  for (unsigned char i = 0; i < num; i++)
  {
    dest_cast[i] = src_buff[i];
  }

  free(src_buff);
  return dest;
}
void *memset(void *ptr, int value, size_t num)
{
  unsigned char *dest_cast = (unsigned char *)ptr;
  for (size_t i = 0; i < num; i++)
  {
    dest_cast[i] = (unsigned char)value;
  }

  return ptr;
}

// STRING
size_t strlen(const char *str)
{
  size_t size = 0;
  while (str[size] != '\0')
  {
    size++;
  }
  return size;
}
char *strcpy(char *dest, const char *src)
{
  size_t len_src = strlen(src);
  memcpy(dest, (void *)src, len_src);
  return dest;
}
char *strcat(char *dest, const char *src)
{
  size_t len_dest = strlen(dest);
  strcpy(dest + len_dest, src);
  return dest;
}
char *strchr(const char *str, int character)
{
  size_t len_str = strlen(str);

  if (character == '\0')
    return (char *)str + len_str;

  return (char *)memchr((void *)str, character, len_str);
}
int strcmp(const char *str1, const char *str2)
{
  size_t len_str1 = strlen(str1);
  size_t len_str2 = strlen(str2);

  size_t lower_len = len_str1 < len_str2 ? len_str1 : len_str2;
  return memcmp((void *)str1, (void *)str2, lower_len);
}
int strcoll(const char *str1, const char *str2)
{
// TODO: Implement with c locale
#ifdef SIMPLE_PRINT
  _print_string("strcoll not yet implemented");
#endif
  return 0;
}
size_t strcspn(const char *str1, const char *str2)
{
  size_t len_str2 = strlen(str2);

  size_t index = 0;
  while (str1[index] != 0)
  {
    char st1_char = str1[index];
    for (size_t i = 0; i < len_str2 + 1; i++)
    {
      if (st1_char == str2[i])
      {
        return index;
      }
    }
  }

  return strlen(str1); // unreachable
}
char *strerror(int errnum)
{
  switch (errnum)
  {
  case EAFNOSUPPORT:
    return (char *)"Address family is not supported";
  case EADDRINUSE:
    return (char *)"Address already in use";
  case EADDRNOTAVAIL:
    return (char *)"Address not available";
  case EISCONN:
    return (char *)"Already connected";
  case E2BIG:
    return (char *)"Argument list too long";
  case EDOM:
    return (char *)"Argument out of domain";
  case EFAULT:
    return (char *)"Bad Address";
  case EBADF:
    return (char *)"Bad filter descriptor";
  case EBADMSG:
    return (char *)"Bad message";
  case EPIPE:
    return (char *)"Broken pipe";
  case ECONNABORTED:
    return (char *)"Connection aborted";
  case EALREADY:
    return (char *)"Connection already in progress";
  case ECONNREFUSED:
    return (char *)"Connection refused";
  case ECONNRESET:
    return (char *)"Connection reset";
  case EXDEV:
    return (char *)"Cross device link";
  case EDESTADDRREQ:
    return (char *)"Destination address required";
  case EBUSY:
    return (char *)"Devicec or resource busy";
  case ENOTEMPTY:
    return (char *)"Directory not empty";
  case ENOEXEC:
    return (char *)"Executable format error";
  case EEXIST:
    return (char *)"File already exists";
  case EFBIG:
    return (char *)"File too large";
  case ENAMETOOLONG:
    return (char *)"Filename too long";
  case ENOSYS:
    return (char *)"Function not supported";
  case EHOSTUNREACH:
    return (char *)"Host is unreachable";
  case EIDRM:
    return (char *)"Identifier removed";
  case EILSEQ:
    return (char *)"Illegal byte sequence";
  case ENOTTY:
    return (char *)"Inappropriate IO control operation";
  case EINTR:
    return (char *)"Interrupted";
  case EINVAL:
    return (char *)"Invalid argument";
  case ESPIPE:
    return (char *)"Invalid seek";
  case EIO:
    return (char *)"IO error";
  case EISDIR:
    return (char *)"Is a directory";
  case EMSGSIZE:
    return (char *)"Message size";
  case ENETDOWN:
    return (char *)"Network down";
  case ENETRESET:
    return (char *)"Network reset";
  case ENETUNREACH:
    return (char *)"Network unreachable";
  case ENOBUFS:
    return (char *)"No buffer space";
  case ECHILD:
    return (char *)"No child process";
  case ENOLINK:
    return (char *)"No link";
  // case ENOLOCK:
  //   return (char *)"No lock available";
  case ENOMSG:
    return (char *)"No message";
  case ENODATA:
    return (char *)"No message available";
  case ENOPROTOOPT:
    return (char *)"No protocol option";
  case ENOSPC:
    return (char *)"No space on device";
  case ENOSR:
    return (char *)"No stream resources";
  case ENODEV:
    return (char *)"No such device";
  case ENXIO:
    return (char *)"No such device or address";
  case ENOENT:
    return (char *)"No such file or directory";
  case ESRCH:
    return (char *)"No such proccess";
  case ENOTDIR:
    return (char *)"Not a directory";
  case ENOTSOCK:
    return (char *)"Not a socket";
  case ENOSTR:
    return (char *)"Not a stream";
  case ENOTCONN:
    return (char *)"Not connected";
  case ENOMEM:
    return (char *)"Not enough memory";
  case ENOTSUP:
    return (char *)"Not supported";
  case ECANCELED:
    return (char *)"Operation canceled";
  case EINPROGRESS:
    return (char *)"Operation in progress";
  case EPERM:
    return (char *)"Operation was not permitted";
  case EOPNOTSUPP:
    return (char *)"Operation not supported";
  case EWOULDBLOCK:
    return (char *)"Operation would block";
  case EOWNERDEAD:
    return (char *)"Owner dead";
  case EACCES:
    return (char *)"Permission denied";
  case EPROTO:
    return (char *)"Protocol error";
  case EPROTONOSUPPORT:
    return (char *)"Protocol not supported";
  case EROFS:
    return (char *)"Read only file system";
  case EDEADLK:
    return (char *)"Resource deadlock would occur";
  case EAGAIN:
    return (char *)"Resource unavailable try again";
  case ERANGE:
    return (char *)"Result out of range";
  case ENOTRECOVERABLE:
    return (char *)"State not recoverable";
  case ETIME:
    return (char *)"Stream timeout";
  case ETXTBSY:
    return (char *)"Text file busy";
  case ETIMEDOUT:
    return (char *)"Timed out";
  case EMFILE:
    return (char *)"Too many files open";
  case ENFILE:
    return (char *)"Too many files open in system";
  case EMLINK:
    return (char *)"Too many links";
  case ELOOP:
    return (char *)"Too many symbolic link levels";
  case EOVERFLOW:
    return (char *)"Value too large";
  case EPROTOTYPE:
    return (char *)"Wrong protocol type";

  default:
    return (char *)"UNKNOWN ERROR CODE";
  }
}
char *strncat(char *dest, const char *src, size_t num)
{
  size_t len_dest = strlen(dest);
  size_t len_src = strlen(src);
  if (len_src > num)
  {
    len_src = num;
  }

  memcpy(dest + len_dest, (void *)src, len_src);
  dest[len_src] = '\0';

  return dest;
}
int strncmp(const char *str1, const char *str2, size_t num)
{
  const size_t len_str1 = strlen(str1);
  const size_t len_str2 = strlen(str2);
  size_t len = len_str1 < len_str2 ? len_str1 : len_str2;
  len = len < num ? len : num;

  return memcmp(str1, str2, len);
}
char *strncpy(char *dest, const char *src, size_t num)
{
  const size_t len_src = strlen(src);
  const size_t len = len_src < num ? len_src : num;

  memcpy(dest, src, len);

  if (len_src < num)
  {
    memset(dest + len, 0, num - len_src);
  }

  return dest;
}
char *strpbrk(const char *str1, const char *str2)
{
  const size_t len_str2 = strlen(str2);

  size_t i = 0;
  while (str1[i] != '\0')
  {
    for (size_t j = 0; j < len_str2; j++)
    {
      if (str1[i] == str2[j])
      {
        return (char *)(str1 + i);
      }
    }
    i++;
  }

  return (char *)NULL;
}
char *strrchr(const char *str, int character)
{
  if (character == 0)
  {
    return (char *)(str + strlen(str));
  }
  else
  {
    size_t last_index = 0;
    bool found = false;

    size_t i = 0;
    while (str[i] != '\0')
    {
      if (str[i] == (char)character)
      {
        found = true;
        last_index = i;
      }
      i++;
    }

    if (found)
    {
      return (char *)(str + last_index);
    }
  }

  return (char *)NULL;
}
size_t strspn(const char *str1, const char *str2)
{
  const size_t len_str2 = strlen(str2);

  size_t i = 0;
  while (str1[i] != '\0')
  {
    bool not_found = true;
    for (size_t j = 0; j < len_str2; j++)
    {
      if (str1[i] == str2[j])
      {
        not_found = false;
        break;
      }
    }

    if (not_found)
    {
      return i;
    }
    i++;
  }

  return i;
}
char *strstr(const char *str1, const char *str2)
{
  if (str2[0] == '\0')
  {
    return (char *)str1;
  }

  const size_t len_str1 = strlen(str1);
  const size_t len_str2 = strlen(str2);

  if (len_str2 > len_str1)
  {
    return (char *)NULL;
  }

  for (size_t i = 0; i < len_str1 + 1 - len_str2; i++)
  {
    bool found = true;
    for (size_t j = 0; j < len_str2; j++)
    {
      if (str1[i + j] != str2[j])
      {
        found = false;
      }
    }

    if (found)
    {
      return (char *)(str1 + i);
    }
  }

  return (char *)NULL;
}
char *strtok(char *str, const char *delimiters)
{
  static char *token_string_start = (char *)NULL;
  static bool end_found = true;

  if (end_found)
  {
    return (char *)NULL;
  }

  if (str != (char *)NULL)
  {
    token_string_start = str;
    end_found = false;
  }

  size_t span_with_delimiters = strspn(token_string_start, delimiters);
  token_string_start += span_with_delimiters;

  char *first_occurrence = strpbrk(token_string_start, delimiters);

  if (first_occurrence != (char *)NULL)
  {
    first_occurrence[0] = '\0';
    return token_string_start;
  }
  else
  {
    token_string_start = (char *)NULL;
    end_found = true;
    return (char *)NULL;
  }
}
size_t strxfrm(char *dest, const char *src, size_t num)
{
  if (num != 0 && dest != (char *)NULL)
  {
    // Transform src according to c locale (no mention how)
    strncpy(dest, src, num);
  }
  return strlen(src);
}
#endif

#endif