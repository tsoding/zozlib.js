#ifndef INC_STDIO
#define INC_STDIO

#ifndef NO_STDIO_INCLUDE

// IMPORTED FNS
#define SIMPLE_PRINT
void _print_string(const char *);

// DEFINES
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#define BUFSIZ 512
#define EOF (-1)
#define FILENAME_MAX 128
#define FOPEN_MAX 16
#define L_tmpnam 0
#define TMP_MAX 0

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2

// DECLARATIONS
typedef struct FILE FILE;
typedef size_t fpos_t;

// STRING PRINT
#define STB_SPRINTF_DECORATE(name) name
#include "stb_sprintf.h"
#undef STB_SPRINTF_DECORATE

// PRINT
int fprintf(FILE *, const char *, ...);
int printf(const char *, ...);
int vfprintf(FILE *, const char *, va_list);
int vprintf(const char *, va_list);

// SCAN
int fscanf(FILE *, const char *, ...);
int scanf(const char *, ...);
int vfscanf(FILE *, const char *, va_list);
int vscanf(const char *, va_list);
int sscanf(const char *, const char *, ...);
int vsscanf(const char *, const char *, va_list);

// OPEN/CLOSE
int remove(const char *filename);
int rename(const char *oldname, const char *newname);
FILE *tmpfile(void);
char *tmpnam(char *);
FILE *fopen(const char *name, const char *mode);
FILE *freopen(const char *name, const char *mode, FILE *stream);
int fclose(FILE *stream);

// BUFFER FNS
int fflush(FILE *stream);
void setbuf(FILE *stream, char *buffer);
void setvbuf(FILE *stream, char *buffer, int mode, size_t size);

// CHARACTER INPUT/OUTPUT
int fgetc(FILE *stream);
char *fgets(char *str, int max_size, FILE *stream);
int fputc(int character, FILE *stream);
int fputs(const char *str, FILE *stream);
int getc(FILE *stream);
int getchar();
char *gets(char *str);
int putc(int character, FILE *stream);
int putchar(int character);
int puts(const char *str, FILE *stream);
int ungetc(int character, FILE *stream);

// DIRECT INPUT/OUTPUT
size_t fread(void *buffer, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream);

// FILE POSITIONING
int fgetpos(FILE *stream, fpos_t *pos);
int fseek(FILE *stream, long int offset, int origin);
int fsetpos(FILE *stream, const fpos_t *pos);
long int ftell(FILE *stream);
void rewind(FILE *stream);

// ERROR HANDLING
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *buf);

#endif

#define STDIO_IMPL
#ifdef STDIO_IMPL

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// TYPES

// if file is not stdout or stderr buf is assumed to contain the whole file
struct FILE
{
  bool used;
  bool error;
  bool eof;

  bool ready; // true: files was fetched or is std stream
  bool is_writable;
  bool is_readable;

  size_t buf_index;
  char *buf;
  size_t buf_size;
  unsigned char mode;
  bool own_buf;
};

bool __files_were_init = false;
FILE __files[FOPEN_MAX];

// STD FILES
char __stderr_buf[BUFSIZ + 1];
FILE __stderr_hold = {
    .used = true,
    .error = false,
    .eof = false,
    .ready = true,
    .is_writable = true,
    .is_readable = false,
    .buf_index = 0,
    .buf = __stderr_buf,
    .buf_size = BUFSIZ,
    .mode = _IOFBF,
    .own_buf = false,
};
FILE *stderr = &__stderr_hold;

char __stdout_buf[BUFSIZ + 1];
FILE __stdout_hold = {
    .used = true,
    .error = false,
    .eof = false,
    .ready = true,
    .is_writable = true,
    .is_readable = false,
    .buf_index = 0,
    .buf = __stdout_buf,
    .buf_size = BUFSIZ,
    .mode = _IOFBF,
    .own_buf = false,
};
FILE *stdout = &__stdout_hold;

char __stdin_buf[BUFSIZ + 1];
FILE __stdin_hold = {
    .used = true,
    .error = true,
    .eof = true,
    .ready = false,
    .is_writable = false,
    .is_readable = false,
    .buf_index = 0,
    .buf = NULL,
    .buf_size = BUFSIZ,
    .mode = _IOFBF,
    .own_buf = false,
};
FILE *stdin = &__stdin_hold;

// BASE FNS
FILE *_init_file(FILE *init)
{
  init->used = true;
  init->error = false;
  init->eof = false;

  init->ready = false;
  init->is_writable = false;
  init->is_readable = false;

  init->buf_index = 0;
  init->buf = (char *)malloc(BUFSIZ + 1);
  init->buf_size = BUFSIZ;
  init->mode = _IOFBF;
  init->own_buf = true;

  return init;
}
FILE *_create_file()
{
  if (!__files_were_init)
  {
    for (size_t i = 0; i < FOPEN_MAX; i++)
    {
      __files[i].used = false;
    }
    __files_were_init = true;
  }

  // Find first free file
  size_t found_free = 0;
  while (__files[found_free].used && found_free < FOPEN_MAX)
  {
    found_free++;
  }

  if (found_free >= FOPEN_MAX)
  {
    // Reached max possible file count
    return NULL;
  }

  return _init_file(__files + found_free);
}
void _set_file_ready(FILE *stream)
{
  stream->ready = true;
}

// STRING PRINT
#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_DECORATE(name) name
#include "stb_sprintf.h"

// PRINT
char __print_buf[BUFSIZ * 8]; // good enough for everyone ?
int fprintf(FILE *stream, const char *format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  int res = vsprintf(__print_buf, format, argptr);
  va_end(argptr);

  fputs(__print_buf, stream);

  return res;
}
int printf(const char *format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  int res = vprintf(format, argptr);
  va_end(argptr);

  return res;
}
int vfprintf(FILE *stream, const char *format, va_list list)
{
  int res = vsprintf(__print_buf, format, list);
  va_end(list); // ?

  fputs(__print_buf, stream);

  return res;
}
int vprintf(const char *format, va_list list)
{
  return vfprintf(stdout, format, list);
}

// SCAN
int fscanf(FILE *stream, const char *format, ...)
{
  printf("scanf is not implemented");
  return -1;
}
int scanf(const char *format, ...)
{
  printf("scanf is not implemented");
  return -1;
}
int vfscanf(FILE *stream, const char *format, va_list args)
{
  printf("scanf is not implemented");
  return -1;
}
int vscanf(const char *format, va_list args)
{
  printf("scanf is not implemented");
  return -1;
}
int sscanf(const char *src, const char *format, ...)
{
  printf("scanf is not implemented");
  return -1;
}
int vsscanf(const char *src, const char *format, va_list args)
{
  printf("scanf is not implemented");
  return -1;
}

// OPEN/CLOSE
int remove(const char *filename)
{
  printf("ERROR: Cannot Remove files on server");
  return 0;
}
int rename(const char *oldname, const char *newname)
{
  printf("ERROR: Cannot Rename files on server");
  return 0;
}
FILE *tmpfile(void)
{
  printf("ERROR: Cannot Create a files on server");
  return 0;
}
char *tmpnam(char *filename_buf)
{
  printf("ERROR: Creating temporary filename is not implemented");
  return NULL;
}
size_t open_files_count = 0;
FILE *_append_fetch_promise(const char *name, FILE *optional_file);
FILE *fopen(const char *name, const char *mode)
{
  if (open_files_count >= FOPEN_MAX)
  {
    // MAX FILES OPEN NUMBER REACHED
    return NULL;
  }
  else if (strcmp(mode, "rb") != 0)
  {
    printf("ERROR: Only supports mode 'rb'");
    return NULL;
  }

  FILE *f = _append_fetch_promise(name, NULL);
  f->is_readable = true;
  return f;
}
FILE *freopen(const char *name, const char *mode, FILE *stream)
{

  if (strcmp(mode, "rb") != 0)
  {
    printf("ERROR: Only supports mode 'rb'");
    return NULL;
  }

  _init_file(stream);

  FILE *f = _append_fetch_promise(name, stream);
  f->is_readable = true;
  return f;
}
int fclose(FILE *stream)
{

  fflush(stream);
  stream->used = false;
  stream->ready = false;

  if (stream->own_buf)
  {
    free(stream->buf);
  }

  if (stream == stderr)
  {
    stderr = NULL;
  }
  else if (stream == stdout)
  {
    stdout = NULL;
  }
  else if (stream == stdin)
  {
    stdin = NULL;
  }

  return 0;
}

// BUFFER FNS
int fflush(FILE *stream)
{

  if (stream == NULL)
  {
    // FLUSH ALL streams with output
    fflush(stderr);
    fflush(stdout);

    // flush others
  }
  else
  {
    stream->buf[stream->buf_index] = '\0';
    _print_string(stream->buf);
    stream->buf_index = 0;
  }

  return 0;
}
void setbuf(FILE *stream, char *buffer)
{

  free(stream->buf);
  stream->buf_size = BUFSIZ;
  stream->buf = buffer;
  stream->own_buf = false;
}
void setvbuf(FILE *stream, char *buffer, int mode, size_t size)
{

  if (stream->own_buf)
  {
    free(stream->buf);
  }

  switch (mode)
  {
  case _IOFBF:
  case _IOLBF:
  case _IONBF:
    stream->mode = mode;
    break;

  default:
    assert(0 && "Unknown mode given");
    return;
  }

  if (buffer == NULL)
  {
    if (size <= BUFSIZ)
    {
      if (stream == stderr)
      {
        stream->buf = __stderr_buf;
        stream->own_buf = false;
      }
      else if (stream == stdout)
      {
        stream->buf = __stdout_buf;
        stream->own_buf = false;
      }
      else if (stream == stdin)
      {
        stream->buf = __stdin_buf;
        stream->own_buf = false;
      }
      else
      {
        stream->buf = (char *)malloc(size + 1);
        stream->own_buf = true;
      }
    }
    else
    {
      stream->buf = (char *)malloc(size + 1);
      stream->own_buf = true;
    }
  }
  else
  {
    stream->buf = buffer;
    stream->own_buf = false;
  }
  stream->buf_size = size;
}

// CHARACTER INPUT/OUTPUT
int fgetc(FILE *stream)
{

  if (stream == stderr || stream == stdout)
  {
    // Cannot read from stderr or stdout
    stream->eof = true;
    return EOF;
  }
  else if (stream == stdin)
  {
    // How read from stdin?
    return EOF;
  }
  else if (stream->is_readable == true)
  {
    if (stream->buf_index < stream->buf_size)
    {
      return stream->buf[stream->buf_index++];
    }
    else
    {
      stream->eof = true;
      return EOF;
    }
  }

  return EOF;
}
char *fgets(char *str, int max_size, FILE *stream)
{
  if (max_size > 0)
  {
    size_t i = 0;
    int c = fgetc(stream);
    while (c != EOF && c != '\n' && i < max_size)
    {
      str[i++] = c;
      c = fgetc(stream);
    }
    str[i] = '\0';
  }

  return str;
}
int fputc(int character, FILE *stream)
{

  if (stream->is_writable)
  {
    if (stream->buf_index >= stream->buf_size)
    {
      fflush(stream);
    }

    stream->buf[stream->buf_index++] = (char)character;

    if (character == '\n')
    {
      fflush(stream);
    }

    return character;
  }
  else
  {
    printf("ERROR: Cannot put char in read only stream");
    return EOF;
  }
}
int fputs(const char *str, FILE *stream)
{
  const size_t len_str = strlen(str);

  for (size_t i = 0; i < len_str; i++)
  {
    if (fputc(str[i], stream) == EOF)
    {
      return EOF;
    }
  }

  return 0;
}
int getc(FILE *stream)
{
  return fgetc(stream);
}
int getchar()
{
  return getc(stdin);
}
char *gets(char *str)
{
  return fgets(str, INT_MAX, stdin);
}
int putc(int character, FILE *stream)
{
  return fputc(character, stream);
}
int putchar(int character)
{
  return fputc(character, stdout);
}
int puts(const char *str, FILE *stream)
{
  return fputs(str, stream);
}
int ungetc(int character, FILE *stream)
{
  printf("ERROR: ungetc not supported");
  return EOF;
}

// DIRECT INPUT/OUTPUT
size_t fread(void *buffer, size_t size, size_t count, FILE *stream)
{
  unsigned char *buf = (unsigned char *)buffer;

  if (size == 0 || count == 0)
    return 0;

  for (size_t i = 0; i < count; i++)
  {
    for (size_t j = 0; i < size; j++)
    {
      int res = fgetc(stream);

      if (res == EOF)
        return i;

      buf[i + j] = (unsigned char)res;
    }
  }

  return count;
}
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
  unsigned char *buf = (unsigned char *)buffer;

  if (size == 0 || count == 0)
    return 0;

  for (size_t i = 0; i < count; i++)
  {
    for (size_t j = 0; i < size; j++)
    {
      int res = fputc((int)buf[i + j], stream);

      if (res == EOF)
        return i;
    }
  }

  return count;
}

// FILE POSITIONING
int fgetpos(FILE *stream, fpos_t *pos)
{

  if (stream->is_readable)
  {
    const long res = ftell(stream);
    if (res >= 0)
    {
      *pos = res;
      return 0;
    }
    return res;
  }
  else
  {
    printf("ERROR: fgetpos is not implemented for writable files");
    return -1;
  }
}
int fseek(FILE *stream, long int offset, int origin)
{

  if (stream->is_readable)
  {
    switch (origin)
    {
    case SEEK_CUR:
    {
      if (stream->buf_index + offset > 0 && stream->buf_index + offset < stream->buf_size)
      {
        stream->buf_index += offset;
      }
      else
      {
        return EOF;
      }
    }
    break;

    case SEEK_SET:
    {
      if (offset > 0 && offset < stream->buf_size)
      {
        stream->buf_index = offset;
      }
      else
      {
        return EOF;
      }
    }
    break;

    case SEEK_END:
      return EOF;

    default:
      break;
    }
    return 0;
  }
  else
  {
    printf("ERROR: fseek is not implemented for writable files");
    return -1L;
  }
}
int fsetpos(FILE *stream, const fpos_t *pos)
{

  if (stream->is_readable)
  {
    return fseek(stream, *pos, SEEK_SET);
  }
  else
  {
    printf("ERROR: fsetpos is not implemented for writable files");
    return -1;
  }
}
long int ftell(FILE *stream)
{

  if (stream->is_readable)
  {
    return stream->buf_index;
  }
  else
  {
    printf("ERROR: ftell is not implemented for writable files");
    return -1L;
  }
}
void rewind(FILE *stream)
{

  stream->eof = false;
  stream->error = false;

  if (stream->is_readable)
  {
    stream->buf_index = 0;
  }
  else
  {
    printf("ERROR: rewind is not implemented for writable files");
  }
}

// ERROR HANDLING
void clearerr(FILE *stream)
{
  stream->eof = false;
  stream->error = false;
}
int feof(FILE *stream)
{
  return stream->eof;
}
int ferror(FILE *stream)
{
  return stream->error;
}
void perror(const char *buf)
{
  if (buf != NULL)
  {
    fprintf(stderr, "%s: ", buf);
  }
  fprintf(stderr, "%s\n", strerror(errno));
}

#endif
#endif