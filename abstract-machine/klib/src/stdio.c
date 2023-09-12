#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int sprintf_string(char* buffer, const char* str)
{
    int written = 0;
    while (*str)
    {
        buffer[written++] = *str++;
    }
    return written;
}

int sprintf_integer(char* buffer, int num)
{
    int written = 0;
    if (num < 0)
    {
        buffer[written++] = '-';
        num = -num;
    }

    // Convert the integer to string
    int temp = num;
    int digit_count = 1;
    while (temp /= 10)
    {
        digit_count *= 10;
    }

    while (digit_count > 0)
    {
        int digit = num / digit_count;
        buffer[written++] = '0' + digit;
        num %= digit_count;
        digit_count /= 10;
    }

    return written;
}


int printf_integer(int num)
{
    int written = 0;
    if (num < 0)
    {
        putch('-');
        written++;
        num = -num;
    }
    // Convert the integer to string
    int temp = num;
    int digit_count = 1;
    while (temp /= 10)
    {
        digit_count *= 10;
    }

    while (digit_count > 0)
    {
        int digit = num / digit_count;
        putch('0' + digit);
        num %= digit_count;
        digit_count /= 10;
    }
    return written;
}

int printf_lx(uint64_t num)
{
    int written = 0;
    // Convert the num to string
    uint64_t temp = num;
    uint64_t digit_count = 1;
    while (temp /= 16)
    {
        digit_count *= 16;
    }

    while (digit_count > 0)
    {
        int digit = num / digit_count;
        if(digit >= 0 && digit <= 9){
            putch('0' + digit);
        } else{
            putch('A' + digit - 10);
        }
        num %= digit_count;
        digit_count /= 16;
    }
    return written;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  int written = 0;
  char ch;

  while ((ch = *fmt++))
  {
      if (ch == '%')
      {
          ch = *fmt++;
          switch (ch)
          {
              case 's':
              {
                  const char* str = va_arg(ap, char*);
                  written += sprintf_string(out, str);
                  break;
              }
              case 'd':
              {
                  int num = va_arg(ap, int);
                  written += sprintf_integer(out+written, num);
                  break;
              }
              default:
                  // Handle unsupported fmt specifier
                  // TODO();
                  break;
          }
      }
      else
      {
          out[written++] = ch;
      }
  }

  out[written] = '\0';
  return written;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args,fmt);
  int written = vsprintf(out,fmt,args);
  va_end(args);
  return written;
}


int printf(const char *fmt, ...) {
    va_list args;
    va_start(args,fmt);
    int written = 0;
    char ch;

    while ((ch = *fmt++))
    {
        if (ch == '%')
        {
            ch = *fmt++;
            switch (ch)
            {
                case 's':
                {
                    const char* str = va_arg(args, char*);
                    while(*str){
                        putch(*str++);
                        written++;
                    }
                    break;
                }
                case 'd':
                {
                    int num = va_arg(args, int);
                    written += printf_integer(num);
                    break;
                }
                case 'c':
                {
                    int _ch = va_arg(args,int);
                    putch(_ch);
                    written++;
                    break;
                }
                case 'x':
                {
                    uint32_t _x = va_arg(args,uint32_t);
                    written +=printf_lx(_x);
                    break;
                }
                case 'l':
                {
                    ch = *fmt++;
                    if(ch == 'x'){
                        uint64_t _lx = va_arg(args,uint64_t);
                        written += printf_lx(_lx);
                        break;
                    }
                }

                default:
                    // Handle unsupported fmt specifier
                    // TODO();
                    break;
            }
        }
        else
        {
            putch(ch);
            written++;
        }
    }
    putch('\0');
    return written;
}

// remember output n-1 characters, and lastly output '\0'
int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}


#endif
